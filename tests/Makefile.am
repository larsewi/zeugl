EXTRA_DIST = testsuite.at $(TESTSUITE) atconfig package.m4

TESTSUITE = $(srcdir)/testsuite
TESTSOURCES = $(srcdir)/testsuite.at
AUTOM4TE = $(SHELL) $(top_srcdir)/missing --run autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest

check-local: atconfig $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

installcheck-local: atconfig $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' AUTOTEST_PATH='$(DESTDIR)$(bindir)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	rm -rf atconfig

atconfig: $(top_builddir)/config.status
	cd $(top_builddir) && $(SHELL) ./config.status tests/$@

$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	$(AM_V_GEN) :;{ \
        echo '# Signature of the current package.' && \
        echo 'm4_define([AT_PACKAGE_NAME], [$(PACKAGE_NAME)])}' && \
        echo 'm4_define([AT_PACKAGE_TARNAME], [$(PACKAGE_TARNAME)])}' && \
        echo 'm4_define([AT_PACKAGE_VERSION], [$(PACKAGE_VERSION)])}' && \
        echo 'm4_define([AT_PACKAGE_STRING], [$(PACKAGE_STRING)])}' && \
        echo 'm4_define([AT_PACKAGE_BUGREPORT], [$(PACKAGE_BUGREPORT)])}'; \
        echo 'm4_define([AT_PACKAGE_URL], [$(PACKAGE_URL)])}'; \
    } > '$(srcdir)/package.m4'

$(TESTSUITE): $(TESTSOURCES) $(srcdir)/package.m4
	$(AM_V_GEN) $(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at; mv $@.tmp $@

AM_CPPFLAGS = -I$(top_builddir)/ -I$(top_srcdir)/include/

check_PROGRAMS = test_multithreaded test_cleanup

test_multithreaded_LDADD = $(top_builddir)/lib/libzeugl.la
test_multithreaded_SOURCES = test_multithreaded.c

test_cleanup_LDADD = $(top_builddir)/lib/libzeugl.la
test_cleanup_SOURCES = test_cleanup.c
