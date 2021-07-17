#include <catch.hpp>

#include <memory>
#include <time.h>
#include "IOController_i.hh"
#include "UInterface.h"
#include "UniSetTypes.h"

using namespace std;
using namespace uniset3;

const std::string sidName = "Input1_S";
const std::string aidName = "AI_AS";
ObjectId testOID;
ObjectId sid;
ObjectId aid;
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
        REQUIRE_THROWS_AS( ui->getValue(DefaultObjectId), uniset3::ORepFailed& );
        REQUIRE_NOTHROW( ui->setValue(sid, 1) );
        REQUIRE( ui->getValue(sid) == 1 );
        REQUIRE_NOTHROW( ui->setValue(sid, 100) );
        REQUIRE( ui->getValue(sid) == 100 ); // хоть это и дискретный датчик.. функция-то универсальная..

        REQUIRE_THROWS_AS( ui->getValue(sid, DefaultObjectId), uniset3::Exception& );
        REQUIRE_THROWS_AS( ui->getValue(sid, 100), uniset3::Exception& );

        REQUIRE_NOTHROW( ui->setValue(aid, 10) );
        REQUIRE( ui->getValue(aid) == 10 );

        uniset3::SensorInfo si;
        si.id = aid;
        si.node = conf->getLocalNode();
        REQUIRE_NOTHROW( ui->setValue(si, 15, DefaultObjectId) );
        REQUIRE( ui->getRawValue(si) == 15 );

        REQUIRE_NOTHROW( ui->fastSetValue(si, 20, DefaultObjectId) );
        REQUIRE( ui->getValue(aid) == 20 );
        REQUIRE_THROWS_AS( ui->getValue(aid, -2), uniset3::Exception& );

        si.id = sid;
        REQUIRE_NOTHROW( ui->setValue(si, 15, DefaultObjectId) );
        REQUIRE( ui->getValue(sid) == 15 );

        si.node = -2;
        REQUIRE_THROWS_AS( ui->setValue(si, 20, DefaultObjectId), uniset3::Exception& );

        REQUIRE_THROWS_AS( ui->getTimeChange(sid, DefaultObjectId), uniset3::ORepFailed& );
        REQUIRE_NOTHROW( ui->getTimeChange(sid, conf->getLocalNode()) );

        si.id = aid;
        si.node = conf->getLocalNode();
        REQUIRE_NOTHROW( ui->setUndefinedState(si, true, testOID) );
        REQUIRE_NOTHROW( ui->setUndefinedState(si, false, testOID) );

        si.id = sid;
        si.node = conf->getLocalNode();
        REQUIRE_NOTHROW( ui->setUndefinedState(si, true, testOID) );
        REQUIRE_NOTHROW( ui->setUndefinedState(si, false, testOID) );

        REQUIRE( ui->getIOType(sid, conf->getLocalNode()) == uniset3::DI );
        REQUIRE( ui->getIOType(aid, conf->getLocalNode()) == uniset3::AI );
    }

    SECTION( "resolve" )
    {
        REQUIRE_NOTHROW( ui->resolve(sid) );
        REQUIRE_THROWS_AS( ui->resolve(sid, 10), uniset3::ResolveNameError& );
        REQUIRE_THROWS_AS( ui->resolve(sid, DefaultObjectId), uniset3::ResolveNameError& );
        REQUIRE_NOTHROW( ui->resolve("UNISET_PLC/Controllers/SharedMemory") );
    }

    SECTION( "send" )
    {
        TransportMessage tm( SensorMessage(sid, 10).transport_msg() );
        REQUIRE_NOTHROW( ui->send(sid, tm) );
    }

    SECTION( "sendText" )
    {
        TransportMessage tm( SensorMessage(sid, 10).transport_msg() );
        REQUIRE_NOTHROW( ui->send(sid, tm) );
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

        CHECK( ui->isExist(sid) );
        CHECK( ui->isExist(sid, conf->getLocalNode()) );
        CHECK_FALSE( ui->isExist(sid, DefaultObjectId) );
        CHECK_FALSE( ui->isExist(sid, 100) );
    }

    SECTION( "get/set list" )
    {
        uniset3::IDList lst;
        lst.add(aid);
        lst.add(sid);
        lst.add(-100); // bad sensor ID

        uniset3::SensorInfoSeq_var seq = ui->getSensorSeq(lst);
        REQUIRE( seq->length() == 3 );

        uniset3::OutSeq_var olst = new uniset3::OutSeq();
        olst->length(2);
        olst[0].si.id = sid;
        olst[0].si.node = conf->getLocalNode();
        olst[0].value = 1;
        olst[1].si.id = aid;
        olst[1].si.node = conf->getLocalNode();
        olst[1].value = 35;

        uniset3::IDSeq_var iseq = ui->setOutputSeq(olst, DefaultObjectId);
        REQUIRE( iseq->length() == 0 );

        uniset3::ShortMapSeq_var slist = ui->getSensors( sid, conf->getLocalNode() );
        REQUIRE( slist->length() >= 2 );
    }

    SECTION( "ask" )
    {
        REQUIRE_THROWS_AS( ui->askSensor(sid, uniset3::UIONotify), uniset3::IOBadParam& );
        REQUIRE_NOTHROW( ui->askSensor(sid, uniset3::UIONotify, testOID) );
        REQUIRE_NOTHROW( ui->askSensor(aid, uniset3::UIONotify, testOID) );
        REQUIRE_NOTHROW( ui->askSensor(aid, uniset3::UIODontNotify, testOID) );
        REQUIRE_NOTHROW( ui->askSensor(sid, uniset3::UIODontNotify, testOID) );

        REQUIRE_THROWS_AS( ui->askSensor(-20, uniset3::UIONotify), uniset3::Exception& );

        REQUIRE_NOTHROW( ui->askRemoteSensor(sid, uniset3::UIONotify, conf->getLocalNode(), testOID) );
        REQUIRE_NOTHROW( ui->askRemoteSensor(aid, uniset3::UIONotify, conf->getLocalNode(), testOID) );
        REQUIRE_THROWS_AS( ui->askRemoteSensor(sid, uniset3::UIONotify, -3, testOID), uniset3::Exception& );

        uniset3::IDList lst;
        lst.add(aid);
        lst.add(sid);
        lst.add(-100); // bad sensor ID

        uniset3::IDSeq_var rseq = ui->askSensorsSeq(lst, uniset3::UIONotify, testOID);
        REQUIRE( rseq->length() == 1 ); // проверяем, что нам вернули один BAD-датчик..(-100)
    }

    SECTION( "Thresholds" )
    {
        REQUIRE_NOTHROW( ui->askThreshold(aid, 10, uniset3::UIONotify, 90, 100, false, testOID) );
        REQUIRE_NOTHROW( ui->askThreshold(aid, 11, uniset3::UIONotify, 50, 70, false, testOID) );
        REQUIRE_NOTHROW( ui->askThreshold(aid, 12, uniset3::UIONotify, 20, 40, false, testOID) );
        REQUIRE_THROWS_AS( ui->askThreshold(aid, 3, uniset3::UIONotify, 50, 20, false, testOID), uniset3::BadRange& );

        uniset3::ThresholdsListSeq_var slist = ui->getThresholdsList(aid);
        REQUIRE( slist->length() == 1 ); // количество датчиков с порогами = 1 (это aid)

        // 3 порога мы создали выше(askThreshold) + 1 который в настроечном файле в секции <thresholds>
        REQUIRE( slist[0].tlist.length() == 4 );

        uniset3::ThresholdInfo ti1 = ui->getThresholdInfo(aid, 10);
        REQUIRE( ti1.id == 10 );
        REQUIRE( ti1.lowlimit == 90 );
        REQUIRE( ti1.hilimit == 100 );

        uniset3::ThresholdInfo ti2 = ui->getThresholdInfo(aid, 11);
        REQUIRE( ti2.id == 11 );
        REQUIRE( ti2.lowlimit == 50 );
        REQUIRE( ti2.hilimit == 70 );

        uniset3::ThresholdInfo ti3 = ui->getThresholdInfo(aid, 12);
        REQUIRE( ti3.id == 12 );
        REQUIRE( ti3.lowlimit == 20 );
        REQUIRE( ti3.hilimit == 40 );

        REQUIRE_THROWS_AS( ui->getThresholdInfo(sid, 10), uniset3::NameNotFound& );

        // проверяем thresholds который был сформирован из секции <thresholds>
        ui->setValue(10, 378);
        REQUIRE( ui->getValue(13) == 1 );
        ui->setValue(10, 0);
        REQUIRE( ui->getValue(13) == 0 );
    }

    SECTION( "calibration" )
    {
        uniset3::SensorInfo si;
        si.id = aid;
        si.node = conf->getLocalNode();

        uniset3::CalibrateInfo ci;
        ci.minRaw = 0;
        ci.maxRaw = 4096;
        ci.minCal = -100;
        ci.maxCal = 100;
        ci.precision = 3;
        REQUIRE_NOTHROW( ui->calibrate(si, ci) );

        uniset3::CalibrateInfo ci2 = ui->getCalibrateInfo(si);
        CHECK( ci.minRaw == ci2.minRaw );
        CHECK( ci.maxRaw == ci2.maxRaw );
        CHECK( ci.minCal == ci2.minCal );
        CHECK( ci.maxCal == ci2.maxCal );
        CHECK( ci.precision == ci2.precision );
    }
}
// -----------------------------------------------------------------------------
TEST_CASE("UInterface::freezeValue", "[UInterface][freezeValue]")
{
    init();
    auto conf = uniset_conf();

    uniset3::SensorInfo si;
    si.id = aid;
    si.node = conf->getLocalNode();

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
    si.id = aid;
    si.node = conf->getLocalNode();

    REQUIRE_NOTHROW( ui->setValue(si, 200, testOID) );
    REQUIRE( ui->getValue(aid) == 200 );

    REQUIRE_NOTHROW( ui->getSensorIOInfo(si) );
    auto inf = ui->getSensorIOInfo(si);

    REQUIRE( inf.supplier == testOID );
    REQUIRE( inf.value == 200 );
    REQUIRE( inf.real_value == 200 );
    REQUIRE( inf.blocked == false );
    REQUIRE( inf.frozen == false );
    REQUIRE( inf.undefined == false );
    REQUIRE( inf.tv_sec > 0 );
    REQUIRE( inf.dbignore == false );
    REQUIRE( inf.depend_sid == DefaultObjectId );

    // freeze/unfreeze
    REQUIRE_NOTHROW( ui->freezeValue(si, true, 10, testOID) );
    inf = ui->getSensorIOInfo(si);
    REQUIRE( inf.frozen == true );
    REQUIRE( inf.supplier == testOID );

    REQUIRE_NOTHROW( ui->freezeValue(si, false, 10, testOID) );
    inf = ui->getSensorIOInfo(si);
    REQUIRE( inf.frozen == false );
    REQUIRE( inf.supplier == testOID );

    // undef
    REQUIRE_NOTHROW( ui->setUndefinedState( si, true, testOID ));
    inf = ui->getSensorIOInfo(si);
    REQUIRE( inf.undefined == true );
    REQUIRE( inf.supplier == testOID );

    REQUIRE_NOTHROW( ui->setUndefinedState( si, false, testOID ));
    inf = ui->getSensorIOInfo(si);
    REQUIRE( inf.undefined == false );
    REQUIRE( inf.supplier == testOID );

    // depend
    si.id = 100;
    REQUIRE_NOTHROW( ui->setValue(si, 0, testOID) );

    si.id = 101;
    inf = ui->getSensorIOInfo(si);

    REQUIRE( inf.blocked == true );
    REQUIRE( inf.depend_sid == 100 );

    si.id = 100;
    REQUIRE_NOTHROW( ui->setValue(si, 10, testOID) );

    si.id = 101;
    inf = ui->getSensorIOInfo(si);
    REQUIRE( inf.blocked == false );
    REQUIRE( inf.depend_sid == 100 );
}
