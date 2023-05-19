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
#include <cmath>
#include "UInterface.h"
#include "IOController.h"
#include "Debug.h"
#include "UHelpers.h"
// ------------------------------------------------------------------------------------------
using namespace uniset3;
using namespace uniset3::umessage;
using namespace std;
// ------------------------------------------------------------------------------------------
IOController::IOController():
    ioMutex("ioMutex"),
    isPingDBServer(true)
{
}

// ------------------------------------------------------------------------------------------
IOController::IOController(ObjectId id):
    UniSetManager(id),
    ioMutex(string(uniset_conf()->oind->getMapName(id)) + "_ioMutex"),
    isPingDBServer(true)
{
    auto conf = uniset_conf();

    if( conf )
        dbserverID = conf->getDBServer();
}

// ------------------------------------------------------------------------------------------
IOController::~IOController()
{
}

// ------------------------------------------------------------------------------------------
std::string IOController::getStrType() const
{
    return "IOController";
}
// ------------------------------------------------------------------------------------------
bool IOController::activateObject()
{
    bool res = UniSetManager::activateObject();
    sensorsRegistration();

    // Начальная инициализация
    activateInit();

    return res;
}
// ------------------------------------------------------------------------------------------
bool IOController::deactivateObject()
{
    if( !isActive() )
        return true;

    sensorsUnregistration();
    return UniSetManager::deactivateObject();
}
// ------------------------------------------------------------------------------------------
void IOController::sensorsUnregistration()
{
    // Разрегистрируем аналоговые датчики
    for( const auto& li : ioList )
    {
        try
        {
            ioUnRegistration( li.second->sinf.si().id() );
        }
        catch( const uniset3::Exception& ex )
        {
            ucrit << myname << "(sensorsUnregistration): " << ex << endl;
        }
    }
}
// ------------------------------------------------------------------------------------------
IOController::InitSignal IOController::signal_init()
{
    return sigInit;
}
// ------------------------------------------------------------------------------------------
void IOController::activateInit()
{
    for( auto&& io : ioList )
    {
        try
        {
            auto s = io.second;

            // Проверка зависимостей
            if( s->sinf.depend_sid() != DefaultObjectId )
            {
                auto d_it = myiofind(s->sinf.depend_sid());

                if( d_it != myioEnd() )
                    s->checkDepend( d_it->second, this);
            }

            sigInit.emit(s, this);
        }
        catch( const uniset3::Exception& ex )
        {
            cerr << myname << "(activateInit): " << ex << endl << flush;
            //std::terminate();
            uterminate();
        }
    }
}
// ------------------------------------------------------------------------------------------
::grpc::Status IOController::getValue(::grpc::ServerContext* context, const ::uniset3::GetValueParams* request, ::google::protobuf::Int64Value* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(getValue): Deadline exceeded or Client cancelled, abandoning.");
    }

    auto li = ioList.end();

    try
    {
        response->set_value(localGetValue(li, request->id()));
        return grpc::Status::OK;
    }
    catch( ... ) {}

    ostringstream err;
    err << "(IOController::getValue): sid=" << request->id() << " not found";
    return grpc::Status(grpc::StatusCode::NOT_FOUND, err.str());
}
// ------------------------------------------------------------------------------------------
long IOController::localGetValue( IOController::IOStateList::iterator& li, const uniset3::ObjectId sid )
{
    if( li == ioList.end() )
    {
        if( sid != DefaultObjectId )
            li = ioList.find(sid);
    }

    if( li != ioList.end() )
        return localGetValue(li->second);

    // -------------
    ostringstream err;
    err << myname << "(localGetValue): Not found sensor (" << sid << ") "
        << uniset_conf()->oind->getNameById(sid);

    uinfo << err.str() << endl;
    throw uniset3::NameNotFound(err.str().c_str());
}
// ------------------------------------------------------------------------------------------
long IOController::localGetValue( std::shared_ptr<USensorInfo>& usi )
{
    if( usi )
    {
        uniset_rwmutex_rlock lock(usi->val_lock);
        return usi->sinf.value();
    }

    // -------------
    ostringstream err;
    err << myname << "(localGetValue): Unknown sensor";
    uinfo << err.str() << endl;
    throw uniset3::NameNotFound(err.str().c_str());
}
// ------------------------------------------------------------------------------------------
grpc::Status IOController::freezeValue(::grpc::ServerContext* context, const ::uniset3::FreezeValueParams* request, ::google::protobuf::Empty* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(freezeValue): Deadline exceeded or Client cancelled, abandoning.");
    }

    auto li = ioList.end();

    try
    {
        localFreezeValueIt( li, request->id(), request->set(), request->value(), request->sup_id() );
        return grpc::Status::OK;
    }
    catch( uniset3::IOBadParam& ex )
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, ex.what());
    }
    catch(...) {}

    ostringstream err;
    err << "(IOController::freezeValue): sid=" << request->id() << " unknown exception. Supplier " << request->sup_id();
    return grpc::Status(grpc::StatusCode::INTERNAL, err.str());
}
// ------------------------------------------------------------------------------------------
void IOController::localFreezeValueIt( IOController::IOStateList::iterator& li,
                                       uniset3::ObjectId sid,
                                       bool set,
                                       long value,
                                       uniset3::ObjectId sup_id )
{
    if( sup_id == uniset3::DefaultObjectId )
        sup_id = getId();

    // сохранение текущего состояния
    if( li == ioList.end() )
    {
        if( sid != DefaultObjectId )
            li = ioList.find(sid);
    }

    if( li == ioList.end() )
    {
        ostringstream err;
        err << myname << "(localFreezeValue): Unknown sensor (" << sid << ")"
            << "name: " << uniset_conf()->oind->getNameById(sid);
        throw uniset3::NameNotFound(err.str().c_str());
    }

    localFreezeValue(li->second, set, value, sup_id);
}
// ------------------------------------------------------------------------------------------
void IOController::localFreezeValue( std::shared_ptr<USensorInfo>& usi,
                                     bool set,
                                     long value,
                                     uniset3::ObjectId sup_id )
{
    if( usi->readonly )
    {
        ostringstream err;
        err << myname << "(localFreezeValue): Readonly sensor (" << usi->sinf.si().id() << ")"
            << "name: " << uniset_conf()->oind->getNameById(usi->sinf.si().id());
        throw uniset3::IOBadParam(err.str().c_str());
    }

    ulog4 << myname << "(localFreezeValue): (" << usi->sinf.si().id() << ")"
          << uniset_conf()->oind->getNameById(usi->sinf.si().id())
          << " value=" << value
          << " set=" << set
          << " supplier=" << sup_id
          << endl;

    {
        // выставляем флаг заморозки
        uniset_rwmutex_wrlock lock(usi->val_lock);
        usi->sinf.set_frozen(set);
        usi->frozen_value = (set ? value : usi->sinf.value());
        value = usi->sinf.real_value();
    }

    localSetValue(usi, value, sup_id);
}
// ------------------------------------------------------------------------------------------
grpc::Status IOController::setValue(::grpc::ServerContext* context, const ::uniset3::SetValueParams* request, ::google::protobuf::Empty* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(setValue): Deadline exceeded or Client cancelled, abandoning.");
    }

    auto li = ioList.end();

    try
    {
        localSetValueIt( li, request->id(), request->value(), request->sup_id() );
        return grpc::Status::OK;
    }
    catch( uniset3::IOBadParam& ex )
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, ex.what());
    }
    catch(...) {}

    ostringstream err;
    err << "(IOController::setValue): sid=" << request->id() << " not found. Supplier " << request->sup_id();
    return grpc::Status(grpc::StatusCode::NOT_FOUND, err.str());
}
// ------------------------------------------------------------------------------------------
long IOController::localSetValueIt( IOController::IOStateList::iterator& li,
                                    uniset3::ObjectId sid,
                                    long value, uniset3::ObjectId sup_id )
{
    if( sup_id == uniset3::DefaultObjectId )
        sup_id = getId();

    // сохранение текущего состояния
    if( li == ioList.end() )
    {
        if( sid != DefaultObjectId )
            li = ioList.find(sid);
    }

    if( li == ioList.end() )
    {
        ostringstream err;
        err << myname << "(localSetValue): Unknown sensor (" << sid << ")"
            << "name: " << uniset_conf()->oind->getNameById(sid);
        throw uniset3::NameNotFound(err.str().c_str());
    }

    return localSetValue(li->second, value, sup_id);
}
// ------------------------------------------------------------------------------------------
long IOController::localSetValue( std::shared_ptr<USensorInfo>& usi,
                                  long value, uniset3::ObjectId sup_id )
{
    if( usi->readonly )
    {
        ostringstream err;
        err << myname << "(localSetValue): Readonly sensor (" << usi->sinf.si().id() << ")"
            << "name: " << uniset_conf()->oind->getNameById(usi->sinf.si().id());
        throw uniset3::IOBadParam(err.str().c_str());
    }

    // if( !usi ) - не проверяем, т.к. считаем что это внутренние функции и несуществующий указатель передать не могут
    bool changed = false;
    bool blockChanged = false;
    bool freezeChanged = false;
    long retValue = value;

    {
        // lock
        uniset_rwmutex_wrlock lock(usi->val_lock);
        usi->sinf.set_supplier(sup_id); // запоминаем того кто изменил

        changed = ( usi->sinf.real_value() != value );

        // Выставление запоненного значения (real_value)
        // если снялась блокировка или заморозка
        blockChanged = ( usi->sinf.blocked() != (usi->sinf.value() == usi->d_off_value) );
        freezeChanged = ( usi->sinf.frozen() != (usi->sinf.value() == usi->frozen_value) );

        if( changed || blockChanged || freezeChanged )
        {
            ulog4 << myname << "(localSetValue): (" << usi->sinf.si().id() << ")"
                  << uniset_conf()->oind->getNameById(usi->sinf.si().id())
                  << " newvalue=" << value
                  << " value=" << usi->sinf.value()
                  << " blocked=" << usi->sinf.blocked()
                  << " frozen=" << usi->sinf.frozen()
                  << " real_value=" << usi->sinf.real_value()
                  << " supplier=" << sup_id
                  << endl;

            usi->sinf.set_real_value(value);

            if( usi->sinf.frozen() )
                usi->sinf.set_value(usi->frozen_value);
            else
                usi->sinf.set_value(usi->sinf.blocked() ? usi->d_off_value : value);

            retValue = usi->sinf.value();

            usi->nchanges++; // статистика

            // запоминаем время изменения
            try
            {
                struct timespec tm = uniset3::now_to_timespec();
                auto ts = usi->sinf.mutable_ts();
                ts->set_sec(tm.tv_sec);
                ts->set_nsec(tm.tv_nsec);
            }
            catch( std::exception& ex )
            {
                ucrit << myname << "(localSetValue): setValue (" << usi->sinf.si().id() << ") ERROR: " << ex.what() << endl;
            }
        }
    }    // unlock

    try
    {
        if( changed || blockChanged || freezeChanged )
        {
            uniset_rwmutex_wrlock l(usi->changeMutex);
            usi->sigChange.emit(usi, this);
        }
    }
    catch(...) {}

    try
    {
        if( changed || blockChanged || freezeChanged )
        {
            std::lock_guard<std::mutex> l(siganyMutex);
            sigAnyChange.emit(usi, this);
        }
    }
    catch(...) {}

    return retValue;
}
// ------------------------------------------------------------------------------------------
grpc::Status IOController::getIOType(::grpc::ServerContext* context, const ::uniset3::GetIOTypeParams* request, ::uniset3::RetIOType* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(getIOType): Deadline exceeded or Client cancelled, abandoning.");
    }

    auto li = ioList.find(request->id());

    if( li != ioList.end() )
    {
        response->set_type(li->second->sinf.type());
        return grpc::Status::OK;
    }

    ostringstream err;
    err << myname << "(getIOType): датчик имя: " << uniset_conf()->oind->getNameById(request->id()) << " не найден";
    return grpc::Status(grpc::StatusCode::NOT_FOUND, err.str());
}
// ---------------------------------------------------------------------------
void IOController::ioRegistration( std::shared_ptr<USensorInfo>& usi )
{
    // проверка задан ли контроллеру идентификатор
    if( getId() == DefaultObjectId )
    {
        ostringstream err;
        err << "(IOCOntroller::ioRegistration): КОНТРОЛЛЕРУ НЕ ЗАДАН ObjectId. Регистрация невозможна.";
        uwarn << err.str() << endl;
        throw ResolveNameError(err.str());
    }

    auto conf  = uniset_conf();
    auto sref = getRef();
    sref.set_id(usi->sinf.si().id());
    sref.set_path(conf->getSensorsSection());
    sref.set_type("sensor");
    auto md = sref.mutable_metadata();
    (*md)[uniset3::MetaDataServiceId] = to_string(getId());

    try
    {
        for( size_t i = 0; i < 2; i++ )
        {
            try
            {
                ulogrep << myname
                        << "(ioRegistration): регистрирую "
                        << conf->oind->getNameById(usi->sinf.si().id())
                        << " addr: " << getRef().addr()
                        << endl;

                ui->registered(sref, true);
                return;
            }
            catch( const ObjectNameAlready& ex )
            {
                uwarn << myname << "(asRegistration): ЗАМЕНЯЮ СУЩЕСТВУЮЩИЙ ОБЪЕКТ (ObjectNameAlready)" << endl;
                ui->unregister(usi->sinf.si().id());
            }
        }
    }
    catch( const uniset3::Exception& ex )
    {
        ucrit << myname << "(ioRegistration): " << ex << endl;
    }
}
// ---------------------------------------------------------------------------
void IOController::ioUnRegistration( const uniset3::ObjectId sid )
{
    ui->unregister(sid);
}
// ---------------------------------------------------------------------------
void IOController::setDBServer( const std::shared_ptr<DBServer>& db )
{
    dbserver = db;
}
// ---------------------------------------------------------------------------
void IOController::logging( uniset3::umessage::SensorMessage& sm )
{
    std::lock_guard<std::mutex> l(loggingMutex);

    try
    {
        if( dbserver )
        {
            sm.mutable_header()->set_consumer(dbserverID);
            umessage::TransportMessage tm = to_transport<SensorMessage>(sm);
            grpc::ServerContext ctx;
            dbserver->push(&ctx, &tm, &empty);
            isPingDBServer = true;
            return;
        }

        // значит на этом узле нет DBServer-а
        if( dbserverID == uniset3::DefaultObjectId )
        {
            isPingDBServer = false;
            return;
        }

        sm.mutable_header()->set_consumer(dbserverID);
        umessage::TransportMessage tm = to_transport<SensorMessage>(sm);
        ui->send(std::move(tm), uniset_conf()->getLocalNode());
        isPingDBServer = true;
    }
    catch(...)
    {
        if( isPingDBServer )
        {
            isPingDBServer = false;
            ucrit << myname << "(logging): DBServer unavailable" << endl;
        }
    }
}
// --------------------------------------------------------------------------------------------------------------
void IOController::dumpToDB()
{
    // значит на этом узле нет DBServer-а
    if( dbserverID == uniset3::DefaultObjectId )
        return;

    {
        // lock
        //        uniset_mutex_lock lock(ioMutex, 100);
        for( auto&& usi : ioList )
        {
            auto& s = usi.second;

            if ( !s->sinf.dbignore() )
            {
                umessage::SensorMessage sm( s->makeSensorMessage() );
                logging(sm);
            }
        }
    }    // unlock
}
// --------------------------------------------------------------------------------------------------------------
grpc::Status IOController::getSensorsMap(::grpc::ServerContext* context, const ::uniset3::GetSensorsMapParams* request, ::uniset3::SensorIOInfoSeq* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(getSensorsMap): Deadline exceeded or Client cancelled, abandoning.");
    }

    size_t i = 0;

    for( const auto& it : ioList )
    {
        uniset_rwmutex_rlock lock(it.second->val_lock);
        auto s = response->add_sensors();
        *s = it.second->sinf;
        i++;
    }

    return grpc::Status::OK;
}
// --------------------------------------------------------------------------------------------------------------
uniset3::umessage::Priority IOController::getPriority( const uniset3::ObjectId sid )
{
    auto it = ioList.find(sid);

    if( it != ioList.end() )
        return (uniset3::umessage::Priority)it->second->sinf.priority();

    return uniset3::umessage::mpMedium; // ??
}
// --------------------------------------------------------------------------------------------------------------
grpc::Status IOController::getSensorIOInfo(::grpc::ServerContext* context, const ::uniset3::GetSensorIOInfoParams* request, ::uniset3::SensorIOInfo* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(getSensorIOInfo): Deadline exceeded or Client cancelled, abandoning.");
    }

    auto it = ioList.find(request->id());

    if( it != ioList.end() )
    {
        uniset_rwmutex_rlock lock(it->second->val_lock);
        (*response) = it->second->sinf;
        return grpc::Status::OK;
    }

    // -------------
    ostringstream err;
    err << myname << "(getSensorIOInfo): Unknown sensor (" << request->id() << ")"
        << uniset_conf()->oind->getNameById(request->id());

    uinfo << err.str() << endl;
    return grpc::Status(grpc::StatusCode::NOT_FOUND, err.str());
}
// --------------------------------------------------------------------------------------------------------------
grpc::Status IOController::getRawValue(::grpc::ServerContext* context, const ::uniset3::GetRawValueParams* request, ::google::protobuf::Int64Value* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(getRawValue): Deadline exceeded or Client cancelled, abandoning.");
    }

    auto it = ioList.find(request->id());

    if( it == ioList.end() )
    {
        ostringstream err;
        err << myname << "(getRawValue): Unknown analog sensor (" << request->id() << ")"
            << uniset_conf()->oind->getNameById(request->id());
        return grpc::Status(grpc::StatusCode::NOT_FOUND, err.str());
    }

    // ??? получаем raw из калиброванного значения ???
    auto ci = it->second->sinf.ci();

    if( ci.maxcal() != 0 && ci.maxcal() != ci.mincal() )
    {
        if( it->second->sinf.type() == uniset3::AI )
        {
            response->set_value(uniset3::lcalibrate(it->second->sinf.value(), ci.minraw(), ci.maxraw(), ci.mincal(), ci.maxcal(), true));
            return grpc::Status::OK;
        }

        if( it->second->sinf.type() == uniset3::AO )
        {
            response->set_value(uniset3::lcalibrate(it->second->sinf.value(), ci.mincal(), ci.maxcal(), ci.minraw(), ci.maxraw(), true));
            return grpc::Status::OK;
        }
    }

    response->set_value(it->second->sinf.value());
    return grpc::Status::OK;
}
// --------------------------------------------------------------------------------------------------------------
grpc::Status IOController::calibrate(::grpc::ServerContext* context, const ::uniset3::CalibrateParams* request, ::google::protobuf::Empty* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(calibrate): Deadline exceeded or Client cancelled, abandoning.");
    }

    auto it = ioList.find(request->id());

    if( it == ioList.end() )
    {
        ostringstream err;
        err << myname << "(calibrate): Unknown analog sensor (" << request->id() << ")"
            << uniset_conf()->oind->getNameById(request->id());
        return grpc::Status(grpc::StatusCode::NOT_FOUND, err.str());
    }

    uinfo << myname << "(calibrate): from " << uniset_conf()->oind->getNameById(request->adminid()) << endl;

    *(it->second->sinf.mutable_ci()) = request->ci();
    return grpc::Status::OK;
}
// --------------------------------------------------------------------------------------------------------------
grpc::Status IOController::getCalibrateInfo(::grpc::ServerContext* context, const ::uniset3::GetCalibrateInfoParams* request, ::uniset3::CalibrateInfo* response )
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(getCalibrateInfo): Deadline exceeded or Client cancelled, abandoning.");
    }

    auto it = ioList.find(request->id());

    if( it == ioList.end() )
    {
        ostringstream err;
        err << myname << "(calibrate): Unknown analog sensor (" << request->id() << ")"
            << uniset_conf()->oind->getNameById(request->id());
        return grpc::Status(grpc::StatusCode::NOT_FOUND, err.str());
    }

    *response = it->second->sinf.ci();
    return grpc::Status::OK;
}
// --------------------------------------------------------------------------------------------------------------
IOController::USensorInfo::USensorInfo( uniset3::SensorIOInfo& ai )
{
    sinf = ai;
}

