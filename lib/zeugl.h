#ifndef __ZEUGL_ZEUGL_H__
#define __ZEUGL_ZEUGL_H__

#include <stdbool.h>

#define Z_CREATE 1 << 0
#define Z_APPEND 1 << 1
#define Z_TRUNCATE 1 << 2
#define Z_NOBLOCK 1 << 2

/**
 * @brief           Begins an  atomic  file  transaction  by  returning  a  file
 *                  descriptor to a temporary copy of  the  file.  Upon  calling
 *                  zclose(), you can chose whether to commit the transaction by
 *                  replacing the original file with the temporary file.
 *
 * @param filename  The file to start the transaction on. This file will be left
 *                  unmodified until the transaction is committed by a  call  to
 *                  zclose().
 *
 * @param flags     Zero or more file creation flags and file status  flags  can
 *                  be bitwise ORed in flags.  Please  note  that  the  file  is
 *                  always opened in read/write.
 *
 *                  Z_CREATE
 *                      if filename does not exist, create it as a regular file.
 *                      Note that this flag requires you  to  specify  the  mode
 *                      argument.
 *
 *                  Z_APPEND
 *                      the file offset is positioned at the  end  of  the  file
 *                      instead of at the start of the file. However,  the  file
 *                      offset is not repositioned to the end of the file before
 *                      each write like O_APPEND in open(2).
 *
 *                  Z_TRUNCATE
 *                      the content of the original is  never  copied  into  the
 *                      temporary copy.
 *
 *                  Z_NOBLOCK
 *                      the file does not block on advisory locking (i.e.,  file
 *                      locks). Furthermore, the function will not retry copying
 *                      the original file to the temporary copy  if  it  detects
 *                      that another process is writing  to  the  original  file
 *                      simultaneously. In all of these cases, the function will
 *                      return error and errno will be set to EBUSY. Please note
 *                      that this flag does not guarantee that the function call
 *                      does not block for any other reasons.
 *
 * @param mode      The mode argument specifies the file mode bits to be applied
 *                  when a new file is created. If Z_CREATE is not specified  in
 *                  flags, then mode is ignored (and can thus be specified as  0
 *                  or simply omitted). The mode argument must  be  supplied  if
 *                  Z_CREATE is specified. Failing to do so will cause arbitrary
 *                  bytes from the stack to be applied as the file mode.
 *
 * @return          Returns the file descriptor (a nonnegative integer)  of  the
 *                  temporary file. On error, a negative number is returned  and
 *                  errno is set to indicate the error.
 *
 * @note            There are some measures to try to detect if another  process
 *                  is writing to the original file while  zopen()  creates  the
 *                  temporary copy. However, this cannot  be  guaranteed  unless
 *                  the other processes modifying to the original file  respects
 *                  advisory locks (file locks). Upon detecting other  processes
 *                  writing to the original file while copying it, zopen()  will
 *                  continuously retry copying unless the Z_NOBLOCK bit  is  set
 *                  in the flag argument.
 */
int zopen(const char *filename, int flags, ... /* mode_t mode */);

/**
 * @brief           Commits or aborts an atomic file transaction. On  abort  the
 *                  temporary copy  of  the  file  is  removed.  On  commit  the
 *                  original file is atomically replaced by the temporary  file.
 *                  If multiple processes commit a file transaction simultaneou-
 *                  sly, this function guarantees that original file is replaced
 *                  once, by one of the temporary files. Any  of  the  remaining
 *                  temporary files are deleted.
 *
 * @param fd        The file descriptor of a file opened with zopen() or -1  for
 *                  no operation.
 *
 * @param commit    If  true,  the  transaction  is  committed.  Otherwise,  the
 *                  transaction is aborted.
 *
 * @return          On success, 0 is returned. On error, a negative  number  is
 *                  returned and errno is set to indicate the error.
 */
int zclose(int fd, bool commit);

#endif /* __ZEUGL_ZEUGL_H__ */
