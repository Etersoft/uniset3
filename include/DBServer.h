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
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#ifndef DBServer_H_
#define DBServer_H_
// --------------------------------------------------------------------------
#include "UniSetTypes.h"
#include "UniSetObject.h"
#include "LogServer.h"
#include "DebugStream.h"
#include "LogAgregator.h"
//------------------------------------------------------------------------------------------
namespace uniset3
{
    /*! Прототип реализации сервиса ведения БД
        Предназначен для работы с БД.
        Основная задача это - сохранение информации о датчиках, ведение журнала сообщений.
        \par Сценарий работы
             На узле, где ведётся БД запускается один экземпляр сервиса. Клиенты могут получить доступ, несколькими способами:
            - через NameService
            - при помощи UInterface::send()

            Сервис является системным, поэтому его идентификатор можно получить при помощи
        uniset3::Configuration::getDBServer() объекта uniset3::conf.
    */
    class DBServer:
        public UniSetObject
    {
        public:
            DBServer( uniset3::ObjectId id, const std::string& prefix = "db" );
            DBServer( const std::string& prefix = "db" );
            ~DBServer();

            static std::string help_print();

            virtual ::grpc::Status getInfo(::grpc::ServerContext* context, const ::uniset3::GetInfoParams* request, ::google::protobuf::StringValue* response) override;
            virtual std::string getStrType() const override;

        protected:

            virtual void processingMessage( const uniset3::umessage::TransportMessage* msg ) override;
            virtual void sysCommand( const uniset3::umessage::SystemMessage* sm ) override;
            virtual void confirmInfo( const uniset3::umessage::ConfirmMessage* cmsg ) {}

            virtual bool activateObject() override;
            virtual void initDBServer() {}
            virtual std::string getMonitInfo( const std::string& params )
            {
                return "";
            }

            std::shared_ptr<LogAgregator> loga;
            std::shared_ptr<DebugStream> dblog;
            std::shared_ptr<LogServer> logserv;
            std::string logserv_host = {""};
            int logserv_port = {0};
            const std::string prefix = { "db" };

        private:
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
//------------------------------------------------------------------------------------------
#endif
