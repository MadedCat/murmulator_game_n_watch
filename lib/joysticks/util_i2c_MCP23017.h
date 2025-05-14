#pragma once
#include <stdio.h>
#include "inttypes.h"

#define MCP_REG_IODIRA (0x00)
#define MCP_REG_IOCON (0x0A)
#define MCP_REG_GPPUA (0x0c)
#define MCP_REG_GPIOA (0x12)

#define MCP_16BUTTON_ADDR (0x23)
#define MCP_NES_JOY_ADDR (0x24)
#define MCP_SEGA_JOY_ADDR (0x25)

uint32_t i2c_MCP_16button();
uint32_t i2c_MCP_NES_joy();
uint32_t i2c_MCP_SEGA_joy();
bool init_MCP23017(uint8_t ADDR);