/*
 * Copyright (c) 2021 Pavel Vainerman.
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
#include <fstream>

#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>

// for stack trace
// --------------------
//#include <execinfo.h>
//#include <cxxabi.h>
//#include <dlfcn.h>
#include <iomanip>
// --------------------
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include "Exceptions.h"
#include "unisetstd.h"
#include "UInterface.h"
#include "UniSetActivator.h"
#include "Debug.h"
#include "Configuration.h"
#include "Mutex.h"
#include "UHelpers.h"
#include "UniSetObjectProxy.h"
#include "UniSetManagerProxy.h"
#include "IOControllerProxy.h"
#include "IONotifyControllerProxy.h"


// ------------------------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// ------------------------------------------------------------------------------------------
static std::mutex              g_donemutex;
static std::condition_variable g_doneevent;
static std::shared_ptr<std::thread> g_finish_guard_thread;
static std::atomic_bool g_done = ATOMIC_VAR_INIT(0);

static const int TERMINATE_TIMEOUT_SEC = 15; //  время, отведённое на завершение процесса [сек]
// ------------------------------------------------------------------------------------------
struct ServiceThreadDeleter
{
    void operator()( ThreadCreator<UniSetActivator>* p ) const
    {
        // не удаляем..
    }
};
// ---------------------------------------------------------------------------
namespace uniset3
{
    UniSetActivatorPtr UniSetActivator::inst;
    // ---------------------------------------------------------------------------
    UniSetActivatorPtr UniSetActivator::Instance()
    {
        if( inst == nullptr )
            inst = shared_ptr<UniSetActivator>( new UniSetActivator() );

        return inst;
    }

    // ---------------------------------------------------------------------------
    UniSetActivator::UniSetActivator():
        myname("UniSetActivator")
    {
        UniSetActivator::init();
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::init()
    {
        auto conf = uniset_conf();

#ifndef DISABLE_REST_API

        if( findArgParam("--activator-run-httpserver", conf->getArgc(), conf->getArgv()) != -1 )
        {
            httpHost = conf->getArgParam("--activator-httpserver-host", "localhost");
            httpPort = conf->getArgInt("--activator-httpserver-port", "8080");
            ulog1 << myname << "(init): http server parameters " << httpHost << ":" << httpPort << endl;
            httpCORS_allow = conf->getArgParam("--activator-httpserver-cors-allow", "*");
        }

#endif
    }

    // ------------------------------------------------------------------------------------------
    UniSetActivator::~UniSetActivator()
    {
        if( active )
        {
            try
            {
                shutdown();
            }
            catch(...)
            {
                std::exception_ptr p = std::current_exception();
                cerr << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
            }
        }
    }
    // ------------------------------------------------------------------------------------------
    bool UniSetActivator::add( const std::shared_ptr<UniSetObject>& obj )
    {
        uniset_rwmutex_wrlock lock(omutex);
        auto i = objects.find(obj->getId());

        if( i == objects.end() )
            objects.emplace(obj->getId(), obj);

        auto ion = std::dynamic_pointer_cast<IONotifyController>(obj);

        if( ion )
            ionproxy.add(ion);

        auto io = std::dynamic_pointer_cast<IOController>(obj);

        if( io )
            ioproxy.add(io);

        auto m = std::dynamic_pointer_cast<UniSetManager>(obj);

        if( m )
            mproxy.add(m);

        oproxy.add(obj);
        metricsproxy.add(obj);
        return true;
    }
    // ------------------------------------------------------------------------------------------
    bool UniSetActivator::isExists() const noexcept
    {
        return active;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::startup()
    {
        uniset3:: umessage::SystemMessage sm = makeSystemMessage(uniset3::umessage::SystemMessage::StartUp);
        auto tm = to_transport<uniset3::umessage::SystemMessage>(sm);
        grpc::ServerContext ctx;
        google::protobuf::Empty reply;

        for( auto&& o : objects )
        {
            try
            {
                o.second->push(&ctx, &tm, &reply);
            }
            catch( std::exception& ex ) {}
        }
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::run( bool thread, bool terminate_control  )
    {
        ulogsys << myname << "(run): ..." << endl;

        oproxy.lock();
        mproxy.lock();
        ioproxy.lock();
        ionproxy.lock();
        metricsproxy.lock();

        termControl = terminate_control;
        auto conf = uniset_conf();

        grpcHost = conf->getArgParam("--activator-grpc-host", "0.0.0.0");
        grpcPort = conf->getArgInt("--activator-grpc-port", "0");

        ostringstream addr;
        addr << grpcHost << ":" << grpcPort;
        builder.AddListeningPort(addr.str(), grpc::InsecureServerCredentials(), &grpcPort);
        builder.RegisterService(static_cast<UniSetObject_i::Service*>(&oproxy));
        builder.RegisterService(static_cast<UniSetManager_i::Service*>(&mproxy));
        builder.RegisterService(static_cast<IOController_i::Service*>(&ioproxy));
        builder.RegisterService(static_cast<IONotifyController_i::Service*>(&ionproxy));
        builder.RegisterService(static_cast<metrics::MetricsExporter_i::Service*>(&metricsproxy));
        server = builder.BuildAndStart();

        uinfo << "GRPC Server listening on " << grpcHost << ":" << grpcPort << std::endl;
        cout << "GRPC server listening on " << grpcHost << ":" << grpcPort << std::endl;

        // INIT
        {
            ostringstream tmp;
            tmp << grpcHost << ":" << grpcPort;
            const string realAddr = tmp.str();

            for( auto&& o : objects )
                o.second->init(realAddr);
        }

        if( termControl )
            set_signals(true);

#ifndef DISABLE_REST_API

        if( !httpHost.empty() )
        {
            try
            {
                auto reg = dynamic_pointer_cast<UHttp::IHttpRequestRegistry>(shared_from_this());
                httpserv = make_shared<UHttp::UHttpServer>(reg, httpHost, httpPort);
                httpserv->setCORS_allow(httpCORS_allow);
                httpserv->start();
            }
            catch( std::exception& ex )
            {
                uwarn << myname << "(run): init http server error: " << ex.what() << endl;
            }
        }

#endif
        // ACTIVATE
        uinfo << myname << "(run): activate objects.." << endl;
        active = true;

        for( auto&& o : objects )
            o.second->activate();

        for( auto&& o : objects )
            o.second->postActivateObjects();

        startup();

        if( !thread )
        {
            std::unique_lock<std::mutex> lk(g_donemutex);
            g_doneevent.wait_for(lk, std::chrono::milliseconds(TERMINATE_TIMEOUT_SEC * 1000), []()
            {
                return (g_done == true);
            });
            shutdown();
        }
    }
    // ------------------------------------------------------------------------------------------
    /*!
     *    Функция останавливает работу orb и завершает поток, а также удаляет ссылку из репозитория.
     *    \note Объект становится недоступен другим процессам
    */
    void UniSetActivator::shutdown()
    {
        if( !active )
            return;

        active = false;

        if( termControl )
        {
            set_signals(false);
            {
                std::unique_lock<std::mutex> lk(g_donemutex);
                g_done = false;
                g_finish_guard_thread = make_shared<std::thread>(on_finish_timeout);
            }
        }

        ulogsys << myname << "(shutdown): deactivate...  " << endl;

        for( auto&& o : objects )
            o.second->deactivate();

        ulogsys << myname << "(shutdown): deactivate ok.  " << endl;

#ifndef DISABLE_REST_API

        if( httpserv )
            httpserv->stop();

#endif

#if 0

        if( server && grpcPort >= 0 )
        {
            ulogsys << myname << "(shutdown): shutdown grpc server..." << endl;
            server->Shutdown();
            ulogsys << myname << "(shutdown): shutdown grpc server [OK]" << endl;
        }

#endif
        {
            std::unique_lock<std::mutex> lk(g_donemutex);
            g_done = true;
        }

        g_doneevent.notify_all();

        if( g_finish_guard_thread )
            g_finish_guard_thread->join();
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::join()
    {
        if( g_done )
            return;

        ulogsys << myname << "(join): ..." << endl;

        std::unique_lock<std::mutex> lk(g_donemutex);
        g_doneevent.wait(lk, []()
        {
            return (g_done == true);
        } );
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::terminate()
    {
        ulogsys << myname << "(terminate): ..." << endl;
        kill(getpid(), SIGTERM);
    }
    // ------------------------------------------------------------------------------------------
    static void activator_terminate( int signo )
    {
        auto act = UniSetActivator::Instance();
        act->shutdown();
        ulogsys << "******** activator_terminate finished **** " << endl;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::on_finish_timeout()
    {
        std::unique_lock<std::mutex> lk(g_donemutex);

        if( g_done )
            return;

        ulogsys << "(FINISH GUARD THREAD): wait " << TERMINATE_TIMEOUT_SEC << " sec.." << endl << flush;

        g_doneevent.wait_for(lk, std::chrono::milliseconds(TERMINATE_TIMEOUT_SEC * 1000), []()
        {
            return (g_done == true);
        } );

        if( !g_done )
        {
            ulogsys << "(FINISH GUARD THREAD): WAIT TIMEOUT "
                    << TERMINATE_TIMEOUT_SEC << " sec..KILL *******" << endl << flush;
            set_signals(false);
            std::abort();
            return;
        }

        ulogsys << "(FINISH GUARD THREAD): [OK]..bye.." << endl;
    }
    // ------------------------------------------------------------------------------------------
    void UniSetActivator::set_signals( bool ask )
    {
        struct sigaction act; // = { { 0 } };
        struct sigaction oact; // = { { 0 } };
        memset(&act, 0, sizeof(act));
        memset(&act, 0, sizeof(oact));

        sigemptyset(&act.sa_mask);
        sigemptyset(&oact.sa_mask);

        // добавляем сигналы, которые будут игнорироваться
        // при обработке сигнала
        sigaddset(&act.sa_mask, SIGINT);
        sigaddset(&act.sa_mask, SIGTERM);
        sigaddset(&act.sa_mask, SIGABRT );
        sigaddset(&act.sa_mask, SIGQUIT);

        if(ask)
            act.sa_handler = activator_terminate;
        else
            act.sa_handler = SIG_DFL;

        sigaction(SIGINT, &act, &oact);
        sigaction(SIGTERM, &act, &oact);
        sigaction(SIGABRT, &act, &oact);
        sigaction(SIGQUIT, &act, &oact);

        // SIGSEGV отдельно
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGSEGV);
        act.sa_flags = 0;
        //  act.sa_flags |= SA_RESTART;
        act.sa_flags |= SA_RESETHAND;

#if 0
        g_sigseg_stack.ss_sp = g_stack_body;
        g_sigseg_stack.ss_flags = SS_ONSTACK;
        g_sigseg_stack.ss_size = sizeof(g_stack_body);
        assert(!sigaltstack(&g_sigseg_stack, nullptr));
        act.sa_flags |= SA_ONSTACK;
#endif

        //  if(ask)
        //      act.sa_handler = activator_terminate_with_calltrace;
        //  else
        //      act.sa_handler = SIG_DFL;

        //  sigaction(SIGSEGV, &act, &oact);
    }
    // ------------------------------------------------------------------------------------------
#ifndef DISABLE_REST_API
    Poco::JSON::Object::Ptr UniSetActivator::httpGetByName( const string& name, const Poco::URI::QueryParameters& p )
    {
#if 0

        if( name == myname )
            return httpGet(p);

        auto obj = deepFind            msleep(10000);
        Object(name);

        if( obj )
            return obj->httpGet(p);

#endif
        ostringstream err;
        err << "Object '" << name << "' not found";

        throw uniset3::NameNotFound(err.str());
    }
    // ------------------------------------------------------------------------------------------
    Poco::JSON::Array::Ptr UniSetActivator::httpGetObjectsList( const Poco::URI::QueryParameters& p )
    {
        Poco::JSON::Array::Ptr jdata = new Poco::JSON::Array();

#if 0
        std::vector<std::shared_ptr<UniSetObject>> vec;
        vec.reserve(objectsCount());

        //! \todo Доделать обработку параметров beg,lim на случай большого количества объектов (и частичных запросов)
        size_t lim = 1000;
        getAllObjectsList(vec, lim);

        for( const auto& o : vec )
            jdata->add(o->getName());

#endif
        return jdata;
    }
    // ------------------------------------------------------------------------------------------
    Poco::JSON::Object::Ptr UniSetActivator::httpHelpByName( const string& name, const Poco::URI::QueryParameters& p )
    {
#if 0

        if( name == myname )
            return httpHelp(p);

        auto obj = deepFindObject(name);

        if( obj )
            return obj->httpHelp(p);

#endif
        ostringstream err;
        err << "Object '" << name << "' not found";
        throw uniset3::NameNotFound(err.str());
    }
    // ------------------------------------------------------------------------------------------
    Poco::JSON::Object::Ptr UniSetActivator::httpRequestByName( const string& name, const std::string& req, const Poco::URI::QueryParameters& p)
    {
#if 0

        if( name == myname )
            return httpRequest(req, p);

        // а вдруг встретится объект с именем "conf" а мы перекрываем имя?!
        // (пока считаем что такого не будет)
        if( name == "configure" )
            return request_configure(req, p);

        auto obj = deepFindObject(name);

        if( obj )
            return obj->httpRequest(req, p);

#endif
        ostringstream err;
        err << "Object '" << name << "' not found";
        throw uniset3::NameNotFound(err.str());
    }
    // ------------------------------------------------------------------------------------------
#endif // #ifndef DISABLE_REST_API
    // ------------------------------------------------------------------------------------------
} // end of namespace uniset3
// ------------------------------------------------------------------------------------------
