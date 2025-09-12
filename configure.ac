#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([zeugl], [0.0.1],
        [https://github.com/larsewi/zeugl/issues],
        [zeugl], [https://github.com/larsewi/zeugl])
AC_CONFIG_SRCDIR([lib/zeugl.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

# Load macros.
m4_include([m4/ax_pthread.m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AM_PROG_AR

LT_INIT


AC_DEFINE([BUFFER_SIZE], 65536,
          [Buffer size used for file copying (default 64 KiB)])

# Check for debug option.
AC_ARG_ENABLE([debug],
              AS_HELP_STRING([--enable-debug], [enable debugging [default=no]]),
              [debug="$enableval"], [debug=no])
AC_MSG_CHECKING([for debug option])
if test "$debug" = yes; then
    AC_MSG_RESULT(yes)
    CFLAGS="$CFLAGS -g -O0 -Werror -Wall -Wextra -Wconversion -Wformat -Wshadow -Wcast-align -Wunused"
else
    AC_MSG_RESULT(no)
    CPPFLAGS="$CPPFLAGS -DNDEBUG"
fi
AM_CONDITIONAL([NDEBUG], [test "$debug" = "no"])

# Checks for libraries.
AX_PTHREAD

# Checks for header files.
AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADERS([fcntl.h
                  unistd.h
                  sys/file.h
                  stdint.h
                  inttypes.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_CHECK_FUNCS([strerror
                stpcpy
                strdup
                strtoul])

AC_CONFIG_TESTDIR([tests])
AC_CONFIG_FILES([Makefile
                 cli/Makefile
                 lib/Makefile
                 tests/Makefile])
AC_OUTPUT
