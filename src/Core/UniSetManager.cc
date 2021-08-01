/*
 * Copyright (c) 2015 Pavel Vainerman.
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
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include <cstdlib>
#include <sstream>
#include <list>
#include <string>
#include <functional>
#include <algorithm>

#include "Configuration.h"
#include "Exceptions.h"
#include "UInterface.h"
#include "UniSetManager.h"
#include "Debug.h"

// ------------------------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// ------------------------------------------------------------------------------------------
UniSetManager::UniSetManager():
    UniSetObject(uniset3::DefaultObjectId),
    olistMutex("UniSetManager_olistMutex")
{
}
// ------------------------------------------------------------------------------------------
UniSetManager::UniSetManager( ObjectId id ):
    UniSetObject(id)
{
    olistMutex.setName(myname + "_olistMutex");
}

// ------------------------------------------------------------------------------------------
UniSetManager::~UniSetManager()
{
    olist.clear();
}
// ------------------------------------------------------------------------------------------
::grpc::Status UniSetManager::getType(::grpc::ServerContext* context, const ::uniset3::GetTypeParams* request, ::google::protobuf::StringValue* response)
{
    response->set_value("UniSetManager");
    return ::grpc::Status::OK;
}
// ------------------------------------------------------------------------------------------
bool UniSetManager::add( const std::shared_ptr<UniSetObject>& obj )
{
    uniset_rwmutex_wrlock lock(olistMutex);
    auto li = find(olist.begin(), olist.end(), obj);

    if( li == olist.end() )
    {
        uinfo << myname << "(activator): добавляем объект " << obj->getName() << endl;
        olist.push_back(obj);
    }

    return true;
}
// ------------------------------------------------------------------------------------------
bool UniSetManager::remove( const std::shared_ptr<UniSetObject>& obj )
{
    //lock
    uniset_rwmutex_wrlock lock(olistMutex);
    auto li = find(olist.begin(), olist.end(), obj);

    if( li != olist.end() )
    {
        uinfo << myname << "(activator): удаляем объект " << obj->getName() << endl;

        try
        {
            if( obj )
                obj->deactivate();
        }
        catch( const std::exception& ex )
        {
            uwarn << myname << "(removeObject): " << ex.what() << endl;
        }
        catch(...) {}

        olist.erase(li);
        return true;
    }

    return false;
}
// ------------------------------------------------------------------------------------------
bool UniSetManager::init( const std::string& svcAddr )
{
    // Инициализация самого менеджера и его подобъектов
    if( !UniSetObject::init(svcAddr) )
        return false;

    uniset_rwmutex_rlock lock(olistMutex);

    for( auto&& o : olist)
        o->init(svcAddr);

    return true;
}
// ------------------------------------------------------------------------------------------
/*!
 *    Регистрация объекта и всех его подобъектов в репозитории.
 *    \note Только после этого он (и они) становятся доступны другим процессам
*/
bool UniSetManager::activateObject()
{
    uinfo << myname << "(activateObjects):  активизирую объекты" << endl;
    UniSetObject::activateObject();
    uniset_rwmutex_rlock lock(olistMutex);

    for( auto&& o : olist)
        o->activateObject();

    return true;
}
// ------------------------------------------------------------------------------------------
/*!
 *    Удаление объекта и всех его подобъектов из репозитория.
 *    \note Объект становится недоступен другим процессам
*/
bool UniSetManager::deactivateObject()
{
    uinfo << myname << "(deactivateObjects):  деактивизирую объекты" << endl;
    uniset_rwmutex_rlock lock(olistMutex);

    for( auto&& o : olist)
    {
        try
        {
            o->deactivateObject();
        }
        catch( std::exception& ex )
        {
            ucrit << "(deactivateObject): " << ex.what() << endl;
        }
        catch(...) {}
    }

    return UniSetObject::deactivateObject();
}
// ------------------------------------------------------------------------------------------
const std::shared_ptr<UniSetObject> UniSetManager::findObject( const string& name ) const
{
    uniset_rwmutex_rlock lock(olistMutex);

    for( auto&& o : olist )
    {
        if( o->getName() == name )
            return o;
    }

    return nullptr;
}
// ------------------------------------------------------------------------------------------
const std::shared_ptr<UniSetObject> UniSetManager::deepFindObject( const string& name ) const
{
    auto obj = findObject(name);

    if( obj )
        return obj;

    return nullptr;
}
// ------------------------------------------------------------------------------------------
void UniSetManager::getAllObjectsList( std::vector<std::shared_ptr<UniSetObject> >& vec, size_t lim )
{
    // добавить себя
    vec.push_back(get_ptr());

    // добавить подчинённые объекты
    for( const auto& o : olist )
    {
        vec.push_back(o);

        if( lim > 0 && vec.size() >= lim )
            return;
    }
}
// ------------------------------------------------------------------------------------------
::grpc::Status UniSetManager::broadcast(::grpc::ServerContext* context, const ::uniset3::umessage::TransportMessage* request, ::google::protobuf::Empty* response)
{
    // Всем объектам...
    {
        //lock
        uniset_rwmutex_rlock lock(olistMutex);

        for( auto&& o : olist )
            o->push(context, request, response);
    } // unlock

    return ::grpc::Status::OK;
}

// ------------------------------------------------------------------------------------------
::grpc::Status UniSetManager::getObjectsInfo(::grpc::ServerContext* context, const ::uniset3::ObjectsInfoParams* request, ::uniset3::SimpleInfoSeq* response)
{
    // получаем у самого менеджера
    ::google::protobuf::StringValue oinf;
    GetInfoParams params;
    params.set_params(request->userparams());
    grpc::Status st = getInfo(context, &params, &oinf);

    if( !st.ok() )
        return st;

    auto seq = response->add_objects();
    seq->set_info(oinf.value());
    seq->set_id(getId());

    if( response->objects().size() > request->maxlength() )
        return ::grpc::Status::OK;

    ::google::protobuf::StringValue inf;

    for( const auto& o : olist )
    {
        try
        {
            auto st = getInfo(context, &params, &inf);

            if( !st.ok() )
                inf.set_value(st.error_message());

            auto seq = response->add_objects();
            seq->set_info(inf.value());
            seq->set_id(o->getId());

            if( response->objects().size() >= request->maxlength() )
                return ::grpc::Status::OK;
        }
        catch( const std::exception& ex )
        {
            uwarn << myname << "(getObjectsInfo): " << ex.what() << endl;
        }
        catch(...)
        {
            uwarn << myname << "(getObjectsInfo): не смог получить у объекта "
                  << uniset_conf()->oind->getNameById( o->getId() ) << " информацию" << endl;
        }
    }

    return ::grpc::Status::OK;
}
// ------------------------------------------------------------------------------------------
size_t UniSetManager::objectsCount() const
{
    uniset_rwmutex_rlock lock(olistMutex);
    return olist.size();
}
// ------------------------------------------------------------------------------------------
