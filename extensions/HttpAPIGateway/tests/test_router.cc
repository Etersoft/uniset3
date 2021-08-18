#include <catch.hpp>
// -----------------------------------------------------------------------------
#include "UHttpRouter.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
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

}
// -----------------------------------------------------------------------------
TEST_CASE("UHttpRouter: router", "[router][base]")
{
    bool r1call = false;
    bool r2call = false;

    UHttpRouter r;
    r.get().add("/info/:id", [&](Poco::Net::HTTPServerRequest&, Poco::Net::HTTPServerResponse&, const UHttpContext&)
    {
        r1call = true;
    });

    r.get().add("/test/:id/call?option1=1", [&](Poco::Net::HTTPServerRequest&, Poco::Net::HTTPServerResponse&, const UHttpContext&)
    {
        r2call = true;
    });
}
// -----------------------------------------------------------------------------
