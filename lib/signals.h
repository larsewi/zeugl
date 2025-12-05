#ifndef __ZEUGL_SIGNALS_H__
#define __ZEUGL_SIGNALS_H__

/**
 * Install cleanup signal handlers for process termination.
 * This function saves any existing signal handlers and chains to them
 * after performing cleanup operations.
 *
 * @param cleanup_func Function to call for cleanup before chaining
 */
void zeugl_install_signal_handlers(void (*cleanup_func)(void));

#endif /* __ZEUGL_SIGNALS_H__ */
