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
  char *fname;
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

int zopen(const char *orig_fname) {
  assert(orig_fname != NULL);

  struct zfile *temp = NULL;
  int orig_fd = -1;

  temp = calloc(1, sizeof(struct zfile));
  if (temp == NULL) {
    LOG_ERROR("Failed to allocate memory: %s", strerror(errno));
    goto FAIL;
  }
  temp->fd = -1;

  temp->fname = malloc(strlen(orig_fname) + strlen(".XXXXXX.tmp") + 1);
  if (temp->fname == NULL) {
    LOG_ERROR("Failed to allocate memory: %s", strerror(errno));
    goto FAIL;
  }
  stpcpy(stpcpy(temp->fname, orig_fname), ".XXXXXX.tmp");

  LOG_DEBUG("Creating temporary file from template '%s'...", temp->fname);
  temp->fd = mkstemps(temp->fname, strlen(".tmp"));
  if (temp->fd < 0) {
    LOG_ERROR("Failed to create temporary file from template name '%s': %s",
              temp->fname, strerror(errno));
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
        orig_fname, temp->fname);
    if (atomic_filecopy(orig_fd, temp->fd) != 0) {
      LOG_ERROR("Failed to copy content from original file '%s' to temporary "
                "file '%s': %s",
                orig_fname, temp->fname, strerror(errno));
      goto FAIL;
    }
  }

  LOG_DEBUG("Closing original file '%s'...", orig_fname);
  close(orig_fd);

  temp->next = open_files;
  open_files = temp;

  return temp->fd;

FAIL:
  if (temp != NULL) {
    free(temp->fname);
    if (temp->fd >= 0) {
      close(temp->fd);
    }
    free(temp);
  }

  if (orig_fd >= 0) {
    close(orig_fd);
  }

  return -1;
}

int create_mole(const char *oldname) {
  int ret = -1;

  char *newname = malloc(strlen(oldname) + strlen(".mole") + 1);
  if (newname == NULL) {
    LOG_ERROR("Failed to allocate memory: %s", strerror(errno));
    goto FAIL;
  }
  stpcpy(stpcpy(newname, oldname), ".mole");

  LOG_DEBUG("Renaming '%s' to '%s'...", oldname, newname);
  if (rename(oldname, newname) != 0) {
    LOG_ERROR("Failed to rename '%s' to '%s': %s", oldname, newname, strerror(errno));
    goto FAIL;
  }

  ret = 0;

FAIL:
  free(newname);
  return ret;
}

int wack_mole(const char *fname) {
  return 0;
}

int zclose(int fd) {
  /* Consider -1 a no-op */
  if (fd == -1) {
    return 0;
  }

  int found = 0;
  struct zfile *f;
  for (f = open_files; (f != NULL) && (found == 0); f = f->next) {
    if (f->fd == fd) {
      found = 1;
    }
  }

  /* We don't need the file descriptor anymore */
  LOG_DEBUG("Closing file descriptor for file '%s'...", f->fname);
  if (close(f->fd) != 0) {
    LOG_ERROR("Failed to close file");
    return -1;
  }

  /* If this file descriptor was not opened with zopen(), then we don't do the
   * wack-a-mole algorithm. */
  if (found == 0) {
    return 0;
  }

  LOG_DEBUG("Creating mole...");
  if (create_mole(f->fname) != 0) {
    LOG_ERROR("Failed to create mole");
    free(f->fname);
    free(f);
    return -1;
  }

  free(f->fname);
  free(f);

  return 0;
  return -1;
}
