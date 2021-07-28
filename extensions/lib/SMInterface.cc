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
        google::protobuf::Int64Value request;
        request.set_value(id);
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
    if( ic )
    {
        SensorIOInfoSeq seq;

        BEG_FUNC1(SMInterface::getSensorsMap)
        auto status = ic->getSensorsMap(&ctx, &empty, &seq);

        if( !status.ok() )
            throw SystemError(status.error_message());

        return seq;
        END_FUNC(SMInterface::getSensorsMap)
    }

    BEG_FUNC(SMInterface::getSensorsMap)
    auto shm = IOController_i::NewStub(oref);
    SensorIOInfoSeq seq;
    auto status = shm->getSensorsMap(&clictx, empty, &seq);

    if( !status.ok() )
        throw SystemError(status.error_message());

    return seq;
    END_FUNC(SMInterface::getSensorsMap)
}
// --------------------------------------------------------------------------
uniset3::ThresholdsListSeq SMInterface::getThresholdsList()
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::getThresholdsList)
        ThresholdsListSeq seq;
        auto status = ic->getThresholdsList(&ctx, &empty, &seq);

        if( !status.ok() )
            throw SystemError(status.error_message());

        return seq;
        END_FUNC(SMInterface::getThresholdsList)
    }

    BEG_FUNC(SMInterface::getThresholdsList)
    auto shm = IONotifyController_i::NewStub(oref);
    ThresholdsListSeq seq;
    auto status = shm->getThresholdsList(&clictx, empty, &seq);

    if( !status.ok() )
        throw SystemError(status.error_message());

    return seq;
    END_FUNC(SMInterface::getThresholdsList)
}
// --------------------------------------------------------------------------
void SMInterface::setUndefinedState( const uniset3::SensorInfo& si, bool undefined,
                                     uniset3::ObjectId sup_id )
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::setUndefinedState)
        SetUndefinedParams request;
        request.set_id(si.id());
        request.set_undefined(undefined);
        request.set_sup_id(sup_id);
        auto status = ic->setUndefinedState(&ctx, &request, &empty);

        if( !status.ok() )
            throw SystemError(status.error_message());

        return;
        END_FUNC(SMInterface::setUndefinedState)
    }

    BEG_FUNC(SMInterface::setUndefinedState)
    auto shm = IOController_i::NewStub(oref);
    SetUndefinedParams request;
    request.set_id(si.id());
    request.set_undefined(undefined);
    request.set_sup_id(sup_id);
    auto status = shm->setUndefinedState(&clictx, request, &empty);

    if( !status.ok() )
        throw SystemError(status.error_message());

    return;
    END_FUNC(SMInterface::setUndefinedState)
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
void SMInterface::localSetUndefinedState( IOController::IOStateList::iterator& it,
        bool undefined,
        uniset3::ObjectId sid )
{
    //    CHECK_IC_PTR(localSetUndefinedState)
    if( !ic )
    {
        uniset3::SensorInfo si;
        si.set_id(sid);
        si.set_node(ui->getConf()->getLocalNode());
        setUndefinedState(si, undefined, myid);
        return;
    }

    ic->localSetUndefinedState(it, undefined, sid);
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
#ifndef DISABLE_REST_API
std::string SMInterface::apiRequest( const std::string& query )
{
    if( ic )
    {
        BEG_FUNC1(SMInterface::apiRequest)
        google::protobuf::StringValue req;
        req.set_value(query);
        google::protobuf::StringValue resp;
        auto status = ic->request(&ctx, &req, &resp);

        if( !status.ok() )
            throw SystemError(status.error_message());

        return resp.value();
        END_FUNC(SMInterface::apiRequest)
    }

    BEG_FUNC(SMInterface::apiRequest)
    return ui->apiRequest(shmID, query, ui->getConf()->getLocalNode());
    END_FUNC(SMInterface::apiRequest)
}
#endif
// --------------------------------------------------------------------------
