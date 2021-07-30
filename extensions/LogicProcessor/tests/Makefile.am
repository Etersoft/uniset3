if ENABLE_LOGICPROC
if HAVE_TESTS

#check_PROGRAMS = tests
noinst_PROGRAMS = tests

tests_SOURCES   = tests.cc NullSM.cc lproc.cc
tests_LDADD	 = $(top_builddir)/lib/libUniSet3.la $(top_builddir)/extensions/lib/libUniSet3Extensions.la \
	$(top_builddir)/extensions/LogicProcessor/libUniSet3LProcessor.la -lpthread
tests_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/LogicProcessor

$(top_builddir)/extensions/lib/libUniSet3Extensions.la:
	cd $(top_builddir)/extensions/lib/ && make

$(top_builddir)/extensions/LogicProcessor/libUniSet3LProcessor.la:
	cd $(top_builddir)/extensions/LogicProcessor/ && make


include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE)
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)
	
clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

include $(top_builddir)/include.mk

endif
endif
