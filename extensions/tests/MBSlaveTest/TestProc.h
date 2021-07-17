// -----------------------------------------------------------------------------
#ifndef TestProc_H_
#define TestProc_H_
// -----------------------------------------------------------------------------
#include <vector>
#include "Debug.h"
#include "TestProc_SK.h"
#include "modbus/ModbusTCPServerSlot.h"
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

        virtual void step();
        virtual void sensorInfo( const uniset3::SensorMessage* sm );
        virtual void timerInfo( const uniset3::TimerMessage* tm );
        virtual void sysCommand( const uniset3::SystemMessage* sm );

        void test_depend();
        void test_undefined_state();
        void test_thresholds();
        void test_loglevel();

    private:
        bool state = { false };
        bool undef = { false };

        std::vector<Debug::type> loglevels;
        std::vector<Debug::type>::iterator lit;

        std::shared_ptr<uniset3::ModbusTCPServerSlot> mbslave;
        /*! обработка 0x06 */
        uniset3::ModbusRTU::mbErrCode writeOutputSingleRegister( uniset3::ModbusRTU::WriteSingleOutputMessage& query,
                uniset3::ModbusRTU::WriteSingleOutputRetMessage& reply );

        std::shared_ptr< uniset3::ThreadCreator<TestProc> > mbthr;
        void mbThread();
};
// -----------------------------------------------------------------------------
#endif // TestProc_H_
// -----------------------------------------------------------------------------
