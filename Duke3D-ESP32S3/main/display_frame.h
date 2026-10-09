/*
 *  Frame hand-off between the game (graphics.c) and the panel task (display.c).
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef DISPLAY_FRAME_H
#define DISPLAY_FRAME_H
#include <stdint.h>
#include <stdbool.h>
#define DISPLAY_NUM_FRAMES    2
typedef struct
{
    uint8_t *pixels;            // SCREEN_WIDTH x rows, 8-bit indexed
    uint16_t palette[256];      // byte-swapped RGB565, ready for the panel
    int rows;
} displayFrame_t;
// Blocks until a snapshot buffer is free (i.e. at most one frame queued ahead).
displayFrame_t *displayAcquireFrame(void);
void displaySubmitFrame(displayFrame_t *frame);
bool displayBusy(void);
#endif