IOController::USensorInfo::USensorInfo( const uniset3::SensorIOInfo& ai )
{
    sinf = ai;
}

IOController::USensorInfo::USensorInfo(uniset3::SensorIOInfo* ai)
{
    sinf = *ai;
}

IOController::USensorInfo&
IOController::USensorInfo::operator=(uniset3::SensorIOInfo& r)
{
    IOController::USensorInfo tmp;
    tmp.sinf = r;
    (*this) = std::move(tmp);
    return *this;
}
// ----------------------------------------------------------------------------------------
IOController::USensorInfo::USensorInfo(): d_value(1), d_off_value(0)
{
    sinf.set_depend_sid(uniset3::DefaultObjectId);
    sinf.set_default_val(0);
    sinf.set_value(sinf.default_val());
    sinf.set_real_value(sinf.default_val());
    sinf.set_dbignore(false);
    sinf.set_blocked(false);
    sinf.set_frozen(false);
    sinf.set_supplier(uniset3::DefaultObjectId);

    // стоит ли выставлять текущее время
    // Мы теряем возможность понять (по tv_sec=0),
    // что значение ещё ни разу никем не менялось
    auto tm = uniset3::now_to_timespec();
    sinf.mutable_ts()->set_sec(tm.tv_sec);
    sinf.mutable_ts()->set_nsec(tm.tv_nsec);
}
// ----------------------------------------------------------------------------------------
IOController::USensorInfo&
IOController::USensorInfo::operator=( uniset3::SensorIOInfo* r )
{
    IOController::USensorInfo tmp;
    tmp.sinf = *r;
    (*this) = std::move(tmp);
    return *this;
}
// ----------------------------------------------------------------------------------------
void* IOController::USensorInfo::getUserData( size_t index )
{
    if( index >= MaxUserData )
        return nullptr;

    uniset3::uniset_rwmutex_rlock ulock(userdata_lock);
    return userdata[index];
}

