#include <sstream>
#include "UniSetActivator.h"
#include "Extensions.h"
#include "UNetExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::extensions;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
    //  std::ios::sync_with_stdio(false);

    try
    {
        if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
        {
            cout << endl;
            cout << "Usage: uniset2-unetexchange --confile configure.xml args1 args2" << endl;
            cout << endl;
            cout << "--smemory-id objectName  - SharedMemory objectID. Default: read from <SharedMemory>" << endl;
            cout << endl;
            UNetExchange::help_print(argc, argv);
            cout << " Global options:" << endl;
            cout << uniset3::Configuration::help() << endl;
            return 0;
        }

        auto conf = uniset_init(argc, argv);

        ObjectId shmID = DefaultObjectId;
        string sID = conf->getArgParam("--smemory-id");

        if( !sID.empty() )
            shmID = conf->getControllerID(sID);
        else
            shmID = getSharedMemoryID();

        if( shmID == DefaultObjectId )
        {
            cerr << sID << "? SharedMemoryID not found in " << conf->getControllersSection() << " section" << endl;
            return 1;
        }

        auto unet = UNetExchange::init_unetexchange(argc, argv, shmID);

        if( !unet )
        {
            cerr << "(unetexchange): init failed.." << endl;
            return 1;
        }

        auto act = UniSetActivator::Instance();
        act->add(unet);

        act->startup();
        act->run(false);
    }
    catch( const std::exception& ex )
    {
        cerr << "(unetexchange): " << ex.what() << std::endl;
    }
    catch(...)
    {
        cerr << "(unetexchange): catch ..." << std::endl;
    }

    return 0;
}
