/*
 *  On-screen touch controls, for boards with a touch panel (Wio Tracker L2).
 *  Adapted from the touch controls of the ESP32-S3 Quake port.
 *
 *  Playing (panel coordinates, 320x240 landscape):
 *  - left half: virtual stick, centered where the finger lands (move / strafe)
 *  - right half: drag to turn / look
 *  - FIRE, USE (open), JUMP, CROUCH and NEXT WEAPON buttons on the right
 *    (dragging from FIRE also looks)
 *  - MENU in the top left corner (Escape)
 *  Menus: swipe up/down/left/right = arrow keys, tap = Enter (also "yes"),
 *  tap on the MENU corner = Escape (back, "no").
 *  The controls are drawn over the 3D view; they are hidden when a Bluetooth
 *  gamepad connects, until the screen is touched again.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "esp_timer.h"
#include "keyboard.h"           // the game's scancodes (sc_*)
#include "port/keyboard.h"
#include "font8x8.h"
#include "board.h"
#include "gamepad.h"
#include "touch.h"

#define TOUCH_POLL_MS           10
#define STICK_RADIUS            40      // pixels for full deflection
#define LOOK_SPLIT_X            (SCREEN_WIDTH / 2)
#define LOOK_GAIN               10      // joystick units per pixel of drag
#define HIT_SLOP                8       // extra radius accepted around buttons
#define TAP_MAX_MOVE            12
#define TAP_MAX_US              400000
#define SWIPE_STEP              24      // pixels per arrow key in menus

typedef enum
{
    ROLE_NONE, ROLE_STICK, ROLE_LOOK, ROLE_BUTTON, ROLE_MENU, ROLE_MENU_NAV
} touchRole_t;

typedef struct
{
    int16_t x, y, r;
    uint8_t scancode;
    char label;
    bool looks;                 // dragging from it also turns / looks
} touchButton_t;

typedef struct
{
    bool active;
    bool seen;                  // present in the latest report
    bool dragged;               // menus: moved enough to be a swipe
    bool escapeHeld;            // MENU pressed during gameplay
    uint8_t id;
    touchRole_t role;
    const touchButton_t *button;
    int16_t x, y;               // current position
    int16_t x0, y0;             // stick center / swipe reference
    int16_t lastX, lastY;
    int64_t downTime;
} touchSlot_t;

// Kept inside the 3D view, above the 40 pixel status bar. The scancodes are
// the second bindings of the keydefaults table (_functio.h).
static const touchButton_t buttons[] =
{
    {270, 150, 28, sc_RightControl, 'F', true},     // fire
    {214, 172, 18, sc_Space, 'U', false},           // use / open
    {296, 90, 18, sc_Slash, 'J', false},            // jump
    {250, 98, 15, sc_Z, 'C', false},                // crouch
    {296, 36, 15, sc_Quote, 'W', false},            // next weapon
};
#define NUM_BUTTONS             ((int) (sizeof(buttons) / sizeof(buttons[0])))
#define MENU_X0                 6
#define MENU_Y0                 6
#define MENU_X1                 46
#define MENU_Y1                 34

static touchSlot_t slots[BOARD_TOUCH_MAX_POINTS];
static portMUX_TYPE touchMux = portMUX_INITIALIZER_UNLOCKED;
static float lookDX, lookDY;            // pixels not yet consumed
static volatile uint32_t buttonsDown;   // bit per buttons[] entry
static volatile bool overlayVisible;
static bool padWasConnected;

static bool inMenuArea(int x, int y)
{
    return x < MENU_X1 + HIT_SLOP && y < MENU_Y1 + HIT_SLOP;
}

static const touchButton_t *buttonAt(int x, int y)
{
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        int dx = x - buttons[i].x, dy = y - buttons[i].y, r = buttons[i].r + HIT_SLOP;
        if (dx * dx + dy * dy <= r * r)
        {
            return &buttons[i];
        }
    }
    return NULL;
}

static void tapKey(uint8_t scancode)
{
    inputPostKeyEvent(scancode, true);
    inputPostKeyEvent(scancode, false);
}

static void touchDown(touchSlot_t *s)
{
    s->x0 = s->lastX = s->x;
    s->y0 = s->lastY = s->y;
    s->dragged = false;
    s->button = NULL;
    s->escapeHeld = false;
    s->downTime = esp_timer_get_time();
    if (inputIsMenuMode())
    {
        s->role = inMenuArea(s->x, s->y) ? ROLE_MENU : ROLE_MENU_NAV;
        return;
    }
    if (inMenuArea(s->x, s->y))
    {
        s->role = ROLE_MENU;
        s->escapeHeld = true;
        inputPostKeyEvent(sc_Escape, true);
    }
    else if ((s->button = buttonAt(s->x, s->y)) != NULL)
    {
        s->role = ROLE_BUTTON;
        inputPostKeyEvent(s->button->scancode, true);
    }
    else
    {
        s->role = s->x < LOOK_SPLIT_X ? ROLE_STICK : ROLE_LOOK;
    }
}

static void touchMove(touchSlot_t *s)
{
    switch (s->role)
    {
        case ROLE_BUTTON:
            if (!s->button->looks)
            {
                break;
            }
            // fall through
        case ROLE_LOOK:
            portENTER_CRITICAL(&touchMux);
            lookDX += s->x - s->lastX;
            lookDY += s->y - s->lastY;
            portEXIT_CRITICAL(&touchMux);
            break;
        case ROLE_STICK:
        {
            // Drag the stick center along when going past the edge.
            float dx = s->x - s->x0, dy = s->y - s->y0;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > STICK_RADIUS)
            {
                s->x0 = s->x - dx * STICK_RADIUS / d;
                s->y0 = s->y - dy * STICK_RADIUS / d;
            }
            break;
        }
        case ROLE_MENU_NAV:
        {
            int dx = s->x - s->x0, dy = s->y - s->y0;
            if (abs(dx) > TAP_MAX_MOVE || abs(dy) > TAP_MAX_MOVE)
            {
                s->dragged = true;
            }
            if (abs(dy) >= SWIPE_STEP && abs(dy) >= abs(dx))
            {
                tapKey(dy < 0 ? sc_UpArrow : sc_DownArrow);
                s->x0 = s->x;
                s->y0 = s->y;
            }
            else if (abs(dx) >= SWIPE_STEP)
            {
                tapKey(dx < 0 ? sc_LeftArrow : sc_RightArrow);
                s->x0 = s->x;
                s->y0 = s->y;
            }
            break;
        }
        default:
            break;
    }
    s->lastX = s->x;
    s->lastY = s->y;
}

static void touchUp(touchSlot_t *s)
{
    bool tap = !s->dragged && esp_timer_get_time() - s->downTime < TAP_MAX_US;
    switch (s->role)
    {
        case ROLE_BUTTON:
            inputPostKeyEvent(s->button->scancode, false);
            break;
        case ROLE_MENU:
            // The menu opened by the press must not get a second Escape.
            if (s->escapeHeld)
            {
                inputPostKeyEvent(sc_Escape, false);
            }
            else if (tap)
            {
                tapKey(sc_Escape);
            }
            break;
        case ROLE_MENU_NAV:
            if (tap)
            {
                tapKey(sc_Enter);
            }
            break;
        default:
            break;
    }
    s->active = false;
    s->role = ROLE_NONE;
    s->button = NULL;
}

static void updateButtonsDown(void)
{
    uint32_t b = 0;
    for (int i = 0; i < BOARD_TOUCH_MAX_POINTS; i++)
    {
        if (slots[i].active && slots[i].role == ROLE_BUTTON)
        {
            b |= 1u << (slots[i].button - buttons);
        }
    }
    buttonsDown = b;
}

static void touchTask(void *arg)
{
    (void) arg;
    boardTouchPoint_t points[BOARD_TOUCH_MAX_POINTS];
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(TOUCH_POLL_MS));
        int n = boardTouchRead(points);
        if (n < 0)
        {
            continue;
        }
        for (int i = 0; i < BOARD_TOUCH_MAX_POINTS; i++)
        {
            slots[i].seen = false;
        }
        for (int p = 0; p < n; p++)
        {
            touchSlot_t *s = NULL, *free = NULL;
            for (int i = 0; i < BOARD_TOUCH_MAX_POINTS; i++)
            {
                if (slots[i].active && slots[i].id == points[p].id)
                {
                    s = &slots[i];
                }
                else if (!slots[i].active && !free)
                {
                    free = &slots[i];
                }
            }
            if (!s && !free)
            {
                continue;
            }
            if (!s)
            {
                s = free;
                s->active = true;
                s->id = points[p].id;
                s->x = points[p].x;
                s->y = points[p].y;
                touchDown(s);
                overlayVisible = true;
            }
            else
            {
                s->x = points[p].x;
                s->y = points[p].y;
                touchMove(s);
            }
            s->seen = true;
        }
        for (int i = 0; i < BOARD_TOUCH_MAX_POINTS; i++)
        {
            if (slots[i].active && !slots[i].seen)
            {
                touchUp(&slots[i]);
            }
        }
        updateButtonsDown();
    }
}

void touchInit(void)
{
    if (!boardHasTouch())
    {
        return;
    }
    overlayVisible = true;
    // Stack in PSRAM: internal RAM is short, and this task never touches flash.
    if (xTaskCreatePinnedToCoreWithCaps(touchTask, "touch", 3072, NULL, 8, NULL, 0, MALLOC_CAP_SPIRAM) != pdPASS)
    {
        xTaskCreatePinnedToCore(touchTask, "touch", 3072, NULL, 8, NULL, 0);
    }
}

void touchGetStick(int32_t *x, int32_t *y)
{
    *x = *y = 0;
    for (int i = 0; i < BOARD_TOUCH_MAX_POINTS; i++)
    {
        const touchSlot_t *s = &slots[i];
        if (s->active && s->role == ROLE_STICK)
        {
            int32_t dx = (s->x - s->x0) * 127 / STICK_RADIUS;
            int32_t dy = (s->y - s->y0) * 127 / STICK_RADIUS;
            *x = dx < -127 ? -127 : (dx > 127 ? 127 : dx);
            *y = dy < -127 ? -127 : (dy > 127 ? 127 : dy);
            return;
        }
    }
}

bool touchIsDown(void)
{
    for (int i = 0; i < BOARD_TOUCH_MAX_POINTS; i++)
    {
        if (slots[i].active)
        {
            return true;
        }
    }
    return false;
}

static int32_t takeLookAxis(float *pixels)
{
    // At most a full deflection per call: the rest of a fast swipe is kept
    // for the next tics instead of being clipped away.
    const float maxPixels = 127.0f / LOOK_GAIN;
    float p = *pixels;
    p = p > maxPixels ? maxPixels : (p < -maxPixels ? -maxPixels : p);
    *pixels -= p;
    return (int32_t) (p * LOOK_GAIN);
}

void touchTakeLook(int32_t *dx, int32_t *dy)
{
    portENTER_CRITICAL(&touchMux);
    *dx = takeLookAxis(&lookDX);
    *dy = takeLookAxis(&lookDY);
    portEXIT_CRITICAL(&touchMux);
}

bool touchOverlayVisible(void)
{
    bool pad = gamepadConnected();
    if (pad && !padWasConnected)
    {
        overlayVisible = false;
    }
    padWasConnected = pad;
    return overlayVisible && !inputIsMenuMode();
}

// ---------------------------------------------------------------------------
// Overlay drawing (display task). Pixels are RGB565, byte-swapped.
// ---------------------------------------------------------------------------
static inline uint16_t swap16(uint16_t v)
{
    return (v >> 8) | (v << 8);
}

static void lightenSpan(uint16_t *line, int x0, int x1, int strength)
{
    if (x0 < 0)
    {
        x0 = 0;
    }
    if (x1 >= SCREEN_WIDTH)
    {
        x1 = SCREEN_WIDTH - 1;
    }
    for (int x = x0; x <= x1; x++)
    {
        uint16_t v = swap16(line[x]);
        // 50% (strength 1) or 75% (strength 2) towards white
        v = strength == 1 ? ((v & 0xF7DE) >> 1) + 0x7BEF : ((v & 0xE79C) >> 2) + 0xBDF7;
        line[x] = swap16(v);
    }
}

static void drawCircle(uint16_t *line, int y, int cx, int cy, int r, bool filled)
{
    const int thickness = 2;
    int dy = y - cy;
    if (dy < -r || dy > r)
    {
        return;
    }
    int wo = (int) sqrtf((float) (r * r - dy * dy));
    int ri = r - thickness;
    int wi = (dy >= -ri && dy <= ri) ? (int) sqrtf((float) (ri * ri - dy * dy)) : -1;
    if (filled)
    {
        lightenSpan(line, cx - wo, cx + wo, 1);
    }
    else if (wi < 0)
    {
        lightenSpan(line, cx - wo, cx + wo, 1);
    }
    else
    {
        lightenSpan(line, cx - wo, cx - wi - 1, 1);
        lightenSpan(line, cx + wi + 1, cx + wo, 1);
    }
}

static void drawLabel(uint16_t *line, int y, int cx, int cy, char c)
{
    int row = y - (cy - FONT_HEIGHT / 2);
    if (row < 0 || row >= FONT_HEIGHT)
    {
        return;
    }
    uint8_t bits = font8x8_basic[(int) c & 0x7F][row];
    for (int i = 0; i < FONT_WIDTH; i++, bits >>= 1)
    {
        int x = cx - FONT_WIDTH / 2 + i;
        if ((bits & 1) && x >= 0 && x < SCREEN_WIDTH)
        {
            lightenSpan(line, x, x, 2);
        }
    }
}

void touchDrawOverlay(uint16_t *line, int y)
{
    uint32_t pressed = buttonsDown;
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        const touchButton_t *b = &buttons[i];
        drawCircle(line, y, b->x, b->y, b->r, (pressed >> i) & 1);
        drawLabel(line, y, b->x, b->y, b->label);
    }
    // Menu button: outline and three bars.
    if (y >= MENU_Y0 && y <= MENU_Y1)
    {
        if (y <= MENU_Y0 + 1 || y >= MENU_Y1 - 1)
        {
            lightenSpan(line, MENU_X0, MENU_X1, 1);
        }
        else
        {
            lightenSpan(line, MENU_X0, MENU_X0 + 1, 1);
            lightenSpan(line, MENU_X1 - 1, MENU_X1, 1);
            int r = y - MENU_Y0;
            if (r == 9 || r == 10 || r == 14 || r == 15 || r == 19 || r == 20)
            {
                lightenSpan(line, MENU_X0 + 10, MENU_X1 - 10, 1);
            }
        }
    }
    // Stick: base ring and knob.
    for (int i = 0; i < BOARD_TOUCH_MAX_POINTS; i++)
    {
        const touchSlot_t *s = &slots[i];
        if (s->active && s->role == ROLE_STICK)
        {
            drawCircle(line, y, s->x0, s->y0, STICK_RADIUS, false);
            int32_t kx, ky;
            touchGetStick(&kx, &ky);
            drawCircle(line, y, s->x0 + kx * STICK_RADIUS / 127, s->y0 + ky * STICK_RADIUS / 127, 12, true);
        }
    }
}
