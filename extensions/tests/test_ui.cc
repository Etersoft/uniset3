#include <catch.hpp>

#include <memory>
#include <time.h>
#include "IOController.grpc.pb.h"
#include "UInterface.h"
#include "UniSetTypes.h"
#include "UHelpers.h"

using namespace std;
using namespace uniset3;
using namespace uniset3::umessage;

extern ObjectId shmID;
const std::string sidName = "Input1_S";
const std::string aidName = "AI_AS";
ObjectId testOID;
ObjectId sid;
ObjectId aid;
ObjectId shmID;
std::shared_ptr<UInterface> ui;

void init()
{
    CHECK( uniset_conf() != nullptr );
    auto conf = uniset_conf();

    testOID = conf->getObjectID("TestProc");
    CHECK( testOID != DefaultObjectId );

    sid = conf->getSensorID(sidName);
    CHECK( sid != DefaultObjectId );

    aid = conf->getSensorID(aidName);
    CHECK( aid != DefaultObjectId );

    if( ui == nullptr )
        ui = std::make_shared<UInterface>();

    CHECK( ui->getObjectIndex() != nullptr );
    CHECK( ui->getConf() == uniset_conf() );

    REQUIRE( ui->getConfIOType(sid) == uniset3::DI );
    REQUIRE( ui->getConfIOType(aid) == uniset3::AI );
}

