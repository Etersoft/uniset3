#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <memory>
#include <string>
#include "Debug.h"
#include "UniSetActivator.h"
#include "PassiveTimer.h"
#include "SharedMemory.h"
#include "SMInterface.h"
#include "Extensions.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::extensions;
// --------------------------------------------------------------------------
static shared_ptr<SMInterface> smi;
static shared_ptr<SharedMemory> shm;
static shared_ptr<UInterface> ui;
static ObjectId myID = 6000;
static ObjectId shmID = DefaultObjectId;
// --------------------------------------------------------------------------
shared_ptr<SharedMemory> shmInstance()
{
    if( !shm )
        throw SystemError("SharedMemory don`t initialize..");

    return shm;
}
// --------------------------------------------------------------------------
shared_ptr<SMInterface> smiInstance()
{
    if( smi == nullptr )
    {
        if( shm == nullptr )
            throw SystemError("SharedMemory don`t initialize..");

        if( ui == nullptr )
            ui = make_shared<UInterface>();

        smi = make_shared<SMInterface>(shm->getId(), ui, myID, shm );
    }

    return smi;
}
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
            session.showHelp();
            return 0;
        }

        int returnCode = session.applyCommandLine( argc, argv );

        //        if( returnCode != 0 ) // Indicates a command line error
        //            return returnCode;

        auto conf = uniset_init(argc, argv);

        shm = SharedMemory::init_smemory(argc, argv);

        if( !shm )
            return 1;

        shmID = shm->getId();

        auto act = UniSetActivator::Instance();

        act->add(shm);
        act->run(true);

        int tout = 6000;
        PassiveTimer pt(tout);

        while( !pt.checkTime() && !act->isExists() )
            msleep(100);

        if( !act->isExists() )
        {
            cerr << "(tests_with_sm): SharedMemory not exists! (timeout=" << tout << ")" << endl;
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
