/*
 *  Enumeration of configuration values (ESP32-S3 port).
 *  Mirrors Port/config_enum_values.h (MG24) and adds the ESP32-S3 boards.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef SRC_ENUM_VALUES_H_
#define SRC_ENUM_VALUES_H_
#define EXT_MEMORY_TYPE_INTERLEAVED_SPI     0
#define EXT_MEMORY_TYPE_SD_CACHED           1
//
#define PWM_AUDIO_MODE                      1
#define DAC_AUDIO_MODE                      2
#define I2S_AUDIO_MODE                      3
//
#define I2C_KEYBOARD                        1
#define PARALLEL_KEYBOARD                   2
#define SPI74165_KEYBOARD                   3
#define SPI2X74165_KEYBOARD                 4
//
#define BOARD_WSTK_1                        1
#define BOARD_WSTK_2                        2
#define BOARD_WSTK_3                        3
#define BOARD_SPARKDOOM                     4
#define BOARD_SPARKDOOM_ARDUINONANOMATTER   5
#define BOARD_THEGAMEPAD_ARDUINONANOMATTER  6
#define BOARD_TDECK                         7       // LilyGO T-Deck (ESP32-S3)
#define BOARD_WIO_TRACKER_L2                8       // Seeed Wio Tracker L2 / L2 Pro (ESP32-S3)
#endif /* SRC_ENUM_VALUES_H_ */
