#pragma once
#include "inttypes.h"
#include <pico.h>

#define D_JOY2_DATA_PIN (NES_GPIO_DATA2)
#define D_JOY1_DATA_PIN (NES_GPIO_DATA1)
#define D_JOY_CLK_PIN (NES_GPIO_CLK)
#define D_JOY_LATCH_PIN (NES_GPIO_LAT)

#define QNT_IMP_NES (13)                // количество импульсов чтения джойстика NES

uint32_t d_joy_get_data();
bool decode_joy();
void d_joy_init();