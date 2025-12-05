ACLOCAL_AMFLAGS = -I m4
SUBDIRS = include lib cli man . tests

.PHONY: format check-format super-clean

format:
if HAVE_CLANG_FORMAT
	find $(top_srcdir)/{lib,include,cli} -name '*.c' -o -name '*.h' | xargs @CLANG_FORMAT@ -i --verbose
else
	echo "Cannot format C sources - please install clang-format" && false
endif # CLANG_FORMAT
if HAVE_SHFMT
	@SHFMT@ --write --simplify --list $(top_srcdir)/tests/*.sh
else
	echo "Cannot format shell scripts - please install shfmt and reconfigure" && false
endif # HAVE_SHFMT

check-format:
if HAVE_CLANG_FORMAT
	find $(top_srcdir)/{lib,include,cli} -name '*.c' -o -name '*.h' | xargs @CLANG_FORMAT@ --dry-run --Werror
else
	echo "Cannot check format on C sources - please install clang-format" && false
endif # CLANG_FORMAT
if HAVE_SHFMT
	@SHFMT@ --diff --simplify --list $(top_srcdir)/tests/*.sh
else
	echo "Cannot check format on shell scripts - please install shfmt and reconfigure" && false
endif # HAVE_SHFMT

super-clean:
if HAVE_GIT
	@GIT@ clean -fxd
else
	echo "Cannot super clean project - please install git and reconfigure" && false
endif # HAVE_SHFMT
