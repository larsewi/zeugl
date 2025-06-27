#ifndef __ZEUGL_ZEUGL_H__
#define __ZEUGL_ZEUGL_H__

#include <sys/types.h>

const char *zversion(void);

int zopen(const char *filename, int flags, mode_t mode);

int zclose(int fd);

#endif /* __ZEUGL_ZEUGL_H__ */
