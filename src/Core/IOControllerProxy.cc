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
#include "Mutex.h"
#include "IOControllerProxy.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    //---------------------------------------------------------------------------
    IOControllerProxy::IOControllerProxy()
    {

    }
    //---------------------------------------------------------------------------
    IOControllerProxy::~IOControllerProxy()
    {

    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::getValue(::grpc::ServerContext* context, const ::uniset3::GetValueParams* request, ::google::protobuf::Int64Value* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->getValue(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;
    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::setValue(::grpc::ServerContext* context, const ::uniset3::SetValueParams* request, ::google::protobuf::Empty* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->setValue(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;
    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::freezeValue(::grpc::ServerContext* context, const ::uniset3::FreezeValueParams* request, ::google::protobuf::Empty* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->freezeValue(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;
    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::getIOType(::grpc::ServerContext* context, const ::uniset3::GetIOTypeParams* request, ::uniset3::RetIOType* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->getIOType(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;
    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::getRawValue(::grpc::ServerContext* context, const ::uniset3::GetRawValueParams* request, ::google::protobuf::Int64Value* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->getRawValue(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;

    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::calibrate(::grpc::ServerContext* context, const ::uniset3::CalibrateParams* request, ::google::protobuf::Empty* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->calibrate(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;

    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::getCalibrateInfo(::grpc::ServerContext* context, const ::uniset3::GetCalibrateInfoParams* request, ::uniset3::CalibrateInfo* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->getCalibrateInfo(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;
    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::getSensorsMap(::grpc::ServerContext* context, const ::uniset3::GetSensorsMapParams* request, ::uniset3::SensorIOInfoSeq* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->getSensorsMap(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;

    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::getSensorIOInfo(::grpc::ServerContext* context, const ::uniset3::GetSensorIOInfoParams* request, ::uniset3::SensorIOInfo* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->getSensorIOInfo(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;

    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::getSensorSeq(::grpc::ServerContext* context, const ::uniset3::GetSensorSeqParams* request, ::uniset3::SensorIOInfoSeq* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->getSensorSeq(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;

    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::setOutputSeq(::grpc::ServerContext* context, const ::uniset3::SetOutputParams* request, ::uniset3::IDSeq* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->setOutputSeq(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;

    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::getTimeChange(::grpc::ServerContext* context, const ::uniset3::GetTimeChangeParams* request, ::uniset3::ShortIOInfo* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->getTimeChange(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;

    }
    //---------------------------------------------------------------------------
    ::grpc::Status IOControllerProxy::getSensors(::grpc::ServerContext* context, const ::uniset3::GetSensorsParams* request, ::uniset3::ShortMapSeq* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->getSensors(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;

    }
    //---------------------------------------------------------------------------
    bool IOControllerProxy::add( const std::shared_ptr<IOController>& obj )
    {
        if( is_running )
            throw SystemError("(IOControllerProxy): proxy already activated! Don't call 'add' after that");

        uniset_rwmutex_wrlock lock(olistMutex);
        auto i = olist.find(obj->getId());

        if( i == olist.end() )
            olist.emplace(obj->getId(), obj);

        return true;
    }
    //---------------------------------------------------------------------------
    bool IOControllerProxy::remove( const std::shared_ptr<IOController>& obj )
    {
        if( is_running )
            throw SystemError("(IOControllerProxy): proxy already activated! Don't call 'remove' after that");

        uniset_rwmutex_wrlock lock(olistMutex);
        auto i = olist.find(obj->getId());

        if( i != olist.end() )
        {
            olist.erase(i);
            return true;
        }

        return false;
    }
    //---------------------------------------------------------------------------
    void IOControllerProxy::lock()
    {
        is_running = true;
    }
    //---------------------------------------------------------------------------
    size_t IOControllerProxy::size() const
    {
        uniset_rwmutex_rlock lock(olistMutex);
        return olist.size();
    }
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
