#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "UHelpers.h"
#include "TestObject.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// --------------------------------------------------------------------------
int main( int argc, const char* argv[] )
{
    try
    {
        Catch::Session session;

        if( argc > 1 && ( strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ) )
        {
            session.showHelp();
            return 0;
        }

        int returnCode = session.applyCommandLine( argc, argv );

        //if( returnCode != 0 ) // Indicates a command line error
        //    return returnCode;

        auto conf = uniset_init(argc, argv);

        auto to = make_object<TestObject>("TestProc", "TestProc");

        if( !to )
            return 1;

        auto act = UniSetActivator::Instance();

        act->add(to);
        act->run(true);
        return session.run();
    }
    catch( const std::exception& e )
    {
        cerr << "(tests_httpresolver): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(tests_httpresolver): catch(...)" << endl;
    }

    return 1;
}
