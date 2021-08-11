#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include "server.grpc.pb.h"
#include <iostream>
#include <string>
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    const std::string addr = "localhost:4444";

    unordered_map<string,string> metadata;
    metadata["srv"] = "111";
    metadata["data2"] = "2222";
    metadata["data3"] = "3333";
    metadata["service_id"] = "2121";

    try
    {
        auto chan = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
        grpc::ClientContext ctx;
        google::protobuf::Empty empty;
        google::protobuf::Int64Value reply;
        for( const auto& m: metadata )
            ctx.AddMetadata(m.first, m.second);

        std::unique_ptr<TestService_i::Stub> stub(TestService_i::NewStub(chan));
        grpc::Status st = stub->getId(&ctx, empty, &reply);

        if( st.ok() )
            cout << "id=" << reply.value() << endl;
        else
            cerr << "error: " << st.error_message() << endl;
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
