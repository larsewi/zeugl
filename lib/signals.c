#include "config.h"

#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "signals.h"

#define NUM_HANDLERS (sizeof(prev_handlers) / sizeof(prev_handlers[0]))

struct handler {
  int sig;
  struct sigaction sa;
};

/* A place to store previous signal handlers to chain them */
static struct handler prev_handlers[] = {
    {SIGINT, {}},
    {SIGTERM, {}},
    {SIGHUP, {}},
    {SIGQUIT, {}},
};

/* Function to call for cleanup */
static void (*cleanup_function)(void) = NULL;

/* Track whether handlers are installed */
static volatile sig_atomic_t handlers_installed = 0;

/**
 * Signal handler for abnormal termination.
 * Chains to previous handler after cleanup.
 */
static void signal_handler(int sig) {
  if (cleanup_function != NULL) {
    LOG_DEBUG("Calling cleanup functions...");
    cleanup_function();
  }

  /* Chain to previous handler (if previously stored) */
  struct sigaction *prev_handler = NULL;
  for (size_t i = 0; i < NUM_HANDLERS; i++) {
    if (sig == prev_handlers[i].sig) {
      LOG_DEBUG("Found previous signal handler for %s", strsignal(sig));
      prev_handler = &prev_handlers[i].sa;
      break;
    }
  }

  if (prev_handler != NULL) {
    LOG_DEBUG("Restoring old signal handler and re-raising signal...");
    sigaction(sig, prev_handler, NULL);
    raise(sig);
  } else {
    LOG_DEBUG("Resetting to default signal handler and re-raising signal...");
    signal(sig, SIG_DFL);
    raise(sig);
  }
}

void zeugl_install_signal_handlers(void (*cleanup_func)(void)) {
  /* We only install the handlers once */
  if (handlers_installed != 0) {
    return;
  }

  /* Save cleanup function */
  cleanup_function = cleanup_func;

  /* Register cleanup for normal exit */
  if (cleanup_func != NULL) {
    if (atexit(cleanup_func) != 0) {
      LOG_DEBUG("Failed to register atexit() handler");
    } else {
      LOG_DEBUG("Registered atexit() handler");
    }
  }

  /* Create new signal handlers for abnormal termination */
  struct sigaction sa = {0};
  sa.sa_handler = signal_handler;
  if (sigemptyset(&sa.sa_mask) == -1) {
    LOG_DEBUG("Failed to initialize signal set to empty: %s", strerror(errno));
  }

  /* Handle common termination signals - save previous handlers */
  for (size_t i = 0; i < NUM_HANDLERS; i++) {
    int sig = prev_handlers[i].sig;
    if (sigaction(sig, &sa, &prev_handlers[i].sa) != 0) {
      LOG_DEBUG("Failed to install %s handler: %s", strsignal(sig),
                strerror(errno));
    } else {
      LOG_DEBUG("Installed signal handler for %s", strsignal(sig));
    }
  }

  handlers_installed = 1;
}
