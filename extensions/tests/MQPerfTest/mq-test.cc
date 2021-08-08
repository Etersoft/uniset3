#include <string>
#include "UniSetActivator.h"
#include "Extensions.h"
#include "UHelpers.h"
#include "TestProc.h"
#include "MessageTypes.pb.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::extensions;
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    try
    {
        auto conf = uniset_init(argc, argv);

        auto act = UniSetActivator::Instance();

        auto tp = uniset3::make_object<TestProc>("TestProc1", "TestProc");
        act->add(tp);

        act->run(true);

        uniset3::umessage::SensorMessage smsg;
        smsg.set_id(100);
        smsg.set_value(2);

        auto tm = uniset3::to_transport<uniset3::umessage::SensorMessage>(smsg);

        size_t num = 0;
        const size_t max = 100000;
        std::chrono::time_point<std::chrono::system_clock> start, end;
        start = std::chrono::system_clock::now();

        grpc::ServerContext ctx;
        google::protobuf::Empty response;

        for( num = 0; num < max; num++ )
        {
            tp->push(&ctx, &tm, &response);

            if( tp->isFullQueue() )
                break;

            if( num % 100 == 0 )
                msleep(50);
        }

        end = std::chrono::system_clock::now();
        int elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cerr << "elapsed time: " << elapsed_seconds << " ms num=" << num << endl;
        std::cerr << "speed: " << ( num > 0 ? ((float)elapsed_seconds / num) : 0 ) << " msg per sec" << endl;

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
