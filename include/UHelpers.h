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
#ifndef UHelpers_H_
#define UHelpers_H_
// --------------------------------------------------------------------------
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
namespace uniset3
{
    // Шаблон для "универсальной инициализации объекта (процесса)".
    // Использование:
    // auto m = make_object<MyClass>("ObjectId","secname",...);
    // --
    // Где MyClass должен содержать конструктор MyClass( const ObjetctId id, xmlNode* cnode, ...any args.. );
    // ---------------
    // Если secname задан, то ищется: <secname name="ObjectId" ....>
    // Если secname не задан, то: <idname name="idname" ...>
    //----------------
    template<typename T, typename... _Args>
    std::shared_ptr<T> make_object( const std::string& idname, const std::string& secname, _Args&& ... __args )
    {
        auto conf = uniset3::uniset_conf();
        uniset3::ObjectId id = conf->getObjectID(idname);

        if( id == uniset3::DefaultObjectId )
            throw uniset3::SystemError("(make_object<" + std::string(typeid(T).name()) + ">): Not found ID for '" + idname + "'");

        auto xml = conf->getConfXML();
        std::string s( (secname.empty() ? idname : secname) );
        xmlNode* cnode = conf->findNode(xml->getFirstNode(), s, idname);

        if( cnode == 0 )
            throw uniset3::SystemError("(make_object<" + std::string(typeid(T).name()) + ">): Not found xmlnode <" + s + " name='" + idname + "' ... >");

        std::shared_ptr<T> obj = std::make_shared<T>(id, cnode, std::forward<_Args>(__args)...);

        if (obj == nullptr)
            throw uniset3::SystemError("(make_object<T>  == nullptr" + std::string(typeid(T).name()));

        return obj;
    }
    // -----------------------------------------------------------------------------
    // версия с указанием начального xml-узла, с которого ведётся поиск xmlNode
    // а ID берётся из поля name="" у найденного xmlnode.
    template<typename T, typename... _Args>
    std::shared_ptr<T> make_object_x( xmlNode* root, const std::string& secname, _Args&& ... __args )
    {
        auto conf = uniset3::uniset_conf();
        auto xml = conf->getConfXML();
        xmlNode* cnode = conf->findNode(root, secname, "");

        if( cnode == 0 )
            throw uniset3::SystemError("(make_object_x<" + std::string(typeid(T).name()) + ">): Not found xmlnode <" + secname + " ... >");

        std::string idname = conf->getProp(cnode, "name");
        uniset3::ObjectId id = conf->getObjectID(idname);

        if( id == uniset3::DefaultObjectId )
            throw uniset3::SystemError("(make_object_x<" + std::string(typeid(T).name()) + ">): Not found ID for '" + idname + "'");

        return std::make_shared<T>(id, cnode, std::forward<_Args>(__args)...);

    }
    // -----------------------------------------------------------------------------
    // Просто обёртка для удобства вывода сообщений об ошибке в лог "объекта"..
    // "по задумке" позволяет не загромождать код..
    // T - тип создаваемого объекта
    // M - (master) - класс который создаёт объект (подразумевается, что он UniSetManager)
    // Использование
    // auto m = make_child_object<MyClass,MyMasterClass>(master, "ObjectId","secname",...);
    template<typename T, typename M, typename... _Args>
    std::shared_ptr<T> make_child_object( M* m, const std::string& idname, const std::string& secname, _Args&& ... __args )
    {
        try
        {
            m->log()->info() << m->getName() << "(" << __FUNCTION__ << "): " << "create " << idname << "..." << std::endl;
            auto o = uniset3::make_object<T>(idname, secname, std::forward<_Args>(__args)...);
            m->add(o);
            m->logAgregator()->add(o->logAgregator());
            return o;
        }
        catch( const uniset3::Exception& ex )
        {
            m->log()->crit() << m->getName() << "(" << __FUNCTION__ << "): " << "(create " << idname << "): " << ex << std::endl;
            throw;
        }
    }
    // -----------------------------------------------------------------------------
    // Версия использующая make_object_x<>
    template<typename T, typename M, typename... _Args>
    std::shared_ptr<T> make_child_object_x( M* m, xmlNode* root, const std::string& secname, _Args&& ... __args )
    {
        try
        {
            auto o = uniset3::make_object_x<T>(root, secname, std::forward<_Args>(__args)...);
            m->add(o);
            m->logAgregator()->add(o->logAgregator());
            return o;
        }
        catch( const uniset3::Exception& ex )
        {
            m->log()->crit() << m->getName() << "(" << __FUNCTION__ << "): " << "(create " << std::string(typeid(T).name()) << "): " << ex << std::endl;
            throw;
        }
    }

    // ---------------------------------------------------------------
    template<class TMessage>
    uniset3::umessage::TransportMessage to_transport( const TMessage& m )
    {
        uniset3::umessage::TransportMessage tm;
        tm.set_priority(m.header().priority());
        tm.set_consumer(m.header().consumer());
        tm.set_supplier(m.header().supplier());
        tm.mutable_data()->PackFrom(m);
        return tm;
    }
    // ---------------------------------------------------------------
    template<class MessageType>
    uniset3::umessage::TransportMessage makeMessage( uniset3::umessage::Priority prior = uniset3::umessage::mpMedium )
    {
        MessageType m;
        m.mutable_header()->set_priority(prior);
        auto ts = uniset3::now_to_uniset_timespec();
        *(m.mutable_header()->mutable_ts()) = ts;
        m.mutable_header()->set_node(uniset3::uniset_conf()->getLocalNode());
        //    m.mutable_header()->set_supplier(XXX);
        //    m.mutable_header()->set_consumer(XXX);
        return m;
    }
    // -------------------------------------------------------------------------------------
} // endof namespace uniset3
// -----------------------------------------------------------------------------------------
#endif // UHelpers_H_
