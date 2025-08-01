#ifndef __ZEUGL_ZEUGL_H__
#define __ZEUGL_ZEUGL_H__

#include <stdbool.h>

#define Z_CREATE 1 << 0
#define Z_APPEND 1 << 1
#define Z_TRUNCATE 1 << 2

/**
 * @brief           Begins an  atomic  file  transaction  by  returning  a  file
 *                  descriptor to a temporary copy of  the  file.  Upon  calling
 *                  zclose() you can chose whether to commit the transaction  by
 *                  replacing the original file with the temporary file.
 *
 * @param filename  The file to start the transaction on. This file will be left
 *                  unmodified until the transaction is committed by a  call  to
 *                  zclose().
 *
 * @param flags     Zero or more file creation flags and file status  flags  can
 *                  be bitwise ORed in flags.
 *
 *                  Z_CREATE
 *                      if filename does not exist, create it as a regular file.
 *
 *                  Z_APPEND
 *                      the file offset is positioned at the  end  of  the  file
 *                      instead of at the start of the file.
 *
 *                  Z_TRUNCATE
 *                      the content of the original is  never  copied  into  the
 *                      temporary copy.
 *
 * @param mode      The mode argument specifies the file mode bits to be applied
 *                  when a new file is created. If Z_CREATE is not specified  in
 *                  flags, then mode is ignored (and can thus be specified as  0
 *                  or simply omitted). The mode argument must  be  supplied  if
 *                  Z_CREATE is specified. Failing to do so will cause arbitrary
 *                  bytes from the stack to be applied as the file mode.
 *
 * @return          Return the file descriptor (a nonnegative  integer)  of  the
 *                  temporary file. On error, -1 is returned and errno is set to
 *                  indicate the error.
 */
int zopen(const char *filename, int flags, ... /* mode_t mode */);

/**
 * @brief
 *
 * @param fd
 *
 * @param commit
 */
int zclose(int fd, bool commit);

#endif /* __ZEUGL_ZEUGL_H__ */
