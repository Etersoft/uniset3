#ifndef MBTCPTestServer_H_
#define MBTCPTestServer_H_
// -------------------------------------------------------------------------
#include <string>
#include <atomic>
#include <ostream>
#include <unordered_set>
#include "modbus/ModbusTCPServerSlot.h"
// -------------------------------------------------------------------------
/*! Реализация MBTCPTestServer для тестирования */
class MBTCPTestServer
{
    public:
        MBTCPTestServer( const std::unordered_set<uniset3::ModbusRTU::ModbusAddr>& vaddr, const std::string& inetaddr, int port = 502, bool verbose = false );
        ~MBTCPTestServer();

        inline void setVerbose( bool state )
        {
            verbose = state;
        }

        inline void setReply( uint32_t val )
        {
            replyVal = val;
        }

        void execute();    /*!< основной цикл работы */
        void setLog( std::shared_ptr<DebugStream> dlog );

        inline bool isRunning()
        {
            return ( sslot && sslot->isActive() );
        }

        inline void disableExchange( bool set = true )
        {
            disabled = set;
        }

        inline bool getForceSingleCoilCmd()
        {
            return forceSingleCoilCmd;
        }
        inline int16_t getLastWriteRegister( uint16_t reg )
        {
            return lastWriteRegister[reg];
        }
        inline uniset3::ModbusRTU::ForceCoilsMessage getLastForceCoilsQ()
        {
            return lastForceCoilsQ;
        }
        inline uniset3::ModbusRTU::WriteOutputMessage getLastWriteOutput()
        {
            return lastWriteOutputQ;
        }

        friend std::ostream& operator<<(std::ostream& os, const MBTCPTestServer* m );

        inline float getF2TestValue()
        {
            return f2_test_value;
        }

    protected:
        // действия при завершении работы
        void sigterm( int signo );

        /*! обработка 0x01 */
        uniset3::ModbusRTU::mbErrCode readCoilStatus( uniset3::ModbusRTU::ReadCoilMessage& query,
                uniset3::ModbusRTU::ReadCoilRetMessage& reply );
        /*! обработка 0x02 */
        uniset3::ModbusRTU::mbErrCode readInputStatus( uniset3::ModbusRTU::ReadInputStatusMessage& query,
                uniset3::ModbusRTU::ReadInputStatusRetMessage& reply );

        /*! обработка 0x03 */
        uniset3::ModbusRTU::mbErrCode readOutputRegisters( uniset3::ModbusRTU::ReadOutputMessage& query,
                uniset3::ModbusRTU::ReadOutputRetMessage& reply );

        /*! обработка 0x04 */
        uniset3::ModbusRTU::mbErrCode readInputRegisters( uniset3::ModbusRTU::ReadInputMessage& query,
                uniset3::ModbusRTU::ReadInputRetMessage& reply );

        /*! обработка 0x05 */
        uniset3::ModbusRTU::mbErrCode forceSingleCoil( uniset3::ModbusRTU::ForceSingleCoilMessage& query,
                uniset3::ModbusRTU::ForceSingleCoilRetMessage& reply );

        /*! обработка 0x0F */
        uniset3::ModbusRTU::mbErrCode forceMultipleCoils( uniset3::ModbusRTU::ForceCoilsMessage& query,
                uniset3::ModbusRTU::ForceCoilsRetMessage& reply );


        /*! обработка 0x10 */
        uniset3::ModbusRTU::mbErrCode writeOutputRegisters( uniset3::ModbusRTU::WriteOutputMessage& query,
                uniset3::ModbusRTU::WriteOutputRetMessage& reply );

        /*! обработка 0x06 */
        uniset3::ModbusRTU::mbErrCode writeOutputSingleRegister( uniset3::ModbusRTU::WriteSingleOutputMessage& query,
                uniset3::ModbusRTU::WriteSingleOutputRetMessage& reply );


        uniset3::ModbusRTU::mbErrCode diagnostics( uniset3::ModbusRTU::DiagnosticMessage& query,
                uniset3::ModbusRTU::DiagnosticRetMessage& reply );

        uniset3::ModbusRTU::mbErrCode read4314( uniset3::ModbusRTU::MEIMessageRDI& query,
                                                uniset3::ModbusRTU::MEIMessageRetRDI& reply );

        /*! обработка запросов на чтение ошибок */
        uniset3::ModbusRTU::mbErrCode journalCommand( uniset3::ModbusRTU::JournalCommandMessage& query,
                uniset3::ModbusRTU::JournalCommandRetMessage& reply );

        /*! обработка запроса на установку времени */
        uniset3::ModbusRTU::mbErrCode setDateTime( uniset3::ModbusRTU::SetDateTimeMessage& query,
                uniset3::ModbusRTU::SetDateTimeRetMessage& reply );

        /*! обработка запроса удалённого сервиса */
        uniset3::ModbusRTU::mbErrCode remoteService( uniset3::ModbusRTU::RemoteServiceMessage& query,
                uniset3::ModbusRTU::RemoteServiceRetMessage& reply );

        uniset3::ModbusRTU::mbErrCode fileTransfer( uniset3::ModbusRTU::FileTransferMessage& query,
                uniset3::ModbusRTU::FileTransferRetMessage& reply );


        /*! интерфейс ModbusSlave для обмена по RS */
        uniset3::ModbusTCPServerSlot* sslot;
        std::unordered_set<uniset3::ModbusRTU::ModbusAddr> vaddr; /*!< адреса данного узла */

        bool verbose;
        uint32_t replyVal;
        bool forceSingleCoilCmd;
        std::unordered_map<int16_t, int16_t> lastWriteRegister;
        uniset3::ModbusRTU::ForceCoilsMessage lastForceCoilsQ;
        uniset3::ModbusRTU::WriteOutputMessage lastWriteOutputQ;
        float f2_test_value = {0.0};

#if 0
        typedef std::map<uniset3::ModbusRTU::mbErrCode, unsigned int> ExchangeErrorMap;
        ExchangeErrorMap errmap;     /*!< статистика обмена */
        uniset3::ModbusRTU::mbErrCode prev;


        // можно было бы сделать unsigned, но аналоговые датчики у нас имеют
        // тип long. А это число передаётся в графику в виде аналогового датчика
        long askCount;    /*!< количество принятых запросов */


        typedef std::map<int, std::string> FileList;
        FileList flist;
#endif

    private:
        bool disabled;
        std::string myname;
};
// -------------------------------------------------------------------------
#endif // MBTCPTestServer_H_
// -------------------------------------------------------------------------
