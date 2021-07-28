#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "UWebSocketGate.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::extensions;
// --------------------------------------------------------------------------
int main(int argc, const char* argv[] )
{
    try
    {
        Catch::Session session;

        if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
        {
            cout << "--confile    - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
            SharedMemory::help_print(argc, argv);
            cout << endl << endl << "--------------- CATCH HELP --------------" << endl;
            session.showHelp("test_with_sm");
            return 0;
        }

        int returnCode = session.applyCommandLine( argc, argv, Catch::Session::OnUnusedOptions::Ignore );

        if( returnCode != 0 ) // Indicates a command line error
            return returnCode;

        auto conf = uniset_init(argc, argv);

        bool apart = findArgParam("--apart", argc, argv) != -1;

        auto shm = SharedMemory::init_smemory(argc, argv);

        if( !shm )
            return 1;

        auto ws = UWebSocketGate::init_wsgate(argc, argv, shm->getId(), shm, "ws-");

        if( !ws )
            return 1;

        auto act = UniSetActivator::Instance();

        act->add(shm);
        act->add(ws);

        act->startup();
        act->run(true);

        int tout = 6000;
        PassiveTimer pt(tout);

        while( !pt.checkTime() && !act->isExists() && !ws->isExists() )
            msleep(100);

        if( !act->isExists() )
        {
            cerr << "(tests_with_sm): SharedMemory not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        if( !ws->isExists() )
        {
            cerr << "(tests_with_sm): UWebSocketGate not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        return session.run();
    }
    catch( const std::exception& e )
    {
        cerr << "(tests_with_sm): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(tests_with_sm): catch(...)" << endl;
    }

    return 1;
}
