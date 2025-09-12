AM_CPPFLAGS = -include $(top_builddir)/config.h

include_HEADERS = zeugl.h

lib_LTLIBRARIES = libzeugl.la

libzeugl_la_SOURCES = zeugl.c \
    filecopy.h filecopy.c \
    signals.h signals.c \
    whackamole.h whackamole.c \
    logger.h utils.h

if !NDEBUG
    libzeugl_la_SOURCES += logger.c
endif
