#include <memory>
#include <chrono>
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
static ObjectId begSensorID = 50000;
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
void run_test(std::size_t concurrency, int bound, shared_ptr<SharedMemory>& shm )
{
    auto&& r_worker = [&shm, bound]
    {
        grpc::ServerContext ctx;
        int num = bound;
        ::uniset3::GetValueParams sid;
        sid.set_id(begSensorID + rand() % 10000);
        google::protobuf::Int64Value ret;

        while (num--)
        {
            sid.set_id(begSensorID + rand() % 10000);
            shm->getValue(&ctx, &sid, &ret);
        }
    };

    auto&& w_worker = [&shm, bound]
    {
        grpc::ServerContext ctx;
        google::protobuf::Empty empty;
        int num = bound;

        SetValueParams p;
        p.set_id(begSensorID + rand() % 10000);
        p.set_sup_id(DefaultObjectId);

        while (num--)
        {
            p.set_value(num);
            shm->setValue(&ctx, &p, &empty);
        }
    };

    std::vector<std::thread> threads;

    for (std::size_t i = 0; i < concurrency / 2 - 1; ++i)
        threads.emplace_back(r_worker);

    for (std::size_t i = 0; i < concurrency / 2 - 1; ++i)
        threads.emplace_back(w_worker);

    for (auto&& thread : threads)
        thread.join();
}
// --------------------------------------------------------------------------
int main(int argc, char* argv[] )
{
    try
    {
        auto conf = uniset_init(argc, argv);

        shm = SharedMemory::init_smemory(argc, argv);

        if( !shm )
            return 1;

        auto act = UniSetActivator::Instance();

        act->add(shm);
        act->run(true);

        int tout = 10000;
        PassiveTimer pt(tout);

        while( !pt.checkTime() && !act->isExists() )
            msleep(100);

        if( !act->isExists() )
        {
            cerr << "(tests_with_sm): SharedMemory not exist! (timeout=" << tout << ")" << endl;
            return 1;
        }

        std::chrono::time_point<std::chrono::system_clock> start, end;
        start = std::chrono::system_clock::now();
        run_test( std::thread::hardware_concurrency(), 1000000, shm);
        end = std::chrono::system_clock::now();

        int elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cerr << "elapsed time: " << elapsed_seconds << " ms\n";
        return 0;
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
