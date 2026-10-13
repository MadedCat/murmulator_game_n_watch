#pragma once
// Olimex RP2040-PICO-PC with a Raspberry Pi Pico 2 (PCp2).
// Pinout as in the other PCp2 builds (pico-nes, pico-z26, murmapple, atari800).
#include "boards/pico2.h"

#define PICO_PC 1
#define BOARD_HDMI_ONLY 1

// Sound: analog stereo jack, right GPIO27, left GPIO28 (AUDIO_PWM_PIN + 1).
// No separate beeper and no I2S DAC on the board: the sound output is forced to PWM.
#define AUDIO_PWM_PIN 27
// I2S pins are defined for the build only (I2S output is not used on PCp2)
#define AUDIO_DATA_PIN 27
#define AUDIO_CLOCK_PIN 28


// on-board microSD
#define SDCARD_PIN_SPI0_CS 22
#define SDCARD_PIN_SPI0_SCK 6
#define SDCARD_PIN_SPI0_MOSI 7
#define SDCARD_PIN_SPI0_MISO 4

// PS/2 keyboard: CLK GPIO0, DATA GPIO1
#define PS2KBD_GPIO_FIRST 0

// NES gamepad on UEXT: CLK GPIO8, LATCH GPIO9, DATA GPIO20.
// The second gamepad is not wired; its data line is read from the same pin.
#define NES_GPIO_CLK 8
#define NES_GPIO_LAT 9
#define NES_GPIO_DATA1 20
#define NES_GPIO_DATA2 20
// I2C joysticks share the NES CLK/LATCH pins: GPIO8 = I2C0 SDA, GPIO9 = I2C0 SCL
#define JOY_I2C_PORT i2c0

// HDMI: CLK 12/13, D0 14/15, D1 16/17, D2 18/19 (R/G lanes swapped in video.c).
// Only HDMI is wired on the board, the video output is forced to HDMI.
#define VGA_BASE_PIN 12
#define HDMI_BASE_PIN 12

// TFT (not present on the board, defines are needed for the build only)
#define TFT_CS_PIN 12
#define TFT_RST_PIN 14
#define TFT_LED_PIN 15
#define TFT_DC_PIN 16
#define TFT_DATA_PIN 18
#define TFT_CLK_PIN 19

#define SMS_SINGLE_FILE 1
