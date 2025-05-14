#pragma once

#include "inttypes.h"
#include "stdbool.h"

//#define beginVGA_PIN (17)
#define PIO_VGA (pio0)
#define beginVGA_PIN (6)
typedef enum g_mode{
    g_mode_320x240x8bpp,g_mode_320x240x4bpp,g_mode_text_80x30
}g_mode;

void graphics_init();

void graphics_set_buffer(uint8_t *buffer);

void graphics_set_textbuffer(uint8_t *ch_buffer,uint8_t* color_buf);
void graphics_set_mode(g_mode mode);
// void graphics_set_offset(int x, int y);

void graphics_set_palette(uint8_t i, uint32_t color888);
