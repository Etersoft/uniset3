/*
 * Copyright (c) 2015 Pavel Vainerman.
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
#ifndef UTCPStream_H_
#define UTCPStream_H_
// -------------------------------------------------------------------------
#include <string>
#include <Poco/Net/SocketStream.h>
#include "PassiveTimer.h" // for timeout_t
// -------------------------------------------------------------------------
namespace uniset3
{

    /*! Специальная "обёртка" над ost::TCPStream, устанавливающая ещё и параметры KeepAlive,
     * для открытого сокета.
     * \note Правда это linux-only
    */
    class UTCPStream:
        public Poco::Net::StreamSocket
    {
        public:

            UTCPStream( const Poco::Net::StreamSocket& so );
            UTCPStream();
            virtual ~UTCPStream();

            void create( const std::string& hname, uint16_t port, timeout_t tout_msec = 1000 );

            bool isConnected() const noexcept;

            // set keepalive params
            // return true if OK
            bool setKeepAliveParams( timeout_t timeout_sec = 5, int conn_keepcnt = 1, int keepintvl = 2 );

            bool isSetLinger() const;
            void forceDisconnect(); // disconnect() без ожидания (с отключением SO_LINGER)
            void disconnect();

            // --------------------------------------------------------------------
            int getSocket() const;
            timeout_t getTimeout() const;

        protected:

        private:

    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// -------------------------------------------------------------------------
#endif // UTCPStream_H_
// -------------------------------------------------------------------------
