if DISABLE_LOGDB

else

bin_SCRIPTS = @PACKAGE@-logdb-adm

bin_PROGRAMS = @PACKAGE@-logdb @PACKAGE@-logdb-conv
@PACKAGE@_logdb_LDADD = $(top_builddir)/extensions/DBServer-SQLite/libUniSet3-sqlite.la $(top_builddir)/lib/libUniSet3.la
@PACKAGE@_logdb_CXXFLAGS = $(SQLITE3_CFLAGS) -I$(top_builddir)/extensions/DBServer-SQLite
@PACKAGE@_logdb_SOURCES = LogDB.cc main.cc

@PACKAGE@_logdb_conv_LDADD = $(top_builddir)/extensions/DBServer-SQLite/libUniSet3-sqlite.la $(top_builddir)/lib/libUniSet3.la
@PACKAGE@_logdb_conv_CXXFLAGS = $(SQLITE3_CFLAGS) -I$(top_builddir)/extensions/DBServer-SQLite
@PACKAGE@_logdb_conv_SOURCES = logdb-conv.cc

include $(top_builddir)/include.mk

endif
