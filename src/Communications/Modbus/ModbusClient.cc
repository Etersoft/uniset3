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
#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include "Exceptions.h"
#include "modbus/ModbusClient.h"
// -------------------------------------------------------------------------
namespace uniset3
{

    // -------------------------------------------------------------------------
    using namespace std;
    using namespace ModbusRTU;
    using namespace uniset3;
    // -------------------------------------------------------------------------
    ModbusClient::ModbusClient():
        replyTimeOut_ms(2000),
        aftersend_msec(0),
        sleepPause_usec(100),
        crcNoCheckit(false),
        sendMutex("ModbusClient_sendMutex")
    {
        dlog = make_shared<DebugStream>();
        tmProcessing.setTiming(replyTimeOut_ms);
    }

    // -------------------------------------------------------------------------
    ModbusClient::~ModbusClient()
    {
    }
    // -------------------------------------------------------------------------
    void ModbusClient::setTimeout( timeout_t msec )
    {
        replyTimeOut_ms = msec;
    }
    // -------------------------------------------------------------------------
    int ModbusClient::setAfterSendPause( timeout_t msec )
    {
        timeout_t old = aftersend_msec;
        aftersend_msec = msec;
        return old;
    }
    // --------------------------------------------------------------------------------
    ReadCoilRetMessage ModbusClient::read01( ModbusAddr addr,
            ModbusData start, ModbusData count )
    {
        ReadCoilMessage::make_to(addr, start, count, qbuf);
        mbErrCode err = query(addr, qbuf, qreply, replyTimeOut_ms);

        if( err == erNoError )
            return ReadCoilRetMessage(qreply);

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------
    ReadInputStatusRetMessage ModbusClient::read02( ModbusAddr addr,
            ModbusData start, ModbusData count )
    {
        ReadInputStatusMessage::make_to(addr, start, count, qbuf);
        mbErrCode err = query(addr, qbuf, qreply, replyTimeOut_ms);

        if( err == erNoError )
            return ReadInputStatusRetMessage(qreply);

        throw mbException(err);
    }

    // --------------------------------------------------------------------------------
    ReadOutputRetMessage ModbusClient::read03( ModbusAddr addr,
            ModbusData start, ModbusData count )
    {
        ReadOutputMessage::make_to(addr, start, count, qbuf);
        mbErrCode err = query(addr, qbuf, qreply, replyTimeOut_ms);

        if( err == erNoError )
        {
            ReadOutputRetMessage ret(qreply);

            if( ret.count != count )
                throw mbException(erBadDataValue);

            return ret;
        }

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------
    ReadInputRetMessage ModbusClient::read04( ModbusAddr addr,
            ModbusData start, ModbusData count )
    {
        ReadInputMessage::make_to(addr, start, count, qbuf);
        mbErrCode err = query(addr, qbuf, qreply, replyTimeOut_ms);

        if( err == erNoError )
        {
            ReadInputRetMessage ret(qreply);

            if( ret.count != count )
                throw mbException(erBadDataValue);

            return ret;
        }

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------
    ForceSingleCoilRetMessage ModbusClient::write05( ModbusAddr addr,
            ModbusData start, bool cmd )
    {
        ForceSingleCoilMessage::make_to(addr, start, cmd, qbuf);
        mbErrCode err = query(addr, qbuf, qreply, replyTimeOut_ms);

        if( err == erNoError )
        {
            ForceSingleCoilRetMessage ret(qreply);

            if( ret.start != start )
                throw mbException(erBadDataValue);

            if( ret.cmd() != cmd )
                throw mbException(erBadDataValue);

            return ret;
        }

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------

    WriteSingleOutputRetMessage ModbusClient::write06( ModbusAddr addr,
            ModbusData start, ModbusData data )
    {
        WriteSingleOutputMessage::make_to(addr, start, data, qbuf);
        mbErrCode err = query(addr, qbuf, qreply, replyTimeOut_ms);

        if( err == erNoError )
        {
            WriteSingleOutputRetMessage ret(qreply);

            if( ret.start != start )
                throw mbException(erBadDataValue);

            if( ret.data != data )
                throw mbException(erBadDataValue);

            return ret;
        }

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------
    ForceCoilsRetMessage ModbusClient::write0F( const ForceCoilsMessage& msg )
    {
        msg.transport_msg_to(qbuf);
        mbErrCode err = query(msg.addr, qbuf, qreply, replyTimeOut_ms);

        if( err == erNoError )
        {
            ForceCoilsRetMessage ret(qreply);

            if( ret.start != msg.start )
                throw mbException(erBadDataValue);

            if( ret.quant != msg.quant )
                throw mbException(erBadDataValue);

            return ret;
        }

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------

    WriteOutputRetMessage ModbusClient::write10( const WriteOutputMessage& msg )
    {
        msg.transport_msg_to(qbuf);
        mbErrCode err = query(msg.addr, qbuf, qreply, replyTimeOut_ms);

        if( err == erNoError )
        {
            WriteOutputRetMessage ret(qreply);

            if( ret.start != msg.start )
                throw mbException(erBadDataValue);

            if( ret.quant != msg.quant )
                throw mbException(erBadDataValue);

            return ret;
        }

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------
    DiagnosticRetMessage ModbusClient::diag08( ModbusAddr addr,
            const DiagnosticsSubFunction subfunc,
            ModbusRTU::ModbusData dat )
    {
        const DiagnosticMessage msg(addr, subfunc, dat);
        msg.transport_msg_to(qbuf);
        mbErrCode err = query(msg.addr, qbuf, qreply, replyTimeOut_ms);

        if( err == erNoError )
        {
            DiagnosticRetMessage ret(qreply);

            if( ret.subf != subfunc )
                throw mbException(erBadDataValue);

            return ret;
        }

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------
    ModbusRTU::MEIMessageRetRDI ModbusClient::read4314( ModbusRTU::ModbusAddr addr,
            const ModbusRTU::ModbusByte devID,
            ModbusRTU::ModbusByte objID )
    {
        MEIMessageRDI::make_to(addr, devID, objID, qbuf);
        mbErrCode err = query(addr, qbuf, qreply, replyTimeOut_ms);

        if( err == erNoError )
        {
            MEIMessageRetRDI ret(qreply);

            if( ret.devID != devID )
                throw mbException(erBadDataValue);

            if( ret.objID != objID )
                throw mbException(erBadDataValue);

            return ret;
        }

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------
    SetDateTimeRetMessage ModbusClient::setDateTime( ModbusAddr addr, ModbusByte hour, ModbusByte min, ModbusByte sec,
            ModbusByte day, ModbusByte mon, ModbusByte year,
            ModbusByte century )
    {
        SetDateTimeMessage msg(addr);
        msg.hour     = hour;
        msg.min     = min;
        msg.sec     = sec;
        msg.day     = day;
        msg.mon     = mon;
        msg.year     = year;
        msg.century = century;
        msg.transport_msg_to(qbuf);

        mbErrCode err = query(addr, qbuf, qreply, replyTimeOut_ms);

        if( err == erNoError )
            return SetDateTimeRetMessage(qreply);

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------
    void ModbusClient::fileTransfer(ModbusAddr addr, ModbusData numfile,
                                    const std::string& save2filename, timeout_t part_timeout_msec )
    {
        //#warning Необходимо реализовать
        //    throw mbException(erUnExpectedPacketType);
        mbErrCode err = erNoError;

        FILE* fdsave = fopen(save2filename.c_str(), "w");

        if( fdsave == NULL )
        {
            if( dlog->is_warn() )
                dlog->warn() << "(fileTransfer): fopen '"
                             << save2filename << "' with error: "
                             << strerror(errno) << endl;

            throw mbException(erHardwareError);
        }

        uint16_t maxpackets = 65535;
        uint16_t curpack = 0;

        PassiveTimer ptTimeout(part_timeout_msec);

        while( curpack < maxpackets && !ptTimeout.checkTime() )
        {
            try
            {
                FileTransferRetMessage ret = partOfFileTransfer( addr, numfile, curpack, part_timeout_msec );

                if( ret.numfile != numfile )
                {
                    if( dlog->is_warn() )
                        dlog->warn() << "(fileTransfer): recv nfile=" << ret.numfile
                                     << " !=numfile(" << numfile << ")" << endl;

                    continue;
                }

                if( ret.packet != curpack )
                {
                    if( dlog->is_warn() )
                        dlog->warn() << "(fileTransfer): recv npack=" << ret.packet
                                     << " !=curpack(" << curpack << ")" << endl;

                    continue;
                }

                maxpackets = ret.numpacks;

                if( dlog->is_info() )
                    dlog->info() << "(fileTransfer): maxpackets="
                                 << ret.numpacks << " curpack=" << curpack + 1 << endl;

                // save data...
                if( fwrite(&ret.data, ret.dlen, 1, fdsave) <= 0 )
                {
                    if( dlog->is_warn() )
                        dlog->warn() << "(fileTransfer): fwrite '"
                                     << save2filename << "' with error: "
                                     << strerror(errno) << endl;

                    err = erHardwareError;
                    break;
                }

                ptTimeout.reset();
                curpack = ret.packet + 1; // curpack++;
            }
            catch( mbException& ex )
            {
                if( ex.err == erBadCheckSum )
                    continue;

                err = ex.err;
                break;
            }
        }

        fclose(fdsave);

        if( curpack == maxpackets )
            return;

        if( err == erNoError )
            err = erTimeOut;

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------
    FileTransferRetMessage ModbusClient::partOfFileTransfer( ModbusAddr addr,
            ModbusData idFile, ModbusData numpack,
            timeout_t part_timeout_msec )
    {
        FileTransferMessage::make_to(addr, idFile, numpack, qbuf);
        mbErrCode err = query(addr, qbuf, qreply, part_timeout_msec);

        if( err == erNoError )
            return FileTransferRetMessage(qreply);

        throw mbException(err);
    }
    // --------------------------------------------------------------------------------
    mbErrCode ModbusClient::recv( ModbusAddr addr, ModbusByte qfunc,
                                  ModbusMessage& rbuf, timeout_t timeout )
    {
        if( timeout == UniSetTimer::WaitUpTime )
            timeout = 15 * 60 * 1000 * 1000; // используем просто большое время (15 минут). Переведя его в наносекунды.

        setChannelTimeout(timeout);
        PassiveTimer tmAbort(timeout);

        size_t bcnt = 0;  // receive bytes count

        try
        {
            bool begin = false;

            while( !tmAbort.checkTime() )
            {
                bcnt = getNextData((unsigned char*)(&rbuf.pduhead.addr), sizeof(rbuf.pduhead.addr));

                if( bcnt > 0 && ( rbuf.addr() == addr ) ) // || (onBroadcast && rbuf.addr==BroadcastAddr) ) )
                {
                    begin = true;
                    break;
                }

                std::this_thread::sleep_for(std::chrono::microseconds(sleepPause_usec));
            }

            if( !begin )
                return erTimeOut;

            /*! \todo Подумать Может стоит всё-таки получать весь пакет, а проверять кому он адресован на уровне выше?!
                        // Lav: конечно стоит, нам же надо буфер чистить
            */
            // Проверка кому адресован пакет...
            if( rbuf.addr() != addr && rbuf.addr() != BroadcastAddr )
            {
                ostringstream err;
                err << "(recv): BadNodeAddress. my= " << addr2str(addr)
                    << " msg.addr=" << addr2str(rbuf.addr());

                if( dlog->is_warn() )
                    dlog->warn() << err.str() << endl;

                cleanupChannel();
                return erBadReplyNodeAddress;
            }

            return recv_pdu(qfunc, rbuf, timeout);
        }
        catch( const uniset3::TimeOut& ex )
        {
            //        cout << "(recv): catch TimeOut " << endl;
        }
        catch( const uniset3::CommFailed& ex )
        {
            if( dlog->is_crit() )
                dlog->crit() << "(recv): " << ex << endl;

            cleanupChannel();
            return erTimeOut;
        }
        catch( const uniset3::Exception& ex ) // SystemError
        {
            if( dlog->is_crit() )
                dlog->crit() << "(recv): " << ex << endl;

            cleanupChannel();
            return erHardwareError;
        }

        return erTimeOut;

    }

    // --------------------------------------------------------------------------------
    mbErrCode ModbusClient::recv_pdu( ModbusByte qfunc, ModbusMessage& rbuf, timeout_t timeout )
    {
        size_t bcnt = 1; // receive bytes count (1 - addr)

        try
        {
            // -----------------------------------
            tmProcessing.setTiming(replyTimeOut_ms);
            // recv func number
            size_t k = getNextData((unsigned char*)(&rbuf.pduhead.func), sizeof(rbuf.pduhead.func));

            if( k < sizeof(rbuf.pduhead.func) )
            {
                if( dlog->is_warn() )
                    dlog->warn() << "(recv): receive " << k << " bytes < " << sizeof(rbuf.pduhead.func) << endl;

                cleanupChannel();
                return erInvalidFormat;
            }

            bcnt += k;
            rbuf.dlen = 0;

            if( dlog->is_level9() )
                dlog->level9() << "(recv): PDU: " << rbuf.pduhead << endl;

            // обработка сообщения об ошибке...
            if( rbuf.func() == (qfunc | MBErrMask) )
            {
                rbuf.dlen = ErrorRetMessage::szData();

                if( crcNoCheckit )
                    rbuf.dlen -= szCRC;

                size_t rlen = getNextData((unsigned char*)(&(rbuf.data)), rbuf.dlen);

                if( rlen < rbuf.dlen )
                {
                    if( dlog->is_warn() )
                    {
                        dlog->warn() << "(recv:Error): buf: " << rbuf << endl;
                        dlog->warn() << "(recv:Error)(" << rbuf.func()
                                     << "): Получили данных меньше чем ждали...(recv="
                                     << rlen << " < wait=" << rbuf.dlen << ")" << endl;
                    }

                    cleanupChannel();
                    return erInvalidFormat;
                }

                bcnt += rlen;

                ErrorRetMessage em(rbuf);

                if( !crcNoCheckit )
                {
                    ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                    if( tcrc != em.crc )
                    {
                        ostringstream err;
                        err << "(recv:error): bad crc. calc.crc=" << dat2str(tcrc)
                            << " msg.crc=" << dat2str(em.crc);

                        if( dlog->is_warn() )
                            dlog->warn() << err.str() << endl;

                        return erBadCheckSum;
                    }
                }

                return (mbErrCode)em.ecode;
            }

            if( qfunc != rbuf.func() )
            {
                cleanupChannel();
                return erUnExpectedPacketType;
            }

            // Определяем тип сообщения
            switch( rbuf.func() )
            {
                case fnReadCoilStatus:
                    rbuf.dlen = ReadCoilRetMessage::szHead();
                    break;

                case fnReadInputStatus:
                    rbuf.dlen = ReadInputStatusRetMessage::szHead();
                    break;

                case fnReadOutputRegisters:
                    rbuf.dlen = ReadOutputRetMessage::szHead();
                    break;

                case fnReadInputRegisters:
                    rbuf.dlen = ReadInputRetMessage::szHead();
                    break;

                case fnForceMultipleCoils:
                    rbuf.dlen = ForceCoilsRetMessage::szData();

                    if( crcNoCheckit )
                        rbuf.dlen -= szCRC;

                    break;

                case fnWriteOutputRegisters:
                    rbuf.dlen = WriteOutputRetMessage::szData();

                    if( crcNoCheckit )
                        rbuf.dlen -= szCRC;

                    break;

                case fnWriteOutputSingleRegister:
                    rbuf.dlen = WriteSingleOutputRetMessage::szData();

                    if( crcNoCheckit )
                        rbuf.dlen -= szCRC;

                    break;

                case fnForceSingleCoil:
                    rbuf.dlen = ForceSingleCoilRetMessage::szData();

                    if( crcNoCheckit )
                        rbuf.dlen -= szCRC;

                    break;

                case fnDiagnostics:
                    rbuf.dlen = DiagnosticRetMessage::szHead();
                    break;

                case fnMEI:
                    rbuf.dlen = MEIMessageRetRDI::szHead();
                    break;

                case fnSetDateTime:
                    rbuf.dlen = SetDateTimeRetMessage::szData();

                    if( crcNoCheckit )
                        rbuf.dlen -= szCRC;

                    break;

                case fnFileTransfer:
                    rbuf.dlen = FileTransferRetMessage::szHead();
                    break;

                /*
                            case fnJournalCommand:
                                rbuf.len = JournalCommandmessages::szData();
                            break;

                            case fnRemoteService:
                                rbuf.len = RemoteServicemessages::szHead();
                            break;
                */
                default:
                    cleanupChannel();
                    return erUnExpectedPacketType;
            }

            // Получаем остальную часть сообщения
            size_t rlen = getNextData((unsigned char*)(rbuf.data), rbuf.dlen);

            if( rlen < rbuf.dlen )
            {
                //            rbuf.len = bcnt + rlen - szModbusHeader;
                if( dlog->is_warn() )
                {
                    dlog->warn() << "(recv): buf: " << rbuf << endl;
                    dlog->warn() << "(recv)(" << rbuf.func()
                                 << "): Получили данных меньше чем ждали...(recv="
                                 << rlen << " < wait=" << rbuf.dlen << ")" << endl;
                }

                cleanupChannel();
                return erInvalidFormat;
            }

            bcnt += rlen;

            // получаем остальное...
            if( rbuf.func() == fnReadCoilStatus )
            {
                int szDataLen = ReadCoilRetMessage::getDataLen(rbuf) + szCRC;

                if( crcNoCheckit )
                    szDataLen -= szCRC;

                // Мы получили только предварительный заголовок
                // Теперь необходимо дополучить данные
                // (c позиции rlen, т.к. часть уже получили)
                int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

                if( rlen1 < szDataLen )
                {
                    rbuf.dlen = bcnt + rlen1 - szModbusHeader;

                    if( dlog->is_warn() )
                    {
                        dlog->warn() << "(0x01): buf: " << rbuf << endl;
                        dlog->warn() << "(0x01)("
                                     << (int)rbuf.func() << "):(fnReadCoilStatus) "
                                     << "Получили данных меньше чем ждали...("
                                     << rlen1 << " < " << szDataLen << ")" << endl;
                    }

                    cleanupChannel();
                    return erInvalidFormat;
                }

                bcnt += rlen1;
                rbuf.dlen = bcnt - szModbusHeader;

                auto mcrc = ReadCoilRetMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(0x01)(fnReadCoilStatus): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(0x01)(fnReadCoilStatus): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);

                    if( dlog->is_warn() )
                        dlog->warn() << err.str() << endl;

                    return erBadCheckSum;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnReadInputStatus )
            {
                int szDataLen = ReadInputStatusRetMessage::getDataLen(rbuf) + szCRC;

                if( crcNoCheckit )
                    szDataLen -= szCRC;

                // Мы получили только предварительный загловок
                // Теперь необходимо дополучить данные
                // (c позиции rlen, т.к. часть уже получили)
                int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

                if( rlen1 < szDataLen )
                {
                    rbuf.dlen = bcnt + rlen1 - szModbusHeader;

                    if( dlog->is_warn() )
                    {
                        dlog->warn() << "(0x02): buf: " << rbuf << endl;
                        dlog->warn() << "(0x02)("
                                     << (int)rbuf.func() << "):(fnReadInputStatus) "
                                     << "Получили данных меньше чем ждали...("
                                     << rlen1 << " < " << szDataLen << ")" << endl;
                    }

                    cleanupChannel();
                    return erInvalidFormat;
                }

                bcnt += rlen1;
                rbuf.dlen = bcnt - szModbusHeader;

                auto mcrc = ReadInputStatusRetMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(0x02)(fnReadInputStatus): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(recv:fnReadInputStatus): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);

                    if( dlog->is_warn() )
                        dlog->warn() << err.str() << endl;

                    return erBadCheckSum;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnReadInputRegisters )
            {
                int szDataLen = ReadInputRetMessage::getDataLen(rbuf) + szCRC;

                if( crcNoCheckit )
                    szDataLen -= szCRC;

                // Мы получили только предварительный загловок
                // Теперь необходимо дополучить данные
                // (c позиции rlen, т.к. часть уже получили)
                int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

                if( rlen1 < szDataLen )
                {
                    rbuf.dlen = bcnt + rlen1 - szModbusHeader;

                    if( dlog->is_warn() )
                    {
                        dlog->warn() << "(0x04): buf: " << rbuf << endl;
                        dlog->warn() << "(0x04)("
                                     << (int)rbuf.func() << "):(fnReadInputRegisters) "
                                     << "Получили данных меньше чем ждали...("
                                     << rlen1 << " < " << szDataLen << ")" << endl;
                    }

                    cleanupChannel();
                    return erInvalidFormat;
                }

                bcnt += rlen1;
                rbuf.dlen = bcnt - szModbusHeader;

                auto mcrc = ReadInputRetMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(recv)(fnReadInputRegisters): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(recv:fnReadInputRegisters): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);

                    if( dlog->is_warn() )
                        dlog->warn() << err.str() << endl;

                    return erBadCheckSum;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnReadOutputRegisters )
            {
                int szDataLen = ReadOutputRetMessage::getDataLen(rbuf) + szCRC;

                if( crcNoCheckit )
                    szDataLen -= szCRC;

                // Мы получили только предварительный загловок
                // Теперь необходимо дополучить данные
                // (c позиции rlen, т.к. часть уже получили)
                int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

                if( rlen1 < szDataLen )
                {
                    rbuf.dlen = bcnt + rlen1 - szModbusHeader;

                    if( dlog->is_warn() )
                    {
                        dlog->warn() << "(0x03): buf: " << rbuf << endl;
                        dlog->warn() << "(0x03)("
                                     << (int)rbuf.func() << "):(fnReadInputRegisters) "
                                     << "Получили данных меньше чем ждали...("
                                     << rlen1 << " < " << szDataLen << ")" << endl;
                    }

                    cleanupChannel();
                    return erInvalidFormat;
                }

                bcnt += rlen1;
                rbuf.dlen = bcnt - szModbusHeader;

                auto mcrc = ReadOutputRetMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(recv)(fnReadOutputRegisters): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(recv:fnReadOutputRegisters): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);

                    if( dlog->is_warn() )
                        dlog->warn() << err.str() << endl;

                    return erBadCheckSum;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnForceMultipleCoils )
            {
                rbuf.dlen = bcnt - szModbusHeader;
                auto mcrc = ForceCoilsRetMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(0x0F): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(0x0F): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);
                    dlog->warn() << err.str() << endl;
                    return erBadCheckSum;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnWriteOutputRegisters )
            {
                rbuf.dlen = bcnt - szModbusHeader;
                auto mcrc = WriteOutputRetMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(0x10): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(0x10): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);
                    dlog->warn() << err.str() << endl;
                    return erBadCheckSum;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnWriteOutputSingleRegister )
            {
                auto mcrc = WriteSingleOutputRetMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(0x06): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(0x06): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);
                    dlog->warn() << err.str() << endl;
                    return erBadCheckSum;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnForceSingleCoil )
            {
                auto mcrc = ForceSingleCoilRetMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(0x05): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(0x05): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);
                    dlog->warn() << err.str() << endl;
                    return erBadCheckSum;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnDiagnostics )
            {
                int szDataLen = DiagnosticRetMessage::getDataLen(rbuf) + szCRC;

                if( crcNoCheckit )
                    szDataLen -= szCRC;

                // Мы получили только предварительный загловок
                // Теперь необходимо дополучить данные
                // (c позиции rlen, т.к. часть уже получили)
                int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

                if( rlen1 < szDataLen )
                {
                    rbuf.dlen = bcnt + rlen1 - szModbusHeader;

                    if( dlog->is_warn() )
                    {
                        dlog->warn() << "(0x08): buf: " << rbuf << endl;
                        dlog->warn() << "(0x08)("
                                     << (int)rbuf.func() << "):(fnDiagnostics) "
                                     << "Получили данных меньше чем ждали...("
                                     << rlen1 << " < " << szDataLen << ")" << endl;
                    }

                    cleanupChannel();
                    return erInvalidFormat;
                }

                bcnt += rlen1;
                rbuf.dlen = bcnt - szModbusHeader;

                auto mcrc = DiagnosticRetMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(recv)(fnDiagnostics): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(recv:fnDiagnostics): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);

                    if( dlog->is_warn() )
                        dlog->warn() << err.str() << endl;

                    return erBadCheckSum;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnMEI )
            {
                MEIMessageRetRDI mPreRDI;
                mPreRDI.pre_init(rbuf);

                if( mPreRDI.objNum > 0 )
                {
                    size_t onum = 0;

                    while( (rlen + 2) < sizeof(rbuf.data) && onum < mPreRDI.objNum )
                    {
                        // сперва получаем два байта, для определения длины последующих данных
                        size_t szDataLen = 2; // object id + len
                        size_t rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

                        if( rlen1 < szDataLen )
                        {
                            rbuf.dlen = bcnt + rlen1 - szModbusHeader;

                            if( dlog->is_warn() )
                            {
                                dlog->warn() << "(0x2B/0x0E): buf: " << rbuf << endl;
                                dlog->warn() << "(0x2B/0x0E)("
                                             << (int)rbuf.func() << "):(fnMEI) "
                                             << "Получили данных меньше чем ждали...("
                                             << rlen1 << " < " << szDataLen << ")" << endl;
                            }

                            cleanupChannel();
                            return erInvalidFormat;
                        }

                        rlen += szDataLen;
                        bcnt += szDataLen;

                        // теперь получаем собственно данные
                        szDataLen = rbuf.data[rlen - 1]; // последний (предыдущий) байт - это длина данных
                        rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

                        if( rlen1 < szDataLen )
                        {
                            rbuf.dlen = bcnt + rlen1 - szModbusHeader;

                            if( dlog->is_warn() )
                            {
                                dlog->warn() << "(0x2B/0x0E): buf: " << rbuf << endl;
                                dlog->warn() << "(0x2B/0x0E)("
                                             << (int)rbuf.func() << "):(fnMEI) "
                                             << "Получили данных меньше чем ждали...("
                                             << rlen1 << " < " << szDataLen << ")" << endl;
                            }

                            cleanupChannel();
                            return erInvalidFormat;
                        }

                        rlen += szDataLen;
                        bcnt += szDataLen;
                        onum++;
                    }
                }

                rbuf.dlen = bcnt - szModbusHeader;

                if( crcNoCheckit )
                {
                    if( dlog->is_info() )
                        dlog->info() << "(recv)(fnMEI): recv buf: " << rbuf << endl;

                    return erNoError;
                }

                // теперь получаем CRC
                size_t rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szCRC);

                if( rlen1 < szCRC )
                {
                    rbuf.dlen = bcnt + rlen1 - szModbusHeader;

                    if( dlog->is_warn() )
                    {
                        dlog->warn() << "(0x2B/0x0E): buf: " << rbuf << endl;
                        dlog->warn() << "(0x2B/0x0E)("
                                     << (int)rbuf.func() << "):(fnMEI) "
                                     << "(CRC): Получили данных меньше чем ждали...("
                                     << rlen1 << " < " << szCRC << ")" << endl;
                    }

                    cleanupChannel();
                    return erInvalidFormat;
                }

                bcnt += rlen1;
                rbuf.dlen = bcnt - szModbusHeader;

                if( dlog->is_info() )
                    dlog->info() << "(recv)(fnMEI): recv buf: " << rbuf << endl;

                auto mcrc = MEIMessageRetRDI::getCrc(rbuf);

                // Проверяем контрольную сумму
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(recv:fnMEI): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);

                    if( dlog->is_warn() )
                        dlog->warn() << err.str() << endl;

                    return erBadCheckSum;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnSetDateTime )
            {
                auto mcrc = SetDateTimeRetMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(0x50): recv buf: " << rbuf << endl;

                if( !crcNoCheckit )
                {
                    // Проверяем контрольную сумму
                    // от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
                    ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                    if( tcrc != mcrc )
                    {
                        ostringstream err;
                        err << "(0x50): bad crc. calc.crc=" << dat2str(tcrc)
                            << " msg.crc=" << dat2str(mcrc);

                        if( dlog->is_warn() )
                            dlog->warn() << err.str() << endl;

                        return erBadCheckSum;
                    }
                }

                if( !SetDateTimeRetMessage::checkFormat(rbuf) )
                {
                    if( dlog->is_warn() )
                        dlog->warn() << "(0x50): некорректные значения..." << endl;

                    return erBadDataValue; // return erInvalidFormat;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnFileTransfer )
            {
                int szDataLen = FileTransferRetMessage::getDataLen(rbuf) + szCRC;

                if( crcNoCheckit )
                    szDataLen -= szCRC;

                // Мы получили только предварительный загловок
                // Теперь необходимо дополучить данные
                // (c позиции rlen, т.к. часть уже получили)
                int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

                if( rlen1 < szDataLen )
                {
                    rbuf.dlen = bcnt + rlen1 - szModbusHeader;

                    if( dlog->is_warn() )
                    {
                        dlog->warn() << "(0x66): buf: " << rbuf << endl;
                        dlog->warn() << "(0x66)("
                                     << rbuf.func() << "):(fnFileTransfer) "
                                     << "Получили данных меньше чем ждали...("
                                     << rlen1 << " < " << szDataLen << ")" << endl;
                    }

                    cleanupChannel();
                    return erInvalidFormat;
                }

                bcnt += rlen1;
                rbuf.dlen = bcnt - szModbusHeader;

                auto mcrc = FileTransferRetMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(0x66): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(0x66): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);

                    if( dlog->is_warn() )
                        dlog->warn() << err.str() << endl;

                    return erBadCheckSum;
                }

                return erNoError;
            }

#if 0
            else if( rbuf.func() == fnJournalCommand )
            {
                auto mcrc = JournalCommandMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(0x65): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                // от начала(включая заголовок) и до конца (исключив последний элемент содержащий CRC)
                // ModbusData tcrc = checkCRC((ModbusByte*)(&rbuf.pduhead),sizeof(ReadOutputMessage)-szCRC);
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(0x65): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);
                    dlog->warn() << err.str() << endl;
                    return erBadCheckSum;
                }

                return erNoError;
            }
            else if( rbuf.func() == fnRemoteService )
            {
                int szDataLen = RemoteServicemessages::getDataLen(rbuf) + szCRC;

                if( crcNoCheckit )
                    szDataLen -= szCRC;

                // Мы получили только предварительный загловок
                // Теперь необходимо дополучить данные
                // (c позиции rlen, т.к. часть уже получили)
                int rlen1 = getNextData((unsigned char*)(&(rbuf.data[rlen])), szDataLen);

                if( rlen1 < szDataLen )
                {
                    rbuf.len = bcnt + rlen1 - szModbusHeader;
                    dlog->warn() << "(0x53): buf: " << rbuf << endl;
                    dlog->warn() << "(0x53)("
                                 << rbuf.func() << "):(fnWriteOutputRegisters) "
                                 << "Получили данных меньше чем ждали...("
                                 << rlen1 << " < " << szDataLen << ")" << endl;

                    cleanupChannel();
                    return erInvalidFormat;
                }

                bcnt += rlen1;
                rbuf.len = bcnt - szModbusHeader;

                auto mcrc = RemoteServiceMessage::getCrc(rbuf);

                if( dlog->is_info() )
                    dlog->info() << "(0x53): recv buf: " << rbuf << endl;

                if( crcNoCheckit )
                    return erNoError;

                // Проверяем контрольную сумму
                // от начала(включая заголовок)
                // и до конца (исключив последний элемент содержащий CRC)
                // int mlen = szModbusHeader + mWrite.szHead()+ mWrite.bcnt;
                ModbusData tcrc = rbuf.pduCRC(bcnt - szCRC);

                if( tcrc != mcrc )
                {
                    ostringstream err;
                    err << "(0x53): bad crc. calc.crc=" << dat2str(tcrc)
                        << " msg.crc=" << dat2str(mcrc);
                    dlog->warn() << err.str() << endl;
                    return erBadCheckSum;
                }

                return erNoError;
            }

#endif // not standart commands

