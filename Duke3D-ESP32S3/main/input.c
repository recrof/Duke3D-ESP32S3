/*
 *  Input core of the ESP32-S3 port.
 *
 *  Every input source (board keyboard and buttons, touch controls, Bluetooth
 *  gamepad) sends scancodes into one queue, which the game's keyhandler()
 *  drains. Analog values (sticks, trackball, touch drags) are merged in
 *  getAnalogInput(), read by the game's joystick code.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "port/keyboard.h"
#include "board.h"
#include "gamepad.h"
#include "touch.h"

#define EVENT_QUEUE_LEN         64

typedef struct
{
    uint8_t scancode;
    uint8_t pressed;
} keyEvent_t;

static QueueHandle_t events;
static volatile bool menuMode = true;    // until the game reports gameplay (keyhandler)

static int32_t clampAxis(int32_t v)
{
    return v > 127 ? 127 : (v < -128 ? -128 : v);
}

void initKeyboard()
{
    static bool done;
    if (done)
    {
        return;
    }
    done = true;
    events = xQueueCreate(EVENT_QUEUE_LEN, sizeof(keyEvent_t));
    boardInputInit();
    touchInit();
}

bool inputGetKeyEvent(uint8_t *scancode, bool *pressed)
{
    keyEvent_t e;
    if (!events || xQueueReceive(events, &e, 0) != pdTRUE)
    {
        return false;
    }
    *scancode = e.scancode;
    *pressed = e.pressed;
    return true;
}

void inputSetMenuMode(bool menu)
{
    menuMode = menu;
}

bool inputIsMenuMode(void)
{
    return menuMode;
}

void inputPostKeyEvent(uint8_t scancode, bool pressed)
{
    if (!events || !scancode)
    {
        return;
    }
    keyEvent_t e = {.scancode = scancode, .pressed = pressed};
    xQueueSend(events, &e, 0);
}

void getKeys(uint16_t *keys)
{
    *keys = boardBootKeys();
}

void getAnalogInput(int32_t *lx, int32_t *ly, int32_t *rx, int32_t *ry)
{
    if (menuMode)
    {
        *lx = *ly = *rx = *ry = 0;
        return;
    }
    int32_t padLx, padLy, padRx, padRy;
    gamepadGetAxes(&padLx, &padLy, &padRx, &padRy);
    int32_t touchX, touchY;
    touchGetStick(&touchX, &touchY);
    *lx = clampAxis(padLx + touchX);
    *ly = clampAxis(padLy + touchY);
    // Relative look devices (trackball, touch drags): the game calls this once
    // per axis within a tic, so consume their movement only on the first call
    // of a burst and repeat it for the rest.
    static int64_t lastSample;
    static int32_t lookX, lookY;
    int64_t now = esp_timer_get_time();
    if (now - lastSample > 2000)
    {
        int32_t bx, by, tx, ty;
        boardTakeLook(&bx, &by);
        touchTakeLook(&tx, &ty);
        lookX = bx + tx;
        lookY = by + ty;
    }
    lastSample = now;
    *rx = clampAxis(lookX + padRx);
    *ry = clampAxis(lookY + padRy);
}
