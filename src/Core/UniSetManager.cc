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
    olistMutex("UniSetManager_olistMutex"),
    mlistMutex("UniSetManager_mlistMutex")
{
}
// ------------------------------------------------------------------------------------------
UniSetManager::UniSetManager( ObjectId id ):
    UniSetObject(id)
{
    olistMutex.setName(myname + "_olistMutex");
    mlistMutex.setName(myname + "_mlistMutex");
}

// ------------------------------------------------------------------------------------------
UniSetManager::~UniSetManager()
{
    olist.clear();
    mlist.clear();
}
// ------------------------------------------------------------------------------------------
std::shared_ptr<UniSetManager> UniSetManager::get_mptr()
{
    return std::dynamic_pointer_cast<UniSetManager>(get_ptr());
}
// ------------------------------------------------------------------------------------------
::grpc::Status UniSetManager::getType(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::StringValue* response)
{
    response->set_value("UniSetManager");
    return ::grpc::Status::OK;
}
// ------------------------------------------------------------------------------------------
bool UniSetManager::add( const std::shared_ptr<UniSetObject>& obj )
{
    auto m = std::dynamic_pointer_cast<UniSetManager>(obj);

    if( m )
        return addManager(m);

    return addObject(obj);
}
// ------------------------------------------------------------------------------------------
bool UniSetManager::remove( const std::shared_ptr<UniSetObject>& obj )
{
    auto m = std::dynamic_pointer_cast<UniSetManager>(obj);

    if( m )
        return removeManager(m);

    return removeObject(obj);
}
// ------------------------------------------------------------------------------------------
bool UniSetManager::addObject( const std::shared_ptr<UniSetObject>& obj )
{

    {
        //lock
        uniset_rwmutex_wrlock lock(olistMutex);
        auto li = find(olist.begin(), olist.end(), obj);

        if( li == olist.end() )
        {
            uinfo << myname << "(activator): добавляем объект " << obj->getName() << endl;
            olist.push_back(obj);
        }
    } // unlock
    return true;
}

// ------------------------------------------------------------------------------------------
bool UniSetManager::removeObject( const std::shared_ptr<UniSetObject>& obj )
{
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
        }
    } // unlock

    return true;
}

