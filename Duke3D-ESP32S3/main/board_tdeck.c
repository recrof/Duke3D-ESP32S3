/*
 *  LilyGO T-Deck board layer: peripheral power, backlight, keyboard (ESP32-C3
 *  controller at I2C 0x55) and trackball.
 *
 *  The keyboard firmware has a raw mode (command 0x03) that reports the 5x7
 *  key matrix, which gives real press/release events. Older firmware only
 *  sends one ASCII code per key press; then presses are synthesized as short
 *  taps (holding a key does not repeat).
 *
 *  A task on core 0 polls the keyboard and turns matrix changes into game
 *  scancodes, so the game never waits on I2C. The trackball pins toggle once
 *  per detent; an ISR counts the edges. During gameplay the counts become an
 *  analog "right stick" (turn / look); in menus they become arrow keys.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include "board.h"
#if BOARD == BOARD_TDECK
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "keyboard.h"           // the game's scancodes (sc_*)
#include "port/keyboard.h"

#define KB_COLS                 5
#define KB_ROWS                 7
#define KB_CMD_RAW_MODE         0x03
#define KB_POLL_MS              8
#define KB_I2C_HZ               100000
#define TAP_RELEASE_US          120000
#define TRACKBALL_GAIN          28      // joystick units per detent per game tic
#define TRACKBALL_ARROW_STEP    2       // detents per arrow key press in menus
#define INPUT_DEBUG       0

// Special (non-character) keys of the matrix.
enum
{
    K_NONE = 0, K_SYM, K_ALT, K_SPACE, K_MIC, K_LSHIFT, K_RSHIFT, K_ENTER, K_BKSP, K_DOLLAR
};
#define CHR(c)      (c)
#define SPC(k)      (0x80 | (k))
// Physical layout [column][row], from the keyboard firmware.
static const uint8_t matrix[KB_COLS][KB_ROWS] =
{
    {CHR('q'), CHR('w'), SPC(K_SYM),    CHR('a'),     SPC(K_ALT),    SPC(K_SPACE), SPC(K_MIC)},
    {CHR('e'), CHR('s'), CHR('d'),      CHR('p'),     CHR('x'),      CHR('z'),     SPC(K_LSHIFT)},
    {CHR('r'), CHR('g'), CHR('t'),      SPC(K_RSHIFT), CHR('v'),     CHR('c'),     CHR('f')},
    {CHR('u'), CHR('h'), CHR('y'),      SPC(K_ENTER), CHR('b'),      CHR('n'),     CHR('j')},
    {CHR('o'), CHR('l'), CHR('i'),      SPC(K_BKSP),  SPC(K_DOLLAR), CHR('m'),     CHR('k')},
};
// What the keys print with Sym held (as on the keycaps).
static const char symbolLayer[KB_COLS][KB_ROWS] =
{
    {'#', '1', 0,   '*', 0,   0,   '0'},
    {'2', '4', '5', '@', '8', '7', 0},
    {'3', '/', '(', 0,   '?', '9', '6'},
    {'_', ':', ')', 0,   '!', ',', ';'},
    {'+', '"', '-', 0,   0,   '.', '\''},
};

static const char *TAG = "input";

static i2c_master_bus_handle_t i2cBus;
static i2c_master_dev_handle_t keyboardDev;
static bool rawMode;
// Scancode sent for each matrix key while it is down, so the release matches
// the press even if Sym was let go in between.
static uint8_t heldScancode[KB_COLS][KB_ROWS];
static uint8_t lastMatrix[KB_COLS];
//
static volatile int32_t trackballX, trackballY;
static int32_t arrowAccX, arrowAccY;
static bool clickDown;
static uint8_t clickScancode;
// key-mode fallback: one pending synthesized release
static uint8_t tapScancode;
static int64_t tapReleaseTime;

static uint8_t charToScancode(char c)
{
    static const uint8_t letters[26] =
    {
        sc_A, sc_B, sc_C, sc_D, sc_E, sc_F, sc_G, sc_H, sc_I, sc_J, sc_K, sc_L, sc_M,
        sc_N, sc_O, sc_P, sc_Q, sc_R, sc_S, sc_T, sc_U, sc_V, sc_W, sc_X, sc_Y, sc_Z
    };
    if (c >= 'a' && c <= 'z')
    {
        return letters[c - 'a'];
    }
    if (c >= 'A' && c <= 'Z')
    {
        return letters[c - 'A'];
    }
    if (c >= '1' && c <= '9')
    {
        return sc_1 + (c - '1');
    }
    switch (c)
    {
        case '0': return sc_0;
        case ' ': return sc_Space;
        case '\r':
        case '\n': return sc_Enter;
        case 0x08: return sc_BackSpace;
        case 0x1B: return sc_Escape;
        case '$': return sc_Escape;
        case '/':
        case '?': return sc_Slash;
        case '-':
        case '_': return sc_Minus;
        case '+': return sc_Equals;
        case '*': return sc_Kpad_Star;
        case ',': return sc_Comma;
        case '.': return sc_Period;
        case ';':
        case ':': return sc_SemiColon;
        case '\'':
        case '"': return sc_Quote;
        case '#': return sc_Tilde;      // console
        case '(': return sc_OpenBracket;
        case ')': return sc_CloseBracket;
        default: return 0;
    }
}

static uint8_t matrixScancode(int col, int row, bool symHeld)
{
    uint8_t k = matrix[col][row];
    if (symHeld && symbolLayer[col][row])
    {
        return charToScancode(symbolLayer[col][row]);
    }
    if (!(k & 0x80))
    {
        return charToScancode(k);
    }
    switch (k & 0x7F)
    {
        case K_ALT: return sc_LeftControl;      // second fire button
        case K_SPACE: return sc_Space;
        case K_MIC: return sc_Tab;              // map
        case K_LSHIFT: return sc_LeftShift;
        case K_RSHIFT: return sc_RightShift;
        case K_ENTER: return sc_Enter;
        case K_BKSP: return sc_BackSpace;
        case K_DOLLAR: return sc_Escape;        // menu
        default: return 0;                      // Sym is only a modifier
    }
}

#define postEvent(scancode, pressed)    inputPostKeyEvent((scancode), (pressed))

static void IRAM_ATTR trackballIsr(void *arg)
{
    switch ((int) arg)
    {
        case PIN_TRACKBALL_UP: trackballY--; break;
        case PIN_TRACKBALL_DOWN: trackballY++; break;
        case PIN_TRACKBALL_LEFT: trackballX--; break;
        case PIN_TRACKBALL_RIGHT: trackballX++; break;
    }
}

static void pollRawMatrix(void)
{
    uint8_t state[KB_COLS];
    if (i2c_master_receive(keyboardDev, state, sizeof(state), 20) != ESP_OK)
    {
        return;
    }
    bool symHeld = (state[0] >> 2) & 1;
    for (int col = 0; col < KB_COLS; col++)
    {
        uint8_t changed = state[col] ^ lastMatrix[col];
        for (int row = 0; changed && row < KB_ROWS; row++)
        {
            if (!(changed & (1 << row)))
            {
                continue;
            }
            if (state[col] & (1 << row))
            {
                heldScancode[col][row] = matrixScancode(col, row, symHeld);
                postEvent(heldScancode[col][row], true);
            }
            else
            {
                postEvent(heldScancode[col][row], false);
                heldScancode[col][row] = 0;
            }
        }
        lastMatrix[col] = state[col];
    }
}

static void pollKeyMode(void)
{
    int64_t now = esp_timer_get_time();
    if (tapScancode && now >= tapReleaseTime)
    {
        postEvent(tapScancode, false);
        tapScancode = 0;
    }
    uint8_t c = 0;
    if (i2c_master_receive(keyboardDev, &c, 1, 20) != ESP_OK || !c)
    {
        return;
    }
    uint8_t sc = charToScancode((char) c);
    if (!sc)
    {
        return;
    }
    if (tapScancode)
    {
        postEvent(tapScancode, false);
    }
    postEvent(sc, true);
    tapScancode = sc;
    tapReleaseTime = now + TAP_RELEASE_US;
}

static void pollTrackball(void)
{
#if INPUT_DEBUG
    static int32_t loggedX, loggedY;
    if (trackballX != loggedX || trackballY != loggedY)
    {
        ESP_LOGI(TAG, "trackball x %ld y %ld click %d menu %d", (long) trackballX, (long) trackballY,
                 !gpio_get_level(PIN_TRACKBALL_CLICK), inputIsMenuMode());
        loggedX = trackballX;
        loggedY = trackballY;
    }
#endif
    bool menuMode = inputIsMenuMode();
    bool down = !gpio_get_level(PIN_TRACKBALL_CLICK);
    if (down != clickDown)
    {
        clickDown = down;
        if (down)
        {
            // Fire in game, select in menus.
            clickScancode = menuMode ? sc_Enter : sc_LeftControl;
            postEvent(clickScancode, true);
        }
        else
        {
            postEvent(clickScancode, false);
        }
    }
    if (!menuMode)
    {
        arrowAccX = arrowAccY = 0;
        return;
    }
    // Menus: turn detents into arrow taps; the game reads joystick values too
    // but menus ignore them.
    int32_t dx = __atomic_exchange_n(&trackballX, 0, __ATOMIC_SEQ_CST);
    int32_t dy = __atomic_exchange_n(&trackballY, 0, __ATOMIC_SEQ_CST);
    arrowAccX += dx;
    arrowAccY += dy;
    while (arrowAccY <= -TRACKBALL_ARROW_STEP)
    {
        postEvent(sc_UpArrow, true);
        postEvent(sc_UpArrow, false);
        arrowAccY += TRACKBALL_ARROW_STEP;
    }
    while (arrowAccY >= TRACKBALL_ARROW_STEP)
    {
        postEvent(sc_DownArrow, true);
        postEvent(sc_DownArrow, false);
        arrowAccY -= TRACKBALL_ARROW_STEP;
    }
    while (arrowAccX <= -TRACKBALL_ARROW_STEP)
    {
        postEvent(sc_LeftArrow, true);
        postEvent(sc_LeftArrow, false);
        arrowAccX += TRACKBALL_ARROW_STEP;
    }
    while (arrowAccX >= TRACKBALL_ARROW_STEP)
    {
        postEvent(sc_RightArrow, true);
        postEvent(sc_RightArrow, false);
        arrowAccX -= TRACKBALL_ARROW_STEP;
    }
}

static void inputTask(void *arg)
{
    (void) arg;
    TickType_t last = xTaskGetTickCount();
    while (1)
    {
        if (keyboardDev)
        {
            if (rawMode)
            {
                pollRawMatrix();
            }
            else
            {
                pollKeyMode();
            }
        }
        pollTrackball();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(KB_POLL_MS));
    }
}

static i2c_master_bus_handle_t boardI2cBus(void)
{
    if (!i2cBus)
    {
        i2c_master_bus_config_t cfg =
        {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = PIN_I2C_SDA,
            .scl_io_num = PIN_I2C_SCL,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &i2cBus));
    }
    return i2cBus;
}

static void initKeyboardController(void)
{
    i2c_master_bus_handle_t bus = boardI2cBus();
    // The C3 boots in parallel with us; give it a moment to show up.
    esp_err_t probe = ESP_FAIL;
    for (int i = 0; i < 20 && probe != ESP_OK; i++)
    {
        probe = i2c_master_probe(bus, KEYBOARD_I2C_ADDRESS, 20);
        if (probe != ESP_OK)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    if (probe != ESP_OK)
    {
        ESP_LOGE(TAG, "keyboard not found at 0x%02x", KEYBOARD_I2C_ADDRESS);
        return;
    }
    i2c_device_config_t dev =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = KEYBOARD_I2C_ADDRESS,
        .scl_speed_hz = KB_I2C_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev, &keyboardDev));
    // Ask for raw matrix reports. Raw rows only use 7 bits, while older
    // firmware pads a 5-byte read of its 1-byte reply with 0xFF.
    const uint8_t cmd = KB_CMD_RAW_MODE;
    i2c_master_transmit(keyboardDev, &cmd, 1, 50);
    vTaskDelay(pdMS_TO_TICKS(20));
    uint8_t probeState[KB_COLS];
    rawMode = i2c_master_receive(keyboardDev, probeState, sizeof(probeState), 50) == ESP_OK;
    for (int i = 0; rawMode && i < KB_COLS; i++)
    {
        rawMode = !(probeState[i] & 0x80);
    }
    memcpy(lastMatrix, probeState, sizeof(lastMatrix));
    if (!rawMode)
    {
        memset(lastMatrix, 0, sizeof(lastMatrix));
    }
    ESP_LOGI(TAG, "keyboard: %s mode", rawMode ? "raw matrix" : "key (tap only, update keyboard firmware for held keys)");
}

void boardInputInit(void)
{
    static bool done;
    if (done)
    {
        return;
    }
    done = true;
    initKeyboardController();
    //
    gpio_config_t click =
    {
        .pin_bit_mask = 1ULL << PIN_TRACKBALL_CLICK,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&click);
    gpio_config_t ball =
    {
        .pin_bit_mask = (1ULL << PIN_TRACKBALL_UP) | (1ULL << PIN_TRACKBALL_DOWN) |
                        (1ULL << PIN_TRACKBALL_LEFT) | (1ULL << PIN_TRACKBALL_RIGHT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&ball);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    const int pins[] = {PIN_TRACKBALL_UP, PIN_TRACKBALL_DOWN, PIN_TRACKBALL_LEFT, PIN_TRACKBALL_RIGHT};
    for (unsigned int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    {
        gpio_isr_handler_add(pins[i], trackballIsr, (void*) pins[i]);
    }
    xTaskCreatePinnedToCore(inputTask, "input", 3072, NULL, 9, NULL, 0);
}

uint16_t boardBootKeys(void)
{
    // Read the matrix directly: the input task may not run yet.
    uint16_t k = 0;
    if (keyboardDev && rawMode)
    {
        uint8_t state[KB_COLS] = {0};
        i2c_master_receive(keyboardDev, state, sizeof(state), 20);
        if (state[0] & (1 << 1)) k |= KEY_UP;       // w
        if (state[1] & (1 << 1)) k |= KEY_DOWN;     // s
        if (state[0] & (1 << 3)) k |= KEY_LEFT;     // a
        if (state[1] & (1 << 2)) k |= KEY_RIGHT;    // d
        if (state[0] & (1 << 4)) k |= KEY_ALT;      // alt
        if (state[0] & (1 << 5)) k |= KEY_USE;      // space
    }
    if (!gpio_get_level(PIN_TRACKBALL_CLICK))
    {
        k |= KEY_FIRE;
    }
    return k;
}

void boardTakeLook(int32_t *dx, int32_t *dy)
{
    *dx = __atomic_exchange_n(&trackballX, 0, __ATOMIC_SEQ_CST) * TRACKBALL_GAIN;
    *dy = __atomic_exchange_n(&trackballY, 0, __ATOMIC_SEQ_CST) * TRACKBALL_GAIN;
}

void boardInit(void)
{
    // Peripheral power, then every chip select on the shared SPI bus high.
    const int outputsHigh[] = {PIN_POWER_ON, PIN_TFT_CS, PIN_SD_CS, PIN_LORA_CS};
    for (unsigned int i = 0; i < sizeof(outputsHigh) / sizeof(outputsHigh[0]); i++)
    {
        gpio_reset_pin(outputsHigh[i]);
        gpio_set_direction(outputsHigh[i], GPIO_MODE_OUTPUT);
        gpio_set_level(outputsHigh[i], 1);
    }
    gpio_reset_pin(PIN_TFT_BACKLIGHT);
    gpio_set_direction(PIN_TFT_BACKLIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_TFT_BACKLIGHT, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
}

void boardBacklight(bool on)
{
    gpio_set_level(PIN_TFT_BACKLIGHT, on);
}

void boardAudioCodecInit(void)
{
    // MAX98357A: nothing to configure.
}

bool boardHasTouch(void)
{
    return false;
}

int boardTouchRead(boardTouchPoint_t *points)
{
    (void) points;
    return -1;
}
#endif
