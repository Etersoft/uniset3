if ENABLE_REST_API
if HAVE_TESTS

noinst_PROGRAMS = run_test_apigateway simple_tests

run_test_apigateway_SOURCES = run_test_apigateway.cc test_apigateway.cc TestObject.cc
run_test_apigateway_LDADD  = $(top_builddir)/lib/libUniSet3.la $(top_builddir)/extensions/lib/libUniSet3Extensions.la $(top_builddir)/extensions/HttpAPIGateway/libUniSet3HttpAPIGateway.la $(SIGC_LIBS) $(POCO_LIBS)
run_test_apigateway_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/HttpAPIGateway $(SIGC_CFLAGS) $(POCO_CFLAGS)

simple_tests_SOURCES = tests.cc test_router.cc
simple_tests_LDADD  = $(top_builddir)/lib/libUniSet3.la $(top_builddir)/extensions/lib/libUniSet3Extensions.la $(top_builddir)/extensions/HttpAPIGateway/libUniSet3HttpAPIGateway.la $(SIGC_LIBS) $(POCO_LIBS)
simple_tests_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/HttpAPIGateway $(SIGC_CFLAGS) $(POCO_CFLAGS)

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) apigateway-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

include $(top_builddir)/include.mk

endif
endif
