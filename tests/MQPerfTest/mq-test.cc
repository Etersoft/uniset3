#include <string>
#include <iostream>
#include <assert.h>
#include <thread>
#include <atomic>
#include "Configuration.h"
#include "Exceptions.h"
#include "MQAtomic.h"
#include "MQMutex.h"
#include "UHelpers.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// --------------------------------------------------------------------------
MQAtomic mq; // тестируемая очередь

const size_t COUNT = 1000000; // сколько сообщений поместить в очередь
// --------------------------------------------------------------------------
// поток записи
void mq_write_thread()
{
    uniset3::umessage::SensorMessage smsg = makeSensorMessage(100, 2, uniset3::AI);
    auto vm = make_shared<uniset3::umessage::TransportMessage>(uniset3::to_transport<uniset3::umessage::SensorMessage>(smsg));

    msleep(100);

    for( size_t i = 0; i < COUNT; i++ )
    {
        mq.push(vm);
    }
}
// --------------------------------------------------------------------------
int one_test()
{
    auto wthread = std::thread(mq_write_thread);

    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    size_t rnum = 0;

    while( rnum < COUNT )
    {
        auto m = mq.top();

        if( m )
            rnum++;
    }

    wthread.join();

    end = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{

    try
    {
        uniset_init(argc, argv);

        auto sm = makeSensorMessage(100, 2, uniset3::AI);
        auto tmp = sm.SerializeAsString();
        cout << "sm: " << tmp.size() << endl;

        auto vm = uniset3::to_transport<uniset3::umessage::SensorMessage>(sm);
        auto tmp2 = vm.SerializeAsString();
        cout << "tm: " << tmp2.size() << endl;

        return 0;

        int tnum = 10;

        // чтобы не происходило переполнение
        mq.setMaxSizeOfMessageQueue(COUNT + 1);

        // сперва просто проверка что очередь работает.
        {
            uniset3::umessage::SensorMessage sm = makeSensorMessage(100, 2, uniset3::AI);
            auto vm = make_shared<uniset3::umessage::TransportMessage>(uniset3::to_transport<uniset3::umessage::SensorMessage>(sm));

            mq.push(vm);
            auto msg = mq.top();
            assert( msg != nullptr );

            uniset3::umessage::SensorMessage sm2;
            assert( vm->data().UnpackTo(&sm2) );
            assert( sm.id() == sm2.id() );
        }

        vector<int> res;
        res.reserve(tnum);

        for( int i = 0; i < tnum; i++ )
        {
            res.push_back(one_test());
        }

        // вычисляем среднее
        int sum = 0;

        for( auto&& r : res )
            sum += r;

        float avg = (float)sum / tnum;

        std::cerr << "average elapsed time [" << tnum << "]: " << avg << " msec for " << COUNT << endl;

        return 0;
    }
    catch( const std::exception& e )
    {
        cerr << "(mq-test): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(mq-test): catch(...)" << endl;
    }

    return 1;
}
