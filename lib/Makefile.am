AM_CPPFLAGS = -include $(top_builddir)/config.h

include_HEADERS = zeugl.h

lib_LTLIBRARIES = libzeugl.la

libzeugl_la_SOURCES = zeugl.c \
    filecopy.h filecopy.c \
    immutable.h \
    signals.h signals.c \
    whackamole.h whackamole.c \
    logger.h utils.h

if IMMUTABLE_IOCTL
libzeugl_la_SOURCES += immutable_ioctl.c
endif

if IMMUTABLE_CHFLAGS
libzeugl_la_SOURCES += immutable_chflags.c
endif

if IMMUTABLE_WEAK
libzeugl_la_SOURCES += immutable_weak.c
endif

if !NDEBUG
    libzeugl_la_SOURCES += logger.c
endif
