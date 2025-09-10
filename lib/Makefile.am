AM_CPPFLAGS = -include $(top_builddir)/config.h

include_HEADERS = zeugl.h

lib_LTLIBRARIES = libzeugl.la

libzeugl_la_SOURCES = zeugl.c \
    filecopy.h filecopy.c \
    whackamole.h whackamole.c \
    logger.h

if !NDEBUG
    libzeugl_la_SOURCES += logger.c
endif
