if ENABLE_LOGICPROC

# не забывайте править версию в2.pc-файле
ULPROC_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet3LProcessor.la
libUniSet3LProcessor_la_LDFLAGS  = -version-info $(ULPROC_VER)
libUniSet3LProcessor_la_LIBADD	= $(top_builddir)/lib/libUniSet3.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la $(SIGC_LIBS)
libUniSet3LProcessor_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libUniSet3LProcessor_la_SOURCES 	= Element.cc TOR.cc TAND.cc TDelay.cc TNOT.cc TA2D.cc \
Schema.cc SchemaXML.cc LProcessor.cc PassiveLProcessor.cc

bin_PROGRAMS = @PACKAGE@-logicproc @PACKAGE@-plogicproc

@PACKAGE@_logicproc_SOURCES = logicproc.cc
@PACKAGE@_logicproc_LDADD   = libUniSet3LProcessor.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(top_builddir)/lib/libUniSet3.la $(SIGC_LIBS)
@PACKAGE@_logicproc_CXXFLAGS	= -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	$(SIGC_CFLAGS)

@PACKAGE@_plogicproc_SOURCES 	= plogicproc.cc
@PACKAGE@_plogicproc_LDADD 	= libUniSet3LProcessor.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(top_builddir)/lib/libUniSet3.la $(SIGC_LIBS)
@PACKAGE@_plogicproc_CXXFLAGS	= -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	$(SIGC_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions/logicproc

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet3LogicProcessor.pc

#all-local:
#	ln -sf ../LogicProcessor/$(devel_include_HEADERS) ../include

# SUBDIRS=tests

include $(top_builddir)/include.mk

endif
