#include <lcom/lcf.h>

#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

void *my_vg_init(uint16_t mode);

int my_vg_draw_pixel(uint16_t x, uint16_t y, uint32_t color);
int my_vg_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint32_t color);
int my_vg_draw_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);
int my_vg_draw_xpm(uint8_t *xpm, uint16_t x, uint16_t y, uint16_t width, uint16_t height);

void vg_clear();
void vg_flush();

#endif