#ifndef __ZEUGL_IMMUTABLE_H__
#define __ZEUGL_IMMUTABLE_H__

#include <stdbool.h>

/**
 * @brief Check if file has immutable attribute.
 * @param path Path to the file to check.
 * @return true if file is immutable, false otherwise.
 */
bool zeugl_is_immutable(const char *path);

/**
 * @brief Remove immutable attribute from file.
 * @param path Path to the file to modify.
 * @return true if immutable attribute was successfully cleared, false
 * otherwise.
 */
bool zeugl_clear_immutable(const char *path);

/**
 * @brief Set immutable attribute on file.
 * @param path Path to the file to modify.
 * @return true if immutable attribute was successfully set, false otherwise.
 */
bool zeugl_set_immutable(const char *path);

#endif /* __ZEUGL_IMMUTABLE_H__ */
