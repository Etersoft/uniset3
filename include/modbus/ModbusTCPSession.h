// -------------------------------------------------------------------------
#ifndef ModbusTCPSession_H_
#define ModbusTCPSession_H_
// -------------------------------------------------------------------------
#include <string>
#include <queue>
#include <unordered_map>
#include <ev++.h>
#include "ModbusServerSlot.h"
#include "ModbusServer.h"
#include "PassiveTimer.h"
#include "UTCPCore.h"
#include "UTCPStream.h"
// -------------------------------------------------------------------------
namespace uniset3
{
    // -------------------------------------------------------------------------
    /*!
     * \brief The ModbusTCPSession class
     * Класс рассчитан на совместную работу с ModbusTCPServer, т.к. построен на основе libev,
     * и главный цикл (default_loop) находиться там.
     *
     * Текущая реализация не доведена до совершенства использования "событий".
     * И рассчитывает, что данные от клиента приходят все сразу, а так как сокеты не блокирующие,
     * то попыток чтения делается несколько с небольшой паузой, что нехорошо, т.к. отнимает время у
     * других "клиентов", ведь сервер по сути однопоточный (!)
     * Альтернативной реализацией могло быть быть.. чтение по событиям и складывание в отдельную очередь,
     * а обработку делать по мере достаточного накопления данных во входной очереди, но это требует асинхронный
     * парсинг данных протокола modbus (т.е. мы анализируем очередной байт и решаем сколько нам нужно ещё
     * "подождать" данных.. чтобы пойти на следующий шаг), это в результате будет слишком сложная реализация.
     * В конце-концов пока нет расчёта на >1000 подключений (хотя libev позволяет держать >10k).
     */
    class ModbusTCPSession:
        public ModbusServerSlot,
        public ModbusServer
    {
        public:

            ModbusTCPSession( const Poco::Net::StreamSocket& s, const std::unordered_set<ModbusRTU::ModbusAddr>& vmbaddr, timeout_t timeout );
            virtual ~ModbusTCPSession();

            void cleanInputStream();

            virtual void cleanupChannel() override;
            virtual void terminate() override;

            typedef sigc::slot<void, const ModbusTCPSession*> FinalSlot;

            void connectFinalSession( FinalSlot sl );

            std::string getClientAddress() const;

            void setSessionTimeout( double t );

            // запуск обработки входящих запросов
            void run( ev::loop_ref& loop );

            virtual bool isActive() const override;

        protected:

            virtual void iowait( timeout_t msec ) override;

            virtual ModbusRTU::mbErrCode realReceive( const std::unordered_set<ModbusRTU::ModbusAddr>& vmbaddr, timeout_t msecTimeout ) override;

            void callback( ev::io& watcher, int revents );
            void onTimeout( ev::timer& watcher, int revents );
            virtual void readEvent( ev::io& watcher );
            virtual void writeEvent( ev::io& watcher );
            virtual void final();

            virtual size_t getNextData( unsigned char* buf, int len ) override;
            virtual void setChannelTimeout( timeout_t msec ) override;
            virtual ModbusRTU::mbErrCode sendData( unsigned char* buf, int len ) override;
            virtual ModbusRTU::mbErrCode tcp_processing( ModbusRTU::MBAPHeader& mhead );
            virtual ModbusRTU::mbErrCode make_adu_header( ModbusRTU::ModbusMessage& request ) override;
            virtual ModbusRTU::mbErrCode post_send_request(ModbusRTU::ModbusMessage& request ) override;

            virtual ModbusRTU::mbErrCode readCoilStatus( const ModbusRTU::ReadCoilMessage& query,
                    ModbusRTU::ReadCoilRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode readInputStatus( const ModbusRTU::ReadInputStatusMessage& query,
                    ModbusRTU::ReadInputStatusRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode readOutputRegisters( const ModbusRTU::ReadOutputMessage& query,
                    ModbusRTU::ReadOutputRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode readInputRegisters( const ModbusRTU::ReadInputMessage& query,
                    ModbusRTU::ReadInputRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode forceSingleCoil( const ModbusRTU::ForceSingleCoilMessage& query,
                    ModbusRTU::ForceSingleCoilRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode writeOutputSingleRegister( const ModbusRTU::WriteSingleOutputMessage& query,
                    ModbusRTU::WriteSingleOutputRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode forceMultipleCoils( const ModbusRTU::ForceCoilsMessage& query,
                    ModbusRTU::ForceCoilsRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode writeOutputRegisters( const ModbusRTU::WriteOutputMessage& query,
                    ModbusRTU::WriteOutputRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode diagnostics( const ModbusRTU::DiagnosticMessage& query,
                    ModbusRTU::DiagnosticRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode read4314( const ModbusRTU::MEIMessageRDI& query,
                                                   ModbusRTU::MEIMessageRetRDI& reply ) override;

            virtual ModbusRTU::mbErrCode journalCommand( const ModbusRTU::JournalCommandMessage& query,
                    ModbusRTU::JournalCommandRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode setDateTime( const ModbusRTU::SetDateTimeMessage& query,
                    ModbusRTU::SetDateTimeRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode remoteService( const ModbusRTU::RemoteServiceMessage& query,
                    ModbusRTU::RemoteServiceRetMessage& reply ) override;

            virtual ModbusRTU::mbErrCode fileTransfer( const ModbusRTU::FileTransferMessage& query,
                    ModbusRTU::FileTransferRetMessage& reply ) override;

        private:
            std::queue<unsigned char> qrecv;
            std::unordered_set<ModbusRTU::ModbusAddr> vaddr;
            ModbusRTU::MBAPHeader curQueryHeader;
            PassiveTimer ptTimeout;
            timeout_t timeout = { 0 };
            ModbusRTU::ModbusMessage buf;

            ev::io  io;
            ev::timer ioTimeout;

            std::shared_ptr<UTCPStream> sock;
            std::queue<UTCPCore::Buffer*> qsend;
            double sessTimeout = { 10.0 };

            bool ignoreAddr = { false };
            std::string peername = { "" };

            std::string caddr = { "" };

            FinalSlot slFin;

            std::atomic_bool cancelled = { false };
            PassiveTimer pt;
            PassiveTimer ptWait;
    };
    // ---------------------------------------------------------------------
} // end of namespace uniset3
// -------------------------------------------------------------------------
#endif // ModbusTCPSession_H_
// -------------------------------------------------------------------------
