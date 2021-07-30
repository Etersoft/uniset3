if ENABLE_OPENTSDB
bin_PROGRAMS = @PACKAGE@-backend-opentsdb

TSDB_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet3BackendOpenTSDB.la
libUniSet3BackendOpenTSDB_la_LDFLAGS  = -version-info $(TSDB_VER)
libUniSet3BackendOpenTSDB_la_LIBADD		= $(top_builddir)/lib/libUniSet3.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
									$(SIGC_LIBS)
libUniSet3BackendOpenTSDB_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS)
libUniSet3BackendOpenTSDB_la_SOURCES 	= BackendOpenTSDB.cc

@PACKAGE@_backend_opentsdb_SOURCES 	= main.cc
@PACKAGE@_backend_opentsdb_LDADD 	= libUniSet3BackendOpenTSDB.la $(top_builddir)/lib/libUniSet3.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
									$(SIGC_LIBS)
@PACKAGE@_backend_opentsdb_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet3BackendOpenTSDB.pc

include $(top_builddir)/include.mk

endif
