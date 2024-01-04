if ENABLE_OPCUA
# не забывайте править версию в2.pc-файле
OPCUA_CLI_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet3OPCUAExchange.la
libUniSet3OPCUAExchange_la_LDFLAGS  = -version-info $(OPCUA_CLI_VER)
libUniSet3OPCUAExchange_la_LIBADD = $(top_builddir)/lib/libUniSet3.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(SIGC_LIBS) $(OPEN62541_LIBS)
libUniSet3OPCUAExchange_la_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(OPEN62541_CFLAGS)
libUniSet3OPCUAExchange_la_SOURCES = OPCUAExchange.cc OPCUAClient.cc

bin_PROGRAMS = @PACKAGE@-opcua-exchange @PACKAGE@-opcua-tester

@PACKAGE@_opcua_exchange_SOURCES = main.cc
@PACKAGE@_opcua_exchange_LDADD = libUniSet3OPCUAExchange.la $(top_builddir)/lib/libUniSet3.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(SIGC_LIBS) $(OPEN62541_LIBS)
@PACKAGE@_opcua_exchange_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(OPEN62541_CFLAGS)


@PACKAGE@_opcua_tester_SOURCES = client.cc
@PACKAGE@_opcua_tester_LDADD = libUniSet3OPCUAExchange.la $(top_builddir)/lib/libUniSet3.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(SIGC_LIBS) $(OPEN62541_LIBS)
@PACKAGE@_opcua_tester_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(OPEN62541_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions/opcua

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libUniSet3OPCUAExchange.pc

include $(top_builddir)/include.mk
endif
