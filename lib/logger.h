#ifndef __ZEUGL_LOGGER_H__
#define __ZEUGL_LOGGER_H__

enum {
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_NONE,
};

#define LOG_DEBUG(...)                                                         \
  LoggerLogMessage(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...)                                                       \
  LoggerLogMessage(LOG_LEVEL_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)                                                         \
  LoggerLogMessage(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

void LoggerLogLevelSet(int level);

void LoggerLogMessage(int level, const char *file, int line, const char *format,
                      ...);

#endif /* __ZEUGL_LOGGER_H__ */
