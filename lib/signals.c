#include "config.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "signals.h"

/* Store previous signal handlers to chain them */
static struct sigaction old_sigint_handler;
static struct sigaction old_sigterm_handler;
static struct sigaction old_sighup_handler;
static struct sigaction old_sigquit_handler;

/* Function to call for cleanup */
static void (*cleanup_function)(void) = NULL;

/* Track whether handlers are installed */
static volatile sig_atomic_t handlers_installed = 0;

/**
 * Signal handler for abnormal termination.
 * Chains to previous handler after cleanup.
 */
static void signal_handler(int sig) {
  /* Call cleanup function if provided */
  if (cleanup_function != NULL) {
    cleanup_function();
  }

  /* Chain to previous handler */
  struct sigaction *old_handler = NULL;
  switch (sig) {
  case SIGINT:
    old_handler = &old_sigint_handler;
    break;
  case SIGTERM:
    old_handler = &old_sigterm_handler;
    break;
  case SIGHUP:
    old_handler = &old_sighup_handler;
    break;
  case SIGQUIT:
    old_handler = &old_sigquit_handler;
    break;
  }

  if (old_handler != NULL) {
    /* Restore the old handler */
    sigaction(sig, old_handler, NULL);

    /* If there was a previous handler, call it */
    if (old_handler->sa_handler != SIG_DFL &&
        old_handler->sa_handler != SIG_IGN) {
      if (old_handler->sa_flags & SA_SIGINFO) {
        /* SA_SIGINFO handler - we can't properly chain this from here */
        /* Just re-raise the signal */
        raise(sig);
      } else if (old_handler->sa_handler != NULL) {
        /* Normal handler - call it directly */
        old_handler->sa_handler(sig);
      }
    } else {
      /* Default or ignore - re-raise to trigger default behavior */
      raise(sig);
    }
  } else {
    /* Fallback: reset to default and re-raise */
    signal(sig, SIG_DFL);
    raise(sig);
  }
}

void install_signal_handlers(void (*cleanup_func)(void)) {
  if (handlers_installed == 0) {
    /* Save cleanup function */
    cleanup_function = cleanup_func;

    /* Register cleanup for normal exit */
    if (cleanup_func != NULL) {
      if (atexit(cleanup_func) != 0) {
        LOG_DEBUG("Failed to register atexit handler");
      }
    }

    /* Install signal handlers for abnormal termination */
    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);

    /* Handle common termination signals - save previous handlers */
    if (sigaction(SIGINT, &sa, &old_sigint_handler) != 0) {
      LOG_DEBUG("Failed to install SIGINT handler");
    }
    if (sigaction(SIGTERM, &sa, &old_sigterm_handler) != 0) {
      LOG_DEBUG("Failed to install SIGTERM handler");
    }
    if (sigaction(SIGHUP, &sa, &old_sighup_handler) != 0) {
      LOG_DEBUG("Failed to install SIGHUP handler");
    }
    if (sigaction(SIGQUIT, &sa, &old_sigquit_handler) != 0) {
      LOG_DEBUG("Failed to install SIGQUIT handler");
    }

    handlers_installed = 1;
    LOG_DEBUG("Installed cleanup handlers for process termination");
  }
}