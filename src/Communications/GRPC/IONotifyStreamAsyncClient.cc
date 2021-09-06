/*
 * Copyright (c) 2021 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// ------------------------------------------------------------------------------------------
#include <sstream>
#include <memory>
#include <queue>
#include <grpcpp/grpcpp.h>
#include <grpcpp/alarm.h>
#include "UHelpers.h"
#include "Configuration.h"
#include "IONotifyStreamAsyncClient.h"
// ------------------------------------------------------------------------------------------
using namespace uniset3;
using namespace std;

// ------------------------------------------------------------------------------------------
IONotifyStreamAsyncClient::IONotifyStreamAsyncClient()
{
}

IONotifyStreamAsyncClient::~IONotifyStreamAsyncClient()
{
    if( active )
        terminate();
}

// ------------------------------------------------------------------------------------------
void IONotifyStreamAsyncClient::async_run( const std::string& host, int port, const std::shared_ptr<UniSetObject>& _consumer )
{
    consumer = _consumer;

    async_run_cb(host, port, [this](const uniset3::umessage::TransportMessage * tmsg)
    {
        grpc::ServerContext ctx;
        google::protobuf::Empty empty;
        consumer->push(&ctx, tmsg, &empty);
    });
}
// ------------------------------------------------------------------------------------------
void IONotifyStreamAsyncClient::async_run_cb( const std::string& host, int port, CallBackFunction cb )
{
    if( active )
    {
        ostringstream err;
        err << "(IONotifyStreamAsyncClient:async_run): async client is already running";
        throw SystemError(err.str());
    }

    active = true;

    thr = std::make_unique<std::thread>([ = ]()
    {
        while( active )
        {
            try
            {
                mainLoop(host, 4444, cb);
            }
            catch( std::exception& ex )
            {
                ucrit << "(IONotifyStreamAsyncClient): server=" << host << ":" << port << " error: " << ex.what() << endl;
            }

            if( !active )
                break;

            ulog4 << "(IONotifyStreamAsyncClient): try reconnect to server=" << host << ":" << port
                  << " after " << reconnectPause_msec << " msec " << endl;
            msleep(reconnectPause_msec);
        }
    });
}

// ------------------------------------------------------------------------------------------
typedef ::grpc::ClientAsyncReaderWriter<::uniset3::SensorsStreamCmd, ::uniset3::umessage::SensorMessage> RWResponder;

// -----------------------------------------------------------------------------
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

    IONotifyStreamAsyncClient::AsyncClientSession* handler;
    Event evt;
};

// -----------------------------------------------------------------------------
struct TagSet
{
    TagSet(IONotifyStreamAsyncClient::AsyncClientSession* self)
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

// -----------------------------------------------------------------------------
class IONotifyStreamAsyncClient::AsyncClientSession
{
    public:
        AsyncClientSession( const CallBackFunction& _cb,
                            std::unique_ptr<IONotifyStreamController_i::Stub>& _stub,
                            grpc::CompletionQueue* _cq)
            : tags(this), cb(_cb), stub(std::move(_stub)), cq(_cq)
        {
            responder = stub->AsyncsensorsStream(&ctx, cq, &tags.on_start);
            wait_data_timer = make_unique<grpc::Alarm>();
        }

        ~AsyncClientSession()
        {
            ulog4 << "IONotifyStreamAsyncClient[" << this << "] destroy.." << endl;

            if( !deleted )
                shutdown();
        }

        static std::chrono::system_clock::time_point deadline_seconds(int s)
        {
            return std::chrono::system_clock::now() + std::chrono::seconds(s);
        }

        static std::chrono::system_clock::time_point deadline_msec(int ms)
        {
            return std::chrono::system_clock::now() + std::chrono::milliseconds(ms);
        }

        bool isClosed() const
        {
            return closed;
        }

        void shutdown()
        {
            ulog4 << "IONotifyStreamAsyncClient[" << this << "] ON SHUTDOWN..." << endl;
            closed = true;
            wait_data_timer->Cancel();
        }

        void on_close()
        {
            if( deleted )
                return;

            deleted = true;
            closed = true;
            wait_data_timer->Cancel();
        }

        void on_create(bool ok)
        {
            if (closed)
                return;

            if (!ok)
            {
                on_close();
                return;
            }

            ulog4 << "IONotifyStreamAsyncClient[" << this << "] ON CREATE..." << endl;
            reset_timer();

            {
                std::lock_guard<std::mutex> lk(cmd_queue_mutex);

                if( !cmd_queue.empty() )
                    wait_data_timer->Cancel();
            }

            responder->Read(&reply, &tags.on_read);
        }

        void on_read(bool ok)
        {
            if (!ok)
            {
                if( closed )
                    return;

                ulog4 << "IONotifyStreamAsyncClient[" << this << "] ON CLOSE READ..." << endl;
                closed = true;
                wait_data_timer->Cancel();
                responder->Finish(&status, &tags.on_close);
                return;
            }

            ulog4 << "IONotifyStreamAsyncClient[" << this << "] ON READ..." << endl;
            responder->Read(&reply, &tags.on_read);
            tmsg = uniset3::to_transport<umessage::SensorMessage>(reply);
            cb(&tmsg);
        }

        void on_data_timer(bool ok)
        {
            if (closed)
                return;

            if (!ok) // timer was break (new data pushed)
                on_write(true);
            else
            {
                ulog4 << "IONotifyStreamAsyncClient[" << this << "] ON WAIT WRITE DATA TIMEOUT..." << endl;
                reset_timer();
            }
        }

        void reset_timer()
        {
            wait_data_timer->Set(cq, deadline_seconds(wait_data_time_in_seconds), &tags.on_data_timer);
        }

        void on_write(bool ok)
        {
            if (closed)
                return;

            if (!ok)
            {
                ulog4 << "IONotifyStreamAsyncClient[" << this << "] ON CLOSE WRITE..." << endl;
                closed = true;
                wait_data_timer->Cancel();
                responder->Finish(&status, &tags.on_close);
                return;
            }

            ulog4 << "IONotifyStreamAsyncClient[" << this << "] ON WRITE..." << endl;
            std::lock_guard<std::mutex> lk(cmd_queue_mutex);

            if (cmd_queue.empty())
            {
                ulog4 << "IONotifyStreamAsyncClient[" << this << "] write: no new data.." << endl;
                reset_timer();
                return;
            }

            auto cmd = cmd_queue.front();
            ulog4 << "IONotifyStreamAsyncClient[" << this << "] write: cmd=" << cmd.cmd() << endl;
            responder->Write(cmd, wr_options, &tags.on_write);
            cmd_queue.pop();
        }

        void push_data(const uniset3::SensorsStreamCmd& cmd)
        {
            if (deleted || closed)
                return;

            std::lock_guard<std::mutex> lk(cmd_queue_mutex);
            cmd_queue.push(cmd);

            if (wait_data_timer)
                wait_data_timer->Cancel();
        }

    private:
        std::atomic_bool closed = {false};
        std::atomic_bool deleted = {false};
        TagSet tags;
        std::unique_ptr<grpc::Alarm> wait_data_timer;
        size_t wait_data_time_in_seconds = {15 * 60 * 60};

        uniset3::umessage::TransportMessage tmsg;
        CallBackFunction cb;
        std::unique_ptr<IONotifyStreamController_i::Stub> stub;
        grpc::CompletionQueue* cq;
        grpc::ClientContext ctx;
        std::unique_ptr<RWResponder> responder;
        grpc::WriteOptions wr_options;

        uniset3::umessage::SensorMessage reply;
        std::queue<uniset3::SensorsStreamCmd> cmd_queue;
        std::mutex cmd_queue_mutex;
        grpc::Status status;
};

// ------------------------------------------------------------------------------------------
void IONotifyStreamAsyncClient::mainLoop( const std::string& host, int port, const CallBackFunction& cb )
{
    using grpc::ClientAsyncReaderWriter;

    const std::string addr = host + ":" + std::to_string(port);
    auto chan = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());

    //    grpc::ChannelArguments args;
    //    args.SetLoadBalancingPolicyName("round_robin");
    //    args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);
    //    args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 2000);
    //    args.SetInt(GRPC_ARG_MIN_RECONNECT_BACKOFF_MS, 300);
    //    args.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 2000);
    //    args.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS, 500);
    //    args.SetInt(GRPC_ARG_GRPCLB_CALL_TIMEOUT_MS, 500);
    //    auto chan = grpc::CreateCustomChannel(addr, grpc::InsecureChannelCredentials(), args);

    ulog4 << "(IONotifyStreamAsyncClient): try connect to " << addr << endl;

    if( !chan->WaitForConnected(AsyncClientSession::deadline_msec(connectTimeout_msec)) )
    {
        ulog4 << "(IONotifyStreamAsyncClient): connect to server " << addr << " timeout..[" << connectTimeout_msec << " msec]" << endl;
        return;
    }

    ulog4 << "(IONotifyStreamAsyncClient): connect to " << addr << endl;
    std::unique_ptr<IONotifyStreamController_i::Stub> stub(IONotifyStreamController_i::NewStub(chan));
    session = std::make_unique<AsyncClientSession>(cb, stub, &cq);

    // send after create session
    if( !connected )
        sendConnectionEvent(true, cb);

    connected = true;
    void* raw_tag;
    bool ok;

    while( active )
    {
        raw_tag = nullptr;
        bool ret = cq.Next(&raw_tag, &ok);
        TagData* tag = reinterpret_cast<TagData*>(raw_tag);

        if (!ret)
        {
            if (tag)
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
        {
            tag->handler->on_close();
            break;
        }
        else if( tag->evt == TagData::Event::start )
            tag->handler->on_create(ok);
    }

    if( connected )
        sendConnectionEvent(false, cb);

    connected = false;
    ulog4 << "(IONotifyStreamAsyncClient): finished [server: " << addr << "]" << endl;
}
// ------------------------------------------------------------------------------------------
void IONotifyStreamAsyncClient::sendConnectionEvent(bool state, const CallBackFunction& cb )
{
    if( connectionEventID <= 0 || !active )
        return;

    ulog4 << "(IONotifyStreamAsyncClient): send connection event [eventID=" << connectionEventID
          << " state=" << state << "]"
          << endl;

    google::protobuf::Empty empty;
    grpc::ServerContext server_ctx;
    auto sm = makeSystemMessage(umessage::SystemMessage::NetworkInfo);
    sm.add_data(connectionEventID);
    sm.add_data(state ? 1 : 0 ); // 1 - connected, 0 - disconnected
    auto tmsg = uniset3::to_transport<umessage::SystemMessage>(sm);
    cb(&tmsg);
}
// ------------------------------------------------------------------------------------------
void IONotifyStreamAsyncClient::terminate()
{
    ulog4 << "(IONotifyStreamAsyncClient): terminate.. " << endl;
    active = false;

    if( session )
        session->shutdown();

    cq.Shutdown();

    if( thr )
        thr->join();

    thr = nullptr;
    consumer = nullptr;
}
// ------------------------------------------------------------------------------------------
void IONotifyStreamAsyncClient::enableConnectionEvent(int ID )
{
    connectionEventID = ID;
}
// ------------------------------------------------------------------------------------------
void IONotifyStreamAsyncClient::disableConectionEvent()
{
    connectionEventID = 0;
}
bool IONotifyStreamAsyncClient::isConnected() const
{
    return connected;
}
// ------------------------------------------------------------------------------------------
void IONotifyStreamAsyncClient::setValue( ObjectId sid, long value )
{
    uniset3::SensorsStreamCmd request;
    request.set_cmd(uniset3::UIOSet);
    auto s = request.add_slist();
    s->set_id(sid);
    s->set_val(value);

    if( session )
        session->push_data(request);
}

// ------------------------------------------------------------------------------------------
void IONotifyStreamAsyncClient::askSensor( ObjectId sid )
{
    uniset3::SensorsStreamCmd request;
    request.set_cmd(uniset3::UIONotify);
    auto s = request.add_slist();
    s->set_id(sid);

    if( session )
        session->push_data(request);
}
// ------------------------------------------------------------------------------------------
