#ifndef MBTCPServer_H_
#define MBTCPServer_H_
// -------------------------------------------------------------------------
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <random>
#include "modbus/ModbusTCPServerSlot.h"

// -------------------------------------------------------------------------
/*! Ничего не делающая реализация MBTCPServer для тестирования */
class MBTCPServer
{
    public:
        MBTCPServer( const std::unordered_set<uniset3::ModbusRTU::ModbusAddr>& myaddr, const std::string& inetaddr, int port = 502, bool verbose = false );
        ~MBTCPServer();

        inline void setVerbose( bool state )
        {
            verbose = state;
        }

        inline void setReply( long val )
        {
            replyVal = val;
        }

        void setRandomReply( long min, long max );

        inline uniset3::timeout_t setAfterSendPause( uniset3::timeout_t msec )
        {
            return sslot->setAfterSendPause(msec);
        }

        void execute();    /*!< основной цикл работы */
        void setLog( std::shared_ptr<DebugStream>& dlog );

        void setMaxSessions( size_t max );

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

        bool verbose = { false };
        long replyVal = { -1 };
        std::unordered_map<uint16_t, uint16_t> reglist = {};
        std::random_device rnd;
        std::unique_ptr<std::mt19937> gen = { nullptr };
        std::unique_ptr<std::uniform_int_distribution<>> rndgen = { nullptr };
#if 0
        typedef std::unordered_map<uniset3::ModbusRTU::mbErrCode, unsigned int> ExchangeErrorMap;
        ExchangeErrorMap errmap;     /*!< статистика обмена */
        uniset3::ModbusRTU::mbErrCode prev;


        // можно было бы сделать unsigned, но аналоговые датчики у нас имеют
        // тип long. А это число передаётся в графику в виде аналогового датчика
        long askCount;    /*!< количество принятых запросов */


        typedef std::unordered_map<int, std::string> FileList;
        FileList flist;
#endif

    private:

};
// -------------------------------------------------------------------------
#endif // MBTCPServer_H_
// -------------------------------------------------------------------------
