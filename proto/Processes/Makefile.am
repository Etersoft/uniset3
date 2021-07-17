############################################################################
# This file is part of the UniSet library                                  #
############################################################################

# Каталоги для размещения генерируемых файлов
CCDIR=$(top_builddir)/src/Processes
HHDIR=$(top_builddir)/include
PROTOFILES=IOController.proto
PROTOFLAGS=-I$(INCPROTODIR)/UniSetTypes

include $(top_builddir)/proto/proto.mk

proto_include_HEADERS = *.proto
proto_includedir = $(datadir)/@PACKAGE@/proto

include $(top_builddir)/include.mk
