#ifndef __ZEUGL_ZEUGL_H__
#define __ZEUGL_ZEUGL_H__

#include <stdbool.h>

#define Z_CREATE 1 << 0
#define Z_APPEND 1 << 1
#define Z_TRUNCATE 1 << 2
#define Z_NOBLOCK 1 << 3

/**
 * @brief           Begins an atomic file transaction.
 * @param filename  The file to begin transaction on.
 * @param flags     File creation flags and file status flags.
 * @param mode      File mode bits to be applied when a new file is created.
 * @return          A file descriptor on success or a negative number on error.
 * On error errno is set to indicate the error.
 */
int zopen(const char *filename, int flags, ... /* mode_t mode */);

/**
 * @brief           Commits or aborts an atomic file transaction.
 * @param fd        A file descriptor of a file or -1 for no operation.
 * @param commit    If true, the transaction is committed. Otherwise, the
 * transaction is aborted.
 * @return          Returns zero on success or a negative number on error. On
 * error errno is set to indicate the error.
 */
int zclose(int fd, bool commit);

#endif /* __ZEUGL_ZEUGL_H__ */
