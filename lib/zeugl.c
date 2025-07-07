#include "config.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "logger.h"
#include "zeugl.h"

static int filecopy(int src, int dst) {
  char buffer[4096];

  int eof = 0;
  do {
    LOG_DEBUG("Reading from source file...");
    size_t n_read = 0;
    do {
      ssize_t ret = read(src, buffer + n_read, sizeof(buffer) - n_read);
      if (ret < 0) {
        if (errno == EINTR) {
          /* Interrupted */
          continue;
        }

        LOG_ERROR("Failed to read from file: %s", strerror(errno));
        return -1;
      }

      /* Is End-of-File reached? */
      eof = (ret == 0);

      n_read += (size_t)ret;
    } while (!eof && (n_read <= sizeof(buffer)));

    LOG_DEBUG("Writing to destination file...");
    size_t n_written = 0;
    do {
      ssize_t ret = write(dst, buffer + n_written, n_read - n_written);
      if (ret < 0) {
        if (errno == EINTR) {
          /* Interrupted */
          continue;
        }

        LOG_ERROR("Failed to write to file: %s", strerror(errno));
        return -1;
      }

      n_written += (size_t)ret;
    } while (n_written < n_read);
  } while (!eof);

  return 0;
}

static int atomic_filecopy(int src, int dst) {
  int ret = 0;

  LOG_DEBUG("Retrieving information about source file...");
  struct stat sb_before;
  if (fstat(src, &sb_before) != 0) {
    LOG_ERROR("Failed to retrieve information about source file: %s",
              strerror(errno));
    return -1;
  }

  LOG_DEBUG("Requesting shared lock for source file...");
  if (flock(src, LOCK_SH) != 0) {
    LOG_ERROR("Failed to get shared lock for source file: %s", strerror(errno));
    return -1;
  }

  LOG_DEBUG("Copying contents from source file to destination file...");
  if (filecopy(src, dst) != 0) {
    LOG_ERROR("Failed to copy contents from source file to destination file");
    goto FAIL;
  }

  LOG_DEBUG("Retrieving information about source file...", src);
  struct stat sb_after;
  if (fstat(src, &sb_after) != 0) {
    LOG_ERROR("Failed to retrieve information about source file: %s",
              strerror(errno));
    goto FAIL;
  }

  LOG_DEBUG("Checking if source file was modified during file copy...");
  if ((sb_before.st_mtim.tv_sec != sb_after.st_mtim.tv_sec) ||
      (sb_before.st_mtim.tv_nsec != sb_after.st_mtim.tv_nsec)) {
    LOG_WARNING(
        "Source file was modified while copying contents to destination file");
    errno = EBUSY;
    goto FAIL;
  }

  ret = 0;
FAIL:
  LOG_DEBUG("Releasing shared lock for source file...");
  if (flock(src, LOCK_UN) != 0) {
    LOG_ERROR("Failed to release shared lock for source file: %s",
              strerror(errno));
    return -1;
  }

  return ret;
}

int zopen(const char *orig_fname) {
  assert(orig_fname != NULL);

  int temp_fd = -1, orig_fd = -1;

  char temp_fname[PATH_MAX];
  if ((strlen(orig_fname) + strlen(".XXXXXX.tmp")) >= sizeof(temp_fname)) {
    errno = ENOBUFS;
    LOG_ERROR("Failed to create template name for temporary file: %s",
              strerror(errno));
    goto FAIL;
  }
  char *p = stpcpy(temp_fname, orig_fname);
  p = stpcpy(p, ".XXXXXX.tmp");

  LOG_DEBUG("Creating temporary file from template '%s'...", temp_fname);
  temp_fd = mkstemps(temp_fname, strlen(".tmp"));
  if (temp_fd < 0) {
    LOG_ERROR("Failed to create temporary file from template name '%s': %s",
              temp_fname, strerror(errno));
    goto FAIL;
  }

  LOG_DEBUG("Opening original file '%s' in read-only mode...", orig_fname);
  orig_fd = open(orig_fname, O_RDONLY);
  if (orig_fd < 0) {
    if (errno != ENOENT) {
      LOG_ERROR("Failed to open original file '%s': %s", orig_fname,
                strerror(errno));
      goto FAIL;
    }
  } else {
    LOG_DEBUG(
        "Copying content from original file '%s' to temporary file '%s'...",
        orig_fname, temp_fname);
    if (atomic_filecopy(orig_fd, temp_fd) != 0) {
      LOG_ERROR("Failed to copy content from original file '%s' to temporary "
                "file '%s': %s",
                orig_fname, temp_fname, strerror(errno));
      goto FAIL;
    }
  }

  if (orig_fd >= 0) {
    close(orig_fd);
  }
  return temp_fd;

FAIL:
  if (temp_fd >= 0) {
    close(temp_fd);
  }
  if (orig_fd >= 0) {
    close(orig_fd);
  }
  return -1;
}

int zclose(int fd) { return -1; }
