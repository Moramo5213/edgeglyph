#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void touch_init(void);
bool touch_read(int *x, int *y);

#ifdef __cplusplus
}
#endif