// ------------------------------------------------------------------------------------------
/*!
 *    Функция работы со списком менеджеров
*/
void UniSetManager::managers( OManagerCommand cmd )
{
    uinfo << myname << "(managers): mlist.size=" << mlist.size() << " cmd=" << cmd  << endl;
    {
        //lock
        uniset_rwmutex_rlock lock(mlistMutex);

        for( const auto& li : mlist )
        {
            if( !li )
                continue;

            try
            {
                switch(cmd)
                {
                    case initial:
                        li->init( get_mptr() );
                        break;

                    case activ:
                        li->activate();
                        break;

                    case deactiv:
                        li->deactivate();
                        break;

                    default:
                        break;
                }
            }
            catch( const std::exception& ex )
            {
                ostringstream err;
                err << myname << "(managers): " << ex.what() << endl
                    << " Не смог зарегистрировать (разрегистрировать) объект -->"
                    << li->getName();

                ucrit << err.str() << endl;

                if( cmd == activ )
                {
                    cerr << err.str();
                    std::terminate();
                }
            }
        }
    } // unlock
}
// ------------------------------------------------------------------------------------------
/*!
 *    Функция работы со списком объектов.
*/
void UniSetManager::objects(OManagerCommand cmd)
{
    uinfo << myname << "(objects): olist.size="
          << olist.size() << " cmd=" << cmd  << endl;
    {
        //lock
        uniset_rwmutex_rlock lock(olistMutex);

        for( const auto& li : olist )
        {
            if( !li )
                continue;

            try
            {
                switch(cmd)
                {
                    case initial:
                        li->init(get_mptr());
                        break;

                    case activ:
                        li->activate();
                        break;

                    case deactiv:
                        li->deactivate();
                        break;

                    default:
                        break;
                }
            }
            catch( const std::exception& ex )
            {
                ostringstream err;
                err << myname << "(objects): " << ex.what() << endl;
                err << myname << "(objects): не смог зарегистрировать (разрегистрировать) объект -->" << li->getName() << endl;

                ucrit << err.str();

                if( cmd == activ )
                {
                    cerr << err.str();
                    std::terminate();
                }
            }
        }
    } // unlock
}
// ------------------------------------------------------------------------------------------
void UniSetManager::initGRPC( const std::weak_ptr<UniSetManager>& rmngr )
{
    auto m = rmngr.lock();

    if( !m )
    {
        ostringstream err;
        err << myname << "(initGRPC): failed weak_ptr !!";
        ucrit << err.str() << endl;
        throw uniset3::SystemError(err.str());
    }

    // Инициализация самого менеджера и его подобъектов
    UniSetObject::init(rmngr);
    objects(initial);
    managers(initial);
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
    managers(activ);
    objects(activ);
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
    // именно в такой последовательности!
    objects(deactiv);
    managers(deactiv);
    return true;
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
const std::shared_ptr<UniSetManager> UniSetManager::findManager( const string& name ) const
{
    uniset_rwmutex_rlock lock(mlistMutex);

    for( auto&& m : mlist )
    {
        if( m->getName() == name )
            return m;
    }

    return nullptr;
}
// ------------------------------------------------------------------------------------------
const std::shared_ptr<UniSetObject> UniSetManager::deepFindObject( const string& name ) const
{
    {
        auto obj = findObject(name);

        if( obj )
            return obj;
    }

    auto man = findManager(name);

    if( man )
    {
        auto obj = dynamic_pointer_cast<UniSetObject>(man);
        return obj;
    }

    // ищем в глубину у каждого менеджера
    for( const auto& m : mlist )
    {
        auto obj = m->deepFindObject(name);

        if( obj )
            return obj;
    }

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

    // добавить рекурсивно по менеджерам
    for( const auto& m : mlist )
    {
        // вызываем рекурсивно
        m->getAllObjectsList(vec, lim);
    }
}
// ------------------------------------------------------------------------------------------
//virtual ::grpc::Status getObjectsInfo(::grpc::ServerContext* context, const ::uniset3::ObjectsInfoParams* request, ::uniset3::SimpleInfoSeq* response) override;

::grpc::Status UniSetManager::broadcast(::grpc::ServerContext* context, const ::uniset3::messages::TransportMessage* request, ::google::protobuf::Empty* response)
{
    // себя не забыть...
    //    push(msg);

    // Всем объектам...
    {
        //lock
        uniset_rwmutex_rlock lock(olistMutex);

        for( auto&& o : olist )
            o->push(context, request, response);
    } // unlock

    // Всем менеджерам....
    {
        //lock
        uniset_rwmutex_rlock lock(mlistMutex);

        for( auto&& m : mlist )
        {
            m->push(context, request, response);
            m->broadcast(context, request, response);
        }
    } // unlock

    return ::grpc::Status::OK;
}

// ------------------------------------------------------------------------------------------
bool UniSetManager::addManager( const std::shared_ptr<UniSetManager>& child )
{
    {
        //lock
        uniset_rwmutex_wrlock lock(mlistMutex);

        // Проверка на совпадение
        auto it = find(mlist.begin(), mlist.end(), child);

        if(it == mlist.end() )
        {
            mlist.push_back( child );
            uinfo << myname << ": добавляем менеджер " << child->getName() << endl;
        }
        else
            uwarn << myname << ": попытка повторного добавления объекта " << child->getName() << endl;
    } // unlock

    return true;
}

// ------------------------------------------------------------------------------------------
bool UniSetManager::removeManager( const std::shared_ptr<UniSetManager>& child )
{
    {
        //lock
        uniset_rwmutex_wrlock lock(mlistMutex);
        mlist.remove(child);
    } // unlock

    return true;
}

// ------------------------------------------------------------------------------------------
::grpc::Status UniSetManager::getObjectsInfo(::grpc::ServerContext* context, const ::uniset3::ObjectsInfoParams* request, ::uniset3::SimpleInfoSeq* response)
{
    // получаем у самого менеджера
    ::google::protobuf::StringValue oinf;
    ::google::protobuf::StringValue params;
    params.set_value(request->userparams());
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

    // а далее у его менеджеров (рекурсивно)
    for( const auto& m : mlist )
    {
        m->getObjectsInfo(context, request, response);

        if( response->objects().size() >= request->maxlength() )
            return ::grpc::Status::OK;
    }

    return ::grpc::Status::OK;
}
// ------------------------------------------------------------------------------------------
void UniSetManager::apply_for_objects( OFunction f )
{
    for( const auto& o : olist )
        f(o);
}
// ------------------------------------------------------------------------------------------
void UniSetManager::apply_for_managers(UniSetManager::MFunction f)
{
    for( const auto& m : mlist )
        f(m);
}
// ------------------------------------------------------------------------------------------
size_t UniSetManager::objectsCount() const
{
    size_t res = olist.size() + mlist.size();

    for( const auto& i : mlist )
        res += i->objectsCount();

    return res;
}
// ------------------------------------------------------------------------------------------
std::ostream& uniset3::operator<<(std::ostream& os, UniSetManager::OManagerCommand& cmd )
{
    // { deactiv, activ, initial, term };
    if( cmd == uniset3::UniSetManager::deactiv )
        return os << "deactivate";

    if( cmd == uniset3::UniSetManager::activ )
        return os << "activate";

    if( cmd == uniset3::UniSetManager::initial )
        return os << "init";

    return os << "unkwnown";
}
