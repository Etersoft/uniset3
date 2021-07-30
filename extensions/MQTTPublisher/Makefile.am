if ENABLE_MQTT
bin_PROGRAMS = @PACKAGE@-mqttpublisher

MQTT_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet3MQTTPublisher.la
libUniSet3MQTTPublisher_la_LDFLAGS  = -version-info $(MQTT_VER)
libUniSet3MQTTPublisher_la_LIBADD		= $(top_builddir)/lib/libUniSet3.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
									$(SIGC_LIBS) $(MQTT_LIBS)
libUniSet3MQTTPublisher_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(MQTT_CFLAGS)
libUniSet3MQTTPublisher_la_SOURCES 	= MQTTPublisher.cc

@PACKAGE@_mqttpublisher_SOURCES 	= main.cc
@PACKAGE@_mqttpublisher_LDADD 	= libUniSet3MQTTPublisher.la $(top_builddir)/lib/libUniSet3.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
									$(SIGC_LIBS) $(MQTT_LIBS)
@PACKAGE@_mqttpublisher_CXXFLAGS	= -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	$(SIGC_CFLAGS) $(MQTT_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions/mqtt

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet3MQTTPublisher.pc

#all-local:
#	ln -sf ../MQTTPublisher/$(devel_include_HEADERS) ../include

include $(top_builddir)/include.mk

endif
