#ifndef __ZEUGL_ZEUGL_H__
#define __ZEUGL_ZEUGL_H__

#include <stdbool.h>

int zopen(const char *filename);

int zclose(int fd, bool commit);

#endif /* __ZEUGL_ZEUGL_H__ */
