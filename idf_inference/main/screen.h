#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_W 240
#define SCREEN_H 240

#define RGB565_BLACK 0x0000
#define RGB565_WHITE 0xFFFF
#define RGB565_RED   0xF800

void screen_init(void);

void screen_fill_screen(uint16_t color);
void screen_fill_rect(int x, int y, int w, int h, uint16_t color);
void screen_draw_pixel(int x, int y, uint16_t color);
void screen_fill_circle(int cx, int cy, int r, uint16_t color);
void screen_draw_rgb565_block(int x, int y, int w, int h, const uint16_t *pixels_be);

#ifdef __cplusplus
}
#endif
