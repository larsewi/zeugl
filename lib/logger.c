#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "logger.h"

static int log_level = LOG_LEVEL_NONE;

void LoggerLogLevelSet(int level) { log_level = level; }

void LoggerLogMessage(int level, const char *file, int line, const char *format,
                      ...) {
  assert(file != NULL);
  assert(format != NULL);

  if (level >= log_level) {
    return;
  }

  char msg[1024];

  va_list ap;
  va_start(ap, format);

  int ret = vsnprintf(msg, sizeof(msg), format, ap);
  if (ret < 0 || (size_t)ret >= sizeof(msg)) {
    LOG_WARNING("Log message truncated: Too long (%d >= %zu)", ret,
                sizeof(msg));
  }

  va_end(ap);

  switch (level) {
  case LOG_LEVEL_DEBUG:
    fprintf(stdout, "[%s:%d] Debug: %s\n", file, line, msg);
    break;
  case LOG_LEVEL_WARNING:
    fprintf(stdout, "[%s:%d] Warning: %s\n", file, line, msg);
    break;
  case LOG_LEVEL_ERROR:
    fprintf(stderr, "[%s:%d] Error: %s\n", file, line, msg);
    break;
  case LOG_LEVEL_NONE:
    break;
  }
}
