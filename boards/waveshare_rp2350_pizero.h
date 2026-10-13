/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/waveshare_rp2350_pizero.h"

#ifndef _BOARDS_WAVESHARE_RP2350_PIZERO
#define _BOARDS_WAVESHARE_RP2350_PIZERO


// For board detection
#define RASPBERRYPI_RP2350_PIZERO

// --- RP2350 VARIANT ---
#define PICO_RP2350A 0

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 10
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 11
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 1
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 30
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 31
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 40
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 43
#endif

// --- FLASH ---

#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

// ---------------------------------------------------------------------------
// Game & Watch firmware (Waveshare RP2350-PiZero, z0p2). Pinout as in the other
// z0p2 builds (frank-msx, atari800, pico-nes). Hardware: mini-HDMI, PCM5102A I2S
// module, USB HID on the native USB port, PS/2 and NES pad on the header.
// ---------------------------------------------------------------------------
#define BOARD_Z0 1
// HDMI only: no VGA/TFT/LCD, no I2C keyboard, no battery monitor
#define BOARD_HDMI_ONLY 1

// HDMI: data lanes GPIO32..37, clock pair GPIO38/39 (see lib/video/video.h, BOARD_Z0)
#define VGA_BASE_PIN 32
#define HDMI_BASE_PIN 32

// Sound: PCM5102A on I2S. DIN GPIO10, BCK GPIO11, LRCK GPIO12 (clock base + 1).
// No PWM jack on the board; the PWM defines are for the build only.
#define AUDIO_DATA_PIN 10
#define AUDIO_CLOCK_PIN 11
#define AUDIO_PWM_PIN 10

// microSD on SPI1: SCK GPIO30, MOSI GPIO31, MISO GPIO40, CS GPIO43
#define SDCARD_SPI_BUS spi1
#define SDCARD_PIN_SPI0_SCK 30
#define SDCARD_PIN_SPI0_MOSI 31
#define SDCARD_PIN_SPI0_MISO 40
#define SDCARD_PIN_SPI0_CS 43

// PS/2 keyboard: CLK GPIO2, DATA GPIO3
#define PS2KBD_GPIO_FIRST 2

// NES gamepad: CLK GPIO4, LATCH GPIO5, DATA GPIO7. The second gamepad is not
// wired; its data line is read from the same pin.
#define NES_GPIO_CLK 4
#define NES_GPIO_LAT 5
#define NES_GPIO_DATA1 7
#define NES_GPIO_DATA2 7
// I2C joysticks share the NES CLK/LATCH pins: GPIO4 = I2C0 SDA, GPIO5 = I2C0 SCL
#define JOY_I2C_PORT i2c0

// TFT (not present on the board, defines are needed for the build only)
#define TFT_CS_PIN 12
#define TFT_RST_PIN 14
#define TFT_LED_PIN 15
#define TFT_DC_PIN 16
#define TFT_DATA_PIN 18
#define TFT_CLK_PIN 19

#define SMS_SINGLE_FILE 1

#endif
