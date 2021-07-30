bin_PROGRAMS		= @PACKAGE@-smemory

# не забывайте править версию в2.pc-файле
USM_VER=@LIBVER@

lib_LTLIBRARIES 	= libUniSet3SharedMemory.la
libUniSet3SharedMemory_la_LDFLAGS	= -version-info $(USM_VER)
libUniSet3SharedMemory_la_LIBADD		= $(SIGC_LIBS) $(top_builddir)/lib/libUniSet3.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la -lrt
libUniSet3SharedMemory_la_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
libUniSet3SharedMemory_la_SOURCES 	= SharedMemory.cc


@PACKAGE@_smemory_LDADD 	= libUniSet3SharedMemory.la $(top_builddir)/lib/libUniSet3.la \
	$(top_builddir)/extensions/lib/libUniSet3Extensions.la
@PACKAGE@_smemory_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
@PACKAGE@_smemory_SOURCES 	= smemory.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet3SharedMemory.pc

#all-local:
#	ln -sf ../SharedMemory/$(devel_include_HEADERS) ../include

include $(top_builddir)/include.mk
