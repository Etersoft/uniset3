bin_PROGRAMS = @PACKAGE@-mbslave

# не забывайте править версию в2.pc-файле
UMBS_VER=@LIBVER@

lib_LTLIBRARIES 			  = libUniSet3MBSlave.la
libUniSet3MBSlave_la_LDFLAGS  = -version-info $(UMBS_VER)
libUniSet3MBSlave_la_LIBADD		= $(top_builddir)/lib/libUniSet3.la \
								$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
								$(SIGC_LIBS) $(POCO_LIBS)
libUniSet3MBSlave_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)
libUniSet3MBSlave_la_SOURCES 	= MBSlave.cc

@PACKAGE@_mbslave_SOURCES 	= mbslave.cc
@PACKAGE@_mbslave_LDADD 	= libUniSet3MBSlave.la $(top_builddir)/lib/libUniSet3.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
									$(SIGC_LIBS) $(POCO_LIBS)
@PACKAGE@_mbslave_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet3MBSlave.pc

#all-local:
#	ln -sf ../ModbusSlave/$(devel_include_HEADERS) ../include

include $(top_builddir)/include.mk

# SUBDIRS=tests
