#include <string>
#include "UniSetActivator.h"
#include "Extensions.h"
#include "UHelpers.h"
#include "TestProc.h"
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

        SystemMessage sm(SystemMessage::StartUp);
        act->broadcast( sm.transport_msg() );
        act->run(true);

        SensorMessage smsg(100, 2);
        TransportMessage tm( smsg.transport_msg() );

        size_t num = 0;
        const size_t max = 100000;
        std::chrono::time_point<std::chrono::system_clock> start, end;
        start = std::chrono::system_clock::now();

        for( num = 0; num < max; num++ )
        {
            tp->push(tm);

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
    catch( const uniset3::SystemError& err )
    {
        cerr << "(mq-test): " << err << endl;
    }
    catch( const uniset3::Exception& ex )
    {
        cerr << "(mq-test): " << ex << endl;
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