void IOController::USensorInfo::setUserData( size_t index, void* data )
{
    if( index >= MaxUserData )
        return;

    uniset3::uniset_rwmutex_wrlock ulock(userdata_lock);
    userdata[index] = data;
}
// ----------------------------------------------------------------------------------------
const IOController::USensorInfo&
IOController::USensorInfo::operator=( const uniset3::SensorIOInfo& r )
{
    IOController::USensorInfo tmp;
    tmp.sinf = r;
    (*this) = std::move(tmp);
    return *this;
}
// ----------------------------------------------------------------------------------------
void IOController::USensorInfo::init( const uniset3::SensorIOInfo& s )
{
    IOController::USensorInfo r;
    r.sinf = s;
    (*this) = std::move(r);
}
// ----------------------------------------------------------------------------------------
IOController::IOStateList::iterator IOController::myioBegin()
{
    return ioList.begin();
}
// ----------------------------------------------------------------------------------------
IOController::IOStateList::iterator IOController::myioEnd()
{
    return ioList.end();
}
// ----------------------------------------------------------------------------------------
void IOController::initIOList( const IOController::IOStateList&& l )
{
    ioList = std::move(l);
}
// ----------------------------------------------------------------------------------------
void IOController::for_iolist( IOController::UFunction f )
{
    uniset_rwmutex_rlock lck(ioMutex);

    for( auto&& s : ioList )
        f(s.second);
}
// ----------------------------------------------------------------------------------------
IOController::IOStateList::iterator IOController::myiofind( const uniset3::ObjectId id )
{
    return ioList.find(id);
}
// -----------------------------------------------------------------------------
grpc::Status IOController::getSensorSeq(::grpc::ServerContext* context, const ::uniset3::GetSensorSeqParams* request, ::uniset3::SensorIOInfoSeq* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(getSensorSeq): Deadline exceeded or Client cancelled, abandoning.");
    }

    uniset3::SensorIOInfo unk;
    unk.mutable_si()->set_id(DefaultObjectId);
    unk.mutable_si()->set_node(DefaultObjectId);

    for( const auto& id : request->seq().ids() )
    {
        auto it = ioList.find(id);

        if( it != ioList.end() )
        {
            *(response->add_sensors()) = it->second->sinf;
            continue;
        }

        // элемент не найден...
        unk.mutable_si()->set_id(id);
        *(response->add_sensors()) = unk;
    }

    return grpc::Status::OK;
}
// -----------------------------------------------------------------------------
grpc::Status IOController::setOutputSeq(::grpc::ServerContext* context, const ::uniset3::SetOutputParams* request, ::uniset3::IDSeq* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(setOutputSeq): Deadline exceeded or Client cancelled, abandoning.");
    }

    uniset3::IDList badlist; // список не найденных идентификаторов

    for(const auto& s : request->lst().sensors())
    {
        {
            auto it = ioList.find(s.si().id());

            if( it != ioList.end() )
            {
                localSetValueIt(it, s.si().id(), s.value(), request->supplier());
                continue;
            }
        }

        // не найден
        response->add_ids(s.si().id());
    }

    return grpc::Status::OK;
}
// -----------------------------------------------------------------------------
grpc::Status IOController::getTimeChange(::grpc::ServerContext* context, const ::uniset3::GetTimeChangeParams* request, ::uniset3::ShortIOInfo* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(getTimeChange): Deadline exceeded or Client cancelled, abandoning.");
    }

    auto ait = ioList.find(request->id());

    if( ait != ioList.end() )
    {
        auto& s = ait->second->sinf;
        uniset_rwmutex_rlock lock(ait->second->val_lock);
        response->set_value(s.value());
        *(response->mutable_ts()) = s.ts();
        response->set_supplier(s.supplier());
        return grpc::Status::OK;
    }

    // -------------
    ostringstream err;
    err << myname << "(getChangedTime): вход(выход) с именем "
        << uniset_conf()->oind->getNameById(request->id()) << " не найден";

    uinfo << err.str() << endl;
    return grpc::Status(grpc::StatusCode::NOT_FOUND, err.str());
}
// -----------------------------------------------------------------------------
grpc::Status IOController::getSensors(::grpc::ServerContext* context, const ::uniset3::GetSensorsParams* request, ::uniset3::ShortMapSeq* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(getSensors): Deadline exceeded or Client cancelled, abandoning.");
    }

    for( const auto& it : ioList )
    {
        auto s = response->add_sensors();
        {
            uniset_rwmutex_rlock lock(it.second->val_lock);
            s->set_id(it.second->sinf.si().id());
            s->set_value(it.second->sinf.value());
            s->set_type(it.second->sinf.type());
        }
    }

    return grpc::Status::OK;
}
// -----------------------------------------------------------------------------
IOController::ChangeSignal IOController::signal_change_value( uniset3::ObjectId sid )
{
    auto it = ioList.find(sid);

    if( it == ioList.end() )
    {
        ostringstream err;
        err << myname << "(signal_change_value): вход(выход) с именем "
            << uniset_conf()->oind->getNameById(sid) << " не найден";

        uinfo << err.str() << endl;
        throw uniset3::NameNotFound(err.str().c_str());
    }

    return it->second->sigChange;
}
// -----------------------------------------------------------------------------
IOController::ChangeSignal IOController::signal_change_value()
{
    return sigAnyChange;
}
// -----------------------------------------------------------------------------
void IOController::USensorInfo::checkDepend( std::shared_ptr<USensorInfo>& d_it, IOController* ic )
{
    bool changed = false;
    ObjectId sup_id = ic->getId();
    {
        uniset_rwmutex_wrlock lock(val_lock);
        bool prev = sinf.blocked();
        uniset_rwmutex_rlock dlock(d_it->val_lock);
        sinf.set_blocked( d_it->sinf.value() != d_value );
        changed = ( prev != sinf.blocked() );
        sup_id = d_it->sinf.supplier();
    }

    ulog4 << ic->getName() << "(checkDepend): check si.id=" << sinf.si().id()
          << " d_it->value=" << d_it->sinf.value()
          << " d_value=" << d_value
          << " d_off_value=" << d_off_value
          << " blocked=" << sinf.blocked()
          << " changed=" << changed
          << " real_value=" << sinf.real_value()
          << endl;

    if( changed )
        ic->localSetValue( d_usi, sinf.real_value(), sup_id );
}
// -----------------------------------------------------------------------------
grpc::Status IOController::getInfo(::grpc::ServerContext* context, const ::uniset3::GetInfoParams* request, ::google::protobuf::StringValue* response)
{
    if (context->IsCancelled())
    {
        return grpc::Status(grpc::StatusCode::CANCELLED, "(getInfo): Deadline exceeded or Client cancelled, abandoning.");
    }

    ::google::protobuf::StringValue oinf;
    grpc::Status st = UniSetManager::getInfo(context, request, &oinf);

    if( !st.ok() )
        return st;

    ostringstream inf;
    inf << oinf.value() << endl;
    inf << "isPingDBServer = " << isPingDBServer << endl;
    inf << "ioListSize = " << ioList.size() << endl;

    response->set_value(inf.str());
    return grpc::Status::OK;
}
// -----------------------------------------------------------------------------
#if 0
Poco::JSON::Object::Ptr IOController::httpHelp( const Poco::URI::QueryParameters& p )
{
    uniset3::json::help::object myhelp( myname, UniSetManager::httpHelp(p) );

    {
        // 'get'
        uniset3::json::help::item cmd("get", "get value for sensor");
        cmd.param("id1,name2,id3", "get value for id1,name2,id3 sensors");
        cmd.param("shortInfo", "get short information for sensors");
        myhelp.add(cmd);
    }

    {
        // 'sensors'
        uniset3::json::help::item cmd("sensors", "get all sensors");
        cmd.param("nameonly", "get only name sensors");
        cmd.param("offset=N", "get from N record");
        cmd.param("limit=M", "limit of records");
        myhelp.add(cmd);
    }

    return myhelp;
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IOController::httpRequest( const string& req, const Poco::URI::QueryParameters& p )
{
    if( req == "get" )
        return request_get(req, p);

    if( req == "sensors" )
        return request_sensors(req, p);

    return UniSetManager::httpRequest(req, p);
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IOController::request_get( const string& req, const Poco::URI::QueryParameters& p )
{
    if( p.empty() )
    {
        ostringstream err;
        err << myname << "(request): 'get'. Unknown ID or Name. Use parameters: get?ID1,name2,ID3,...";
        throw uniset3::SystemError(err.str());
    }

    auto conf = uniset_conf();
    auto slist = uniset3::getSInfoList( p[0].first, conf );

    if( slist.empty() )
    {
        ostringstream err;
        err << myname << "(request): 'get'. Unknown ID or Name. Use parameters: get?ID1,name2,ID3,...";
        throw uniset3::SystemError(err.str());
    }

    bool shortInfo = false;

    if( p.size() > 1 && p[1].first == "shortInfo" )
        shortInfo = true;

    // {
    //   "sensors" [
    //           { name: string, value: long, error: string, ...},
    //           { name: string, value: long, error: string, ...},
    //           ...
    //   ],
    //
    //   "object" { mydata... }
    //  }

    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    auto my = httpGetMyInfo(jdata);
    auto jsens = uniset3::json::make_child_array(jdata, "sensors");

    for( const auto& s : slist )
    {
        try
        {
            auto sinf = ioList.find(s.si.id());

            if( sinf == ioList.end() )
            {
                Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
                jr->set("name", s.fname);
                jr->set("error", "Sensor not found");
                jsens->add(jr);
                continue;
            }

            getSensorInfo(jsens, sinf->second, shortInfo);
        }
        catch( uniset3::NameNotFound& ex )
        {
            Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
            jr->set("name", s.fname);
            jr->set("error", string(ex.what()));
            jsens->add(jr);
        }
        catch( std::exception& ex )
        {
            Poco::JSON::Object::Ptr jr = new Poco::JSON::Object();
            jr->set("name", s.fname);
            jr->set("error", string(ex.what()));
            jsens->add(jr);
        }
    }

    return jdata;
}
// -----------------------------------------------------------------------------
void IOController::getSensorInfo( Poco::JSON::Array::Ptr& jdata, std::shared_ptr<USensorInfo>& s, bool shortInfo )
{
    Poco::JSON::Object::Ptr jsens = new Poco::JSON::Object();
    jdata->add(jsens);

    {
        uniset_rwmutex_rlock lock(s->val_lock);
        jsens->set("value", s->sinf.value());
        jsens->set("real_value", s->sinf.real_value());
    }

    jsens->set("id", s->sinf.si().id());
    jsens->set("name", ObjectIndex::getShortName(uniset_conf()->oind->getMapName(s->sinf.si().id())));
    jsens->set("ts_sec", s->sinf.ts().sec());
    jsens->set("ts_nsec", s->sinf.ts().nsec());

    if( shortInfo )
        return;

    <<< <<< < HEAD
    jsens->set("type", uniset3::iotype2str(s->sinf.type()));
    jsens->set("default_val", s->sinf.default_val());
    jsens->set("dbignore", s->sinf.dbignore());
    jsens->set("nchanges", s->nchanges);
    jsens->set("frozen", s->sinf.frozen());
    jsens->set("blocked", s->sinf.blocked());
    == == == =
        jsens->set("type", uniset::iotype2str(s->type));
    jsens->set("default_val", s->default_val);
    jsens->set("dbignore", s->dbignore);
    jsens->set("nchanges", s->nchanges);
    jsens->set("undefined", s->undefined);
    jsens->set("frozen", s->frozen);
    jsens->set("blocked", s->blocked);
    jsens->set("readonly", s->readonly);

    if( s->depend_sid != DefaultObjectId )
    {
        jsens->set("depend_sensor", ORepHelpers::getShortName(uniset_conf()->oind->getMapName(s->depend_sid)));
        jsens->set("depend_sensor_id", s->depend_sid);
        jsens->set("depend_value", s->d_value);
        jsens->set("depend_off_value", s->d_off_value);
    }

    >>> >>> > e87c4a09 ((iocontrol): supported "readonly" sensors, supported "frozen_value" by default)

    if( s->sinf.depend_sid() != DefaultObjectId )
    {
        jsens->set("depend_sensor", ObjectIndex::getShortName(uniset_conf()->oind->getMapName(s->sinf.depend_sid())));
        jsens->set("depend_sensor_id", s->sinf.depend_sid());
        jsens->set("depend_value", s->d_value);
        jsens->set("depend_off_value", s->d_off_value);
    }

    Poco::JSON::Object::Ptr calibr = uniset3::json::make_child(jsens, "calibration");
    calibr->set("cmin", s->sinf.ci().mincal());
    calibr->set("cmax", s->sinf.ci().maxcal());
    calibr->set("rmin", s->sinf.ci().minraw());
    calibr->set("rmax", s->sinf.ci().maxraw());
    calibr->set("precision", s->sinf.ci().precision());

    //  undefined;
    //  blocked;
    //  priority;
    //  long d_value = { 1 }; /*!< разрешающее работу значение датчика от которого зависит данный */
    //  long d_off_value = { 0 }; /*!< блокирующее значение */
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr IOController::request_sensors( const string& req, const Poco::URI::QueryParameters& params )
{
    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    Poco::JSON::Array::Ptr jsens = uniset3::json::make_child_array(jdata, "sensors");
    auto my = httpGetMyInfo(jdata);

    size_t num = 0;
    size_t offset = 0;
    size_t limit = 0;
    size_t count = 0;

    for( const auto& p : params )
    {
        if( p.first == "offset" )
            offset = uni_atoi(p.second);
        else if( p.first == "limit" )
            limit = uni_atoi(p.second);
    }

    size_t endnum = offset + limit;

    for( auto it = myioBegin(); it != myioEnd(); ++it, num++ )
    {
        if( limit > 0 && num >= endnum )
            break;

        if( offset > 0 && num < offset )
            continue;

        getSensorInfo(jsens, it->second, false);
        count++;
    }

    jdata->set("count", count);
    jdata->set("size", ioCount());
    return jdata;
}
// -----------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
