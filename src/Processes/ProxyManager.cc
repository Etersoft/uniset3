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
#include <sstream>
#include "Exceptions.h"
#include "Debug.h"
#include "Configuration.h"
#include "ProxyManager.h"
#include "PassiveObject.h"
// -------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// -------------------------------------------------------------------------

ProxyManager::~ProxyManager()
{
}

ProxyManager::ProxyManager( uniset3::ObjectId id ):
    UniSetObject(id)
{
    uin = ui;
}


// -------------------------------------------------------------------------
void ProxyManager::attachObject( PassiveObject* po, uniset3::ObjectId id )
{
    if( id == DefaultObjectId )
    {
        uwarn << myname << "(attachObject): попытка добавить объект с id="
              << DefaultObjectId << " PassiveObject=" << po->getName() << endl;

        return;
    }

    auto it = omap.find(id);

    if( it == omap.end() )
        omap.emplace(id, po);
}
// -------------------------------------------------------------------------
void ProxyManager::detachObject( uniset3::ObjectId id )
{
    auto it = omap.find(id);

    if( it != omap.end() )
        omap.erase(it);
}
// -------------------------------------------------------------------------
bool ProxyManager::activateObject()
{
    bool ret = UniSetObject::activateObject();

    if( !ret )
        return false;

    // Регистрируемся от имени объектов
    for( const auto& it : omap )
    {
        try
        {
            auto oref = getRef();
            oref.set_id(it.first);

            for( size_t i = 0; i < 2; i++ )
            {
                try
                {
                    ulogrep << myname << "(registered): попытка "
                            << i + 1 << " регистриую (id=" << it.first << ") "
                            << " (pname=" << it.second->getName() << ") "
                            << uniset_conf()->oind->getNameById(it.first) << endl;

                    ui->registered(oref, true);
                    break;
                }
                catch( uniset3::ObjectNameAlready& ex )
                {
                    ucrit << myname << "(registered): СПЕРВА РАЗРЕГИСТРИРУЮ (ObjectNameAlready)" << endl;

                    try
                    {
                        ui->unregister(it.first);
                    }
                    catch( const uniset3::Exception& ex )
                    {
                        ucrit << myname << "(unregistered): " << ex << endl;
                    }
                }
            }
        }
        catch( const std::exception& ex )
        {
            cerr << myname << "(activate): " << ex.what() << endl << flush;
            uterminate();
        }
    }

    return ret;
}
// -------------------------------------------------------------------------
bool ProxyManager::deactivateObject()
{
    for( PObjectMap::const_iterator it = omap.begin(); it != omap.end(); ++it )
    {
        try
        {
            ui->unregister(it->first);
        }
        catch( const uniset3::Exception& ex )
        {
            ucrit << myname << "(activate): " << ex << endl;
        }
    }

    return UniSetObject::deactivateObject();
}
// -------------------------------------------------------------------------
void ProxyManager::processingMessage( const uniset3::umessage::TransportMessage* msg )
{
    try
    {
        switch(msg->header().type())
        {
            case umessage::mtSysCommand:
                allMessage(msg);
                break;

            default:
            {
                auto it = omap.find(msg->header().consumer());

                if( it != omap.end() )
                    it->second->processingMessage(msg);
                else
                    ucrit << myname << "(processingMessage): не найден объект "
                          << " consumer= " << msg->header().consumer() << endl;
            }
            break;
        }
    }
    catch( const uniset3::Exception& ex )
    {
        ucrit << myname << "(processingMessage): " << ex << endl;
    }
}
// -------------------------------------------------------------------------
void ProxyManager::allMessage( const uniset3::umessage::TransportMessage* msg )
{
    for( const auto& o : omap )
    {
        try
        {
            o.second->processingMessage(msg);
        }
        catch( const uniset3::Exception& ex )
        {
            ucrit << myname << "(allMessage): " << ex << endl;
        }
    }
}
// -------------------------------------------------------------------------
