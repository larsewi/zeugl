#include "config.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#include "immutable.h"
#include "logger.h"
#include "whackamole.h"

static char *create_a_mole(const char *temp) {
  char *mole = malloc(strlen(temp) + strlen(".mole") + 1);
  if (mole == NULL) {
    LOG_DEBUG("Failed to allocate memory: %s", strerror(errno));
    return NULL;
  }

  /* Create mole filename */
  stpcpy(stpcpy(mole, temp), ".mole");

  if (rename(temp, mole) != 0) {
    LOG_DEBUG("Failed to rename '%s' to '%s': %s", temp, mole, strerror(errno));
    free(mole);
    return NULL;
  }
  LOG_DEBUG("Renamed '%s' to '%s'", temp, mole);

  return mole;
}

static bool is_a_mole(const char *orig, const char *mole) {
  const size_t orig_len = strlen(orig);                  /* Original filename */
  const size_t mole_len = strlen(mole);                  /* Potential mole */
  const size_t uid_len = strlen(".XXXXXX");              /* Unique identifier */
  const size_t suf_len = strlen(".mole");                /* Mole suffix */
  const size_t exp_len = (orig_len + uid_len + suf_len); /* Expected length */

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

static bool replace_original(const char *orig, const char *survivor) {
  if (rename(survivor, orig) == 0) {
    LOG_DEBUG(
        "Replaced the last survivor (mole '%s') with the original file '%s'",
        survivor, orig);
    return true;
  }

  LOG_DEBUG("Failed to replace last survivor (mole '%s') with the original "
            "file '%s': %s",
            survivor, orig, strerror(errno));

  /* We don't really care if it fails due to missing file. It just means that
   * another agent adopted the mole and beat us to it. */
  return (errno == ENOENT);
}

static bool replace_immutable_original(const char *orig, const char *survivor,
                                       bool handle_immutable) {
  bool was_immutable = handle_immutable ? is_immutable(orig) : false;
  if (!was_immutable) {
    return replace_original(orig, survivor);
  }

  if (clear_immutable(orig)) {
    LOG_DEBUG("Temporarily cleared immutable attribute from '%s'", orig);
  } else {
    LOG_DEBUG("Failed to temporarily clear immutable attribute from '%s'",
              orig);
    return false;
  }

  if (!replace_original(orig, survivor)) {
    /* Error is already logged */
    return false;
  }

  /* Restore immutable bit before releasing lock */
  if (!set_immutable(orig)) {
    LOG_DEBUG("Failed to restore the immutable bit on '%s'", orig);
    return false;
  }
  LOG_DEBUG("Restored immutable bit on '%s'", orig);

  return true;
}

static bool atomic_replace_immutable_original(const char *orig,
                                              const char *survivor,
                                              bool handle_immutable,
                                              bool no_block) {
  bool success = false;

  /* Open original file for locking before clearing immutable flag */
  int lock_fd = open(orig, O_RDONLY);
  if (lock_fd < 0) {
    if (errno == ENOENT) {
      /* Original file doesn't exist yet - this is fine for new files */
      LOG_DEBUG("Original file '%s' does not exist yet", orig);
      return replace_original(orig, survivor);
    } else {
      LOG_DEBUG("Failed to open original file '%s' for locking: %s", orig,
                strerror(errno));
      return false;
    }
  }
  LOG_DEBUG("Opened original file '%s' (fd = %d) for locking", orig, lock_fd);

  /* Acquire exclusive lock */
  int lock = LOCK_EX;
  if (no_block) {
    lock |= LOCK_NB;
  }
  if (flock(lock_fd, lock) != 0) {
    LOG_DEBUG("Failed to acquire exclusive lock on '%s' (fd = %d): %s", orig,
              lock_fd, strerror(errno));
    close(lock_fd);
    goto FAIL;
  }
  LOG_DEBUG("Acquired exclusive lock on '%s' (fd = %d)", orig, lock_fd);

  if (!replace_immutable_original(orig, survivor, handle_immutable)) {
    /* Error already logged */
    goto FAIL;
  }

  success = true;
FAIL:;
  int save_errno = errno;

  if (flock(lock_fd, LOCK_UN) == 0) {
    LOG_DEBUG("Released exclusive lock on file (fd = %d)", orig, lock_fd);
  } else {
    LOG_DEBUG("Failed to release exclusive lock on original file (fd = %d): %s",
              orig, strerror(errno));
    success = false;
  }

  if (close(lock_fd) == 0) {
    LOG_DEBUG("Closed original file '%s'", orig);
  } else {
    LOG_DEBUG("Failed to close original file '%s' (fd = %d)", orig, lock_fd);
    success = false;
  }

  errno = save_errno;
  return success;
}

bool whack_a_mole(const char *orig, const char *temp, bool handle_immutable,
                  bool no_block) {
  bool success = false;
  DIR *dirp = NULL;
  char *mole = NULL;
  char *buf_1 = NULL;    /* Buffer for dirname() */
  char *buf_2 = NULL;    /* Buffer for basename() */
  char *survivor = NULL; /* Last survivor mole */

  mole = create_a_mole(temp);
  if (mole == NULL) {
    LOG_DEBUG("Failed to create a mole from temporary file '%s'", temp);
    goto FAIL;
  }

  /* Get original dirname */
  buf_1 = strdup(orig);
  if (buf_1 == NULL) {
    LOG_DEBUG("Failed to allocate memory: %s", strerror(errno));
    goto FAIL;
  }
  const char *dname = dirname(buf_1);

  /* Get original basename */
  buf_2 = strdup(orig);
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
    if (is_a_mole(bname, dire->d_name)) {
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
        LOG_DEBUG("Previous survivor '%s' got whacked", survivor);
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
        LOG_DEBUG("New challenger '%s' got whacked", dire->d_name);
      }
    }

    errno = 0;
    dire = readdir(dirp);
  }

  if (errno != 0) {
    LOG_DEBUG("Failed to read directory '%s': %s", dname, strerror(errno));
    goto FAIL;
  }
  LOG_DEBUG("Reached End-of-Directory '%s'", dname);

  if (!atomic_replace_immutable_original(orig, survivor, handle_immutable,
                                         no_block)) {
    /* Error already logged */
    goto FAIL;
  }

  success = true;
FAIL:;

  int save_errno = errno;
  free(mole);
  free(buf_1);
  free(buf_2);
  free(survivor);
  if (dirp != NULL) {
    if (closedir(dirp) == 0) {
      LOG_DEBUG("Successfully closed directory '%s'", dname);
    } else {
      LOG_DEBUG("Failed to close directory '%s': %s", dname, strerror(errno));
    }
  }
  errno = save_errno;

  return success;
}
