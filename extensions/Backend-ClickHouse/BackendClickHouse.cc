/*
 * Copyright (c) 2020 Pavel Vainerman.
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
// -------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "Exceptions.h"
#include "UniSetTypes.h"
#include "unisetstd.h"
#include "BackendClickHouse.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::extensions;
using namespace uniset3::umessage;
// -----------------------------------------------------------------------------
BackendClickHouse::TagList BackendClickHouse::parseTags( const std::string& tags )
{
    BackendClickHouse::TagList ret;

    auto taglist = uniset3::split(tags, ' ');

    if( taglist.empty() )
        return ret;

    for( const auto& t : taglist )
    {
        auto tag = uniset3::split(t, '=');

        if( tag.size() < 2 )
            throw uniset3::SystemError("Bad format for tag '" + t + "'. Must be 'tag=val'");

        ret.emplace_back(std::make_pair(tag[0], tag[1]));
    }

    return ret;
}
//--------------------------------------------------------------------------------
BackendClickHouse::BackendClickHouse( uniset3::ObjectId objId, xmlNode* cnode,
                                      uniset3::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic,
                                      const string& prefix ):
    UObject_SK(objId, cnode, string(prefix + "-")),
    prefix(prefix)
{
    auto conf = uniset_conf();

    if( ic )
        ic->logAgregator()->add(logAgregator());

    shm = make_shared<SMInterface>(shmId, ui, objId, ic);
    db = unisetstd::make_unique<ClickHouseInterface>();
    dyntags = unisetstd::make_unique<uniset3::ClickHouseTagsConfig>();

    createColumns();

    init(cnode);

    if( smTestID == DefaultObjectId && !clickhouseParams.empty() )
    {
        // берём первый датчик из списка
        smTestID = clickhouseParams.begin()->first;
    }
}
// -----------------------------------------------------------------------------
BackendClickHouse::~BackendClickHouse()
{
}
// -----------------------------------------------------------------------------
void BackendClickHouse::init( xmlNode* cnode )
{
    UniXML::iterator it(cnode);

    auto conf = uniset_conf();

    dbhost = conf->getArg2Param("--" + prefix + "-dbhost", it.getProp("dbhost"), "localhost");
    dbport = conf->getArgPInt("--" + prefix + "-dbport", it.getProp("dbport"), 9000);
    dbuser = conf->getArg2Param("--" + prefix + "-dbuser", it.getProp("dbuser"), "");
    dbpass = conf->getArg2Param("--" + prefix + "-dbpass", it.getProp("dbpass"), "");
    dbname = conf->getArg2Param("--" + prefix + "-dbname", it.getProp("dbname"), "");
    reconnectTime = conf->getArgPInt("--" + prefix + "-reconnect-time", it.getProp("reconnectTime"), reconnectTime);
    bufMaxSize = conf->getArgPInt("--" + prefix + "-buf-maxsize", it.getProp("bufMaxSize"), bufMaxSize);
    bufSize = conf->getArgPInt("--" + prefix + "-buf-size", it.getProp("bufMaxSize"), bufSize);
    bufSyncTime = conf->getArgPInt("--" + prefix + "-buf-sync-time", it.getProp("bufSyncTimeout"), bufSyncTime);

    const string tblname = conf->getArg2Param("--" + prefix + "-dbtablename", it.getProp("dtablebname"), "main_history");
    fullTableName = dbname.empty() ? tblname : dbname + "." + tblname;

    int sz = conf->getArgPInt("--" + prefix + "-uniset-object-size-message-queue", it.getProp("sizeOfMessageQueue"), 10000);

    if( sz > 0 )
        setMaxSizeOfMessageQueue(sz);

    const string ff = conf->getArg2Param("--" + prefix + "-filter-field", it.getProp("filter_field"), "" );
    const string fv = conf->getArg2Param("--" + prefix + "-filter-value", it.getProp("filter_value"), "" );

    const string gtags = conf->getArg2Param("--" + prefix + "-tags", it.getProp("tags"), "");
    globalTags = parseTags(gtags);

    myinfo << myname << "(init): clickhouse host=" << dbhost << ":" << dbport << " user=" << dbuser
           << " " << ff << "='" << fv << "'"
           << " tags='" << gtags << "'"
           << endl;

    UniXML::iterator tit = it;

    if( tit.goChildren() )
    {
        if( tit.find("clickhouse_tags", false) )
            dyntags->loadTagsMap(tit);
    }

    // try
    {
        xmlNode* snode = conf->getXMLSensorsSection();

        if( !snode )
        {
            ostringstream err;
            err << myname << "(init): Not found section <sensors>";
            mycrit << err.str() << endl;
            throw SystemError(err.str());
        }

        UniXML::iterator it1(snode);

        if( !it1.goChildren() )
        {
            ostringstream err;
            err << myname << "(init): section <sensors> empty?!";
            mycrit << err.str() << endl;
            throw SystemError(err.str());
        }

        for(; it1.getCurrent(); it1.goNext() )
        {
            if( !uniset3::check_filter(it1, ff, fv) )
                continue;

            const std::string name = it1.getProp("name");
            ObjectId sid = conf->getSensorID( name );

            if( sid == DefaultObjectId )
            {
                ostringstream err;
                err << myname << "(init): Unknown SensorID for '" << name << "'";
                mycrit << err.str();
                throw SystemError(err.str());
            }

            auto tags = parseTags(it1.getProp("clickhouse_tags"));

            clickhouseParams.emplace( sid, ParamInfo(name, tags) );
            dyntags->initFromItem(conf, it1);
        }

        if( clickhouseParams.empty() )
        {
            ostringstream err;
            err << myname << "(init): Not found items for send to ClickHouse..";
            mycrit << err.str() << endl;
            throw SystemError(err.str());
        }

    }

    myinfo << myname << "(init): " << clickhouseParams.size() << " sensors.." << endl;
}
//--------------------------------------------------------------------------------
void BackendClickHouse::createColumns()
{
    colTimeStamp = std::make_shared<clickhouse::ColumnDateTime64>(9);
    colValue = std::make_shared<clickhouse::ColumnFloat64>();
    colName = std::make_shared<clickhouse::ColumnString>();
    colNodeName = std::make_shared<clickhouse::ColumnString>();
    colProducer = std::make_shared<clickhouse::ColumnString>();
    arrTagKeys = std::make_shared<clickhouse::ColumnArray>(std::make_shared<clickhouse::ColumnString>());
    arrTagValues = std::make_shared<clickhouse::ColumnArray>(std::make_shared<clickhouse::ColumnString>());
}
//--------------------------------------------------------------------------------------------
void BackendClickHouse::clearData()
{
    colTimeStamp->Clear();
    colValue->Clear();
    colName->Clear();
    colNodeName->Clear();
    colProducer->Clear();
    arrTagKeys->Clear();
    arrTagValues->Clear();
}
//--------------------------------------------------------------------------------
void BackendClickHouse::help_print( int argc, const char* const* argv )
{
    cout << " Default prefix='clickhouse'" << endl;
    cout << "--clickhouse-name                - ID. Default: BackendClickHouse." << endl;
    cout << "--clickhouse-confnode            - configuration section name. Default: <NAME name='NAME'...> " << endl;
    cout << endl;
    cout << " ClickHouse: " << endl;
    cout << "--clickhouse-host  ip                        - host. Default: localhost" << endl;
    cout << "--clickhouse-port  num                       - port. Default: 9000" << endl;
    cout << "--clickhouse-dbuser user                     - DB user" << endl;
    cout << "--clickhouse-dbpass pass                     - DB pass" << endl;
    cout << "--clickhouse-dbname name                     - DB name" << endl;
    cout << "--clickhouse-tags 'TAG1=VAL1 TAG2=VAL2...'   - tags for data" << endl;
    cout << "--clickhouse-reconnect-time msec             - Time for attempts to connect to DB. Default: 5 sec" << endl;
    cout << endl;
    cout << "--clickhouse-buf-size  sz        - Buffer before save to DB. Default: 500" << endl;
    cout << "--clickhouse-buf-maxsize  sz     - Maximum size for buffer (drop messages). Default: 5000" << endl;
    cout << "--clickhouse-buf-sync-time msec  - Time period for forced data writing to DB. Default: 5 sec" << endl;
    cout << endl;
    cout << "--clickhouse-heartbeat-id name   - ID for heartbeat sensor." << endl;
    cout << "--clickhouse-heartbeat-max val   - max value for heartbeat sensor." << endl;
    cout << endl;
    cout << " Logs: " << endl;
    cout << "--clickhouse-log-...            - log control" << endl;
    cout << "             add-levels ...  " << endl;
    cout << "             del-levels ...  " << endl;
    cout << "             set-levels ...  " << endl;
    cout << "             logfile filename" << endl;
    cout << "             no-debug " << endl;
    cout << endl;
    cout << " LogServer: " << endl;
    cout << "--clickhouse-run-logserver      - run logserver. Default: localhost:id" << endl;
    cout << "--clickhouse-logserver-host ip  - listen ip. Default: localhost" << endl;
    cout << "--clickhouse-logserver-port num - listen port. Default: ID" << endl;
    cout << LogServer::help_print("prefix-logserver") << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<BackendClickHouse> BackendClickHouse::init_clickhouse( int argc,
        const char* const* argv,
        uniset3::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
        const std::string& prefix )
{
    auto conf = uniset_conf();

    ObjectId ID = conf->getDBServer();
    string name = conf->getArgParam("--" + prefix + "-name", "BackendClickHouse");

    if( !name.empty() )
    {
        ID = conf->getServiceID(name);

        if( ID == uniset3::DefaultObjectId )
        {
            cerr << "(BackendClickHouse): Not found ServiceID for '" << name
                  << " in '" << conf->getServicesSection() << "' section" << endl;
            return nullptr;
        }
    }

    string confname = conf->getArgParam("--" + prefix + "-confnode", name);
    xmlNode* cnode = conf->getNode(confname);

    if( !cnode )
    {
        cerr << "(BackendClickHouse): " << name << "(init): Not found <" + confname + ">" << endl;
        return nullptr;
    }

    dinfo << "(BackendClickHouse): name = " << name << "(" << ID << ")" << endl;
    return make_shared<BackendClickHouse>(ID, cnode, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
void BackendClickHouse::callback() noexcept
{
    // используем стандартную "низкоуровневую" реализацию
    // т.к. она нас устраивает (обработка очереди сообщений и таймеров)
    UniSetObject::callback();
}
// -----------------------------------------------------------------------------
void BackendClickHouse::askSensors( uniset3::UIOCommand cmd )
{
    UObject_SK::askSensors(cmd);

    // прежде чем заказывать датчики, надо убедиться что SM доступна
    if( !waitSM(smReadyTimeout) )
    {
        uterminate();
        return;
    }

    myinfo << myname << ": ask " << clickhouseParams.size() << " sensors" << endl;

    for( const auto& s : clickhouseParams )
    {
        try
        {
            shm->askSensor(s.first, cmd);
        }
        catch( const std::exception& ex )
        {
            mycrit << myname << "(askSensors): " << ex.what() << endl;
        }
    }

    myinfo << myname << ": ask " << clickhouseParams.size() << " sensors [OK]" << endl;

}
// -----------------------------------------------------------------------------
void BackendClickHouse::sensorInfo( const uniset3::umessage::SensorMessage* sm )
{
    auto it = clickhouseParams.find(sm->id());

    if( it == clickhouseParams.end() )
        return;

    auto oinf = uniset_conf()->oind->getObjectInfo(sm->id());

    if( !oinf )
    {
        mycrit << myname << "(sensorInfo): unknown object info for sensor_id=" << sm->id() << endl;
        return;
    }

    auto suppinf = uniset_conf()->oind->getObjectInfo(sm->header().supplier());
    auto nodeinf = uniset_conf()->oind->getObjectInfo(sm->header().node());

    try
    {
        colTimeStamp->Append( uniset3::uniset_timespec_to_nanosec(sm->sm_ts()) );
        colValue->Append(sm->value());
        colName->Append(oinf->name);

        if( nodeinf )
            colNodeName->Append(nodeinf->name);
        else
            colNodeName->Append("");

        if( suppinf )
            colProducer->Append(suppinf->name);
        else if( sm->header().supplier() == uniset3::AdminID )
            colProducer->Append("uniset-admin");
        else
            colProducer->Append("");

        // TAGS
        auto key = std::make_shared<clickhouse::ColumnString>();
        auto val = std::make_shared<clickhouse::ColumnString>();

        for( const auto& t : it->second.tags )
        {
            key->Append(t.first);
            val->Append(t.second);
        }

        // GLOBAL TAGS
        for( const auto& t : globalTags )
        {
            key->Append(t.first);
            val->Append(t.second);
        }

        // dyn tags
        // обновляем значения в динамических тегах
        dyntags->updateTags(sm->id(), sm->value());
        auto dtags = dyntags->getTags(sm->id());

        for( const auto& t : dtags )
        {
            key->Append(t.key);
            val->Append(t.value);
        }

        // save tags
        arrTagKeys->AppendAsColumn(key);
        arrTagValues->AppendAsColumn(val);

        if( colTimeStamp->Size() >= bufSize )
        {
            if( flushBuffer() )
                return;

            if( colTimeStamp->Size() >= bufMaxSize )
            {
                mycrit << "BUFFER OVERFLOW! MaxBufSize=" << bufMaxSize
                       << ". ALL DATA LOST!" << endl;
                clearData();
            }
        }

        if( !timerIsOn )
        {
            timerIsOn = true;
            askTimer(tmFlushBuffer, bufSyncTime, 1);
            return;
        }
    }
    catch( const uniset3::Exception& ex )
    {
        mycrit << myname << "(insert_main_history): " << ex << endl;
    }
    catch( const std::exception& ex )
    {
        mycrit << myname << "(insert_main_history): " << ex.what() << endl;
    }
    catch( ... )
    {
        mycrit << myname << "(insert_main_history): catch ..." << endl;
    }
}
// -----------------------------------------------------------------------------
void BackendClickHouse::timerInfo( const uniset3::umessage::TimerMessage* tm )
{
    if( tm->id() == tmFlushBuffer )
    {
        if( flushBuffer() )
            timerIsOn = false;
        else if( !db->ping() )
        {
            askTimer(tmFlushBuffer, 0);
            timerIsOn = false;
            askTimer(tmReconnect, reconnectTime);
        }
    }
    else if( tm->id() == tmReconnect )
    {
        myinfo << myname << " try reconnect.." << endl;

        if( reconnect() )
        {
            myinfo << myname << " reconnect [OK]" << endl;
            askTimer(tmReconnect, 0);
            flushBuffer();
            askTimer(tmFlushBuffer, bufSyncTime, 1);
            timerIsOn = true;
        }
    }
}
// -----------------------------------------------------------------------------
void BackendClickHouse::sysCommand(const umessage::SystemMessage* sm)
{
    if( sm->cmd() == SystemMessage::StartUp )
    {
        if( !reconnect() )
            askTimer(tmReconnect, reconnectTime);
    }
}
// -----------------------------------------------------------------------------
bool BackendClickHouse::flushBuffer()
{
    if( colTimeStamp->Size() == 0 )
        return false;

    if( !db || !connect_ok )
        return false;

    myinfo << myname << "(flushBuffer): write insert buffer[" << colTimeStamp->Size() << "] to DB.." << endl;

    clickhouse::Block blk(8, colTimeStamp->Size());
    blk.AppendColumn("timestamp", colTimeStamp);
    blk.AppendColumn("value", colValue);
    blk.AppendColumn("name", colName);
    blk.AppendColumn("nodename", colNodeName);
    blk.AppendColumn("producer", colProducer);
    blk.AppendColumn("tags.name", arrTagKeys);
    blk.AppendColumn("tags.value", arrTagValues);

    if( !db->insert(fullTableName, blk) )
    {
        mycrit << myname << "(flushBuffer): error: " << db->error() << endl;
        return false;
    }

    clearData();
    return true;
}
//------------------------------------------------------------------------------
bool BackendClickHouse::reconnect()
{
    connect_ok = db->reconnect(dbhost, dbuser, dbpass, dbname, dbport);
    return connect_ok;
}
//------------------------------------------------------------------------------
std::string BackendClickHouse::getMonitInfo() const
{
    ostringstream inf;

    inf << "Database: " << dbhost << ":" << dbport << " user=" << dbuser
        << " ["
        << " reconnect=" << reconnectTime
        << " bufSyncTime=" << bufSyncTime
        << " bufSize=" << bufSize
        << " tags:";

    for( const auto& t : globalTags )
        inf << " " << t.first << "=" << t.second;

    inf << " ]" << endl
        << "  connection: " << ( connect_ok ? "OK" : "FAILED") << endl
        << " buffer size: " << colTimeStamp->Size() << endl
        << "   lastError: " << lastError << endl;

    return inf.str();
}
// -----------------------------------------------------------------------------
