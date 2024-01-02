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
#include <grpcpp/resource_quota.h>
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
        cproxy.add(obj);
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
        cproxy.lock();

        termControl = terminate_control;
        auto conf = uniset_conf();

        auto grpcConfNode = conf->getGRPCConfNode();

        if( grpcConfNode != nullptr )
        {
            UniXML::iterator git = grpcConfNode;
            grpc::ResourceQuota rq("UniSetActivator");

            if( !git.getProp("maxThreads").empty() )
            {
                rq.SetMaxThreads(git.getIntProp("maxThreads"));
                builder.SetResourceQuota(rq);
            }

            //            builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 2000);
            //            builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 3000);
            //            builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        }

        grpcHost = conf->getGRPCHost();
        grpcPort = conf->getGRPCPort();

        ostringstream addr;
        addr << grpcHost << ":" << grpcPort;
        builder.AddListeningPort(addr.str(), grpc::InsecureServerCredentials(), &grpcPort);
        builder.RegisterService(static_cast<UniSetObject_i::Service*>(&oproxy));
        builder.RegisterService(static_cast<UniSetManager_i::Service*>(&mproxy));
        builder.RegisterService(static_cast<IOController_i::Service*>(&ioproxy));
        builder.RegisterService(static_cast<IONotifyController_i::Service*>(&ionproxy));
        builder.RegisterService(static_cast<metrics::MetricsExporter_i::Service*>(&metricsproxy));
        builder.RegisterService(static_cast<configurator::Configurator_i::Service*>(&cproxy));

        // INIT
        for( auto&& o : objects )
            o.second->initBeforeRunServer(builder);

        server = builder.BuildAndStart();
        uinfo << "GRPC Server listening on " << grpcHost << ":" << grpcPort << std::endl;
        cout << "GRPC server listening on " << grpcHost << ":" << grpcPort << std::endl;
        // INIT AFTER START
        {
            ostringstream tmp;
            tmp << grpcHost << ":" << grpcPort;
            const string realAddr = tmp.str();

            for( auto&& o : objects )
                o.second->initAfterRunServer(builder, realAddr);
        }

        if( termControl )
            set_signals(true);

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

            while( !g_done )
            {
                g_doneevent.wait(lk, []()
                {
                    return (g_done == true);
                });
            }

            shutdown();
        }
    }
    // ------------------------------------------------------------------------------------------
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

        if( server && grpcPort >= 0 )
        {
            ulogsys << myname << "(shutdown): shutdown grpc server..." << endl;
            auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
            server->Shutdown(deadline);
            ulogsys << myname << "(shutdown): shutdown grpc server [OK]" << endl;
        }

        ulogsys << myname << "(shutdown): deactivate after stop server...  " << endl;

        for( auto&& o : objects )
        {
            try
            {
                o.second->deactivateAfterStopServer();
            }
            catch(...) {}
        }

        ulogsys << myname << "(shutdown): deactivate after stop server ok. " << endl;

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

        while( !g_done )
        {
            std::unique_lock<std::mutex> lk(g_donemutex);
            g_doneevent.wait(lk, []()
            {
                return (g_done == true);
            });
        }
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

        g_doneevent.wait_until(lk, std::chrono::steady_clock::now() + std::chrono::milliseconds(TERMINATE_TIMEOUT_SEC * 1000), []()
        {
            return (g_done == true);
        });

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
} // end of namespace uniset3
// ------------------------------------------------------------------------------------------
