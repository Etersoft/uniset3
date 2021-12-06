#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include "async_server.grpc.pb.h"
#include <iostream>
#include <string>
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    const std::string addr = "localhost:4444";

    try
    {
        auto chan = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());

        if( !chan )
        {
            cerr << "can't connect to " << addr << endl;
            return 1;
        }

        grpc::ClientContext ctx;
        TestParams request;
        request.set_cmd(1);
        request.set_msg("client message");
        RetMessage reply;

        std::unique_ptr<TestAsyncService_i::Stub> stub(TestAsyncService_i::NewStub(chan));
        auto rwStream = stub->streamTest(&ctx);

        if( !rwStream->Write(request) )
            cerr << "write error..." << endl;

        request.set_msg("client message 2");

        if( !rwStream->Write(request) )
            cerr << "write error..." << endl;

        request.set_msg("client message 3");

        if( !rwStream->Write(request) )
            cerr << "write error..." << endl;

        cout << "begin read..." << endl;

        while( rwStream->Read(&reply) )
        {
            cout << "read: " << reply.msg() << endl;
            request.set_msg("pong for " + reply.msg());

            if( !rwStream->Write(request) )
                cerr << "write error..." << endl;
        }

        cout << "read terminated..." << endl;
        rwStream->Finish();
        return 0;
    }
    catch( const std::exception& e )
    {
        cerr << "(grpc-test): " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "(grpc-test): catch(...)" << endl;
    }

    return 1;
}
