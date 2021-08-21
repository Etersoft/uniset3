#include <catch.hpp>
// -----------------------------------------------------------------------------
#include <Poco/JSON/Parser.h>
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

    auto s = cli.get("localhost", 8009, "api/v01/metrics/TestProc");
    REQUIRE_FALSE( s.empty() );

    Poco::JSON::Parser parser;
    auto result = parser.parse(s);

//    {
//        "id": "6000",
//        "name": "TestProc",
//        "metrics": [
//                {
//            "name": "msgCount",
//            "labels": {},
//            "description": "",
//            "dvalue": 0
//            },
//     ....

    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);
    REQUIRE( json->get("id").convert<ObjectId>() == 6000 );
    REQUIRE( json->get("name").convert<std::string>() == "TestProc" );

    auto jmetrics = json->get("metrics").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(jmetrics);
    REQUIRE(jmetrics->size() > 0 );
}
// -----------------------------------------------------------------------------
TEST_CASE("HttpApiGateway: resolve", "[apigateway][resolve]")
{
    InitTest();

    UHttp::UHttpClient cli;

    auto s = cli.get("localhost", 8009, "api/v01/resolve/TestProc");
    REQUIRE_FALSE( s.empty() );

    Poco::JSON::Parser parser;
    auto result = parser.parse(s);

//    {
//        "id": "6000",
//        "addr": "0.0.0.0:38831",
//        "path": "UNISET_PLC/UniObjects/",
//        "type": "UniSetManager",
//        "metadata": {}
//    }

    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);
    REQUIRE( json->get("id").convert<ObjectId>() == 6000 );
}
// -----------------------------------------------------------------------------
TEST_CASE("HttpApiGateway: configure", "[apigateway][configure]")
{
    InitTest();
    UHttp::UHttpClient cli;

    std::string s = cli.get("localhost", 8009, "api/v01/configure/get?2,Input5_S");
    REQUIRE_FALSE( s.empty() );

    Poco::JSON::Parser parser;
    auto result = parser.parse(s);

    // Ожидаемый формат ответа:
//    {
//        "conf": [
//          {
//            "id": "2",
//            "iotype": "DI",
//            "mbaddr": "0x01",
//            "mbfunc": "0x06",
//            "mbreg": "0x02",
//            "mbtype": "rtu",
//            "name": "Input2_S",
//            "nbit": "11",
//            "priority": "Medium",
//            "rs": "4",
//            "textname": "Команда 2"
//          },
//          {
//            "depend": "Input4_S",
//            "id": "5",
//            "iotype": "DI",
//            "name": "Input5_S",
//            "priority": "Medium",
//            "textname": "Команда 5",
//            "udp": "2"
//          }
//        ]
//    }

    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);

    auto jconf = json->get("conf").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(jconf);

    Poco::JSON::Object::Ptr jret = jconf->getObject(0);
    REQUIRE(jret);

    REQUIRE( jret->get("iotype").convert<std::string>() == "DI" );
    REQUIRE( jret->get("id").convert<ObjectId>() == 2 );

    Poco::JSON::Object::Ptr jret2 = jconf->getObject(1);
    REQUIRE(jret2);

    REQUIRE( jret2->get("iotype").convert<std::string>() == "DI" );
    REQUIRE( jret2->get("name").convert<std::string>() == "Input5_S" );
    REQUIRE( jret2->get("id").convert<ObjectId>() == 5 );
    REQUIRE( jret2->get("priority").convert<std::string>() == "Medium");
}
// -----------------------------------------------------------------------------
TEST_CASE("HttpApiGateway: configure by props", "[apigateway][configure][props]")
{
    InitTest();
    UHttp::UHttpClient cli;

    std::string s = cli.get("localhost", 8009, "api/v01/configure/get?2,Input5_S&props=iotype");
    REQUIRE_FALSE( s.empty() );

    Poco::JSON::Parser parser;
    auto result = parser.parse(s);

    // Ожидаемый формат ответа:
//    {
//        "conf": [
//          {
//            "id": "2",
//            "iotype": "DI",
//            "name": "Input2_S"
//          },
//          {
//            "id": "5",
//            "iotype": "DI",
//            "name": "Input5_S"
//          }
//       ]
//    }

    Poco::JSON::Object::Ptr json = result.extract<Poco::JSON::Object::Ptr>();
    REQUIRE(json);

    auto jconf = json->get("conf").extract<Poco::JSON::Array::Ptr>();
    REQUIRE(jconf);

    Poco::JSON::Object::Ptr jret = jconf->getObject(0);
    REQUIRE(jret);

    REQUIRE( jret->get("iotype").convert<std::string>() == "DI" );
    REQUIRE( jret->get("id").convert<ObjectId>() == 2 );
    auto p1 = jret->get("priority");
    REQUIRE( p1.isEmpty() );

    Poco::JSON::Object::Ptr jret2 = jconf->getObject(1);
    REQUIRE(jret2);

    REQUIRE( jret2->get("iotype").convert<std::string>() == "DI" );
    REQUIRE( jret2->get("name").convert<std::string>() == "Input5_S" );
    REQUIRE( jret2->get("id").convert<ObjectId>() == 5 );
    auto p2 = jret2->get("priority");
    REQUIRE( p2.isEmpty() );
}