#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include "server.grpc.pb.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// --------------------------------------------------------------------------
class TestServerImpl:
    public TestService_i::Service
{
    public:
        TestServerImpl( const std::string& name):
            myname(name)
        {}
        virtual ~TestServerImpl() {}
        virtual ::grpc::Status getId(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Int64Value* response)
        {
            throw std::exception();

            cerr << myname << ": call " << endl;

            for( const auto& i : context->client_metadata() )
                cout << i.first << "=" << i.second << endl;

            auto srvIt = context->client_metadata().find("serviceId");

            if( srvIt != context->client_metadata().end() )
                cout << "serviceId=" <<  atoi(srvIt->second.data());

            response->set_value(7);
            return grpc::Status::OK;
        }

        virtual ::grpc::Status streamTest(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::grpc::ServerWriter< ::google::protobuf::StringValue>* writer) override
        {
            int i = 0;

            while(true)
            {
                ::google::protobuf::StringValue s;
                s.set_value(to_string(i));

                if( !writer->Write(s) )
                {
                    cerr << "client closed.." << endl;
                    return grpc::Status::OK;
                }

                i++;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            return grpc::Status::OK;
        }

    protected:
        const std::string myname;

};
// --------------------------------------------------------------------------
int main(int argc, const char** argv)
{
    const std::string addr = "0.0.0.0:4444";

    try
    {
        grpc::EnableDefaultHealthCheckService(true);
        //    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
        grpc::ServerBuilder builder;
        int port;
        builder.AddListeningPort(addr, grpc::InsecureServerCredentials(), &port);

        TestServerImpl s1("svc1");
        builder.RegisterService(static_cast<TestService_i::Service*>(&s1));
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        cout << "service listening on 0.0.0.0:" << port << std::endl;
        server->Wait();
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
