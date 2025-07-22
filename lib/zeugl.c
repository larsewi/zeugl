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

int zopen(const char *fname) {
  assert(fname != NULL);

  struct zfile *file = NULL;
  int fd = -1;

  file = calloc(1, sizeof(struct zfile));
  if (file == NULL) {
    LOG_ERROR("Failed to allocate memory: %s", strerror(errno));
    return -1;
  }
  file->fd = -1;

  file->orig = strdup(fname);
  if (file->orig == NULL) {
    LOG_ERROR("Failed to allocate memory: %s", strerror(errno));
    goto FAIL;
  }

  file->temp = malloc(strlen(file->orig) + strlen(".XXXXXX.tmp") + 1);
  if (file->temp == NULL) {
    LOG_ERROR("Failed to allocate memory: %s", strerror(errno));
    goto FAIL;
  }
  stpcpy(stpcpy(file->temp, file->orig), ".XXXXXX.tmp");

  LOG_DEBUG("Creating temporary file from template '%s'...", file->temp);
  file->fd = mkstemps(file->temp, strlen(".tmp"));
  if (file->fd < 0) {
    LOG_ERROR("Failed to create temporary file from template name '%s': %s",
              file->temp, strerror(errno));
    goto FAIL;
  }

  LOG_DEBUG("Opening original file '%s' in read-only mode...", file->orig);
  fd = open(file->orig, O_RDONLY);
  if (fd < 0) {
    if (errno != ENOENT) {
      LOG_ERROR("Failed to open original file '%s': %s", file->orig,
                strerror(errno));
      goto FAIL;
    }
  } else {
    LOG_DEBUG(
        "Copying content from original file '%s' to temporary file '%s'...",
        file->orig, file->temp);
    if (atomic_filecopy(fd, file->fd) != 0) {
      LOG_ERROR("Failed to copy content from original file '%s' to temporary "
                "file '%s': %s",
                file->orig, file->temp, strerror(errno));
      goto FAIL;
    }
  }

  LOG_DEBUG("Closing original file '%s'...", file->orig);
  close(fd);

  file->next = open_files;
  open_files = file;

  return file->fd;

FAIL:
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
    LOG_ERROR("Failed to allocate memory: %s", strerror(errno));
    return -1;
  }
  stpcpy(stpcpy(file->mole, file->temp), ".mole");

  LOG_DEBUG("Renaming '%s' to '%s'...", file->temp, file->mole);
  if (rename(file->temp, file->mole) != 0) {
    LOG_ERROR("Failed to rename '%s' to '%s': %s", file->temp, file->mole,
              strerror(errno));
    return -1;
  }

  return 0;
}

int wack_mole(const char *fname) { return 0; }

int zclose(int fd) {
  /* Consider -1 a no-op */
  if (fd == -1) {
    return 0;
  }

  struct zfile *file = open_files;
  while ((file != NULL) && (file->fd != fd)) {
    file = file->next;
  }

  /* We don't need the file descriptor anymore */
  LOG_DEBUG("Closing file descriptor %d...", fd);
  if (close(fd) != 0) {
    LOG_ERROR("Failed to close file");
    return -1;
  }

  if ((file == NULL) || (file->fd != fd)) {
    LOG_DEBUG("This file was not opened with zopen()");
    /* This file was not opened with zopen(). Hence, no need to perform the
     * wack-a-mole. */
    return 0;
  }
  LOG_DEBUG("This file was opened with zopen()");

  LOG_DEBUG("Creating mole...");
  int ret = create_mole(file);
  if (ret != 0) {
    LOG_ERROR("Failed to create mole");
  }

  free(file->orig);
  free(file->temp);
  free(file->mole);
  free(file);

  return ret;
}
