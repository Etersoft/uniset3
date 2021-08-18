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
#include "ujson.h"
#include "HttpAPIGateway.h"
#include "Configuration.h"
#include "Exceptions.h"
#include "Debug.h"
#include "UniXML.h"
#include "HttpAPIGatewaySugar.h"
#include "UniSetObject.grpc.pb.h"
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

    std::ostream& out = resp.send();

    resp.setContentType("text/json");
    resp.set("Access-Control-Allow-Methods", "GET");
    resp.set("Access-Control-Allow-Request-Method", "*");
    resp.set("Access-Control-Allow-Origin", httpCORS_allow /* req.get("Origin") */);

    try
    {
        // В этой версии API поддерживается только GET
        if( req.getMethod() != "GET" )
        {
            auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, "method must be 'GET'");
            jdata->stringify(out);
            out.flush();
            return;
        }

        Poco::URI uri(req.getURI());

        rlog3 << req.getHost() << ": query: " << uri.getQuery() << endl;

        std::vector<std::string> seg;
        uri.getPathSegments(seg);

        // example: http://host:port/api/version/resolve/[json|text]
        if( seg.size() < 4
                || seg[0] != "api"
                || seg[1] != HTTP_API_GATEWAY_VERSION
                || seg[2].empty())
        {
            ostringstream err;
            err << "Bad request structure. Must be /api/" << HTTP_API_GATEWAY_VERSION << "/xxxx";
            auto jdata = respError(resp, HTTPResponse::HTTP_BAD_REQUEST, err.str());
            jdata->stringify(out);
            out.flush();
            return;
        }

        auto qp = uri.getQueryParameters();
        resp.setStatus(HTTPResponse::HTTP_OK);
        string cmd = seg[3];

        if( cmd == "help" )
        {
            using uniset3::json::help::item;
            uniset3::json::help::object myhelp("help");
            myhelp.emplace(item("help", "this help"));
            myhelp.emplace(item("resolve?name", "resolve name"));
            //          myhelp.emplace(item("apidocs", "https://github.com/Etersoft/uniset3"));

            item l("resolve?name", "resolve name");
            l.param("format=json|text", "response format");
            myhelp.add(l);
            myhelp.get()->stringify(out);
        }
        else if( cmd == "json" )
        {
            auto json = httpJsonResolve(uri.getQuery(), qp);
            json->stringify(out);
        }
        else if( cmd == "text" )
        {
            resp.setContentType("text/plain");
            auto txt = httpTextResolve(uri.getQuery(), qp);
            out << txt;
        }
    }
    catch( std::exception& ex )
    {
        auto jdata = respError(resp, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, ex.what());
        jdata->stringify(out);
    }

    out.flush();
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
Poco::JSON::Object::Ptr HttpAPIGateway::httpJsonResolve( const std::string& query, const Poco::URI::QueryParameters& p )
{
    auto iorstr = httpTextResolve(query, p);
    Poco::JSON::Object::Ptr jdata = new Poco::JSON::Object();
    jdata->set("ior", iorstr);
    return jdata;
}
// -----------------------------------------------------------------------------
std::string HttpAPIGateway::httpTextResolve(  const std::string& query, const Poco::URI::QueryParameters& p )
{
    if( query.empty() )
    {
        rwarn << myname << "undefined parameters for resolve" << endl;
        return "";
    }

    rwarn << myname << "unknown parameter type '" << query << "'" << endl;
    return "";
}
// -----------------------------------------------------------------------------
