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
//--------------------------------------------------------------------------
#ifndef SMInterface_H_
#define SMInterface_H_
//--------------------------------------------------------------------------
#include <string>
#include <memory>
#include <atomic>
#include "UniSetTypes.h"
#include "Mutex.h"
#include "IONotifyController.h"
#include "UInterface.h"
// --------------------------------------------------------------------------
namespace uniset3
{
    // --------------------------------------------------------------------------
    class SMInterface
    {
        public:

            SMInterface( uniset3::ObjectId _shmID, const std::shared_ptr<UInterface>& ui,
                         uniset3::ObjectId myid, const std::shared_ptr<IONotifyController> ic = nullptr );
            ~SMInterface();

            void setValue ( uniset3::ObjectId, long value );
            void setUndefinedState( const uniset3::SensorInfo& si, bool undefined, uniset3::ObjectId supplier );

            long getValue ( uniset3::ObjectId id );

            void askSensor( uniset3::ObjectId id, uniset3::UIOCommand cmd,
                            uniset3::ObjectId backid = uniset3::DefaultObjectId );

            uniset3::SensorIOInfoSeq getSensorsMap();

            void localSetValue( IOController::IOStateList::iterator& it,
                                uniset3::ObjectId sid,
                                long newvalue, uniset3::ObjectId sup_id );

            long localGetValue( IOController::IOStateList::iterator& it,
                                uniset3::ObjectId sid );

            // специальные функции
            IOController::IOStateList::iterator ioEnd();
            void initIterator( IOController::IOStateList::iterator& it );

            bool exists();
            bool waitSMready( int msec, int pause = 5000 );
            bool waitSMworking( uniset3::ObjectId, int msec, int pause = 3000 );
            bool waitSMreadyWithCancellation( int msec, std::atomic_bool& cancelFlag, int pause = 5000 );

            inline bool isLocalwork() const noexcept
            {
                return (ic == NULL);
            }
            inline uniset3::ObjectId ID() const noexcept
            {
                return myid;
            }
            inline const std::shared_ptr<IONotifyController> SM() noexcept
            {
                return ic;
            }
            inline uniset3::ObjectId getSMID() const noexcept
            {
                return shmID;
            }

        protected:
            const std::shared_ptr<IONotifyController> ic;
            const std::shared_ptr<UInterface> ui;
            std::shared_ptr<UInterface::ORefInfo> oref;
            grpc::ServerContext ctx;
            google::protobuf::Empty empty;
            uniset3::ObjectId shmID;
            uniset3::ObjectId myid;
            uniset3::uniset_rwmutex shmMutex;
    };
    // --------------------------------------------------------------------------
} // end of namespace uniset3
//--------------------------------------------------------------------------
#endif
