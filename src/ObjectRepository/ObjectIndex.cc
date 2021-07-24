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
/*!
    \todo Добавить проверку на предельный номер id
*/
// -----------------------------------------------------------------------------------------
#include <string>
#include "ObjectIndex.h"
#include "Configuration.h"
// -----------------------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// -----------------------------------------------------------------------------------------

std::string ObjectIndex::getNameById( const ObjectId id ) const noexcept
{
    return getMapName(id);
}
// -----------------------------------------------------------------------------------------
std::string ObjectIndex::getNodeName(const ObjectId id) const noexcept
{
    return getNameById(id);
}
// -----------------------------------------------------------------------------------------
ObjectId ObjectIndex::getNodeId(const std::string& name) const noexcept
{
    return getIdByName(name);
}
// -----------------------------------------------------------------------------------------
std::string ObjectIndex::getBaseName( const std::string& fname ) noexcept
{
    std::string::size_type pos = fname.rfind('/');

    try
    {
        if( pos != std::string::npos )
            return fname.substr(pos + 1);
    }
    catch(...) {}

    return fname;
}
// -----------------------------------------------------------------------------------------
void ObjectIndex::initLocalNode( const ObjectId nodeid ) noexcept
{
    nmLocalNode = getMapName(nodeid);
}
// -----------------------------------------------------------------------------------------
std::string ObjectIndex::getSectionName(const std::string& fullName, const std::string& brk)
{
    string::size_type pos = fullName.rfind(brk);

    if( pos == string::npos )
        return "";

    return fullName.substr(0, pos);
}
// -----------------------------------------------------------------------------------------
std::string ObjectIndex::getShortName(const std::string& fname, const std::string& brk)
{
    string::size_type pos = fname.rfind(brk);

    if( pos == string::npos )
        return fname;

    return fname.substr( pos + 1, fname.length() );
}
// -----------------------------------------------------------------------------------------
