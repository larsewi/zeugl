#include <fcntl.h>
#include <unistd.h>

#include "config.h"
#include "immutable.h"
#include "logger.h"
#include "utils.h"

bool zeugl_is_immutable(ZEUGL_NDEBUG_UNUSED const char *path) {
  LOG_DEBUG("Immutable operations not supported on this platform", path);
  return false;
}

bool zeugl_clear_immutable(ZEUGL_NDEBUG_UNUSED const char *path) {
  LOG_DEBUG("Immutable operations not supported on this platform");
  return true;
}

bool zeugl_set_immutable(ZEUGL_NDEBUG_UNUSED const char *path) {
  LOG_DEBUG("Immutable operations not supported on this platform");
  return true;
}
