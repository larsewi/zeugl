#include "immutable.h"
#include "config.h"
#include "logger.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#ifdef __linux__
#include <linux/fs.h>
#include <sys/ioctl.h>
#define HAVE_LINUX_IMMUTABLE_SUPPORT
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) ||   \
    defined(__APPLE__) && HAVE_CHFLAGS
#include <sys/stat.h>
#include <unistd.h>
#ifdef UF_IMMUTABLE
#define HAVE_BSD_IMMUTABLE_SUPPORT
#endif /* UF_IMMUTABLE */
#endif

bool is_immutable(const char *path) {
#if defined(HAVE_LINUX_IMMUTABLE_SUPPORT)
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

#elif defined(HAVE_BSD_IMMUTABLE_SUPPORT)
  struct stat st;
  if (stat(path, &st) == 0) {
    LOG_DEBUG("Retrieved file attributes for '%s': %s",
              path, strerror(errno));
  } else {
    LOG_DEBUG("Failed to retrieve file attributes for '%s': %s", path,
              strerror(errno));
    return false;
  }

  bool immutable = (st.st_flags & (UF_IMMUTABLE | SF_IMMUTABLE)) != 0;
  LOG_DEBUG("File '%s' is %s", path,
            immutable ? "immutable" : "mutable");
  return immutable;
#else /* No immutable support */
  LOG_DEBUG("Immutable operations not supported on this platform or filesystem", path);
  return false;
#endif
}

bool clear_immutable(const char *path) {
#if defined(HAVE_LINUX_IMMUTABLE_SUPPORT)
  int fd = open(path, O_RDONLY);
  if (fd >= 0) {
    LOG_DEBUG("Opened file '%s' (fd = %d)", path, fd);
  }
  else {
    LOG_DEBUG("Failed to open file '%s': %s", path, strerror(errno));
    return false;
  }

  int flags;
  if (ioctl(fd, FS_IOC_GETFLAGS, &flags) == 0) {
    LOG_DEBUG("Retrieved file attributes for '%s' (fd = %d)", path, fd);
  } else {
    LOG_DEBUG("Failed to get file attributes for '%s' (fd = %d): %s", path, strerror(errno));
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
    LOG_DEBUG("Failed to clear immutable bit for '%s' (fd = %d): %s", path, fd,
              strerror(errno));
    close(fd);
    return false;
  }

  LOG_DEBUG("Cleared immutable bit for '%s' (fd = %d)", path, fd);
  close(fd);
  return true;

#elif defined(BSD_IMMUTABLE_SUPPORT)
  struct stat st;
  if (stat(path, &st) == 0) {
      LOG_DEBUG("Retrieved file attributes for '%s'", path);
  } else {
    LOG_DEBUG("Failed to retrieve file attributes for '%s': %s", path, strerror(errno));
    return false;
  }

  u_int32_t flags = st.st_flags;
  flags &= ~(UF_IMMUTABLE | SF_IMMUTABLE);

  if (chflags(path, flags) == 0) {
    LOG_DEBUG("Failed to clear immutable flag for '%s': %s", path,
              strerror(errno));
    return false;
  }

  LOG_DEBUG("Cleared immutable flag from '%s'", path);
  return true;
#else
  LOG_DEBUG("Immutable operations not supported on this platform");
  return true;
#endif
}

bool set_immutable(const char *path) {
#if defined(HAVE_LINUX_IMMUTABLE_SUPPORT)
  int fd = open(path, O_RDONLY);
  if (fd == 0) {
    LOG_DEBUG(
        "Opened file '%s' (fd = %d)", path, fd);
  }
  if (fd < 0) {
    LOG_DEBUG("Failed to open file '%s': %s", path, strerror(errno));
    return false;
  }

  int flags;
  if (ioctl(fd, FS_IOC_GETFLAGS, &flags) == 0) {
    LOG_DEBUG("Retrieved file attributes for '%s' (fd = %d)", path, fd);
  } else {
    LOG_DEBUG("Failed retrieve file attributes for '%s' (fd = %d): %s", path, fd, strerror(errno));
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
  LOG_DEBUG("Set immutable bit on '%s'", path);
  return true;

#elif HAVE_BSD_IMMUTABLE_SUPPORT
  struct stat st;
  if (stat(path, &st) == 0) {
    LOG_DEBUG("Retrieved file attributes for '%s': %s", path);
  } else {
    LOG_DEBUG("Failed to retrieve file attributes for '%s': %s", path,
              strerror(errno));
    return false;
  }

  u_int32_t flags = st.st_flags;
  flags |= UF_IMMUTABLE;

  if (chflags(path, flags) < 0) {
    LOG_DEBUG("Failed to set immutable flag for '%s': %s", path,
              strerror(errno));
    return false;
  }

  LOG_DEBUG("Set immutable flag on '%s'", path);
  return true;
#else
  LOG_DEBUG("Immutable operations not supported on this platform or filesystem");
  return true;
#endif
}
