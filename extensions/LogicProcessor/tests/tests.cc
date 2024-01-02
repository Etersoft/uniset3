#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "NullSM.h"
#include "LProcessor.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// --------------------------------------------------------------------------
int main(int argc, const char* argv[] )
{
    try
    {
        Catch::Session session;

        if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
        {
            cout << "--confile    - Использовать указанный конф. файл. По умолчанию configure.xml" << endl;
            cout << endl << endl << "--------------- CATCH HELP --------------" << endl;
            session.showHelp();
            return 0;
        }

        int returnCode = session.applyCommandLine( argc, argv );

        //        if( returnCode != 0 ) // Indicates a command line error
        //            return returnCode;

        auto conf = uniset_init(argc, argv, "lp-configure.xml");

        auto act = UniSetActivator::Instance();

        ObjectId ns_id = conf->getControllerID("SharedMemory");

        if( ns_id == DefaultObjectId )
        {
            cerr << "Not found ID for 'SharedMemory'" << endl;
            return 1;
        }

        auto nullsm = make_shared<NullSM>(ns_id, "lp-configure.xml");
        act->add(nullsm);

        act->run(true);

        int tout = 6000;
        PassiveTimer pt(tout);

        while( !pt.checkTime() && !act->isExists() )
            msleep(100);

        if( !act->isExists() )
        {
            cerr << "(tests): UActivator not exists! (timeout=" << tout << ")" << endl;
            return 1;
        }

        return session.run();
    }
    catch( const std::exception& e )
    {
        cerr << "(tests): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(tests): catch(...)" << endl;
    }

    return 1;
}
