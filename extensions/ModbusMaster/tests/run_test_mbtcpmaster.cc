#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "Extensions.h"
#include "MBTCPMaster.h"
#include "SMInterface.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::extensions;
// --------------------------------------------------------------------------
std::shared_ptr<SharedMemory> shm;
std::shared_ptr<MBTCPMaster> mbm;
// --------------------------------------------------------------------------
int main( int argc, const char* argv[] )
{
    try
    {
        Catch::Session session;

        if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
        {
            cout << "--confile    - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
            SharedMemory::help_print(argc, argv);
            cout << endl << endl << "--------------- CATCH HELP --------------" << endl;
            session.showHelp();
            return 0;
        }

        int returnCode = session.applyCommandLine( argc, argv );

        //        if( returnCode != 0 ) // Indicates a command line error
        //            return returnCode;

        auto conf = uniset_init(argc, argv);
        dlog()->logFile("./smtest.log");

        bool apart = findArgParam("--apart", argc, argv) != -1;

        shm = SharedMemory::init_smemory(argc, argv);

        if( !shm )
            return 1;

        mbm = MBTCPMaster::init_mbmaster(argc, argv, shm->getId(), (apart ? nullptr : shm ));

        if( !mbm )
            return 1;

        auto act = UniSetActivator::Instance();
        act->add(shm);
        act->add(mbm);

        act->run(true);

        int tout = conf->getArgPInt("--timeout", 8000);
        PassiveTimer pt(tout);

        while( !pt.checkTime() && !act->isExists() && !mbm->isExists() )
            msleep(100);

        if( !act->isExists() )
        {
            cerr << "(tests_mbtcpmaster): SharedMemory not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        if( !mbm->isExists() )
        {
            cerr << "(tests_mbtcpmaster): ModbusMaster not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        return session.run();
    }
    catch( const std::exception& e )
    {
        cerr << "(tests_mbtcpmaster): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(tests_mbtcpmaster): catch(...)" << endl;
    }

    return 1;
}