            else
            {
                // А как мы сюда добрались?!!!!!!
                cleanupChannel();
                return erUnExpectedPacketType;
            }
        }
        catch( mbException& ex )
        {
            if( dlog->is_crit() )
                dlog->crit() << "(recv): " << ex << endl;

            return ex.err;
        }
        catch( const uniset3::TimeOut& ex )
        {
            //        cout << "(recv): catch TimeOut " << endl;
        }
        catch( const uniset3::CommFailed& ex )
        {
            if( dlog->is_crit() )
                dlog->crit() << "(recv): " << ex << endl;

            return erTimeOut;
        }
        catch( const uniset3::Exception& ex ) // SystemError
        {
            if( dlog->is_crit() )
                dlog->crit() << "(recv): " << ex << endl;

            return erHardwareError;
        }

        return erTimeOut;
    }

    // -------------------------------------------------------------------------
    mbErrCode ModbusClient::send( ModbusMessage& msg )
    {
        if( msg.len() > msg.maxSizeOfMessage() )
        {
            if( dlog->is_warn() )
                dlog->warn() << "(ModbusClient::send): message len=" << msg.len()
                             << " > MAXLEN=" << msg.maxSizeOfMessage() << endl;

            return erPacketTooLong;
        }

        if( dlog->is_info() )
            dlog->info() << "(ModbusClient::send): [" << msg.len() << " bytes]: " << msg << endl;

        try
        {
            size_t len = msg.len(); // т.к. swapHead() поменяет
            msg.swapHead();
            sendData(msg.buf(), len);
            msg.swapHead();
        }
        catch( mbException& ex )
        {
            if( dlog->is_crit() )
                dlog->crit() << "(send): " << ex << endl;

            msg.swapHead();
            return ex.err;
        }
        catch( const uniset3::Exception& ex ) // SystemError
        {
            if( dlog->is_crit() )
                dlog->crit() << "(send): " << ex << endl;

            msg.swapHead();
            return erHardwareError;
        }

        // Пауза, чтобы не ловить свою посылку
        if( aftersend_msec > 0 )
            msleep(aftersend_msec);

        return erNoError;
    }

    // -------------------------------------------------------------------------
    void ModbusClient::initLog( std::shared_ptr<uniset3::Configuration> conf,
                                const std::string& lname, const string& logfile )
    {
        conf->initLogStream(dlog, lname);

        if( !logfile.empty() )
            dlog->logFile( logfile );
    }
    // -------------------------------------------------------------------------
    void ModbusClient::setLog( std::shared_ptr<DebugStream> l )
    {
        this->dlog = l;
    }
    // -------------------------------------------------------------------------
    void ModbusClient::printProcessingTime()
    {
        if( dlog->is_info() )
        {
            dlog->info() << "(processingTime): "
                         << tmProcessing.getCurrent() << " [мсек]" << endl;
        }
    }
    // -------------------------------------------------------------------------
} // end of namespace uniset3
