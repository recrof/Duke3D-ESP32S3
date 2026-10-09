/**
 *  Display utility functions for Doom Port to EFR32xG2x MCUs.
 *  It also contains the interrupt-assisted DMA update of the display
 *  and double buffer display data structure.
 *
 *  Copyright (C) 2021-2023 by Nicola Wrachien (next-hack in the comments)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 *  DESCRIPTION:
 *  Interrupt assisted DMA-based display update engine, with double buffering
 *  256-color functions to print general purpose texts.
 *
 */
#ifndef SRC_GRAPHICS_H_
#define SRC_GRAPHICS_H_
#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include "port/display.h"
#define PIXELS_PER_DMA_LINE         256
#define NUMBER_OF_DMA_LINES         (SCREEN_WIDTH * SCREEN_HEIGHT / 256)
#define NUM_VIDEO_BUFFERS            1
typedef struct
{
    union
    {
        struct
        {
            uint8_t displayFrameBuffer[NUM_VIDEO_BUFFERS][SCREEN_WIDTH * DRAW_BUFFER_HEIGHT];
            uint16_t displayDmaLineBuffer[2][PIXELS_PER_DMA_LINE];  // small buffer to speed up DMA send
        };
        uint8_t tempData[SCREEN_WIDTH * DRAW_BUFFER_HEIGHT + 2 * 2 * PIXELS_PER_DMA_LINE];   // next-hack: yes, we can use this large struct as buffer is display is not used
    };
    volatile uint16_t * pPalette;
    uint8_t * currentDmaFrameBuffer;            // points to data to be converted, using pPalette on the displayDmaBuffer
    volatile uint16_t displayDmaLineBuffersSent; // how many lines we sent
    volatile uint8_t currentDisplayDmaLineBuffer;
    uint8_t workingBuffer;
    uint8_t displayMode;
    uint8_t rowsToUpdate;
    volatile uint8_t dmaBusy;
#if NEW_DISPLAY_UPDATE_WAY
    volatile uint8_t doNotDisableShorts;
    volatile uint8_t nextBufferIndex;
#endif
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
static inline void waitForDisplayDMA(unsigned int y)
{
    // by setting TEST_IMPROVEMENT to 1 we will wait always for the full
    // 3D scene to be sent to the display (i.e. at least 200 pix)
    // In this way, one can understand how much we improve by waiting
    // just for a specified line y, instead of the whole 3D scene.
    //
#if TEST_IMPROVEMENT
    if (y < 200)
        y = 199;
#endif
  // wait
  while ((y + 1) * 320 >= displayData.displayDmaLineBuffersSent * PIXELS_PER_DMA_LINE );
}
extern uint8_t lineYoffs;
// this will wait until the y-th line is rendered.
// Multiplayer will shift everything down, so this is taking into account of this.
static inline void waitRenderLine(int y)
{
#if !WIN32
  y += lineYoffs;
  if (y >= SCREEN_HEIGHT)
      y = SCREEN_HEIGHT - 1;
  waitForDisplayDMA(y);
 #endif
}

#endif /* SRC_GRAPHICS_H_ */
