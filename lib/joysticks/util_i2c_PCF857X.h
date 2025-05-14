#pragma once
#include <stdio.h>
#include "inttypes.h"


// #define SEGA_UP ()

uint32_t i2c_PCF_8button();
uint32_t i2c_PCF_16button();
uint32_t i2c_PCF_NES_joy();
uint32_t i2c_PCF_SEGA_joy();
bool init_PCF857X(uint8_t ADDR);