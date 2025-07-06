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

int zopen(const char *orig_fname) {
  assert(orig_fname != NULL);

  int ret = -1, temp_fd = -1, orig_fd = -1;

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
  temp_fd = ret = mkstemps(temp_fname, strlen(".tmp"));
  if (ret < 0) {
    LOG_ERROR("Failed to create temporary file from template name '%s': %s",
              temp_fname, strerror(errno));
    goto FAIL;
  }

  LOG_DEBUG("Opening original file '%s' in read-only mode...", orig_fname);
  orig_fd = ret = open(orig_fname, O_RDONLY);
  if (ret < 0) {
    if (errno != ENOENT) {
      LOG_ERROR("Failed to open original file '%s': %s", orig_fname,
                strerror(errno));
      goto FAIL;
    }
  }

  LOG_DEBUG("Retrieving information about original file '%s'...", orig_fname);
  struct stat sb;
  ret = fstat(orig_fd, &sb);
  if (ret != 0) {
    LOG_ERROR("Failed to retrieve information about original file '%s': %s",
              orig_fname, strerror(errno));
    goto FAIL;
  }

  LOG_DEBUG("Requesting shared lock for file '%s'...", orig_fname);
  ret = flock(orig_fd, LOCK_SH);
  if (ret != 0) {
    LOG_ERROR("Failed to get shared lock for file '%s': %s", orig_fname,
              strerror(errno));
    goto FAIL;
  }

  LOG_DEBUG(
      "Copying contents from original file '%s' to temporary file '%s'...",
      orig_fname, temp_fname);
  ret = filecopy(orig_fd, temp_fd);
  if (ret != 0) {
    LOG_ERROR("Failed to copy contents from original file '%s' to temporary "
              "file '%s'",
              orig_fname, temp_fname);
    goto FAIL;
  }

FAIL:
  LOG_DEBUG("Releasing shared lock for file '%s'...", orig_fname);
  ret = flock(orig_fd, LOCK_UN);
  if (ret != 0) {
    LOG_ERROR("Failed to release shared lock for file '%s': %s", orig_fname,
              strerror(errno));
    goto FAIL;
  }

  if (temp_fd >= 0) {
    close(temp_fd);
  }
  if (orig_fd >= 0) {
    close(orig_fd);
  }
  return ret;
}

int zclose(int fd) { return -1; }
