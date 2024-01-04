############################################################################
# This file is part of the UniSet library                                  #
############################################################################

#

if HAVE_EXTENTIONS
SUBDIRS = lib include SharedMemory SharedMemory/tests IOControl IOControl/tests LogicProcessor LogicProcessor/tests \
	UNetUDP UNetUDP/tests \
	ModbusMaster  ModbusSlave  SMViewer  \
	DBServer-MySQL DBServer-SQLite DBServer-PostgreSQL MQTTPublisher \
	tests ModbusMaster/tests ModbusSlave/tests LogDB LogDB/tests \
	Backend-OpenTSDB URepository URepository/tests UWebSocketGate UWebSocketGate/tests \
	OPCUAServer OPCUAServer/tests OPCUAExchange OPCUAExchange/tests \
	HttpAPIGateway HttpAPIGateway/tests


pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet3Extensions.pc

else
SUBDIRS =
endif
