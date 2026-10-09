/*
 *  Frame buffer, frame submission and the 8x8 boot console (ESP32-S3 port).
 *
 *  Adapted from the MG24 port's graphics.c: the text console is unchanged,
 *  while the line-by-line DMA engine is replaced by a frame snapshot handed
 *  to the panel task in display.c.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "graphics.h"
#include "font8x8.h"
#include "display_frame.h"

#define LE2BE16(x) ((uint16_t)((((x) >> 8) & 0xFF) | ((x) << 8)))
#define RGB(r, g, b) ((((r) >> 3) << (6 + 5)) | (((g) >> 2) << 5) | (((b) >> 3) << (0)))
#define MAX_STRING_SIZE (SCREEN_WIDTH / FONT_WIDTH + 1)

displayData_t displayData;

static const uint16_t consolePalette[] =
{
    LE2BE16(RGB(170, 170, 170)),
    LE2BE16(RGB(0, 0, 0)),
    LE2BE16(RGB(255, 255, 255)),
    LE2BE16(RGB(86, 119, 170)),
};
static uint16_t consolePalette256[256];

static uint8_t penColor = 1;
static uint8_t penBackground = 0;
static uint8_t line = 0;

void initGraphics()
{
    memset(&displayData, 0, sizeof(displayData));
    memcpy(consolePalette256, consolePalette, sizeof(consolePalette));
    displayData.pPalette = consolePalette256;
    displayData.displayDmaLineBuffersSent = 0xFFFF;
    setDisplayPen(1, 0);
    memset(displayData.displayFrameBuffer[0], 0, sizeof(displayData.displayFrameBuffer[0]));
}

void startDisplayRefresh(uint8_t bufferNumber, unsigned int rowsToUpdate)
{
    UNUSED(bufferNumber);
    if (rowsToUpdate == 0 || rowsToUpdate > SCREEN_HEIGHT)
    {
        rowsToUpdate = SCREEN_HEIGHT;
    }
    displayFrame_t *f = displayAcquireFrame();
    memcpy(f->pixels, displayData.displayFrameBuffer[0], rowsToUpdate * SCREEN_WIDTH);
    memcpy(f->palette, (const void*) displayData.pPalette, sizeof(f->palette));
    f->rows = rowsToUpdate;
    displayData.rowsToUpdate = rowsToUpdate;
    displaySubmitFrame(f);
    // The snapshot is taken: the engine may overwrite every line right away.
    displayData.displayDmaLineBuffersSent = 0xFFFF;
}

void waitDisplaySnapshot(void)
{
    // startDisplayRefresh() copies synchronously, so there is never a wait.
}

static void waitDisplayIdle(void)
{
    while (displayBusy())
    {
    }
}

void * getDisplayFrameBuffer(int num)
{
    UNUSED(num);
    return displayData.displayFrameBuffer[0];
}

static void setPixel(unsigned int x, unsigned int y, int c)
{
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)
    {
        return;
    }
    displayData.displayFrameBuffer[0][y * SCREEN_WIDTH + x] = c;
}

static void displayPutChar(char c, int x, int y)
{
    for (int cy = 0; cy < FONT_HEIGHT; cy++)
    {
        uint8_t fb = font8x8_basic[0x7F & c][cy];
        for (int cx = 0; cx < FONT_WIDTH; cx++)
        {
            setPixel(x + cx, y + cy, (fb & 1) ? penColor : penBackground);
            fb >>= 1;
        }
    }
}

void setDisplayPen(int color, int background)
{
    penColor = color;
    penBackground = background;
}

void clearScreen4bpp()
{
    memset(displayData.displayFrameBuffer[0], penBackground, sizeof(displayData.displayFrameBuffer[0]));
    line = 0;
}

static void displayVPrintf(int x, int y, const char *format, va_list va)
{
    char outString[MAX_STRING_SIZE];
    vsnprintf(outString, MAX_STRING_SIZE, format, va);
    for (int i = 0; i < MAX_STRING_SIZE && x < SCREEN_WIDTH && outString[i] <= 0x7F && outString[i] > 0; i++)
    {
        displayPutChar(outString[i], x, y);
        x += FONT_WIDTH;
    }
}

void displayPrintf(int x, int y, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    displayVPrintf(x, y, format, va);
    va_end(va);
}

void displayPrintln(bool update, const char *format, ...)
{
    int y;
    if (line < SCREEN_HEIGHT / FONT_HEIGHT)
    {
        y = line * FONT_HEIGHT;
        line++;
    }
    else
    {
        // At the bottom: scroll everything up one text row.
        y = SCREEN_HEIGHT - FONT_HEIGHT;
        uint8_t *fb = displayData.displayFrameBuffer[0];
        memmove(fb, fb + SCREEN_WIDTH * FONT_HEIGHT, SCREEN_WIDTH * (SCREEN_HEIGHT - FONT_HEIGHT));
        memset(fb + SCREEN_WIDTH * (SCREEN_HEIGHT - FONT_HEIGHT), penBackground, SCREEN_WIDTH * FONT_HEIGHT);
    }
    va_list va;
    va_start(va, format);
    // Mirror the boot console on the serial port.
    va_list vaCopy;
    va_copy(vaCopy, va);
    vprintf(format, vaCopy);
    printf("\r\n");
    va_end(vaCopy);
    displayVPrintf(0, y, format, va);
    va_end(va);
    if (update)
    {
        startDisplayRefresh(0, SCREEN_HEIGHT);
        waitDisplayIdle();
    }
}
