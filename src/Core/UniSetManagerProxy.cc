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
#include "UniSetManagerProxy.h"
//---------------------------------------------------------------------------
namespace uniset3
{
    UniSetManagerProxy::UniSetManagerProxy()
    {

    }
    // -------------------------------------------------------------------------
    UniSetManagerProxy::~UniSetManagerProxy()
    {

    }
    // -------------------------------------------------------------------------
    ::grpc::Status UniSetManagerProxy::broadcast(::grpc::ServerContext* context, const ::uniset3::umessage::TransportMessage* request, ::google::protobuf::Empty* response)
    {
        auto i = mlist.find(request->consumer());

        if( i == mlist.end() )
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

        return i->second->push(context, request, response);
    }
    // -------------------------------------------------------------------------
    ::grpc::Status UniSetManagerProxy::getObjectsInfo(::grpc::ServerContext* context, const ::uniset3::ObjectsInfoParams* request, ::uniset3::SimpleInfoSeq* response)
    {
        auto i = mlist.find(request->id());

        if( i == mlist.end() )
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "");

        return i->second->getObjectsInfo(context, request, response);
    }
    // -------------------------------------------------------------------------
    bool UniSetManagerProxy::add( const std::shared_ptr<UniSetManager>& obj )
    {
        if( is_running )
            throw SystemError("(UniSetManagerProxy): proxy already activated! Don't call 'add' after that");

        uniset_rwmutex_wrlock lock(mlistMutex);
        auto i = mlist.find(obj->getId());

        if( i == mlist.end() )
            mlist.emplace(obj->getId(), obj);

        return true;
    }
    // -------------------------------------------------------------------------
    bool UniSetManagerProxy::remove( const std::shared_ptr<UniSetManager>& obj )
    {
        if( is_running )
            throw SystemError("(UniSetManagerProxy): proxy already activated! Don't call 'remove' after that");

        uniset_rwmutex_wrlock lock(mlistMutex);
        auto i = mlist.find(obj->getId());

        if( i != mlist.end() )
        {
            mlist.erase(i);
            return true;
        }

        return false;
    }
    // -------------------------------------------------------------------------
    void UniSetManagerProxy::lock()
    {
        is_running = true;
    }
    //---------------------------------------------------------------------------
    size_t UniSetManagerProxy::size() const
    {
        uniset_rwmutex_rlock lock(mlistMutex);
        return mlist.size();
    }
    // -------------------------------------------------------------------------
} // end of uniset3 namespace
