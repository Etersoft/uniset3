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
// --------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include <unistd.h>

#include "unisetstd.h"
#include <Poco/Net/NetException.h>
#include <Poco/URI.h>
#include <google/protobuf/util/json_util.h>
#include "ujson.h"
#include "HttpAPIGateway.h"
#include "Configuration.h"
#include "Exceptions.h"
#include "Debug.h"
#include "UniXML.h"
#include "HttpAPIGatewaySugar.h"
#include "MetricsExporter.grpc.pb.h"
#include "APIGateway.pb.h"
// --------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// --------------------------------------------------------------------------
HttpAPIGateway::HttpAPIGateway( const string& name, int argc, const char* const* argv, const string& prefix ):
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

    xmlNode* cnode = xml->findNode(xml->getFirstNode(), "HttpAPIGateway", name);

    if( !cnode )
    {
        ostringstream err;
        err << name << "(init): Not found confnode <HttpAPIGateway name='" << name << "'...>";
        cerr << err.str() << endl;
        throw uniset3::SystemError(err.str());
    }

    UniXML::iterator it(cnode);

    httpHost = uniset3::getArgParam("--" + prefix + "host", argc, argv, it.getProp2("host", "0.0.0.0"));
    httpPort = uniset3::getArgInt("--" + prefix + "port", argc, argv, it.getProp2("port", "8081"));
    httpCORS_allow = uniset3::getArgParam("--" + prefix + "cors-allow", argc, argv, it.getProp2("cors", httpCORS_allow));

    rinfo << myname << "(init): http server parameters " << httpHost << ":" << httpPort << endl;
    Poco::Net::SocketAddress sa(httpHost, httpPort);

    try
    {
        Poco::Net::HTTPServerParams* httpParams = new Poco::Net::HTTPServerParams;

        int maxQ = uniset3::getArgPInt("--" + prefix + "max-queued", argc, argv, it.getProp("maxQueued"), 100);
        int maxT = uniset3::getArgPInt("--" + prefix + "max-threads", argc, argv, it.getProp("maxThreads"), 3);

        httpParams->setMaxQueued(maxQ);
        httpParams->setMaxThreads(maxT);
        httpserv = std::make_shared<Poco::Net::HTTPServer>(new HttpAPIGatewayRequestHandlerFactory(this), Poco::Net::ServerSocket(sa), httpParams );
    }
    catch( std::exception& ex )
    {
        std::stringstream err;
        err << myname << "(init): " << httpHost << ":" << httpPort << " ERROR: " << ex.what();
        throw uniset3::SystemError(err.str());
    }

    initRouter();
}
//--------------------------------------------------------------------------------------------
HttpAPIGateway::~HttpAPIGateway()
{
    if( httpserv )
        httpserv->stop();
}
//--------------------------------------------------------------------------------------------
std::shared_ptr<HttpAPIGateway> HttpAPIGateway::init_apigateway( int argc, const char* const* argv, const std::string& prefix )
{
    string name = uniset3::getArgParam("--" + prefix + "name", argc, argv, "HttpAPIGateway");

    if( name.empty() )
    {
        cerr << "(HttpAPIGateway): Unknown name. Use --" << prefix << "name" << endl;
        return nullptr;
    }

    return make_shared<HttpAPIGateway>(name, argc, argv, prefix);
}
// -----------------------------------------------------------------------------
void HttpAPIGateway::help_print()
{
    cout << "Default: prefix='api'" << endl;
    cout << "--prefix-host ip          - IP на котором слушает http сервер. По умолчанию: 0.0.0.0" << endl;
    cout << "--prefix-port num         - Порт на котором принимать запросы. По умолчанию: 8008" << endl;
    cout << "--prefix-max-queued num   - Размер очереди запросов к http серверу. По умолчанию: 100" << endl;
    cout << "--prefix-max-threads num  - Разрешённое количество потоков для http-сервера. По умолчанию: 3" << endl;
    cout << "--prefix-cors-allow addr  - (CORS): Access-Control-Allow-Origin. Default: *" << endl;
}
// -----------------------------------------------------------------------------
void HttpAPIGateway::run()
{
    if( httpserv )
        httpserv->start();

    pause();
}
// -----------------------------------------------------------------------------
class HttpAPIGatewayRequestHandler:
    public Poco::Net::HTTPRequestHandler
{
    public:

        HttpAPIGatewayRequestHandler( HttpAPIGateway* r ): api(r) {}

        virtual void handleRequest( Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response ) override
        {
            api->handleRequest(request, response);
        }

    private:
        HttpAPIGateway* api;
};
// -----------------------------------------------------------------------------
Poco::Net::HTTPRequestHandler* HttpAPIGateway::HttpAPIGatewayRequestHandlerFactory::createRequestHandler( const Poco::Net::HTTPServerRequest& req )
{
    return new HttpAPIGatewayRequestHandler(api);
}
// -----------------------------------------------------------------------------
void HttpAPIGateway::handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp )
{
    using Poco::Net::HTTPResponse;

    resp.setContentType("text/json");
    resp.set("Access-Control-Allow-Methods", req.getMethod());
    resp.set("Access-Control-Allow-Request-Method", "*");
    resp.set("Access-Control-Allow-Origin", httpCORS_allow /* req.get("Origin") */);

    try
    {
        if( !router.call(req, resp) )
        {
            std::ostream& out = resp.send();
            auto jdata = respError(resp, HTTPResponse::HTTP_BAD_GATEWAY, "Unknown route: " + req.getURI());
            jdata->stringify(out);
            out.flush();
        }
    }
    catch( const Poco::Exception& ex )
    {
        std::ostream& out = resp.send();
        rwarn << myname << "(handleRequest): " << ex.displayText() << endl;
        auto jdata = respError(resp, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, ex.what());
        jdata->stringify(out);
        out.flush();
    }
    catch( std::exception& ex )
    {
        std::ostream& out = resp.send();
        auto jdata = respError(resp, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, ex.what());
        jdata->stringify(out);
        out.flush();
    }

}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr HttpAPIGateway::respError( Poco::Net::HTTPServerResponse& resp,
        Poco::Net::HTTPResponse::HTTPStatus estatus,
        const string& message )
{
    resp.setStatus(estatus);
    resp.setContentType("text/json");
    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    jdata->set("error", resp.getReasonForStatus(resp.getStatus()));
    jdata->set("ecode", (int)resp.getStatus());
    jdata->set("message", message);
    return jdata;
}
// -----------------------------------------------------------------------------
void HttpAPIGateway::initRouter()
{
    using namespace std::placeholders;
    router.setPrefix("/api/" + HTTP_API_GATEWAY_VERSION);
    router.get().add("/metrics/:oname", std::bind(&HttpAPIGateway::requestMetrics, this, _1, _2, _3) );
    router.get().add("/resolve/:oname", std::bind(&HttpAPIGateway::requestResolve, this, _1, _2, _3) );
    router.get().add("/configure/get", std::bind(&HttpAPIGateway::requestConfigureGet, this, _1, _2, _3) );
}
// -----------------------------------------------------------------------------
void HttpAPIGateway::requestMetrics(Poco::Net::HTTPServerRequest& req,
                                    Poco::Net::HTTPServerResponse& resp,
                                    const UHttpContext& ctx)
{
    rlog1 << myname << "(requestInfo): " << req.getURI() << endl;

    using Poco::Net::HTTPResponse;
    std::ostream& out = resp.send();

    const auto name = ctx.key("oname");

    if( name.empty() )
    {
        rlog2 << myname << "(requestInfo): Unknown object name" << endl;
        auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, "Unknown name object: " + req.getURI());
        jdata->stringify(out);
        return;
    }

    auto conf = uniset_conf();
    ObjectId id = DefaultObjectId;

    if( uniset3::is_digit(name) )
        id = uni_atoi(name);
    else
        id = uniset_conf()->getAnyID(name);

    if( id == DefaultObjectId )
    {
        rlog2 << myname << "(requestInfo): not found id for " << name << endl;
        auto jdata = respError(resp, HTTPResponse::HTTP_NOT_FOUND, "Unknown ID for object: " + name);
        jdata->stringify(out);
        return;
    }

    uniset3::metrics::Metrics ret;
    try
    {
        ret = ui.metrics(id);
    }
    catch( std::exception& ex )
    {
        rlog2 << myname << "(requestInfo): call " << name << " error: " << ex.what() << endl;
        auto jdata = respError(resp, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "call to " + name + " error");
        jdata->stringify(out);
        return;
    }

    std::string json_string;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    auto st  = google::protobuf::util::MessageToJsonString(ret, &json_string, options);

    if( !st.ok() )
    {
        auto jdata = respError(resp, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "convert to json error: " + st.ToString());
        jdata->stringify(out);
        return;
    }

