noinst_LTLIBRARIES 		= libMBMaster.la
libMBMaster_la_LIBADD	= $(top_builddir)/lib/libUniSet3.la \
								$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
								$(SIGC_LIBS)
libMBMaster_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libMBMaster_la_SOURCES 	= MBExchange.cc MBConfig.cc

bin_PROGRAMS 		= @PACKAGE@-mbtcpmaster @PACKAGE@-mbtcpmultimaster @PACKAGE@-rtuexchange @PACKAGE@-mtr-conv @PACKAGE@-vtconv @PACKAGE@-mtr-setup @PACKAGE@-mtr-read
lib_LTLIBRARIES 	= libUniSet3MBTCPMaster.la libUniSet3RTU.la

# -------------------- Mobus TCP -------------------
# не забывайте править версию в2.pc-файле
UMBTCP_VER=@LIBVER@


libUniSet3MBTCPMaster_la_LDFLAGS  = -version-info $(UMBTCP_VER)
libUniSet3MBTCPMaster_la_LIBADD	 = libMBMaster.la $(top_builddir)/lib/libUniSet3.la \
								$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
								$(SIGC_LIBS)
libUniSet3MBTCPMaster_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libUniSet3MBTCPMaster_la_SOURCES 	= MBTCPMaster.cc MBTCPMultiMaster.cc

@PACKAGE@_mbtcpmaster_SOURCES 	= main.cc
@PACKAGE@_mbtcpmaster_LDADD 	= libUniSet3MBTCPMaster.la libMBMaster.la $(top_builddir)/lib/libUniSet3.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
									$(SIGC_LIBS)
@PACKAGE@_mbtcpmaster_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)

@PACKAGE@_mbtcpmultimaster_SOURCES 	= multi-main.cc
@PACKAGE@_mbtcpmultimaster_LDADD 	= libUniSet3MBTCPMaster.la libMBMaster.la $(top_builddir)/lib/libUniSet3.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
									$(SIGC_LIBS)
@PACKAGE@_mbtcpmultimaster_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)

# -------------------- Mobus RTU -------------------
# не забывайте править версию в2.pc-файле
URTU_VER=@LIBVER@

libUniSet3RTU_la_LDFLAGS  = -version-info $(URTU_VER)
libUniSet3RTU_la_LIBADD   = libMBMaster.la $(top_builddir)/lib/libUniSet3.la \
							$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
							$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
							$(SIGC_LIBS)
libUniSet3RTU_la_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libUniSet3RTU_la_SOURCES  = RTUExchange.cc

@PACKAGE@_rtuexchange_SOURCES = rtuexchange.cc
@PACKAGE@_rtuexchange_LDADD   = libUniSet3RTU.la libMBMaster.la $(top_builddir)/lib/libUniSet3.la \
								$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
								$(SIGC_LIBS)
@PACKAGE@_rtuexchange_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)

@PACKAGE@_mtr_conv_SOURCES	= mtr-conv.cc
@PACKAGE@_mtr_conv_LDADD	= $(top_builddir)/extensions/lib/libUniSet3Extensions.la $(top_builddir)/lib/libUniSet3.la
@PACKAGE@_mtr_conv_CXXFLAGS = -I$(top_builddir)/extensions/include

@PACKAGE@_mtr_setup_SOURCES	= mtr-setup.cc
@PACKAGE@_mtr_setup_LDADD	= $(top_builddir)/extensions/lib/libUniSet3Extensions.la $(top_builddir)/lib/libUniSet3.la
@PACKAGE@_mtr_setup_CXXFLAGS = -I$(top_builddir)/extensions/include

@PACKAGE@_mtr_read_SOURCES	= mtr-read.cc
@PACKAGE@_mtr_read_LDADD	= $(top_builddir)/extensions/lib/libUniSet3Extensions.la $(top_builddir)/lib/libUniSet3.la
@PACKAGE@_mtr_read_CXXFLAGS = -I$(top_builddir)/extensions/include

@PACKAGE@_vtconv_SOURCES	= vtconv.cc
@PACKAGE@_vtconv_LDADD	= $(top_builddir)/extensions/lib/libUniSet3Extensions.la $(top_builddir)/lib/libUniSet3.la
@PACKAGE@_vtconv_CXXFLAGS = -I$(top_builddir)/extensions/include

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet3MBTCPMaster.pc libUniSet3RTU.pc

#all-local:
#	ln -sf ../ModbusMaster/$(devel_include_HEADERS) ../include

noinst_PROGRAMS = mb-perf-test

mb_perf_test_SOURCES = mb-perf-test.cc
mb_perf_test_LDADD 	= libUniSet3MBTCPMaster.la libMBMaster.la $(top_builddir)/lib/libUniSet3.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
									$(SIGC_LIBS)
mb_perf_test_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)


include $(top_builddir)/include.mk
