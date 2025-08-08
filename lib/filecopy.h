#ifndef __ZEUGL_FILECOPY_H__
#define __ZEUGL_FILECOPY_H__

#include <stdbool.h>

bool filecopy(int src, int dst);

bool atomic_filecopy(int src, int dst);

#endif /* __ZEUGL_FILECOPY_H__ */
