/*
 * Copyright (c) 2021 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include <unistd.h>

#include "unisetstd.h"
#include <Poco/Net/NetException.h>
#include <Poco/Net/WebSocket.h>
#include "ujson.h"
#include "LogDB.h"
#include "Configuration.h"
#include "Exceptions.h"
#include "Debug.h"
#include "UniXML.h"
#include "LogDBSugar.h"
// --------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// --------------------------------------------------------------------------
LogDB::LogDB( const string& name, int argc, const char* const* argv, const string& prefix ):
    myname(name)
{
    dblog = make_shared<DebugStream>();

    std::string specconfig;

    int i = uniset3::findArgParam("--" + prefix + "single-confile", argc, argv);

    if( i != -1 )
        specconfig = uniset3::getArgParam("--" + prefix + "single-confile", argc, argv, "");

    std::shared_ptr<UniXML> xml;

    if( specconfig.empty() )
    {
        cout << myname << "(init): init from uniset configure..." << endl;
        uniset_init(argc, argv, "configure.xml");
        auto conf = uniset_conf();

        xml = conf->getConfXML();
    }
    else
    {
        cout << myname << "(init): init from single-confile " << specconfig << endl;
        xml = make_shared<UniXML>();

        try
        {
            xml->open(specconfig);
        }
        catch( std::exception& ex )
        {
            throw ex;
        }
    }

    xmlNode* cnode = xml->findNode(xml->getFirstNode(), "LogDB", name);

    if( !cnode )
    {
        ostringstream err;
        err << name << "(init): Not found confnode <LogDB name='" << name << "'...>";
        dbcrit << err.str() << endl;
        throw uniset3::SystemError(err.str());
    }

    UniXML::iterator it(cnode);

    if( specconfig.empty() )
        uniset_conf()->initLogStream(dblog, prefix + "log" );
    else
    {
        // инициализируем сами, т.к. conf нету..
        const std::string loglevels = uniset3::getArg2Param("--" + prefix + "log-add-levels", argc, argv, it.getProp("log"), "");

        if( !loglevels.empty() )
            dblog->level(Debug::value(loglevels));
    }


    qbufSize = uniset3::getArgPInt("--" + prefix + "db-buffer-size", argc, argv, it.getProp("dbBufferSize"), qbufSize);
    maxdbRecords = uniset3::getArgPInt("--" + prefix + "db-max-records", argc, argv, it.getProp("dbMaxRecords"), qbufSize);

    string tformat = uniset3::getArg2Param("--" + prefix + "db-timestamp-format", argc, argv, it.getProp("dbTeimastampFormat"), "localtime");

    if( tformat == "localtime" || tformat == "utc" )
        tmsFormat = tformat;

    double checkConnection_sec = atof( uniset3::getArg2Param("--" + prefix + "ls-check-connection-sec", argc, argv, it.getProp("lsCheckConnectionSec"), "5").c_str());

    std::string s_overflow = uniset3::getArg2Param("--" + prefix + "db-overflow-factor", argc, argv, it.getProp("dbOverflowFactor"), "1.3");
    float ovf = atof(s_overflow.c_str());

    numOverflow = lroundf( (float)maxdbRecords * ovf );

    if( numOverflow == 0 )
        numOverflow = maxdbRecords;

    dbinfo << myname << "(init) maxdbRecords=" << maxdbRecords << " numOverflow=" << numOverflow << endl;

    flushBufferTimer.set<LogDB, &LogDB::onCheckBuffer>(this);
    wsactivate.set<LogDB, &LogDB::onActivate>(this);
    logEventSig.set<LogDB, &LogDB::onLogEvent>(this);
    sigTERM.set<LogDB, &LogDB::onTerminate>(this);
    sigQUIT.set<LogDB, &LogDB::onTerminate>(this);
    sigINT.set<LogDB, &LogDB::onTerminate>(this);

    bool dbDisabled = ( uniset3::findArgParam("--" + prefix + "db-disable", argc, argv) != -1 );

    if( dbDisabled )
        dbinfo << myname << "(init): save to database DISABLED.." << endl;

    UniXML::iterator sit(cnode);

    if( !sit.goChildren() )
    {
        ostringstream err;
        err << name << "(init): Not found confnode list of log servers for <LogDB name='" << name << "'...>";
        dbcrit << err.str() << endl;
        throw uniset3::SystemError(err.str());
    }

    for( ; sit.getCurrent(); sit++ )
    {
        auto l = make_shared<Log>(sit.getProp("ip"), sit.getIntProp("port"));

        l->name = sit.getProp("name");
        l->cmd = sit.getProp("cmd");
        l->description = sit.getProp("description");

        l->setCheckConnectionTime(checkConnection_sec);

        if( l->name.empty()  )
        {
            ostringstream err;
            err << name << "(init): Unknown name for logserver..";
            dbcrit << err.str() << endl;
            throw uniset3::SystemError(err.str());
        }

        if( l->ip.empty()  )
        {
            ostringstream err;
            err << name << "(init): Unknown 'ip' for '" << l->name << "'..";
            dbcrit << err.str() << endl;
            throw uniset3::SystemError(err.str());
        }

        if( l->port == 0 )
        {
            ostringstream err;
            err << name << "(init): Unknown 'port' for '" << l->name << "'..";
            dbcrit << err.str() << endl;
            throw uniset3::SystemError(err.str());
        }

        //      l->tcp = make_shared<UTCPStream>();
        l->dblog = dblog;

        if( !dbDisabled )
            l->signal_on_read().connect(sigc::mem_fun(this, &LogDB::addLog));

        auto lfile = sit.getProp("logfile");

        if( !lfile.empty() )
        {
            l->logfile = make_shared<DebugStream>();
            l->logfile->logFile(lfile, false);
            l->logfile->level(Debug::ANY);
            l->logfile->showDateTime(false);
            l->logfile->showLogType(false);
            l->logfile->disableOnScreen();
            l->signal_on_read().connect(sigc::mem_fun(this, &LogDB::log2File));
        }

        //      l->set(loop);

        logservers.push_back(l);
    }

    if( logservers.empty() )
    {
        ostringstream err;
        err << name << "(init): empty list of log servers for <LogDB name='" << name << "'...>";
        dbcrit << err.str() << endl;
        throw uniset3::SystemError(err.str());
    }


    if( !dbDisabled )
    {
        const std::string dbfile = uniset3::getArgParam("--" + prefix + "dbfile", argc, argv, it.getProp("dbfile"));

        if( dbfile.empty() )
        {
            ostringstream err;
            err << name << "(init): dbfile (sqlite) not defined. Use: <LogDB name='" << name << "' dbfile='..' ...>";
            dbcrit << err.str() << endl;
            throw uniset3::SystemError(err.str());
        }

        db = unisetstd::make_unique<SQLiteInterface>();

        if( !db->connect(dbfile, false, SQLITE_OPEN_FULLMUTEX) )
        {
            ostringstream err;
            err << myname
                << "(init): DB connection error: "
                << db->error();
            dbcrit << err.str() << endl;
            throw uniset3::SystemError(err.str());
        }
    }

    tpool = std::make_shared<Poco::ThreadPool>(1, logservers.size());

#ifndef DISABLE_REST_API
    wsHeartbeatTime_sec = (float)uniset3::getArgPInt("--" + prefix + "ws-heartbeat-time", argc, argv, it.getProp("wsPingTime"), wsHeartbeatTime_sec) / 1000.0;
    wsSendTime_sec = (float)uniset3::getArgPInt("--" + prefix + "ws-send-time", argc, argv, it.getProp("wsSendTime"), wsSendTime_sec) / 1000.0;
    wsMaxSend = uniset3::getArgPInt("--" + prefix + "ws-max-send", argc, argv, it.getProp("wsMaxSend"), wsMaxSend);

    httpHost = uniset3::getArgParam("--" + prefix + "httpserver-host", argc, argv, "localhost");
    httpPort = uniset3::getArgInt("--" + prefix + "httpserver-port", argc, argv, "8080");
    httpCORS_allow = uniset3::getArgParam("--" + prefix + "httpserver-cors-allow", argc, argv, httpCORS_allow);
    httpReplyAddr = uniset3::getArgParam("--" + prefix + "httpserver-reply-addr", argc, argv, "");

    dblog1 << myname << "(init): http server parameters " << httpHost << ":" << httpPort << endl;
    Poco::Net::SocketAddress sa(httpHost, httpPort);

    maxwsocks = uniset3::getArgPInt("--" + prefix + "ws-max", argc, argv, it.getProp("wsMax"), maxwsocks);
    bgColor = uniset3::getArg2Param("--" + prefix + "bg-color", argc, argv, it.getProp("bgColor"), bgColor);
    fgColor = uniset3::getArg2Param("--" + prefix + "fg-color", argc, argv, it.getProp("fgColor"), fgColor);
    fgColorTitle = uniset3::getArg2Param("--" + prefix + "fg-color-title", argc, argv, it.getProp("fgColorTitle"), fgColorTitle);
    bgColorTitle = uniset3::getArg2Param("--" + prefix + "bg-color-title", argc, argv, it.getProp("bgColorTitle"), bgColorTitle);


    try
    {
        Poco::Net::HTTPServerParams* httpParams = new Poco::Net::HTTPServerParams;

        int maxQ = uniset3::getArgPInt("--" + prefix + "httpserver-max-queued", argc, argv, it.getProp("httpMaxQueued"), 100);
        int maxT = uniset3::getArgPInt("--" + prefix + "httpserver-max-threads", argc, argv, it.getProp("httpMaxThreads"), 3);

        httpParams->setMaxQueued(maxQ);
        httpParams->setMaxThreads(maxT);
        httpserv = std::make_shared<Poco::Net::HTTPServer>(new LogDBRequestHandlerFactory(this), Poco::Net::ServerSocket(sa), httpParams );
    }
    catch( std::exception& ex )
    {
        std::stringstream err;
        err << myname << "(init): " << httpHost << ":" << httpPort << " ERROR: " << ex.what();
        throw uniset3::SystemError(err.str());
    }

#endif
}
//--------------------------------------------------------------------------------------------
LogDB::~LogDB()
{
    if( evIsActive() )
        evstop();

#ifndef DISABLE_REST_API

    if( httpserv )
        httpserv->stop();

#endif

    for( auto&& s : logservers )
    {
        try
        {
            s->terminate();
        }
        catch( ... ) {}
    }

    if( db )
        db->close();
}
//--------------------------------------------------------------------------------------------
void LogDB::onTerminate( ev::sig& evsig, int revents )
{
    if( EV_ERROR & revents )
    {
        dbcrit << myname << "(onTerminate): invalid event" << endl;
        return;
    }

    dbinfo << myname << "(onTerminate): terminate..." << endl;

    try
    {
        dblog1 << myname << "(onTerminate): flush buffer..." << endl;
        flushBuffer();
        dblog1 << myname << "(onTerminate): flush buffer [OK]" << endl;
    }
    catch( std::exception& ex )
    {
        dbinfo << myname << "(onTerminate): "  << ex.what() << endl;

    }

    evsig.stop();

    //evsig.loop.break_loop();
#ifndef DISABLE_REST_API

    try
    {
        dblog1 << myname << "(onTerminate): http server stopping..." << endl;
        httpserv->stop();
        dblog1 << myname << "(onTerminate): http server stopped [OK]..." << endl;
    }
    catch( std::exception& ex )
    {
        dbinfo << myname << "(onTerminate): "  << ex.what() << endl;
    }

#endif

    try
    {
        dblog1 << myname << "(onTerminate): event loop stopping..." << endl;
        evstop();
        dblog1 << myname << "(onTerminate): event loop stopped [OK]" << endl;
    }
    catch( std::exception& ex )
    {
        dbinfo << myname << "(onTerminate): "  << ex.what() << endl;
    }

    dblog1 << myname << "(onTerminate): terminating log readers..." << endl;

    for( auto&& s : logservers )
    {
        try
        {
            s->terminate();
        }
        catch( std::exception& ex )
        {
            dbinfo << myname << "(onTerminate): "  << ex.what() << endl;
        }
    }

    dblog1 << myname << "(onTerminate): terminated log readers [OK]" << endl;
}
//--------------------------------------------------------------------------------------------
void LogDB::flushBuffer()
{
    if( !db || qbuf.empty() || !db->isConnection() )
        return;

    // без BEGIN и COMMIT вставка большого количества данных будет тормозить!
    db->query("BEGIN;");

    while( !qbuf.empty() )
    {
        if( !db->insert(qbuf.front()) )
        {
            dbcrit << myname << "(flushBuffer): error: " << db->error()
                   << " lost query: " << qbuf.front() << endl;
        }

        qbuf.pop();
    }

    db->query("COMMIT;");

    if( !db->lastQueryOK() )
    {
        dbcrit << myname << "(flushBuffer): error: " << db->error() << endl;
    }

    // вызываем каждый раз, для отслеживания переполнения..
    rotateDB();
}
//--------------------------------------------------------------------------------------------
void LogDB::rotateDB()
{
    if( !db )
        return;

    // ротация отключена
    if( maxdbRecords == 0 )
        return;

    size_t num = getCountOfRecords();

    if( num <= numOverflow )
        return;

    dblog2 << myname << "(rotateDB): num=" << num << " > " << numOverflow << endl;

    size_t firstOldID = getFirstOfOldRecord(numOverflow);

    DBResult ret = db->query("DELETE FROM logs WHERE id <= " + std::to_string(firstOldID) + ";");

    if( !db->lastQueryOK() )
    {
        dbwarn << myname << "(rotateDB): delete error: " << db->error() << endl;
    }

    ret = db->query("VACUUM;");

    if( !db->lastQueryOK() )
    {
        dbwarn << myname << "(rotateDB): vacuum error: " << db->error() << endl;
    }

    //  dblog3 <<  myname << "(rotateDB): after rotate: " << getCountOfRecords() << " records" << endl;
}
//--------------------------------------------------------------------------------------------
void LogDB::addLog( LogDB::Log* log, const string& txt )
{
    auto tm = uniset3::now_to_timespec();

    ostringstream q;

    q << "INSERT INTO logs(tms,usec,name,text) VALUES("
      << "datetime(" << tm.tv_sec << ",'unixepoch'),'"   //  timestamp
      << tm.tv_nsec << "','"  //  usec
      << log->name << "','"
      << qEscapeString(txt) << "');";

    {
        uniset3::uniset_rwmutex_wrlock lck(qbufMut);
        qbuf.emplace(q.str());
    }

    logEventSig.send();
}
//--------------------------------------------------------------------------------------------
void LogDB::log2File( LogDB::Log* log, const string& txt )
{
    if( !log->logfile || !log->logfile->isOnLogFile() )
        return;

    log->logfile->any() << txt << endl;
}
//--------------------------------------------------------------------------------------------
size_t LogDB::getCountOfRecords( const std::string& logname )
{
    ostringstream q;

    q << "SELECT count(*) FROM logs";

    if( !logname.empty() )
        q << " WHERE name='" << logname << "'";

    DBResult ret = db->query(q.str());

    if( !ret )
        return 0;

    return (size_t) ret.begin().as_int(0);
}
//--------------------------------------------------------------------------------------------
size_t LogDB::getFirstOfOldRecord( size_t maxnum )
{
    ostringstream q;

    q << "SELECT id FROM logs order by id DESC limit " << maxnum << ",1";
    DBResult ret = db->query(q.str());

    if( !ret )
        return 0;

    return (size_t) ret.begin().as_int(0);
}
//--------------------------------------------------------------------------------------------
std::shared_ptr<LogDB> LogDB::init_logdb( int argc, const char* const* argv, const std::string& prefix )
{
    string name = uniset3::getArgParam("--" + prefix + "name", argc, argv, "LogDB");

    if( name.empty() )
    {
        cerr << "(LogDB): Unknown name. Use --" << prefix << "name" << endl;
        return nullptr;
    }

    return make_shared<LogDB>(name, argc, argv, prefix);
}
// -----------------------------------------------------------------------------
void LogDB::help_print()
{
    cout << "Default: prefix='logdb'" << endl;
    cout << "--prefix-single-confile conf.xml     - Отдельный конфигурационный файл (не требующий структуры uniset)" << endl;
    cout << "--prefix-name name                   - Имя. Для поиска настроечной секции в configure.xml" << endl;
    cout << "database: " << endl;
    cout << "--prefix-db-buffer-size sz                  - Размер буфера (до скидывания в БД)." << endl;
    cout << "--prefix-db-max-records sz                  - Максимальное количество записей в БД. При превышении, старые удаляются. 0 - не удалять" << endl;
    cout << "--prefix-db-overflow-factor float           - Коэффициент переполнения, после которого запускается удаление старых записей. По умолчанию: 1.3" << endl;
    cout << "--prefix-db-disable                         - Отключить запись в БД" << endl;
    cout << "--prefix-db-timestamp-format localtime|utc  - Формат времени в ответе на запросы. По умолчанию: localtime" << endl;

    cout << "websockets: " << endl;
    cout << "--prefix-ws-max num                  - Максимальное количество websocket-ов" << endl;
    cout << "--prefix-ws-heartbeat-time msec      - Период сердцебиения в соединении. По умолчанию: 3000 мсек" << endl;
    cout << "--prefix-ws-send-time msec           - Период посылки сообщений. По умолчанию: 500 мсек" << endl;
    cout << "--prefix-ws-max num                  - Максимальное число сообщений посылаемых за один раз. По умолчанию: 200" << endl;
    cout << "--prefix-bg-color clr                - Цвет фона при выводе логов. По умолчанию: #111111" << endl;
    cout << "--prefix-fg-color clr                - Цвет текста при выводе логов. По умолчанию: #c4c4c4" << endl;
    cout << "--prefix-bg-color-title clr          - Цвет фона заголовка окна. По умолчанию: green" << endl;
    cout << "--prefix-fg-color-title clr          - Цвет текста заголовка окна. По умолчанию: #ececec" << endl;

    cout << "logservers: " << endl;
    cout << "--prefix-ls-check-connection-sec sec    - Период проверки соединения с логсервером" << endl;

    cout << "http: " << endl;
    cout << "--prefix-httpserver-host ip                 - IP на котором слушает http сервер. По умолчанию: localhost" << endl;
    cout << "--prefix-httpserver-port num                - Порт на котором принимать запросы. По умолчанию: 8080" << endl;
    cout << "--prefix-httpserver-max-queued num          - Размер очереди запросов к http серверу. По умолчанию: 100" << endl;
    cout << "--prefix-httpserver-max-threads num         - Разрешённое количество потоков для http-сервера. По умолчанию: 3" << endl;
    cout << "--prefix-httpserver-cors-allow addr         - (CORS): Access-Control-Allow-Origin. Default: *" << endl;
    cout << "--prefix-httpserver-reply-addr host[:port]  - Адрес отдаваемый клиенту для подключения. По умолчанию адрес узла где запущен logdb" << endl;
}
// -----------------------------------------------------------------------------
void LogDB::run( bool async )
{
#ifndef DISABLE_REST_API

    if( httpserv )
        httpserv->start();

#endif

    start_threadpool();

    if( async )
        async_evrun();
    else
        evrun();
}
// -----------------------------------------------------------------------------
void LogDB::start_threadpool()
{
    tpoolRet = std::async(std::launch::async, [this]()
    {
        std::vector<LogRunner> lrunners;
        lrunners.reserve(logservers.size());

        for( auto&& s : logservers )
            lrunners.emplace_back(LogRunner(s));

        for( auto&& r : lrunners )
            tpool->start(r);

        tpool->joinAll();
        return true;
    });
}
// -----------------------------------------------------------------------------
void LogDB::evfinish()
{
    flushBufferTimer.stop();
    wsactivate.stop();
    logEventSig.stop();
}
// -----------------------------------------------------------------------------
void LogDB::evprepare()
{
    if( db )
    {
        flushBufferTimer.set(loop);
        flushBufferTimer.start(0, tmFlushBuffer_sec);
    }

    wsactivate.set(loop);
    wsactivate.start();

    logEventSig.set(loop);
    logEventSig.start();

    sigTERM.set(loop);
    sigTERM.start(SIGTERM);
    sigQUIT.set(loop);
    sigQUIT.start(SIGQUIT);
    sigINT.set(loop);
    sigINT.start(SIGINT);
}
// -----------------------------------------------------------------------------
void LogDB::onCheckBuffer(ev::timer& t, int revents)
{
    if (EV_ERROR & revents)
    {
        dbcrit << myname << "(LogDB::onCheckBuffer): invalid event" << endl;
        return;
    }

    uniset3::uniset_rwmutex_rlock lock(qbufMut);

    if( qbuf.size() >= qbufSize )
        flushBuffer();
}
// -----------------------------------------------------------------------------
void LogDB::onLogEvent( ev::async& watcher, int revents )
{
    if (EV_ERROR & revents)
    {
        dbcrit << myname << "(LogDB::onLogEvent): invalid event" << endl;
        return;
    }

    uniset3::uniset_rwmutex_rlock lock(qbufMut);

    if( qbuf.size() >= qbufSize )
        flushBuffer();
}
// -----------------------------------------------------------------------------
void LogDB::onActivate( ev::async& watcher, int revents )
{
    if (EV_ERROR & revents)
    {
        dbcrit << myname << "(LogDB::onActivate): invalid event" << endl;
        return;
    }

#ifndef DISABLE_REST_API
    uniset_rwmutex_rlock lk(wsocksMutex);

    for( const auto& s : wsocks )
    {
        if( !s->isActive() )
            s->set(loop);
    }

#endif
}
// -----------------------------------------------------------------------------
LogDB::LogRunner::LogRunner( std::shared_ptr<LogDB::Log> l ):
    log(l)
{

}
// -----------------------------------------------------------------------------
LogDB::LogRunner::~LogRunner()
{
    if( log )
    {
        log->terminate();
        log = nullptr;
    }
}
// -----------------------------------------------------------------------------
void LogDB::LogRunner::run()
{
    if( log )
        log->run();
}
// -----------------------------------------------------------------------------
bool LogDB::Log::isConnected() const
{
    return reader && reader->isConnection();
}
// -----------------------------------------------------------------------------
LogDB::Log::Log( const std::string& _ip, int _port ):
    ip(_ip),
    port(_port)
{
    reader = make_shared<LogReader>();
    reader->signal_stream_event().connect( sigc::mem_fun(this, &LogDB::Log::onLogEvent) );
    text.reserve(reservsize);
}
// -----------------------------------------------------------------------------
LogDB::Log::~Log()
{
    if( reader )
        reader->terminate();
}
// -----------------------------------------------------------------------------
void LogDB::Log::terminate()
{
    if( reader )
        reader->terminate();
}
// -----------------------------------------------------------------------------
void LogDB::Log::run()
{
    logserver::LogCommandList cmdlist = LogReader::getCommands(cmd);
    reader->readLoop(ip, port, cmdlist);

    if( !text.empty() )
    {
        sigRead.emit(this, text);
        text = "";

        if( text.capacity() < reservsize )
            text.reserve(reservsize);
    }
}
// -----------------------------------------------------------------------------
void LogDB::Log::onLogEvent( const std::string& s )
{
    if( text.capacity() < reservsize )
        text.reserve(reservsize);

    auto pos = s.find_first_of('\n');

    if( pos == string::npos )
    {
        text += s;
        return;
    }

    // Нарезаем на строки (откидывая перевод строки)
    text += s.substr(0, pos);
    sigRead.emit(this, text);

    if( pos + 2 >= s.size() )
    {
        text = "";
        return;
    }

    text = s.substr(pos + 2);
    pos = text.find_first_of('\n');

    while( pos != string::npos )
    {
        string tmp = text.substr(0, pos);
        sigRead.emit(this, tmp);

        if( pos + 2 >= tmp.size() )
        {
            text = "";
            return;
        }

        text = text.substr(pos + 2);
        pos = text.find_first_of('\n');
    }
}
// -----------------------------------------------------------------------------
LogDB::Log::ReadSignal LogDB::Log::signal_on_read()
{
    return sigRead;
}
// -----------------------------------------------------------------------------
void LogDB::Log::setCheckConnectionTime( double sec )
{
    if( sec > 0 )
        checkConnection_sec = sec;
}
// -----------------------------------------------------------------------------
std::string LogDB::qEscapeString( const string& txt )
{
    ostringstream ret;

    for( const auto& c : txt )
    {
        ret << c;

        if( c == '\'' || c == '"' )
            ret << c;
    }

    return ret.str();
}
// -----------------------------------------------------------------------------
#ifndef DISABLE_REST_API
// -----------------------------------------------------------------------------
class LogDBRequestHandler:
    public Poco::Net::HTTPRequestHandler
{
    public:

        LogDBRequestHandler( LogDB* l ): logdb(l) {}

        virtual void handleRequest( Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response ) override
        {
            logdb->handleRequest(request, response);
        }

    private:
        LogDB* logdb;
};
// -----------------------------------------------------------------------------
class LogDBWebSocketRequestHandler:
    public Poco::Net::HTTPRequestHandler
{
    public:

        LogDBWebSocketRequestHandler( LogDB* l ): logdb(l) {}

        virtual void handleRequest( Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response ) override
        {
            logdb->onWebSocketSession(request, response);
        }

    private:
        LogDB* logdb;
};
// -----------------------------------------------------------------------------
Poco::Net::HTTPRequestHandler* LogDB::LogDBRequestHandlerFactory::createRequestHandler( const Poco::Net::HTTPServerRequest& req )
{
    if( req.find("Upgrade") != req.end() && Poco::icompare(req["Upgrade"], "websocket") == 0 )
        return new LogDBWebSocketRequestHandler(logdb);

    return new LogDBRequestHandler(logdb);
}
// -----------------------------------------------------------------------------
void LogDB::handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp )
{
    using Poco::Net::HTTPResponse;

    std::ostream& out = resp.send();

    resp.setContentType("text/json");
    resp.set("Access-Control-Allow-Methods", "GET");
    resp.set("Access-Control-Allow-Request-Method", "*");
    resp.set("Access-Control-Allow-Origin", httpCORS_allow /* req.get("Origin") */);

    try
    {
        // В этой версии API поддерживается только GET
        if( req.getMethod() != "GET" )
        {
            auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, "method must be 'GET'");
            jdata->stringify(out);
            out.flush();
            return;
        }

        Poco::URI uri(req.getURI());

        dblog3 << req.getHost() << ": query: " << uri.getQuery() << endl;

        std::vector<std::string> seg;
        uri.getPathSegments(seg);

        // проверка подключения к страничке со списком websocket-ов
        // http://[xxxx:port]/logdb/ws/
        if( seg.size() > 1 && seg[0] == "logdb" && seg[1] == "ws" )
        {
            // подключение..
            if( seg.size() > 2 )
            {
                httpWebSocketConnectPage(out, req, resp, seg[2]);
                return;
            }

            // default page
            httpWebSocketPage(out, req, resp);
            out.flush();
            return;
        }


        // example: http://host:port/api/version/logdb/..
        if( seg.size() < 4
                || seg[0] != "api"
                || seg[1] != uniset3::UHttp::UHTTP_API_VERSION
                || seg[2].empty()
                || seg[2] != "logdb")
        {
            ostringstream err;
            err << "Bad request structure. Must be /api/" << uniset3::UHttp::UHTTP_API_VERSION << "/logdb/xxx";
            auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, err.str());
            jdata->stringify(out);
            out.flush();
            return;
        }

        if( !db )
        {
            ostringstream err;
            err << "Working with the database is disabled";
            auto jdata = respError(resp, HTTPResponse::HTTP_SERVICE_UNAVAILABLE, err.str());
            jdata->stringify(out);
            out.flush();
            return;
        }

        auto qp = uri.getQueryParameters();

        resp.setStatus(HTTPResponse::HTTP_OK);
        string cmd = seg[3];

        if( cmd == "help" )
        {
            using uniset3::json::help::item;
            uniset3::json::help::object myhelp("help");
            myhelp.emplace(item("help", "this help"));
            myhelp.emplace(item("list", "list of logs"));
            myhelp.emplace(item("count?logname", "count of logs for logname"));

            item l("logs", "read logs");
            l.param("from='YYYY-MM-DD'", "From date");
            l.param("to='YYYY-MM-DD'", "To date");
            l.param("last=XX[m|h|d|M]", "Last records (m - minute, h - hour, d - day, M - month)");
            l.param("offset=N", "offset");
            l.param("limit=M", "limit records for response");
            myhelp.add(l);

            myhelp.emplace(item("apidocs", "https://github.com/Etersoft/uniset3"));
            myhelp.get()->stringify(out);
        }
        else
        {
            auto json = httpGetRequest(cmd, qp);
            json->stringify(out);
        }
    }
    catch( std::exception& ex )
    {
        auto jdata = respError(resp, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, ex.what());
        jdata->stringify(out);
    }

    out.flush();
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::respError( Poco::Net::HTTPServerResponse& resp,
        Poco::Net::HTTPResponse::HTTPStatus estatus,
        const string& message )
{
    resp.setStatus(estatus);
    resp.setContentType("text/json");
    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    jdata->set("error", resp.getReasonForStatus(resp.getStatus()));
    jdata->set("ecode", (int)resp.getStatus());
    jdata->set("message", message);
    return jdata;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetRequest( const string& cmd, const Poco::URI::QueryParameters& p )
{
    if( cmd == "list" )
        return httpGetList(p);

    if( cmd == "logs" )
        return httpGetLogs(p);

    if( cmd == "count" )
        return httpGetCount(p);

    ostringstream err;
    err << "Unknown command '" << cmd << "'";
    throw uniset3::SystemError(err.str());
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetList( const Poco::URI::QueryParameters& p )
{
    if( !db )
    {
        ostringstream err;
        err << "DB unavailable..";
        throw uniset3::SystemError(err.str());
    }

    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();

    Poco::JSON::Array::Ptr jlist = uniset3::json::make_child_array(jdata, "logs");

#if 0
    // Получение из БД
    // хорошо тем, что возвращаем список реально доступных логов (т.е. тех что есть в БД)
    // плохо тем, что если в конфигурации добавили какие-то логи, но в БД
    // ещё ничего не попало, мы их не увидим

    ostringstream q;

    q << "SELECT COUNT(*), name FROM logs GROUP BY name";
    DBResult ret = db->query(q.str());

    if( !ret )
        return jdata;

    for( auto it = ret.begin(); it != ret.end(); ++it )
    {
        Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
        j->set("name", it.as_string("name"));
        jlist->add(j);
    }

#else
    // Получение из конфигурации
    // хорошо тем, что если логов ещё не было
    // то всё-равно видно, какие доступны потенциально
    // плохо тем, что если конфигурацию поменяли (убрали какой-то лог)
    // а в БД записи по нему остались, то мы не получим к ним доступ

    /*! \todo пока список logservers формируется только в начале (в конструкторе)
     * можно не защищаться mutex-ом, т.к. мы его не меняем
     * если вдруг в REST API будет возможность добавлять логи.. нужно защищаться
     * либо переделывать обработку
     */
    for( const auto& s : logservers )
    {
        Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
        j->set("name", s->name);
        j->set("description", s->description);
        jlist->add(j);
    }

#endif

    return jdata;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetLogs( const Poco::URI::QueryParameters& params )
{
    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();

    if( params.empty() || params[0].first.empty() )
    {
        ostringstream err;
        err << "BAD REQUEST: unknown logname";
        throw uniset3::SystemError(err.str());
    }

    std::string logname = params[0].first;

    size_t offset = 0;
    size_t limit = 0;

    vector<std::string> q_where;

    for( const auto& p : params )
    {
        if( p.first == "offset" )
            offset = uni_atoi(p.second);
        else if( p.first == "limit" )
            limit = uni_atoi(p.second);
        else if( p.first == "from" )
            q_where.push_back("tms>='" + qDate(p.second) + " 00:00:00'");
        else if( p.first == "to" )
            q_where.push_back("tms<='" + qDate(p.second) + " 23:59:59'");
        else if( p.first == "last" )
            q_where.push_back(qLast(p.second));
    }

    Poco::JSON::Array::Ptr jlist = uniset3::json::make_child_array(jdata, "logs");

    ostringstream q;

    q << "SELECT tms,"
      << " strftime('%d-%m-%Y',datetime(tms,'" << tmsFormat << "')) as date,"
      << " strftime('%H:%M:%S',datetime(tms,'" << tmsFormat << "')) as time,"
      << " usec, text FROM logs WHERE name='" << logname << "'";

    if( !q_where.empty() )
    {
        for( const auto& w : q_where )
            q << " AND " << w;
    }

    if( limit > 0 )
        q << " ORDER BY tms ASC LIMIT " << offset << "," << limit;

    DBResult ret = db->query(q.str());

    if( !ret )
        return jdata;

    for( auto it = ret.begin(); it != ret.end(); ++it )
    {
        Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
        j->set("tms", it.as_string("tms"));
        j->set("date", it.as_string("date"));
        j->set("time", it.as_string("time"));
        j->set("usec", it.as_string("usec"));
        j->set("text", it.as_string("text"));
        jlist->add(j);
    }

    return jdata;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr LogDB::httpGetCount( const Poco::URI::QueryParameters& params )
{
    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();

    std::string logname;

    if( !params.empty() )
        logname = params[0].first;

    size_t count = getCountOfRecords(logname);
    jdata->set("name", logname);
    jdata->set("count", count);
    return jdata;
}
// -----------------------------------------------------------------------------
string LogDB::qLast( const string& p )
{
    if( p.empty() )
        return "";

    char unit =  p[p.size() - 1];
    std::string sval = p.substr(0, p.size() - 1);

    if( unit == 'h' || unit == 'H' )
    {
        size_t h = uni_atoi(sval);
        ostringstream q;
        q << "tms >= strftime('%s',datetime('now')) - " << h << "*60*60";
        return q.str();
    }
    else if( unit == 'd' || unit == 'D' )
    {
        size_t d = uni_atoi(sval);
        ostringstream q;
        q << "tms >= strftime('%s',datetime('now')) - " << d << "*24*60*60";
        return q.str();
    }
    else if( unit == 'M' )
    {
        size_t m = uni_atoi(sval);
        ostringstream q;
        q << "tms >= strftime('%s',datetime('now')) - " << m << "*30*24*60*60";
        return q.str();
    }
    else // по умолчанию минут
    {
        size_t m = (unit == 'm') ? uni_atoi(sval) : uni_atoi(p);
        ostringstream q;
        q << "tms >= strftime('%s',datetime('now')) - " << m << "*60";
        return q.str();
    }

    return "";
}
// -----------------------------------------------------------------------------
string LogDB::qDate( const string& p, const char sep )
{
    if( p.size() < 8 || p.size() > 10 )
        return ""; // bad format

    // преобразование в дату 'YYYY-MM-DD' из строки 'YYYYMMDD' или 'YYYY/MM/DD'
    if( p.size() == 10 ) // <-- значит у нас длинная строка
    {
        std::string ret(p);
        // независимо от того, правильная она или нет
        // расставляем разделитель
        ret[4] = sep;
        ret[8] = sep;
        return ret;
    }

    return p.substr(0, 4) + "-" + p.substr(4, 2) + "-" + p.substr(6, 2);
}
// -----------------------------------------------------------------------------
void LogDB::onWebSocketSession(Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp)
{
    using Poco::Net::WebSocket;
    using Poco::Net::WebSocketException;
    using Poco::Net::HTTPResponse;
    using Poco::Net::HTTPServerRequest;

    std::vector<std::string> seg;

    Poco::URI uri(req.getURI());

    uri.getPathSegments(seg);

    // example: http://host:port/logdb/ws/logname
    if( seg.size() < 3
            || seg[0] != "logdb"
            || seg[1] != "ws"
            || seg[2].empty())
    {

        resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
        resp.setContentType("text/html");
        resp.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
        resp.setContentLength(0);
        std::ostream& err = resp.send();
        err << "Bad request. Must be:  http://host:port/logdb/ws/logname";
        err.flush();
        return;
    }

    {
        uniset_rwmutex_rlock lk(wsocksMutex);

        if( wsocks.size() >= maxwsocks )
        {
            resp.setStatus(HTTPResponse::HTTP_SERVICE_UNAVAILABLE);
            resp.setContentType("text/html");
            resp.setStatusAndReason(HTTPResponse::HTTP_SERVICE_UNAVAILABLE);
            resp.setContentLength(0);
            std::ostream& err = resp.send();
            err << "Error: exceeding the maximum number of open connections (" << maxwsocks << ")";
            err.flush();
            return;
        }
    }

    auto ws = newWebSocket(&req, &resp, seg[2]);

    if( !ws )
        return;

    LogWebSocketGuard lk(ws, this);

    dblog3 << myname << "(onWebSocketSession): start session for " << req.clientAddress().toString() << endl;

    // т.к. вся работа происходит в eventloop
    // то здесь просто ждём..
    ws->waitCompletion();

    dblog3 << myname << "(onWebSocketSession): finish session for " << req.clientAddress().toString() << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<LogDB::LogWebSocket> LogDB::newWebSocket( Poco::Net::HTTPServerRequest* req,
        Poco::Net::HTTPServerResponse* resp,
        const std::string& logname )
{
    using Poco::Net::WebSocket;
    using Poco::Net::WebSocketException;
    using Poco::Net::HTTPResponse;
    using Poco::Net::HTTPServerRequest;

    std::shared_ptr<Log> log;

    for( const auto& s : logservers )
    {
        if( s->name == logname )
        {
            log = s;
            break;
        }
    }

    if( !log )
    {
        resp->setStatus(HTTPResponse::HTTP_BAD_REQUEST);
        resp->setContentType("text/html");
        resp->setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
        std::ostream& err = resp->send();
        err << "Not found '" << logname << "'";
        err.flush();
        return nullptr;
    }


    std::shared_ptr<LogWebSocket> ws;

    {
        uniset_rwmutex_wrlock lock(wsocksMutex);
        ws = make_shared<LogWebSocket>(req, resp, log);
        ws->setHearbeatTime(wsHeartbeatTime_sec);
        ws->setSendPeriod(wsSendTime_sec);
        ws->setMaxSendCount(wsMaxSend);
        ws->dblog = dblog;
        wsocks.emplace_back(ws);
    }
    // wsocksMutex надо отпустить, прежде чем посылать сигнал
    // т.е. в обработчике происходит его захват
    wsactivate.send();
    return ws;
}
// -----------------------------------------------------------------------------
void LogDB::delWebSocket( std::shared_ptr<LogWebSocket>& ws )
{
    uniset_rwmutex_wrlock lock(wsocksMutex);

    for( auto it = wsocks.begin(); it != wsocks.end(); it++ )
    {
        if( (*it).get() == ws.get() )
        {
            dblog3 << myname << ": delete websocket "
                   << endl;
            wsocks.erase(it);
            return;
        }
    }
}
// -----------------------------------------------------------------------------
LogDB::LogWebSocket::LogWebSocket(Poco::Net::HTTPServerRequest* _req,
                                  Poco::Net::HTTPServerResponse* _resp,
                                  std::shared_ptr<Log>& _log ):
    Poco::Net::WebSocket(*_req, *_resp),
    req(_req),
    resp(_resp)
    //  log(_log)
{
    setBlocking(false);

    cancelled = false;

    con = _log->signal_on_read().connect( sigc::mem_fun(*this, &LogWebSocket::add));

    // т.к. создание websocket-а происходит в другом потоке
    // то активация и привязка к loop происходит в функции set()
    // вызываемой из eventloop
    ioping.set<LogDB::LogWebSocket, &LogDB::LogWebSocket::ping>(this);
    iosend.set<LogDB::LogWebSocket, &LogDB::LogWebSocket::send>(this);

    maxsize = maxsend * 10; // пока так
}
// -----------------------------------------------------------------------------
LogDB::LogWebSocket::~LogWebSocket()
{
    if( !cancelled )
        term();

    // удаляем всё что осталось
    while(!wbuf.empty())
    {
        delete wbuf.front();
        wbuf.pop();
    }
}
// -----------------------------------------------------------------------------
bool LogDB::LogWebSocket::isActive()
{
    return iosend.is_active();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::set( ev::dynamic_loop& loop )
{
    iosend.set(loop);
    ioping.set(loop);

    iosend.start(0, send_sec);
    ioping.start(ping_sec, ping_sec);
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::send( ev::timer& t, int revents )
{
    if( EV_ERROR & revents )
        return;

    for( size_t i = 0; !wbuf.empty() && i < maxsend && !cancelled; i++ )
        write();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::ping( ev::timer& t, int revents )
{
    if( EV_ERROR & revents )
        return;

    if( cancelled )
        return;

    if( !wbuf.empty() )
    {
        ioping.stop();
        return;
    }

    wbuf.emplace(new UTCPCore::Buffer("."));

    if( ioping.is_active() )
        ioping.stop();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::add( LogDB::Log* log, const string& txt )
{
    if( cancelled || txt.empty())
        return;

    if( wbuf.size() > maxsize )
    {
        dbwarn << req->clientAddress().toString() << " lost messages..." << endl;
        return;
    }

    wbuf.emplace(new UTCPCore::Buffer(txt));

    if( ioping.is_active() )
        ioping.stop();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::write()
{
    UTCPCore::Buffer* msg = 0;

    if( wbuf.empty() )
    {
        if( !ioping.is_active() )
            ioping.start(ping_sec, ping_sec);

        return;
    }

    msg = wbuf.front();

    if( !msg )
        return;

    using Poco::Net::WebSocket;
    using Poco::Net::WebSocketException;
    using Poco::Net::HTTPResponse;
    using Poco::Net::HTTPServerRequest;

    int flags = WebSocket::FRAME_TEXT;

    if( msg->len == 1 ) // это пинг состоящий из "."
        flags = WebSocket::FRAME_FLAG_FIN | WebSocket::FRAME_OP_PING;

    try
    {
        ssize_t ret = sendFrame(msg->dpos(), msg->nbytes(), flags);

        if( ret < 0 )
        {
            int errnum = errno;

            dblog3 << "(websocket): " << req->clientAddress().toString()
                   << "  write to socket error(" << errnum << "): " << strerror(errnum) << endl;

            if( errnum == EPIPE || errnum == EBADF )
            {
                dblog3 << "(websocket): "
                       << req->clientAddress().toString()
                       << " write error.. terminate session.." << endl;

                term();
            }

            return;
        }

        msg->pos += ret;

        if( msg->nbytes() == 0 )
        {
            wbuf.pop();
            delete msg;
        }

        if( !wbuf.empty() )
        {
            if( ioping.is_active() )
                ioping.stop();
        }
        else
        {
            if( !ioping.is_active() )
                ioping.start(ping_sec, ping_sec);
        }

        return;
    }
    catch( WebSocketException& exc )
    {
        switch( exc.code() )
        {
            case WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
                resp->set("Sec-WebSocket-Version", WebSocket::WEBSOCKET_VERSION);

            case WebSocket::WS_ERR_NO_HANDSHAKE:
            case WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:
            case WebSocket::WS_ERR_HANDSHAKE_NO_KEY:
                resp->setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
                resp->setContentLength(0);
                resp->send();
                break;
        }
    }
    catch( const Poco::Net::NetException& e )
    {
        dblog3 << "(websocket):NetException: "
               << req->clientAddress().toString()
               << " error: " << e.displayText()
               << endl;
    }
    catch( Poco::IOException& ex )
    {
        dblog3 << "(websocket): IOException: "
               << req->clientAddress().toString()
               << " error: " << ex.displayText()
               << endl;
    }

    term();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::term()
{
    if( cancelled )
        return;

    cancelled = true;
    con.disconnect();
    ioping.stop();
    iosend.stop();
    finish.notify_all();
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::waitCompletion()
{
    std::unique_lock<std::mutex> lk(finishmut);

    while( !cancelled )
        finish.wait(lk);
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::setHearbeatTime( const double& sec )
{
    if( sec > 0 )
        ping_sec = sec;
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::setSendPeriod ( const double& sec )
{
    if( sec > 0 )
        send_sec = sec;
}
// -----------------------------------------------------------------------------
void LogDB::LogWebSocket::setMaxSendCount( size_t val )
{
    if( val > 0 )
        maxsend = val;
}
// -----------------------------------------------------------------------------
void LogDB::httpWebSocketPage( std::ostream& ostr, Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp )
{
    using Poco::Net::HTTPResponse;

    resp.setChunkedTransferEncoding(true);
    resp.setContentType("text/html");

    ostr << "<html>" << endl;
    ostr << "<head>" << endl;
    ostr << "<title>" << myname << ": log servers list</title>" << endl;
    ostr << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">" << endl;
    ostr << "</head>" << endl;
    ostr << "<body>" << endl;
    ostr << "<h1>servers:</h1>" << endl;
    ostr << "<ul>" << endl;

    for( const auto& l : logservers )
    {
        ostr << "  <li><a target='_blank' href=\"http://"
             << ( httpReplyAddr.empty() ? req.serverAddress().toString() : httpReplyAddr )
             << "/logdb/ws/" << l->name << "\">"
             << l->name << "</a>  &#8211; "
             << "<i>" << l->description << "</i></li>"
             << endl;
    }

    ostr << "</ul>" << endl;
    ostr << "</body>" << endl;
}
// -----------------------------------------------------------------------------
void LogDB::httpWebSocketConnectPage( ostream& ostr,
                                      Poco::Net::HTTPServerRequest& req,
                                      Poco::Net::HTTPServerResponse& resp,
                                      const std::string& logname )
{
    resp.setChunkedTransferEncoding(true);
    resp.setContentType("text/html");

    // code base on example from
    // https://github.com/pocoproject/poco/blob/developNet/samples/WebSocketServer/src/WebSocketServer.cpp

    ostr << "<html>" << endl;
    ostr << "<head>" << endl;
    ostr << "<title>" << myname << " log '" << logname << "'</title>" << endl;
    ostr << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">" << endl;
    ostr << "<script type=\"text/javascript\">" << endl;
    ostr << "logscrollStopped = false;" << endl;
    ostr << "" << endl;
    ostr << "function clickScroll()" << endl;
    ostr << "{" << endl;
    ostr << "	if( logscrollStopped )" << endl;
    ostr << "		logscrollStopped = false;" << endl;
    ostr << "	else" << endl;
    ostr << "		logscrollStopped = true;" << endl;
    ostr << "}" << endl;
    ostr << "function LogAutoScroll()" << endl;
    ostr << "{" << endl;
    ostr << "   if( logscrollStopped == false )" << endl;
    ostr << "   {" << endl;
    ostr << "	   document.getElementById('end').scrollIntoView();" << endl;
    ostr << "   }" << endl;
    ostr << "}" << endl;
    ostr << "" << endl;
    ostr << "function WebSocketCreate(logname)" << endl;
    ostr << "{" << endl;
    ostr << "  if (\"WebSocket\" in window)" << endl;
    ostr << "  {" << endl;
    ostr << "    // LogScrollTimer = setInterval(LogAutoScroll,800);" << endl;
    ostr << "    var ws = new WebSocket(\"ws://" << ( httpReplyAddr.empty() ? req.serverAddress().toString() : httpReplyAddr ) << "/logdb/ws/\" + logname);" << endl;
    ostr << "    var l = document.getElementById('logname');" << endl;
    ostr << "    l.innerHTML = logname" << endl;
    ostr << "    ws.onmessage = function(evt)" << endl;
    ostr << "    {" << endl;
    ostr << "    	var p = document.getElementById('logs');" << endl;
    ostr << "    	if( evt.data != '.' ) {" << endl;
    ostr << "    		p.innerHTML = p.innerHTML + \"</br>\"+evt.data" << endl;
    ostr << "    		LogAutoScroll();" << endl;
    ostr << "    	}" << endl;
    ostr << "    };" << endl;
    ostr << "    ws.onclose = function()" << endl;
    ostr << "      { " << endl;
    ostr << "        alert(\"WebSocket closed.\");" << endl;
    ostr << "      };" << endl;
    ostr << "  }" << endl;
    ostr << "  else" << endl;
    ostr << "  {" << endl;
    ostr << "     alert(\"This browser does not support WebSockets.\");" << endl;
    ostr << "  }" << endl;
    ostr << "}" << endl;

    ostr << "</script>" << endl;
    ostr << "<style media='all' type='text/css'>" << endl;
    ostr << ".logs {" << endl;
    ostr << "	font-family: 'Liberation Mono', 'DejaVu Sans Mono', 'Courier New', monospace;" << endl;
    ostr << "	padding-top: 30px;" << endl;
    ostr << "}" << endl;
    ostr << "" << endl;
    ostr << ".logtitle {" << endl;
    ostr << "	position: fixed;" << endl;
    ostr << "	top: 0;" << endl;
    ostr << "	left: 0;" << endl;
    ostr << "	padding: 10px;" << endl;
    ostr << "	width: 100%;" << endl;
    ostr << "	height: 25px;" << endl;
    ostr << "	background-color: " << bgColorTitle << ";" << endl;
    ostr << "	color: " << fgColorTitle << ";" << endl;
    ostr << "	border-top: 2px solid;" << endl;
    ostr << "	border-bottom: 2px solid;" << endl;
    ostr << "	border-color: white;" << endl;
    ostr << "}" << endl;
    ostr << "</style>" << endl;
    ostr << "</head>" << endl;
    ostr << "<body style='background: " << bgColor << "; color: " << fgColor << ";' onload=\"javascript:WebSocketCreate('" << logname << "')\">" << endl;
    ostr << "<h4><div onclick='javascritpt:clickScroll()' id='logname' class='logtitle'></div></h4>" << endl;
    ostr << "<div id='logs' class='logs'></div>" << endl;
    ostr << "<p><div id='end' style='display: hidden;'>&nbsp;</div></p>" << endl;
    ostr << "</body>" << endl;
}
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
