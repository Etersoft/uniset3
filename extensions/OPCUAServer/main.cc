#include "Configuration.h"
#include "Extensions.h"
#include "OPCUAServer.h"
#include "UniSetActivator.h"
// --------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    //  std::ios::sync_with_stdio(false);

    try
    {
        if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
        {
            cout << "--confile filename - configuration file. Default: configure.xml" << endl;
            OPCUAServer::help_print();
            return 0;
        }

        auto conf = uniset_init(argc, argv);

        ObjectId shmID = DefaultObjectId;
        string sID = conf->getArgParam("--smemory-id");

        if( !sID.empty() )
            shmID = conf->getControllerID(sID);
        else
            shmID = uniset3::extensions::getSharedMemoryID();

        if( shmID == DefaultObjectId )
        {
            cerr << sID << "? SharedMemoryID not found in " << conf->getControllersSection() << " section" << endl;
            return 1;
        }

        auto srv = OPCUAServer::init_opcua_server(argc, argv, shmID, nullptr, "opcua");

        if( !srv )
            return 1;

        auto act = UniSetActivator::Instance();
        act->add(srv);

        act->run(false);
        return 0;
    }
    catch( const std::exception& ex )
    {
        cerr << "(OPCUAServer::main): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(OPCUAServer::main): catch ..." << endl;
    }

    return 1;
}
