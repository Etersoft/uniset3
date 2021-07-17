// -------------------------------------------------------------------------
#ifndef ModbusHelpers_H_
#define ModbusHelpers_H_
// -------------------------------------------------------------------------
#include <string>
#include "ModbusTypes.h"
#include "ComPort.h"
// -------------------------------------------------------------------------
namespace uniset3
{
    // -------------------------------------------------------------------------
    class ModbusRTUMaster;
    class ModbusClient;
    // -------------------------------------------------------------------------
    namespace ModbusHelpers
    {
        ModbusRTU::ModbusAddr autodetectSlave( uniset3::ModbusClient* m,
                                               ModbusRTU::ModbusAddr beg = 0,
                                               ModbusRTU::ModbusAddr end = 255,
                                               ModbusRTU::ModbusData reg = 0,
                                               ModbusRTU::SlaveFunctionCode fn = ModbusRTU::fnReadInputRegisters
                                             ); // throw uniset3::TimeOut();

        ModbusRTU::ModbusAddr autodetectSlave( std::string dev,
                                               ComPort::Speed s, int tout = 1000,
                                               ModbusRTU::ModbusAddr beg = 0,
                                               ModbusRTU::ModbusAddr end = 255,
                                               ModbusRTU::ModbusData reg = 0,
                                               ModbusRTU::SlaveFunctionCode fn = ModbusRTU::fnReadInputRegisters
                                             ); // throw uniset3::TimeOut();

        ComPort::Speed autodetectSpeed( ModbusRTUMaster* m, ModbusRTU::ModbusAddr slave,
                                        ModbusRTU::ModbusData reg = 0,
                                        ModbusRTU::SlaveFunctionCode fn = ModbusRTU::fnReadInputRegisters
                                      ); // throw uniset3::TimeOut();

        ComPort::Speed autodetectSpeed( std::string dev,
                                        ModbusRTU::ModbusAddr slave,
                                        int timeout_msec = 1000,
                                        ModbusRTU::ModbusData reg = 0,
                                        ModbusRTU::SlaveFunctionCode fn = ModbusRTU::fnReadInputRegisters
                                      ); // throw uniset3::TimeOut();

    } // end of namespace ModbusHelpers
    // ---------------------------------------------------------------------------
} // end of namespace uniset3
// ---------------------------------------------------------------------------
#endif // ModbusHelpers_H_
// ---------------------------------------------------------------------------
