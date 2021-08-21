if ENABLE_REST_API
bin_PROGRAMS = @PACKAGE@-api-gateway

APIGATEWAY_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet3HttpAPIGateway.la
libUniSet3HttpAPIGateway_la_SOURCES = UHttpRouter.cc HttpAPIGateway.cc APIGateway.pb.cc
libUniSet3HttpAPIGateway_la_LDFLAGS = -version-info $(APIGATEWAY_VER)
libUniSet3HttpAPIGateway_la_LIBADD = $(top_builddir)/lib/libUniSet3.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS)
libUniSet3HttpAPIGateway_la_CXXFLAGS = -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)


@PACKAGE@_api_gateway_SOURCES = main.cc
@PACKAGE@_api_gateway_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)
@PACKAGE@_api_gateway_LDADD = libUniSet3HttpAPIGateway.la $(top_builddir)/lib/libUniSet3.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS)

BUILT_SOURCES=APIGateway.pb.h APIGateway.pb.cc

$(BUILT_SOURCES): APIGateway.proto
	${PROTOC} --cpp_out=$(srcdir) APIGateway.proto

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions/apigateway

proto_include_HEADERS = *.proto
proto_includedir = $(pkgincludedir)/extensions/apigateway

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = libUniSet3HttpAPIGateway.pc


include $(top_builddir)/include.mk
endif
