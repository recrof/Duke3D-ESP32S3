/*
 *  Duke Nukem 3D for ESP32-S3 boards: entry point and boot sequence.
 *
 *  Core 1 runs the game; core 0 runs the display, audio and input tasks.
 *
 *  Erasing settings and savegames (hold while the boot text is shown):
 *    T-Deck                Alt + Space
 *    Wio Tracker L2        WAKE + touch the screen
 *
 *  A Bluetooth LE gamepad (e.g. Xbox Wireless Controller) in pairing mode is
 *  picked up automatically, see gamepad.c.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_chip_info.h"
#include "esp_log.h"
#include "esp_system.h"
#include "main.h"
#include "graphics.h"
#include "port/keyboard.h"
#include "extMemory.h"
#include "delay.h"
#include "gamepad.h"
#include "board.h"

#define GAME_TASK_STACK_SIZE        (48 * 1024)   // the MG24 build used 41.5 kB
#define KEY_COMBINATION_FOR_ERASE_OPTIONS     (KEY_ALT | KEY_USE)

int dn3dmain(int argc, char **argv);

uint32_t espTimerCount(void)
{
    return (uint32_t) esp_timer_get_time();
}

unsigned int I_GetTimeMicrosecs(void)
{
    return (unsigned int) esp_timer_get_time();
}

void delay(uint32_t milliseconds)
{
    int64_t end = esp_timer_get_time() + (int64_t) milliseconds * 1000;
    while (esp_timer_get_time() < end)
    {
    }
}

// The game resets the MCU after a fatal error.
void NVIC_SystemReset(void)
{
    esp_restart();
}

static void halt(void)
{
    displayPrintln(1, "");
    displayPrintln(1, "Halted. Fix the problem and reset.");
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void gameTask(void *arg)
{
    (void) arg;
    boardInit();
    displayInit();
    initGraphics();
    displayPrintln(1, "Duke Nukem 3D for " BOARD_NAME);
    displayPrintln(1, "Port of MGM240DN3D by Nicola Wrachien");
    displayPrintln(1, "Build %s %s", __DATE__, __TIME__);
    displayPrintln(1, "PSRAM free: %d kB, internal: %d kB",
                   (int) (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024),
                   (int) (heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));
    initKeyboard();
    if (!extMemInit())
    {
        displayPrintln(1, "Put " DATA_FILE_NAME " in the root of a FAT32 card.");
        halt();
    }
    uint16_t keys;
    getKeys(&keys);
    if ((keys & KEY_COMBINATION_FOR_ERASE_OPTIONS) == KEY_COMBINATION_FOR_ERASE_OPTIONS)
    {
        displayPrintln(1, "Erasing options and savegames...");
        extMemEraseNvm();
        displayPrintln(1, "Done.");
    }
    displayPrintln(1, "Starting...");
    dn3dmain(0, NULL);
    halt();
}

void app_main(void)
{
    // The engine is single threaded and keeps big arrays on the stack (the
    // audio mixer alone uses ~16 kB), so it gets its own task on core 1.
    // Bluetooth first: it needs internal RAM, and its NVS writes need a task
    // with an internal stack (this one).
    gamepadInit();
    // Internal RAM is much faster; PSRAM is the fallback if it is short.
    ESP_LOGI("main", "internal free %u, largest block %u",
             (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    if (xTaskCreatePinnedToCoreWithCaps(gameTask, "duke3d", GAME_TASK_STACK_SIZE, NULL, 5, NULL, 1,
                                        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) != pdPASS)
    {
        ESP_LOGW("main", "game stack in PSRAM");
        xTaskCreatePinnedToCoreWithCaps(gameTask, "duke3d", GAME_TASK_STACK_SIZE, NULL, 5, NULL, 1,
                                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
}
