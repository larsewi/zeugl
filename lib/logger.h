#ifndef __ZEUGL_LOGGER_H__
#define __ZEUGL_LOGGER_H__

#if NDEBUG

#define LOG_DEBUG(...)

#else /* NDEBUG */

#define LOG_DEBUG(...) LoggerLogMessage(__FILE__, __LINE__, __VA_ARGS__)

void LoggerEnable();

void LoggerLogMessage(const char *file, int line, const char *format, ...);

#endif /* NDEBUG */

#endif /* __ZEUGL_LOGGER_H__ */
