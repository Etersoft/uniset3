/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// -------------------------------------------------------------------------
#include "modbus/ModbusTCPServerSlot.h"
// -------------------------------------------------------------------------
namespace uniset3
{
    // -------------------------------------------------------------------------
    using namespace ModbusRTU;
    using namespace std;
    // -------------------------------------------------------------------------
    ModbusTCPServerSlot::ModbusTCPServerSlot(const string& ia, int port ):
        ModbusTCPServer(ia, port)
    {
    }
    // -------------------------------------------------------------------------
    ModbusTCPServerSlot::~ModbusTCPServerSlot()
    {

    }
    // -------------------------------------------------------------------------
    mbErrCode ModbusTCPServerSlot::readCoilStatus( const ReadCoilMessage& query,
            ReadCoilRetMessage& reply )
    {
        if( !slReadCoil )
            return erOperationFailed;

        return slReadCoil(query, reply);
    }

    // -------------------------------------------------------------------------
    mbErrCode ModbusTCPServerSlot::readInputStatus( const ReadInputStatusMessage& query,
            ReadInputStatusRetMessage& reply )
    {
        if( !slReadInputStatus )
            return erOperationFailed;

        return slReadInputStatus(query, reply);
    }

    // -------------------------------------------------------------------------

    mbErrCode ModbusTCPServerSlot::readOutputRegisters( const ReadOutputMessage& query,
            ReadOutputRetMessage& reply )
    {
        if( !slReadOutputs )
            return erOperationFailed;

        return slReadOutputs(query, reply);
    }

    // -------------------------------------------------------------------------
    mbErrCode ModbusTCPServerSlot::readInputRegisters( const ReadInputMessage& query,
            ReadInputRetMessage& reply )
    {
        if( !slReadInputs )
            return erOperationFailed;

        return slReadInputs(query, reply);
    }

    // -------------------------------------------------------------------------
    mbErrCode ModbusTCPServerSlot::forceMultipleCoils( const ForceCoilsMessage& query,
            ForceCoilsRetMessage& reply )
    {
        if( !slForceCoils )
            return erOperationFailed;

        return slForceCoils(query, reply);
    }

    // -------------------------------------------------------------------------

    mbErrCode ModbusTCPServerSlot::writeOutputRegisters( const WriteOutputMessage& query,
            WriteOutputRetMessage& reply )
    {
        if( !slWriteOutputs )
            return erOperationFailed;

        return slWriteOutputs(query, reply);
    }

    // -------------------------------------------------------------------------
    mbErrCode ModbusTCPServerSlot::diagnostics( const DiagnosticMessage& query,
            DiagnosticRetMessage& reply )
    {
        if( !slDiagnostics )
            return erOperationFailed;

        return slDiagnostics(query, reply);
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode ModbusTCPServerSlot::read4314( const ModbusRTU::MEIMessageRDI& query,
            ModbusRTU::MEIMessageRetRDI& reply )
    {
        if( !slMEIRDI )
            return erOperationFailed;

        return slMEIRDI(query, reply);
    }
    // -------------------------------------------------------------------------
    mbErrCode ModbusTCPServerSlot::forceSingleCoil( const ForceSingleCoilMessage& query,
            ForceSingleCoilRetMessage& reply )
    {
        if( !slForceSingleCoil )
            return erOperationFailed;

        return slForceSingleCoil(query, reply);
    }

    // -------------------------------------------------------------------------
    mbErrCode ModbusTCPServerSlot::writeOutputSingleRegister( const WriteSingleOutputMessage& query,
            WriteSingleOutputRetMessage& reply )
    {
        if( !slWriteSingleOutputs )
            return erOperationFailed;

        return slWriteSingleOutputs(query, reply);
    }

    // -------------------------------------------------------------------------
    mbErrCode ModbusTCPServerSlot::journalCommand( const JournalCommandMessage& query,
            JournalCommandRetMessage& reply )
    {
        if( !slJournalCommand )
            return erOperationFailed;

        return slJournalCommand(query, reply);
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode ModbusTCPServerSlot::setDateTime( const ModbusRTU::SetDateTimeMessage& query,
            ModbusRTU::SetDateTimeRetMessage& reply )
    {
        if( !slSetDateTime )
            return erOperationFailed;

        return slSetDateTime(query, reply);
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode ModbusTCPServerSlot::remoteService( const ModbusRTU::RemoteServiceMessage& query,
            ModbusRTU::RemoteServiceRetMessage& reply )
    {
        if( !slRemoteService )
            return erOperationFailed;

        return slRemoteService(query, reply);
    }
    // -------------------------------------------------------------------------
    ModbusRTU::mbErrCode ModbusTCPServerSlot::fileTransfer( const ModbusRTU::FileTransferMessage& query,
            ModbusRTU::FileTransferRetMessage& reply )
    {
        if( !slFileTransfer )
            return erOperationFailed;

        return slFileTransfer(query, reply);
    }
    // -------------------------------------------------------------------------
    void ModbusTCPServerSlot::terminate()
    {
        try
        {
            ModbusTCPServer::terminate();
        }
        catch( std::exception& ex ) {}
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset3
