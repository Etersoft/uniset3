#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <queue>
#include <atomic>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/alarm.h>
#include "async_server.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using namespace std;

typedef ::grpc::ServerAsyncReaderWriter<::uniset3::RetMessage, ::uniset3::TestParams> RWResponder;

class Client;

class AsyncClientSession;

class ServerImpl final
{
    public:
        ~ServerImpl()
        {
            server_->Shutdown();
            // Always shutdown the completion queue after the server.
            cq->Shutdown();
        }

        // There is no shutdown handling in this code.
        void Run()
        {
            std::string server_address("0.0.0.0:4444");

            ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&service_);
            cq = builder.AddCompletionQueue();
            server_ = builder.BuildAndStart();
            std::cout << "Server listening on " << server_address << std::endl;

            active = true;
            auto t = thread([this]()
            {
                while (active)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    //                std::this_thread::sleep_for(std::chrono::microseconds(10));

                    // write fake event
                    std::lock_guard<std::mutex> l(mutClients);
                    msg.set_msg("ping " + std::to_string(num++));

                    for (const auto& c : clients)
                        c.second->pushData(msg);
                }
            });

            mainLoop();
            active = false;
            t.join();
        }

        class ClientSession;

        struct Client
        {
            Client(ClientSession* s) :
                session(s) {}

            ~Client()
            {
                delete session;
            }

            void readEvent(const uniset3::TestParams& request)
            {
                cerr << "[" << session << "](read): " << request.msg() << endl;
            }

            void pushData(const uniset3::RetMessage& r)
            {
                session->push_data(r);
            }

            ClientSession* session;
        };

        std::shared_ptr<Client> newClient(ClientSession* sess)
        {
            std::lock_guard<std::mutex> l(mutClients);
            cerr << "ADD NEW CLIENT: " << sess << endl;
            auto cli = make_shared<Client>(sess);
            clients[sess] = cli;
            return cli;
        }

        void releaseClient(ClientSession* sess)
        {
            std::lock_guard<std::mutex> l(mutClients);
            auto i = clients.find(sess);

            if (i != clients.end())
            {
                cerr << "REMOVE CLIENT: " << i->first << endl;
                clients.erase(i);
            }
        }

    private:

        std::unique_ptr<ServerCompletionQueue> cq;
        uniset3::TestAsyncService_i::AsyncService service_;
        std::unique_ptr<Server> server_;

        std::unordered_map<ClientSession*, std::shared_ptr<Client>> clients;
        std::mutex mutClients;

        uniset3::RetMessage msg;
        size_t num = {0};

        std::atomic_bool active;

    public:

        struct TagData
        {
            enum class Event
            {
                start,
                read,
                write,
                data_timer,
                close
            };

            ClientSession* handler;
            Event evt;
        };

        struct TagReadSet
        {
            TagReadSet(ClientSession* self)
                : on_start{self, TagData::Event::start},
                  on_read{self, TagData::Event::read},
                  on_write{self, TagData::Event::write},
                  on_data_timer{self, TagData::Event::data_timer},
                  on_close{self, TagData::Event::close} {}

            TagData on_start;
            TagData on_read;
            TagData on_write;
            TagData on_data_timer;
            TagData on_close;
        };

        class ClientSession
        {
            public:
                ClientSession(ServerImpl* i, uniset3::TestAsyncService_i::AsyncService* service, ServerCompletionQueue* cq)
                    : impl(i), tags(this), service_(service), cq_(cq), responder_(&ctx_)
                {

                    service_->RequeststreamTest(&ctx_, &responder_, cq_, cq_, &tags.on_start);
                    wait_data_timer = make_unique<grpc::Alarm>();
                }

                bool is_closed() const
                {
                    return closed;
                }

                void shutdown()
                {
                    cerr << "ON SHUTDOWN..." << endl;
                    impl->releaseClient(this);
                    closed = true;
                }

                void on_create(bool ok)
                {
                    cerr << "ON CREATE..." << endl;
                    cli = impl->newClient(this);
                    new ClientSession(impl, service_, cq_);
                    responder_.Read(&request_, &tags.on_read);
                    reset_timer();
                }

                void on_close()
                {
                    impl->releaseClient(this);
                    wait_data_timer->Cancel();
                    closed = true;
                }

                void on_read(bool ok)
                {

                    if (!ok)
                    {
                        cerr << "ON CLOSE READ..." << endl;
                        responder_.Finish(grpc::Status::OK, &tags.on_close);
                        wait_data_timer->Cancel();
                        closed = true;
                        return;
                    }

                    cerr << "ON READ..." << endl;
                    responder_.Read(&request_, &tags.on_read);
                    cli->readEvent(request_);
                }

                void on_data_timer(bool ok)
                {
                    if (closed)
                        return;

                    if (!ok) // timer was break (new data pushed)
                        on_write(true);
                    else
                    {
                        cerr << "ON WAIT WRITE DATA TIMEOUT..." << endl;
                        reset_timer();
                    }
                }

                void reset_timer()
                {
                    wait_data_timer->Set(cq_, deadline_seconds(10), &tags.on_data_timer);
                }

                void on_write(bool ok)
                {

                    if (closed)
                        return;

                    if (!ok)
                    {
                        cerr << "ON CLOSE WRITE..." << endl;
                        responder_.Finish(grpc::Status::OK, &tags.on_close);
                        return;
                    }

                    cerr << "ON WRITE..." << endl;
                    std::lock_guard<std::mutex> lk(wr_queue_mutex_);

                    if (wr_queue_.empty())
                    {
                        reset_timer();
                        return;
                    }

                    auto reply = wr_queue_.front();
                    cerr << "write: " << reply.msg() << endl;
                    responder_.Write(reply, wr_options_, &tags.on_write);
                    wr_queue_.pop();
                }

                void push_data(const uniset3::RetMessage& r)
                {
                    std::lock_guard<std::mutex> lk(wr_queue_mutex_);
                    wr_queue_.push(r);

                    if (wait_data_timer)
                        wait_data_timer->Cancel();
                }

            private:
                bool closed = {false};
                TagReadSet tags;
                uniset3::TestAsyncService_i::AsyncService* service_;
                std::unique_ptr<grpc::Alarm> wait_data_timer;

                ServerImpl* impl;
                std::shared_ptr<Client> cli;
                ServerCompletionQueue* cq_;
                ServerContext ctx_;
                RWResponder responder_;
                grpc::WriteOptions wr_options_;

                uniset3::TestParams request_;
                std::queue<uniset3::RetMessage> wr_queue_;
                std::mutex wr_queue_mutex_;
        };

        static std::chrono::system_clock::time_point deadline_seconds(int s)
        {
            return std::chrono::system_clock::now() + std::chrono::seconds(s);
        }

        void mainLoop()
        {
            auto cd = new ClientSession(this, &service_, cq.get());
            void* raw_tag;
            bool ok;

            while (true)
            {

                raw_tag = nullptr;
                raw_tag = nullptr;
                bool ret = cq->Next(&raw_tag, &ok);
                TagData* tag = reinterpret_cast<TagData*>(raw_tag);

                if (!ret)
                {
                    if( tag )
                        tag->handler->shutdown();

                    break;
                }

                if (tag->evt == TagData::Event::read)
                    tag->handler->on_read(ok);
                else if (tag->evt == TagData::Event::data_timer)
                    tag->handler->on_data_timer(ok);
                else if (tag->evt == TagData::Event::write)
                    tag->handler->on_write(ok);
                else if (tag->evt == TagData::Event::close)
                    tag->handler->on_close();
                else if (tag->evt == TagData::Event::start)
                    tag->handler->on_create(ok);
            }
        }

};

int main(int argc, char** argv)
{
    ServerImpl server;
    server.Run();

    return 0;
}
