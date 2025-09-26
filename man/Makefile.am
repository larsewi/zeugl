man_MANS = zeugl.1 zopen.3
man_LINKS = zclose.3:zopen.3

CLEANFILES = $(man_MANS)
EXTRA_DIST = zeugl.1.in zopen.3.in

# Suffix rules to generate man pages from .in files
SUFFIXES = .1.in .1 .3.in .3

.1.in.1:
	$(AM_V_GEN)$(SED) \
		-e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g' \
		-e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g' \
		-e 's|@PACKAGE_BUGREPORT[@]|$(PACKAGE_BUGREPORT)|g' \
		< $< > $@

.3.in.3:
	$(AM_V_GEN)$(SED) \
		-e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g' \
		-e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g' \
		-e 's|@PACKAGE_BUGREPORT[@]|$(PACKAGE_BUGREPORT)|g' \
		< $< > $@
