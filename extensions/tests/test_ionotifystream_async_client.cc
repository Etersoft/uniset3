#include <catch.hpp>

#include <memory>
#include <time.h>
#include "UInterface.h"
#include "UniSetTypes.h"
#include "UHelpers.h"
#include "IONotifyStreamAsyncClient.h"
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
    ClientRAII(IONotifyStreamAsyncClient& _cons):
        cons(_cons){}

    ~ClientRAII()
    {
        cons.terminate();
    }

    IONotifyStreamAsyncClient& cons;
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

TEST_CASE("IONotifyStreamAsyncClient with UniSetObject", "[IONotifyStreamAsyncClient][uobject]")
{
    async_test_init();

    IONotifyStreamAsyncClient cli;
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
TEST_CASE("IONotifyStreamAsyncClient with callback", "[IONotifyStreamAsyncClient][cb]")
{
    async_test_init();

    IONotifyStreamAsyncClient cli;
    ClientRAII raii(cli);

    cli.enableConnectionEvent(1);

    umessage::SensorMessage sm;
    umessage::SystemMessage sysm;

    cli.async_run_cb("localhost", SM_GRPC_PORT, [&](const uniset3::umessage::TransportMessage *msg){
        if( msg->data().Is<umessage::SensorMessage>() )
        {
            REQUIRE( msg->data().UnpackTo(&sm) );
        }
        else if( msg->data().Is<umessage::SystemMessage>() )
        {
            REQUIRE( msg->data().UnpackTo(&sysm) );
        }
    });

    uniset3::PassiveTimer ptTimeout(2000);
    while( !cli.isConnected() && !ptTimeout.checkTime() )
        msleep(200);

    REQUIRE_FALSE(ptTimeout.checkTime() );

    // ask sensor
    cli.askSensor(async_sid);
    msleep(500);
    REQUIRE( sm.id() == async_sid );
    //    REQUIRE( sm.value() == 10 ); // default value

    cli.setValue(async_sid,100);
    msleep(500);
    REQUIRE( sm.id() == async_sid );
    REQUIRE( sm.value() == 100 );
}
// -----------------------------------------------------------------------------
