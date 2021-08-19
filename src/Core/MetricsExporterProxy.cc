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
#include "MetricsExporterProxy.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    //-------------------------------------------------------------------------
    MetricsExporterProxy::MetricsExporterProxy()
    {
    }

    // -------------------------------------------------------------------------
    MetricsExporterProxy::~MetricsExporterProxy()
    {
    }

    // -------------------------------------------------------------------------
    ::grpc::Status MetricsExporterProxy::metrics(::grpc::ServerContext* context, const ::uniset3::metrics::MetricsParams* request, ::uniset3::metrics::Metrics* response)
    {
        auto id = request->id();
        auto srvIt = context->client_metadata().find(uniset3::MetaDataServiceId);

        if (srvIt != context->client_metadata().end())
            id = uniset3::uni_atoi(srvIt->second.data());

        auto i = olist.find(id);

        if (i == olist.end())
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

        return i->second->metrics(context, request, response);
    }

    // -------------------------------------------------------------------------
    bool MetricsExporterProxy::add(const std::shared_ptr<UniSetObject>& obj)
    {
        if (is_running)
            throw SystemError("(MetricsExporterProxy): proxy already activated! Don't call 'add' after that");

        uniset_rwmutex_wrlock lock(olistMutex);
        auto i = olist.find(obj->getId());

        if (i == olist.end())
            olist.emplace(obj->getId(), obj);

        return true;
    }

    // -------------------------------------------------------------------------
    bool MetricsExporterProxy::remove(const std::shared_ptr<UniSetObject>& obj)
    {
        if (is_running)
            throw SystemError("(MetricsExporterProxy): proxy already activated! Don't call 'remove' after that");

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
    void MetricsExporterProxy::lock()
    {
        is_running = true;
    }

    // -------------------------------------------------------------------------
    size_t MetricsExporterProxy::size() const
    {
        uniset_rwmutex_rlock lock(olistMutex);
        return olist.size();
    }
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
// -------------------------------------------------------------------------
