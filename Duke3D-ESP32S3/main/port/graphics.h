/*
 *  Frame buffer and boot-console definitions (ESP32-S3 port).
 *
 *  The layout of displayData_t matches the MG24 port: game code borrows the
 *  frame buffer (tempData) as scratch memory during level load.
 *
 *  On the MG24 the engine raced the display DMA line by line. Here
 *  startDisplayRefresh() snapshots the 8-bit frame into a second buffer that a
 *  task on the other core converts and pushes over SPI, so the engine can start
 *  drawing the next frame at once and the line waits are no-ops.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef SRC_GRAPHICS_H_
#define SRC_GRAPHICS_H_
#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include "port/display.h"
#define PIXELS_PER_DMA_LINE         256
#define NUMBER_OF_DMA_LINES         (SCREEN_WIDTH * SCREEN_HEIGHT / 256)
#define NUM_VIDEO_BUFFERS           1
typedef struct
{
    union
    {
        struct
        {
            uint8_t displayFrameBuffer[NUM_VIDEO_BUFFERS][SCREEN_WIDTH * DRAW_BUFFER_HEIGHT];
            uint16_t displayDmaLineBuffer[2][PIXELS_PER_DMA_LINE];
        };
        uint8_t tempData[SCREEN_WIDTH * DRAW_BUFFER_HEIGHT + 2 * 2 * PIXELS_PER_DMA_LINE];
    };
    volatile uint16_t * pPalette;
    uint8_t * currentDmaFrameBuffer;
    volatile uint16_t displayDmaLineBuffersSent;
    volatile uint8_t currentDisplayDmaLineBuffer;
    uint8_t workingBuffer;
    uint8_t displayMode;
    uint8_t rowsToUpdate;
    volatile uint8_t dmaBusy;
} displayData_t;
extern displayData_t displayData;
static inline int getFrameBufferSize(void)
{
    return SCREEN_WIDTH * DRAW_BUFFER_HEIGHT;
}
void displayPrintf(int x, int y, const char * format, ...);
void startDisplayRefresh(uint8_t bufferNumber, unsigned int rowsToUpdate);
void setDisplayPen(int color, int background);
void clearScreen4bpp();
void displayPrintln(bool update, const char * format, ...);
void initGraphics();
void * getDisplayFrameBuffer(int num);
// Waits until the frame previously handed to startDisplayRefresh() has been
// copied out, after which the whole draw buffer may be overwritten.
void waitDisplaySnapshot(void);
static inline void waitForDisplayDMA(unsigned int y)
{
    (void) y;
    waitDisplaySnapshot();
}
extern uint8_t lineYoffs;
static inline void waitRenderLine(int y)
{
    (void) y;
}
#endif /* SRC_GRAPHICS_H_ */
