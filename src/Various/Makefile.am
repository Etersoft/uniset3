############################################################################
# This file is part of the UniSet library                                  #
############################################################################

noinst_LTLIBRARIES = libVarious.la
libVarious_la_CXXFLAGS 	= $(SIGC_CFLAGS) $(POCO_CFLAGS)
libVarious_la_LIBADD 	= $(SIGC_LIBS) $(POCO_LIBS)
libVarious_la_SOURCES 	= UniXML.cc MQMutex.cc MQAtomic.cc \
	Mutex.cc SViewer.cc SMonitor.cc WDTInterface.cc VMonitor.cc \
	ujson.cc

local-clean:
	rm -rf *iSK.cc
	

include $(top_builddir)/include.mk
