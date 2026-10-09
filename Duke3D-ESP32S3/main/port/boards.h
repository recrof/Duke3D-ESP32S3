/*
 *  Board definitions for the ESP32-S3 port.
 *
 *  The board is selected at build time (see platformio.ini / CMakeLists.txt):
 *    BOARD_TDECK            LilyGO T-Deck / T-Deck Plus
 *    BOARD_WIO_TRACKER_L2   Seeed Studio Wio Tracker L2 / L2 Pro
 *  Every board has an ESP32-S3 with 16 MB flash and 8 MB octal PSRAM, a 320x240
 *  panel and a microSD slot. Board specific bring-up and input devices are in
 *  board_<name>.c.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef SRC_BOARDS_H_
#define SRC_BOARDS_H_
#include "macros.h"
#include "config_enum_values.h"
#ifndef BOARD
#define BOARD                                 BOARD_TDECK
#endif
#define EXT_MEMORY_TYPE                       EXT_MEMORY_TYPE_SD_CACHED
#define AUDIO_MODE                            I2S_AUDIO_MODE
#define HAS_SD                                1
#define STEREO_AUDIO                          1   // the mixer is stereo; downmixed for the mono speakers
//
#define CENTER_SCREEN                         1
#define SCREEN_WIDTH                          320
#define SCREEN_HEIGHT                         240
#define DRAW_BUFFER_HEIGHT                    240
//
#define DISPLAY_CONTROLLER_ST7789             0   // 4-wire SPI (D/C pin)
#define DISPLAY_CONTROLLER_NV3031B            1   // QSPI, no D/C pin
#define SD_MODE_SPI                           0
#define SD_MODE_SDMMC_1BIT                    1

#if BOARD == BOARD_TDECK
// -----------------------------------------------------------------------------
// LilyGO T-Deck. Pins from https://github.com/Xinyuan-LilyGO/T-Deck (utilities.h).
// ST7789 display, SD card and LoRa radio share SPI2; MAX98357A I2S amplifier;
// ESP32-C3 keyboard controller on I2C and a trackball. See board_tdeck.c.
// -----------------------------------------------------------------------------
#define BOARD_NAME                            "LilyGO T-Deck"
#define DISPLAY_CONTROLLER                    DISPLAY_CONTROLLER_ST7789
#define DISPLAY_SPI_HOST                      SPI2_HOST
#define DISPLAY_SPI_CLOCK_HZ                  (40 * 1000 * 1000)
#define DISPLAY_MADCTL                        0x60    // landscape (MX | MV), keyboard at the bottom
#define DISPLAY_INVERT_COLORS                 1       // IPS panel
#define PIN_SPI_MOSI                          41
#define PIN_SPI_MISO                          38
#define PIN_SPI_SCK                           40
#define PIN_TFT_CS                            12
#define PIN_TFT_DC                            11
#define PIN_TFT_BACKLIGHT                     42
//
#define SD_MODE                               SD_MODE_SPI   // on the display bus
#define PIN_SD_CS                             39
//
#define PIN_POWER_ON                          10   // peripheral power enable, must be high
#define PIN_LORA_CS                           9    // held high so the radio stays off the bus
//
#define PIN_I2C_SDA                           18
#define PIN_I2C_SCL                           8
#define KEYBOARD_I2C_ADDRESS                  0x55
#define PIN_KEYBOARD_INT                      46
#define PIN_TOUCH_INT                         16
//
#define PIN_TRACKBALL_UP                      3
#define PIN_TRACKBALL_DOWN                    15
#define PIN_TRACKBALL_LEFT                    1
#define PIN_TRACKBALL_RIGHT                   2
#define PIN_TRACKBALL_CLICK                   0    // also the BOOT strap
//
#define PIN_I2S_MCLK                          -1
#define PIN_I2S_BCK                           7
#define PIN_I2S_WS                            5
#define PIN_I2S_DOUT                          6
#define AUDIO_OUTPUT_SHIFT                    7    // u8 -> s16; no volume control on the amplifier
//
#define PIN_BAT_ADC                           4

#elif BOARD == BOARD_WIO_TRACKER_L2
// -----------------------------------------------------------------------------
// Seeed Studio Wio Tracker L2 / L2 Pro: 3.2" NV3031B QSPI panel with a GT911
// touch controller, microSD on 1-bit SDMMC, ES8311 codec. Power rails, resets
// and the WAKE button are on a PCA9555 GPIO expander and the backlight on an
// LP5814 driver, all on I2C. See board_l2.c.
// -----------------------------------------------------------------------------
#define BOARD_NAME                            "Seeed Wio Tracker L2"
#define DISPLAY_CONTROLLER                    DISPLAY_CONTROLLER_NV3031B
#define DISPLAY_SPI_HOST                      SPI3_HOST
#define DISPLAY_SPI_CLOCK_HZ                  (40 * 1000 * 1000)  // x4 lines
#define DISPLAY_MADCTL                        0xA0    // landscape (MY | MV)
#define DISPLAY_INVERT_COLORS                 0
#define PIN_SPI_SCK                           42
#define PIN_SPI_MOSI                          41      // QSPI IO0
#define PIN_QSPI_IO1                          40
#define PIN_QSPI_IO2                          39
#define PIN_QSPI_IO3                          38
#define PIN_TFT_CS                            46
//
#define SD_MODE                               SD_MODE_SDMMC_1BIT
#define PIN_SD_CMD                            3
#define PIN_SD_D0                             1
#define PIN_SD_CLK                            2
//
#define PIN_I2C_SDA                           47
#define PIN_I2C_SCL                           48
//
#define PIN_I2S_MCLK                          10
#define PIN_I2S_BCK                           11
#define PIN_I2S_WS                            12
#define PIN_I2S_DOUT                          16
#define AUDIO_OUTPUT_SHIFT                    8    // the ES8311 volume is set in board_l2.c
#define AUDIO_HAS_CODEC                       1
//
#define PIN_BOOT_BUTTON                       0    // also the BOOT strap
// GT911 native orientation is portrait: mapping to the landscape panel.
#define TOUCH_NATIVE_WIDTH                    240  // if the controller does not report it
#define TOUCH_NATIVE_HEIGHT                   320
#define TOUCH_SWAP_XY                         1
#define TOUCH_INVERT_X                        1
#define TOUCH_INVERT_Y                        0

#else
#error Unsupported BOARD: use BOARD_TDECK or BOARD_WIO_TRACKER_L2
#endif

#ifndef AUDIO_HAS_CODEC
#define AUDIO_HAS_CODEC                       0
#endif
#endif /* SRC_BOARDS_H_ */
