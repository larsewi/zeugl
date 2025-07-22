#include "config.h"

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

struct zfile {
  char *orig;
  char *temp;
  char *mole;
  int fd;
  struct zfile *next;
};

static struct zfile *open_files = NULL;

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

        LOG_DEBUG("Failed to read from source file (fd = %d): %s", src,
                  strerror(errno));
        return -1;
      }

      /* Is End-of-File reached? */
      eof = (ret == 0);

      n_read += (size_t)ret;
    } while (!eof && (n_read <= sizeof(buffer)));
    LOG_DEBUG("Read %zu bytes from source file (fd = %d)", n_read, src);

    size_t n_written = 0;
    do {
      ssize_t ret = write(dst, buffer + n_written, n_read - n_written);
      if (ret < 0) {
        if (errno == EINTR) {
          /* Interrupted */
          continue;
        }

        LOG_DEBUG("Failed to write content to destination file (fd = %d): %s",
                  dst, strerror(errno));
        return -1;
      }

      n_written += (size_t)ret;
    } while (n_written < n_read);
    LOG_DEBUG("Wrote %zu bytes to destination file (fd = %d)", n_written, dst);
  } while (!eof);

  return 0;
}

static int atomic_filecopy(int src, int dst) {
  int ret = 0;

  struct stat sb_before;
  if (fstat(src, &sb_before) != 0) {
    LOG_DEBUG("Failed to retrieve information about source file (fd = %d): %s",
              src, strerror(errno));
    return -1;
  }
  LOG_DEBUG("Retrieved information about source file (fd = %d): Before copy",
            src);

  if (flock(src, LOCK_SH) != 0) {
    LOG_DEBUG("Failed to get shared lock for source file (fd = %d): %s", src,
              strerror(errno));
    return -1;
  }
  LOG_DEBUG("Requested shared lock for source file (fd = %d)", src);

  if (filecopy(src, dst) != 0) {
    LOG_DEBUG("Failed to copy content from source file (fd = %d) to "
              "destination file (fd = %d)",
              src, dst);
    goto FAIL;
  }
  LOG_DEBUG(
      "Copied content from source file (fd = %d) to destination file (fd = %d)",
      src, dst);

  struct stat sb_after;
  if (fstat(src, &sb_after) != 0) {
    LOG_DEBUG("Failed to retrieve information about source file (fd = %d): %s",
              src, strerror(errno));
    goto FAIL;
  }
  LOG_DEBUG("Retrieved information about source file (fd = %d): After copy",
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

  ret = 0;
FAIL:
  if (flock(src, LOCK_UN) != 0) {
    LOG_DEBUG("Failed to release shared lock for source file (fd = %d): %s",
              src, strerror(errno));
    return -1;
  }
  LOG_DEBUG("Released shared lock for source file (fd = %d)", src);

  return ret;
}

int zopen(const char *fname) {
  assert(fname != NULL);

  struct zfile *file = NULL;
  int fd = -1;

  file = calloc(1, sizeof(struct zfile));
  if (file == NULL) {
    LOG_DEBUG("Failed to allocate memory for zfile: %s", strerror(errno));
    return -1;
  }
  file->fd = -1;

  file->orig = strdup(fname);
  if (file->orig == NULL) {
    LOG_DEBUG("Failed to allocate memory for copy of original filename: %s",
              strerror(errno));
    goto FAIL;
  }

  file->temp = malloc(strlen(file->orig) + strlen(".XXXXXX.tmp") + 1);
  if (file->temp == NULL) {
    LOG_DEBUG("Failed to allocate memory for temporary filename: %s",
              strerror(errno));
    goto FAIL;
  }

  stpcpy(stpcpy(file->temp, file->orig), ".XXXXXX.tmp");
  LOG_DEBUG("Generated template for temporary filename '%s'", file->temp);

  file->fd = mkstemps(file->temp, strlen(".tmp"));
  if (file->fd < 0) {
    LOG_DEBUG("Failed to create temporary file: %s", strerror(errno));
    goto FAIL;
  }
  LOG_DEBUG("Created temporary file '%s' (fd = %d)", file->temp, file->fd);

  fd = open(file->orig, O_RDONLY);
  if (fd < 0) {
    if (errno != ENOENT) {
      LOG_DEBUG("Failed to open original file '%s' in read-only mode: %s",
                file->orig, strerror(errno));
      goto FAIL;
    }
  } else {
    LOG_DEBUG("Opened original file '%s' (fd = %d) in read-only mode",
              file->orig, fd);

    if (atomic_filecopy(fd, file->fd) != 0) {
      LOG_DEBUG("Failed to copy content from original file '%s' (fd = %d) to "
                "temporary file '%s' (fd = %d): %s",
                file->orig, fd, file->temp, file->fd, strerror(errno));
      goto FAIL;
    }
    LOG_DEBUG("Copied content from original file '%s' (fd = %d) to temporary "
              "file '%s' (fd = %d)",
              file->orig, fd, file->temp, file->fd);
  }

  close(fd);
  LOG_DEBUG("Closed original file '%s' (fd = %d)", file->orig, fd);

  file->next = open_files;
  open_files = file;

  return file->fd;

FAIL:
  LOG_DEBUG("Entered failure clean-up");
  if (file != NULL) {
    free(file->orig);
    free(file->temp);
    if (file->fd >= 0) {
      close(file->fd);
    }
    free(file);
  }

  if (fd >= 0) {
    close(fd);
  }

  return -1;
}

int create_mole(struct zfile *file) {
  file->mole = malloc(strlen(file->temp) + strlen(".mole") + 1);
  if (file->mole == NULL) {
    LOG_DEBUG("Failed to allocate memory for mole filename: %s",
              strerror(errno));
    return -1;
  }

  stpcpy(stpcpy(file->mole, file->temp), ".mole");
  LOG_DEBUG("Generated filename for mole '%s'", file->mole);

  if (rename(file->temp, file->mole) != 0) {
    LOG_DEBUG("Failed to rename '%s' to '%s': %s", file->temp, file->mole,
              strerror(errno));
    return -1;
  }
  LOG_DEBUG("Renamed '%s' to '%s'", file->temp, file->mole);

  return 0;
}

int wack_mole(const char *fname) { return 0; }

int zclose(int fd) {
  /* Consider -1 a no-op */
  if (fd == -1) {
    return 0;
  }

  /* We don't need the file descriptor anymore */
  if (close(fd) != 0) {
    LOG_DEBUG("Failed to close file (fd = %d)", fd);
    return -1;
  }
  LOG_DEBUG("Closed file (fd = %d)", fd);

  LOG_DEBUG("Looking for file with matching file descriptor %d...", fd);
  struct zfile *file = open_files;
  while ((file != NULL) && (file->fd != fd)) {
    file = file->next;
  }

  if ((file == NULL) || (file->fd != fd)) {
    LOG_DEBUG("Did not find a file with matching file descriptor (fd = %d): "
              "This file was not opened with zopen()",
              fd);
    /* This file was not opened with zopen(). Hence, no need to perform the
     * wack-a-mole. */
    return 0;
  }
  LOG_DEBUG("Found file '%s' with matching file descriptor (fd = %d): "
            "This file was opened with zopen()",
            file->temp, file->fd);

  int ret = create_mole(file);
  if (ret != 0) {
    LOG_DEBUG("Failed to create mole from temporary file '%s'", file->temp);
  }
  LOG_DEBUG("Created a mole '%s' from temporary file '%s' (fd = %d)",
            file->mole, file->temp, file->fd);

  free(file->orig);
  free(file->temp);
  free(file->mole);
  free(file);

  return ret;
}
