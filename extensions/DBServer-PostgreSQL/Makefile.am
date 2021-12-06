if ENABLE_PGSQL
UPGSQL_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet3-pgsql.la
libUniSet3_pgsql_la_LDFLAGS  = -version-info $(UPGSQL_VER)
libUniSet3_pgsql_la_SOURCES = PostgreSQLInterface.cc DBServer_PostgreSQL.cc
libUniSet3_pgsql_la_LIBADD = $(top_builddir)/lib/libUniSet3.la $(PGSQL_LIBS)
libUniSet3_pgsql_la_CXXFLAGS = $(PGSQL_CFLAGS)

bin_PROGRAMS = @PACKAGE@-pgsql-dbserver
@PACKAGE@_pgsql_dbserver_LDADD = libUniSet3-pgsql.la $(top_builddir)/lib/libUniSet3.la $(PGSQL_LIBS)
@PACKAGE@_pgsql_dbserver_CXXFLAGS = $(PGSQL_CFLAGS)
@PACKAGE@_pgsql_dbserver_SOURCES = main.cc

noinst_PROGRAMS = pgsql-test
pgsql_test_LDADD   = libUniSet3-pgsql.la $(top_builddir)/lib/libUniSet3.la $(PGSQL_LIBS)
pgsql_test_CXXFLAGS = $(PGSQL_CFLAGS)
pgsql_test_SOURCES = test.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/extensions/pgsql

pkgconfigdir    = $(libdir)/pkgconfig
pkgconfig_DATA  = libUniSet3PostgreSQL.pc

include $(top_builddir)/include.mk

endif
