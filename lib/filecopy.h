#ifndef __ZEUGL_FILECOPY_H__
#define __ZEUGL_FILECOPY_H__

#include <stdbool.h>

bool filecopy(int src, int dst);

bool safe_filecopy(int src, int dst);

bool atomic_filecopy(int src, int dst, bool no_block);

#endif /* __ZEUGL_FILECOPY_H__ */
