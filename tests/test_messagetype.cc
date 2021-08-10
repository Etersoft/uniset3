#include <catch.hpp>
// ---------------------------------------------------------------
#include "Configuration.h"
#include "MessageTypes.pb.h"
#include "UniSetTypes.h"
#include "UHelpers.h"
// ---------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::umessage;
// ---------------------------------------------------------------
TEST_CASE("SensorMessage", "[basic][message types][SensorMessage]" )
{
    CHECK( uniset_conf() != nullptr );

    auto conf = uniset_conf();

    SECTION("Default consturctor")
    {
        auto sm = makeSensorMessage(DefaultObjectId,0,uniset3::DI);
        CHECK( sm.header().priority() == mpMedium );
        CHECK( sm.header().node() == conf->getLocalNode() );
        CHECK( sm.header().supplier() == DefaultObjectId );
        CHECK( sm.header().consumer() == DefaultObjectId );
        CHECK( sm.id() == DefaultObjectId );
        CHECK( sm.value() == 0 );
        CHECK( sm.undefined() == false );

        CHECK( sm.sensor_type() == uniset3::DI ); // UnknownIOType
        CHECK( sm.ci().precision() == 0 );
        CHECK( sm.ci().minraw() == 0 );
        CHECK( sm.ci().maxraw() == 0 );
        CHECK( sm.ci().mincal() == 0 );
        CHECK( sm.ci().maxcal() == 0 );
        CHECK( sm.threshold() == 0 );
        CHECK( sm.tid() == uniset3::DefaultThresholdId );
    }

    SECTION("Default SensorMessage")
    {
        ObjectId sid = 1;
        long val = 100;
        auto sm = makeSensorMessage(sid, val, uniset3::AI);
        REQUIRE( sm.id() == sid );
        REQUIRE( sm.value() == val );
        REQUIRE( sm.sensor_type() == uniset3::AI );
    }

    SECTION("Transport SensorMessage")
    {
        ObjectId sid = 1;
        long val = 100;
        auto sm = makeSensorMessage(sid, val, uniset3::AI);
        REQUIRE( sm.id() == sid );
        REQUIRE( sm.value() == val );
        REQUIRE( sm.sensor_type() == uniset3::AI );

        auto tm = uniset3::to_transport<SensorMessage>(sm);

        SensorMessage sm2;
        REQUIRE( tm.data().UnpackTo(&sm2) );
        REQUIRE( sm2.id() == sid );
        REQUIRE( sm2.value() == val );
        REQUIRE( sm2.sensor_type() == uniset3::AI );
    }
}
// ---------------------------------------------------------------
TEST_CASE("SystemMessage", "[basic][message types][SystemMessage]" )
{
    CHECK( uniset_conf() != nullptr );
    auto conf = uniset_conf();

    SECTION("Default consturctor")
    {
        SystemMessage sm = makeSystemMessage();
        CHECK( sm.header().priority() == mpHigh );
        CHECK( sm.header().node() == conf->getLocalNode() );
        CHECK( sm.header().supplier() == DefaultObjectId );
        CHECK( sm.header().consumer() == DefaultObjectId );
        CHECK( sm.cmd() == SystemMessage::Unknown );
        CHECK( sm.data_size() == 0 );
    }

    SECTION("Default SystemMessage")
    {
        SystemMessage::Command cmd = SystemMessage::StartUp;
        auto sm = makeSystemMessage(cmd);
        REQUIRE( sm.cmd() == cmd );
        CHECK( sm.header().priority() == mpHigh );
    }

    SECTION("Transport SystemMessage")
    {
        SystemMessage::Command cmd = SystemMessage::StartUp;
        int dat = 100;
        auto sm = makeSystemMessage(cmd);
        sm.add_data(dat);
        REQUIRE( sm.cmd() == cmd );

        auto tm = to_transport<SystemMessage>(sm);

        SystemMessage sm2;
        REQUIRE( tm.data().UnpackTo(&sm2) );
        REQUIRE( sm2.cmd() == cmd );
        REQUIRE( sm2.data(0) == dat );
    }
}
// ---------------------------------------------------------------
TEST_CASE("TimerMessage", "[basic][message types][TimerMessage]" )
{
    CHECK( uniset_conf() != nullptr );
    auto conf = uniset_conf();

    SECTION("Default consturctor")
    {
        TimerMessage tm = makeTimerMessage();
        CHECK( tm.header().priority() == mpMedium );
        CHECK( tm.header().node() == conf->getLocalNode() );
        CHECK( tm.header().supplier() == DefaultObjectId );
        CHECK( tm.header().consumer() == DefaultObjectId );
        CHECK( tm.id() == uniset3::DefaultTimerId );
    }

    SECTION("Default TimerMessage")
    {
        int tid = 100;
        TimerMessage tm =makeTimerMessage(tid);
        REQUIRE( tm.id() == tid );
    }

    SECTION("Transport TimerMessage")
    {
        int tid = 100;
        TimerMessage tm = makeTimerMessage(tid);
        REQUIRE( tm.id() == tid );

        auto m = to_transport<TimerMessage>(tm);

        TimerMessage tm2;
        REQUIRE( m.data().UnpackTo(&tm2) );
        REQUIRE( tm2.id() == tid );
    }
}
// ---------------------------------------------------------------
TEST_CASE("ConfirmMessage", "[basic][message types][ConfirmMessage]" )
{
    CHECK( uniset_conf() != nullptr );
    auto conf = uniset_conf();

    ObjectId sid = 1;
    double val = 100;
    timespec t1_event = { 10, 300 };
    auto t_event = to_uniset_timespec(t1_event);

    timespec t1_confirm = { 10, 90 };
    auto t_confirm = to_uniset_timespec(t1_confirm);

    SECTION("Default consturctor")
    {
        ConfirmMessage cm = makeConfirmMessage(sid, val, t_event, t_confirm);
        CHECK( cm.header().priority() == mpMedium );
        CHECK( cm.header().node() == conf->getLocalNode() );
        CHECK( cm.header().supplier() == DefaultObjectId );
        CHECK( cm.header().consumer() == DefaultObjectId );
        REQUIRE( cm.sensor_id() == sid );
        REQUIRE( cm.sensor_value() == val );
        REQUIRE( equal(cm.sensor_ts(), t_event) );
        REQUIRE( equal(cm.confirm_ts(), t_confirm) );
        CHECK( cm.broadcast() == false );
        CHECK( cm.forward() == false );
    }

    SECTION("Transport ConfirmMessage")
    {
        ConfirmMessage cm = makeConfirmMessage(sid, val, t_event, t_confirm);
        REQUIRE( cm.sensor_id() == sid );
        REQUIRE( cm.sensor_value() == val );
        REQUIRE( equal(cm.sensor_ts(), t_event) );
        REQUIRE( equal(cm.confirm_ts(), t_confirm) );

        auto tm = to_transport<ConfirmMessage>(cm);

        ConfirmMessage cm2;
        REQUIRE( tm.data().UnpackTo(&cm2) );
        REQUIRE( cm2.sensor_id() == sid );
        REQUIRE( cm2.sensor_value() == val );
        REQUIRE( equal(cm2.sensor_ts(), t_event) );
        REQUIRE( equal(cm2.confirm_ts(), t_confirm) );
    }
}
// ---------------------------------------------------------------
TEST_CASE("TextMessage", "[basic][message types][TextMessage]" )
{
    CHECK( uniset_conf() != nullptr );
    auto conf = uniset_conf();

    SECTION("Default consturctor")
    {
        TextMessage tm = makeTextMessage();
        CHECK( tm.header().priority() == mpMedium );
        CHECK( tm.header().node() == conf->getLocalNode() );
        CHECK( tm.header().supplier() == DefaultObjectId );
        CHECK( tm.header().consumer() == DefaultObjectId );
        CHECK( tm.txt() == "" );
    }

    SECTION("TextMessage from network")
    {
        std::string txt = "Hello world";

        uniset3::Timespec tspec;
        tspec.set_sec(10);
        tspec.set_nsec(100);

        uniset3::ProducerInfo pi;
        pi.set_id(30);
        pi.set_node(conf->getLocalNode());

        ObjectId consumer = 40;

        TextMessage tm = makeTextMessage(txt, 3, tspec, pi, umessage::mpHigh, consumer );
        REQUIRE( tm.header().consumer() == consumer );
        REQUIRE( tm.header().node() == pi.node() );
        REQUIRE( tm.header().supplier() == pi.id() );
        REQUIRE( equal(tm.header().ts(), tspec) );
        REQUIRE( tm.txt() == txt );
        REQUIRE( tm.mtype() == 3 );

        auto m = to_transport<TextMessage>(tm);

        TextMessage tm2;
        REQUIRE( m.data().UnpackTo(&tm2) );
        REQUIRE( tm2.header().consumer() == consumer );
        REQUIRE( tm2.header().node() == pi.node() );
        REQUIRE( tm2.header().supplier() == pi.id() );
        REQUIRE( equal(tm2.header().ts(), tspec) );
        REQUIRE( tm2.txt() == txt );
        REQUIRE( tm2.mtype() == 3 );
    }
}
// ---------------------------------------------------------------
