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
// -----------------------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "Exceptions.h"
#include "Configuration.h"
#include "ObjectIndex_idXML.h"
// -----------------------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// -----------------------------------------------------------------------------------------
ObjectIndex_idXML::ObjectIndex_idXML( const string& xmlfile )
{
    auto xml = make_shared<UniXML>();
    //    try
    //    {
    xml->open(xmlfile);
    build(xml);
    //    }
    //    catch(...){}
}
// -----------------------------------------------------------------------------------------
ObjectIndex_idXML::ObjectIndex_idXML( const shared_ptr<UniXML>& xml )
{
    build(xml);
}
// -----------------------------------------------------------------------------------------
ObjectIndex_idXML::~ObjectIndex_idXML()
{
}
// -----------------------------------------------------------------------------------------
ObjectId ObjectIndex_idXML::getIdByName( const string& name ) const noexcept
{
    try
    {
        auto it = mok.find(name);

        if( it != mok.end() )
            return it->second;
    }
    catch(...) {}

    return DefaultObjectId;
}
// -----------------------------------------------------------------------------------------
string ObjectIndex_idXML::getMapName( const ObjectId id ) const noexcept
{
    try
    {
        auto it = omap.find(id);

        if( it != omap.end() )
            return it->second.repName;
    }
    catch(...) {}

    return "";
}
// -----------------------------------------------------------------------------------------
string ObjectIndex_idXML::getTextName( const ObjectId id ) const noexcept
{
    try
    {
        auto it = omap.find(id);

        if( it != omap.end() )
            return it->second.textName;
    }
    catch(...) {}

    return "";
}
// -----------------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, ObjectIndex_idXML& oi )
{
    return oi.printMap(os);
}
// -----------------------------------------------------------------------------------------
std::ostream& ObjectIndex_idXML::printMap( std::ostream& os ) const noexcept
{
    os << "size: " << omap.size() << endl;

    for( const auto& it: omap )
    {
        if( it.second.repName.empty() )
            continue;

        os  << setw(5) << it.second.id << "  "
            << setw(45) << it.second.name
            << "  " << it.second.textName << endl;
    }

    return os;
}
// -----------------------------------------------------------------------------------------
void ObjectIndex_idXML::build( const shared_ptr<UniXML>& xml )
{
    read_section(xml, "sensors");
    read_section(xml, "objects");
    read_section(xml, "controllers");
    read_section(xml, "services");
    read_nodes(xml, "nodes");
}
// ------------------------------------------------------------------------------------------
void ObjectIndex_idXML::read_section( const std::shared_ptr<UniXML>& xml, const std::string& sec )
{
    string secRoot = xml->getProp( xml->findNode(xml->getFirstNode(), "RootSection"), "name");

    if( secRoot.empty() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build):: не нашли параметр RootSection в конф. файле ";
        ucrit << msg.str() << endl;
        throw SystemError(msg.str());
    }

    xmlNode* root( xml->findNode(xml->getFirstNode(), sec) );

    if( !root )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build): не нашли корневого раздела " << sec;
        throw NameNotFound(msg.str());
    }

    // Считываем список элементов
    UniXML::iterator it(root);

    if( !it.goChildren() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build): не удалось перейти к списку элементов " << sec;
        throw NameNotFound(msg.str());
    }

    string secname = xml->getProp(root, "section");

    if( secname.empty() )
        secname = xml->getProp(root, "name");

    if( secname.empty() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build): у секции " << sec << " не указано свойство 'name' ";
        throw NameNotFound(msg.str());
    }

    // прибавим корень
    secname = secRoot + "/" + secname + "/";

    for( ; it.getCurrent(); it.goNext() )
    {
        ObjectInfo inf;
        inf.id = it.getPIntProp("id", DefaultObjectId);

        if( inf.id == DefaultObjectId )
        {
            ostringstream msg;
            msg << "(ObjectIndex_idXML::build): НЕ УКАЗАН id для " << it.getProp("name")
                << " секция " << sec;
            throw NameNotFound(msg.str());
        }

        // name
        inf.name = it.getProp("name");
        ostringstream n;
        n << secname << inf.name;
        inf.repName = n.str();

        inf.textName = xml->getProp(it, "textname");
        inf.xmlnode = it;

        mok.emplace(inf.repName, inf.id);
        omap.emplace(inf.id, std::move(inf));
    }
}
// ------------------------------------------------------------------------------------------
void ObjectIndex_idXML::read_nodes( const std::shared_ptr<UniXML>& xml, const std::string& sec )
{
    xmlNode* root( xml->findNode(xml->getFirstNode(), sec) );

    if( !root )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build): не нашли корневого раздела " << sec;
        throw NameNotFound(msg.str());
    }

    // Считываем список элементов
    UniXML::iterator it(root);

    if( !it.goChildren() )
    {
        ostringstream msg;
        msg << "(ObjectIndex_idXML::build): не удалось перейти к списку элементов "
            << " секция " << sec;
        throw NameNotFound(msg.str());
    }

    for( ; it.getCurrent(); it.goNext() )
    {
        ObjectInfo inf;

        inf.id = it.getIntProp("id");

        if( inf.id <= 0 )
        {
            ostringstream msg;
            msg << "(ObjectIndex_idXML::build): НЕ УКАЗАН id для " << it.getProp("name") << endl;
            throw NameNotFound(msg.str());
        }

        inf.name = it.getProp("name");
        inf.repName = inf.name;
        inf.textName = xml->getProp(it, "textname");
        inf.xmlnode = it;

        mok.emplace(inf.repName, inf.id);
        omap.emplace(inf.id, std::move(inf));
    }
}
// ------------------------------------------------------------------------------------------
const ObjectInfo* ObjectIndex_idXML::getObjectInfo( const ObjectId id ) const noexcept
{
    try
    {
        auto it = omap.find(id);

        if( it != omap.end() )
            return &(it->second);
    }
    catch(...) {}

    return nullptr;
}
// ------------------------------------------------------------------------------------------
const ObjectInfo* ObjectIndex_idXML::getObjectInfo( const std::string& name ) const noexcept
{
    try
    {
        auto it = mok.find(name);

        if( it != mok.end() )
            return getObjectInfo(it->second);
    }
    catch(...) {}

    return nullptr;
}
// ------------------------------------------------------------------------------------------
