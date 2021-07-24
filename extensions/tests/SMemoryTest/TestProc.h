// -----------------------------------------------------------------------------
#ifndef TestProc_H_
#define TestProc_H_
// -----------------------------------------------------------------------------
#include <vector>
#include "Debug.h"
#include "TestProc_SK.h"
// -----------------------------------------------------------------------------
class TestProc:
    public TestProc_SK
{
    public:
        TestProc( uniset3::ObjectId id, xmlNode* confnode = uniset3::uniset_conf()->getNode("TestProc") );
        virtual ~TestProc();

    protected:
        TestProc();

        enum Timers
        {
            tmChange,
            tmCheckWorking,
            tmCheck,
            tmLogControl
        };

        virtual void step() override;
        virtual void sensorInfo( const uniset3::messages::SensorMessage* sm ) override;
        virtual void timerInfo( const uniset3::messages::TimerMessage* tm ) override;
        virtual void sysCommand( const uniset3::messages::SystemMessage* sm ) override;
        virtual std::string getMonitInfo() const override;

        void test_depend();
        void test_undefined_state();
        void test_thresholds();
        void test_loglevel();

    private:
        bool state = { false };
        bool undef = { false };

        std::vector<Debug::type> loglevels;
        std::vector<Debug::type>::iterator lit;
};
// -----------------------------------------------------------------------------
#endif // TestProc_H_
// -----------------------------------------------------------------------------
