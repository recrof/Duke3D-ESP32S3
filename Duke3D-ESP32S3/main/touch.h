/*
 *  On-screen touch controls (see touch.c).
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef TOUCH_H
#define TOUCH_H
#include <stdint.h>
#include <stdbool.h>
// Starts the touch task if the board has a touch panel.
void touchInit(void);
// Movement stick, -127..127 (x: right, y: down = backwards).
void touchGetStick(int32_t *x, int32_t *y);
// True while a finger is on the screen.
bool touchIsDown(void);
// Look drag since the last call, in joystick units.
void touchTakeLook(int32_t *dx, int32_t *dy);
// Display task: whether to draw the controls, and drawing one panel line
// (RGB565, byte-swapped as sent to the panel).
bool touchOverlayVisible(void);
void touchDrawOverlay(uint16_t *line, int y);
#endif
