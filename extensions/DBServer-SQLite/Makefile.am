if ENABLE_SQLITE
USQLITE_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet3-sqlite.la
libUniSet3_sqlite_la_LDFLAGS = -version-info $(USQLITE_VER)
libUniSet3_sqlite_la_SOURCES = SQLiteInterface.cc DBServer_SQLite.cc
libUniSet3_sqlite_la_LIBADD = $(top_builddir)/lib/libUniSet3.la $(SQLITE3_LIBS)
libUniSet3_sqlite_la_CXXFLAGS = $(SQLITE3_CFLAGS)

bin_PROGRAMS = @PACKAGE@-sqlite-dbserver
@PACKAGE@_sqlite_dbserver_LDADD = libUniSet3-sqlite.la $(top_builddir)/lib/libUniSet3.la
@PACKAGE@_sqlite_dbserver_SOURCES = main.cc

noinst_PROGRAMS = sqlite-test
sqlite_test_LDADD = libUniSet3-sqlite.la $(top_builddir)/lib/libUniSet3.la
sqlite_test_SOURCES = test.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/extensions/sqlite

pkgconfigdir	= $(libdir)/pkgconfig
pkgconfig_DATA  = libUniSet3SQLite.pc

include $(top_builddir)/include.mk

endif
