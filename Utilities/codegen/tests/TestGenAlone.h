// -----------------------------------------------------------------------------
#ifndef TestGenAlone_H_
#define TestGenAlone_H_
// -----------------------------------------------------------------------------
#include "TestGenAlone_SK.h"
// -----------------------------------------------------------------------------
class TestGenAlone:
    public TestGenAlone_SK
{
    public:
        TestGenAlone( uniset3::ObjectId id, xmlNode* confnode = uniset3::uniset_conf()->getNode("TestGenAlone") );
        virtual ~TestGenAlone();


    protected:
        virtual void step() override;
        void sensorInfo( const uniset3::umessage::SensorMessage* sm ) override;
        void timerInfo( const uniset3::umessage::TimerMessage* tm ) override;

    private:
};
// -----------------------------------------------------------------------------
#endif // TestGenAlone_H_
// -----------------------------------------------------------------------------
