AM_CFLAGS = -Wall -Wextra -Wconversion -Wformat
AM_CPPFLAGS = -include $(top_builddir)/config.h

include_HEADERS = zeugl.h

lib_LTLIBRARIES = libzeugl.la

libzeugl_la_SOURCES = zeugl.c \
    logger.h logger.c \
    utils.h utils.c
