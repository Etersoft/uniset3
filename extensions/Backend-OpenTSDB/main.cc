#include <sstream>
#include "BackendOpenTSDB.h"
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
    //  std::ios::sync_with_stdio(false);

    if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
    {
        cout << endl;
        cout << "Usage: uniset2-opentsdb-dbserver --confile configure.xml args1 args2" << endl;
        cout << endl;
        cout << "--smemory-id objectName  - SharedMemory objectID. Default: autodetect" << endl;
        cout << endl;
        BackendOpenTSDB::help_print(argc, argv);
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

        auto db = BackendOpenTSDB::init_opendtsdb(argc, argv, shmID);

        if( !db )
        {
            cerr << "(opendtsdb): init failed..." << endl;
            return 1;
        }

        auto act = UniSetActivator::Instance();
        act->add(db);

        act->startup();
        act->run(false);
        return 0;
    }
    catch( std::exception& ex )
    {
        cerr << "(opendtsdb): " << ex.what() << std::endl;
    }
    catch(...)
    {
        cerr << "(opendtsdb): catch ..." << std::endl;
    }

    return 1;
}
