SUBDIRS=MQPerfTest PocoTest UHttpTest GRPCTest Open62541Test

#TCPSocketTest
if HAVE_TESTS
############################################################################
# This file is part of the UniSet library                                  #
############################################################################

#check_PROGRAMS = tests tests_with_conf
noinst_PROGRAMS = tests tests_with_conf develop

#umutex threadtst dlog
tests_LDADD 	= $(top_builddir)/lib/libUniSet3.la $(SIGC_LIBS) $(POCO_LIBS) -lpthread
tests_CPPFLAGS	= -I$(top_builddir)/include $(SIGC_CFLAGS) $(POCO_CFLAGS)
tests_SOURCES = tests.cc \
test_passivetimer.cc \
test_passivecondtimer.cc \
test_hourglass.cc \
test_delaytimer.cc \
test_unixml.cc \
test_sscanf_hex.cc \
test_trigger.cc \
test_triggerOR.cc \
test_triggerAND.cc \
test_triggerOUT.cc \
test_pulse.cc \
test_modbustypes.cc \
test_mutex.cc \
test_logserver.cc \
test_tcpcheck.cc \
test_utcpsocket.cc \
test_iocontroller_types.cc \
test_debugstream.cc \
test_oindex_hash.cc
#test_uhttp.cc

tests_with_conf_LDADD   = $(top_builddir)/lib/libUniSet3.la $(top_builddir)/contrib/cityhash102/libCityHash102.la $(top_builddir)/contrib/murmurhash/libMurMurHash.la
tests_with_conf_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/contrib/cityhash102/include -I$(top_builddir)/contrib/murmurhash/include -I$(top_builddir)/contrib/catch2
tests_with_conf_SOURCES = tests_with_conf.cc \
test_conftest.cc \
test_ui.cc \
test_iorfile.cc \
test_messagetype.cc \
test_utypes.cc \
test_mqueue.cc \
test_uobject.cc \
test_lt_object.cc

# threadtst_SOURCES = threadtst.cc
# threadtst_LDADD 	= $(top_builddir)/lib/libUniSet3.la ${SIGC_LIBS}
# threadtst_CPPFLAGS	= -I$(top_builddir)/include ${SIGC_CFLAGS}
# 


develop_LDADD   = $(top_builddir)/lib/libUniSet3.la
develop_CPPFLAGS = -I$(top_builddir)/include
develop_SOURCES  = develop.cc



include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE)
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)'|| $(SHELL) '$(TESTSUITE)' --clean
	rm -rf $(CLEANFILES)

if HAVE_COVERAGE
COVERAGE_DIRS=$(abs_srcdir) $(top_builddir)/src
include $(top_builddir)/testsuite/testcoverage-common.mk
endif

endif

include $(top_builddir)/include.mk
