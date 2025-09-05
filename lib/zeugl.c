#include "config.h"

#include <asm-generic/errno-base.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "filecopy.h"
#include "logger.h"
#include "wackamole.h"
#include "zeugl.h"

struct zfile {
  char *orig;
  char *temp;
  char *mole;
  int fd;
  mode_t mode;
  struct zfile *next;
};

#ifdef HAVE_PTHREAD
static pthread_mutex_t OPEN_FILES_MUTEX = PTHREAD_MUTEX_INITIALIZER;
#endif /* HAVE_PTHREADS */
static struct zfile *OPEN_FILES = NULL;

int zopen(const char *fname, int flags, ...) {
  assert(fname != NULL);

  struct zfile *file = NULL;

  file = calloc(1, sizeof(struct zfile));
  if (file == NULL) {
    LOG_DEBUG("Failed to allocate memory: %s", strerror(errno));
    return -1;
  }
  file->fd = -1;

  file->orig = strdup(fname);
  if (file->orig == NULL) {
    LOG_DEBUG("Failed to allocate memory: %s", strerror(errno));
    goto FAIL;
  }

  file->temp = malloc(strlen(file->orig) + strlen(".XXXXXX") + 1);
  if (file->temp == NULL) {
    LOG_DEBUG("Failed to allocate memory: %s", strerror(errno));
    goto FAIL;
  }

  /* Create template filename */
  stpcpy(stpcpy(file->temp, file->orig), ".XXXXXX");

  file->fd = mkstemp(file->temp);
  if (file->fd < 0) {
    LOG_DEBUG("Failed to create temporary file: %s", strerror(errno));
    goto FAIL;
  }
  LOG_DEBUG("Created temporary file '%s' (fd = %d)", file->temp, file->fd);

  /* Extract mode argument from zopen() if Z_CREATE was specified */
  mode_t mode;
  if (flags & Z_CREATE) {
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, mode_t) & 0777; /* Don't keep user bit */
    va_end(ap);
  }

  if (flags & Z_TRUNCATE) {
    struct stat sb;
    if (lstat(file->orig, &sb) == 0) {
      file->mode = sb.st_mode & 0777; /* Don't keep user bit */
      LOG_DEBUG("Original file '%s' exists: Using original mode %04jo",
                file->orig, (uintmax_t)file->mode);
    } else {
      if ((flags & Z_CREATE) && (errno == ENOENT)) {
        /* Use mode specified in argument */
        file->mode = mode;
        LOG_DEBUG("Original file '%s' does not exist: "
                  "Using specified mode %04jo",
                  file->orig, (uintmax_t)file->mode);
      } else {
        LOG_DEBUG("Failed to get mode from original file '%s': %s", file->orig,
                  strerror(errno));
        goto FAIL;
      }
    }
  } else {
    int fd = open(file->orig, O_RDONLY);
    if (fd < 0) {
      if ((flags & Z_CREATE) && (errno == ENOENT)) {
        /* If Z_CREATE was specified, then ENOENT can be expected */
        /* Use mode specified in argument */
        file->mode = mode;
        LOG_DEBUG("Original file '%s' does not exist: "
                  "Using specified mode %04jo",
                  file->orig, (uintmax_t)file->mode);
      } else {
        LOG_DEBUG("Failed to open original file '%s' in read-only mode: %s",
                  file->orig, strerror(errno));
        goto FAIL;
      }
    } else {
      LOG_DEBUG("Opened original file '%s' (fd = %d) in read-only mode",
                file->orig, fd);

      struct stat sb;
      if (fstat(fd, &sb) != 0) {
        LOG_DEBUG("Failed to get mode from original file '%s' (fd = %d): %s",
                  file->orig, fd, strerror(errno));
        goto FAIL;
      }
      file->mode = sb.st_mode & 0777; /* Don't keep user bit */
      LOG_DEBUG("Using mode %04jo from original file '%s' (fd = %d)",
                (uintmax_t)file->mode, file->orig, fd);

      if (!atomic_filecopy(fd, file->fd, false)) {
        LOG_DEBUG("Failed to copy content from original file '%s' (fd = %d) to "
                  "temporary file '%s' (fd = %d): %s",
                  file->orig, fd, file->temp, file->fd, strerror(errno));
        if (close(fd) == 0) {
          LOG_DEBUG("Closed original file '%s' (fd = %d)", file->orig, fd);
        } else {
          LOG_DEBUG("Failed to close original file '%s' (fd = %d): %s",
                    file->orig, fd, strerror(errno));
        }
        goto FAIL;
      }
      LOG_DEBUG("Successfully copied content from original file '%s' (fd = %d) "
                "to temporary file '%s' (fd = %d)",
                file->orig, fd, file->temp, file->fd);

      if (close(fd) == 0) {
        LOG_DEBUG("Closed original file '%s' (fd = %d)", file->orig, fd);
      } else {
        LOG_DEBUG("Failed to close original file '%s' (fd = %d): %s",
                  file->orig, fd, strerror(errno));
      }
    }
  }

  if (!(flags & (Z_APPEND | Z_TRUNCATE))) {
    if (lseek(file->fd, 0, SEEK_SET) != 0) {
      LOG_DEBUG("Failed to reposition file offset to the beginning of the file "
                "'%s' (fd = %d): %s",
                file->temp, file->fd, strerror(errno));
      goto FAIL;
    }
    LOG_DEBUG("Repositioned the file offset to the beginning of the file "
              "'%s' (fd = %d)",
              file->temp, file->fd);
  }

