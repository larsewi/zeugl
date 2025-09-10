#include "config.h"

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

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

bool whack_a_mole(const char *orig, const char *temp) {
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

  if (rename(survivor, orig) == 0) {
    LOG_DEBUG(
        "Replaced the last survivor (mole '%s') with the original file '%s'",
        survivor, orig);
  } else {
    /* We don't really care if it fails. It just means that another agent
     * adopted the mole and beat us to it. */
    LOG_DEBUG("Failed to replace last survivor (mole '%s') with the original "
              "file '%s'",
              survivor, orig);
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
