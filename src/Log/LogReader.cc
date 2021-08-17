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
// -------------------------------------------------------------------------
#include <string.h>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <regex>
#include "PassiveTimer.h"
#include "LogReader.h"
#include "UniSetTypes.h"
// -------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// -------------------------------------------------------------------------
LogReader::LogReader():
    readTimeout(10000),
    reconDelay(5000),
    iaddr(""),
    readcount(0),
    active(false)
{
    outlog = std::make_shared<DebugStream>();

    outlog->level(Debug::ANY);
    outlog->signal_stream_event().connect( sigc::mem_fun(this, &LogReader::logOnEvent) );
}

// -------------------------------------------------------------------------
LogReader::~LogReader()
{
    terminate();
    ctx = nullptr;
    chan = nullptr;
}
// -------------------------------------------------------------------------
void LogReader::setLogLevel( Debug::type t )
{
    outlog->level(t);
}
// -------------------------------------------------------------------------
std::shared_ptr<DebugStream> LogReader::log()
{
    return outlog;
}
// -------------------------------------------------------------------------
DebugStream::StreamEvent_Signal LogReader::signal_stream_event()
{
    return m_logsig;
}
// -------------------------------------------------------------------------
bool LogReader::isConnection() const
{
    return chan != nullptr;
}
// -------------------------------------------------------------------------
void LogReader::setReadCount( size_t n )
{
    readcount = n;
}
// -------------------------------------------------------------------------
void LogReader::setTimeout(timeout_t msec)
{
    if( msec == UniSetTimer::WaitUpTime )
        readTimeout = 24 * 60 * 60 * 1000; // 24 hours
    else
        readTimeout = msec;
}
// -------------------------------------------------------------------------
void LogReader::setReconnectDelay(timeout_t msec)
{
    reconDelay = msec;
}
// -------------------------------------------------------------------------
void LogReader::setTextFilter(const string& f)
{
    textfilter  = f;
}
// -------------------------------------------------------------------------
bool LogReader::list( const std::string& _addr, int _port, const std::string& logname, bool verbose )
{
    if( verbose )
        rlog.addLevel(Debug::ANY);

    ostringstream saddr;
    saddr << _addr << ":" << _port;
    chan = grpc::CreateChannel(saddr.str(), grpc::InsecureChannelCredentials());

    if( !chan )
    {
        rlog.crit() << "(list): failed to connect " << saddr.str() << endl;
        return false;
    }

    std::unique_ptr<logserver::LogServer_i::Stub> stub(logserver::LogServer_i::NewStub(chan));

    ctx = make_shared<grpc::ClientContext>();
    ctx->set_deadline(uniset3::deadline_msec(readTimeout));
//    ctx.set_wait_for_ready(true);
    logserver::LogListParams req;
    req.set_logname(logname);
    logserver::LogMessage msg;
    auto status = stub->list(ctx.get(), req, &msg);

    if( !status.ok() )
    {
        rlog.crit() << "(LogReader::list): error: " << status.error_message() << endl;
        return false;
    }

    outlog->any(false) << msg.text() << endl;
    return true;
}
// -------------------------------------------------------------------------
bool LogReader::loglevel( const std::string& _addr, int _port, const std::string& logname, bool verbose )
{
    if( verbose )
        rlog.addLevel(Debug::ANY);

    ostringstream saddr;
    saddr << _addr << ":" << _port;
    chan = grpc::CreateChannel(saddr.str(), grpc::InsecureChannelCredentials());

    if( !chan )
    {
        rlog.crit() << "(loglevel): failed to connect " << saddr.str() << endl;
        return false;
    }

    std::unique_ptr<logserver::LogServer_i::Stub> stub(logserver::LogServer_i::NewStub(chan));

    rlog.info() << saddr.str() << ": call 'loglevel'" << endl;

    ctx = make_shared<grpc::ClientContext>();
    ctx->set_deadline(uniset3::deadline_msec(readTimeout));
//    ctx.set_wait_for_ready(true);
    logserver::LogLevelParams req;
    req.set_logname(logname);
    logserver::LogMessage msg;
    auto status = stub->loglevel(ctx.get(), req, &msg);

    if( !status.ok() )
    {
        rlog.crit() << "(LogReader::loglevel): error: " << status.error_message() << endl;
        return false;
    }

    outlog->any(false) << msg.text() << endl;
    return true;
}
// -------------------------------------------------------------------------
bool LogReader::command(const std::string& _addr, int _port, const logserver::LogCommandList& lst, bool verbose)
{
    if( verbose )
        rlog.addLevel(Debug::ANY);

    ostringstream saddr;
    saddr << _addr << ":" << _port;
    chan = grpc::CreateChannel(saddr.str(), grpc::InsecureChannelCredentials());

    if( !chan )
    {
        rlog.crit() << "(command): failed to connect " << saddr.str() << endl;
        return false;
    }

    std::unique_ptr<logserver::LogServer_i::Stub> stub(logserver::LogServer_i::NewStub(chan));

    rlog.info() << saddr.str() << ": call 'command'" << endl;

    ctx = make_shared<grpc::ClientContext>();
    ctx->set_deadline(uniset3::deadline_msec(readTimeout));
    google::protobuf::Empty response;
    auto status = stub->command(ctx.get(), lst, &response);

    if( !status.ok() )
    {
        rlog.crit() << "(LogReader::command): error: " << status.error_message() << endl;
        return false;
    }

    return true;
}
// -------------------------------------------------------------------------
void LogReader::readLoop(const std::string& addr, int port, const logserver::LogCommandList& lst, bool verbose)
{
    size_t rcount = 1;

    if( readcount > 0 )
        rcount = readcount;

    active = true;
    while(active)
    {
        if( lst.cmd_size() > 0 )
        {
            if( !command(addr, port, lst, verbose) )
            {
                if( !active )
                    return;

                msleep(reconDelay);
                continue;
            }
        }

        read(addr, port, verbose);

        if( !active )
            return;

        if( readcount > 0 )
        {
            rcount--;

            if( rcount <= 0 )
                return;
        }

        msleep(reconDelay);
    }
}
// -------------------------------------------------------------------------
void LogReader::terminate()
{
    if( !active )
        return;

    active = false;
    if( ctx )
        ctx->TryCancel();
}
// -------------------------------------------------------------------------
bool LogReader::isActive() const
{
    return active;
}
// -------------------------------------------------------------------------
bool LogReader::read( const std::string& _addr, int _port,  bool verbose )
{
    if( verbose )
        rlog.addLevel(Debug::ANY);

    ostringstream saddr;
    saddr << _addr << ":" << _port;
    chan = grpc::CreateChannel(saddr.str(), grpc::InsecureChannelCredentials());

    if( !chan )
    {
        rlog.crit() << "(read): failed to connect " << saddr.str() << endl;
        return false;
    }

    std::unique_ptr<logserver::LogServer_i::Stub> stub(logserver::LogServer_i::NewStub(chan));

    rlog.info() << saddr.str() << ": read logs" << endl;

    ctx = make_shared<grpc::ClientContext>();
    ctx->set_deadline(uniset3::deadline_msec(readTimeout));
//    ctx->set_wait_for_ready(true);
    google::protobuf::Empty req;
    auto reader = stub->read(ctx.get(), req);

    if( !reader )
    {
        rlog.warn() << "(LogReader::read): can't connect to " << saddr.str() << endl;
        return false;
    }

    active = true;
    logserver::LogMessage msg;
    std::regex rule(textfilter);

    size_t rcount = 1;

    if( readcount > 0 )
        rcount = readcount;

    // Если мы работаем с текстовым фильтром
    // то надо читать построчно..
    ostringstream line;

    while( active && reader->Read(&msg) )
    {
        if( textfilter.empty() )
            outlog->any(false) << msg.text();
        else
        {
            //
            auto pos = msg.text().find_first_of('\n');
            if( pos == string::npos )
                line << msg.text();
            else
            {
                line << msg.text().substr(0, pos+1);
                const std::string s(line.str());
                if( std::regex_search(s, rule) )
                    outlog->any(false) << s;

                line.str("");
                if( pos+2 < msg.text().size() )
                    line << msg.text().substr(pos+2);
            }
        }

        if( readcount > 0 )
        {
            rcount--;

            if( rcount <= 0 )
                return true;
        }
    }

    auto st = reader->Finish();
    return true;
}
// -------------------------------------------------------------------------
void LogReader::logOnEvent( const std::string& s )
{
    m_logsig.emit(s);
}
// -------------------------------------------------------------------------
static const std::string checkArg( size_t i, const std::vector<std::string>& v )
{
    if( i < v.size() && (v[i])[0] != '-' )
        return v[i];

    return "";
}
// -------------------------------------------------------------------------
logserver::LogCommandList LogReader::getCommands( const std::string& cmd )
{
    logserver::LogCommandList cmdlist;

    auto v = uniset3::explode_str(cmd, ' ');

    if( v.empty() )
        return cmdlist;

    for( size_t i = 0; i < v.size(); i++ )
    {
        auto c = v[i];

        const string arg1 = checkArg(i + 1, v);

        if( arg1.empty() )
            continue;

        i++;

        const std::string filter = checkArg(i + 2, v);

        if( !filter.empty() )
            i++;

        if( c == "-s" || c == "--set" )
        {
            logserver::LogCommand cmd;
            cmd.set_cmd(logserver::LOG_CMD_SET);
            cmd.set_logname(filter);
            cmd.set_data((uint64_t)Debug::value(arg1));
            cmdlist.add_cmd()->PackFrom(cmd);
        }
        else if( c == "-a" || c == "--add" )
        {
            logserver::LogCommand cmd;
            cmd.set_cmd(logserver::LOG_CMD_ADD);
            cmd.set_logname(filter);
            cmd.set_data((uint64_t)Debug::value(arg1));
            cmdlist.add_cmd()->PackFrom(cmd);

        }
        else if( c == "-d" || c == "--del" )
        {
            logserver::LogCommand cmd;
            cmd.set_cmd(logserver::LOG_CMD_DEL);
            cmd.set_logname(filter);
            cmd.set_data((uint64_t)Debug::value(arg1));
            cmdlist.add_cmd()->PackFrom(cmd);
        }
    }

    return cmdlist;
}
// -------------------------------------------------------------------------
