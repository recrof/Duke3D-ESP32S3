/*
 *  Board layer interface of the ESP32-S3 port, implemented by board_<name>.c
 *  (only the file of the selected BOARD compiles to anything).
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef BOARD_H
#define BOARD_H
#include <stdbool.h>
#include <stdint.h>
#include "main.h"
#include "boards.h"
// PSRAM budget (8 MB, of which ~1 MB holds code and rodata copies): the level
// cache arena (the MG24 had well under 1 MB of internal flash for it) and,
// with whatever is left, the GRP page cache.
#define INTERNAL_FLASH_ARENA_SIZE       (2 * 1024 * 1024)
#define PSRAM_RESERVE                   (384 * 1024)    // left for small runtime allocations
//
// Power rails, GPIO expanders, resets. Called before displayInit().
void boardInit(void);
// Called once the panel has been cleared (and with false before it is set up).
void boardBacklight(bool on);
// Configures the audio codec, if any. The I2S clocks are already running.
void boardAudioCodecInit(void);
// Starts the board input devices (keyboard, trackball, buttons). They send
// scancodes with inputPostKeyEvent().
void boardInputInit(void);
// Boot chord bits (KEY_* in port/keyboard.h) held right now.
uint16_t boardBootKeys(void);
// Look movement since the last call, in joystick units (trackball).
void boardTakeLook(int32_t *dx, int32_t *dy);
// Touch panel, used by touch.c.
#define BOARD_TOUCH_MAX_POINTS          5
typedef struct
{
    uint8_t id;                 // tracking id, stable while the finger stays down
    int16_t x, y;               // panel coordinates, 320x240 landscape
} boardTouchPoint_t;
bool boardHasTouch(void);
// Returns -1 if there is no new data, else the number of points down (0: all released).
int boardTouchRead(boardTouchPoint_t *points);
#endif
