############################################################################
# This file is part of the UniSet library								  #
############################################################################

if HAVE_EXTENTIONS
SUBDIRS = lib include SharedMemory SharedMemory/tests IOControl IOControl/tests LogicProcessor LogicProcessor/tests \
	ModbusMaster  ModbusSlave  SMViewer UNetUDP UNetUDP/tests \
	DBServer-MySQL DBServer-SQLite DBServer-PostgreSQL MQTTPublisher \
	tests ModbusMaster/tests ModbusSlave/tests LogDB LogDB/tests \
	Backend-OpenTSDB HttpResolver HttpResolver/tests URepository URepository/tests UWebSocketGate UWebSocketGate/tests


pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet3Extensions.pc

else
SUBDIRS =
endif
