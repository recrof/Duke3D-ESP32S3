/*
 *  On-device GRP conversion (see grpconvert.c).
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef GRPCONVERT_H
#define GRPCONVERT_H
#include <stdbool.h>
#include <stddef.h>
// Picks the GRP to play from the mounted FatFs drive ("0:"), converting
// DUKE3D.GRP for the MCU engine first if needed. Returns false if no GRP exists.
bool prepareGrp(const char *drive, char *grpPath, size_t grpPathLen);
#endif
