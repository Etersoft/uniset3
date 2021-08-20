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
#include "UniSetTypes.h"
#include "UniSetObjectProxy.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    //-------------------------------------------------------------------------
    UniSetObjectProxy::UniSetObjectProxy()
    {
    }

    // -------------------------------------------------------------------------
    UniSetObjectProxy::~UniSetObjectProxy()
    {
    }

    // -------------------------------------------------------------------------
    ::grpc::Status UniSetObjectProxy::getType(::grpc::ServerContext* context, const ::uniset3::GetTypeParams* request,
            ::google::protobuf::StringValue* response)
    {
        auto id = request->id();
        auto srvIt = context->client_metadata().find(uniset3::MetaDataServiceId);

        if (srvIt != context->client_metadata().end())
            id = uniset3::uni_atoi(srvIt->second.data());

        // не лочим, т.к. после запуска уже не меняется (см. is_running)
        auto i = olist.find(id);

        if (i == olist.end())
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

        return i->second->getType(context, request, response);
    }

    // -------------------------------------------------------------------------
    ::grpc::Status UniSetObjectProxy::getInfo(::grpc::ServerContext* context, const ::uniset3::GetInfoParams* request,
            ::google::protobuf::StringValue* response)
    {
        auto id = request->id();
        auto srvIt = context->client_metadata().find(uniset3::MetaDataServiceId);

        if (srvIt != context->client_metadata().end())
            id = uniset3::uni_atoi(srvIt->second.data());

        // не лочим, т.к. после запуска уже не меняется (см. is_running)
        auto i = olist.find(id);

        if (i == olist.end())
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

        return i->second->getInfo(context, request, response);
    }
    // -------------------------------------------------------------------------
    ::grpc::Status UniSetObjectProxy::exists(::grpc::ServerContext* context, const ::uniset3::ExistsParams* request,
            ::google::protobuf::BoolValue* response)
    {
        auto id = request->id();
        auto srvIt = context->client_metadata().find(uniset3::MetaDataServiceId);

        if (srvIt != context->client_metadata().end())
            id = uniset3::uni_atoi(srvIt->second.data());

        // не лочим, т.к. после запуска уже не меняется (см. is_running)
        auto i = olist.find(id);

        if (i == olist.end())
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

        return i->second->exists(context, request, response);
    }

    // -------------------------------------------------------------------------
    ::grpc::Status
    UniSetObjectProxy::push(::grpc::ServerContext* context, const ::uniset3::umessage::TransportMessage* request,
                            ::google::protobuf::Empty* response)
    {
        auto id = request->consumer();
        auto srvIt = context->client_metadata().find(uniset3::MetaDataServiceId);

        if (srvIt != context->client_metadata().end())
            id = uniset3::uni_atoi(srvIt->second.data());

        // не лочим, т.к. после запуска уже не меняется (см. is_running)
        auto i = olist.find(id);

        if (i == olist.end())
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

        return i->second->push(context, request, response);
    }

    // -------------------------------------------------------------------------
    bool UniSetObjectProxy::add(const std::shared_ptr<UniSetObject>& obj)
    {
        if (is_running)
            throw SystemError("(UniSetObjectProxy): proxy already activated! Don't call 'add' after that");

        uniset_rwmutex_wrlock lock(olistMutex);
        auto i = olist.find(obj->getId());

        if (i == olist.end())
            olist.emplace(obj->getId(), obj);

        return true;
    }

    // -------------------------------------------------------------------------
    bool UniSetObjectProxy::remove(const std::shared_ptr<UniSetObject>& obj)
    {
        if (is_running)
            throw SystemError("(UniSetObjectProxy): proxy already activated! Don't call 'remove' after that");

        uniset_rwmutex_wrlock lock(olistMutex);
        auto i = olist.find(obj->getId());

        if (i != olist.end())
        {
            olist.erase(i);
            return true;
        }

        return false;
    }

    // -------------------------------------------------------------------------
    void UniSetObjectProxy::lock()
    {
        is_running = true;
    }

    // -------------------------------------------------------------------------
    size_t UniSetObjectProxy::size() const
    {
        uniset_rwmutex_rlock lock(olistMutex);
        return olist.size();
    }
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
// -------------------------------------------------------------------------
