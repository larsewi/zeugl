ACLOCAL_AMFLAGS = -I m4
SUBDIRS = lib cli . tests

format:
	clang-format -i --verbose **/*.{c,h}
	shfmt --write --simplify --list **/*.sh

check-format:
	clang-format --dry-run --Werror **/*.{c,h}
	shfmt --diff --simplify --list **/*.sh

super-clean:
	if command -v git; then git clean -fxd; fi
