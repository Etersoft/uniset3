#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <sstream>
#include <limits>
#include "Configuration.h"
#include "UniSetTypes.h"
#include "IOController.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// -----------------------------------------------------------------------------
//! \todo test_iocontroller_types: Дописать больше тестов
// -----------------------------------------------------------------------------
TEST_CASE("IOController: USensorInfo", "[ioc][usi]" )
{
    IOController::USensorInfo usi;
    usi.sinf.set_supplier(100);
    usi.sinf.set_blocked(true);
    usi.sinf.set_value(9);

    SECTION( "makeSensorIOInfo" )
    {
        uniset3::SensorIOInfo si( usi.makeSensorIOInfo() );
        REQUIRE( si.supplier() == 100 );
        REQUIRE( si.blocked() == true );
        REQUIRE( si.value() == 9 );
    }

    SECTION( "makeSensorMessage" )
    {
        uniset3::umessage::SensorMessage sm( usi.makeSensorMessage() );
        REQUIRE( sm.header().supplier() == 100 );
        REQUIRE( sm.value() == 9 );
    }

    WARN("Tests for 'UniSetTypes' incomplete...");
}
// -----------------------------------------------------------------------------
