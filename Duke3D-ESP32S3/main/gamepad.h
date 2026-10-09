/*
 *  Bluetooth LE gamepad (see gamepad.c).
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef GAMEPAD_H
#define GAMEPAD_H
#include <stdbool.h>
#include <stdint.h>
void gamepadInit(void);
bool gamepadConnected(void);
// Stick deflections in -128..127 (zero when no pad is connected).
void gamepadGetAxes(int32_t *lx, int32_t *ly, int32_t *rx, int32_t *ry);
#endif
