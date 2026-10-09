/*
 *  Input of the ESP32-S3 port (input.c): board keyboard/buttons, touch
 *  controls and Bluetooth gamepads.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef PORT_KEYBOARD_H
#define PORT_KEYBOARD_H
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "config_enum_values.h"
#include "boards.h"
void initKeyboard();
// Legacy gamepad bitmask, used only by the boot menu.
#define KEY_UP                                0x0001
#define KEY_DOWN                              0x0002
#define KEY_LEFT                              0x0004
#define KEY_RIGHT                             0x0008
#define KEY_FIRE                              0x0010
#define KEY_USE                               0x0020
#define KEY_ALT                               0x0040
#define KEY_CHGW                              0x0080
void getKeys(uint16_t *keys);
// Joystick deflections in -128..127: lx/ly move, rx/ry turn/look.
void getAnalogInput(int32_t * lx, int32_t *ly, int32_t *rx, int32_t *ry);
// Scancode events (DOS set 1 codes, as used by the game's sc_* constants).
// Returns false when the queue is empty.
bool inputGetKeyEvent(uint8_t *scancode, bool *pressed);
// Set by the game: in menus the trackball and touch screen navigate (arrows,
// Enter), in game they turn/look and fire.
void inputSetMenuMode(bool menu);
bool inputIsMenuMode(void);
// Feeds the scancode queue (any input source, any task).
void inputPostKeyEvent(uint8_t scancode, bool pressed);
#endif
