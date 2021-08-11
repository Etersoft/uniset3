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
#ifndef IOContollerProxy_H_
#define IOContollerProxy_H_
// --------------------------------------------------------------------------
#include <memory>
#include <atomic>
#include <unordered_map>
#include "UniSetTypes.h"
#include "IOController.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    //-------------------------------------------------------------------------
    class UniSetActivator;
    //-------------------------------------------------------------------------
    /*! Реализует grpc-интерфейс IOController_i и проксирует запросы к дочерним объектам
     * Но т.к. мы не знаем какой датчик к какому IOController относится
     * но при этом датчики не пересекаются,
     * то вызываем у всех, пока кто-то не вернёт OK.
     * (не оптимально, но список IOController-ов обычно будет состоять из одного элемента (SharedMemory)
     */
    class IOControllerProxy final:
            public IOController_i::Service
    {
        public:
            IOControllerProxy();
            virtual ~IOControllerProxy();

            // ------  GRPC интерфейс ------
            virtual ::grpc::Status getValue(::grpc::ServerContext* context, const ::uniset3::GetValueParams* request, ::google::protobuf::Int64Value* response) override;
            virtual ::grpc::Status setValue(::grpc::ServerContext* context, const ::uniset3::SetValueParams* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status freezeValue(::grpc::ServerContext* context, const ::uniset3::FreezeValueParams* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status getIOType(::grpc::ServerContext* context, const ::uniset3::GetIOTypeParams* request, ::uniset3::RetIOType* response) override;
            virtual ::grpc::Status getRawValue(::grpc::ServerContext* context, const ::uniset3::GetRawValueParams* request, ::google::protobuf::Int64Value* response) override;
            virtual ::grpc::Status calibrate(::grpc::ServerContext* context, const ::uniset3::CalibrateParams* request, ::google::protobuf::Empty* response) override;
            virtual ::grpc::Status getCalibrateInfo(::grpc::ServerContext* context, const ::uniset3::GetCalibrateInfoParams* request, ::uniset3::CalibrateInfo* response) override;
            virtual ::grpc::Status getSensorsMap(::grpc::ServerContext* context, const ::uniset3::GetSensorsMapParams* request, ::uniset3::SensorIOInfoSeq* response) override;
            virtual ::grpc::Status getSensorIOInfo(::grpc::ServerContext* context, const ::uniset3::GetSensorIOInfoParams* request, ::uniset3::SensorIOInfo* response) override;
            virtual ::grpc::Status getSensorSeq(::grpc::ServerContext* context, const ::uniset3::GetSensorSeqParams* request, ::uniset3::SensorIOInfoSeq* response) override;
            virtual ::grpc::Status setOutputSeq(::grpc::ServerContext* context, const ::uniset3::SetOutputParams* request, ::uniset3::IDSeq* response) override;
            virtual ::grpc::Status getTimeChange(::grpc::ServerContext* context, const ::uniset3::GetTimeChangeParams* request, ::uniset3::ShortIOInfo* response) override;
            virtual ::grpc::Status getSensors(::grpc::ServerContext* context, const ::uniset3::GetSensorsParams* request, ::uniset3::ShortMapSeq* response) override;
            // --------------------------
            bool add( const std::shared_ptr<IOController>& obj );
            bool remove( const std::shared_ptr<IOController>& obj );
            // --------------------------
            size_t size() const;    // количество подчинённых объектов
            // ---------------
            void lock(); // запретить модификацию (добавление/удаление новых объектов)

        private:
            std::unordered_map<ObjectId, std::shared_ptr<IOController> > olist;
            mutable uniset3::uniset_rwmutex olistMutex;
            std::atomic_bool is_running = { false };
    };
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
#endif
