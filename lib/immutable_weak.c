#include <fcntl.h>
#include <unistd.h>

#include "config.h"
#include "immutable.h"
#include "logger.h"
#include "utils.h"

bool is_immutable(NDEBUG_UNUSED const char *path) {
  LOG_DEBUG("Immutable operations not supported on this platform", path);
  return false;
}

bool clear_immutable(NDEBUG_UNUSED const char *path) {
  LOG_DEBUG("Immutable operations not supported on this platform");
  return true;
}

bool set_immutable(NDEBUG_UNUSED const char *path) {
  LOG_DEBUG("Immutable operations not supported on this platform");
  return true;
}
