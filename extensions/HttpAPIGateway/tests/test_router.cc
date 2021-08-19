#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <iostream>
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "UHttpRouter.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// -----------------------------------------------------------------------------
class TestParams:
    public Poco::Net::HTTPServerParams
{
    public:
        TestParams() {}
        virtual ~TestParams() {}
};
// -----------------------------------------------------------------------------
class TestResponse:
    public Poco::Net::HTTPServerResponse
{
    public:
        virtual void sendContinue() {};
        virtual std::ostream& send()
        {
            return cout;
        }
        virtual void sendFile(const std::string& path, const std::string& mediaType) {}
        virtual void sendBuffer(const void* pBuffer, std::size_t length) {}
        virtual void redirect(const std::string& uri, HTTPStatus status = HTTP_FOUND) {}
        virtual void requireAuthentication(const std::string& realm) {}
        virtual bool sent() const
        {
            return true;
        }
};

class TestRequest:
    public Poco::Net::HTTPServerRequest
{
    public:

        TestRequest( TestResponse& _resp, const std::string& method, const std::string& request ):
            resp(_resp)
        {
            setMethod(method);
            setURI(request);
        }
        virtual ~TestRequest() {}

        virtual std::istream& stream() override
        {
            return cin;
        }
        virtual const Poco::Net::SocketAddress& clientAddress() const override
        {
            return sa;
        }

        virtual const Poco::Net::SocketAddress& serverAddress() const override
        {
            return sa;
        }

        virtual const Poco::Net::HTTPServerParams& serverParams() const override
        {
            return params;
        };

        virtual Poco::Net::HTTPServerResponse& response() const override
        {
            return resp;
        }

        virtual bool secure() const
        {
            return false;
        };

    protected:
        Poco::Net::SocketAddress sa;
        TestResponse& resp;
        TestParams params;
};


// -----------------------------------------------------------------------------
TEST_CASE("UHttpRouter: build", "[router][path][build]")
{
    std::vector<UPath::PathMeta> mEmpty;
    REQUIRE( mEmpty == UPath::build("") );

    std::vector<UPath::PathMeta> m1 =
    {
        {"root", ""},
        {"", "key1"},
        {"query", ""}
    };

    REQUIRE( m1 == UPath::build("/root/:key1/query?option1") );
    REQUIRE_FALSE( m1 == UPath::build("/root/:key1/") );

    std::vector<UPath::PathMeta> m2 =
    {
        {"root", ""},
        {"path2", ""},
        {"", "key1"},
        {"", "key2"},
        {"query", ""}
    };

    REQUIRE( m2 == UPath::build("/root/path2/:key1/:key2/query") );

    std::vector<UPath::PathMeta> m3 =
    {
        {"root", ""},
        {"path2", ""},
        {"", "key1"},
        {"", "key2"},
        {"", "key3"}
    };

    REQUIRE( m3 == UPath::build("/root/path2/:key1/:key2/:key3") );

}
// -----------------------------------------------------------------------------
TEST_CASE("UHttpRouter: path", "[router][path]")
{
    UPath p("/root/:key1/path/:key2/query");

    UHttpContext::Keys k0 =
    {
        {"key1", "k1"},
        {"key2", "k2"}
    };

    UHttpContext::Keys keys;

    REQUIRE( p.compare("/root/k1/path/k2/query", keys) );
    REQUIRE( p.compare("/root/k1/path/k2/query?opt1", keys) );
    REQUIRE( keys == k0 );
    REQUIRE_FALSE( p.compare("/root/k1/path/k2/", keys) );
    REQUIRE_FALSE( p.compare("/root/k1/p/k2/query", keys) );
    REQUIRE_FALSE( p.compare("", keys) );
    REQUIRE_FALSE( p.compare("/", keys) );

    UPath p2("/root/:key1");

    UHttpContext::Keys k2 =
    {
        {"key1", "k1"},
    };
    UHttpContext::Keys keys2;

    REQUIRE( p2.compare("/root/k1?opt1", keys2) );
    REQUIRE( keys2 == k2 );
}
// -----------------------------------------------------------------------------
TEST_CASE("UHttpRouter: router", "[router][base]")
{
    bool r1call = false;
    bool r2call = false;

    TestResponse res;
    TestRequest req1(res, "GET", "/info/25");
    TestRequest req2(res, "GET", "/test/42/call");

    REQUIRE( req1.getMethod() == Poco::Net::HTTPRequest::HTTP_GET );

    UHttpRouter r;
    r.get().add("/info/:id", [&](Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp, const UHttpContext& ctx)
    {
        r1call = true;
        REQUIRE( ctx.key_exists("id") );
        REQUIRE( ctx.key("id") == "25" );
    });

    r.get().add("/test/:id/call", [&](Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp, const UHttpContext& ctx)
    {
        r2call = true;
        REQUIRE( ctx.key_exists("id") );
        REQUIRE( ctx.key("id") == "42" );
    });

    REQUIRE( r.call(req1, res) );
    REQUIRE( r1call );

    REQUIRE( r.call(req2, res) );
    REQUIRE( r2call );
}
// -----------------------------------------------------------------------------
