#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "logger.h"
#include "utils.h"

static int enabled = 0;

void zeugl_logger_enable(void) { enabled = 1; }

void zeugl_logger_log_message(const char *file, int line, const char *format,
                              ...) {
  assert(file != NULL);
  assert(format != NULL);

  if (!enabled) {
    return;
  }

  char msg[1024];

  va_list ap;
  va_start(ap, format);

  ZEUGL_NDEBUG_UNUSED int ret = vsnprintf(msg, sizeof(msg), format, ap);
  assert(ret >= 0 && (size_t)ret < sizeof(msg));

  va_end(ap);

  ret = printf("[%s:%d] Debug: %s\n", file, line, msg);
  assert(ret >= 0);
}
