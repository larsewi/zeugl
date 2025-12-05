#ifndef __ZEUGL_WACKAMOLE_H__
#define __ZEUGL_WACKAMOLE_H__

#include <stdbool.h>

bool zeugl_whack_a_mole(const char *orig, const char *temp,
                        bool handle_immutable, bool no_block);

#endif /* __ZEUGL_WACKAMOLE_H__ */
