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
#ifndef UniSetObjectProxy_H_
#define UniSetObjectProxy_H_
// --------------------------------------------------------------------------
#include <memory>
#include <atomic>
#include <unordered_map>
#include "UniSetTypes.h"
#include "UniSetObject.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    //-------------------------------------------------------------------------
    class UniSetActivator;
    //-------------------------------------------------------------------------
    /*! Реализует grpc-интерфейс UniSetObject_i и проксирует запросы к дочерним объектам по svcId
     *  после запуска (вызова activate()) добавление и удаление объектов уже недоступны и вызывает исключение
     *  это сделано чтобы во время работы уже не требовалось лочить (mutex) список
     */
    class UniSetObjectProxy final:
            public UniSetObject_i::Service
    {
        public:
            UniSetObjectProxy();
            virtual ~UniSetObjectProxy();

            // ------  GRPC интерфейс ------
            virtual ::grpc::Status getType(::grpc::ServerContext* context, const ::uniset3::GetTypeParams* request, ::google::protobuf::StringValue* response) override;
            virtual ::grpc::Status getInfo(::grpc::ServerContext* context, const ::uniset3::GetInfoParams* request, ::google::protobuf::StringValue* response) override;
            virtual ::grpc::Status exists(::grpc::ServerContext* context, const ::uniset3::ExistsParams* request, ::google::protobuf::BoolValue* response) override;
            virtual ::grpc::Status push(::grpc::ServerContext* context, const ::uniset3::umessage::TransportMessage* request, ::google::protobuf::Empty* response) override;
            // --------------------------
            bool add( const std::shared_ptr<UniSetObject>& obj );
            bool remove( const std::shared_ptr<UniSetObject>& obj );
            // --------------------------
            size_t size() const;    // количество подчинённых объектов
            // ---------------
            void lock(); // запретить модификацию (добавление/удаление новых объектов)

        private:
            std::unordered_map<ObjectId, std::shared_ptr<UniSetObject> > olist;
            mutable uniset3::uniset_rwmutex olistMutex;
            std::atomic_bool is_running = { false };
    };
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
#endif
