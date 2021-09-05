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
// -----------------------------------------------------------------------------
#ifndef UAsyncClient_H_
#define UAsyncClient_H_
// -----------------------------------------------------------------------------
#include <memory>
#include <UniSetObject.h>
#include "UniSetTypes.h"
#include "IOController.grpc.pb.h"
// -----------------------------------------------------------------------------
namespace uniset3
{
    /*! Объект создаёт соединение с указанным сервером и через это соединение работает с датчиками и т..п */
    class UAsyncClient:
        public UniSetObject
    {
        public:
            UAsyncClient();
            ~UAsyncClient();

            void setValue( ObjectId sid, long value );
            void askSensor( ObjectId sid );

            void async_run( const std::string& addr, int port, const std::shared_ptr<UniSetObject>& consumer );
            void terminate();

            // enabled/disable SystemMessage::NetworkInfo (disconnect/connect)
            void enableConnectionEvent(int ID );
            void disableConectionEvent();

            bool isConnected() const;

            class AsyncClientSession;

        protected:
            void mainLoop( const std::string& host, int port, const std::shared_ptr<UniSetObject>& consumer );
            void sendConnectionEvent(bool state);

        private:
            std::unique_ptr<AsyncClientSession> session;
            std::shared_ptr<UniSetObject> consumer;
            std::unique_ptr<std::thread> thr;
            grpc::CompletionQueue cq;
            timeout_t reconnectPause_msec = { 5000 };
            timeout_t connectTimeout_msec = { 5000 };
            int connectionEventID = {0 };
            std::atomic_bool connected = { false };
            std::atomic_bool active = { false };
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
// -----------------------------------------------------------------------------
#endif
// -----------------------------------------------------------------------------
