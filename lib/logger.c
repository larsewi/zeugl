#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "logger.h"

static int enabled = 0;

void LoggerEnable(void) { enabled = 1; }

void LoggerLogMessage(const char *file, int line, const char *format, ...) {
  assert(file != NULL);
  assert(format != NULL);

  if (!enabled) {
    return;
  }

  char msg[1024];

  va_list ap;
  va_start(ap, format);

#ifdef NDEBUG
  __attribute__((unused))
#endif
  int ret = vsnprintf(msg, sizeof(msg), format, ap);
  assert(ret >= 0 && (size_t)ret < sizeof(msg));

  va_end(ap);

  fprintf(stdout, "[%s:%d] Debug: %s\n", file, line, msg);
}
