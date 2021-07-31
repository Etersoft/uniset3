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
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#ifndef URepository_H_
#define URepository_H_
// --------------------------------------------------------------------------
#include <unordered_map>
#include "UniSetTypes.h"
#include "Mutex.h"
#include "DebugStream.h"
#include "URepository.grpc.pb.h"
// -------------------------------------------------------------------------
namespace uniset3
{
    //------------------------------------------------------------------------------------------
    /*!
        \page page_URepository GRPC-cервис для получения ссылок для обращения к объектам (URepository)

        - \ref sec_URepository_Comm
        - \ref sec_URepository_Conf

        \section sec_URepository_Comm Общее описание работы URepository
            URepository это сервис, который отдаёт ссылки, позволяющие обращаться к объектам.

        \section sec_URepository_Conf Настройка работы URepository
        Для запуска URepository необходимо настроить на каком порту и сетевом интерфейсе будут приниматься запросы.
        Для этого необходимо в настройках прописать следующую секцию
        \code
        <UniSet>
          ...
          <URepository name="URepository" port="8081" ip="0.0.0.0"/>
          ...
        </UniSet>
        \endcode
        - 0.0.0.0 - слушать на всех доступных интерфейсах
        - `--confile configure.xml`            - Файл с настройками
    */
    class URepository:
        public URepository_i::Service
    {
        public:
            URepository( const std::string& name, int argc, const char* const* argv, const std::string& prefix );
            virtual ~URepository();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<URepository> init_repository( int argc, const char* const* argv, const std::string& prefix = "urepository-" );

            /*! глобальная функция для вывода help-а */
            static void help_print();

            inline std::shared_ptr<DebugStream> log()
            {
                return rlog;
            }

            void run();
            std::string status();


            // proto interface
            virtual ::grpc::Status resolve(::grpc::ServerContext* context, const ::google::protobuf::Int64Value* request, ::uniset3::ObjectRef* response) override;
            virtual ::grpc::Status registration(::grpc::ServerContext* context, const ::uniset3::ObjectRef* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status unregistration(::grpc::ServerContext* context, const ::google::protobuf::Int64Value* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status list(::grpc::ServerContext* context, const ::google::protobuf::StringValue* request, ::uniset3::ObjectRefList* response) override;
            virtual ::grpc::Status getInfo(::grpc::ServerContext* context, const ::google::protobuf::StringValue* request, ::google::protobuf::StringValue* response) override;

        protected:
            std::string addr;
            std::string myname;

            uniset3::uniset_rwmutex omutex;
            std::unordered_map<uniset3::ObjectId, uniset3::ObjectRef> omap;

            std::shared_ptr<DebugStream> rlog;

        private:
    };
    // ----------------------------------------------------------------------------------
} // end of namespace uniset3
//------------------------------------------------------------------------------------------
#endif
