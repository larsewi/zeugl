ACLOCAL_AMFLAGS = -I m4
SUBDIRS = lib cli . tests

format:
	if command -v clang-format; then clang-format -i --verbose **/*.{c,h}; fi
	if command -v shfmt; then shfmt --write --simplify --list **/*.sh; fi

super-clean:
	if command -v git; then git clean -fxd; fi
