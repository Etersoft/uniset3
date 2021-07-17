############################################################################
# This file is part of the UniSet library                                  #
############################################################################

# Каталоги для размещения генерируемых файлов
CCDIR=$(top_builddir)/src/Core
HHDIR=$(top_builddir)/include
PROTOFLAGS=-I$(INCPROTODIR)/UniSetTypes
PROTOFILES=UniSetTypes.proto UniSetObject.proto UniSetManager.proto

include $(top_builddir)/proto/proto.mk

proto_include_HEADERS = *.proto
proto_includedir = $(datadir)/@PACKAGE@/proto

include $(top_builddir)/include.mk
