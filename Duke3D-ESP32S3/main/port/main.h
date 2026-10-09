/*
 *  main.h for the ESP32-S3 port.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef MAIN_H
#define MAIN_H
#include <stdbool.h>
#include <stdint.h>
#include "macros.h"
//
#define FULL_FRAMEBUFFER                  1
#define DISABLE_CACHING_TEXTURE_TO_FLASH  0
#define TEST_DISABLE_ASYNCH_LOAD          0
#define FAST_CPU_SMALL_FLASH              0
#define CORRECT_TABLE_ERROR               1
#define CACHE_ALL_COLORMAP_TO_RAM         0
#define DEMO_SUPPORT                      0
#define HAS_NETWORK                       0       // no 802.15.4 radio on the ESP32-S3
//
// The shared engine/game code uses MG24 to mean "embedded build" (as opposed to
// the WIN32/SDL build). ESP_PLATFORM (defined by ESP-IDF) selects the ESP32-S3
// variants where the MG24 code used ARM-specific instructions or MG24 peripherals.
#define MG24                              1
#define DEBUG_SETUP                       0
#define DATA_FILE_NAME                    "DUKE3D.GRP"
#define SD_MOUNT_POINT                    "/sd"
//
#define DEBUG_OUT_PRINTF                  1
//
// Free running "timer": the engine reads TICK_TIMER->CNT directly.
#define TICK_TIMER_FREQUENCY_HZ           1000000UL
#define TIMER_TICKS_PER_MILLISECOND       (TICK_TIMER_FREQUENCY_HZ / 1000UL)
typedef struct
{
    uint32_t CNT;
} espTickTimer_t;
uint32_t espTimerCount(void);
#define TICK_TIMER                        (&(espTickTimer_t) {.CNT = espTimerCount()})
unsigned int I_GetTimeMicrosecs(void);
//
#define ENABLE_MUSIC                      1
#define MUSIC_NUM_SAMPLES                 1024
//
// "Internal flash" (read-only level cache) lives in PSRAM.
#define FLASH_PAGE_SIZE                   8192
#endif
