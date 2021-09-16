noinst_LTLIBRARIES = libUCore.la
libUCore_la_SOURCES = UniSetTypes.pb.cc UniSetTypes.grpc.pb.cc UniSetTypes.cc \
	MetricsExporter.pb.cc MetricsExporter.grpc.pb.cc MetricsExporterProxy.cc \
	Configurator.pb.cc Configurator.grpc.pb.cc ConfiguratorProxy.cc \
	UniSetObject.pb.cc UniSetObject.grpc.pb.cc UniSetObject.cc \
	UniSetManager.pb.cc UniSetManager.grpc.pb.cc UniSetManager.cc UniSetActivator.cc \
	Configuration.cc MessageTypes.pb.cc MessageTypes.grpc.pb.cc \
	UInterface.cc URepository.grpc.pb.cc URepository.pb.cc \
	UniSetObjectProxy.cc UniSetManagerProxy.cc IOControllerProxy.cc IONotifyControllerProxy.cc
libUCore_la_CXXFLAGS = -I$(top_builddir)/contrib/cityhash102/include \
	-I$(top_builddir)/contrib/libfarmhash \
	$(PROTOBUF_CFLAGS)
libUCore_la_LIBADD = $(top_builddir)/contrib/cityhash102/libCityHash102.la \
	$(top_builddir)/contrib/libfarmhash/libFarmHash.la \
	$(PROTOBUF_LIBS)

include $(top_builddir)/include.mk
