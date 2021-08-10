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
#include <unordered_map>
#include <map>
#include <unistd.h>
#include <signal.h>
#include <iomanip>
#include <pthread.h>
#include <sys/types.h>
#include <sstream>
#include <chrono>
#include <Poco/Process.h>

#include "unisetstd.h"
#include "Exceptions.h"
#include "UInterface.h"
#include "UniSetObject.h"
#include "UniSetActivator.h"
#include "Debug.h"

// ------------------------------------------------------------------------------------------
using namespace std;
namespace uniset3
{
    using namespace uniset3::umessage;

#define CREATE_TIMER    unisetstd::make_unique<PassiveCondTimer>();
    // new PassiveSysTimer();

    // ------------------------------------------------------------------------------------------
    UniSetObject::UniSetObject():
        msgpid(0),
        regOK(false),
        active(0),
        threadcreate(false),
        myid(uniset3::DefaultObjectId)
    {
        ui = make_shared<UInterface>(uniset3::DefaultObjectId);

        tmr = CREATE_TIMER;
        myname = "noname";
        initObject();
    }
    // ------------------------------------------------------------------------------------------
    UniSetObject::UniSetObject( ObjectId id ):
        msgpid(0),
        regOK(false),
        active(0),
        threadcreate(true),
        myid(id)
    {
        ui = make_shared<UInterface>(id);
        tmr = CREATE_TIMER;

        if( myid != DefaultObjectId )
            setID(id);
        else
        {
            threadcreate = false;
            myname = "UnknownUniSetObject";
        }

        initObject();
    }
    // ------------------------------------------------------------------------------------------
    UniSetObject::~UniSetObject()
    {
    }
    // ------------------------------------------------------------------------------------------
    std::shared_ptr<UniSetObject> UniSetObject::get_ptr()
    {
        return shared_from_this();
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::initObject()
    {
        //      a_working = ATOMIC_VAR_INIT(0);
        active = ATOMIC_VAR_INIT(0);

        refmutex.setName(myname + "_refmutex");

        auto conf = uniset_conf();

        if( !conf )
        {
            ostringstream err;
            err << myname << "(initObject): Unknown configuration!!";
            throw SystemError(err.str());
        }

        {
            uniset3::uniset_rwmutex_wrlock lock(refmutex);
            oref.set_id(myid);
            auto oinf = conf->oind->getObjectInfo(myid);

            if( oinf )
                oref.set_path(oinf->secName);
        }

        int sz = conf->getArgPInt("--uniset-object-size-message-queue", conf->getField("SizeOfMessageQueue"), 1000);

        if( sz > 0 )
            setMaxSizeOfMessageQueue(sz);

        uinfo << myname << "(init): SizeOfMessageQueue=" << getMaxSizeOfMessageQueue() << endl;
    }
    // ------------------------------------------------------------------------------------------

    /*!
     *    \param om - указатель на менеджер, управляющий объектом
     *    \return Возвращает \a true если инициализация прошла успешно, и \a false если нет
    */
    bool UniSetObject::init( const std::string& svcAddr )
    {
        uinfo << myname << ": init..." << endl;
        uniset3::uniset_rwmutex_wrlock lock(refmutex);
        oref.set_addr(svcAddr);
        oref.set_type(getStrType());
        uinfo << myname << ": init ok [" << oref << "]" << endl;
        return true;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::setID( uniset3::ObjectId id )
    {
        if( isActive() )
            throw ObjectNameAlready("Set ID error: ObjectId is active..");

        string myfullname = ui->getNameById(id);
        myname = ObjectIndex::getShortName(myfullname);
        myid = id;

        {
            uniset3::uniset_rwmutex_wrlock lock(refmutex);
            oref.set_id(myid);
        }

        ui->initBackId(myid);
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::setMaxSizeOfMessageQueue( size_t s )
    {
        mqueueMedium.setMaxSizeOfMessageQueue(s);
        mqueueLow.setMaxSizeOfMessageQueue(s);
        mqueueHi.setMaxSizeOfMessageQueue(s);
    }
    // ------------------------------------------------------------------------------------------
    size_t UniSetObject::getMaxSizeOfMessageQueue() const
    {
        return mqueueMedium.getMaxSizeOfMessageQueue();
    }
    // ------------------------------------------------------------------------------------------
    bool UniSetObject::isActive() const
    {
        return active;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::setActive(bool set)
    {
        active = set;
    }
    // ------------------------------------------------------------------------------------------
    /*!
     *    \param  vm - указатель на структуру, которая заполняется, если есть сообщение
     *    \return Возвращает указатель VoidMessagePtr если сообщение есть, и shared_ptr(nullptr) если нет
    */
    VoidMessagePtr UniSetObject::receiveMessage()
    {
        if( !mqueueHi.empty() )
            return mqueueHi.top();

        if( !mqueueMedium.empty() )
            return mqueueMedium.top();

        return mqueueLow.top();
    }
    // ------------------------------------------------------------------------------------------
    VoidMessagePtr UniSetObject::waitMessage( timeout_t timeMS )
    {
        auto m = receiveMessage();

        if( m )
            return m;

        tmr->wait(timeMS);
        return receiveMessage();
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::registration()
    {
        ulogrep << myname << ": registration..." << endl;

        if( myid == uniset3::DefaultObjectId )
        {
            ulogrep << myname << "(registration): Don`t registration. myid=DefaultObjectId \n";
            return;
        }

        {
            uniset3::uniset_rwmutex_rlock lock(refmutex);

            if( oref.addr().empty() )
            {
                uwarn << myname << "(registration): repository address is NULL.." << endl;
                return;
            }
        }

        auto conf = uniset_conf();
        regOK = false;

        for( size_t i = 0; i < conf->getRepeatCount(); i++ )
        {
            try
            {
                ui->registered(getRef(), true);
                regOK = true;
                break;
            }
            catch( ObjectNameAlready& al )
            {
                /*!
                        \warning По умолчанию объекты должны быть уникальны! Поэтому если идёт попытка повторной регистрации.
                        Мы чистим существующую ссылку и заменяем её на новую.
                        Это сделано для более надёжной работы, иначе может получится, что если объект перед завершением
                        не очистил за собой ссылку (не разрегистрировался), то больше он никогда не сможет вновь зарегистрироваться.
                        Т.к. \b надёжной функции проверки "жив" ли объект пока нет...
                        (так бы можно было проверить и если "не жив", то смело заменять ссылку на новую). Но существует обратная сторона:
                        если заменяемый объект "жив" и завершит свою работу, то он может почистить за собой ссылку и это тогда наш (новый)
                        объект станет недоступен другим, а знать об этом не будет!!!
                    */
                uwarn << myname << "(registration): replace object (ObjectNameAlready)" << endl;
                unregistration();
            }
            catch( const uniset3::ORepFailed& ex )
            {
                uwarn << myname << "(registration): don`t registration in object reposotory "
                      << " err: " << ex << endl;
            }
            catch( const uniset3::Exception& ex )
            {
                uwarn << myname << "(registration):  " << ex << endl;
            }

            msleep(conf->getRepeatTimeout());
        }

        if( !regOK )
        {
            string err(myname + "(registration): don`t registration in object reposotory");
            ucrit << err << endl;
            throw ORepFailed(err);
        }
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::unregistration()
    {
        if( myid < 0 ) // || !reg )
        {
            regOK = false;
            return;
        }

        if( myid == uniset3::DefaultObjectId ) // -V547
        {
            uinfo << myname << "(unregister): myid=DefaultObjectId \n";
            regOK = false;
            return;
        }

        {
            uniset3::uniset_rwmutex_rlock lock(refmutex);

            if( oref.addr().empty() )
            {
                uwarn << myname << "(unregister): unknown repository address" << endl;
                regOK = false;
                return;
            }
        }


        try
        {
            uinfo << myname << ": unregister " << endl;
            ui->unregister(myid);
            uinfo << myname << ": unregister ok. " << endl;
        }
        catch(...)
        {
            std::exception_ptr p = std::current_exception();
            uwarn << myname << ": don`t registration in object repository"
                  << " err: " << (p ? p.__cxa_exception_type()->name() : "unknown")
                  << endl;
        }

        regOK = false;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::waitFinish()
    {
        // поток завершаем в конце, после пользовательских deactivateObject()
        if( !thr )
            return;

        std::unique_lock<std::mutex> lk(m_working);

        //        cv_working.wait_for(lk, std::chrono::milliseconds(workingTerminateTimeout), [&](){ return (a_working == false); } );
        cv_working.wait(lk, [ = ]()
        {
            return a_working == false;
        });

        if( thr->isRunning() )
            thr->join();
    }
    // ------------------------------------------------------------------------------------------
    bool UniSetObject::isExists()
    {
        return true;
    }
    // ------------------------------------------------------------------------------------------
    ::grpc::Status UniSetObject::exists(::grpc::ServerContext* context, const ::uniset3::ExistsParams* request, ::google::protobuf::BoolValue* response)
    {
        response->set_value(isExists());
        return ::grpc::Status::OK;
    }
    // ------------------------------------------------------------------------------------------
    ObjectId UniSetObject::getId() const
    {
        return myid;
    }
    // ------------------------------------------------------------------------------------------
    ::grpc::Status UniSetObject::getType(::grpc::ServerContext* context, const ::uniset3::GetTypeParams* request, ::google::protobuf::StringValue* response)
    {
        response->set_value(getStrType());
        return ::grpc::Status::OK;
    }
    // ------------------------------------------------------------------------------------------
    string UniSetObject::getName() const
    {
        return myname;
    }
    // ------------------------------------------------------------------------------------------
    string UniSetObject::getStrType() const
    {
        return "UniSetObject";
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::termWaiting()
    {
        if( tmr )
            tmr->terminate();
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::setThreadPriority( Poco::Thread::Priority p )
    {
        if( thr )
            thr->setPriority(p);
    }
    // ------------------------------------------------------------------------------------------
    ::grpc::Status UniSetObject::push(::grpc::ServerContext* context, const ::uniset3::umessage::TransportMessage* request, ::google::protobuf::Empty* response)
    {
        // make copy
        auto vm = make_shared<umessage::TransportMessage>(*request);

        if( request->priority() == umessage::mpMedium )
            mqueueMedium.push(vm);
        else if( request->priority() == umessage::mpHigh )
            mqueueHi.push(vm);
        else if( request->priority() == umessage::mpLow )
            mqueueLow.push(vm);
        else // на всякий по умолчанию medium
            mqueueMedium.push(vm);

        termWaiting();
        return ::grpc::Status::OK;
    }
    // ------------------------------------------------------------------------------------------
#ifndef DISABLE_REST_API
    Poco::JSON::Object::Ptr UniSetObject::httpGet( const Poco::URI::QueryParameters& p )
    {
        Poco::JSON::Object::Ptr jret = new Poco::JSON::Object();
        httpGetMyInfo(jret);
        return jret;
    }
    // ------------------------------------------------------------------------------------------
    Poco::JSON::Object::Ptr UniSetObject::httpHelp( const Poco::URI::QueryParameters& p )
    {
        uniset3::json::help::object myhelp(myname);

        {
            uniset3::json::help::item cmd("params/get", "get value for parameter");
            cmd.param("param1,param2,...", "paremeter names. Default: all");
            myhelp.add(cmd);
        }
        {
            uniset3::json::help::item cmd("params/set", "set value for parameter");
            cmd.param("param1=val1,param2=val2,...", "paremeters");
            myhelp.add(cmd);
        }

        return myhelp;
    }
    // ------------------------------------------------------------------------------------------
    Poco::JSON::Object::Ptr UniSetObject::httpGetMyInfo( Poco::JSON::Object::Ptr root )
    {
        Poco::JSON::Object::Ptr my = uniset3::json::make_child(root, "object");
        my->set("name", myname);
        my->set("id", getId());
        my->set("msgCount", countMessages());
        my->set("lostMessages", getCountOfLostMessages());
        my->set("maxSizeOfMessageQueue", getMaxSizeOfMessageQueue());
        my->set("isActive", isActive());
        my->set("objectType", getStrType());
        return my;
    }
    // ------------------------------------------------------------------------------------------
    // обработка запроса вида: /configure/xxxx
    Poco::JSON::Object::Ptr UniSetObject::request_configure( const std::string& req, const Poco::URI::QueryParameters& params )
    {
        if( req == "get" )
            return request_configure_get(req, params);

        ostringstream err;
        err << "(request_conf):  BAD REQUEST: Unknown command..";
        throw uniset3::SystemError(err.str());
    }
    // ------------------------------------------------------------------------------------------
    // обработка запроса вида: /params/xxxx
    Poco::JSON::Object::Ptr UniSetObject::request_params( const std::string& req, const Poco::URI::QueryParameters& params )
    {
        if( req == "get" )
            return request_params_get(req, params);

        if( req == "set" )
            return request_params_set(req, params);

        ostringstream err;
        err << "(request_conf):  BAD REQUEST: Unknown command..";
        throw uniset3::SystemError(err.str());
    }
    // ------------------------------------------------------------------------------------------
    // обработка запроса вида: /configure/get?[ID|NAME]&props=testname,name] from configure.xml
    Poco::JSON::Object::Ptr UniSetObject::request_configure_get( const std::string& req, const Poco::URI::QueryParameters& params )
    {
        Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
        Poco::JSON::Array::Ptr jdata = uniset3::json::make_child_array(json, "conf");
        auto my = httpGetMyInfo(json);

        if( params.empty() )
        {
            ostringstream err;
            err << "(request_conf):  BAD REQUEST: Unknown id or name...";
            throw uniset3::SystemError(err.str());
        }

        auto idlist = uniset3::explode_str(params[0].first, ',');

        if( idlist.empty() )
        {
            ostringstream err;
            err << "(request_conf):  BAD REQUEST: Unknown id or name in '" << params[0].first << "'";
            throw uniset3::SystemError(err.str());
        }

        string props = {""};

        for( const auto& p : params )
        {
            if( p.first == "props" )
            {
                props = p.second;
                break;
            }
        }

        for( const auto& id : idlist )
        {
            Poco::JSON::Object::Ptr j = request_configure_by_name(id, props);

            if( j )
                jdata->add(j);
        }

        return json;
    }
    // ------------------------------------------------------------------------------------------
    Poco::JSON::Object::Ptr UniSetObject::request_configure_by_name( const string& name, const std::string& props )
    {
        Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
        auto conf = uniset_conf();

        ObjectId id = conf->getAnyID(name);

        if( id == DefaultObjectId )
        {
            ostringstream err;
            err << name << " not found..";
            jdata->set(name, "");
            jdata->set("error", err.str());
            return jdata;
        }

        xmlNode* xmlnode = conf->getXMLObjectNode(id);

        if( !xmlnode )
        {
            ostringstream err;
            err << name << " not found confnode..";
            jdata->set(name, "");
            jdata->set("error", err.str());
            return jdata;
        }

        UniXML::iterator it(xmlnode);

        jdata->set("name", it.getProp("name"));
        jdata->set("id", it.getProp("id"));

        if( !props.empty() )
        {
            auto lst = uniset3::explode_str(props, ',');

            for( const auto& p : lst )
                jdata->set(p, it.getProp(p));
        }
        else
        {
            auto lst = it.getPropList();

            for( const auto& p : lst )
                jdata->set(p.first, p.second);
        }

        return jdata;
    }
    // ------------------------------------------------------------------------------------------
    // обработка запроса вида: /conf/set?prop1=val1&prop2=val2
    Poco::JSON::Object::Ptr UniSetObject::request_params_set( const std::string& req, const Poco::URI::QueryParameters& p )
    {
        ostringstream err;
        err << "(request_params): 'set' not realized yet";
        throw uniset3::SystemError(err.str());
    }
    // ------------------------------------------------------------------------------------------
    // обработка запроса вида: /conf/get?prop1&prop2&prop3
    Poco::JSON::Object::Ptr UniSetObject::request_params_get( const std::string& req, const Poco::URI::QueryParameters& p )
    {
        ostringstream err;
        err << "(request_params): 'get' not realized yet";
        throw uniset3::SystemError(err.str());
    }
    // ------------------------------------------------------------------------------------------
#endif
    // ------------------------------------------------------------------------------------------
    uniset3::ObjectRef UniSetObject::getRef() const
    {
        uniset3::uniset_rwmutex_rlock lock(refmutex);
        return oref;
    }
    // ------------------------------------------------------------------------------------------
    size_t UniSetObject::countMessages()
    {
        return (mqueueMedium.size() + mqueueLow.size() + mqueueHi.size());
    }
    // ------------------------------------------------------------------------------------------
    size_t UniSetObject::getCountOfLostMessages() const
    {
        return (mqueueMedium.getCountOfLostMessages() +
                mqueueLow.getCountOfLostMessages() +
                mqueueHi.getCountOfLostMessages() );
    }
    // ------------------------------------------------------------------------------------------
    bool UniSetObject::activateObject()
    {
        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool UniSetObject::postActivateObjects()
    {
        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool UniSetObject::deactivateObject()
    {
        return true;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::uterminate()
    {
        //      setActive(false);
        auto act = UniSetActivator::Instance();
        act->terminate();
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::thread(bool create)
    {
        threadcreate = create;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::offThread()
    {
        threadcreate = false;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::onThread()
    {
        threadcreate = true;
    }
    // ------------------------------------------------------------------------------------------
    bool UniSetObject::deactivate()
    {
        if( !isActive() )
        {
            try
            {
                deactivateObject();
            }
            catch( std::exception& ex )
            {
                uwarn << myname << "(deactivate): " << ex.what() << endl;
            }

            return true;
        }

        setActive(false); // завершаем поток обработки сообщений

        if( tmr )
            tmr->terminate();

        try
        {
            uinfo << myname << "(deactivate): ..." << endl;

            try
            {
                deactivateObject();
            }
            catch( std::exception& ex )
            {
                uwarn << myname << "(deactivate): " << ex.what() << endl;
            }

            unregistration();
            uinfo << myname << "(deactivate): finished..." << endl;
            waitFinish();
            return true;
        }
        catch( std::exception& ex )
        {
            uwarn << myname << "(deactivate): " << ex.what() << endl;
        }

        return false;
    }

    // ------------------------------------------------------------------------------------------
    bool UniSetObject::activate()
    {
        uinfo << myname << ": activate..." << endl;
        registration();

        // Запускаем поток обработки сообщений
        setActive(true);

        if( myid != uniset3::DefaultObjectId && threadcreate )
        {
            thr = unisetstd::make_unique< ThreadCreator<UniSetObject> >(this, &UniSetObject::work);
            //thr->setCancel(ost::Thread::cancelDeferred);

            std::unique_lock<std::mutex> locker(m_working);
            a_working = true;
            thr->start();
        }
        else
        {
            // выдаём предупреждение только если поток не отключён, но при этом не задан ID
            if( threadcreate )
            {
                uinfo << myname << ": ?? не задан ObjectId...("
                      << "myid=" << myid << " threadcreate=" << threadcreate
                      << ")" << endl;
            }

            thread(false);
        }

        activateObject();
        uinfo << myname << ": activate ok." << endl;
        return true;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::work()
    {
        uinfo << myname << ": thread processing umessage running..." << endl;

        msgpid = thr ? thr->getTID() : Poco::Process::id();

        {
            std::unique_lock<std::mutex> locker(m_working);
            a_working = true;
        }

        while( isActive() )
            callback();

        uinfo << myname << ": thread processing umessage stopped..." << endl;

        {
            std::unique_lock<std::mutex> locker(m_working);
            a_working = false;
        }

        cv_working.notify_all();
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::callback()
    {
        // При реализации с использованием waitMessage() каждый раз при вызове askTimer() необходимо
        // проверять возвращаемое значение на UniSetTimers::WaitUpTime и вызывать termWaiting(),
        // чтобы избежать ситуации, когда процесс до заказа таймера 'спал'(в функции waitMessage()) и после
        // заказа продолжит спать(т.е. обработчик вызван не будет)...
        try
        {
            auto m = waitMessage(sleepTime);

            if( m )
                processingMessage(m.get());

            if( !isActive() )
                return;

            sleepTime = checkTimers(this);
        }
        catch( const uniset3::Exception& ex )
        {
            ucrit << myname << "(callback): " << ex << endl;
        }
    }
    // ------------------------------------------------------------------------------------------
    void UniSetObject::processingMessage( const uniset3::umessage::TransportMessage* msg )
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

        /*
            catch( ... )
            {
                std::exception_ptr p = std::current_exception();
                ucrit <<(p ? p.__cxa_exception_type()->name() : "null") << std::endl;
            }
        */
    }
    // ------------------------------------------------------------------------------------------
    timeout_t UniSetObject::askTimer( TimerId timerid, timeout_t timeMS, clock_t ticks, umessage::Priority p )
    {
        timeout_t tsleep = LT_Object::askTimer(timerid, timeMS, ticks, p);

        if( tsleep != UniSetTimer::WaitUpTime )
            termWaiting();

        return tsleep;
    }
    // ------------------------------------------------------------------------------------------

    ::grpc::Status UniSetObject::getInfo(::grpc::ServerContext* context, const ::uniset3::GetInfoParams* request, ::google::protobuf::StringValue* response)
    {
        ostringstream info;
        info.setf(ios::left, ios::adjustfield);
        info << "(" << myid << ")" << setw(40) << myname
             << " date: " << uniset3::dateToString()
             << " time: " << uniset3::timeToString()
             << "\n===============================================================================\n"
             << "pid=" << setw(10) << Poco::Process::id()
             << " tid=" << setw(10);

        if( threadcreate )
        {
            if(thr)
            {
                msgpid = thr->getTID();    // заодно(на всякий) обновим и внутреннюю информацию
                info << msgpid;
            }
            else
                info << "не запущен";
        }
        else
            info << "откл.";

        info << "\tcount=" << countMessages()
             << "\t medium: "
             << " maxMsg=" << mqueueMedium.getMaxQueueMessages()
             << " qFull(" << mqueueMedium.getMaxSizeOfMessageQueue() << ")=" << mqueueMedium.getCountOfLostMessages()
             << "\t     hi: "
             << " maxMsg=" << mqueueHi.getMaxQueueMessages()
             << " qFull(" << mqueueHi.getMaxSizeOfMessageQueue() << ")=" << mqueueHi.getCountOfLostMessages()
             << "\t    low: "
             << " maxMsg=" << mqueueLow.getMaxQueueMessages()
             << " qFull(" << mqueueLow.getMaxSizeOfMessageQueue() << ")=" << mqueueLow.getCountOfLostMessages();

        response->set_value(info.str());
        return grpc::Status::OK;
    }
    // ------------------------------------------------------------------------------------------

    //    SimpleInfo UniSetObject::apiRequest( const char* request )
    ::grpc::Status UniSetObject::request(::grpc::ServerContext* context, const ::uniset3::RequestParams* request, ::google::protobuf::StringValue* response)
    {
#ifdef DISABLE_REST_API
        return getInfo(context, request, response)
#else
        ostringstream err;

        try
        {
            Poco::URI uri(request->query());

            if( ulog()->is_level9() )
                ulog()->level9() << myname << "(apiRequest): request: " << request->query() << endl;

            ostringstream out;
            std::string query = "";

            // Пока не будем требовать обязательно использовать формат /api/vesion/query..
            // но если указан, то проверяем..
            std::vector<std::string> seg;
            uri.getPathSegments(seg);

            size_t qind = 0;

            if( seg.size() > 0 && seg[0] == "api" )
            {
                // проверка: /api/version/query[?params]..
                if( seg.size() < 2 || seg[1] != UHttp::UHTTP_API_VERSION )
                {
                    Poco::JSON::Object jdata;
                    jdata.set("error", Poco::Net::HTTPServerResponse::getReasonForStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST));
                    jdata.set("ecode", (int)Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
                    jdata.set("message", "BAD REQUEST STRUCTURE");
                    jdata.stringify(out);
                    response->set_value(out.str());
                    return ::grpc::Status::OK;
                }

                if( seg.size() > 2 )
                    qind = 2;
            }
            else if( seg.size() == 1 )
                qind = 0;

            query = seg.empty() ? "" : seg[qind];

            // обработка запроса..
            if( query == "help" )
            {
                // запрос вида: /help?params
                auto reply = httpHelp(uri.getQueryParameters());
                reply->stringify(out);
            }
            else if( query == "configure" )
            {
                // запрос вида: /configure/query?params
                string qconf = ( seg.size() > (qind + 1) ) ? seg[qind + 1] : "";
                auto reply = request_configure(qconf, uri.getQueryParameters());
                reply->stringify(out);
            }
            else if( query == "params" )
            {
                // запрос вида: /params/query?params
                string qconf = ( seg.size() > (qind + 1) ) ? seg[qind + 1] : "";
                auto reply = request_params(qconf, uri.getQueryParameters());
                reply->stringify(out);
            }
            else if( !query.empty() )
            {
                // запрос вида: /cmd?params
                auto reply = httpRequest(query, uri.getQueryParameters());
                reply->stringify(out);
            }
            else
            {
                // запрос без команды /?params
                auto reply = httpGet(uri.getQueryParameters());
                reply->stringify(out);
            }

            response->set_value(out.str());
            return ::grpc::Status::OK;
        }
        catch( Poco::SyntaxException& ex )
        {
            err << ex.displayText();
        }
        catch( uniset3::SystemError& ex )
        {
            err << ex;
        }
        catch( std::exception& ex )
        {
            err << ex.what();
        }

        Poco::JSON::Object jdata;
        jdata.set("error", err.str());
        jdata.set("ecode", (int)Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        //      jdata.set("ename", Poco::Net::HTTPResponse::getReasonForStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR));

        ostringstream out;
        jdata.stringify(out);
        response->set_value(out.str());
        return ::grpc::Status::OK;
#endif
    }
    // ------------------------------------------------------------------------------------------
    ostream& operator<<(ostream& os, UniSetObject& obj )
    {
        grpc::ServerContext ctx;
        const ::uniset3::GetInfoParams params;
        ::google::protobuf::StringValue response;
        obj.getInfo(&ctx, &params, &response);
        return os << response.value();
    }
    // ------------------------------------------------------------------------------------------
#undef CREATE_TIMER
} // end of namespace uniset3
