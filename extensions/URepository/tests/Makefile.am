if HAVE_TESTS

noinst_PROGRAMS = run_test_urepository

run_test_urepository_SOURCES = run_test_urepository.cc test_urepository.cc TestObject.cc
run_test_urepository_LDADD  = $(top_builddir)/lib/libUniSet3.la $(top_builddir)/extensions/lib/libUniSet3Extensions.la $(SIGC_LIBS) $(POCO_LIBS)
run_test_urepository_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(POCO_CFLAGS)

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) urepository-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

include $(top_builddir)/include.mk

endif
