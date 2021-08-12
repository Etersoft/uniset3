SUBDIRS=proto

bin_PROGRAMS = @PACKAGE@-unetexchange @PACKAGE@-unet-udp-tester @PACKAGE@-unet-multicast-tester

# не забывайте править версию в2.pc-файле
UNETUDP_VER=@LIBVER@

lib_LTLIBRARIES 	      = libUniSet3UNetUDP.la
libUniSet3UNetUDP_la_LDFLAGS  = -version-info $(UNETUDP_VER)
libUniSet3UNetUDP_la_LIBADD   = $(top_builddir)/lib/libUniSet3.la \
				$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
				$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
                                $(SIGC_LIBS) $(POCO_LIBS) $(PROTOBUF_LIBS)
libUniSet3UNetUDP_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS) $(PROTOBUF_CFLAGS)
libUniSet3UNetUDP_la_SOURCES 	= proto/unet.pb.cc UDPPacket.cc UDPTransport.cc MulticastTransport.cc UNetReceiver.cc UNetSender.cc UNetExchange.cc

@PACKAGE@_unetexchange_SOURCES 		= unetexchange.cc
@PACKAGE@_unetexchange_LDADD 		= libUniSet3UNetUDP.la $(top_builddir)/lib/libUniSet3.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet3Extensions.la \
									$(SIGC_LIBS) $(POCO_LIBS)
@PACKAGE@_unetexchange_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)


@PACKAGE@_unet_udp_tester_SOURCES	= proto/unet.pb.cc UDPPacket.cc unet-udp-tester.cc
@PACKAGE@_unet_udp_tester_LDADD 	= $(top_builddir)/lib/libUniSet3.la $(POCO_LIBS) $(PROTOBUF_LIBS)
@PACKAGE@_unet_udp_tester_CXXFLAGS	= $(POCO_CFLAGS) $(PROTOBUF_CFLAGS)

@PACKAGE@_unet_multicast_tester_SOURCES	 = proto/unet.pb.cc UDPPacket.cc MulticastTransport.cc unet-multicast-tester.cc
@PACKAGE@_unet_multicast_tester_LDADD 	 = $(top_builddir)/lib/libUniSet3.la $(POCO_LIBS) $(PROTOBUF_LIBS)
@PACKAGE@_unet_multicast_tester_CXXFLAGS = $(POCO_CFLAGS) $(PROTOBUF_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet3UNetUDP.pc

#all-local:
#	ln -sf ../UNetUDP/$(devel_include_HEADERS) ../include

include $(top_builddir)/include.mk
