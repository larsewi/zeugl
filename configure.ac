#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([zeugl],[0.0.1],[https://github.com/larsewi/zeugl/issues],[zeugl],[https://github.com/larsewi/zeugl])
AC_CONFIG_SRCDIR([lib/zeugl.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

# Load macros
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

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h unistd.h sys/file.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_MODE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strerror
                stpcpy
                strdup])

AX_PTHREAD([
  AC_MSG_NOTICE([POSIX threads found and enabled.])
  LIBS="$LIBS $PTHREAD_LIBS"
  CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
],[
  AC_MSG_NOTICE([POSIX threads not found.])
])

AC_CONFIG_TESTDIR([tests])
AC_CONFIG_FILES([Makefile
                 cli/Makefile
                 lib/Makefile
                 tests/Makefile])
AC_OUTPUT
