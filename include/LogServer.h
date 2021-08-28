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
#ifndef LogServer_H_
#define LogServer_H_
// -------------------------------------------------------------------------
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include "Mutex.h"
#include "UniXML.h"
#include "DebugStream.h"
#include "ThreadCreator.h"
#include "LogAgregator.h"
#include "UniSetTypes.h"
#include "LogServer.grpc.pb.h"
#include "MetricsExporter.grpc.pb.h"
#include "Configurator.grpc.pb.h"
// -------------------------------------------------------------------------
namespace uniset3
{
    // -------------------------------------------------------------------------
    /*! \page pgLogServer Лог сервер
        Лог сервер предназначен для возможности удалённого чтения логов (DebugStream).
    Ему указывается host и port на котором он отвечает, подключаться можно при помощи
    LogReader. Читающих клиентов может быть скольугодно много, на каждого создаётся своя "сессия"(LogSession).
    При этом через лог сервер имеется возможность управлять включением или отключением определённых уровней логов,
    записью, отключением записи или ротацией файла с логами. DebugStream за которым ведётся "слежение"
    задаётся в конструкторе для LogServer.

     По умолчанию, при завершении \b ВСЕХ подключений, LogServer автоматически восстанавливает уровни логов,
     которые были на момент \b первого \b подключения.
     Но если была передана команда LogServerTypes::cmdSaveLogLevel (в любом из подключений), то будут сохранены те уровни,
     которые выставлены подключившимся клиентом.
     Для этого LogServer подключается к сигналу на получение команд у каждой сессии и сам обрабатывает команды,
     на сохранение, восстановление и показ текущих "умолчательных" логов.

    \code
       DebugStream mylog;
       LogServer logsrv(mylog);
       ...
       logsrv.run(host,port);
       ...
    \endcode

    При этом если необходимо управлять или читать сразу несколько логов можно воспользоваться специальным классом LogAgregator.
    \code
        auto log1 = make_shared<DebugStream>();
        log1->setLogName("log1");

        auto log2 = make_shared<DebugStream>();
        log2->setLogName("log2");

        LogAgregator la;
        la.add(log1);
        la.add(log2);

        LogServer logsrv(la);
        ...
        logsrv.run(host,port,create_thread);
        ...
    \endcode

    \warning Логи отдаются "клиентам" только целиком строкой. Т.е. по сети информация передаваться не будет пока не будет записан "endl".
        Это сделано для "оптимизации передачи" (чтобы не передавать каждый байт)
    */
    // -------------------------------------------------------------------------
    class LogServer final:
        public uniset3::logserver::LogServer_i::Service,
        public uniset3::metrics::MetricsExporter_i::Service,
        public uniset3::configurator::Configurator_i::Service
    {
        public:

            LogServer( std::shared_ptr<DebugStream> log );
            LogServer( std::shared_ptr<LogAgregator> log );
            virtual ~LogServer() noexcept;

            virtual ::grpc::Status read(::grpc::ServerContext* context, const google::protobuf::Empty* request, ::grpc::ServerWriter< ::uniset3::logserver::LogMessage>* writer) override;
            virtual ::grpc::Status list(::grpc::ServerContext* context, const ::uniset3::logserver::LogListParams* request, ::uniset3::logserver::LogMessage* response) override;
            virtual ::grpc::Status loglevel(::grpc::ServerContext* context, const ::uniset3::logserver::LogLevelParams* request, ::uniset3::logserver::LogMessage* response) override;
            virtual ::grpc::Status command(::grpc::ServerContext* context, const ::uniset3::logserver::LogCommandList* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status metrics(::grpc::ServerContext* context, const ::uniset3::metrics::MetricsParams* request, ::uniset3::metrics::Metrics* response) override;
            virtual ::grpc::Status setParams(::grpc::ServerContext* context, const ::uniset3::configurator::Params* request, ::uniset3::configurator::Params* response) override;
            virtual ::grpc::Status getParams(::grpc::ServerContext* context, const ::uniset3::configurator::Params* request, ::uniset3::configurator::Params* response) override;
            virtual ::grpc::Status loadConfig(::grpc::ServerContext* context, const ::uniset3::configurator::ConfigCmdParams* request, ::grpc::ServerWriter< ::uniset3::configurator::Config>* writer) override;
            virtual ::grpc::Status reloadConfig(::grpc::ServerContext* context, const ::uniset3::configurator::ConfigCmdParams* request, ::google::protobuf::Empty* response) override;


            void setServerLog( Debug::type t ) noexcept;
            void setMaxSessionCount( size_t num ) noexcept;
            void updateDefaultLogLevel() noexcept;

            bool async_run( const std::string& addr, Poco::UInt16 port );
            bool run( const std::string& addr, Poco::UInt16 port );

            void terminate();

            bool isRunning() const noexcept;

            bool check( bool restart_if_fail = true );

            void init( const std::string& prefix, xmlNode* cnode = 0 );

            static std::string help_print( const std::string& prefix );

            std::string getShortInfo();

        protected:
            LogServer();

            void saveDefaultLogLevels( const std::string& logname );
            void restoreDefaultLogLevels( const std::string& logname );
            void logOnEvent( const std::string& s ) noexcept;
            grpc::Status cmdProcessing( const google::protobuf::Any& cmd );
            std::list<LogAgregator::iLog> getLogList( const std::string& logname );

        private:

            size_t sessMaxCount = { 10 };

            grpc::ServerBuilder builder;
            std::unique_ptr<grpc::Server> server;

            struct LogSession
            {
                ::grpc::ServerWriter< ::uniset3::logserver::LogMessage>* writer;
                std::mutex              mut;
                std::condition_variable cond;
                std::atomic_bool done;
                void wait();
                void term();
            };

            typedef std::vector< std::shared_ptr<LogSession> > SessionList;
            SessionList slist;
            uniset3::uniset_rwmutex mutSList;

            DebugStream mylog;

            std::shared_ptr<DebugStream> elog; // eventlog..
            std::shared_ptr<LogAgregator> alog;
            sigc::connection conn;
            std::atomic_bool cancelled;

            // map с уровнями логов по умолчанию (инициализируются при создании первой сессии),
            // (они необходимы для восстановления настроек после завершения всех (!) сессий)
            // т.к. shared_ptr-ов может быть много, то в качестве ключа используем указатель на "реальный объект"(внутри shared_ptr)
            // но только для этого(!), пользоваться этим указателем ни в коем случае нельзя (и нужно проверять shared_ptr на существование)
            std::unordered_map< DebugStream*, Debug::type > defaultLogLevels;

            std::string myname = { "LogServer" };
            std::string addr = { "" };
            Poco::UInt16 port = { 0 };
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // LogServer_H_
// -------------------------------------------------------------------------
