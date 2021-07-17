noinst_LTLIBRARIES = libUCore.la
libUCore_la_SOURCES = UniSetTypes.pb.cc UniSetObject.pb.cc UniSetTypes.cc \
	UniSetManager.pb.cc UniSetObject.cc UniSetManager.cc UniSetActivator.cc \
	Configuration.cc MessageType.cc UInterface.cc
libUCore_la_CFLAGS = $(PROTOBUF_CFLAGS)
libUCore_la_LIBADD = $(PROTOBUF_LIBS)

include $(top_builddir)/include.mk
