#include "config.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "logger.h"
#include "utils.h"
#include "zeugl.h"

struct zfile {
  char *orig;
  char *temp;
  char *mole;
  int fd;
  struct zfile *next;
};

static pthread_mutex_t OPEN_FILES_MUTEX = PTHREAD_MUTEX_INITIALIZER;
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

  stpcpy(stpcpy(file->temp, file->orig), ".XXXXXX");
  LOG_DEBUG("Generated template for temporary filename '%s'", file->temp);

  file->fd = mkstemp(file->temp);
  if (file->fd < 0) {
    LOG_DEBUG("Failed to create temporary file: %s", strerror(errno));
    goto FAIL;
  }
  LOG_DEBUG("Created temporary file '%s' (fd = %d)", file->temp, file->fd);

  int fd = open(file->orig, O_RDONLY);
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
      close(fd);
      goto FAIL;
    }
    LOG_DEBUG("Successfully copied content from original file '%s' (fd = %d) "
              "to temporary "
              "file '%s' (fd = %d)",
              file->orig, fd, file->temp, file->fd);

    close(fd);
    LOG_DEBUG("Closed original file '%s' (fd = %d)", file->orig, fd);
  }

  int ret = pthread_mutex_lock(&OPEN_FILES_MUTEX);
  if (ret != 0) {
    LOG_DEBUG("Failed to acquire mutex protecting list of open files: %s",
              strerror(ret));
    goto FAIL;
  }
  LOG_DEBUG("Successfully acquired mutex protecting list of open files");

  file->next = OPEN_FILES;
  OPEN_FILES = file;
  LOG_DEBUG("Added file '%s' (fd = %d) to list of open files", file->temp,
            file->fd);

  ret = pthread_mutex_unlock(&OPEN_FILES_MUTEX);
  if (ret != 0) {
    LOG_DEBUG("Failed to release mutex protecting list of open files: %s",
              strerror(ret));
    goto FAIL;
  }
  LOG_DEBUG("Successfully released mutex protecting list of open files");

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

  return -1;
}

