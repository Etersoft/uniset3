#include <catch.hpp>

#include <memory>
#include <time.h>
#include "UInterface.h"
#include "UniSetTypes.h"
#include "UHelpers.h"
#include "UAsyncClient.h"
#include "PassiveTimer.h"

using namespace std;
using namespace uniset3;
using namespace uniset3::umessage;

extern ObjectId shmID;
const int SM_GRPC_PORT = 4444; // see tests_with_sm.sh
const ObjectId async_sid = 123;

class ClientRAII
{
public:
    ClientRAII(UAsyncClient& _cons):
        cons(_cons){}

    ~ClientRAII()
    {
        cons.terminate();
    }

    UAsyncClient& cons;
};

class TestObject :
    public UniSetObject
{

    public:
        TestObject(){}
        ~TestObject(){}

        umessage::SensorMessage sm;
        umessage::SystemMessage sysm;

        virtual ::grpc::Status push(::grpc::ServerContext* context, const ::uniset3::umessage::TransportMessage* msg,
                                    ::google::protobuf::Empty* response) override
        {
            if( msg->data().Is<umessage::SensorMessage>() )
            {
                if( !msg->data().UnpackTo(&sm) )
                {
                    cerr << myname << "(push): SensorInfo: parse error" << endl;
                    return grpc::Status::OK;
                }
            }
            else if( msg->data().Is<umessage::SystemMessage>() )
            {
                if( !msg->data().UnpackTo(&sysm) )
                {
                    cerr << myname << "(push): SystemMessage: parse error" << endl;
                    return grpc::Status::OK;
                }

//                if( m.data(1) != 0 )
//                {
//                    cons.askSensor(10);
//                    cons.setValue(10, 100);
//                    cons.setValue(10, 110);
//                }
            }

            return grpc::Status::OK;
        }
};


void async_test_init()
{
    CHECK( uniset_conf() != nullptr );
    auto conf = uniset_conf();
    REQUIRE_FALSE( conf->oind->getNameById(async_sid).empty() );
}

TEST_CASE("UAsyncClient", "[UAsyncClient]")
{
    async_test_init();

    UAsyncClient cli;
    ClientRAII raii(cli);

    auto obj = make_shared<TestObject>();
    cli.enableConnectionEvent(1);
    cli.async_run("localhost", SM_GRPC_PORT, obj);

    uniset3::PassiveTimer ptTimeout(2000);
    while( !cli.isConnected() && !ptTimeout.checkTime() )
        msleep(200);

    REQUIRE_FALSE(ptTimeout.checkTime() );

    // ask sensor
    cli.askSensor(async_sid);
    msleep(500);
    REQUIRE( obj->sm.id() == async_sid );
//    REQUIRE( obj->sm.value() == 10 ); // default value

    cli.setValue(async_sid,100);
    msleep(500);
    REQUIRE( obj->sm.id() == async_sid );
    REQUIRE( obj->sm.value() == 100 );
}
// -----------------------------------------------------------------------------
