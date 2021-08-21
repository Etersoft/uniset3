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
// -------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include <Poco/Net/NetException.h>
#include "LogServer.h"
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "LogAgregator.h"
#include "Configuration.h"
#include "UTCPStream.h"
// -------------------------------------------------------------------------
namespace uniset3
{
    // -------------------------------------------------------------------------
    using namespace std;
    // -------------------------------------------------------------------------
    LogServer::~LogServer() noexcept
    {
        try
        {
            terminate();
        }
        catch(...) {}

        try
        {
            conn.disconnect();
        }
        catch(...) {}
    }
    // -------------------------------------------------------------------------
    void LogServer::setServerLog( Debug::type t ) noexcept
    {
        mylog.level(t);
    }
    // -------------------------------------------------------------------------
    void LogServer::setMaxSessionCount( size_t num ) noexcept
    {
        sessMaxCount = num;
    }
    // -------------------------------------------------------------------------
    void LogServer::updateDefaultLogLevel() noexcept
    {
        try
        {
            saveDefaultLogLevels("ALL");
        }
        catch(...) {}
    }
    // -------------------------------------------------------------------------
    LogServer::LogServer( std::shared_ptr<LogAgregator> log ):
        LogServer()
    {
        elog = dynamic_pointer_cast<DebugStream>(log);

        if( !elog )
        {
            ostringstream err;
            err << myname << "(LogServer): dynamic_pointer_cast FAILED! ";

            if( mylog.is_info() )
                mylog.info() << myname << "(init): terminate..." << endl;

            if( mylog.is_crit() )
                mylog.crit() << err.str() << endl;

            cerr << err.str()  << endl;

            throw SystemError(err.str());
        }

        alog = log;
        conn = alog->signal_stream_event().connect( sigc::mem_fun(this, &LogServer::logOnEvent) );
    }
    // -------------------------------------------------------------------------
    LogServer::LogServer( std::shared_ptr<DebugStream> log ):
        LogServer()
    {
        elog = log;
        conn = elog->signal_stream_event().connect( sigc::mem_fun(this, &LogServer::logOnEvent) );
    }
    // -------------------------------------------------------------------------
    LogServer::LogServer():
        elog(nullptr),
        alog(nullptr),
        cancelled(false)
    {
        slist.reserve(sessMaxCount);
    }
    // -------------------------------------------------------------------------
    std::list<LogAgregator::iLog> LogServer::getLogList( const std::string& logname )
    {
        std::list<LogAgregator::iLog> loglist;

        if( alog ) // если у нас "агрегатор", то работаем с его списком логов
        {
            if( logname.empty() || logname == "ALL" || logname == alog->getLogName())
                loglist = alog->getLogList();
            else
                loglist = alog->getLogList(logname);
        }
        else
        {
            if( logname.empty() || logname == "ALL" || elog->getLogName() == logname )
                loglist.emplace_back(elog, elog->getLogName());
        }

        return loglist;
    }
    // -------------------------------------------------------------------------
    ::grpc::Status LogServer::list(::grpc::ServerContext* context, const ::uniset3::logserver::LogListParams* request, ::uniset3::logserver::LogMessage* response)
    {
        if( cancelled )
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "(list): Server terminated.");

        if( context->IsCancelled() )
            return grpc::Status(grpc::StatusCode::CANCELLED, "(list): Deadline exceeded or Client cancelled, abandoning.");

        auto loglist = getLogList(request->logname());