static int create_a_mole(struct zfile *file) {
  file->mole = malloc(strlen(file->temp) + strlen(".mole") + 1);
  if (file->mole == NULL) {
    LOG_DEBUG("Failed to allocate memory: %s", strerror(errno));
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

static bool file_is_mole(const char *orig, const char *mole) {
  size_t orig_len = strlen(orig);                  /* Original filename */
  size_t mole_len = strlen(mole);                  /* Potential mole */
  size_t uid_len = strlen(".XXXXXX");              /* Unique identifier */
  size_t suf_len = strlen(".mole");                /* Mole suffix */
  size_t exp_len = (orig_len + uid_len + suf_len); /* Expected length */

  if (mole_len != exp_len) {
    /* Potential mole filename has wrong length */
    return false;
  }

  if (strncmp(mole, orig, orig_len) != 0) {
    /* Potential mole filename doesn't start with the original filename */
    return false;
  }

  if (strcmp(mole + orig_len + uid_len, ".mole") != 0) {
    /* Potential mole filename is missing the mole suffix */
    return false;
  }

  return true;
}

static int wack_a_mole(const char *fname) {
  int ret = -1;
  DIR *dirp = NULL;
  char *buf_1 = NULL;    /* Buffer for dirname() */
  char *buf_2 = NULL;    /* Buffer for basename() */
  char *survivor = NULL; /* Last survivor mole */

  /* Get directory name */
  buf_1 = strdup(fname);
  if (buf_1 == NULL) {
    LOG_DEBUG("Failed to allocate memory: %s", strerror(errno));
    goto FAIL;
  }
  const char *dname = dirname(buf_1);

  /* Get filename */
  buf_2 = strdup(fname);
  if (buf_2 == NULL) {
    LOG_DEBUG("Failed to allocate memory: %s", strerror(errno));
    goto FAIL;
  }
  const char *bname = basename(buf_2);

  dirp = opendir(dname);
  if (dirp == NULL) {
    LOG_DEBUG("Failed to open directory '%s'", dname);
    goto FAIL;
  }
  LOG_DEBUG("Opened directory '%s'", dname);

  errno = 0; /* To distinguish between End-of-Directory and ERROR */
  struct dirent *dire = readdir(dirp);

  while (dire != NULL) {
    if (file_is_mole(bname, dire->d_name)) {
      const char *challenger = dire->d_name;

      LOG_DEBUG("Successfully identified a mole '%s'", challenger);

      if /* Initial survivor */ (survivor == NULL) {
        survivor = strdup(challenger);
        if (survivor == NULL) {
          LOG_DEBUG("Failed to allocate memory: %s", strerror(errno));
          goto FAIL;
        }
        LOG_DEBUG("Initial challenger '%s' was appointed as the new survivor",
                  survivor);
      } else if /* New survivor */ (strcmp(challenger, survivor) > 0) {
        unlink(survivor); /* Don't care if it fails */
        LOG_DEBUG("Previous survivor '%s' got wacked", survivor);
        free(survivor);

        survivor = strdup(challenger);
        if (survivor == NULL) {
          LOG_DEBUG("Failed to allocate memory: %s", strerror(errno));
          goto FAIL;
        }
        LOG_DEBUG("New challenger '%s' was appointed as the new survivor",
                  survivor);
      } else /* Keep old survivor */ {
        unlink(challenger); /* Don't care if it fails */
        LOG_DEBUG("New challenger '%s' got wacked", dire->d_name);
      }
    }

    errno = 0;
    dire = readdir(dirp);
  }

  if (errno != 0) {
    LOG_DEBUG("Failed to read directory '%s': %s", dname, errno);
    goto FAIL;
  }
  LOG_DEBUG("Reached End-of-Directory '%s'", dname);

  if (rename(survivor, fname) == -1) {
    /* We don't really care if it fails. It just means that another agent
     * adopted the mole and beat us to it. */
    LOG_DEBUG("Failed to replace last survivor (mole '%s') with the original "
              "file '%s'",
              survivor, fname);
  } else {
    LOG_DEBUG(
        "Replaced the last survivor (mole '%s') with the original file '%s'",
        survivor, fname);
  }

  ret = 0;
FAIL:

  free(buf_1);
  free(buf_2);
  free(survivor);
  if (dirp != NULL) {
    closedir(dirp);
  }

  return ret;
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

  int err = pthread_mutex_lock(&OPEN_FILES_MUTEX);
  if (err != 0) {
    LOG_DEBUG("Failed to acquire mutex protecting list of open files: %s",
              strerror(err));
    return -1;
  }
  LOG_DEBUG("Successfully acquired mutex protecting list of open files");

  int ret = -1;

  LOG_DEBUG("Looking for file with matching file descriptor %d...", fd);
  struct zfile *prev = NULL, *file = OPEN_FILES;
  while ((file != NULL) && (file->fd != fd)) {
    if (file != NULL) {
      /* We need this to merge left and right of list after removal */
      prev = file;
    }
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
    if (create_a_mole(file) != 0) {
      LOG_DEBUG("Failed to create mole from temporary file '%s'", file->temp);
      goto FAIL;
    }
    LOG_DEBUG("Created a mole '%s' from temporary file '%s' (fd = %d)",
              file->mole, file->temp, file->fd);

    if (wack_a_mole(file->orig) != 0) {
      LOG_DEBUG("Failed to wack the moles");
      goto FAIL;
    }
  } else {
    LOG_DEBUG("Aborting file transaction");
    if (unlink(file->temp) == 0) {
      LOG_DEBUG("Deleted temporary file '%s'", file->temp);
    } else {
      LOG_DEBUG("Failed to delete temporary file '%s': %s", file->temp,
                strerror(errno));
      goto FAIL;
    }
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

  err = pthread_mutex_unlock(&OPEN_FILES_MUTEX);
  if (err != 0) {
    LOG_DEBUG("Failed to release mutex protecting list of open files: %s",
              strerror(err));
    ret = -1;
  }
  LOG_DEBUG("Successfully released mutex protecting list of open files");

  if (file != NULL) {
    free(file->orig);
    free(file->temp);
    free(file->mole);
    free(file);
  }

  return ret;
}
