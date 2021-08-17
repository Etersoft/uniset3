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
#ifndef LogReader_H_
#define LogReader_H_
// -------------------------------------------------------------------------
#include <string>
#include <memory>
#include <queue>
#include <vector>
#include <atomic>
#include "DebugStream.h"
#include "LogServer.grpc.pb.h"
// -------------------------------------------------------------------------
namespace uniset3
{
    class LogReader
    {
        public:

            LogReader();
            ~LogReader();

            bool loglevel( const std::string& addr, int port, const std::string& logname, bool verbose = false );
            bool list( const std::string& addr, int port, const std::string& logname, bool verbose = false );
            bool read( const std::string& addr, int port, bool verbose = false );
            bool command(const std::string& addr, int port, const logserver::LogCommandList& lst, bool verbose = false);
            void readLoop(const std::string& addr, int port, const logserver::LogCommandList& lst, bool verbose = false);
            void terminate();

            bool isActive() const;
            bool isConnection() const;
            void setReadCount( size_t n );

            void setTimeout( timeout_t msec );
            void setReconnectDelay( timeout_t msec );
            void setTextFilter( const std::string& f );

            DebugStream::StreamEvent_Signal signal_stream_event();

            void setLogLevel( Debug::type t );

            std::shared_ptr<DebugStream> log();

            /*! Разбор строки на команды:
             *
             * [-a | --add] info,warn,crit,... [logfilter] - Add log levels.
             * [-d | --del] info,warn,crit,... [logfilter] - Delete log levels.
             * [-s | --set] info,warn,crit,... [logfilter] - Set log levels.
             *
             * 'logfilter' - regexp for name of log. Default: ALL logs
            */
            static logserver::LogCommandList getCommands( const std::string& cmd );

        protected:

            void logOnEvent( const std::string& s );
            timeout_t readTimeout = { 10000 };
            timeout_t reconDelay = { 5000 };

        private:
            std::shared_ptr<grpc::Channel> chan;
            std::shared_ptr<grpc::ClientContext> ctx;
            std::string iaddr = { "" };
            int port = { 0 };
            size_t readcount = { 0 }; // количество циклов чтения
            std::string textfilter = { "" };

            DebugStream rlog;
            std::shared_ptr<DebugStream> outlog; // рабочий лог в который выводиться полученная информация..

            DebugStream::StreamEvent_Signal m_logsig;
            std::atomic_bool active;
    };
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
// -------------------------------------------------------------------------
#endif // LogReader_H_
// -------------------------------------------------------------------------
