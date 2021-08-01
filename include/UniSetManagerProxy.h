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
#ifndef UniSetManagerProxy_H_
#define UniSetManagerProxy_H_
// --------------------------------------------------------------------------
#include <memory>
#include <unordered_map>
#include "UniSetTypes.h"
#include "UniSetManager.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    //-------------------------------------------------------------------------
    class UniSetActivator;
    //-------------------------------------------------------------------------
    /*! Реализует grpc-интерфейс UniSetManager_i и проксирует запросы к дочерним объектам по svcId */
    class UniSetManagerProxy final:
        public UniSetManager_i::Service
    {
        public:
            UniSetManagerProxy();
            virtual ~UniSetManagerProxy();

            // ------  GRPC интерфейс ------
            virtual ::grpc::Status broadcast(::grpc::ServerContext* context, const ::uniset3::umessage::TransportMessage* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status getObjectsInfo(::grpc::ServerContext* context, const ::uniset3::ObjectsInfoParams* request, ::uniset3::SimpleInfoSeq* response) override;

            // --------------------------
            bool add( const std::shared_ptr<UniSetManager>& obj );
            bool remove( const std::shared_ptr<UniSetManager>& obj );
            // --------------------------
            size_t size() const;    // количество подчинённых объектов
            // ---------------
            void lock(); // запретить модификацию (добавление/удаление новых объектов)

        private:
            std::unordered_map<ObjectId, std::shared_ptr<UniSetManager> > mlist;
            mutable uniset3::uniset_rwmutex mlistMutex;

            std::atomic_bool is_running = {false};
    };
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
#endif
