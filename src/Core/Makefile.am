noinst_LTLIBRARIES = libUCore.la
libUCore_la_SOURCES = UniSetTypes.pb.cc UniSetTypes.grpc.pb.cc UniSetTypes.cc \
	UniSetObject.pb.cc UniSetObject.grpc.pb.cc UniSetObject.cc \
	UniSetManager.pb.cc UniSetManager.grpc.pb.cc UniSetManager.cc UniSetActivator.cc \
	Configuration.cc MessageTypes.pb.cc MessageTypes.grpc.pb.cc \
	UInterface.cc URepository.grpc.pb.cc URepository.pb.cc
libUCore_la_CFLAGS = $(PROTOBUF_CFLAGS)
libUCore_la_LIBADD = $(PROTOBUF_LIBS)

include $(top_builddir)/include.mk
