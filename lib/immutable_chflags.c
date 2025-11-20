#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "immutable.h"
#include "logger.h"

bool is_immutable(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    LOG_DEBUG("Retrieved file attributes for '%s'", path);
  } else {
    LOG_DEBUG("Failed to retrieve file attributes for '%s': %s", path,
              strerror(errno));
    return false;
  }

  bool immutable = (st.st_flags & (UF_IMMUTABLE | SF_IMMUTABLE)) != 0;
  LOG_DEBUG("File '%s' is %s", path, immutable ? "immutable" : "mutable");
  return immutable;
}

bool clear_immutable(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    LOG_DEBUG("Retrieved file attributes for '%s'", path);
  } else {
    LOG_DEBUG("Failed to retrieve file attributes for '%s': %s", path,
              strerror(errno));
    return false;
  }

  u_int32_t flags = st.st_flags;
  flags &= (u_int32_t) ~(UF_IMMUTABLE | SF_IMMUTABLE);

  if (chflags(path, flags) < 0) {
    LOG_DEBUG("Failed to clear immutable flag for '%s': %s", path,
              strerror(errno));
    return false;
  }

  LOG_DEBUG("Cleared immutable flag from '%s'", path);
  return true;
}

bool set_immutable(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    LOG_DEBUG("Retrieved file attributes for '%s'", path);
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
}
