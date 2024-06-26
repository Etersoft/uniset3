#ifndef MBTCPTestServer_H_
#define MBTCPTestServer_H_
// -------------------------------------------------------------------------
#include <string>
#include <atomic>
#include <ostream>
#include <unordered_set>
#include "modbus/ModbusTCPServerSlot.h"
#include "VTypes.h"
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
            return lastWriteRegister[reg].value;
        }
        inline float getLastWriteRegisterF2( uint16_t reg )
        {
            uniset3::ModbusRTU::ModbusData data[] = {(uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                     (uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                    };
            uniset3::VTypes::F2 f2(data, uniset3::VTypes::F2::wsize());
            return (float)f2;
        }
        inline uint16_t getLastWriteRegisterByte( uint16_t reg )
        {
            uniset3::VTypes::Byte b((uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg].value);
            return (uint16_t)b;
        }
        inline float getLastWriteRegisterF2r( uint16_t reg )
        {
            uniset3::ModbusRTU::ModbusData data[] = {(uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                     (uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                    };
            uniset3::VTypes::F2r f2r(data, uniset3::VTypes::F2r::wsize());
            return (float)f2r;
        }
        inline double getLastWriteRegisterF4( uint16_t reg )
        {
            uniset3::ModbusRTU::ModbusData data[] = {(uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                     (uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value,
                                                     (uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg + 2].value,
                                                     (uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg + 3].value
                                                    };
            uniset3::VTypes::F4 f4(data, uniset3::VTypes::F4::wsize());
            return (double)f4;
        }
        inline int32_t getLastWriteRegisterI2( uint16_t reg )
        {
            uniset3::ModbusRTU::ModbusData data[] = {(uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                     (uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                    };
            uniset3::VTypes::I2 i2(data, uniset3::VTypes::I2::wsize());
            return (int32_t)i2;
        }
        inline int32_t getLastWriteRegisterI2r( uint16_t reg )
        {
            uniset3::ModbusRTU::ModbusData data[] = {(uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                     (uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                    };
            uniset3::VTypes::I2r i2r(data, uniset3::VTypes::I2r::wsize());
            return (int32_t)i2r;
        }
        inline uint32_t getLastWriteRegisterU2( uint16_t reg )
        {
            uniset3::ModbusRTU::ModbusData data[] = {(uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                     (uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                    };
            uniset3::VTypes::U2 u2(data, uniset3::VTypes::U2::wsize());
            return (uint32_t)u2;
        }
        inline uint32_t getLastWriteRegisterU2r( uint16_t reg )
        {
            uniset3::ModbusRTU::ModbusData data[] = {(uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg].value,
                                                     (uniset3::ModbusRTU::ModbusData)lastWriteRegister[reg + 1].value
                                                    };
            uniset3::VTypes::U2r u2r(data, uniset3::VTypes::U2r::wsize());
            return (uint32_t)u2r;
        }
        inline long getWriteRegisterCount( uint16_t reg )
        {
            return lastWriteRegister[reg].count;
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
        uniset3::ModbusRTU::mbErrCode readCoilStatus( const uniset3::ModbusRTU::ReadCoilMessage& query,
                uniset3::ModbusRTU::ReadCoilRetMessage& reply );
        /*! обработка 0x02 */
        uniset3::ModbusRTU::mbErrCode readInputStatus( const uniset3::ModbusRTU::ReadInputStatusMessage& query,
                uniset3::ModbusRTU::ReadInputStatusRetMessage& reply );

        /*! обработка 0x03 */
        uniset3::ModbusRTU::mbErrCode readOutputRegisters( const uniset3::ModbusRTU::ReadOutputMessage& query,
                uniset3::ModbusRTU::ReadOutputRetMessage& reply );

        /*! обработка 0x04 */
        uniset3::ModbusRTU::mbErrCode readInputRegisters( const uniset3::ModbusRTU::ReadInputMessage& query,
                uniset3::ModbusRTU::ReadInputRetMessage& reply );

        /*! обработка 0x05 */
        uniset3::ModbusRTU::mbErrCode forceSingleCoil( const uniset3::ModbusRTU::ForceSingleCoilMessage& query,
                uniset3::ModbusRTU::ForceSingleCoilRetMessage& reply );

        /*! обработка 0x0F */
        uniset3::ModbusRTU::mbErrCode forceMultipleCoils( const uniset3::ModbusRTU::ForceCoilsMessage& query,
                uniset3::ModbusRTU::ForceCoilsRetMessage& reply );


        /*! обработка 0x10 */
        uniset3::ModbusRTU::mbErrCode writeOutputRegisters( const uniset3::ModbusRTU::WriteOutputMessage& query,
                uniset3::ModbusRTU::WriteOutputRetMessage& reply );

        /*! обработка 0x06 */
        uniset3::ModbusRTU::mbErrCode writeOutputSingleRegister( const uniset3::ModbusRTU::WriteSingleOutputMessage& query,
                uniset3::ModbusRTU::WriteSingleOutputRetMessage& reply );


        uniset3::ModbusRTU::mbErrCode diagnostics( const uniset3::ModbusRTU::DiagnosticMessage& query,
                uniset3::ModbusRTU::DiagnosticRetMessage& reply );

        uniset3::ModbusRTU::mbErrCode read4314( const uniset3::ModbusRTU::MEIMessageRDI& query,
                                               uniset3::ModbusRTU::MEIMessageRetRDI& reply );

        /*! обработка запросов на чтение ошибок */
        uniset3::ModbusRTU::mbErrCode journalCommand( const uniset3::ModbusRTU::JournalCommandMessage& query,
                uniset3::ModbusRTU::JournalCommandRetMessage& reply );

        /*! обработка запроса на установку времени */
        uniset3::ModbusRTU::mbErrCode setDateTime( const uniset3::ModbusRTU::SetDateTimeMessage& query,
                uniset3::ModbusRTU::SetDateTimeRetMessage& reply );

        /*! обработка запроса удалённого сервиса */
        uniset3::ModbusRTU::mbErrCode remoteService( const uniset3::ModbusRTU::RemoteServiceMessage& query,
                uniset3::ModbusRTU::RemoteServiceRetMessage& reply );

        uniset3::ModbusRTU::mbErrCode fileTransfer( const uniset3::ModbusRTU::FileTransferMessage& query,
                uniset3::ModbusRTU::FileTransferRetMessage& reply );

        /*! интерфейс ModbusSlave для обмена по RS */
        uniset3::ModbusTCPServerSlot* sslot;
        std::unordered_set<uniset3::ModbusRTU::ModbusAddr> vaddr; /*!< адреса данного узла */

        bool verbose;
        uint32_t replyVal;
        bool forceSingleCoilCmd;
        struct regData
        {
            int16_t value = {0};
            long count = {0};
            void setValue(int16_t val)
            {
                value = val;
                count++;
            }
        };
        std::unordered_map<int16_t, regData> lastWriteRegister;
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
