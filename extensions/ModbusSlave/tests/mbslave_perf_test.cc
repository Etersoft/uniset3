// --------------------------------------------------------------------------
#include <string>
#include "UniSetActivator.h"
#include "SharedMemory.h"
#include "MBSlave.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// --------------------------------------------------------------------------
int main( int argc, const char** argv )
{
    try
    {
        auto conf = uniset_init(argc, argv);

        auto act = UniSetActivator::Instance();

        // ------------ SharedMemory ----------------
        auto shm = SharedMemory::init_smemory(argc, argv);

        if( !shm )
            return 1;

        act->add(shm);

        int num = conf->getArgPInt("--numproc", 10);

        for( int i = 1; i <= num; i++)
        {
            //          auto mbs = MBSlave::init_mbslave(argc, argv, shm->getId(), shm, "mbs");
            ostringstream s;
            s << "MBTCP" << i;

            cout << "..create " << s.str() << endl;

            ostringstream p;
            p << "mbs" << i;

            auto mbs = make_shared<MBSlave>( conf->getObjectID(s.str()), shm->getId(), shm, p.str());

            if( !mbs )
            {
                cerr << "Ошибка при создании MBSlave " << s.str() << endl;
                return 1;
            }

            act->add(mbs);
        }

        // -------------------------------------------
        act->run(false);
        return 0;
    }
    catch( std::exception& ex )
    {
        cerr << "(mbslave_perf_test): " << ex.what() << endl;
    }
    catch(...)
    {
        cerr << "(mbslave_perf_test): catch(...)" << endl;
    }

    return 1;
}
// --------------------------------------------------------------------------
