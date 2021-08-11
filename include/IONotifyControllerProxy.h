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
#ifndef IONotifyContollerProxy_H_
#define IONotifyContollerProxy_H_
// --------------------------------------------------------------------------
#include <memory>
#include <atomic>
#include <unordered_map>
#include "UniSetTypes.h"
#include "IONotifyController.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    //-------------------------------------------------------------------------
    class UniSetActivator;
    //-------------------------------------------------------------------------
    /*! Реализует grpc-интерфейс IONotifyController_i и проксирует запросы к дочерним объектам
     * Но т.к. мы не знаем какой датчик к какому IOController относится
    * но при этом датчики не пересекаются,
    * то вызываем у всех, пока кто-то не вернёт OK.
    * (не оптимально, но список IOController-ов обычно будет состоять из одного элемента (SharedMemory)
    */
    class IONotifyControllerProxy final:
            public IONotifyController_i::Service
    {
        public:
            IONotifyControllerProxy();
            virtual ~IONotifyControllerProxy();

            // ------  GRPC интерфейс ------
            virtual ::grpc::Status askSensor(::grpc::ServerContext* context, const ::uniset3::AskParams* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status askSensorsSeq(::grpc::ServerContext* context, const ::uniset3::AskSeqParams* request, ::uniset3::IDSeq* response) override;
            // --------------------------
            bool add( const std::shared_ptr<IONotifyController>& obj );
            bool remove( const std::shared_ptr<IONotifyController>& obj );
            // --------------------------
            size_t size() const;    // количество подчинённых объектов
            // ---------------
            void lock(); // запретить модификацию (добавление/удаление новых объектов)

        private:
            std::unordered_map<ObjectId, std::shared_ptr<IONotifyController> > olist;
            mutable uniset3::uniset_rwmutex olistMutex;
            std::atomic_bool is_running = { false };
    };
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
#endif
