#include "config.h"
#include "logger.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>

#include "zeugl.h"

const char *zversion(void) { return PACKAGE_VERSION; }

int zopen(const char *filename, int flags, mode_t mode) {
  assert(filename != NULL);

  LOG_DEBUG("Opening the original file '%s' in read-only mode...", filename);
  int orig_fd = open(filename, O_RDONLY);
  if (orig_fd < 0) {
    LOG_ERROR("Failed to open original file '%s': %s", filename,
              strerror(errno));
    return orig_fd;
  }

  LOG_DEBUG("Requesting shared lock for original file '%s'...", filename);
  {
    int ret = flock(orig_fd, LOCK_SH | LOCK_UN);
    if (ret != 0) {
      LOG_ERROR("Failed to get shared lock for original file '%s': %s",
                filename, strerror(errno));
      return ret;
    }
  }

  LOG_DEBUG("Fetching information about original file '%s'...", filename);
  struct stat orig_sb;
  {
    int ret = fstat(orig_fd, &orig_sb);
    if (ret == -1) {
      LOG_ERROR("Failed to retrieve information about original file '%s': %s",
                filename, strerror(errno));
      return ret;
    }
  }

  return -1;
}

int zclose(int fd) { return -1; }
