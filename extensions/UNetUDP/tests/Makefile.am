if HAVE_TESTS

noinst_PROGRAMS = tests-with-sm tests-multicast-with-sm urecv-perf-test

tests_with_sm_SOURCES   = tests_with_sm.cc test_unetudp.cc test_unet_proto.cc
tests_with_sm_LDADD     = $(top_builddir)/lib/libUniSet3.la $(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(top_builddir)/extensions/UNetUDP/libUniSet3UNetUDP.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS) $(PROTOBUF_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/UNetUDP -I$(top_builddir)/extensions/UNetUDP/proto \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS) $(PROTOBUF_CFLAGS)

tests_multicast_with_sm_SOURCES   = tests_multicast_with_sm.cc test_unetmulticast.cc test_unetudp_multicast.cc
tests_multicast_with_sm_LDADD     = $(top_builddir)/lib/libUniSet3.la $(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(top_builddir)/extensions/UNetUDP/libUniSet3UNetUDP.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS) $(PROTOBUF_LIBS)
tests_multicast_with_sm_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/UNetUDP -I$(top_builddir)/extensions/UNetUDP/proto \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS) $(PROTOBUF_CFLAGS)

urecv_perf_test_SOURCES   = urecv_perf_test.cc
urecv_perf_test_LDADD     = $(top_builddir)/lib/libUniSet3.la $(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(top_builddir)/extensions/UNetUDP/libUniSet3UNetUDP.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet3SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS) $(PROTOBUF_LIBS)
urecv_perf_test_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/UNetUDP -I$(top_builddir)/extensions/UNetUDP/proto \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS) $(PROTOBUF_CFLAGS)


include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) unetudp-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

endif
