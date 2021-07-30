if ENABLE_UWEBSOCKETGATE
bin_PROGRAMS = @PACKAGE@-wsgate

WSGATE_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet3UWebSocketGate.la
libUniSet3UWebSocketGate_la_SOURCES = UWebSocketGate.cc
libUniSet3UWebSocketGate_la_LDFLAGS = -version-info $(WSGATE_VER)
libUniSet3UWebSocketGate_la_LIBADD = $(top_builddir)/lib/libUniSet3.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS)
libUniSet3UWebSocketGate_la_CXXFLAGS = -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)


@PACKAGE@_wsgate_SOURCES = main.cc
@PACKAGE@_wsgate_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)
@PACKAGE@_wsgate_LDADD = libUniSet3UWebSocketGate.la $(top_builddir)/lib/libUniSet3.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS)


# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions/wsgate

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = libUniSet3UWebSocketGate.pc


include $(top_builddir)/include.mk
endif