//    rlog2 << myname << "(requestInfo): " << json_string << endl;

    out << json_string;
    out.flush();
}
// -----------------------------------------------------------------------------
void HttpAPIGateway::requestResolve( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp, const uniset3::UHttpContext& ctx )
{
    rlog1 << myname << "(requestResolve): " << req.getURI() << endl;

    using Poco::Net::HTTPResponse;
    std::ostream& out = resp.send();

    const auto name = ctx.key("oname");

    if( name.empty() )
    {
        rlog2 << myname << "(requestResolve): Unknown object name" << endl;
        auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, "Unknown name object: " + req.getURI());
        jdata->stringify(out);
        return;
    }

    auto conf = uniset_conf();
    ObjectId id = DefaultObjectId;

    if( uniset3::is_digit(name) )
        id = uni_atoi(name);
    else
        id = uniset_conf()->getAnyID(name);

    if( id == DefaultObjectId )
    {
        rlog2 << myname << "(requestResolve): not found id for " << name << endl;
        auto jdata = respError(resp, HTTPResponse::HTTP_NOT_FOUND, "Unknown ID for object: " + name);
        jdata->stringify(out);
        return;
    }

    uniset3::ObjectRef oref;
    try
    {
        oref = ui.resolveORefOnly(id, uniset_conf()->getLocalNode());
    }
    catch( std::exception& ex )
    {
        rlog2 << myname << "(requestResolve): call " << name << " error: " << ex.what() << endl;
        auto jdata = respError(resp, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "call to " + name + " error");
        jdata->stringify(out);
        return;
    }

    // convert LITE format -> FULL format (with support json)
    uniset3::apigateway::ObjectRef ret;
    ret.set_id(oref.id());
    ret.set_addr(oref.addr());
    ret.set_path(oref.path());
    ret.set_type(oref.type());
    *ret.mutable_metadata() = oref.metadata();

    std::string json_string;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    auto st  = google::protobuf::util::MessageToJsonString(ret, &json_string, options);

    if( !st.ok() )
    {
        auto jdata = respError(resp, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "convert to json error: " + st.ToString());
        jdata->stringify(out);
        return;
    }

    //    rlog2 << myname << "(requestInfo): " << json_string << endl;

    out << json_string;
    out.flush();
}
// -----------------------------------------------------------------------------
// обработка запроса вида: /configure/get?[ID|NAME]&props=testname,name] from configure.xml
void HttpAPIGateway::requestConfigureGet(Poco::Net::HTTPServerRequest& req,
                                    Poco::Net::HTTPServerResponse& resp,
                                    const UHttpContext& ctx)
{
    using Poco::Net::HTTPResponse;

    rlog1 << myname << "(requestConfigureGet): " << req.getURI() << endl;
    std::ostream& out = resp.send();

    Poco::URI uri(req.getURI());
    auto params = uri.getQueryParameters();
    if( params.empty() )
    {
        auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, "Unknown id or name");
        jdata->stringify(out);
        return;
    }

    auto idlist = uniset3::split(params[0].first, ',');

    if( idlist.empty() )
    {
        auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, "Unknown id or name in '" + params[0].first + "'");
        jdata->stringify(out);
        return;
    }

    Poco::JSON::Object::Ptr json = new Poco::JSON::Object();
    Poco::JSON::Array::Ptr jdata = uniset3::json::make_child_array(json, "conf");

    string props = {""};

    for( const auto& p : params )
    {
        if( p.first == "props" )
        {
            props = p.second;
            break;
        }
    }

    for( const auto& id : idlist )
    {
        Poco::JSON::Object::Ptr j = configure_params_by_name(id, props);
        if( j )
            jdata->add(j);
    }

    json->stringify(out);
    out.flush();
}
// -----------------------------------------------------------------------------
Poco::JSON::Object::Ptr HttpAPIGateway::configure_params_by_name( const std::string& name, const std::string& props )
{
    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    auto conf = uniset_conf();

    ObjectId id = conf->getAnyID(name);

    if( id == DefaultObjectId )
    {
        ostringstream err;
        err << name << " not found..";
        jdata->set(name, "");
        jdata->set("error", err.str());
        return jdata;
    }

    xmlNode* xmlnode = conf->getXMLObjectNode(id);

    if( !xmlnode )
    {
        ostringstream err;
        err << name << " not found confnode..";
        jdata->set(name, "");
        jdata->set("error", err.str());
        return jdata;
    }

    UniXML::iterator it(xmlnode);

    jdata->set("name", it.getProp("name"));
    jdata->set("id", it.getProp("id"));

    if( !props.empty() )
    {
        auto lst = uniset3::split(props, ',');

        for( const auto& p : lst )
            jdata->set(p, it.getProp(p));
    }
    else
    {
        auto lst = it.getPropList();

        for( const auto& p : lst )
            jdata->set(p.first, p.second);
    }

    return jdata;
}