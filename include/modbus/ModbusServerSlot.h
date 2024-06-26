// -------------------------------------------------------------------------
#ifndef ModbusServerSlot_H_
#define ModbusServerSlot_H_
// -------------------------------------------------------------------------
#include <sigc++/sigc++.h>
#include "ModbusTypes.h"
#include "ModbusServer.h"
// -------------------------------------------------------------------------
namespace uniset3
{
    // -------------------------------------------------------------------------
    /*! */
    class ModbusServerSlot
    {
        public:
            ModbusServerSlot();
            virtual ~ModbusServerSlot();

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::ReadCoilMessage&,
                    ModbusRTU::ReadCoilRetMessage&> ReadCoilSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::ReadInputStatusMessage&,
                    ModbusRTU::ReadInputStatusRetMessage&> ReadInputStatusSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::ReadOutputMessage&,
                    ModbusRTU::ReadOutputRetMessage&> ReadOutputSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::ReadInputMessage&,
                    ModbusRTU::ReadInputRetMessage&> ReadInputSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::ForceSingleCoilMessage&,
                    ModbusRTU::ForceSingleCoilRetMessage&> ForceSingleCoilSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::WriteSingleOutputMessage&,
                    ModbusRTU::WriteSingleOutputRetMessage&> WriteSingleOutputSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::ForceCoilsMessage&,
                    ModbusRTU::ForceCoilsRetMessage&> ForceCoilsSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::WriteOutputMessage&,
                    ModbusRTU::WriteOutputRetMessage&> WriteOutputSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::DiagnosticMessage&,
                    ModbusRTU::DiagnosticRetMessage&> DiagnosticsSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::MEIMessageRDI&,
                    ModbusRTU::MEIMessageRetRDI&> MEIRDISlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::JournalCommandMessage&,
                    ModbusRTU::JournalCommandRetMessage&> JournalCommandSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::SetDateTimeMessage&,
                    ModbusRTU::SetDateTimeRetMessage&> SetDateTimeSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::RemoteServiceMessage&,
                    ModbusRTU::RemoteServiceRetMessage&> RemoteServiceSlot;

            typedef sigc::slot<ModbusRTU::mbErrCode,
                    const ModbusRTU::FileTransferMessage&,
                    ModbusRTU::FileTransferRetMessage&> FileTransferSlot;

            /*! подключение обработчика 'получения данных' 0x01 */
            void connectReadCoil( ReadCoilSlot sl );

            /*! подключение обработчика 'получения данных' 0x02 */
            void connectReadInputStatus( ReadInputStatusSlot sl );

            /*! подключение обработчика 'получения данных' 0x03 */
            void connectReadOutput( ReadOutputSlot sl );

            /*! подключение обработчика 'получения данных' 0x04 */
            void connectReadInput( ReadInputSlot sl );

            /*! подключение обработчика 'записи данных' 0x05 */
            void connectForceSingleCoil( ForceSingleCoilSlot sl );

            /*! подключение обработчика 'записи данных' 0x06 */
            void connectWriteSingleOutput( WriteSingleOutputSlot sl );

            /*! подключение обработчика 'записи данных' 0x08 */
            void connectDiagnostics( DiagnosticsSlot sl );

            /*! подключение обработчика 0x2B(43) */
            void connectMEIRDI( MEIRDISlot sl );

            /*! подключение обработчика 'записи данных' 0x0F */
            void connectForceCoils( ForceCoilsSlot sl );

            /*! подключение обработчика 'записи данных' 0x10 */
            void connectWriteOutput( WriteOutputSlot sl );

            /*! подключение обработчика 'чтение ошибки' 0x65 */
            void connectJournalCommand( JournalCommandSlot sl );

            /*! подключение обработчика 'установка времени' 0x50 */
            void connectSetDateTime( SetDateTimeSlot sl );

            /*! подключение обработчика 'удалённый сервис' 0x53 */
            void connectRemoteService( RemoteServiceSlot sl );

            /*! подключение обработчика 'передача файла' 0x66 */
            void connectFileTransfer( FileTransferSlot sl );

            virtual void terminate() {}

        protected:
            ReadCoilSlot slReadCoil;
            ReadInputStatusSlot slReadInputStatus;
            ReadOutputSlot slReadOutputs;
            ReadInputSlot slReadInputs;
            ForceCoilsSlot slForceCoils;
            WriteOutputSlot slWriteOutputs;
            ForceSingleCoilSlot slForceSingleCoil;
            WriteSingleOutputSlot slWriteSingleOutputs;
            DiagnosticsSlot slDiagnostics;
            MEIRDISlot slMEIRDI;
            JournalCommandSlot slJournalCommand;
            SetDateTimeSlot slSetDateTime;
            RemoteServiceSlot slRemoteService;
            FileTransferSlot slFileTransfer;
    };
    // -------------------------------------------------------------------------
} // end of namespace uniset3
// -------------------------------------------------------------------------
#endif // ModbusServerSlot_H_
// -------------------------------------------------------------------------
