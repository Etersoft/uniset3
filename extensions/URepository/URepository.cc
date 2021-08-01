/*
 * Copyright (c) 2020 Pavel Vainerman.
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
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "unisetstd.h"
#include <Poco/Net/NetException.h>
#include "ujson.h"
#include "IORFile.h"
#include "URepository.h"
#include "Configuration.h"
#include "Exceptions.h"
#include "Debug.h"
#include "UniXML.h"
#include "URepositorySugar.h"
// --------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// --------------------------------------------------------------------------
URepository::URepository( const string& name, int argc, const char* const* argv, const string& prefix ):
    myname(name)
{
    rlog = make_shared<DebugStream>();

    auto logLevels = uniset3::getArgParam("--" + prefix + "log-add-levels", argc, argv, "crit,warn");

    if( !logLevels.empty() )
        rlog->addLevel( Debug::value(logLevels) );

    std::string config = uniset3::getArgParam("--confile", argc, argv, "configure.xml");

    if( config.empty() )
        throw SystemError("Unknown config file");

    std::shared_ptr<UniXML> xml = make_shared<UniXML>();

    try
    {
        cout << myname << "(init): init from " << config << endl;
        xml->open(config);
    }
    catch( std::exception& ex )
    {
        throw ex;
    }

    xmlNode* cnode = xml->findNode(xml->getFirstNode(), "URepository", name);

    if( !cnode )
    {
        ostringstream err;
        err << name << "(init): Not found confnode <URepository name='" << name << "'...>";
        rcrit << err.str() << endl;
        throw uniset3::SystemError(err.str());
    }

    UniXML::iterator it(cnode);

    addr = it.getProp2("ip", "0.0.0.0");
    auto port = it.getPIntProp("port", 8111);
    addr = addr + ":" + to_string(port);
}
//--------------------------------------------------------------------------------------------
URepository::~URepository()
{
}
//--------------------------------------------------------------------------------------------
std::shared_ptr<URepository> URepository::init_repository( int argc, const char* const* argv, const std::string& prefix )
{
    string name = uniset3::getArgParam("--" + prefix + "name", argc, argv, "URepository");

    if( name.empty() )
    {
        cerr << "(URepository): Unknown name. Use --" << prefix << "name" << endl;
        return nullptr;
    }

    return make_shared<URepository>(name, argc, argv, prefix);
}
// -----------------------------------------------------------------------------
void URepository::help_print()
{
    cout << "Default: prefix='urepository'" << endl;
    cout << "--prefix-host ip               - IP на котором слушает сервер. По умолчанию: 0.0.0.0" << endl;
    cout << "--prefix-port num              - Порт на котором принимать запросы. По умолчанию: 8111" << endl;
    cout << "--prefix-log-add-levels level  - Уровень логов." << endl;
}
// ------------------------------------------------------------------------------------------
grpc::Status URepository::getInfo(::grpc::ServerContext* context, const ::google::protobuf::StringValue* request, ::google::protobuf::StringValue* response)
{
    ostringstream inf;
    {
        uniset3::uniset_rwmutex_rlock l(omutex);
        inf << "objects: " << omap.size() << endl;
    }

    response->set_value(inf.str());
    return grpc::Status::OK;
}
// ------------------------------------------------------------------------------------------
::grpc::Status URepository::exists(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::BoolValue* response)
{
    response->set_value(true);
    return grpc::Status::OK;
}
// ------------------------------------------------------------------------------------------
void URepository::run()
{
    grpc::EnableDefaultHealthCheckService(true);
//    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(static_cast<URepository_i::Service*>(this));
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    rinfo << myname << "(run): URepository listening on " << addr << std::endl;
    server->Wait();
}
// -----------------------------------------------------------------------------
std::string URepository::status()
{
    google::protobuf::StringValue request;
    google::protobuf::StringValue reply;
    grpc::ClientContext ctx;
    auto chan = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<URepository_i::Stub> stub(URepository_i::NewStub(chan));
    grpc::Status status = stub->getInfo(&ctx, request, &reply);
    if( !status.ok() )
    {
        ostringstream err;
        err << "error: (" << status.error_code() << ")" << status.error_message();
        return err.str();
    }

    return reply.value();
}
// -----------------------------------------------------------------------------
grpc::Status URepository::resolve(::grpc::ServerContext* context, const ::google::protobuf::Int64Value* request, ::uniset3::ObjectRef* response)
{
//    rinfo << "call resolve id=" << request->value() << endl;
    uniset3::uniset_rwmutex_rlock l(omutex);
    auto i = omap.find(request->value());
    if( i!=omap.end() )
    {
        *response = i->second;
        return grpc::Status::OK;
    }

    return grpc::Status(grpc::StatusCode::NOT_FOUND, "");
}
// -----------------------------------------------------------------------------
grpc::Status URepository::registration(::grpc::ServerContext* context, const ::uniset3::ObjectRef* request, ::google::protobuf::Empty* response)
{
    uniset3::uniset_rwmutex_wrlock l(omutex);
    omap[request->id()] = *(request);
    return grpc::Status::OK;
}
// -----------------------------------------------------------------------------
grpc::Status URepository::unregistration(::grpc::ServerContext* context, const ::google::protobuf::Int64Value* request, ::google::protobuf::Empty* response)
{
    uniset3::uniset_rwmutex_wrlock l(omutex);
    auto i = omap.find(request->value());
    if( i!= omap.end() )
        omap.erase(i);

    return grpc::Status::OK;
}
// -----------------------------------------------------------------------------
grpc::Status URepository::list(::grpc::ServerContext* context, const ::google::protobuf::StringValue* request, ::uniset3::ObjectRefList* response)
{
    uniset3::uniset_rwmutex_rlock l(omutex);
    for( const auto& o: omap )
    {
        if( o.second.path() != request->value() )
            continue;

        auto r = response->add_refs();
        *r = o.second;
    }

    return grpc::Status::OK;
}
// -----------------------------------------------------------------------------
