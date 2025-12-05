#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "config.h"
#include "immutable.h"
#include "logger.h"

bool zeugl_is_immutable(const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd >= 0) {
    LOG_DEBUG("Opened file '%s' (fd = %d)", path, fd);
  } else {
    LOG_DEBUG("Failed to open file '%s': %s", path, strerror(errno));
    return false;
  }

  int flags;
  if (ioctl(fd, FS_IOC_GETFLAGS, &flags) == 0) {
    LOG_DEBUG("Retrieved file attributes for '%s' (fd = %d)", path, fd);
  } else {
    LOG_DEBUG("Failed to get file attributes for '%s': %s", path,
              strerror(errno));
    close(fd);
    return false;
  }

  if (close(fd) == 0) {
    LOG_DEBUG("Closed file '%s' (fd = %d)", path, fd);
  } else {
    LOG_DEBUG("Failed to closed file '%s' (fd = %d)", path, fd);
  }

  bool immutable = (flags & FS_IMMUTABLE_FL) != 0;
  LOG_DEBUG("File '%s' is %s", path, immutable ? "immutable" : "mutable");
  return immutable;
}

bool zeugl_clear_immutable(const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd >= 0) {
    LOG_DEBUG("Opened file '%s' (fd = %d)", path, fd);
  } else {
    LOG_DEBUG("Failed to open file '%s': %s", path, strerror(errno));
    return false;
  }

  int flags;
  if (ioctl(fd, FS_IOC_GETFLAGS, &flags) == 0) {
    LOG_DEBUG("Retrieved file attributes for '%s' (fd = %d)", path, fd);
  } else {
    LOG_DEBUG("Failed to get file attributes for '%s' (fd = %d): %s", path,
              strerror(errno));
    close(fd);
    return false;
  }

  if (!(flags & FS_IMMUTABLE_FL)) {
    LOG_DEBUG("File '%s' is not immutable, nothing to clear", path);
    close(fd);
    return true;
  }

  flags &= ~FS_IMMUTABLE_FL;
  if (ioctl(fd, FS_IOC_SETFLAGS, &flags) < 0) {
    LOG_DEBUG("Failed to clear immutable flag for '%s' (fd = %d): %s", path, fd,
              strerror(errno));
    close(fd);
    return false;
  }

  LOG_DEBUG("Cleared immutable flag for '%s' (fd = %d)", path, fd);
  close(fd);
  return true;
}

bool zeugl_set_immutable(const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd == 0) {
    LOG_DEBUG("Opened file '%s' (fd = %d)", path, fd);
  }
  if (fd < 0) {
    LOG_DEBUG("Failed to open file '%s': %s", path, strerror(errno));
    return false;
  }

  int flags;
  if (ioctl(fd, FS_IOC_GETFLAGS, &flags) == 0) {
    LOG_DEBUG("Retrieved file attributes for '%s' (fd = %d)", path, fd);
  } else {
    LOG_DEBUG("Failed retrieve file attributes for '%s' (fd = %d): %s", path,
              fd, strerror(errno));
    close(fd);
    return false;
  }

  flags |= FS_IMMUTABLE_FL;
  if (ioctl(fd, FS_IOC_SETFLAGS, &flags) < 0) {
    LOG_DEBUG("Failed to set immutable flag for '%s' (fd = %d): %s", path, fd,
              strerror(errno));
    close(fd);
    return false;
  }

  close(fd);
  LOG_DEBUG("Set immutable flag on '%s'", path);
  return true;
}
