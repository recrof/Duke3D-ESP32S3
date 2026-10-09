/*
 *  Display driver for the 320x240 panels of the ESP32-S3 port:
 *  - ST7789 on 4-wire SPI (T-Deck, where the SD card shares the bus),
 *  - NV3031B on QSPI (Wio Tracker L2): commands are sent as the single line
 *    frame 0x02 0x00 <cmd> 0x00 followed by the parameters, pixel data as
 *    0x32 0x00 0x2C 0x00 followed by quad data, with CS held low.
 *
 *  Frames are handed over by startDisplayRefresh() (graphics.c) as an 8-bit
 *  snapshot plus the palette that was current when it was taken; a task on
 *  core 0 converts it to RGB565 in strips and streams the strips with queued
 *  DMA transactions, so the game on core 1 never waits for the panel unless
 *  it outruns it by two frames. The touch controls are drawn over the strips.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "hal/gpio_ll.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "main.h"
#include "boards.h"
#include "graphics.h"
#include "display_frame.h"
#include "board.h"
#include "touch.h"

#define STRIP_LINES                 8
#define STRIP_PIXELS                (SCREEN_WIDTH * STRIP_LINES)
#define NUM_STRIPS                  2
#define IS_QSPI                     (DISPLAY_CONTROLLER == DISPLAY_CONTROLLER_NV3031B)
//
#define CMD_SWRESET                 0x01
#define CMD_SLPOUT                  0x11
#define CMD_NORON                   0x13
#define CMD_INVOFF                  0x20
#define CMD_INVON                   0x21
#define CMD_DISPON                  0x29
#define CMD_CASET                   0x2A
#define CMD_RASET                   0x2B
#define CMD_RAMWR                   0x2C
#define CMD_MADCTL                  0x36
#define CMD_COLMOD                  0x3A
#define CMD_DELAY                   0x80    // flag in the argument count: a delay (ms) follows the arguments
#define CMD_END                     0xFF
#define QSPI_CMD_WRITE_REG          0x02
#define QSPI_CMD_WRITE_PIXELS       0x32

static const char *TAG = "display";

static spi_device_handle_t panel;
static uint16_t *strips[NUM_STRIPS];
static QueueHandle_t readyFrames;
static QueueHandle_t freeFrames;
static displayFrame_t frames[DISPLAY_NUM_FRAMES];
static volatile int framesInFlight;

#if DISPLAY_CONTROLLER == DISPLAY_CONTROLLER_NV3031B
// From the LovyanGFX Panel_NV3031B driver
static const uint8_t initCommands[] =
{
    0xFD, 2, 0x06, 0x08,                        // unlock factory registers
    0x60, 1, 0x0C,
    0x61, 2, 0x07, 0x04,
    0xB4, 1, 0x01,
    0xB1, 3, 0x0F, 0x02, 0x03,
    0xB5, 4, 0x02, 0x02, 0x0A, 0x14,
    0xB6, 5, 0x44, 0x01, 0x9F, 0x00, 0x02,
    0xDF, 1, 0x11,
    0x67, 1, 0x21,
    0x68, 4, 0x90, 0x4F, 0x27, 0x21,
    0xE1, 2, 0x20, 0x69,
    0xE4, 2, 0x69, 0x20,
    0xE2, 6, 0x10, 0x12, 0x12, 0x30, 0x39, 0x3F,
    0xE5, 6, 0x3F, 0x33, 0x2D, 0x12, 0x12, 0x10,
    0xE0, 8, 0x06, 0x06, 0x0B, 0x12, 0x11, 0x11, 0x0E, 0x19,
    0xE3, 8, 0x19, 0x13, 0x14, 0x14, 0x14, 0x12, 0x08, 0x05,
    0xE6, 2, 0x00, 0xFF,
    0xE7, 6, 0x01, 0x04, 0x03, 0x03, 0x00, 0x12,
    0xE8, 3, 0x00, 0x70, 0x00,
    0xEC, 1, 0x54,
    0xFD, 2, 0xFA, 0xFC,                        // lock factory registers
    CMD_COLMOD, 1, 0x55,                        // 16 bit/pixel
    CMD_SLPOUT, CMD_DELAY, 120,
    CMD_MADCTL, 1, DISPLAY_MADCTL,
    DISPLAY_INVERT_COLORS ? CMD_INVON : CMD_INVOFF, 0,
    CMD_END
};
#else
static const uint8_t initCommands[] =
{
    CMD_SWRESET, CMD_DELAY, 150,
    CMD_SLPOUT, CMD_DELAY, 120,
    CMD_COLMOD, 1, 0x55,                        // 16 bit/pixel
    CMD_MADCTL, 1, DISPLAY_MADCTL,
    DISPLAY_INVERT_COLORS ? CMD_INVON : CMD_INVOFF, 0,
    CMD_NORON, CMD_DELAY, 10,
    CMD_END
};
#endif

#if !IS_QSPI
static void IRAM_ATTR panelPreTransfer(spi_transaction_t *t)
{
    gpio_ll_set_level(&GPIO, PIN_TFT_DC, (int) t->user);
}
#endif

static void panelCommand(uint8_t cmd, const uint8_t *data, int len)
{
#if IS_QSPI
    spi_transaction_t t =
    {
        .cmd = QSPI_CMD_WRITE_REG,
        .addr = cmd << 8,
        .length = 8 * len,
        .tx_buffer = len ? data : NULL,     // no data phase for commands without parameters
    };
    spi_device_polling_transmit(panel, &t);
#else
    spi_transaction_t t =
    {
        .length = 8,
        .tx_buffer = &cmd,
        .user = (void*) 0,
    };
    spi_device_polling_transmit(panel, &t);
    if (len)
    {
        spi_transaction_t d =
        {
            .length = 8 * len,
            .tx_buffer = data,
            .user = (void*) 1,
        };
        spi_device_polling_transmit(panel, &d);
    }
#endif
}

static void panelCommands(const uint8_t *cmds)
{
    while (*cmds != CMD_END)
    {
        uint8_t cmd = *cmds++;
        uint8_t numArgs = *cmds++;
        bool hasDelay = numArgs & CMD_DELAY;
        numArgs &= ~CMD_DELAY;
        panelCommand(cmd, cmds, numArgs);
        cmds += numArgs;
        if (hasDelay)
        {
            vTaskDelay(pdMS_TO_TICKS(*cmds++));
        }
    }
}

static void panelSetWindow(int rows)
{
    const uint8_t caset[] = {0, 0, (SCREEN_WIDTH - 1) >> 8, (SCREEN_WIDTH - 1) & 0xFF};
    const uint8_t raset[] = {0, 0, (rows - 1) >> 8, (rows - 1) & 0xFF};
    panelCommand(CMD_CASET, caset, sizeof(caset));
    panelCommand(CMD_RASET, raset, sizeof(raset));
#if IS_QSPI
    // The pixel transfers keep CS low across several transactions.
    spi_device_acquire_bus(panel, portMAX_DELAY);
#else
    panelCommand(CMD_RAMWR, NULL, 0);
#endif
}

static void panelQueueStrip(spi_transaction_ext_t *t, const uint16_t *data, int pixels, bool first, bool last)
{
    memset(t, 0, sizeof(*t));
    t->base.length = pixels * 16;
    t->base.tx_buffer = data;
#if IS_QSPI
    t->base.flags = SPI_TRANS_MODE_QIO | (last ? 0 : SPI_TRANS_CS_KEEP_ACTIVE);
    if (first)
    {
        t->base.cmd = QSPI_CMD_WRITE_PIXELS;
        t->base.addr = CMD_RAMWR << 8;
    }
    else
    {
        // Continuation: no command/address phase.
        t->base.flags |= SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    }
#else
    (void) first;
    (void) last;
    t->base.user = (void*) 1;
#endif
    spi_device_queue_trans(panel, &t->base, portMAX_DELAY);
}

static void displayTask(void *arg)
{
    (void) arg;
    static spi_transaction_ext_t trans[NUM_STRIPS];
    while (1)
    {
        int idx;
        xQueueReceive(readyFrames, &idx, portMAX_DELAY);
        displayFrame_t *f = &frames[idx];
        int rows = f->rows;
        bool overlay = touchOverlayVisible();
        panelSetWindow(rows);
        int queued = 0;
        int strip = 0;
        for (int y = 0; y < rows; y += STRIP_LINES)
        {
            int lines = rows - y < STRIP_LINES ? rows - y : STRIP_LINES;
            if (queued == NUM_STRIPS)
            {
                spi_transaction_t *done;
                spi_device_get_trans_result(panel, &done, portMAX_DELAY);
                queued--;
            }
            uint16_t *dst = strips[strip];
            const uint8_t *src = &f->pixels[y * SCREEN_WIDTH];
            const uint16_t *pal = f->palette;
            for (int i = 0; i < lines * SCREEN_WIDTH; i += 4)
            {
                dst[i] = pal[src[i]];
                dst[i + 1] = pal[src[i + 1]];
                dst[i + 2] = pal[src[i + 2]];
                dst[i + 3] = pal[src[i + 3]];
            }
            if (overlay)
            {
                for (int l = 0; l < lines; l++)
                {
                    touchDrawOverlay(&dst[l * SCREEN_WIDTH], y + l);
                }
            }
            panelQueueStrip(&trans[strip], dst, lines * SCREEN_WIDTH, y == 0, y + lines >= rows);
            queued++;
            strip = (strip + 1) % NUM_STRIPS;
        }
        while (queued--)
        {
            spi_transaction_t *done;
            spi_device_get_trans_result(panel, &done, portMAX_DELAY);
        }
#if IS_QSPI
        spi_device_release_bus(panel);
#endif
        __atomic_sub_fetch(&framesInFlight, 1, __ATOMIC_SEQ_CST);
        xQueueSend(freeFrames, &idx, portMAX_DELAY);
    }
}

displayFrame_t *displayAcquireFrame(void)
{
    int idx;
    xQueueReceive(freeFrames, &idx, portMAX_DELAY);
    return &frames[idx];
}

void displaySubmitFrame(displayFrame_t *frame)
{
    int idx = frame - frames;
    __atomic_add_fetch(&framesInFlight, 1, __ATOMIC_SEQ_CST);
    xQueueSend(readyFrames, &idx, portMAX_DELAY);
}

bool displayBusy(void)
{
    return __atomic_load_n(&framesInFlight, __ATOMIC_SEQ_CST) != 0;
}

void displaySetBacklight(uint8_t level)
{
    boardBacklight(level != 0);
}

void displayInit(void)
{
    spi_bus_config_t bus =
    {
        .mosi_io_num = PIN_SPI_MOSI,
        .sclk_io_num = PIN_SPI_SCK,
        .max_transfer_sz = STRIP_PIXELS * sizeof(uint16_t),
#if IS_QSPI
        .miso_io_num = PIN_QSPI_IO1,
        .quadwp_io_num = PIN_QSPI_IO2,
        .quadhd_io_num = PIN_QSPI_IO3,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_QUAD,
#else
        .miso_io_num = PIN_SPI_MISO,        // for the SD card on the same bus
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
#endif
    };
    ESP_ERROR_CHECK(spi_bus_initialize(DISPLAY_SPI_HOST, &bus, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t dev =
    {
        .clock_speed_hz = DISPLAY_SPI_CLOCK_HZ,
        .spics_io_num = PIN_TFT_CS,
        .queue_size = NUM_STRIPS,
        .flags = SPI_DEVICE_HALFDUPLEX,
#if IS_QSPI
        .mode = 3,
        .command_bits = 8,
        .address_bits = 24,
#else
        .mode = 0,
        .pre_cb = panelPreTransfer,
#endif
    };
#if !IS_QSPI
    gpio_reset_pin(PIN_TFT_DC);
    gpio_set_direction(PIN_TFT_DC, GPIO_MODE_OUTPUT);
#endif
    ESP_ERROR_CHECK(spi_bus_add_device(DISPLAY_SPI_HOST, &dev, &panel));
    boardBacklight(false);
    //
#if IS_QSPI
    panelCommand(0x00, NULL, 0);    // NOP frame to flush the QSPI interface
#endif
    panelCommands(initCommands);
    //
    for (int i = 0; i < NUM_STRIPS; i++)
    {
        strips[i] = heap_caps_malloc(STRIP_PIXELS * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        assert(strips[i]);
    }
    readyFrames = xQueueCreate(DISPLAY_NUM_FRAMES, sizeof(int));
    freeFrames = xQueueCreate(DISPLAY_NUM_FRAMES, sizeof(int));
    for (int i = 0; i < DISPLAY_NUM_FRAMES; i++)
    {
        // Snapshots live in PSRAM: they are read once per frame, sequentially.
        frames[i].pixels = heap_caps_malloc(SCREEN_WIDTH * SCREEN_HEIGHT, MALLOC_CAP_SPIRAM);
        assert(frames[i].pixels);
        xQueueSend(freeFrames, &i, 0);
    }
    xTaskCreatePinnedToCore(displayTask, "display", 4096, NULL, 10, NULL, 0);
    // Black frame before the backlight comes on.
    displayFrame_t *f = displayAcquireFrame();
    memset(f->pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
    memset(f->palette, 0, sizeof(f->palette));
    f->rows = SCREEN_HEIGHT;
    displaySubmitFrame(f);
    while (displayBusy())
    {
        vTaskDelay(1);
    }
#if IS_QSPI
    const uint8_t dummy = 0;        // NV303x needs a parameter
    panelCommand(CMD_DISPON, &dummy, 1);
#else
    panelCommand(CMD_DISPON, NULL, 0);
#endif
    vTaskDelay(pdMS_TO_TICKS(10));
    boardBacklight(true);
    ESP_LOGI(TAG, "%s panel up at %d MHz", IS_QSPI ? "NV3031B QSPI" : "ST7789 SPI", DISPLAY_SPI_CLOCK_HZ / 1000000);
}
