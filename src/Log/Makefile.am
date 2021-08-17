############################################################################
# This file is part of the UniSet library                                  #
############################################################################

noinst_LTLIBRARIES = libLog.la
libLog_la_CXXFLAGS = $(SIGC_CFLAGS) $(POCO_CFLAGS) $(PROTOBUF_CFLAGS)
libLog_la_LIBADD = $(SIGC_LIBS) $(POCO_LIBS) $(PROTOBUF_LIBS)
libLog_la_SOURCES = DebugStream.cc Debug.cc LogServer.cc \
	LogReader.cc LogAgregator.cc \
	LogServer.grpc.pb.cc LogServer.pb.cc 

include $(top_builddir)/include.mk
