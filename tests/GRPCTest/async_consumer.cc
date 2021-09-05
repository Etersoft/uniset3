#include <iostream>
#include <memory>
#include "UAsyncClient.h"


using namespace std;
using namespace uniset3;

class TestObject :
        public UniSetObject
{

public:
    TestObject( UAsyncClient& _cons ):
        cons(_cons) {}

    UAsyncClient& cons;

    virtual ::grpc::Status push(::grpc::ServerContext *context, const ::uniset3::umessage::TransportMessage *msg,
                                ::google::protobuf::Empty *response) override {

        if( msg->data().Is<umessage::SensorMessage>() )
        {
            umessage::SensorMessage m;
            if( !msg->data().UnpackTo(&m) )
            {
                cerr << myname << "(push): SensorInfo: parse error" << endl;
                return grpc::Status::OK;
            }

            cerr << "SensorMessage: sid=" << m.id() << " value=" << m.value() << endl;
        }
        else if( msg->data().Is<umessage::SystemMessage>() )
        {
            umessage::SystemMessage m;
            if( !msg->data().UnpackTo(&m) )
            {
                cerr << myname << "(push): SystemMessage: parse error" << endl;
                return grpc::Status::OK;
            }

            cerr << "SystemMessage: cmd=" << m.cmd() << " data[0]=" << m.data(0) << " data[1]=" << m.data(1) << endl;
            if( m.data(1) != 0 )
            {
                cons.askSensor(10);
                cons.setValue(10,100);
                cons.setValue(10,110);
            }
        }

        return grpc::Status::OK;
    }
};

int main(int argc, char **argv) {

    try {
        uniset_init(argc, argv);

        UAsyncClient cons;
        auto obj = make_shared<TestObject>(cons);

        cons.enableConnectionEvent(1);
        cons.async_run("localhost", 4444, obj);
        cout << "async client startred.." << endl;

        msleep(500);
        cons.askSensor(10);
        cons.setValue(10,100);
        cons.setValue(10,110);
        pause();
        cons.terminate();
        return 0;
    }
    catch( std::exception &ex ) {
        cerr << ex.what() << endl;
    }

    return 1;
}
