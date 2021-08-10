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
#include <iomanip>
#include "ProxyManager.h"
#include "PassiveObject.h"
#include "Configuration.h"
// ------------------------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// ------------------------------------------------------------------------------------------
PassiveObject::PassiveObject():
    mngr(0),
    id(uniset3::DefaultObjectId)
{

}

PassiveObject::PassiveObject( uniset3::ObjectId id ):
    mngr(0),
    id(id)
{
    const string myfullname = uniset_conf()->oind->getNameById(id);
    myname = ObjectIndex::getShortName(myfullname);
}

PassiveObject::PassiveObject( ObjectId id, ProxyManager* mngr ):
    mngr(mngr),
    id(id)
{
    const string myfullname = uniset_conf()->oind->getNameById(id);
    myname = ObjectIndex::getShortName(myfullname);

    if( mngr )
        mngr->attachObject(this, id);
}

// ------------------------------------------------------------------------------------------

PassiveObject::~PassiveObject()
{
}

// ------------------------------------------------------------------------------------------
void PassiveObject::setID( uniset3::ObjectId id_ )
{
    id = id_;
}
// ------------------------------------------------------------------------------------------
void PassiveObject::init( ProxyManager* _mngr )
{
    if( _mngr == mngr || !_mngr )
        return;

    // если уже инициализирован другим mngr (см. конструктор)
    if( mngr )
        mngr->detachObject(id);

    mngr = _mngr;
    mngr->attachObject(this, id);
}

// ------------------------------------------------------------------------------------------
void PassiveObject::processingMessage( const uniset3::umessage::TransportMessage* msg )
{
    try
    {
        if( msg->data().Is<umessage::SensorMessage>() )
        {
            umessage::SensorMessage m;

            if( !msg->data().UnpackTo(&m) )
            {
                ucrit << myname << "(processingMessage): SensorInfo: parse error" << endl;
                return;
            }

            sensorInfo(&m);
            return;
        }

        if( msg->data().Is<umessage::TimerMessage>() )
        {
            umessage::TimerMessage m;

            if( !msg->data().UnpackTo(&m) )
            {
                ucrit << myname << "(processingMessage): TimerInfo: parse error" << endl;
                return;
            }

            timerInfo(&m);
            return;
        }

        if( msg->data().Is<umessage::SystemMessage>() )
        {
            umessage::SystemMessage m;

            if( !msg->data().UnpackTo(&m) )
            {
                ucrit << myname << "(processingMessage): SysCommand: parse error" << endl;
                return;
            }

            sysCommand(&m);
            return;
        }

        if( msg->data().Is<umessage::TextMessage>() )
        {
            umessage::TextMessage m;

            if( !msg->data().UnpackTo(&m) )
            {
                ucrit << myname << "(processingMessage): TextMessage: parse error" << endl;
                return;
            }

            onTextMessage( &m );
            return;
        }
    }
    catch( const std::exception& ex )
    {
        ucrit << myname << "(processingMessage): " << ex.what() << endl;
    }
}

// -------------------------------------------------------------------------
void PassiveObject::sysCommand( const uniset3::umessage::SystemMessage* sm )
{
    switch( sm->cmd() )
    {
        case umessage::SystemMessage::StartUp:
            askSensors(uniset3::UIONotify);
            break;

        case umessage::SystemMessage::FoldUp:
        case umessage::SystemMessage::Finish:
            askSensors(uniset3::UIODontNotify);
            break;

        case umessage::SystemMessage::WatchDog:
        case umessage::SystemMessage::LogRotate:
            break;

        default:
            break;
    }
}
// -------------------------------------------------------------------------