TEST_CASE("UInterface", "[UInterface]")
{
    init();
    auto conf = uniset_conf();

    SECTION( "GET/SET" )
    {
        REQUIRE_THROWS_AS( ui->getValue(DefaultObjectId), uniset3::ORepFailed );
        REQUIRE_NOTHROW( ui->setValue(sid, 1) );
        REQUIRE( ui->getValue(sid) == 1 );
        REQUIRE_NOTHROW( ui->setValue(sid, 100) );
        REQUIRE( ui->getValue(sid) == 100 ); // хоть это и дискретный датчик.. функция-то универсальная..

        REQUIRE_THROWS_AS( ui->getValue(sid, DefaultObjectId), uniset3::Exception );
        REQUIRE_THROWS_AS( ui->getValue(sid, 100), uniset3::Exception );

        REQUIRE_NOTHROW( ui->setValue(aid, 10) );
        REQUIRE( ui->getValue(aid) == 10 );

        uniset3::SensorInfo si;
        si.set_id(aid);
        si.set_node(conf->getLocalNode());
        REQUIRE_NOTHROW( ui->setValue(si, 15, DefaultObjectId) );
        REQUIRE( ui->getRawValue(si) == 15 );

        REQUIRE_NOTHROW( ui->setValue(si, 20, DefaultObjectId) );
        REQUIRE( ui->getValue(aid) == 20 );
        REQUIRE_THROWS_AS( ui->getValue(aid, -2), uniset3::Exception );

        si.set_id(sid);
        REQUIRE_NOTHROW( ui->setValue(si, 15, DefaultObjectId) );
        REQUIRE( ui->getValue(sid) == 15 );

        si.set_node(-2);
        REQUIRE_THROWS_AS( ui->setValue(si, 20, DefaultObjectId), uniset3::Exception );

        REQUIRE_THROWS_AS( ui->getTimeChange(sid, DefaultObjectId), uniset3::ORepFailed );
        REQUIRE_NOTHROW( ui->getTimeChange(sid, conf->getLocalNode()) );

        REQUIRE( ui->getIOType(sid, conf->getLocalNode()) == uniset3::DI );
        REQUIRE( ui->getIOType(aid, conf->getLocalNode()) == uniset3::AI );
    }

    SECTION( "resolve" )
    {
        REQUIRE_NOTHROW( ui->resolve(sid) );
        REQUIRE_THROWS_AS( ui->resolve(sid, 10), uniset3::ResolveNameError );
        REQUIRE_THROWS_AS( ui->resolve(sid, DefaultObjectId), uniset3::ResolveNameError );
        REQUIRE_NOTHROW( ui->resolveORefOnly(sid, conf->getLocalNode()) );
        REQUIRE_THROWS_AS( ui->resolveORefOnly(sid, 10), uniset3::ResolveNameError );
        REQUIRE_THROWS_AS( ui->resolveORefOnly(sid, DefaultObjectId), uniset3::ResolveNameError );
    }

    SECTION( "send" )
    {
        auto sm = makeSensorMessage(sid, 10, uniset3::AI);
        sm.mutable_header()->set_consumer(sid);
        TransportMessage tm = uniset3::to_transport<SensorMessage>(sm);
        REQUIRE_NOTHROW( ui->send(tm) );
    }

    SECTION( "sendText" )
    {
        uniset3::ProducerInfo pi;
        pi.set_id(sid);
        pi.set_node(conf->getLocalNode());
        auto tm = makeTextMessage("test", 0, now_to_uniset_timespec(), pi, uniset3::umessage::mpMedium, sid);
        REQUIRE_NOTHROW( ui->sendText(tm) );
        REQUIRE_NOTHROW( ui->sendText(sid, "text", 1) );
    }


    SECTION( "wait..exist.." )
    {
        CHECK( ui->waitReady(sid, 200, 50) );
        CHECK( ui->waitReady(sid, 200, 50, conf->getLocalNode()) );
        CHECK_FALSE( ui->waitReady(sid, 300, 50, DefaultObjectId) );
        CHECK_FALSE( ui->waitReady(sid, 300, 50, -20) );
        CHECK_FALSE( ui->waitReady(sid, -1, 50) );
        CHECK( ui->waitReady(sid, 300, -1) );

        CHECK( ui->waitWorking(sid, 200, 50) );
        CHECK( ui->waitWorking(sid, 200, 50, conf->getLocalNode()) );
        CHECK_FALSE( ui->waitWorking(sid, 100, 50, DefaultObjectId) );
        CHECK_FALSE( ui->waitWorking(sid, 100, 50, -20) );
        CHECK_FALSE( ui->waitWorking(sid, -1, 50) );
        CHECK( ui->waitWorking(sid, 100, -1) );

        CHECK( ui->isExists(sid) );
        CHECK( ui->isExists(sid, conf->getLocalNode()) );
        CHECK_FALSE( ui->isExists(sid, DefaultObjectId) );
        CHECK_FALSE( ui->isExists(sid, 100) );
    }

    SECTION( "get/set list" )
    {
        uniset3::IDList lst;
        lst.add(aid);
        lst.add(sid);
        lst.add(-100); // bad sensor ID

        uniset3::SensorIOInfoSeq seq = ui->getSensorSeq(lst);
        REQUIRE( seq.sensors().size() == 3 );

        uniset3::OutSeq olst;
        auto a1 = olst.add_sensors();
        a1->mutable_si()->set_id(sid);
        a1->mutable_si()->set_node(conf->getLocalNode());
        a1->set_value(1);
        auto a2 = olst.add_sensors();
        a2->mutable_si()->set_id(aid);
        a2->mutable_si()->set_node(conf->getLocalNode());
        a2->set_value(35);

        uniset3::IDSeq iseq = ui->setOutputSeq(olst, DefaultObjectId);
        REQUIRE( iseq.ids().size() == 0 );

        uniset3::ShortMapSeq slist = ui->getSensors( sid, conf->getLocalNode() );
        REQUIRE( slist.sensors().size() >= 2 );
    }

    SECTION( "ask" )
    {
        REQUIRE_THROWS_AS( ui->askSensor(sid, uniset3::UIONotify), uniset3::IOBadParam );
        REQUIRE_NOTHROW( ui->askSensor(sid, uniset3::UIONotify, testOID) );
        REQUIRE_NOTHROW( ui->askSensor(aid, uniset3::UIONotify, testOID) );
        REQUIRE_NOTHROW( ui->askSensor(aid, uniset3::UIODontNotify, testOID) );
        REQUIRE_NOTHROW( ui->askSensor(sid, uniset3::UIODontNotify, testOID) );

        REQUIRE_THROWS_AS( ui->askSensor(-20, uniset3::UIONotify), uniset3::Exception );

        REQUIRE_NOTHROW( ui->askRemoteSensor(sid, uniset3::UIONotify, conf->getLocalNode(), testOID) );
        REQUIRE_NOTHROW( ui->askRemoteSensor(aid, uniset3::UIONotify, conf->getLocalNode(), testOID) );
        REQUIRE_THROWS_AS( ui->askRemoteSensor(sid, uniset3::UIONotify, -3, testOID), uniset3::Exception );

        uniset3::IDList lst;
        lst.add(aid);
        lst.add(sid);
        lst.add(-100); // bad sensor ID

        uniset3::IDSeq rseq = ui->askSensorsSeq(lst, uniset3::UIONotify, testOID);
        REQUIRE( rseq.ids().size() == 1 ); // проверяем, что нам вернули один BAD-датчик..(-100)
    }

    SECTION( "Thresholds" )
    {
        // проверяем thresholds который был сформирован из секции <thresholds>
        ui->setValue(10, 378);
        REQUIRE( ui->getValue(13) == 1 );
        ui->setValue(10, 0);
        REQUIRE( ui->getValue(13) == 0 );
    }

    SECTION( "calibration" )
    {
        uniset3::SensorInfo si;
        si.set_id(aid);
        si.set_node(conf->getLocalNode());

        uniset3::CalibrateInfo ci;
        ci.set_minraw(0);
        ci.set_maxraw(4096);
        ci.set_mincal(-100);
        ci.set_maxcal(100);
        ci.set_precision(3);
        REQUIRE_NOTHROW( ui->calibrate(si, ci) );

        uniset3::CalibrateInfo ci2 = ui->getCalibrateInfo(si);
        CHECK( ci.minraw() == ci2.minraw() );
        CHECK( ci.maxraw() == ci2.maxraw() );
        CHECK( ci.mincal() == ci2.mincal() );
        CHECK( ci.maxcal() == ci2.maxcal() );
        CHECK( ci.precision() == ci2.precision() );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("UInterface::freezeValue", "[UInterface][freezeValue]")
{
    init();
    auto conf = uniset_conf();

    uniset3::SensorInfo si;
    si.set_id(aid);
    si.set_node(conf->getLocalNode());

    REQUIRE_NOTHROW( ui->setValue(aid, 200) );
    REQUIRE( ui->getValue(aid) == 200 );

    REQUIRE_NOTHROW( ui->freezeValue(si, true, 10, testOID) );
    REQUIRE( ui->getValue(aid) == 10 );

    REQUIRE_NOTHROW( ui->setValue(aid, 100) );
    REQUIRE( ui->getValue(aid) == 10 );

    REQUIRE_NOTHROW( ui->freezeValue(si, false, 10, testOID) );
    REQUIRE( ui->getValue(aid) == 100 );

    REQUIRE_NOTHROW( ui->freezeValue(si, true, -1, testOID) );
    REQUIRE( ui->getValue(aid) == -1 );
    REQUIRE_NOTHROW( ui->freezeValue(si, false, 0, testOID) );

    REQUIRE( ui->getValue(aid) == 100 );
    REQUIRE_NOTHROW( ui->setValue(aid, 200) );
    REQUIRE( ui->getValue(aid) == 200 );
}

// -----------------------------------------------------------------------------
TEST_CASE("UInterface::getSensorIOInfo", "[UInterface][getSensorIOInfo]")
{
    init();
    auto conf = uniset_conf();

    uniset3::SensorInfo si;
    si.set_id(aid);
    si.set_node(conf->getLocalNode());

    REQUIRE_NOTHROW( ui->setValue(si, 200, testOID) );
    REQUIRE( ui->getValue(aid) == 200 );

    REQUIRE_NOTHROW( ui->getSensorIOInfo(si) );
    auto inf = ui->getSensorIOInfo(si);

    REQUIRE( inf.supplier() == testOID );
    REQUIRE( inf.value() == 200 );
    REQUIRE( inf.real_value() == 200 );
    REQUIRE( inf.blocked() == false );
    REQUIRE( inf.frozen() == false );
    REQUIRE( inf.ts().sec() > 0 );
    REQUIRE( inf.dbignore() == false );
    REQUIRE( inf.depend_sid() == DefaultObjectId );

    // freeze/unfreeze
    REQUIRE_NOTHROW( ui->freezeValue(si, true, 10, testOID) );
    inf = ui->getSensorIOInfo(si);
    REQUIRE( inf.frozen() == true );
    REQUIRE( inf.supplier() == testOID );

    REQUIRE_NOTHROW( ui->freezeValue(si, false, 10, testOID) );
    inf = ui->getSensorIOInfo(si);
    REQUIRE( inf.frozen() == false );
    REQUIRE( inf.supplier() == testOID );

    // depend
    si.set_id(100);
    REQUIRE_NOTHROW( ui->setValue(si, 0, testOID) );

    si.set_id(101);
    inf = ui->getSensorIOInfo(si);

    REQUIRE( inf.blocked() == true );
    REQUIRE( inf.depend_sid() == 100 );

    si.set_id(100);
    REQUIRE_NOTHROW( ui->setValue(si, 10, testOID) );

    si.set_id(101);
    inf = ui->getSensorIOInfo(si);
    REQUIRE( inf.blocked() == false );
    REQUIRE( inf.depend_sid() == 100 );
}
