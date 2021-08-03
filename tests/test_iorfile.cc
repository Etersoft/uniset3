#include <catch.hpp>

#include <sstream>
#include <fstream>
#include "Configuration.h"
#include "UniSetTypes.h"
#include "IORFile.h"
using namespace std;
using namespace uniset3;

TEST_CASE("IORFile", "[iorfile][basic]" )
{
    CHECK( uniset_conf() != nullptr );

    ObjectId testID = 101;
    ObjectRef oref;
    oref.set_id(testID);
    oref.set_addr("localhost:1234");
    oref.set_path("/sensors");
    oref.set_type("UniSetObject");

    IORFile ior(uniset_conf()->getLockDir());

    REQUIRE_NOTHROW( ior.setIOR(testID, oref) );
    CHECK( file_exists(ior.getFileName(testID)) );

    REQUIRE_NOTHROW(ior.getRef(testID) );
    auto oref2 = ior.getRef(testID);
    REQUIRE( oref.id() == oref2.id() );
    REQUIRE( oref.addr() == oref2.addr() );
    REQUIRE( oref.path() == oref2.path() );
    REQUIRE( oref.type() == oref2.type() );

    ior.unlinkIOR(testID);
    CHECK_FALSE( file_exists(ior.getFileName(testID)) );
}
