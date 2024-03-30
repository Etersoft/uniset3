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
#include "IONotifyControllerProxy.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    //-------------------------------------------------------------------------
    IONotifyControllerProxy::IONotifyControllerProxy()
    {

    }
    // -------------------------------------------------------------------------
    IONotifyControllerProxy::~IONotifyControllerProxy()
    {

    }
    // -------------------------------------------------------------------------
    ::grpc::Status IONotifyControllerProxy::askSensor(::grpc::ServerContext* context, const ::uniset3::AskParams* request, ::google::protobuf::Empty* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->askSensor(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;
    }
    // -------------------------------------------------------------------------
    ::grpc::Status IONotifyControllerProxy::askSensorsSeq(::grpc::ServerContext* context, const ::uniset3::AskSeqParams* request, ::uniset3::IDSeq* response)
    {
        grpc::Status status;

        for( const auto& o : olist )
        {
            status = o.second->askSensorsSeq(context, request, response);

            if( status.ok() )
                return status;
        }

        // возвращаем последнюю ошибку
        return status;
    }
    // -------------------------------------------------------------------------
    bool IONotifyControllerProxy::add( const std::shared_ptr<IONotifyController>& obj )
    {
        if( is_running )
            throw SystemError("(IONotifyControllerProxy): proxy already activated! Don't call 'add' after that");

        uniset_rwmutex_wrlock lock(olistMutex);
        auto i = olist.find(obj->getId());

        if( i == olist.end() )
            olist.emplace(obj->getId(), obj);

        return true;
    }
    // -------------------------------------------------------------------------
    bool IONotifyControllerProxy::remove( const std::shared_ptr<IONotifyController>& obj )
    {
        if( is_running )
            throw SystemError("(IONotifyControllerProxy): proxy already activated! Don't call 'remove' after that");

        uniset_rwmutex_wrlock lock(olistMutex);
        auto i = olist.find(obj->getId());

        if( i != olist.end() )
        {
            olist.erase(i);
            return true;
        }

        return false;
    }
    // -------------------------------------------------------------------------
    void IONotifyControllerProxy::lock()
    {
        is_running = true;
    }
    // -------------------------------------------------------------------------
    size_t IONotifyControllerProxy::size() const
    {
        uniset_rwmutex_rlock lock(olistMutex);
        return olist.size();
    }
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
