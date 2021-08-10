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
        // UI понадобиться для проверки записанных в SM значений.
        CHECK( ui->getObjectIndex() != nullptr );
        CHECK( ui->getConf() == conf );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("HttpResolver: resolve", "[urepository][ui]")
{
    InitTest();

    REQUIRE_NOTHROW( ui->resolve(TestProc, Node1) );
    REQUIRE_THROWS_AS( ui->resolve(DefaultObjectId, Node1), uniset3::ResolveNameError& );
    REQUIRE( ui->isExists(TestProc, Node1) );
}
// -----------------------------------------------------------------------------

