#pragma once
#include "inttypes.h"

#define D_JOY2_DATA_PIN (17)
#define D_JOY1_DATA_PIN (16)
#define D_JOY_CLK_PIN (14)
#define D_JOY_LATCH_PIN (15)

#define QNT_IMP_NES (13)                // количество импульсов чтения джойстика NES

uint32_t d_joy_get_data();
bool decode_joy();
void d_joy_init();