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
#include "ConfiguratorProxy.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    //-------------------------------------------------------------------------
    ConfiguratorProxy::ConfiguratorProxy()
    {
    }

    // -------------------------------------------------------------------------
    ConfiguratorProxy::~ConfiguratorProxy()
    {
    }

    // -------------------------------------------------------------------------
    ::grpc::Status ConfiguratorProxy::setParams(::grpc::ServerContext* context, const ::uniset3::configurator::Params* request, ::uniset3::configurator::Params* response)
    {
        auto id = request->id();
        auto srvIt = context->client_metadata().find(uniset3::MetaDataServiceId);

        if (srvIt != context->client_metadata().end())
            id = uniset3::uni_atoi(srvIt->second.data());

        auto i = olist.find(id);

        if (i == olist.end())
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

        return i->second->setParams(context, request, response);
    }
    // -------------------------------------------------------------------------
    ::grpc::Status ConfiguratorProxy::getParams(::grpc::ServerContext* context, const ::uniset3::configurator::Params* request, ::uniset3::configurator::Params* response)
    {
        auto id = request->id();
        auto srvIt = context->client_metadata().find(uniset3::MetaDataServiceId);

        if (srvIt != context->client_metadata().end())
            id = uniset3::uni_atoi(srvIt->second.data());

        auto i = olist.find(id);

        if (i == olist.end())
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

        return i->second->getParams(context, request, response);
    }
    // -------------------------------------------------------------------------
    ::grpc::Status ConfiguratorProxy::loadConfig(::grpc::ServerContext* context, const ::uniset3::configurator::ConfigCmdParams* request, ::grpc::ServerWriter< ::uniset3::configurator::Config>* writer)
    {
        auto id = request->id();
        auto srvIt = context->client_metadata().find(uniset3::MetaDataServiceId);

        if (srvIt != context->client_metadata().end())
            id = uniset3::uni_atoi(srvIt->second.data());

        auto i = olist.find(id);

        if (i == olist.end())
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

        return i->second->loadConfig(context, request, writer);

    }
    // -------------------------------------------------------------------------
    ::grpc::Status ConfiguratorProxy::reloadConfig(::grpc::ServerContext* context, const ::uniset3::configurator::ConfigCmdParams* request, ::google::protobuf::Empty* response)
    {
        auto id = request->id();
        auto srvIt = context->client_metadata().find(uniset3::MetaDataServiceId);

        if (srvIt != context->client_metadata().end())
            id = uniset3::uni_atoi(srvIt->second.data());

        auto i = olist.find(id);

        if (i == olist.end())
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

        return i->second->reloadConfig(context, request, response);

    }
    // -------------------------------------------------------------------------
    bool ConfiguratorProxy::add(const std::shared_ptr<UniSetObject>& obj)
    {
        if (is_running)
            throw SystemError("(ConfiguratorProxy): proxy already activated! Don't call 'add' after that");

        uniset_rwmutex_wrlock lock(olistMutex);
        auto i = olist.find(obj->getId());

        if (i == olist.end())
            olist.emplace(obj->getId(), obj);

        return true;
    }

    // -------------------------------------------------------------------------
    bool ConfiguratorProxy::remove(const std::shared_ptr<UniSetObject>& obj)
    {
        if (is_running)
            throw SystemError("(ConfiguratorProxy): proxy already activated! Don't call 'remove' after that");

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
    void ConfiguratorProxy::lock()
    {
        is_running = true;
    }

    // -------------------------------------------------------------------------
    size_t ConfiguratorProxy::size() const
    {
        uniset_rwmutex_rlock lock(olistMutex);
        return olist.size();
    }
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
// -------------------------------------------------------------------------
