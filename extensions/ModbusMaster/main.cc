#include <sstream>
#include "MBTCPMaster.h"
#include "Configuration.h"
#include "Debug.h"
#include "UniSetActivator.h"
#include "Extensions.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::extensions;
// -----------------------------------------------------------------------------
int main( int argc, const char** argv )
{
    //std::ios::sync_with_stdio(false);

    if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
    {
        cout << endl;
        cout << "Usage: uniset3-mbtcpmaster args1 args2" << endl;
        cout << endl;
        cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
        cout << "--confile filename       - configuration file. Default: configure.xml" << endl;
        cout << endl;
        MBTCPMaster::help_print(argc, argv);
        cout << endl;
        cout << " Global options:" << endl;
        cout << uniset3::Configuration::help() << endl;
        return 0;
    }

    try
    {
        auto conf = uniset_init( argc, argv );

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

        auto mb = MBTCPMaster::init_mbmaster(argc, argv, shmID);

        if( !mb )
        {
            dcrit << "(mbmaster): init MBTCPMaster failed." << endl;
            return 1;
        }

        auto act = UniSetActivator::Instance();
        act->add(mb);

        act->run(false);
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(mbtcpmaster): " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::exception_ptr p = std::current_exception();
        cerr << "(mbtcpmaster): catch.." << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
    }

    return 1;
}
