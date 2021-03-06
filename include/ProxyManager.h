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
 * \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#ifndef ProxyManager_H_
#define ProxyManager_H_
//---------------------------------------------------------------------------
#include <unordered_map>
#include <memory>
#include "UniSetObject.h"

//----------------------------------------------------------------------------
namespace uniset3
{
    //----------------------------------------------------------------------------
    class PassiveObject;
    //----------------------------------------------------------------------------

    /*! \class ProxyManager
     *    Менеджер пассивных объектов, который выступает вместо них во всех внешних связях...
     *  В целом связка ProxyManager-PassiveObject является DEPRECATED и лучше её не использовать.
     *
     *  \todo Перейти на shared_ptr, weak_ptr для взаимодействия с PassiveObject
     *  своё взаимодействие "менеджер-объекты" исходя из условий "бизнес"-задачи
     *
     * DEPRECATED
    */
    class ProxyManager:
        public UniSetObject
    {

        public:
            ProxyManager( uniset3::ObjectId id );
            ~ProxyManager();

            void attachObject( PassiveObject* po, uniset3::ObjectId id );
            void detachObject( uniset3::ObjectId id );

            std::shared_ptr<UInterface> uin;

        protected:
            ProxyManager();
            virtual void processingMessage( const uniset3::umessage::TransportMessage* msg ) override;
            virtual void allMessage( const uniset3::umessage::TransportMessage* msg );

            virtual bool activateObject() override;
            virtual bool deactivateObject() override;

        private:
            typedef std::unordered_map<uniset3::ObjectId, PassiveObject*> PObjectMap;
            PObjectMap omap;
    };
    // -------------------------------------------------------------------------
} // end of uniset namespace
//----------------------------------------------------------------------------------------
#endif // ProxyManager
//----------------------------------------------------------------------------------------
