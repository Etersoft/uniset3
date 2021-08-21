#include <catch.hpp>
// -----------------------------------------------------------------------------
#include "UniSetTypes.h"
#include "Exceptions.h"
#include "UInterface.h"
#include "UHttpClient.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// -----------------------------------------------------------------------------
static shared_ptr<UInterface> ui;
static const ObjectId TestProc = 6000;
static const ObjectId Node1 = 3001;
// -----------------------------------------------------------------------------
static void InitTest()
{
    auto conf = uniset_conf();
    CHECK( conf != nullptr );

    if( !ui )
    {
        ui = make_shared<UInterface>();
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
    }

    REQUIRE( conf->isLocalIOR() );
    REQUIRE( ui->isExists(TestProc) );

}
// -----------------------------------------------------------------------------
TEST_CASE("HttpApiGateway: metrics", "[apigateway][metrics]")
{
    InitTest();

    UHttp::UHttpClient cli;

    auto ret = cli.get("localhost", 8009, "api/v01/metrics/TestProc");
    REQUIRE_FALSE( ret.empty() );
}
// -----------------------------------------------------------------------------
TEST_CASE("HttpApiGateway: resolve", "[apigateway][resolve]")
{
    InitTest();

    UHttp::UHttpClient cli;

    auto ret = cli.get("localhost", 8009, "api/v01/resolve/TestProc");
    REQUIRE_FALSE( ret.empty() );
}
// -----------------------------------------------------------------------------