#ifdef HAVE_PTHREAD
  int ret = pthread_mutex_lock(&OPEN_FILES_MUTEX);
  if (ret != 0) {
    LOG_DEBUG("Failed to acquire mutex protecting list of open files: %s",
              strerror(ret));
    goto FAIL;
  }
  LOG_DEBUG("Successfully acquired mutex protecting list of open files");
#endif /* HAVE_PTHREAD */

  file->next = OPEN_FILES;
  OPEN_FILES = file;
  LOG_DEBUG("Added file to list of open files "
            "(orig = '%s', temp = '%s', fd = %d, mode = %04jo)",
            file->orig, file->temp, file->fd, file->mode);

#ifdef HAVE_PTHREAD
  ret = pthread_mutex_unlock(&OPEN_FILES_MUTEX);
  if (ret != 0) {
    LOG_DEBUG("Failed to release mutex protecting list of open files: %s",
              strerror(ret));
    goto FAIL;
  }
  LOG_DEBUG("Successfully released mutex protecting list of open files");
#endif /* HAVE_PTHREAD */

  return file->fd;

FAIL:
  if (file != NULL) {
    int save_errno = errno;

    free(file->orig);
    if (file->fd >= 0) {
      if (close(file->fd) == 0) {
        LOG_DEBUG("Closed temporary file '%s' (fd = %d)", file->temp, file->fd);
      } else {
        LOG_DEBUG("Failed to close temporary file '%s' (fd = %d): %s",
                  file->temp, file->fd, strerror(errno));
      }
    }

    if (file->temp != NULL) {
      if (unlink(file->temp) == 0) {
        LOG_DEBUG("Deleted temporary file '%s'", file->temp);
      } else {
        LOG_DEBUG("Failed to delete temporary file '%s': %s", file->temp,
                  strerror(errno));
      }
      free(file->temp);
    }
    free(file);

    /* Restore errno */
    errno = save_errno;
  }

  return -1;
}

int zclose(int fd, bool commit) {
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

#ifdef HAVE_PTHREAD
  int err = pthread_mutex_lock(&OPEN_FILES_MUTEX);
  if (err != 0) {
    LOG_DEBUG("Failed to acquire mutex protecting list of open files: %s",
              strerror(err));
    return -1;
  }
  LOG_DEBUG("Successfully acquired mutex protecting list of open files");
#endif /* HAVE_PTHREAD */

  int ret = -1;

  LOG_DEBUG("Looking for file with matching file descriptor %d...", fd);
  struct zfile *prev = NULL, *file = OPEN_FILES;
  while ((file != NULL) && (file->fd != fd)) {
    prev = file;
    file = file->next;
  }

  if ((file == NULL) || (file->fd != fd)) {
    LOG_DEBUG("Did not find a file with matching file descriptor (fd = %d): "
              "This file was not opened with zopen()",
              fd);
    /* This file was not opened with zopen(). Hence, no need to perform the
     * wack-a-mole. */
    goto FAIL;
  }
  LOG_DEBUG("Found file '%s' with matching file descriptor (fd = %d): "
            "This file was opened with zopen()",
            file->temp, file->fd);

  if (commit) {
    if (chmod(file->temp, file->mode) != 0) {
      LOG_DEBUG("Failed to change file mode for file '%s' to %04jo: %s",
                file->temp, (uintmax_t)file->mode, strerror(errno));
      goto FAIL;
    }
    LOG_DEBUG("Changed file mode for file '%s' to %04jo", file->temp,
              (uintmax_t)file->mode);

    if (!wack_a_mole(file->orig, file->temp)) {
      LOG_DEBUG("Failed to execute wack-a-mole algorithm "
                "(orig = '%s', temp = '%s'): %s",
                file->orig, file->temp, strerror(errno));
      goto FAIL;
    }
    LOG_DEBUG("Successfully executed wack-a-mole algorithm "
              "(orig = '%s', temp = '%s')",
              file->orig, file->temp);
  } else {
    LOG_DEBUG("Aborting file transaction");
    if (unlink(file->temp) != 0) {
      LOG_DEBUG("Failed to delete temporary file '%s': %s", file->temp,
                strerror(errno));
      goto FAIL;
    }
    LOG_DEBUG("Deleted temporary file '%s'", file->temp);
  }

  ret = 0;
FAIL:

  if (prev == NULL) {
    /* The file was the first element in the list */
    OPEN_FILES = file->next;
  } else {
    /* The file was not the first element in the list. We need to merge left
     * and right before removal. */
    prev->next = file->next;
  }

#ifdef HAVE_PTHREAD
  err = pthread_mutex_unlock(&OPEN_FILES_MUTEX);
  if (err != 0) {
    LOG_DEBUG("Failed to release mutex protecting list of open files: %s",
              strerror(err));
    ret = -1;
  }
  LOG_DEBUG("Successfully released mutex protecting list of open files");
#endif /* HAVE_PTHREAD */

  if (file != NULL) {
    free(file->orig);
    free(file->temp);
    free(file->mole);
    free(file);
  }

  return ret;
}
