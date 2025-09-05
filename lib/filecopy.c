#include "config.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "filecopy.h"
#include "logger.h"

bool filecopy(int src, int dst) {
  char buffer[4096];

  int eof = 0;
  do {
    size_t n_read = 0;
    do {
      ssize_t ret = read(src, buffer + n_read, sizeof(buffer) - n_read);
      if (ret < 0) {
        if (errno == EINTR) {
          /* Interrupted! It happens, just continue... */
          continue;
        }

        LOG_DEBUG("Failed to read from source file (fd = %d): %s", src,
                  strerror(errno));
        return false;
      }

      /* Is End-of-File reached? */
      eof = (ret == 0);

      n_read += (size_t)ret;
    } while (!eof && (n_read < sizeof(buffer)));
    LOG_DEBUG("Read %zu bytes from source file (fd = %d)", n_read, src);

    size_t n_written = 0;
    do {
      ssize_t ret = write(dst, buffer + n_written, n_read - n_written);
      if (ret < 0) {
        if (errno == EINTR) {
          /* Interrupted! It happens, just continue... */
          continue;
        }

        LOG_DEBUG("Failed to write content to destination file (fd = %d): %s",
                  dst, strerror(errno));
        return false;
      }

      n_written += (size_t)ret;
    } while (n_written < n_read);
    LOG_DEBUG("Wrote %zu bytes to destination file (fd = %d)", n_written, dst);
  } while (!eof);

  return true;
}

bool atomic_filecopy(int src, int dst) {
  bool success = false;

  struct stat sb_before;
  if (fstat(src, &sb_before) != 0) {
    LOG_DEBUG("Failed to retrieve mtime from source file (fd = %d): %s", src,
              strerror(errno));
    return false;
  }
  LOG_DEBUG(
      "Retrieved mtime (%jd s, %jd ns) from source file (fd = %d) before copy",
      (uintmax_t)sb_before.st_mtim.tv_sec, (uintmax_t)sb_before.st_mtim.tv_nsec,
      src);

  if (flock(src, LOCK_SH) != 0) {
    LOG_DEBUG("Failed to get shared lock for source file (fd = %d): %s", src,
              strerror(errno));
    return false;
  }
  LOG_DEBUG("Requested shared lock for source file (fd = %d)", src);

  if (!filecopy(src, dst)) {
    LOG_DEBUG("Failed to copy content from source file (fd = %d) to "
              "destination file (fd = %d): %s",
              src, dst, strerror(errno));
    goto FAIL;
  }
  LOG_DEBUG("Successfully copied content from source file (fd = %d) to "
            "destination file (fd = %d)",
            src, dst);

  struct stat sb_after;
  if (fstat(src, &sb_after) != 0) {
    LOG_DEBUG("Failed to retrieve mtime from source file (fd = %d): %s", src,
              strerror(errno));
    goto FAIL;
  }
  LOG_DEBUG(
      "Retrieved mtime (%jd s, %jd ns) from source file (fd = %d) after copy",
      (uintmax_t)sb_before.st_mtim.tv_sec, (uintmax_t)sb_before.st_mtim.tv_nsec,
      src);

  if ((sb_before.st_mtim.tv_sec != sb_after.st_mtim.tv_sec) ||
      (sb_before.st_mtim.tv_nsec != sb_after.st_mtim.tv_nsec)) {
    LOG_DEBUG("Source file (fd = %d) was modified while copying contents to "
              "destination file (%d)",
              src, dst);
    errno = EBUSY;
    goto FAIL;
  }
  LOG_DEBUG("Source file (fd = %d) appears to not be modified during file copy",
            src);

  success = true;
FAIL:
  if (flock(src, LOCK_UN) != 0) {
    LOG_DEBUG("Failed to release shared lock for source file (fd = %d): %s",
              src, strerror(errno));
    return false;
  }
  LOG_DEBUG("Released shared lock for source file (fd = %d)", src);

  return success;
}
