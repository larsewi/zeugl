#include "logger.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "zeugl.h"

const char *zversion(void) { return PACKAGE_VERSION; }

int zopen(const char *filename, int flags, mode_t mode) {
  assert(filename != NULL);

  int ret;

  LOG_DEBUG("Opening file '%s' in read-only mode...", filename);
  int fd = ret = open(filename, O_RDONLY | (O_CREAT | flags), mode);
  if (ret < 0) {
    LOG_ERROR("Failed to open file '%s': open(): %s", filename,
              strerror(errno));
    goto FAIL;
  }

  LOG_DEBUG("Retrieving information about file '%s'...", filename);
  struct stat sb;
  ret = fstat(fd, &sb);
  if (ret != 0) {
    LOG_ERROR("Failed to retrieve information about file '%s': fstat(): %s",
              filename, strerror(errno));
    goto FAIL;
  }

  LOG_DEBUG("Requesting shared lock for file '%s'...", filename);
  ret = flock(fd, LOCK_SH);
  if (ret != 0) {
    LOG_ERROR("Failed to get shared lock for file '%s': flock(): %s", filename,
              strerror(errno));
    goto FAIL;
  }

FAIL:
  LOG_DEBUG("Releasing shared lock for file '%s'...", filename);
  ret = flock(fd, LOCK_UN);
  if (ret != 0) {
    LOG_ERROR("Failed to release shared lock for file '%s': flock(): %s",
              filename, strerror(errno));
    goto FAIL;
  }

  close(fd);
  return ret;
}

int zclose(int fd) { return -1; }
