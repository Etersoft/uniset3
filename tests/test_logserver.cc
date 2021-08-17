#include <catch.hpp>

#include <memory>
#include <sstream>
#include <thread>
#include <future>

#include "UniSetTypes.h"
#include "LogServer.h"
#include "LogAgregator.h"
#include "LogReader.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// --------------------------------------------------------------------------
static const std::string ip = "localhost";
static const int port = 33333;

static const std::string test_msg1 = "Test message N1";
static const std::string test_msg2 = "Test message N2";

static ostringstream msg;
static ostringstream msg2;
static ostringstream la_msg;
static std::mutex r1_mutex;
static std::mutex r2_mutex;

static int readTimeout = 5000;
static int readPause = 2000;

struct LogThreadGuard
{
    LogThreadGuard(std::shared_ptr<LogReader>& l, std::future<bool>& f):
      lreader(l),ret(f){}

    ~LogThreadGuard()
    {
        if( lreader )
            lreader->terminate();
        if( ret.valid() )
            ret.get();
    }

    std::shared_ptr<LogReader>& lreader;
    std::future<bool>& ret;
};

// --------------------------------------------------------------------------
void rlog1OnEvent( const std::string& s )
{
    std::lock_guard<std::mutex> l(r1_mutex);
    msg << s;
}
// --------------------------------------------------------------------------
void rlog2OnEvent( const std::string& s )
{
    std::lock_guard<std::mutex> l(r2_mutex);
    msg2 << s;
}
// --------------------------------------------------------------------------
void la_logOnEvent( const std::string& s )
{
    la_msg << s;
}
// --------------------------------------------------------------------------
static std::shared_ptr<LogReader> lr1;
bool readlog_thread1()
{
    try
    {
        if( !lr1 )
        {
            lr1 = make_shared<LogReader>();
            lr1->setTimeout(readTimeout);
            lr1->signal_stream_event().connect( sigc::ptr_fun(rlog1OnEvent) );
            // lr1->setReadCount(1);
            // lr1->setLogLevel(Debug::ANY);
        }
        logserver::LogCommandList cmd;

        do
        {
            lr1->readLoop(ip, port, cmd, false);
        }
        while( lr1->isActive() );
        return true;
    }
    catch( std::exception& ex )
    {
    }

    return false;
}
// --------------------------------------------------------------------------
static std::shared_ptr<LogReader> lr2;
bool readlog_thread2()
{
    try
    {
        if( !lr2 )
        {
            lr2 = make_shared<LogReader>();
            lr2->setTimeout(readTimeout);
            lr2->signal_stream_event().connect( sigc::ptr_fun(rlog2OnEvent) );
            // lr2->setReadCount(1);
            // lr2->setLogLevel(Debug::ANY);
        }
        logserver::LogCommandList cmd;

        do
        {
            lr2->readLoop(ip, port, cmd, false);
        }
        while( lr2->isActive() );

        return true;
    }
    catch( std::exception& ex )
    {
    }

    return false;
}
// --------------------------------------------------------------------------
TEST_CASE("LogAgregator", "[LogServer][LogAgregator]" )
{
    auto la = make_shared<LogAgregator>();
    auto log1 = la->create("log1");
    auto log2 = la->create("log2");

    log1->showDateTime(false);
    log2->showDateTime(false);
    log1->showLogType(false);
    log2->showLogType(false);

    la->signal_stream_event().connect( sigc::ptr_fun(la_logOnEvent) );

    log1->level(Debug::ANY);
    log2->level(Debug::ANY);

    log1->any() << test_msg1;
    REQUIRE( la_msg.str() == test_msg1 );

    la_msg.str("");

    log2->any() << test_msg2;
    REQUIRE( la_msg.str() == test_msg2 );

    auto lst = la->getLogList();
    REQUIRE( lst.size() == 2 );

    // Проверка поиска по регулярным выражениям
    auto lst2 = la->getLogList("lo.*");
    REQUIRE( lst2.size() == 2 );

    auto lst3 = la->getLogList("log\\d{1}");
    REQUIRE( lst3.size() == 2 );
}
// --------------------------------------------------------------------------
TEST_CASE("LogServer", "[LogServer]" )
{
    auto la = make_shared<LogAgregator>();
    auto log1 = la->create("log1");
    auto log2 = la->create("log2");

    log1->level(Debug::ANY);
    log2->level(Debug::ANY);
    log1->showDateTime(false);
    log2->showDateTime(false);
    log1->showLogType(false);
    log2->showLogType(false);

    la_msg.str("");
    la->signal_stream_event().connect( sigc::ptr_fun(la_logOnEvent) );

    LogServer ls(la);
    //ls.setSessionLog(Debug::ANY);
    ls.async_run( ip, port);

    for( int i = 0; i < 3 && !ls.isRunning(); i++ )
        msleep(600);

    REQUIRE( ls.isRunning() );

    msg.str("");
    std::future<bool> ret = std::async(std::launch::async, readlog_thread1); // make_shared<std::thread>(readlog_thread1);

    LogThreadGuard g(lr1, ret);

    msleep(100); // небольшая пауза на создание потока и т.п.

    ostringstream m;
    m << test_msg1 << endl;

    log1->any() << m.str();

    REQUIRE( la_msg.str() == m.str() );

    msleep(readPause); // пауза на подключение reader-а к серверу..

    {
        std::lock_guard<std::mutex> l(r1_mutex);
        // message contains text
        REQUIRE( msg.str().find(m.str()) != string::npos );
    }

    msg.str("");
    la_msg.str("");

    ostringstream m2;
    m2 << test_msg2 << endl;
    log2->any() << m2.str();
    REQUIRE( la_msg.str() == m2.str() );

    msleep(readPause);

    {
        std::lock_guard<std::mutex> l(r1_mutex);
        // message contains text
        REQUIRE( msg.str().find(m2.str()) != string::npos );
    }
}
// --------------------------------------------------------------------------
TEST_CASE("MaxSessions", "[LogServer]" )
{
    auto la = make_shared<LogAgregator>();
    auto log1 = la->create("log1");
    auto log2 = la->create("log2");

    log1->level(Debug::ANY);
    log2->level(Debug::ANY);
    log1->showDateTime(false);
    log2->showDateTime(false);
    log1->showLogType(false);
    log2->showLogType(false);

    la_msg.str("");
    la->signal_stream_event().connect( sigc::ptr_fun(la_logOnEvent) );

    LogServer ls(la);
    //ls.setSessionLog(Debug::ANY);
    ls.setMaxSessionCount(1);
    ls.async_run( ip, port );

    for( int i = 0; i < 4 && !ls.isRunning(); i++ )
        msleep(600);

    REQUIRE( ls.isRunning() );

    msg.str("");
    msg2.str("");

    auto ret1 = std::async(std::launch::async, readlog_thread1); // make_shared<std::thread>(readlog_thread1);
    LogThreadGuard g1(lr1, ret1);

    msleep(500); // пауза чтобы первый заведомо успел подключиться раньше второго..

    auto ret2 = std::async(std::launch::async, readlog_thread2); // make_shared<std::thread>(readlog_thread2);
    LogThreadGuard g2(lr2, ret2);

    msleep(100); // небольшая пауза на создание потока и т.п.

    ostringstream m;
    m << test_msg1 << endl;

    log1->any() << m.str();

    REQUIRE( la_msg.str() == m.str() );

    msleep(readPause); // пауза на переподключение reader-а к серверу..
    {
        std::lock_guard<std::mutex> l(r1_mutex);
        // message contains text
        REQUIRE( msg.str().find(m.str()) != string::npos );
    }

    {
        std::lock_guard<std::mutex> l(r2_mutex);
        /*
            // Ищем часть сообщения об ошибке: '(LOG SERVER): Exceeded the limit on the number of sessions = 1'
            size_t pos = msg2.str().find("Exceeded the limit");
            REQUIRE( pos != std::string::npos );
        */
        // ничего не получили..
        REQUIRE( msg2.str() == "" );
    }
}
// --------------------------------------------------------------------------
TEST_CASE("LogAgregator regexp", "[LogAgregator][regexp]" )
{
    auto la = make_shared<LogAgregator>("ROOT");
    auto log1 = la->create("log1");
    auto log2 = la->create("log2");
    auto log31 = la->create("a3-log1");
    auto log32 = la->create("a3-log2");
    auto log33 = la->create("a3-log3");
    auto log341 = la->create("a3-a4-log1");
    auto log342 = la->create("a3-a4-log2");

    auto lst = la->getLogList(".*a4.*");
    REQUIRE( lst.size() == 2 );

    lst = la->getLogList(".*a3.*");
    REQUIRE( lst.size() == 5 );

    lst = la->getLogList(".*log1.*");
    REQUIRE( lst.size() == 3 );

    lst = la->getLogList("log1");
    REQUIRE( lst.size() == 1 );

    lst = la->getLogList("log2");
    REQUIRE( lst.size() == 1 );

    lst = la->getLogList("a3-log3");
    REQUIRE( lst.size() == 1 );

    lst = la->getLogList("a4-.*");
    REQUIRE( lst.size() == 0 );


    // la/la2/log5
    // la/la2/log6
    // la/la2/la3/log7
    // la/la2/la3/log8
    auto la2 = make_shared<LogAgregator>("la2");
    auto la3 = make_shared<LogAgregator>("la3");
    la2->add(la3);
    la->add(la2);
    auto log5 = la2->create("log5");
    auto log6 = la2->create("log6");
    auto log7 = la3->create("log7");
    auto log8 = la3->create("log8");

    lst = la->getLogList("la2"); // log5,log6 + la3/log7,la3/log8
    REQUIRE( lst.size() == 4 );

    lst = la->getLogList("la3"); // log7,log8
    REQUIRE( lst.size() == 2 );

    lst = la->getLogList("log.*"); // log[125678]
    REQUIRE( lst.size() == 6 );
}

// --------------------------------------------------------------------------
TEST_CASE("LogAgregator findlog", "[LogAgregator][getLog]" )
{
    auto la = make_shared<LogAgregator>("ROOT");
    auto log1 = la->create("log1");
    auto la2 = make_shared<LogAgregator>("la2");
    la->add(la2);
    auto log2 = la2->create("log2");

    auto la3 = make_shared<LogAgregator>("la3");
    auto log3 = la3->create("log3");
    la2->add(la3);

    auto log = la->findByLogName("log1");
    REQUIRE( log != nullptr );
    REQUIRE( log->getLogName() == "log1" );

    log = la->findByLogName("log2");
    REQUIRE( log != nullptr );
    REQUIRE( log->getLogName() == "log2" );

    log = la->findByLogName("log3");
    REQUIRE( log != nullptr );
    REQUIRE( log->getLogName() == "log3" );

    auto agg = la->findByLogName("la2");
    REQUIRE( agg != nullptr );
    REQUIRE( agg->getLogName() == "la2" );

    agg = la->findByLogName("la3");
    REQUIRE( agg != nullptr );
    REQUIRE( agg->getLogName() == "la3" );

}

// --------------------------------------------------------------------------
