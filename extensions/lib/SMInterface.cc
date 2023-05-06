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
// -------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "Exceptions.h"
#include "SMInterface.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// --------------------------------------------------------------------------
#define BEG_FUNC(name) \
    try \
    {     \
        auto conf = ui->getConf(); \
        uniset_rwmutex_wrlock l(shmMutex); \
        std::unique_ptr<IONotifyController_i::Stub> shm;\
        for( unsigned int i=0; i<conf->getRepeatCount(); i++)\
        {\
            if( !oref ) \
                oref = ui->resolve( shmID, conf->getLocalNode() ); \
            \
            if( !oref ) \
                continue;\

#define BEG_FUNC1(name) \
    try \
    {     \
        auto conf = ui->getConf(); \
        uniset_rwmutex_wrlock l(shmMutex); \
        if(true) \
        { \

#define END_FUNC(fname) \
    oref = nullptr;\
    msleep(conf->getRepeatTimeout());    \
    } \
    } \
    catch( const std::exception& ex ) \
    { \
        uwarn << "(" << __STRING(fname) << "): " << ex.what() << endl; \
    } \
    oref = nullptr; \
    throw uniset3::TimeOut(); \

#define CHECK_IC_PTR(fname) \
    if( !ic )  \
    { \
        uwarn << "(" << __STRING(fname) << "): function NOT DEFINED..." << endl; \
        throw uniset3::TimeOut(); \
    } \

// --------------------------------------------------------------------------
SMInterface::SMInterface( uniset3::ObjectId _shmID, const std::shared_ptr<UInterface>& _ui,
                          uniset3::ObjectId _myid, const std::shared_ptr<IONotifyController> ic ):
    ic(ic),
    ui(_ui),
    shmID(_shmID),
    myid(_myid)
{
    if( shmID == DefaultObjectId )
        throw uniset3::SystemError("(SMInterface): Unknown shmID!" );
}
// --------------------------------------------------------------------------
SMInterface::~SMInterface()
{

}
// --------------------------------------------------------------------------
void SMInterface::setValue( uniset3::ObjectId id, long value )
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::setValue)
        SetValueParams request;
        request.set_id(id);
        request.set_value(value);
        request.set_sup_id(myid);
        ic->setValue(&ctx, &request, &empty);
        return;
        END_FUNC(SMInterface::setValue)
    }

    uniset3::SensorInfo si;
    si.set_id(id);
    si.set_node(ui->getConf()->getLocalNode());

    BEG_FUNC1(SMInterface::setValue)
    ui->setValue(si, value, myid);
    return;
    END_FUNC(SMInterface::setValue)
}
// --------------------------------------------------------------------------
long SMInterface::getValue( uniset3::ObjectId id )
{

    if( ic )
    {
        BEG_FUNC1(SMInterface::getValue)
        GetValueParams request;
        request.set_id(id);
        google::protobuf::Int64Value response;
        auto status = ic->getValue(&ctx, &request, &response);

        if( !status.ok() )
            throw SystemError(status.error_message());

        return response.value();
        END_FUNC(SMInterface::getValue)
    }

    BEG_FUNC1(SMInterface::getValue)
    return ui->getValue(id);
    END_FUNC(SMInterface::getValue)
}
// --------------------------------------------------------------------------
void SMInterface::askSensor( uniset3::ObjectId id, uniset3::UIOCommand cmd, uniset3::ObjectId backid )
{
    ConsumerInfo ci;
    ci.set_id((backid == DefaultObjectId) ? myid : backid);
    ci.set_node(ui->getConf()->getLocalNode());

    if( ic )
    {
        BEG_FUNC1(SMInterface::askSensor)
        AskParams p;
        p.set_sid(id);
        *(p.mutable_ci()) = ci;
        p.set_cmd(cmd);
        auto status = ic->askSensor(&ctx, &p, &empty);

        if( !status.ok() )
            throw SystemError(status.error_message());

        return;
        END_FUNC(SMInterface::askSensor)
    }

    BEG_FUNC1(SMInterface::askSensor)
    ui->askRemoteSensor(id, cmd, conf->getLocalNode(), ci.id());
    return;
    END_FUNC(SMInterface::askSensor)
}
// --------------------------------------------------------------------------
uniset3::SensorIOInfoSeq SMInterface::getSensorsMap()
{
    GetSensorsMapParams request;

    if( ic )
    {
        SensorIOInfoSeq seq;

        BEG_FUNC1(SMInterface::getSensorsMap)
        request.set_id(ic->getId());
        auto status = ic->getSensorsMap(&ctx, &request, &seq);

        if( !status.ok() )
            throw SystemError(status.error_message());

        return seq;
        END_FUNC(SMInterface::getSensorsMap)
    }

    BEG_FUNC(SMInterface::getSensorsMap)
    auto shm = IOController_i::NewStub(oref->c);
    SensorIOInfoSeq seq;
    request.set_id(shmID);
    grpc::ClientContext ctx;
    oref->addMetaData(ctx);
    auto status = shm->getSensorsMap(&ctx, request, &seq);

    if( !status.ok() )
        throw SystemError(status.error_message());

    return seq;
    END_FUNC(SMInterface::getSensorsMap)
}
// --------------------------------------------------------------------------
void SMInterface::freezeValue( uniset3::ObjectId id, bool set,
                               long value, uniset3::ObjectId sup_id )
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::freezeValue)
        FreezeValueParams request;
        request.set_id(id);
        request.set_value(value);
        request.set_sup_id(myid);
        request.set_set(set);
        ic->freezeValue(&ctx, &request, &empty);
        return;
        END_FUNC(SMInterface::freezeValue)
    }

    BEG_FUNC(SMInterface::freezeValue)
    uniset3::SensorInfo si;
    si.set_id(id);
    si.set_node(ui->getConf()->getLocalNode());
    ui->freezeValue(si, set, value, sup_id);
    return;
    END_FUNC(SMInterface::freezeValue)
}
// --------------------------------------------------------------------------
bool SMInterface::exists()
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::exist)
        return ic->isExists();
        END_FUNC(SMInterface::exist)
    }

    return ui->isExists(shmID);
}
// --------------------------------------------------------------------------
IOController::IOStateList::iterator SMInterface::ioEnd()
{
    CHECK_IC_PTR(ioEnd)
    return ic->ioEnd();
}
// --------------------------------------------------------------------------
void SMInterface::localSetValue( IOController::IOStateList::iterator& it,
                                 uniset3::ObjectId sid,
                                 long value, uniset3::ObjectId sup_id )
{
    if( !ic )
        return setValue(sid, value);

    ic->localSetValueIt(it, sid, value, sup_id);
}
// --------------------------------------------------------------------------
long SMInterface::localGetValue( IOController::IOStateList::iterator& it, uniset3::ObjectId sid )
{
    if( !ic )
        return getValue( sid );

    //    CHECK_IC_PTR(localGetValue)
    return ic->localGetValue(it, sid);
}
// --------------------------------------------------------------------------
void SMInterface::initIterator( IOController::IOStateList::iterator& it )
{
    if( ic )
        it = ic->ioEnd();
}
// --------------------------------------------------------------------------
bool SMInterface::waitSMready( int ready_timeout, int pmsec )
{
    std::atomic_bool cancelFlag = { false };
    return waitSMreadyWithCancellation(ready_timeout, cancelFlag, pmsec);
}
// --------------------------------------------------------------------------
bool SMInterface::waitSMworking( uniset3::ObjectId sid, int msec, int pmsec )
{
    PassiveTimer ptSMready(msec);
    bool sm_ready = false;

    while( !ptSMready.checkTime() && !sm_ready )
    {
        try
        {
            getValue(sid);
            sm_ready = true;
            break;
        }
        catch(...) {}

        msleep(pmsec);
    }

    return sm_ready;
}
// --------------------------------------------------------------------------
bool SMInterface::waitSMworkingWithCancellation( uniset3::ObjectId sid, int ready_timeout, std::atomic_bool& cancelFlag, int pmsec )
{
    PassiveTimer ptSMready(ready_timeout);
    bool sm_ready = false;

    while( !ptSMready.checkTime() && !sm_ready && !cancelFlag )
    {
        try
        {
            getValue(sid);
            sm_ready = true;
            break;
        }
        catch(...) {}

        msleep(pmsec);
    }

    return sm_ready;
}
// --------------------------------------------------------------------------
bool SMInterface::waitSMreadyWithCancellation(int ready_timeout, std::atomic_bool& cancelFlag, int pmsec)
{
    PassiveTimer ptSMready(ready_timeout);
    bool sm_ready = false;

    while( !ptSMready.checkTime() && !sm_ready && !cancelFlag )
    {
        try
        {
            sm_ready = exists();

            if( sm_ready )
                break;
        }
        catch(...) {}

        msleep(pmsec);
    }

    return sm_ready;
}
// --------------------------------------------------------------------------