        ostringstream s;
        s << "List of managed logs(filter='" << request->logname() << "'):" << endl;
        s << "=====================" << endl;
        LogAgregator::printLogList(s, loglist);
        s << "=====================" << endl << endl;
        response->set_text(s.str());
        return grpc::Status::OK;
    }
    // -------------------------------------------------------------------------
    ::grpc::Status LogServer::loglevel(::grpc::ServerContext* context, const ::uniset3::logserver::LogLevelParams* request, ::uniset3::logserver::LogMessage* response)
    {
        if( cancelled )
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "(loglevel): Server terminated.");

        if( context->IsCancelled() )
            return grpc::Status(grpc::StatusCode::CANCELLED, "(loglevel): Deadline exceeded or Client cancelled, abandoning.");

        ostringstream s;
        s << "List of saved default log levels (filter='" << request->logname() << "')[" << defaultLogLevels.size() << "]: " << endl;
        s << "=================================" << endl;
        auto alog = dynamic_pointer_cast<LogAgregator>(elog);

        if( alog ) // если у нас "агрегатор", то работаем с его списком потоков
        {
            std::list<LogAgregator::iLog> lst;

            if( request->logname().empty() || request->logname() == "ALL" )
                lst = alog->getLogList();
            else
                lst = alog->getLogList(request->logname());

            std::string::size_type max_width = 1;

            // ищем максимальное название для выравнивания по правому краю
            for( const auto& l : lst )
                max_width = std::max(max_width, l.name.length() );

            for( const auto& l : lst )
            {
                Debug::type deflevel = Debug::NONE;
                auto i = defaultLogLevels.find(l.log.get());

                if( i != defaultLogLevels.end() )
                    deflevel = i->second;

                s << std::left << setw(max_width) << l.name << std::left << " [ " << Debug::str(deflevel) << " ]" << endl;
            }
        }
        else if( elog )
        {
            Debug::type deflevel = Debug::NONE;
            auto i = defaultLogLevels.find(elog.get());

            if( i != defaultLogLevels.end() )
                deflevel = i->second;

            s << elog->getLogName() << " [" << Debug::str(deflevel) << " ]" << endl;
        }

        s << "=================================" << endl << endl;

        response->set_text(s.str());
        return grpc::Status::OK;
    }
    // -------------------------------------------------------------------------
    ::grpc::Status LogServer::command(::grpc::ServerContext* context, const ::uniset3::logserver::LogCommandList* request, ::google::protobuf::Empty* response)
    {
        if( cancelled )
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "(command): Server terminated.");

        if( context->IsCancelled() )
            return grpc::Status(grpc::StatusCode::CANCELLED, "(command): Deadline exceeded or Client cancelled, abandoning.");

        mylog.info() << myname << "(command): call with " << request->cmd_size() << " commands.." << endl;

        for( const auto& c : request->cmd() )
        {
            auto ret = cmdProcessing(c);

            if( !ret.ok() )
                return ret;
        }

        return grpc::Status::OK;
    }
    // -------------------------------------------------------------------------
    ::grpc::Status LogServer::read(::grpc::ServerContext* context, const google::protobuf::Empty* request, ::grpc::ServerWriter< ::uniset3::logserver::LogMessage>* writer)
    {
        if( cancelled )
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "(read): Server terminated.");

        if( context->IsCancelled() )
            return grpc::Status(grpc::StatusCode::CANCELLED, "(read): Deadline exceeded or Client cancelled, abandoning.");

        mylog.info() << myname << "(read): create new session " << context->peer() << endl;

        auto s = std::make_shared<LogSession>();
        s->writer = writer;

        // add to session list
        {
            uniset3::uniset_rwmutex_wrlock lock(mutSList);

            if( sessMaxCount <= 0 || slist.size() >= sessMaxCount )
            {
                mylog.info() << myname << "(read): max session limit " << sessMaxCount << endl;
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, "(read): max session limit " + to_string(sessMaxCount));
            }

            slist.push_back(s);
        }

        s->wait();
        mylog.info() << myname << "(read): terminate session: " << context->peer() << endl;
        return grpc::Status::OK;
    }
    // --------------------------------------------------------------------------------
    grpc::Status LogServer::cmdProcessing( const google::protobuf::Any& any )
    {
        if( !any.Is<uniset3::logserver::LogCommand>() )
        {
            mylog.warn() << myname << "(cmdProcessing): unsupported command type" << endl;
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "unsupported log server command type");
        }

        uniset3::logserver::LogCommand msg;

        if( !any.UnpackTo(&msg) )
        {
            mylog.crit() << myname << "(cmdProcessing): parse log server command error" << endl;
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "unpack log server command error");
        }

        auto loglist = getLogList(msg.logname());

        if( msg.cmd() == logserver::LOG_CMD_FILTER )
        {
            // отключаем старый обработчик
            if( conn )
                conn.disconnect();
        }

        // обрабатываем команды только если нашли подходящие логи
        for( auto&& l : loglist )
        {
            // Обработка команд..
            // \warning Работа с логом ведётся без mutex-а, хотя он разделяется отдельными потоками
            switch( msg.cmd() )
            {
                case logserver::LOG_CMD_SET:
                    l.log->level( (Debug::type)msg.data() );
                    break;

                case logserver::LOG_CMD_ADD:
                    l.log->addLevel( (Debug::type)msg.data() );
                    break;

                case logserver::LOG_CMD_DEL:
                    l.log->delLevel( (Debug::type)msg.data() );
                    break;

                case logserver::LOG_CMD_ROTATE:
                    l.log->onLogFile(true);
                    break;

                case logserver::LOG_CMD_LOGFILE_DISABLE:
                    l.log->offLogFile();
                    break;

                case logserver::LOG_CMD_LOGFILE_ENABLE:
                    l.log->onLogFile();
                    break;

                case logserver::LOG_CMD_FILTER:
                    l.log->signal_stream_event().connect( sigc::mem_fun(this, &LogServer::logOnEvent) );
                    break;

                case logserver::LOG_CMD_SAVE_LOGLEVEL:
                    saveDefaultLogLevels(msg.logname());
                    break;

                case logserver::LOG_CMD_RESTORE_LOGLEVEL:
                    restoreDefaultLogLevels(msg.logname());
                    break;

                case logserver::LOG_CMD_NOP:
                    break;

                default:
                    mylog.warn() << "(cmdProcessing): Unknown command '" << msg.cmd() << "'" << endl;
                    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "unknown log server command " + to_string(msg.cmd()));
            }
        } // end of for loglist

        return grpc::Status::OK;
    }
    // -------------------------------------------------------------------------
    ::grpc::Status LogServer::metrics(::grpc::ServerContext* context, const ::uniset3::metrics::MetricsParams* request, ::uniset3::metrics::Metrics* response)
    {
        response->set_id(DefaultObjectId);
        response->set_name(myname);
        {
            uniset3::uniset_rwmutex_rlock lock(mutSList);
            *response->add_metrics() = uniset3::createMetric("sessionCount", slist.size());
        }

        return grpc::Status::OK;
    }
    // -------------------------------------------------------------------------
    ::grpc::Status LogServer::setParams(::grpc::ServerContext* context, const ::uniset3::configurator::Params* request, ::uniset3::configurator::Params* response)
    {
        response->set_id(DefaultObjectId);
        auto i = request->params().find("sessMaxCount");

        if( i != request->params().end() && i->second.has_dvalue() )
        {
            uniset3::uniset_rwmutex_rlock lock(mutSList);
            setMaxSessionCount((size_t)i->second.dvalue());
        }

        return grpc::Status::OK;
    }
    // -------------------------------------------------------------------------
    ::grpc::Status LogServer::getParams(::grpc::ServerContext* context, const ::uniset3::configurator::Params* request, ::uniset3::configurator::Params* response)
    {
        auto m = response->mutable_params();
        (*m)["host"] = uniset3::createParamValue(addr);
        (*m)["port"] = uniset3::createParamValue(port);
        (*m)["sessMaxCount"] = uniset3::createParamValue(sessMaxCount);
        return grpc::Status::OK;
    }
    // -------------------------------------------------------------------------
    ::grpc::Status LogServer::loadConfig(::grpc::ServerContext* context, const ::uniset3::configurator::ConfigCmdParams* request, ::grpc::ServerWriter< ::uniset3::configurator::Config>* writer)
    {
        return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "loadConfig unimplemented");
    }
    // -------------------------------------------------------------------------
    ::grpc::Status LogServer::reloadConfig(::grpc::ServerContext* context, const ::uniset3::configurator::ConfigCmdParams* request, ::google::protobuf::Empty* response)
    {
        return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "reloadConfig unimplemented");
    }
    // -------------------------------------------------------------------------
    void LogServer::LogSession::wait()
    {
        std::unique_lock<std::mutex> lk(mut);
        cond.wait(lk, [&]()
        {
            return (done == true);
        });
    }
    // -------------------------------------------------------------------------
    void LogServer::LogSession::term()
    {
        {
            std::unique_lock<std::mutex> lk(mut);
            done = true;
        }
        cond.notify_all();
    }
    // -------------------------------------------------------------------------
    void LogServer::logOnEvent( const std::string& s ) noexcept
    {
        if( cancelled ) //  || s.empty() )
            return;

        logserver::LogMessage m;
        m.set_text(s);

        uniset3::uniset_rwmutex_rlock lock(mutSList);
        bool beforeEmpty = slist.empty();

        for( auto it = slist.begin(); it != slist.end(); it++ )
        {
            try
            {
                if( !(*it)->writer->Write(m) )
                {
                    (*it)->term();
                    it = slist.erase(it);

                    if( it == slist.end() )
                        it--;
                }
            }
            catch(...) {}
        }

        if( !beforeEmpty && slist.empty() )
        {
            // восстанавливаем уровни логов по умолчанию
            // т.к. все отключились
            restoreDefaultLogLevels("ALL");
        }
    }
    // -------------------------------------------------------------------------
    bool LogServer::run( const std::string& _addr, Poco::UInt16 _port )
    {
        if( server )
        {
            cerr << "LogServer already running.." << endl;
            return false;
        }

        addr = _addr;
        port = _port;

        {
            ostringstream s;
            s << _addr << ":" << _port;
            myname = s.str();
        }

        cancelled = false;
        ostringstream saddr;
        saddr << addr << ":" << port;
        builder.AddListeningPort(saddr.str(), grpc::InsecureServerCredentials());
        builder.RegisterService(static_cast<uniset3::logserver::LogServer_i::Service*>(this));
        server = builder.BuildAndStart();
        server->Wait();
        saveDefaultLogLevels("ALL");
        return  true;
    }
    // -------------------------------------------------------------------------
    bool LogServer::async_run( const std::string& _addr, Poco::UInt16 _port )
    {
        if( server )
        {
            cerr << "LogServer already running.." << endl;
            return false;
        }

        addr = _addr;
        port = _port;

        {
            ostringstream s;
            s << _addr << ":" << _port;
            myname = s.str();
        }

        cancelled = false;
        ostringstream saddr;
        saddr << addr << ":" << port;
        builder.AddListeningPort(saddr.str(), grpc::InsecureServerCredentials());
        builder.RegisterService(static_cast<uniset3::logserver::LogServer_i::Service*>(this));
        //        builder.SetSyncServerOption(grpc::ServerBuilder::CQ_TIMEOUT_MSEC, 5000);
        server = builder.BuildAndStart();
        saveDefaultLogLevels("ALL");
        return true;
    }
    // -------------------------------------------------------------------------
    void LogServer::terminate()
    {
        if( cancelled )
            return;

        cancelled = true;

        {
            uniset3::uniset_rwmutex_rlock lock(mutSList);

            for( auto&& r : slist )
                r->term();

            slist.clear();
        }

        if( server )
        {
            server->Shutdown();
            server = nullptr;
        }
    }
    // -------------------------------------------------------------------------
    bool LogServer::isRunning() const noexcept
    {
        return server != nullptr;
    }
    // -------------------------------------------------------------------------
    bool LogServer::check( bool restart_if_fail )
    {
        // смущает пока только, что эта функция будет вызыватся (обычно) из другого потока
        try
        {
            // для проверки пробуем открыть соединение..
            UTCPStream s;
            s.create(addr, port, 500);
            s.disconnect();
            return true;
        }
        catch(...) {}

        if( !restart_if_fail )
            return false;

        server->Shutdown();

        if( !async_run(addr, port) )
            return false;

        // Проверяем..
        try
        {
            UTCPStream s;
            s.create(addr, port, 500);
            s.disconnect();
            return true;
        }
        catch( Poco::Net::NetException& ex )
        {
            ostringstream err;
            err << myname << "(check): socket error:" << ex.message();

            if( mylog.is_crit() )
                mylog.crit() << err.str() << endl;
        }

        return false;
    }
    // -------------------------------------------------------------------------
    void LogServer::init( const std::string& prefix, xmlNode* cnode )
    {
        //        auto conf = uniset_conf();

        // можем на cnode==0 не проверять, т.е. UniXML::iterator корректно отрабатывает эту ситуацию
        //        UniXML::iterator it(cnode);
    }
    // -----------------------------------------------------------------------------
    std::string LogServer::help_print( const std::string& prefix )
    {
        std::ostringstream h;
        h << "--" << prefix << "-cmd-timeout msec      - Timeout for wait command. Default: 2000 msec." << endl;
        return h.str();
    }
    // -----------------------------------------------------------------------------
    string LogServer::getShortInfo()
    {
        std::ostringstream inf;

        inf << "LogServer: " << myname
            << " ["
            << " sessMaxCount=" << sessMaxCount
            << " ]"
            << endl;
        return inf.str();
    }
    // -----------------------------------------------------------------------------
    void LogServer::saveDefaultLogLevels( const std::string& logname )
    {
        if( mylog.is_info() )
            mylog.info() << myname << "(saveDefaultLogLevels): SAVE DEFAULT LOG LEVELS (logname=" << logname << ")" << endl;

        if( alog )
        {
            std::list<LogAgregator::iLog> lst;

            if( logname.empty() || logname == "ALL" )
                lst = alog->getLogList();
            else
                lst = alog->getLogList(logname);

            for( auto&& l : lst )
                defaultLogLevels[l.log.get()] = l.log->level();
        }
        else if( elog )
            defaultLogLevels[elog.get()] = elog->level();
    }
    // -----------------------------------------------------------------------------
    void LogServer::restoreDefaultLogLevels( const std::string& logname )
    {
        if( mylog.is_info() )
            mylog.info() << myname << "(restoreDefaultLogLevels): RESTORE DEFAULT LOG LEVELS (logname=" << logname << ")" << endl;

        if( alog )
        {
            std::list<LogAgregator::iLog> lst;

            if( logname.empty() || logname == "ALL" )
                lst = alog->getLogList();
            else
                lst = alog->getLogList(logname);

            for( auto&& l : lst )
            {
                auto d = defaultLogLevels.find(l.log.get());

                if( d != defaultLogLevels.end() )
                    l.log->level(d->second);
            }
        }
        else if( elog )
        {
            auto d = defaultLogLevels.find(elog.get());

            if( d != defaultLogLevels.end() )
                elog->level(d->second);
        }
    }
    // -----------------------------------------------------------------------------
} // end of namespace uniset3
