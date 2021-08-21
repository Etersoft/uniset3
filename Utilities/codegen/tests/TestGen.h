// -----------------------------------------------------------------------------
#ifndef TestGen_H_
#define TestGen_H_
// -----------------------------------------------------------------------------
#include "TestGen_SK.h"
// -----------------------------------------------------------------------------
class TestGen:
    public TestGen_SK
{
    public:
        TestGen( uniset3::ObjectId id, xmlNode* confnode = uniset3::uniset_conf()->getNode("TestGen") );
        virtual ~TestGen();

    protected:
        TestGen();

        virtual void step() override;
        virtual void sensorInfo( const uniset3::umessage::SensorMessage* sm ) override;
        virtual void timerInfo( const uniset3::umessage::TimerMessage* tm ) override;
        virtual void sysCommand( const uniset3::umessage::SystemMessage* sm ) override;
    private:
        bool bool_var = { false };
        int int_var = {0};
        uniset3::timeout_t t_val = { 0 };
};
// -----------------------------------------------------------------------------
#endif // TestGen_H_
// -----------------------------------------------------------------------------
