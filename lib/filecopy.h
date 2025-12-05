#ifndef __ZEUGL_FILECOPY_H__
#define __ZEUGL_FILECOPY_H__

#include <stdbool.h>

bool zeugl_filecopy(int src, int dst);

bool zeugl_safe_filecopy(int src, int dst, bool no_block);

bool zeugl_atomic_filecopy(int src, int dst, bool no_block);

#endif /* __ZEUGL_FILECOPY_H__ */
