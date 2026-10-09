/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/**********************************************************************
   module: MULTIVOC.C

   author: James R. Dose
   date:   December 20, 1993

   Routines to provide multichannel digitized sound playback for
   Sound Blaster compatible sound cards.

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifdef WIN32
#include <stdio.h>
#include "macros.h"
#else
#include <printf.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "dpmi.h"
#include "usrhooks.h"
#include "linklist.h"
#include "sndcards.h"
#include "dsl.h"
#include "sounds.h"
#include "pitch.h"
#include "multivoc.h"
#include "_multivc.h"
#include "music.h"
#include "../sounddebugdefs.h"
#if !WIN32
#include "audio.h"
#include "midi/nh_midi.h"
#endif
#ifndef max
#define max(x, y)  (((x) > (y)) ? (x) : (y))
#endif

#ifndef min
#define min(x, y)  (((x) < (y)) ? (x) : (y))
#endif
#define RoundFixed( fixedval, bits )            \
        (                                       \
          (                                     \
            (fixedval) + ( 1 << ( (bits) - 1 ) )\
          ) >> (bits)                           \
        )

// #define IS_QUIET( ptr )  ( ( void * )( ptr ) == ( void * )&MV_VolumeTable[ 0 ] )
#define IS_QUIET( vol ) ( ( vol ) == 0 )

#if !WIN32
void MV_ReverbClear(void);
#endif

playbackstatus MV_GetNextWAVBlock(VoiceNode *voice);
// fixme: remove all the crap related to old reverb

static int       MV_ReverbLevel;
int       MV_ReverbDelay;
static int       MV_ReverbTable = -1;

//static signed short MV_VolumeTable[ MV_MaxVolume + 1 ][ 256 ];
//static signed short MV_VolumeTable[ 63 + 1 ][ 256 ];
static uint8_t soundNum[NUM_SOUNDS / 2];
int getSoundNum(int sound)
{
    if (sound & 1)
    {
        return soundNum[sound >> 1] >> 4;
    }
    else
    {
        return soundNum[sound >> 1] & 0xF;
    }
}
void setSoundNum(int sound, unsigned int num)
{
    if (sound & 1)
    {
        soundNum[sound >> 1] &= 0x0F;
        soundNum[sound >> 1] |= num << 4;
    }
    else
    {
        soundNum[sound >> 1] &=  0xF0;
        soundNum[sound >> 1] |= num & 0x0F;
    }
}
//static Pan MV_PanTable[ MV_NumPanPositions ][ MV_MaxVolume + 1 ];
#define RUNTIME_PANTABLE 0
#if RUNTIME_PANTABLE
static Pan MV_PanTable[ MV_NumPanPositions ][ 63 + 1 ];
#else
const Pan MV_PanTable[ MV_NumPanPositions + 1 ][ 63 + 1 ] =
{
    {
        {.left = 0xff, .right = 0xff},
        {.left = 0xfa, .right = 0xfa},
        {.left = 0xf6, .right = 0xf6},
        {.left = 0xf2, .right = 0xf2},
        {.left = 0xee, .right = 0xee},
        {.left = 0xea, .right = 0xea},
        {.left = 0xe6, .right = 0xe6},
        {.left = 0xe2, .right = 0xe2},
        {.left = 0xde, .right = 0xde},
        {.left = 0xda, .right = 0xda},
        {.left = 0xd6, .right = 0xd6},
        {.left = 0xd2, .right = 0xd2},
        {.left = 0xce, .right = 0xce},
        {.left = 0xca, .right = 0xca},
        {.left = 0xc6, .right = 0xc6},
        {.left = 0xc2, .right = 0xc2},
        {.left = 0xbe, .right = 0xbe},
        {.left = 0xba, .right = 0xba},
        {.left = 0xb6, .right = 0xb6},
        {.left = 0xb2, .right = 0xb2},
        {.left = 0xae, .right = 0xae},
        {.left = 0xaa, .right = 0xaa},
        {.left = 0xa5, .right = 0xa5},
        {.left = 0xa1, .right = 0xa1},
        {.left = 0x9d, .right = 0x9d},
        {.left = 0x99, .right = 0x99},
        {.left = 0x95, .right = 0x95},
        {.left = 0x91, .right = 0x91},
        {.left = 0x8d, .right = 0x8d},
        {.left = 0x89, .right = 0x89},
        {.left = 0x85, .right = 0x85},
        {.left = 0x81, .right = 0x81},
        {.left = 0x7d, .right = 0x7d},
        {.left = 0x79, .right = 0x79},
        {.left = 0x75, .right = 0x75},
        {.left = 0x71, .right = 0x71},
        {.left = 0x6d, .right = 0x6d},
        {.left = 0x69, .right = 0x69},
        {.left = 0x65, .right = 0x65},
        {.left = 0x61, .right = 0x61},
        {.left = 0x5d, .right = 0x5d},
        {.left = 0x59, .right = 0x59},
        {.left = 0x55, .right = 0x55},
        {.left = 0x50, .right = 0x50},
        {.left = 0x4c, .right = 0x4c},
        {.left = 0x48, .right = 0x48},
        {.left = 0x44, .right = 0x44},
        {.left = 0x40, .right = 0x40},
        {.left = 0x3c, .right = 0x3c},
        {.left = 0x38, .right = 0x38},
        {.left = 0x34, .right = 0x34},
        {.left = 0x30, .right = 0x30},
        {.left = 0x2c, .right = 0x2c},
        {.left = 0x28, .right = 0x28},
        {.left = 0x24, .right = 0x24},
        {.left = 0x20, .right = 0x20},
        {.left = 0x1c, .right = 0x1c},
        {.left = 0x18, .right = 0x18},
        {.left = 0x14, .right = 0x14},
        {.left = 0x10, .right = 0x10},
        {.left = 0x0c, .right = 0x0c},
        {.left = 0x08, .right = 0x08},
        {.left = 0x04, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xe0, .right = 0xff},
        {.left = 0xdb, .right = 0xfa},
        {.left = 0xd8, .right = 0xf6},
        {.left = 0xd4, .right = 0xf2},
        {.left = 0xd1, .right = 0xee},
        {.left = 0xcd, .right = 0xea},
        {.left = 0xca, .right = 0xe6},
        {.left = 0xc6, .right = 0xe2},
        {.left = 0xc3, .right = 0xde},
        {.left = 0xbf, .right = 0xda},
        {.left = 0xbc, .right = 0xd6},
        {.left = 0xb8, .right = 0xd2},
        {.left = 0xb5, .right = 0xce},
        {.left = 0xb1, .right = 0xca},
        {.left = 0xae, .right = 0xc6},
        {.left = 0xaa, .right = 0xc2},
        {.left = 0xa7, .right = 0xbe},
        {.left = 0xa3, .right = 0xba},
        {.left = 0xa0, .right = 0xb6},
        {.left = 0x9c, .right = 0xb2},
        {.left = 0x99, .right = 0xae},
        {.left = 0x95, .right = 0xaa},
        {.left = 0x91, .right = 0xa5},
        {.left = 0x8d, .right = 0xa1},
        {.left = 0x8a, .right = 0x9d},
        {.left = 0x86, .right = 0x99},
        {.left = 0x83, .right = 0x95},
        {.left = 0x7f, .right = 0x91},
        {.left = 0x7c, .right = 0x8d},
        {.left = 0x78, .right = 0x89},
        {.left = 0x75, .right = 0x85},
        {.left = 0x71, .right = 0x81},
        {.left = 0x6e, .right = 0x7d},
        {.left = 0x6a, .right = 0x79},
        {.left = 0x67, .right = 0x75},
        {.left = 0x63, .right = 0x71},
        {.left = 0x60, .right = 0x6d},
        {.left = 0x5c, .right = 0x69},
        {.left = 0x59, .right = 0x65},
        {.left = 0x55, .right = 0x61},
        {.left = 0x52, .right = 0x5d},
        {.left = 0x4e, .right = 0x59},
        {.left = 0x4b, .right = 0x55},
        {.left = 0x46, .right = 0x50},
        {.left = 0x43, .right = 0x4c},
        {.left = 0x3f, .right = 0x48},
        {.left = 0x3c, .right = 0x44},
        {.left = 0x38, .right = 0x40},
        {.left = 0x35, .right = 0x3c},
        {.left = 0x31, .right = 0x38},
        {.left = 0x2e, .right = 0x34},
        {.left = 0x2a, .right = 0x30},
        {.left = 0x27, .right = 0x2c},
        {.left = 0x23, .right = 0x28},
        {.left = 0x20, .right = 0x24},
        {.left = 0x1c, .right = 0x20},
        {.left = 0x19, .right = 0x1c},
        {.left = 0x15, .right = 0x18},
        {.left = 0x12, .right = 0x14},
        {.left = 0x0e, .right = 0x10},
        {.left = 0x0b, .right = 0x0c},
        {.left = 0x07, .right = 0x08},
        {.left = 0x04, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xc0, .right = 0xff},
        {.left = 0xbc, .right = 0xfa},
        {.left = 0xb9, .right = 0xf6},
        {.left = 0xb6, .right = 0xf2},
        {.left = 0xb3, .right = 0xee},
        {.left = 0xb0, .right = 0xea},
        {.left = 0xad, .right = 0xe6},
        {.left = 0xaa, .right = 0xe2},
        {.left = 0xa7, .right = 0xde},
        {.left = 0xa4, .right = 0xda},
        {.left = 0xa1, .right = 0xd6},
        {.left = 0x9e, .right = 0xd2},
        {.left = 0x9b, .right = 0xce},
        {.left = 0x98, .right = 0xca},
        {.left = 0x95, .right = 0xc6},
        {.left = 0x92, .right = 0xc2},
        {.left = 0x8f, .right = 0xbe},
        {.left = 0x8c, .right = 0xba},
        {.left = 0x89, .right = 0xb6},
        {.left = 0x86, .right = 0xb2},
        {.left = 0x83, .right = 0xae},
        {.left = 0x80, .right = 0xaa},
        {.left = 0x7c, .right = 0xa5},
        {.left = 0x79, .right = 0xa1},
        {.left = 0x76, .right = 0x9d},
        {.left = 0x73, .right = 0x99},
        {.left = 0x70, .right = 0x95},
        {.left = 0x6d, .right = 0x91},
        {.left = 0x6a, .right = 0x8d},
        {.left = 0x67, .right = 0x89},
        {.left = 0x64, .right = 0x85},
        {.left = 0x61, .right = 0x81},
        {.left = 0x5e, .right = 0x7d},
        {.left = 0x5b, .right = 0x79},
        {.left = 0x58, .right = 0x75},
        {.left = 0x55, .right = 0x71},
        {.left = 0x52, .right = 0x6d},
        {.left = 0x4f, .right = 0x69},
        {.left = 0x4c, .right = 0x65},
        {.left = 0x49, .right = 0x61},
        {.left = 0x46, .right = 0x5d},
        {.left = 0x43, .right = 0x59},
        {.left = 0x40, .right = 0x55},
        {.left = 0x3c, .right = 0x50},
        {.left = 0x39, .right = 0x4c},
        {.left = 0x36, .right = 0x48},
        {.left = 0x33, .right = 0x44},
        {.left = 0x30, .right = 0x40},
        {.left = 0x2d, .right = 0x3c},
        {.left = 0x2a, .right = 0x38},
        {.left = 0x27, .right = 0x34},
        {.left = 0x24, .right = 0x30},
        {.left = 0x21, .right = 0x2c},
        {.left = 0x1e, .right = 0x28},
        {.left = 0x1b, .right = 0x24},
        {.left = 0x18, .right = 0x20},
        {.left = 0x15, .right = 0x1c},
        {.left = 0x12, .right = 0x18},
        {.left = 0x0f, .right = 0x14},
        {.left = 0x0c, .right = 0x10},
        {.left = 0x09, .right = 0x0c},
        {.left = 0x06, .right = 0x08},
        {.left = 0x03, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xa0, .right = 0xff},
        {.left = 0x9d, .right = 0xfa},
        {.left = 0x9a, .right = 0xf6},
        {.left = 0x98, .right = 0xf2},
        {.left = 0x95, .right = 0xee},
        {.left = 0x93, .right = 0xea},
        {.left = 0x90, .right = 0xe6},
        {.left = 0x8e, .right = 0xe2},
        {.left = 0x8b, .right = 0xde},
        {.left = 0x89, .right = 0xda},
        {.left = 0x86, .right = 0xd6},
        {.left = 0x84, .right = 0xd2},
        {.left = 0x81, .right = 0xce},
        {.left = 0x7f, .right = 0xca},
        {.left = 0x7c, .right = 0xc6},
        {.left = 0x7a, .right = 0xc2},
        {.left = 0x77, .right = 0xbe},
        {.left = 0x75, .right = 0xba},
        {.left = 0x72, .right = 0xb6},
        {.left = 0x70, .right = 0xb2},
        {.left = 0x6d, .right = 0xae},
        {.left = 0x6b, .right = 0xaa},
        {.left = 0x68, .right = 0xa5},
        {.left = 0x65, .right = 0xa1},
        {.left = 0x63, .right = 0x9d},
        {.left = 0x60, .right = 0x99},
        {.left = 0x5e, .right = 0x95},
        {.left = 0x5b, .right = 0x91},
        {.left = 0x59, .right = 0x8d},
        {.left = 0x56, .right = 0x89},
        {.left = 0x54, .right = 0x85},
        {.left = 0x51, .right = 0x81},
        {.left = 0x4f, .right = 0x7d},
        {.left = 0x4c, .right = 0x79},
        {.left = 0x4a, .right = 0x75},
        {.left = 0x47, .right = 0x71},
        {.left = 0x45, .right = 0x6d},
        {.left = 0x42, .right = 0x69},
        {.left = 0x40, .right = 0x65},
        {.left = 0x3d, .right = 0x61},
        {.left = 0x3b, .right = 0x5d},
        {.left = 0x38, .right = 0x59},
        {.left = 0x36, .right = 0x55},
        {.left = 0x32, .right = 0x50},
        {.left = 0x30, .right = 0x4c},
        {.left = 0x2d, .right = 0x48},
        {.left = 0x2b, .right = 0x44},
        {.left = 0x28, .right = 0x40},
        {.left = 0x26, .right = 0x3c},
        {.left = 0x23, .right = 0x38},
        {.left = 0x21, .right = 0x34},
        {.left = 0x1e, .right = 0x30},
        {.left = 0x1c, .right = 0x2c},
        {.left = 0x19, .right = 0x28},
        {.left = 0x17, .right = 0x24},
        {.left = 0x14, .right = 0x20},
        {.left = 0x12, .right = 0x1c},
        {.left = 0x0f, .right = 0x18},
        {.left = 0x0d, .right = 0x14},
        {.left = 0x0a, .right = 0x10},
        {.left = 0x08, .right = 0x0c},
        {.left = 0x05, .right = 0x08},
        {.left = 0x03, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0x80, .right = 0xff},
        {.left = 0x7d, .right = 0xfa},
        {.left = 0x7b, .right = 0xf6},
        {.left = 0x79, .right = 0xf2},
        {.left = 0x77, .right = 0xee},
        {.left = 0x75, .right = 0xea},
        {.left = 0x73, .right = 0xe6},
        {.left = 0x71, .right = 0xe2},
        {.left = 0x6f, .right = 0xde},
        {.left = 0x6d, .right = 0xda},
        {.left = 0x6b, .right = 0xd6},
        {.left = 0x69, .right = 0xd2},
        {.left = 0x67, .right = 0xce},
        {.left = 0x65, .right = 0xca},
        {.left = 0x63, .right = 0xc6},
        {.left = 0x61, .right = 0xc2},
        {.left = 0x5f, .right = 0xbe},
        {.left = 0x5d, .right = 0xba},
        {.left = 0x5b, .right = 0xb6},
        {.left = 0x59, .right = 0xb2},
        {.left = 0x57, .right = 0xae},
        {.left = 0x55, .right = 0xaa},
        {.left = 0x53, .right = 0xa5},
        {.left = 0x51, .right = 0xa1},
        {.left = 0x4f, .right = 0x9d},
        {.left = 0x4d, .right = 0x99},
        {.left = 0x4b, .right = 0x95},
        {.left = 0x49, .right = 0x91},
        {.left = 0x47, .right = 0x8d},
        {.left = 0x45, .right = 0x89},
        {.left = 0x43, .right = 0x85},
        {.left = 0x41, .right = 0x81},
        {.left = 0x3f, .right = 0x7d},
        {.left = 0x3d, .right = 0x79},
        {.left = 0x3b, .right = 0x75},
        {.left = 0x39, .right = 0x71},
        {.left = 0x37, .right = 0x6d},
        {.left = 0x35, .right = 0x69},
        {.left = 0x33, .right = 0x65},
        {.left = 0x31, .right = 0x61},
        {.left = 0x2f, .right = 0x5d},
        {.left = 0x2d, .right = 0x59},
        {.left = 0x2b, .right = 0x55},
        {.left = 0x28, .right = 0x50},
        {.left = 0x26, .right = 0x4c},
        {.left = 0x24, .right = 0x48},
        {.left = 0x22, .right = 0x44},
        {.left = 0x20, .right = 0x40},
        {.left = 0x1e, .right = 0x3c},
        {.left = 0x1c, .right = 0x38},
        {.left = 0x1a, .right = 0x34},
        {.left = 0x18, .right = 0x30},
        {.left = 0x16, .right = 0x2c},
        {.left = 0x14, .right = 0x28},
        {.left = 0x12, .right = 0x24},
        {.left = 0x10, .right = 0x20},
        {.left = 0x0e, .right = 0x1c},
        {.left = 0x0c, .right = 0x18},
        {.left = 0x0a, .right = 0x14},
        {.left = 0x08, .right = 0x10},
        {.left = 0x06, .right = 0x0c},
        {.left = 0x04, .right = 0x08},
        {.left = 0x02, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0x60, .right = 0xff},
        {.left = 0x5e, .right = 0xfa},
        {.left = 0x5d, .right = 0xf6},
        {.left = 0x5b, .right = 0xf2},
        {.left = 0x5a, .right = 0xee},
        {.left = 0x58, .right = 0xea},
        {.left = 0x57, .right = 0xe6},
        {.left = 0x55, .right = 0xe2},
        {.left = 0x54, .right = 0xde},
        {.left = 0x52, .right = 0xda},
        {.left = 0x51, .right = 0xd6},
        {.left = 0x4f, .right = 0xd2},
        {.left = 0x4e, .right = 0xce},
        {.left = 0x4c, .right = 0xca},
        {.left = 0x4b, .right = 0xc6},
        {.left = 0x49, .right = 0xc2},
        {.left = 0x48, .right = 0xbe},
        {.left = 0x46, .right = 0xba},
        {.left = 0x45, .right = 0xb6},
        {.left = 0x43, .right = 0xb2},
        {.left = 0x42, .right = 0xae},
        {.left = 0x40, .right = 0xaa},
        {.left = 0x3e, .right = 0xa5},
        {.left = 0x3d, .right = 0xa1},
        {.left = 0x3b, .right = 0x9d},
        {.left = 0x3a, .right = 0x99},
        {.left = 0x38, .right = 0x95},
        {.left = 0x37, .right = 0x91},
        {.left = 0x35, .right = 0x8d},
        {.left = 0x34, .right = 0x89},
        {.left = 0x32, .right = 0x85},
        {.left = 0x31, .right = 0x81},
        {.left = 0x2f, .right = 0x7d},
        {.left = 0x2e, .right = 0x79},
        {.left = 0x2c, .right = 0x75},
        {.left = 0x2b, .right = 0x71},
        {.left = 0x29, .right = 0x6d},
        {.left = 0x28, .right = 0x69},
        {.left = 0x26, .right = 0x65},
        {.left = 0x25, .right = 0x61},
        {.left = 0x23, .right = 0x5d},
        {.left = 0x22, .right = 0x59},
        {.left = 0x20, .right = 0x55},
        {.left = 0x1e, .right = 0x50},
        {.left = 0x1d, .right = 0x4c},
        {.left = 0x1b, .right = 0x48},
        {.left = 0x1a, .right = 0x44},
        {.left = 0x18, .right = 0x40},
        {.left = 0x17, .right = 0x3c},
        {.left = 0x15, .right = 0x38},
        {.left = 0x14, .right = 0x34},
        {.left = 0x12, .right = 0x30},
        {.left = 0x11, .right = 0x2c},
        {.left = 0x0f, .right = 0x28},
        {.left = 0x0e, .right = 0x24},
        {.left = 0x0c, .right = 0x20},
        {.left = 0x0b, .right = 0x1c},
        {.left = 0x09, .right = 0x18},
        {.left = 0x08, .right = 0x14},
        {.left = 0x06, .right = 0x10},
        {.left = 0x05, .right = 0x0c},
        {.left = 0x03, .right = 0x08},
        {.left = 0x02, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0x40, .right = 0xff},
        {.left = 0x3f, .right = 0xfa},
        {.left = 0x3e, .right = 0xf6},
        {.left = 0x3d, .right = 0xf2},
        {.left = 0x3c, .right = 0xee},
        {.left = 0x3b, .right = 0xea},
        {.left = 0x3a, .right = 0xe6},
        {.left = 0x39, .right = 0xe2},
        {.left = 0x38, .right = 0xde},
        {.left = 0x37, .right = 0xda},
        {.left = 0x36, .right = 0xd6},
        {.left = 0x35, .right = 0xd2},
        {.left = 0x34, .right = 0xce},
        {.left = 0x33, .right = 0xca},
        {.left = 0x32, .right = 0xc6},
        {.left = 0x31, .right = 0xc2},
        {.left = 0x30, .right = 0xbe},
        {.left = 0x2f, .right = 0xba},
        {.left = 0x2e, .right = 0xb6},
        {.left = 0x2d, .right = 0xb2},
        {.left = 0x2c, .right = 0xae},
        {.left = 0x2b, .right = 0xaa},
        {.left = 0x2a, .right = 0xa5},
        {.left = 0x29, .right = 0xa1},
        {.left = 0x28, .right = 0x9d},
        {.left = 0x27, .right = 0x99},
        {.left = 0x26, .right = 0x95},
        {.left = 0x25, .right = 0x91},
        {.left = 0x24, .right = 0x8d},
        {.left = 0x23, .right = 0x89},
        {.left = 0x22, .right = 0x85},
        {.left = 0x21, .right = 0x81},
        {.left = 0x20, .right = 0x7d},
        {.left = 0x1f, .right = 0x79},
        {.left = 0x1e, .right = 0x75},
        {.left = 0x1d, .right = 0x71},
        {.left = 0x1c, .right = 0x6d},
        {.left = 0x1b, .right = 0x69},
        {.left = 0x1a, .right = 0x65},
        {.left = 0x19, .right = 0x61},
        {.left = 0x18, .right = 0x5d},
        {.left = 0x17, .right = 0x59},
        {.left = 0x16, .right = 0x55},
        {.left = 0x14, .right = 0x50},
        {.left = 0x13, .right = 0x4c},
        {.left = 0x12, .right = 0x48},
        {.left = 0x11, .right = 0x44},
        {.left = 0x10, .right = 0x40},
        {.left = 0x0f, .right = 0x3c},
        {.left = 0x0e, .right = 0x38},
        {.left = 0x0d, .right = 0x34},
        {.left = 0x0c, .right = 0x30},
        {.left = 0x0b, .right = 0x2c},
        {.left = 0x0a, .right = 0x28},
        {.left = 0x09, .right = 0x24},
        {.left = 0x08, .right = 0x20},
        {.left = 0x07, .right = 0x1c},
        {.left = 0x06, .right = 0x18},
        {.left = 0x05, .right = 0x14},
        {.left = 0x04, .right = 0x10},
        {.left = 0x03, .right = 0x0c},
        {.left = 0x02, .right = 0x08},
        {.left = 0x01, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0x20, .right = 0xff},
        {.left = 0x20, .right = 0xfa},
        {.left = 0x1f, .right = 0xf6},
        {.left = 0x1f, .right = 0xf2},
        {.left = 0x1e, .right = 0xee},
        {.left = 0x1e, .right = 0xea},
        {.left = 0x1d, .right = 0xe6},
        {.left = 0x1d, .right = 0xe2},
        {.left = 0x1c, .right = 0xde},
        {.left = 0x1c, .right = 0xda},
        {.left = 0x1b, .right = 0xd6},
        {.left = 0x1b, .right = 0xd2},
        {.left = 0x1a, .right = 0xce},
        {.left = 0x1a, .right = 0xca},
        {.left = 0x19, .right = 0xc6},
        {.left = 0x19, .right = 0xc2},
        {.left = 0x18, .right = 0xbe},
        {.left = 0x18, .right = 0xba},
        {.left = 0x17, .right = 0xb6},
        {.left = 0x17, .right = 0xb2},
        {.left = 0x16, .right = 0xae},
        {.left = 0x16, .right = 0xaa},
        {.left = 0x15, .right = 0xa5},
        {.left = 0x15, .right = 0xa1},
        {.left = 0x14, .right = 0x9d},
        {.left = 0x14, .right = 0x99},
        {.left = 0x13, .right = 0x95},
        {.left = 0x13, .right = 0x91},
        {.left = 0x12, .right = 0x8d},
        {.left = 0x12, .right = 0x89},
        {.left = 0x11, .right = 0x85},
        {.left = 0x11, .right = 0x81},
        {.left = 0x10, .right = 0x7d},
        {.left = 0x10, .right = 0x79},
        {.left = 0x0f, .right = 0x75},
        {.left = 0x0f, .right = 0x71},
        {.left = 0x0e, .right = 0x6d},
        {.left = 0x0e, .right = 0x69},
        {.left = 0x0d, .right = 0x65},
        {.left = 0x0d, .right = 0x61},
        {.left = 0x0c, .right = 0x5d},
        {.left = 0x0c, .right = 0x59},
        {.left = 0x0b, .right = 0x55},
        {.left = 0x0a, .right = 0x50},
        {.left = 0x0a, .right = 0x4c},
        {.left = 0x09, .right = 0x48},
        {.left = 0x09, .right = 0x44},
        {.left = 0x08, .right = 0x40},
        {.left = 0x08, .right = 0x3c},
        {.left = 0x07, .right = 0x38},
        {.left = 0x07, .right = 0x34},
        {.left = 0x06, .right = 0x30},
        {.left = 0x06, .right = 0x2c},
        {.left = 0x05, .right = 0x28},
        {.left = 0x05, .right = 0x24},
        {.left = 0x04, .right = 0x20},
        {.left = 0x04, .right = 0x1c},
        {.left = 0x03, .right = 0x18},
        {.left = 0x03, .right = 0x14},
        {.left = 0x02, .right = 0x10},
        {.left = 0x02, .right = 0x0c},
        {.left = 0x01, .right = 0x08},
        {.left = 0x01, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0x00, .right = 0xff},
        {.left = 0x00, .right = 0xfa},
        {.left = 0x00, .right = 0xf6},
        {.left = 0x00, .right = 0xf2},
        {.left = 0x00, .right = 0xee},
        {.left = 0x00, .right = 0xea},
        {.left = 0x00, .right = 0xe6},
        {.left = 0x00, .right = 0xe2},
        {.left = 0x00, .right = 0xde},
        {.left = 0x00, .right = 0xda},
        {.left = 0x00, .right = 0xd6},
        {.left = 0x00, .right = 0xd2},
        {.left = 0x00, .right = 0xce},
        {.left = 0x00, .right = 0xca},
        {.left = 0x00, .right = 0xc6},
        {.left = 0x00, .right = 0xc2},
        {.left = 0x00, .right = 0xbe},
        {.left = 0x00, .right = 0xba},
        {.left = 0x00, .right = 0xb6},
        {.left = 0x00, .right = 0xb2},
        {.left = 0x00, .right = 0xae},
        {.left = 0x00, .right = 0xaa},
        {.left = 0x00, .right = 0xa5},
        {.left = 0x00, .right = 0xa1},
        {.left = 0x00, .right = 0x9d},
        {.left = 0x00, .right = 0x99},
        {.left = 0x00, .right = 0x95},
        {.left = 0x00, .right = 0x91},
        {.left = 0x00, .right = 0x8d},
        {.left = 0x00, .right = 0x89},
        {.left = 0x00, .right = 0x85},
        {.left = 0x00, .right = 0x81},
        {.left = 0x00, .right = 0x7d},
        {.left = 0x00, .right = 0x79},
        {.left = 0x00, .right = 0x75},
        {.left = 0x00, .right = 0x71},
        {.left = 0x00, .right = 0x6d},
        {.left = 0x00, .right = 0x69},
        {.left = 0x00, .right = 0x65},
        {.left = 0x00, .right = 0x61},
        {.left = 0x00, .right = 0x5d},
        {.left = 0x00, .right = 0x59},
        {.left = 0x00, .right = 0x55},
        {.left = 0x00, .right = 0x50},
        {.left = 0x00, .right = 0x4c},
        {.left = 0x00, .right = 0x48},
        {.left = 0x00, .right = 0x44},
        {.left = 0x00, .right = 0x40},
        {.left = 0x00, .right = 0x3c},
        {.left = 0x00, .right = 0x38},
        {.left = 0x00, .right = 0x34},
        {.left = 0x00, .right = 0x30},
        {.left = 0x00, .right = 0x2c},
        {.left = 0x00, .right = 0x28},
        {.left = 0x00, .right = 0x24},
        {.left = 0x00, .right = 0x20},
        {.left = 0x00, .right = 0x1c},
        {.left = 0x00, .right = 0x18},
        {.left = 0x00, .right = 0x14},
        {.left = 0x00, .right = 0x10},
        {.left = 0x00, .right = 0x0c},
        {.left = 0x00, .right = 0x08},
        {.left = 0x00, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0x20, .right = 0xff},
        {.left = 0x20, .right = 0xfa},
        {.left = 0x1f, .right = 0xf6},
        {.left = 0x1f, .right = 0xf2},
        {.left = 0x1e, .right = 0xee},
        {.left = 0x1e, .right = 0xea},
        {.left = 0x1d, .right = 0xe6},
        {.left = 0x1d, .right = 0xe2},
        {.left = 0x1c, .right = 0xde},
        {.left = 0x1c, .right = 0xda},
        {.left = 0x1b, .right = 0xd6},
        {.left = 0x1b, .right = 0xd2},
        {.left = 0x1a, .right = 0xce},
        {.left = 0x1a, .right = 0xca},
        {.left = 0x19, .right = 0xc6},
        {.left = 0x19, .right = 0xc2},
        {.left = 0x18, .right = 0xbe},
        {.left = 0x18, .right = 0xba},
        {.left = 0x17, .right = 0xb6},
        {.left = 0x17, .right = 0xb2},
        {.left = 0x16, .right = 0xae},
        {.left = 0x16, .right = 0xaa},
        {.left = 0x15, .right = 0xa5},
        {.left = 0x15, .right = 0xa1},
        {.left = 0x14, .right = 0x9d},
        {.left = 0x14, .right = 0x99},
        {.left = 0x13, .right = 0x95},
        {.left = 0x13, .right = 0x91},
        {.left = 0x12, .right = 0x8d},
        {.left = 0x12, .right = 0x89},
        {.left = 0x11, .right = 0x85},
        {.left = 0x11, .right = 0x81},
        {.left = 0x10, .right = 0x7d},
        {.left = 0x10, .right = 0x79},
        {.left = 0x0f, .right = 0x75},
        {.left = 0x0f, .right = 0x71},
        {.left = 0x0e, .right = 0x6d},
        {.left = 0x0e, .right = 0x69},
        {.left = 0x0d, .right = 0x65},
        {.left = 0x0d, .right = 0x61},
        {.left = 0x0c, .right = 0x5d},
        {.left = 0x0c, .right = 0x59},
        {.left = 0x0b, .right = 0x55},
        {.left = 0x0a, .right = 0x50},
        {.left = 0x0a, .right = 0x4c},
        {.left = 0x09, .right = 0x48},
        {.left = 0x09, .right = 0x44},
        {.left = 0x08, .right = 0x40},
        {.left = 0x08, .right = 0x3c},
        {.left = 0x07, .right = 0x38},
        {.left = 0x07, .right = 0x34},
        {.left = 0x06, .right = 0x30},
        {.left = 0x06, .right = 0x2c},
        {.left = 0x05, .right = 0x28},
        {.left = 0x05, .right = 0x24},
        {.left = 0x04, .right = 0x20},
        {.left = 0x04, .right = 0x1c},
        {.left = 0x03, .right = 0x18},
        {.left = 0x03, .right = 0x14},
        {.left = 0x02, .right = 0x10},
        {.left = 0x02, .right = 0x0c},
        {.left = 0x01, .right = 0x08},
        {.left = 0x01, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0x40, .right = 0xff},
        {.left = 0x3f, .right = 0xfa},
        {.left = 0x3e, .right = 0xf6},
        {.left = 0x3d, .right = 0xf2},
        {.left = 0x3c, .right = 0xee},
        {.left = 0x3b, .right = 0xea},
        {.left = 0x3a, .right = 0xe6},
        {.left = 0x39, .right = 0xe2},
        {.left = 0x38, .right = 0xde},
        {.left = 0x37, .right = 0xda},
        {.left = 0x36, .right = 0xd6},
        {.left = 0x35, .right = 0xd2},
        {.left = 0x34, .right = 0xce},
        {.left = 0x33, .right = 0xca},
        {.left = 0x32, .right = 0xc6},
        {.left = 0x31, .right = 0xc2},
        {.left = 0x30, .right = 0xbe},
        {.left = 0x2f, .right = 0xba},
        {.left = 0x2e, .right = 0xb6},
        {.left = 0x2d, .right = 0xb2},
        {.left = 0x2c, .right = 0xae},
        {.left = 0x2b, .right = 0xaa},
        {.left = 0x2a, .right = 0xa5},
        {.left = 0x29, .right = 0xa1},
        {.left = 0x28, .right = 0x9d},
        {.left = 0x27, .right = 0x99},
        {.left = 0x26, .right = 0x95},
        {.left = 0x25, .right = 0x91},
        {.left = 0x24, .right = 0x8d},
        {.left = 0x23, .right = 0x89},
        {.left = 0x22, .right = 0x85},
        {.left = 0x21, .right = 0x81},
        {.left = 0x20, .right = 0x7d},
        {.left = 0x1f, .right = 0x79},
        {.left = 0x1e, .right = 0x75},
        {.left = 0x1d, .right = 0x71},
        {.left = 0x1c, .right = 0x6d},
        {.left = 0x1b, .right = 0x69},
        {.left = 0x1a, .right = 0x65},
        {.left = 0x19, .right = 0x61},
        {.left = 0x18, .right = 0x5d},
        {.left = 0x17, .right = 0x59},
        {.left = 0x16, .right = 0x55},
        {.left = 0x14, .right = 0x50},
        {.left = 0x13, .right = 0x4c},
        {.left = 0x12, .right = 0x48},
        {.left = 0x11, .right = 0x44},
        {.left = 0x10, .right = 0x40},
        {.left = 0x0f, .right = 0x3c},
        {.left = 0x0e, .right = 0x38},
        {.left = 0x0d, .right = 0x34},
        {.left = 0x0c, .right = 0x30},
        {.left = 0x0b, .right = 0x2c},
        {.left = 0x0a, .right = 0x28},
        {.left = 0x09, .right = 0x24},
        {.left = 0x08, .right = 0x20},
        {.left = 0x07, .right = 0x1c},
        {.left = 0x06, .right = 0x18},
        {.left = 0x05, .right = 0x14},
        {.left = 0x04, .right = 0x10},
        {.left = 0x03, .right = 0x0c},
        {.left = 0x02, .right = 0x08},
        {.left = 0x01, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0x60, .right = 0xff},
        {.left = 0x5e, .right = 0xfa},
        {.left = 0x5d, .right = 0xf6},
        {.left = 0x5b, .right = 0xf2},
        {.left = 0x5a, .right = 0xee},
        {.left = 0x58, .right = 0xea},
        {.left = 0x57, .right = 0xe6},
        {.left = 0x55, .right = 0xe2},
        {.left = 0x54, .right = 0xde},
        {.left = 0x52, .right = 0xda},
        {.left = 0x51, .right = 0xd6},
        {.left = 0x4f, .right = 0xd2},
        {.left = 0x4e, .right = 0xce},
        {.left = 0x4c, .right = 0xca},
        {.left = 0x4b, .right = 0xc6},
        {.left = 0x49, .right = 0xc2},
        {.left = 0x48, .right = 0xbe},
        {.left = 0x46, .right = 0xba},
        {.left = 0x45, .right = 0xb6},
        {.left = 0x43, .right = 0xb2},
        {.left = 0x42, .right = 0xae},
        {.left = 0x40, .right = 0xaa},
        {.left = 0x3e, .right = 0xa5},
        {.left = 0x3d, .right = 0xa1},
        {.left = 0x3b, .right = 0x9d},
        {.left = 0x3a, .right = 0x99},
        {.left = 0x38, .right = 0x95},
        {.left = 0x37, .right = 0x91},
        {.left = 0x35, .right = 0x8d},
        {.left = 0x34, .right = 0x89},
        {.left = 0x32, .right = 0x85},
        {.left = 0x31, .right = 0x81},
        {.left = 0x2f, .right = 0x7d},
        {.left = 0x2e, .right = 0x79},
        {.left = 0x2c, .right = 0x75},
        {.left = 0x2b, .right = 0x71},
        {.left = 0x29, .right = 0x6d},
        {.left = 0x28, .right = 0x69},
        {.left = 0x26, .right = 0x65},
        {.left = 0x25, .right = 0x61},
        {.left = 0x23, .right = 0x5d},
        {.left = 0x22, .right = 0x59},
        {.left = 0x20, .right = 0x55},
        {.left = 0x1e, .right = 0x50},
        {.left = 0x1d, .right = 0x4c},
        {.left = 0x1b, .right = 0x48},
        {.left = 0x1a, .right = 0x44},
        {.left = 0x18, .right = 0x40},
        {.left = 0x17, .right = 0x3c},
        {.left = 0x15, .right = 0x38},
        {.left = 0x14, .right = 0x34},
        {.left = 0x12, .right = 0x30},
        {.left = 0x11, .right = 0x2c},
        {.left = 0x0f, .right = 0x28},
        {.left = 0x0e, .right = 0x24},
        {.left = 0x0c, .right = 0x20},
        {.left = 0x0b, .right = 0x1c},
        {.left = 0x09, .right = 0x18},
        {.left = 0x08, .right = 0x14},
        {.left = 0x06, .right = 0x10},
        {.left = 0x05, .right = 0x0c},
        {.left = 0x03, .right = 0x08},
        {.left = 0x02, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0x80, .right = 0xff},
        {.left = 0x7d, .right = 0xfa},
        {.left = 0x7b, .right = 0xf6},
        {.left = 0x79, .right = 0xf2},
        {.left = 0x77, .right = 0xee},
        {.left = 0x75, .right = 0xea},
        {.left = 0x73, .right = 0xe6},
        {.left = 0x71, .right = 0xe2},
        {.left = 0x6f, .right = 0xde},
        {.left = 0x6d, .right = 0xda},
        {.left = 0x6b, .right = 0xd6},
        {.left = 0x69, .right = 0xd2},
        {.left = 0x67, .right = 0xce},
        {.left = 0x65, .right = 0xca},
        {.left = 0x63, .right = 0xc6},
        {.left = 0x61, .right = 0xc2},
        {.left = 0x5f, .right = 0xbe},
        {.left = 0x5d, .right = 0xba},
        {.left = 0x5b, .right = 0xb6},
        {.left = 0x59, .right = 0xb2},
        {.left = 0x57, .right = 0xae},
        {.left = 0x55, .right = 0xaa},
        {.left = 0x53, .right = 0xa5},
        {.left = 0x51, .right = 0xa1},
        {.left = 0x4f, .right = 0x9d},
        {.left = 0x4d, .right = 0x99},
        {.left = 0x4b, .right = 0x95},
        {.left = 0x49, .right = 0x91},
        {.left = 0x47, .right = 0x8d},
        {.left = 0x45, .right = 0x89},
        {.left = 0x43, .right = 0x85},
        {.left = 0x41, .right = 0x81},
        {.left = 0x3f, .right = 0x7d},
        {.left = 0x3d, .right = 0x79},
        {.left = 0x3b, .right = 0x75},
        {.left = 0x39, .right = 0x71},
        {.left = 0x37, .right = 0x6d},
        {.left = 0x35, .right = 0x69},
        {.left = 0x33, .right = 0x65},
        {.left = 0x31, .right = 0x61},
        {.left = 0x2f, .right = 0x5d},
        {.left = 0x2d, .right = 0x59},
        {.left = 0x2b, .right = 0x55},
        {.left = 0x28, .right = 0x50},
        {.left = 0x26, .right = 0x4c},
        {.left = 0x24, .right = 0x48},
        {.left = 0x22, .right = 0x44},
        {.left = 0x20, .right = 0x40},
        {.left = 0x1e, .right = 0x3c},
        {.left = 0x1c, .right = 0x38},
        {.left = 0x1a, .right = 0x34},
        {.left = 0x18, .right = 0x30},
        {.left = 0x16, .right = 0x2c},
        {.left = 0x14, .right = 0x28},
        {.left = 0x12, .right = 0x24},
        {.left = 0x10, .right = 0x20},
        {.left = 0x0e, .right = 0x1c},
        {.left = 0x0c, .right = 0x18},
        {.left = 0x0a, .right = 0x14},
        {.left = 0x08, .right = 0x10},
        {.left = 0x06, .right = 0x0c},
        {.left = 0x04, .right = 0x08},
        {.left = 0x02, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xa0, .right = 0xff},
        {.left = 0x9d, .right = 0xfa},
        {.left = 0x9a, .right = 0xf6},
        {.left = 0x98, .right = 0xf2},
        {.left = 0x95, .right = 0xee},
        {.left = 0x93, .right = 0xea},
        {.left = 0x90, .right = 0xe6},
        {.left = 0x8e, .right = 0xe2},
        {.left = 0x8b, .right = 0xde},
        {.left = 0x89, .right = 0xda},
        {.left = 0x86, .right = 0xd6},
        {.left = 0x84, .right = 0xd2},
        {.left = 0x81, .right = 0xce},
        {.left = 0x7f, .right = 0xca},
        {.left = 0x7c, .right = 0xc6},
        {.left = 0x7a, .right = 0xc2},
        {.left = 0x77, .right = 0xbe},
        {.left = 0x75, .right = 0xba},
        {.left = 0x72, .right = 0xb6},
        {.left = 0x70, .right = 0xb2},
        {.left = 0x6d, .right = 0xae},
        {.left = 0x6b, .right = 0xaa},
        {.left = 0x68, .right = 0xa5},
        {.left = 0x65, .right = 0xa1},
        {.left = 0x63, .right = 0x9d},
        {.left = 0x60, .right = 0x99},
        {.left = 0x5e, .right = 0x95},
        {.left = 0x5b, .right = 0x91},
        {.left = 0x59, .right = 0x8d},
        {.left = 0x56, .right = 0x89},
        {.left = 0x54, .right = 0x85},
        {.left = 0x51, .right = 0x81},
        {.left = 0x4f, .right = 0x7d},
        {.left = 0x4c, .right = 0x79},
        {.left = 0x4a, .right = 0x75},
        {.left = 0x47, .right = 0x71},
        {.left = 0x45, .right = 0x6d},
        {.left = 0x42, .right = 0x69},
        {.left = 0x40, .right = 0x65},
        {.left = 0x3d, .right = 0x61},
        {.left = 0x3b, .right = 0x5d},
        {.left = 0x38, .right = 0x59},
        {.left = 0x36, .right = 0x55},
        {.left = 0x32, .right = 0x50},
        {.left = 0x30, .right = 0x4c},
        {.left = 0x2d, .right = 0x48},
        {.left = 0x2b, .right = 0x44},
        {.left = 0x28, .right = 0x40},
        {.left = 0x26, .right = 0x3c},
        {.left = 0x23, .right = 0x38},
        {.left = 0x21, .right = 0x34},
        {.left = 0x1e, .right = 0x30},
        {.left = 0x1c, .right = 0x2c},
        {.left = 0x19, .right = 0x28},
        {.left = 0x17, .right = 0x24},
        {.left = 0x14, .right = 0x20},
        {.left = 0x12, .right = 0x1c},
        {.left = 0x0f, .right = 0x18},
        {.left = 0x0d, .right = 0x14},
        {.left = 0x0a, .right = 0x10},
        {.left = 0x08, .right = 0x0c},
        {.left = 0x05, .right = 0x08},
        {.left = 0x03, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xc0, .right = 0xff},
        {.left = 0xbc, .right = 0xfa},
        {.left = 0xb9, .right = 0xf6},
        {.left = 0xb6, .right = 0xf2},
        {.left = 0xb3, .right = 0xee},
        {.left = 0xb0, .right = 0xea},
        {.left = 0xad, .right = 0xe6},
        {.left = 0xaa, .right = 0xe2},
        {.left = 0xa7, .right = 0xde},
        {.left = 0xa4, .right = 0xda},
        {.left = 0xa1, .right = 0xd6},
        {.left = 0x9e, .right = 0xd2},
        {.left = 0x9b, .right = 0xce},
        {.left = 0x98, .right = 0xca},
        {.left = 0x95, .right = 0xc6},
        {.left = 0x92, .right = 0xc2},
        {.left = 0x8f, .right = 0xbe},
        {.left = 0x8c, .right = 0xba},
        {.left = 0x89, .right = 0xb6},
        {.left = 0x86, .right = 0xb2},
        {.left = 0x83, .right = 0xae},
        {.left = 0x80, .right = 0xaa},
        {.left = 0x7c, .right = 0xa5},
        {.left = 0x79, .right = 0xa1},
        {.left = 0x76, .right = 0x9d},
        {.left = 0x73, .right = 0x99},
        {.left = 0x70, .right = 0x95},
        {.left = 0x6d, .right = 0x91},
        {.left = 0x6a, .right = 0x8d},
        {.left = 0x67, .right = 0x89},
        {.left = 0x64, .right = 0x85},
        {.left = 0x61, .right = 0x81},
        {.left = 0x5e, .right = 0x7d},
        {.left = 0x5b, .right = 0x79},
        {.left = 0x58, .right = 0x75},
        {.left = 0x55, .right = 0x71},
        {.left = 0x52, .right = 0x6d},
        {.left = 0x4f, .right = 0x69},
        {.left = 0x4c, .right = 0x65},
        {.left = 0x49, .right = 0x61},
        {.left = 0x46, .right = 0x5d},
        {.left = 0x43, .right = 0x59},
        {.left = 0x40, .right = 0x55},
        {.left = 0x3c, .right = 0x50},
        {.left = 0x39, .right = 0x4c},
        {.left = 0x36, .right = 0x48},
        {.left = 0x33, .right = 0x44},
        {.left = 0x30, .right = 0x40},
        {.left = 0x2d, .right = 0x3c},
        {.left = 0x2a, .right = 0x38},
        {.left = 0x27, .right = 0x34},
        {.left = 0x24, .right = 0x30},
        {.left = 0x21, .right = 0x2c},
        {.left = 0x1e, .right = 0x28},
        {.left = 0x1b, .right = 0x24},
        {.left = 0x18, .right = 0x20},
        {.left = 0x15, .right = 0x1c},
        {.left = 0x12, .right = 0x18},
        {.left = 0x0f, .right = 0x14},
        {.left = 0x0c, .right = 0x10},
        {.left = 0x09, .right = 0x0c},
        {.left = 0x06, .right = 0x08},
        {.left = 0x03, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xe0, .right = 0xff},
        {.left = 0xdb, .right = 0xfa},
        {.left = 0xd8, .right = 0xf6},
        {.left = 0xd4, .right = 0xf2},
        {.left = 0xd1, .right = 0xee},
        {.left = 0xcd, .right = 0xea},
        {.left = 0xca, .right = 0xe6},
        {.left = 0xc6, .right = 0xe2},
        {.left = 0xc3, .right = 0xde},
        {.left = 0xbf, .right = 0xda},
        {.left = 0xbc, .right = 0xd6},
        {.left = 0xb8, .right = 0xd2},
        {.left = 0xb5, .right = 0xce},
        {.left = 0xb1, .right = 0xca},
        {.left = 0xae, .right = 0xc6},
        {.left = 0xaa, .right = 0xc2},
        {.left = 0xa7, .right = 0xbe},
        {.left = 0xa3, .right = 0xba},
        {.left = 0xa0, .right = 0xb6},
        {.left = 0x9c, .right = 0xb2},
        {.left = 0x99, .right = 0xae},
        {.left = 0x95, .right = 0xaa},
        {.left = 0x91, .right = 0xa5},
        {.left = 0x8d, .right = 0xa1},
        {.left = 0x8a, .right = 0x9d},
        {.left = 0x86, .right = 0x99},
        {.left = 0x83, .right = 0x95},
        {.left = 0x7f, .right = 0x91},
        {.left = 0x7c, .right = 0x8d},
        {.left = 0x78, .right = 0x89},
        {.left = 0x75, .right = 0x85},
        {.left = 0x71, .right = 0x81},
        {.left = 0x6e, .right = 0x7d},
        {.left = 0x6a, .right = 0x79},
        {.left = 0x67, .right = 0x75},
        {.left = 0x63, .right = 0x71},
        {.left = 0x60, .right = 0x6d},
        {.left = 0x5c, .right = 0x69},
        {.left = 0x59, .right = 0x65},
        {.left = 0x55, .right = 0x61},
        {.left = 0x52, .right = 0x5d},
        {.left = 0x4e, .right = 0x59},
        {.left = 0x4b, .right = 0x55},
        {.left = 0x46, .right = 0x50},
        {.left = 0x43, .right = 0x4c},
        {.left = 0x3f, .right = 0x48},
        {.left = 0x3c, .right = 0x44},
        {.left = 0x38, .right = 0x40},
        {.left = 0x35, .right = 0x3c},
        {.left = 0x31, .right = 0x38},
        {.left = 0x2e, .right = 0x34},
        {.left = 0x2a, .right = 0x30},
        {.left = 0x27, .right = 0x2c},
        {.left = 0x23, .right = 0x28},
        {.left = 0x20, .right = 0x24},
        {.left = 0x1c, .right = 0x20},
        {.left = 0x19, .right = 0x1c},
        {.left = 0x15, .right = 0x18},
        {.left = 0x12, .right = 0x14},
        {.left = 0x0e, .right = 0x10},
        {.left = 0x0b, .right = 0x0c},
        {.left = 0x07, .right = 0x08},
        {.left = 0x04, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0xff},
        {.left = 0xfa, .right = 0xfa},
        {.left = 0xf6, .right = 0xf6},
        {.left = 0xf2, .right = 0xf2},
        {.left = 0xee, .right = 0xee},
        {.left = 0xea, .right = 0xea},
        {.left = 0xe6, .right = 0xe6},
        {.left = 0xe2, .right = 0xe2},
        {.left = 0xde, .right = 0xde},
        {.left = 0xda, .right = 0xda},
        {.left = 0xd6, .right = 0xd6},
        {.left = 0xd2, .right = 0xd2},
        {.left = 0xce, .right = 0xce},
        {.left = 0xca, .right = 0xca},
        {.left = 0xc6, .right = 0xc6},
        {.left = 0xc2, .right = 0xc2},
        {.left = 0xbe, .right = 0xbe},
        {.left = 0xba, .right = 0xba},
        {.left = 0xb6, .right = 0xb6},
        {.left = 0xb2, .right = 0xb2},
        {.left = 0xae, .right = 0xae},
        {.left = 0xaa, .right = 0xaa},
        {.left = 0xa5, .right = 0xa5},
        {.left = 0xa1, .right = 0xa1},
        {.left = 0x9d, .right = 0x9d},
        {.left = 0x99, .right = 0x99},
        {.left = 0x95, .right = 0x95},
        {.left = 0x91, .right = 0x91},
        {.left = 0x8d, .right = 0x8d},
        {.left = 0x89, .right = 0x89},
        {.left = 0x85, .right = 0x85},
        {.left = 0x81, .right = 0x81},
        {.left = 0x7d, .right = 0x7d},
        {.left = 0x79, .right = 0x79},
        {.left = 0x75, .right = 0x75},
        {.left = 0x71, .right = 0x71},
        {.left = 0x6d, .right = 0x6d},
        {.left = 0x69, .right = 0x69},
        {.left = 0x65, .right = 0x65},
        {.left = 0x61, .right = 0x61},
        {.left = 0x5d, .right = 0x5d},
        {.left = 0x59, .right = 0x59},
        {.left = 0x55, .right = 0x55},
        {.left = 0x50, .right = 0x50},
        {.left = 0x4c, .right = 0x4c},
        {.left = 0x48, .right = 0x48},
        {.left = 0x44, .right = 0x44},
        {.left = 0x40, .right = 0x40},
        {.left = 0x3c, .right = 0x3c},
        {.left = 0x38, .right = 0x38},
        {.left = 0x34, .right = 0x34},
        {.left = 0x30, .right = 0x30},
        {.left = 0x2c, .right = 0x2c},
        {.left = 0x28, .right = 0x28},
        {.left = 0x24, .right = 0x24},
        {.left = 0x20, .right = 0x20},
        {.left = 0x1c, .right = 0x1c},
        {.left = 0x18, .right = 0x18},
        {.left = 0x14, .right = 0x14},
        {.left = 0x10, .right = 0x10},
        {.left = 0x0c, .right = 0x0c},
        {.left = 0x08, .right = 0x08},
        {.left = 0x04, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0xe0},
        {.left = 0xfa, .right = 0xdb},
        {.left = 0xf6, .right = 0xd8},
        {.left = 0xf2, .right = 0xd4},
        {.left = 0xee, .right = 0xd1},
        {.left = 0xea, .right = 0xcd},
        {.left = 0xe6, .right = 0xca},
        {.left = 0xe2, .right = 0xc6},
        {.left = 0xde, .right = 0xc3},
        {.left = 0xda, .right = 0xbf},
        {.left = 0xd6, .right = 0xbc},
        {.left = 0xd2, .right = 0xb8},
        {.left = 0xce, .right = 0xb5},
        {.left = 0xca, .right = 0xb1},
        {.left = 0xc6, .right = 0xae},
        {.left = 0xc2, .right = 0xaa},
        {.left = 0xbe, .right = 0xa7},
        {.left = 0xba, .right = 0xa3},
        {.left = 0xb6, .right = 0xa0},
        {.left = 0xb2, .right = 0x9c},
        {.left = 0xae, .right = 0x99},
        {.left = 0xaa, .right = 0x95},
        {.left = 0xa5, .right = 0x91},
        {.left = 0xa1, .right = 0x8d},
        {.left = 0x9d, .right = 0x8a},
        {.left = 0x99, .right = 0x86},
        {.left = 0x95, .right = 0x83},
        {.left = 0x91, .right = 0x7f},
        {.left = 0x8d, .right = 0x7c},
        {.left = 0x89, .right = 0x78},
        {.left = 0x85, .right = 0x75},
        {.left = 0x81, .right = 0x71},
        {.left = 0x7d, .right = 0x6e},
        {.left = 0x79, .right = 0x6a},
        {.left = 0x75, .right = 0x67},
        {.left = 0x71, .right = 0x63},
        {.left = 0x6d, .right = 0x60},
        {.left = 0x69, .right = 0x5c},
        {.left = 0x65, .right = 0x59},
        {.left = 0x61, .right = 0x55},
        {.left = 0x5d, .right = 0x52},
        {.left = 0x59, .right = 0x4e},
        {.left = 0x55, .right = 0x4b},
        {.left = 0x50, .right = 0x46},
        {.left = 0x4c, .right = 0x43},
        {.left = 0x48, .right = 0x3f},
        {.left = 0x44, .right = 0x3c},
        {.left = 0x40, .right = 0x38},
        {.left = 0x3c, .right = 0x35},
        {.left = 0x38, .right = 0x31},
        {.left = 0x34, .right = 0x2e},
        {.left = 0x30, .right = 0x2a},
        {.left = 0x2c, .right = 0x27},
        {.left = 0x28, .right = 0x23},
        {.left = 0x24, .right = 0x20},
        {.left = 0x20, .right = 0x1c},
        {.left = 0x1c, .right = 0x19},
        {.left = 0x18, .right = 0x15},
        {.left = 0x14, .right = 0x12},
        {.left = 0x10, .right = 0x0e},
        {.left = 0x0c, .right = 0x0b},
        {.left = 0x08, .right = 0x07},
        {.left = 0x04, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0xc0},
        {.left = 0xfa, .right = 0xbc},
        {.left = 0xf6, .right = 0xb9},
        {.left = 0xf2, .right = 0xb6},
        {.left = 0xee, .right = 0xb3},
        {.left = 0xea, .right = 0xb0},
        {.left = 0xe6, .right = 0xad},
        {.left = 0xe2, .right = 0xaa},
        {.left = 0xde, .right = 0xa7},
        {.left = 0xda, .right = 0xa4},
        {.left = 0xd6, .right = 0xa1},
        {.left = 0xd2, .right = 0x9e},
        {.left = 0xce, .right = 0x9b},
        {.left = 0xca, .right = 0x98},
        {.left = 0xc6, .right = 0x95},
        {.left = 0xc2, .right = 0x92},
        {.left = 0xbe, .right = 0x8f},
        {.left = 0xba, .right = 0x8c},
        {.left = 0xb6, .right = 0x89},
        {.left = 0xb2, .right = 0x86},
        {.left = 0xae, .right = 0x83},
        {.left = 0xaa, .right = 0x80},
        {.left = 0xa5, .right = 0x7c},
        {.left = 0xa1, .right = 0x79},
        {.left = 0x9d, .right = 0x76},
        {.left = 0x99, .right = 0x73},
        {.left = 0x95, .right = 0x70},
        {.left = 0x91, .right = 0x6d},
        {.left = 0x8d, .right = 0x6a},
        {.left = 0x89, .right = 0x67},
        {.left = 0x85, .right = 0x64},
        {.left = 0x81, .right = 0x61},
        {.left = 0x7d, .right = 0x5e},
        {.left = 0x79, .right = 0x5b},
        {.left = 0x75, .right = 0x58},
        {.left = 0x71, .right = 0x55},
        {.left = 0x6d, .right = 0x52},
        {.left = 0x69, .right = 0x4f},
        {.left = 0x65, .right = 0x4c},
        {.left = 0x61, .right = 0x49},
        {.left = 0x5d, .right = 0x46},
        {.left = 0x59, .right = 0x43},
        {.left = 0x55, .right = 0x40},
        {.left = 0x50, .right = 0x3c},
        {.left = 0x4c, .right = 0x39},
        {.left = 0x48, .right = 0x36},
        {.left = 0x44, .right = 0x33},
        {.left = 0x40, .right = 0x30},
        {.left = 0x3c, .right = 0x2d},
        {.left = 0x38, .right = 0x2a},
        {.left = 0x34, .right = 0x27},
        {.left = 0x30, .right = 0x24},
        {.left = 0x2c, .right = 0x21},
        {.left = 0x28, .right = 0x1e},
        {.left = 0x24, .right = 0x1b},
        {.left = 0x20, .right = 0x18},
        {.left = 0x1c, .right = 0x15},
        {.left = 0x18, .right = 0x12},
        {.left = 0x14, .right = 0x0f},
        {.left = 0x10, .right = 0x0c},
        {.left = 0x0c, .right = 0x09},
        {.left = 0x08, .right = 0x06},
        {.left = 0x04, .right = 0x03},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0xa0},
        {.left = 0xfa, .right = 0x9d},
        {.left = 0xf6, .right = 0x9a},
        {.left = 0xf2, .right = 0x98},
        {.left = 0xee, .right = 0x95},
        {.left = 0xea, .right = 0x93},
        {.left = 0xe6, .right = 0x90},
        {.left = 0xe2, .right = 0x8e},
        {.left = 0xde, .right = 0x8b},
        {.left = 0xda, .right = 0x89},
        {.left = 0xd6, .right = 0x86},
        {.left = 0xd2, .right = 0x84},
        {.left = 0xce, .right = 0x81},
        {.left = 0xca, .right = 0x7f},
        {.left = 0xc6, .right = 0x7c},
        {.left = 0xc2, .right = 0x7a},
        {.left = 0xbe, .right = 0x77},
        {.left = 0xba, .right = 0x75},
        {.left = 0xb6, .right = 0x72},
        {.left = 0xb2, .right = 0x70},
        {.left = 0xae, .right = 0x6d},
        {.left = 0xaa, .right = 0x6b},
        {.left = 0xa5, .right = 0x68},
        {.left = 0xa1, .right = 0x65},
        {.left = 0x9d, .right = 0x63},
        {.left = 0x99, .right = 0x60},
        {.left = 0x95, .right = 0x5e},
        {.left = 0x91, .right = 0x5b},
        {.left = 0x8d, .right = 0x59},
        {.left = 0x89, .right = 0x56},
        {.left = 0x85, .right = 0x54},
        {.left = 0x81, .right = 0x51},
        {.left = 0x7d, .right = 0x4f},
        {.left = 0x79, .right = 0x4c},
        {.left = 0x75, .right = 0x4a},
        {.left = 0x71, .right = 0x47},
        {.left = 0x6d, .right = 0x45},
        {.left = 0x69, .right = 0x42},
        {.left = 0x65, .right = 0x40},
        {.left = 0x61, .right = 0x3d},
        {.left = 0x5d, .right = 0x3b},
        {.left = 0x59, .right = 0x38},
        {.left = 0x55, .right = 0x36},
        {.left = 0x50, .right = 0x32},
        {.left = 0x4c, .right = 0x30},
        {.left = 0x48, .right = 0x2d},
        {.left = 0x44, .right = 0x2b},
        {.left = 0x40, .right = 0x28},
        {.left = 0x3c, .right = 0x26},
        {.left = 0x38, .right = 0x23},
        {.left = 0x34, .right = 0x21},
        {.left = 0x30, .right = 0x1e},
        {.left = 0x2c, .right = 0x1c},
        {.left = 0x28, .right = 0x19},
        {.left = 0x24, .right = 0x17},
        {.left = 0x20, .right = 0x14},
        {.left = 0x1c, .right = 0x12},
        {.left = 0x18, .right = 0x0f},
        {.left = 0x14, .right = 0x0d},
        {.left = 0x10, .right = 0x0a},
        {.left = 0x0c, .right = 0x08},
        {.left = 0x08, .right = 0x05},
        {.left = 0x04, .right = 0x03},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0x80},
        {.left = 0xfa, .right = 0x7d},
        {.left = 0xf6, .right = 0x7b},
        {.left = 0xf2, .right = 0x79},
        {.left = 0xee, .right = 0x77},
        {.left = 0xea, .right = 0x75},
        {.left = 0xe6, .right = 0x73},
        {.left = 0xe2, .right = 0x71},
        {.left = 0xde, .right = 0x6f},
        {.left = 0xda, .right = 0x6d},
        {.left = 0xd6, .right = 0x6b},
        {.left = 0xd2, .right = 0x69},
        {.left = 0xce, .right = 0x67},
        {.left = 0xca, .right = 0x65},
        {.left = 0xc6, .right = 0x63},
        {.left = 0xc2, .right = 0x61},
        {.left = 0xbe, .right = 0x5f},
        {.left = 0xba, .right = 0x5d},
        {.left = 0xb6, .right = 0x5b},
        {.left = 0xb2, .right = 0x59},
        {.left = 0xae, .right = 0x57},
        {.left = 0xaa, .right = 0x55},
        {.left = 0xa5, .right = 0x53},
        {.left = 0xa1, .right = 0x51},
        {.left = 0x9d, .right = 0x4f},
        {.left = 0x99, .right = 0x4d},
        {.left = 0x95, .right = 0x4b},
        {.left = 0x91, .right = 0x49},
        {.left = 0x8d, .right = 0x47},
        {.left = 0x89, .right = 0x45},
        {.left = 0x85, .right = 0x43},
        {.left = 0x81, .right = 0x41},
        {.left = 0x7d, .right = 0x3f},
        {.left = 0x79, .right = 0x3d},
        {.left = 0x75, .right = 0x3b},
        {.left = 0x71, .right = 0x39},
        {.left = 0x6d, .right = 0x37},
        {.left = 0x69, .right = 0x35},
        {.left = 0x65, .right = 0x33},
        {.left = 0x61, .right = 0x31},
        {.left = 0x5d, .right = 0x2f},
        {.left = 0x59, .right = 0x2d},
        {.left = 0x55, .right = 0x2b},
        {.left = 0x50, .right = 0x28},
        {.left = 0x4c, .right = 0x26},
        {.left = 0x48, .right = 0x24},
        {.left = 0x44, .right = 0x22},
        {.left = 0x40, .right = 0x20},
        {.left = 0x3c, .right = 0x1e},
        {.left = 0x38, .right = 0x1c},
        {.left = 0x34, .right = 0x1a},
        {.left = 0x30, .right = 0x18},
        {.left = 0x2c, .right = 0x16},
        {.left = 0x28, .right = 0x14},
        {.left = 0x24, .right = 0x12},
        {.left = 0x20, .right = 0x10},
        {.left = 0x1c, .right = 0x0e},
        {.left = 0x18, .right = 0x0c},
        {.left = 0x14, .right = 0x0a},
        {.left = 0x10, .right = 0x08},
        {.left = 0x0c, .right = 0x06},
        {.left = 0x08, .right = 0x04},
        {.left = 0x04, .right = 0x02},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0x60},
        {.left = 0xfa, .right = 0x5e},
        {.left = 0xf6, .right = 0x5d},
        {.left = 0xf2, .right = 0x5b},
        {.left = 0xee, .right = 0x5a},
        {.left = 0xea, .right = 0x58},
        {.left = 0xe6, .right = 0x57},
        {.left = 0xe2, .right = 0x55},
        {.left = 0xde, .right = 0x54},
        {.left = 0xda, .right = 0x52},
        {.left = 0xd6, .right = 0x51},
        {.left = 0xd2, .right = 0x4f},
        {.left = 0xce, .right = 0x4e},
        {.left = 0xca, .right = 0x4c},
        {.left = 0xc6, .right = 0x4b},
        {.left = 0xc2, .right = 0x49},
        {.left = 0xbe, .right = 0x48},
        {.left = 0xba, .right = 0x46},
        {.left = 0xb6, .right = 0x45},
        {.left = 0xb2, .right = 0x43},
        {.left = 0xae, .right = 0x42},
        {.left = 0xaa, .right = 0x40},
        {.left = 0xa5, .right = 0x3e},
        {.left = 0xa1, .right = 0x3d},
        {.left = 0x9d, .right = 0x3b},
        {.left = 0x99, .right = 0x3a},
        {.left = 0x95, .right = 0x38},
        {.left = 0x91, .right = 0x37},
        {.left = 0x8d, .right = 0x35},
        {.left = 0x89, .right = 0x34},
        {.left = 0x85, .right = 0x32},
        {.left = 0x81, .right = 0x31},
        {.left = 0x7d, .right = 0x2f},
        {.left = 0x79, .right = 0x2e},
        {.left = 0x75, .right = 0x2c},
        {.left = 0x71, .right = 0x2b},
        {.left = 0x6d, .right = 0x29},
        {.left = 0x69, .right = 0x28},
        {.left = 0x65, .right = 0x26},
        {.left = 0x61, .right = 0x25},
        {.left = 0x5d, .right = 0x23},
        {.left = 0x59, .right = 0x22},
        {.left = 0x55, .right = 0x20},
        {.left = 0x50, .right = 0x1e},
        {.left = 0x4c, .right = 0x1d},
        {.left = 0x48, .right = 0x1b},
        {.left = 0x44, .right = 0x1a},
        {.left = 0x40, .right = 0x18},
        {.left = 0x3c, .right = 0x17},
        {.left = 0x38, .right = 0x15},
        {.left = 0x34, .right = 0x14},
        {.left = 0x30, .right = 0x12},
        {.left = 0x2c, .right = 0x11},
        {.left = 0x28, .right = 0x0f},
        {.left = 0x24, .right = 0x0e},
        {.left = 0x20, .right = 0x0c},
        {.left = 0x1c, .right = 0x0b},
        {.left = 0x18, .right = 0x09},
        {.left = 0x14, .right = 0x08},
        {.left = 0x10, .right = 0x06},
        {.left = 0x0c, .right = 0x05},
        {.left = 0x08, .right = 0x03},
        {.left = 0x04, .right = 0x02},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0x40},
        {.left = 0xfa, .right = 0x3f},
        {.left = 0xf6, .right = 0x3e},
        {.left = 0xf2, .right = 0x3d},
        {.left = 0xee, .right = 0x3c},
        {.left = 0xea, .right = 0x3b},
        {.left = 0xe6, .right = 0x3a},
        {.left = 0xe2, .right = 0x39},
        {.left = 0xde, .right = 0x38},
        {.left = 0xda, .right = 0x37},
        {.left = 0xd6, .right = 0x36},
        {.left = 0xd2, .right = 0x35},
        {.left = 0xce, .right = 0x34},
        {.left = 0xca, .right = 0x33},
        {.left = 0xc6, .right = 0x32},
        {.left = 0xc2, .right = 0x31},
        {.left = 0xbe, .right = 0x30},
        {.left = 0xba, .right = 0x2f},
        {.left = 0xb6, .right = 0x2e},
        {.left = 0xb2, .right = 0x2d},
        {.left = 0xae, .right = 0x2c},
        {.left = 0xaa, .right = 0x2b},
        {.left = 0xa5, .right = 0x2a},
        {.left = 0xa1, .right = 0x29},
        {.left = 0x9d, .right = 0x28},
        {.left = 0x99, .right = 0x27},
        {.left = 0x95, .right = 0x26},
        {.left = 0x91, .right = 0x25},
        {.left = 0x8d, .right = 0x24},
        {.left = 0x89, .right = 0x23},
        {.left = 0x85, .right = 0x22},
        {.left = 0x81, .right = 0x21},
        {.left = 0x7d, .right = 0x20},
        {.left = 0x79, .right = 0x1f},
        {.left = 0x75, .right = 0x1e},
        {.left = 0x71, .right = 0x1d},
        {.left = 0x6d, .right = 0x1c},
        {.left = 0x69, .right = 0x1b},
        {.left = 0x65, .right = 0x1a},
        {.left = 0x61, .right = 0x19},
        {.left = 0x5d, .right = 0x18},
        {.left = 0x59, .right = 0x17},
        {.left = 0x55, .right = 0x16},
        {.left = 0x50, .right = 0x14},
        {.left = 0x4c, .right = 0x13},
        {.left = 0x48, .right = 0x12},
        {.left = 0x44, .right = 0x11},
        {.left = 0x40, .right = 0x10},
        {.left = 0x3c, .right = 0x0f},
        {.left = 0x38, .right = 0x0e},
        {.left = 0x34, .right = 0x0d},
        {.left = 0x30, .right = 0x0c},
        {.left = 0x2c, .right = 0x0b},
        {.left = 0x28, .right = 0x0a},
        {.left = 0x24, .right = 0x09},
        {.left = 0x20, .right = 0x08},
        {.left = 0x1c, .right = 0x07},
        {.left = 0x18, .right = 0x06},
        {.left = 0x14, .right = 0x05},
        {.left = 0x10, .right = 0x04},
        {.left = 0x0c, .right = 0x03},
        {.left = 0x08, .right = 0x02},
        {.left = 0x04, .right = 0x01},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0x00},
        {.left = 0xfa, .right = 0x00},
        {.left = 0xf6, .right = 0x00},
        {.left = 0xf2, .right = 0x00},
        {.left = 0xee, .right = 0x00},
        {.left = 0xea, .right = 0x00},
        {.left = 0xe6, .right = 0x00},
        {.left = 0xe2, .right = 0x00},
        {.left = 0xde, .right = 0x00},
        {.left = 0xda, .right = 0x00},
        {.left = 0xd6, .right = 0x00},
        {.left = 0xd2, .right = 0x00},
        {.left = 0xce, .right = 0x00},
        {.left = 0xca, .right = 0x00},
        {.left = 0xc6, .right = 0x00},
        {.left = 0xc2, .right = 0x00},
        {.left = 0xbe, .right = 0x00},
        {.left = 0xba, .right = 0x00},
        {.left = 0xb6, .right = 0x00},
        {.left = 0xb2, .right = 0x00},
        {.left = 0xae, .right = 0x00},
        {.left = 0xaa, .right = 0x00},
        {.left = 0xa5, .right = 0x00},
        {.left = 0xa1, .right = 0x00},
        {.left = 0x9d, .right = 0x00},
        {.left = 0x99, .right = 0x00},
        {.left = 0x95, .right = 0x00},
        {.left = 0x91, .right = 0x00},
        {.left = 0x8d, .right = 0x00},
        {.left = 0x89, .right = 0x00},
        {.left = 0x85, .right = 0x00},
        {.left = 0x81, .right = 0x00},
        {.left = 0x7d, .right = 0x00},
        {.left = 0x79, .right = 0x00},
        {.left = 0x75, .right = 0x00},
        {.left = 0x71, .right = 0x00},
        {.left = 0x6d, .right = 0x00},
        {.left = 0x69, .right = 0x00},
        {.left = 0x65, .right = 0x00},
        {.left = 0x61, .right = 0x00},
        {.left = 0x5d, .right = 0x00},
        {.left = 0x59, .right = 0x00},
        {.left = 0x55, .right = 0x00},
        {.left = 0x50, .right = 0x00},
        {.left = 0x4c, .right = 0x00},
        {.left = 0x48, .right = 0x00},
        {.left = 0x44, .right = 0x00},
        {.left = 0x40, .right = 0x00},
        {.left = 0x3c, .right = 0x00},
        {.left = 0x38, .right = 0x00},
        {.left = 0x34, .right = 0x00},
        {.left = 0x30, .right = 0x00},
        {.left = 0x2c, .right = 0x00},
        {.left = 0x28, .right = 0x00},
        {.left = 0x24, .right = 0x00},
        {.left = 0x20, .right = 0x00},
        {.left = 0x1c, .right = 0x00},
        {.left = 0x18, .right = 0x00},
        {.left = 0x14, .right = 0x00},
        {.left = 0x10, .right = 0x00},
        {.left = 0x0c, .right = 0x00},
        {.left = 0x08, .right = 0x00},
        {.left = 0x04, .right = 0x00},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0x00},
        {.left = 0xfa, .right = 0x00},
        {.left = 0xf6, .right = 0x00},
        {.left = 0xf2, .right = 0x00},
        {.left = 0xee, .right = 0x00},
        {.left = 0xea, .right = 0x00},
        {.left = 0xe6, .right = 0x00},
        {.left = 0xe2, .right = 0x00},
        {.left = 0xde, .right = 0x00},
        {.left = 0xda, .right = 0x00},
        {.left = 0xd6, .right = 0x00},
        {.left = 0xd2, .right = 0x00},
        {.left = 0xce, .right = 0x00},
        {.left = 0xca, .right = 0x00},
        {.left = 0xc6, .right = 0x00},
        {.left = 0xc2, .right = 0x00},
        {.left = 0xbe, .right = 0x00},
        {.left = 0xba, .right = 0x00},
        {.left = 0xb6, .right = 0x00},
        {.left = 0xb2, .right = 0x00},
        {.left = 0xae, .right = 0x00},
        {.left = 0xaa, .right = 0x00},
        {.left = 0xa5, .right = 0x00},
        {.left = 0xa1, .right = 0x00},
        {.left = 0x9d, .right = 0x00},
        {.left = 0x99, .right = 0x00},
        {.left = 0x95, .right = 0x00},
        {.left = 0x91, .right = 0x00},
        {.left = 0x8d, .right = 0x00},
        {.left = 0x89, .right = 0x00},
        {.left = 0x85, .right = 0x00},
        {.left = 0x81, .right = 0x00},
        {.left = 0x7d, .right = 0x00},
        {.left = 0x79, .right = 0x00},
        {.left = 0x75, .right = 0x00},
        {.left = 0x71, .right = 0x00},
        {.left = 0x6d, .right = 0x00},
        {.left = 0x69, .right = 0x00},
        {.left = 0x65, .right = 0x00},
        {.left = 0x61, .right = 0x00},
        {.left = 0x5d, .right = 0x00},
        {.left = 0x59, .right = 0x00},
        {.left = 0x55, .right = 0x00},
        {.left = 0x50, .right = 0x00},
        {.left = 0x4c, .right = 0x00},
        {.left = 0x48, .right = 0x00},
        {.left = 0x44, .right = 0x00},
        {.left = 0x40, .right = 0x00},
        {.left = 0x3c, .right = 0x00},
        {.left = 0x38, .right = 0x00},
        {.left = 0x34, .right = 0x00},
        {.left = 0x30, .right = 0x00},
        {.left = 0x2c, .right = 0x00},
        {.left = 0x28, .right = 0x00},
        {.left = 0x24, .right = 0x00},
        {.left = 0x20, .right = 0x00},
        {.left = 0x1c, .right = 0x00},
        {.left = 0x18, .right = 0x00},
        {.left = 0x14, .right = 0x00},
        {.left = 0x10, .right = 0x00},
        {.left = 0x0c, .right = 0x00},
        {.left = 0x08, .right = 0x00},
        {.left = 0x04, .right = 0x00},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0x40},
        {.left = 0xfa, .right = 0x3f},
        {.left = 0xf6, .right = 0x3e},
        {.left = 0xf2, .right = 0x3d},
        {.left = 0xee, .right = 0x3c},
        {.left = 0xea, .right = 0x3b},
        {.left = 0xe6, .right = 0x3a},
        {.left = 0xe2, .right = 0x39},
        {.left = 0xde, .right = 0x38},
        {.left = 0xda, .right = 0x37},
        {.left = 0xd6, .right = 0x36},
        {.left = 0xd2, .right = 0x35},
        {.left = 0xce, .right = 0x34},
        {.left = 0xca, .right = 0x33},
        {.left = 0xc6, .right = 0x32},
        {.left = 0xc2, .right = 0x31},
        {.left = 0xbe, .right = 0x30},
        {.left = 0xba, .right = 0x2f},
        {.left = 0xb6, .right = 0x2e},
        {.left = 0xb2, .right = 0x2d},
        {.left = 0xae, .right = 0x2c},
        {.left = 0xaa, .right = 0x2b},
        {.left = 0xa5, .right = 0x2a},
        {.left = 0xa1, .right = 0x29},
        {.left = 0x9d, .right = 0x28},
        {.left = 0x99, .right = 0x27},
        {.left = 0x95, .right = 0x26},
        {.left = 0x91, .right = 0x25},
        {.left = 0x8d, .right = 0x24},
        {.left = 0x89, .right = 0x23},
        {.left = 0x85, .right = 0x22},
        {.left = 0x81, .right = 0x21},
        {.left = 0x7d, .right = 0x20},
        {.left = 0x79, .right = 0x1f},
        {.left = 0x75, .right = 0x1e},
        {.left = 0x71, .right = 0x1d},
        {.left = 0x6d, .right = 0x1c},
        {.left = 0x69, .right = 0x1b},
        {.left = 0x65, .right = 0x1a},
        {.left = 0x61, .right = 0x19},
        {.left = 0x5d, .right = 0x18},
        {.left = 0x59, .right = 0x17},
        {.left = 0x55, .right = 0x16},
        {.left = 0x50, .right = 0x14},
        {.left = 0x4c, .right = 0x13},
        {.left = 0x48, .right = 0x12},
        {.left = 0x44, .right = 0x11},
        {.left = 0x40, .right = 0x10},
        {.left = 0x3c, .right = 0x0f},
        {.left = 0x38, .right = 0x0e},
        {.left = 0x34, .right = 0x0d},
        {.left = 0x30, .right = 0x0c},
        {.left = 0x2c, .right = 0x0b},
        {.left = 0x28, .right = 0x0a},
        {.left = 0x24, .right = 0x09},
        {.left = 0x20, .right = 0x08},
        {.left = 0x1c, .right = 0x07},
        {.left = 0x18, .right = 0x06},
        {.left = 0x14, .right = 0x05},
        {.left = 0x10, .right = 0x04},
        {.left = 0x0c, .right = 0x03},
        {.left = 0x08, .right = 0x02},
        {.left = 0x04, .right = 0x01},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0x60},
        {.left = 0xfa, .right = 0x5e},
        {.left = 0xf6, .right = 0x5d},
        {.left = 0xf2, .right = 0x5b},
        {.left = 0xee, .right = 0x5a},
        {.left = 0xea, .right = 0x58},
        {.left = 0xe6, .right = 0x57},
        {.left = 0xe2, .right = 0x55},
        {.left = 0xde, .right = 0x54},
        {.left = 0xda, .right = 0x52},
        {.left = 0xd6, .right = 0x51},
        {.left = 0xd2, .right = 0x4f},
        {.left = 0xce, .right = 0x4e},
        {.left = 0xca, .right = 0x4c},
        {.left = 0xc6, .right = 0x4b},
        {.left = 0xc2, .right = 0x49},
        {.left = 0xbe, .right = 0x48},
        {.left = 0xba, .right = 0x46},
        {.left = 0xb6, .right = 0x45},
        {.left = 0xb2, .right = 0x43},
        {.left = 0xae, .right = 0x42},
        {.left = 0xaa, .right = 0x40},
        {.left = 0xa5, .right = 0x3e},
        {.left = 0xa1, .right = 0x3d},
        {.left = 0x9d, .right = 0x3b},
        {.left = 0x99, .right = 0x3a},
        {.left = 0x95, .right = 0x38},
        {.left = 0x91, .right = 0x37},
        {.left = 0x8d, .right = 0x35},
        {.left = 0x89, .right = 0x34},
        {.left = 0x85, .right = 0x32},
        {.left = 0x81, .right = 0x31},
        {.left = 0x7d, .right = 0x2f},
        {.left = 0x79, .right = 0x2e},
        {.left = 0x75, .right = 0x2c},
        {.left = 0x71, .right = 0x2b},
        {.left = 0x6d, .right = 0x29},
        {.left = 0x69, .right = 0x28},
        {.left = 0x65, .right = 0x26},
        {.left = 0x61, .right = 0x25},
        {.left = 0x5d, .right = 0x23},
        {.left = 0x59, .right = 0x22},
        {.left = 0x55, .right = 0x20},
        {.left = 0x50, .right = 0x1e},
        {.left = 0x4c, .right = 0x1d},
        {.left = 0x48, .right = 0x1b},
        {.left = 0x44, .right = 0x1a},
        {.left = 0x40, .right = 0x18},
        {.left = 0x3c, .right = 0x17},
        {.left = 0x38, .right = 0x15},
        {.left = 0x34, .right = 0x14},
        {.left = 0x30, .right = 0x12},
        {.left = 0x2c, .right = 0x11},
        {.left = 0x28, .right = 0x0f},
        {.left = 0x24, .right = 0x0e},
        {.left = 0x20, .right = 0x0c},
        {.left = 0x1c, .right = 0x0b},
        {.left = 0x18, .right = 0x09},
        {.left = 0x14, .right = 0x08},
        {.left = 0x10, .right = 0x06},
        {.left = 0x0c, .right = 0x05},
        {.left = 0x08, .right = 0x03},
        {.left = 0x04, .right = 0x02},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0x80},
        {.left = 0xfa, .right = 0x7d},
        {.left = 0xf6, .right = 0x7b},
        {.left = 0xf2, .right = 0x79},
        {.left = 0xee, .right = 0x77},
        {.left = 0xea, .right = 0x75},
        {.left = 0xe6, .right = 0x73},
        {.left = 0xe2, .right = 0x71},
        {.left = 0xde, .right = 0x6f},
        {.left = 0xda, .right = 0x6d},
        {.left = 0xd6, .right = 0x6b},
        {.left = 0xd2, .right = 0x69},
        {.left = 0xce, .right = 0x67},
        {.left = 0xca, .right = 0x65},
        {.left = 0xc6, .right = 0x63},
        {.left = 0xc2, .right = 0x61},
        {.left = 0xbe, .right = 0x5f},
        {.left = 0xba, .right = 0x5d},
        {.left = 0xb6, .right = 0x5b},
        {.left = 0xb2, .right = 0x59},
        {.left = 0xae, .right = 0x57},
        {.left = 0xaa, .right = 0x55},
        {.left = 0xa5, .right = 0x53},
        {.left = 0xa1, .right = 0x51},
        {.left = 0x9d, .right = 0x4f},
        {.left = 0x99, .right = 0x4d},
        {.left = 0x95, .right = 0x4b},
        {.left = 0x91, .right = 0x49},
        {.left = 0x8d, .right = 0x47},
        {.left = 0x89, .right = 0x45},
        {.left = 0x85, .right = 0x43},
        {.left = 0x81, .right = 0x41},
        {.left = 0x7d, .right = 0x3f},
        {.left = 0x79, .right = 0x3d},
        {.left = 0x75, .right = 0x3b},
        {.left = 0x71, .right = 0x39},
        {.left = 0x6d, .right = 0x37},
        {.left = 0x69, .right = 0x35},
        {.left = 0x65, .right = 0x33},
        {.left = 0x61, .right = 0x31},
        {.left = 0x5d, .right = 0x2f},
        {.left = 0x59, .right = 0x2d},
        {.left = 0x55, .right = 0x2b},
        {.left = 0x50, .right = 0x28},
        {.left = 0x4c, .right = 0x26},
        {.left = 0x48, .right = 0x24},
        {.left = 0x44, .right = 0x22},
        {.left = 0x40, .right = 0x20},
        {.left = 0x3c, .right = 0x1e},
        {.left = 0x38, .right = 0x1c},
        {.left = 0x34, .right = 0x1a},
        {.left = 0x30, .right = 0x18},
        {.left = 0x2c, .right = 0x16},
        {.left = 0x28, .right = 0x14},
        {.left = 0x24, .right = 0x12},
        {.left = 0x20, .right = 0x10},
        {.left = 0x1c, .right = 0x0e},
        {.left = 0x18, .right = 0x0c},
        {.left = 0x14, .right = 0x0a},
        {.left = 0x10, .right = 0x08},
        {.left = 0x0c, .right = 0x06},
        {.left = 0x08, .right = 0x04},
        {.left = 0x04, .right = 0x02},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0xa0},
        {.left = 0xfa, .right = 0x9d},
        {.left = 0xf6, .right = 0x9a},
        {.left = 0xf2, .right = 0x98},
        {.left = 0xee, .right = 0x95},
        {.left = 0xea, .right = 0x93},
        {.left = 0xe6, .right = 0x90},
        {.left = 0xe2, .right = 0x8e},
        {.left = 0xde, .right = 0x8b},
        {.left = 0xda, .right = 0x89},
        {.left = 0xd6, .right = 0x86},
        {.left = 0xd2, .right = 0x84},
        {.left = 0xce, .right = 0x81},
        {.left = 0xca, .right = 0x7f},
        {.left = 0xc6, .right = 0x7c},
        {.left = 0xc2, .right = 0x7a},
        {.left = 0xbe, .right = 0x77},
        {.left = 0xba, .right = 0x75},
        {.left = 0xb6, .right = 0x72},
        {.left = 0xb2, .right = 0x70},
        {.left = 0xae, .right = 0x6d},
        {.left = 0xaa, .right = 0x6b},
        {.left = 0xa5, .right = 0x68},
        {.left = 0xa1, .right = 0x65},
        {.left = 0x9d, .right = 0x63},
        {.left = 0x99, .right = 0x60},
        {.left = 0x95, .right = 0x5e},
        {.left = 0x91, .right = 0x5b},
        {.left = 0x8d, .right = 0x59},
        {.left = 0x89, .right = 0x56},
        {.left = 0x85, .right = 0x54},
        {.left = 0x81, .right = 0x51},
        {.left = 0x7d, .right = 0x4f},
        {.left = 0x79, .right = 0x4c},
        {.left = 0x75, .right = 0x4a},
        {.left = 0x71, .right = 0x47},
        {.left = 0x6d, .right = 0x45},
        {.left = 0x69, .right = 0x42},
        {.left = 0x65, .right = 0x40},
        {.left = 0x61, .right = 0x3d},
        {.left = 0x5d, .right = 0x3b},
        {.left = 0x59, .right = 0x38},
        {.left = 0x55, .right = 0x36},
        {.left = 0x50, .right = 0x32},
        {.left = 0x4c, .right = 0x30},
        {.left = 0x48, .right = 0x2d},
        {.left = 0x44, .right = 0x2b},
        {.left = 0x40, .right = 0x28},
        {.left = 0x3c, .right = 0x26},
        {.left = 0x38, .right = 0x23},
        {.left = 0x34, .right = 0x21},
        {.left = 0x30, .right = 0x1e},
        {.left = 0x2c, .right = 0x1c},
        {.left = 0x28, .right = 0x19},
        {.left = 0x24, .right = 0x17},
        {.left = 0x20, .right = 0x14},
        {.left = 0x1c, .right = 0x12},
        {.left = 0x18, .right = 0x0f},
        {.left = 0x14, .right = 0x0d},
        {.left = 0x10, .right = 0x0a},
        {.left = 0x0c, .right = 0x08},
        {.left = 0x08, .right = 0x05},
        {.left = 0x04, .right = 0x03},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0xc0},
        {.left = 0xfa, .right = 0xbc},
        {.left = 0xf6, .right = 0xb9},
        {.left = 0xf2, .right = 0xb6},
        {.left = 0xee, .right = 0xb3},
        {.left = 0xea, .right = 0xb0},
        {.left = 0xe6, .right = 0xad},
        {.left = 0xe2, .right = 0xaa},
        {.left = 0xde, .right = 0xa7},
        {.left = 0xda, .right = 0xa4},
        {.left = 0xd6, .right = 0xa1},
        {.left = 0xd2, .right = 0x9e},
        {.left = 0xce, .right = 0x9b},
        {.left = 0xca, .right = 0x98},
        {.left = 0xc6, .right = 0x95},
        {.left = 0xc2, .right = 0x92},
        {.left = 0xbe, .right = 0x8f},
        {.left = 0xba, .right = 0x8c},
        {.left = 0xb6, .right = 0x89},
        {.left = 0xb2, .right = 0x86},
        {.left = 0xae, .right = 0x83},
        {.left = 0xaa, .right = 0x80},
        {.left = 0xa5, .right = 0x7c},
        {.left = 0xa1, .right = 0x79},
        {.left = 0x9d, .right = 0x76},
        {.left = 0x99, .right = 0x73},
        {.left = 0x95, .right = 0x70},
        {.left = 0x91, .right = 0x6d},
        {.left = 0x8d, .right = 0x6a},
        {.left = 0x89, .right = 0x67},
        {.left = 0x85, .right = 0x64},
        {.left = 0x81, .right = 0x61},
        {.left = 0x7d, .right = 0x5e},
        {.left = 0x79, .right = 0x5b},
        {.left = 0x75, .right = 0x58},
        {.left = 0x71, .right = 0x55},
        {.left = 0x6d, .right = 0x52},
        {.left = 0x69, .right = 0x4f},
        {.left = 0x65, .right = 0x4c},
        {.left = 0x61, .right = 0x49},
        {.left = 0x5d, .right = 0x46},
        {.left = 0x59, .right = 0x43},
        {.left = 0x55, .right = 0x40},
        {.left = 0x50, .right = 0x3c},
        {.left = 0x4c, .right = 0x39},
        {.left = 0x48, .right = 0x36},
        {.left = 0x44, .right = 0x33},
        {.left = 0x40, .right = 0x30},
        {.left = 0x3c, .right = 0x2d},
        {.left = 0x38, .right = 0x2a},
        {.left = 0x34, .right = 0x27},
        {.left = 0x30, .right = 0x24},
        {.left = 0x2c, .right = 0x21},
        {.left = 0x28, .right = 0x1e},
        {.left = 0x24, .right = 0x1b},
        {.left = 0x20, .right = 0x18},
        {.left = 0x1c, .right = 0x15},
        {.left = 0x18, .right = 0x12},
        {.left = 0x14, .right = 0x0f},
        {.left = 0x10, .right = 0x0c},
        {.left = 0x0c, .right = 0x09},
        {.left = 0x08, .right = 0x06},
        {.left = 0x04, .right = 0x03},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0xe0},
        {.left = 0xfa, .right = 0xdb},
        {.left = 0xf6, .right = 0xd8},
        {.left = 0xf2, .right = 0xd4},
        {.left = 0xee, .right = 0xd1},
        {.left = 0xea, .right = 0xcd},
        {.left = 0xe6, .right = 0xca},
        {.left = 0xe2, .right = 0xc6},
        {.left = 0xde, .right = 0xc3},
        {.left = 0xda, .right = 0xbf},
        {.left = 0xd6, .right = 0xbc},
        {.left = 0xd2, .right = 0xb8},
        {.left = 0xce, .right = 0xb5},
        {.left = 0xca, .right = 0xb1},
        {.left = 0xc6, .right = 0xae},
        {.left = 0xc2, .right = 0xaa},
        {.left = 0xbe, .right = 0xa7},
        {.left = 0xba, .right = 0xa3},
        {.left = 0xb6, .right = 0xa0},
        {.left = 0xb2, .right = 0x9c},
        {.left = 0xae, .right = 0x99},
        {.left = 0xaa, .right = 0x95},
        {.left = 0xa5, .right = 0x91},
        {.left = 0xa1, .right = 0x8d},
        {.left = 0x9d, .right = 0x8a},
        {.left = 0x99, .right = 0x86},
        {.left = 0x95, .right = 0x83},
        {.left = 0x91, .right = 0x7f},
        {.left = 0x8d, .right = 0x7c},
        {.left = 0x89, .right = 0x78},
        {.left = 0x85, .right = 0x75},
        {.left = 0x81, .right = 0x71},
        {.left = 0x7d, .right = 0x6e},
        {.left = 0x79, .right = 0x6a},
        {.left = 0x75, .right = 0x67},
        {.left = 0x71, .right = 0x63},
        {.left = 0x6d, .right = 0x60},
        {.left = 0x69, .right = 0x5c},
        {.left = 0x65, .right = 0x59},
        {.left = 0x61, .right = 0x55},
        {.left = 0x5d, .right = 0x52},
        {.left = 0x59, .right = 0x4e},
        {.left = 0x55, .right = 0x4b},
        {.left = 0x50, .right = 0x46},
        {.left = 0x4c, .right = 0x43},
        {.left = 0x48, .right = 0x3f},
        {.left = 0x44, .right = 0x3c},
        {.left = 0x40, .right = 0x38},
        {.left = 0x3c, .right = 0x35},
        {.left = 0x38, .right = 0x31},
        {.left = 0x34, .right = 0x2e},
        {.left = 0x30, .right = 0x2a},
        {.left = 0x2c, .right = 0x27},
        {.left = 0x28, .right = 0x23},
        {.left = 0x24, .right = 0x20},
        {.left = 0x20, .right = 0x1c},
        {.left = 0x1c, .right = 0x19},
        {.left = 0x18, .right = 0x15},
        {.left = 0x14, .right = 0x12},
        {.left = 0x10, .right = 0x0e},
        {.left = 0x0c, .right = 0x0b},
        {.left = 0x08, .right = 0x07},
        {.left = 0x04, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0xff, .right = 0xff},
        {.left = 0xfa, .right = 0xfa},
        {.left = 0xf6, .right = 0xf6},
        {.left = 0xf2, .right = 0xf2},
        {.left = 0xee, .right = 0xee},
        {.left = 0xea, .right = 0xea},
        {.left = 0xe6, .right = 0xe6},
        {.left = 0xe2, .right = 0xe2},
        {.left = 0xde, .right = 0xde},
        {.left = 0xda, .right = 0xda},
        {.left = 0xd6, .right = 0xd6},
        {.left = 0xd2, .right = 0xd2},
        {.left = 0xce, .right = 0xce},
        {.left = 0xca, .right = 0xca},
        {.left = 0xc6, .right = 0xc6},
        {.left = 0xc2, .right = 0xc2},
        {.left = 0xbe, .right = 0xbe},
        {.left = 0xba, .right = 0xba},
        {.left = 0xb6, .right = 0xb6},
        {.left = 0xb2, .right = 0xb2},
        {.left = 0xae, .right = 0xae},
        {.left = 0xaa, .right = 0xaa},
        {.left = 0xa5, .right = 0xa5},
        {.left = 0xa1, .right = 0xa1},
        {.left = 0x9d, .right = 0x9d},
        {.left = 0x99, .right = 0x99},
        {.left = 0x95, .right = 0x95},
        {.left = 0x91, .right = 0x91},
        {.left = 0x8d, .right = 0x8d},
        {.left = 0x89, .right = 0x89},
        {.left = 0x85, .right = 0x85},
        {.left = 0x81, .right = 0x81},
        {.left = 0x7d, .right = 0x7d},
        {.left = 0x79, .right = 0x79},
        {.left = 0x75, .right = 0x75},
        {.left = 0x71, .right = 0x71},
        {.left = 0x6d, .right = 0x6d},
        {.left = 0x69, .right = 0x69},
        {.left = 0x65, .right = 0x65},
        {.left = 0x61, .right = 0x61},
        {.left = 0x5d, .right = 0x5d},
        {.left = 0x59, .right = 0x59},
        {.left = 0x55, .right = 0x55},
        {.left = 0x50, .right = 0x50},
        {.left = 0x4c, .right = 0x4c},
        {.left = 0x48, .right = 0x48},
        {.left = 0x44, .right = 0x44},
        {.left = 0x40, .right = 0x40},
        {.left = 0x3c, .right = 0x3c},
        {.left = 0x38, .right = 0x38},
        {.left = 0x34, .right = 0x34},
        {.left = 0x30, .right = 0x30},
        {.left = 0x2c, .right = 0x2c},
        {.left = 0x28, .right = 0x28},
        {.left = 0x24, .right = 0x24},
        {.left = 0x20, .right = 0x20},
        {.left = 0x1c, .right = 0x1c},
        {.left = 0x18, .right = 0x18},
        {.left = 0x14, .right = 0x14},
        {.left = 0x10, .right = 0x10},
        {.left = 0x0c, .right = 0x0c},
        {.left = 0x08, .right = 0x08},
        {.left = 0x04, .right = 0x04},
        {.left = 0x00, .right = 0x00},
    },
    {
        {.left = 0x01, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x10},
        {.left = 0x00, .right = 0x00},
        {.left = 0x03, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x11, .right = 0x2b},
        {.left = 0x00, .right = 0x00},
        {.left = 0x0a, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x04},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x30, .right = 0x8c},
        {.left = 0x43, .right = 0x0a},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
        {.left = 0x00, .right = 0x00},
    },
};
#endif
#if WIN32
static int MV_Installed   = FALSE;
static int MV_SoundCard   = SC_Unknown;
#else
#define MV_Installed    TRUE
#endif
static int MV_TotalVolume = MV_MaxTotalVolume;
#if WIN32
static int MV_MaxVoices   = 1;
#endif
int MV_BufferSize = MixBufferSize;
static int MV_BufferLength;

static int MV_NumberOfBuffers = NumberOfBuffers;

#if USE_VOC
static int MV_Bits       = 8;
static int MV_MixMode    = MONO_8BIT;
int MV_Channels   = 1;

#else
#if !MG24
static const int MV_MixMode    = STEREO;
#endif

static const int MV_Bits       = 8;
const int MV_Channels   = 2;
#endif
static int MV_Silence    = SILENCE_8BIT;
static int MV_SwapLeftRight = FALSE;
#if WIN32
static int MV_RequestedMixRate;
#endif
int MV_MixRate;

static int MV_BuffShift;

//static int MV_TotalMemory;
//static int MV_FooMemory;
#if WIN32
static void* MV_BufferDescriptor;
static int   MV_BufferEmpty[ NumberOfBuffers ];
char *MV_MixBuffer[ NumberOfBuffers + 1 ];
MV_FLOAT *MV_FooBuffer = NULL;
#endif

#if VOICE_LINKED_LIST
static VoiceNode *MV_Voices = NULL;

static volatile VoiceNode VoiceList;
static volatile VoiceNode VoicePool;
#else
#if WIN32
volatile VoiceNode VoiceArray[MAX_VOICES];
#else
VoiceNode VoiceArray[MAX_VOICES];
#endif
#endif
/*static*/ int MV_MixPage      = 0;
//static int MV_VoiceHandle  = MV_MinVoiceHandle;
#if WIN32
static void ( *MV_CallBackFunc )( int32_t ) = NULL;
#endif
//static void ( *MV_RecordFunc )( char *ptr, int length ) = NULL;
//static void ( *MV_MixFunction )( VoiceNode *voice);

int *MV_GLast, *MV_GPos, *MV_GVal;

// char  *MV_HarshClipTable;
char  *MV_MixDestination;
int    MV_LeftVolume;
int    MV_RightVolume;
int    MV_SampleSize = 1;
int    MV_RightChannelOffset;

unsigned long MV_MixPosition;

int MV_ErrorCode = MV_Ok;

#define MV_SetErrorCode( status ) \
   MV_ErrorCode   = ( status );


/*---------------------------------------------------------------------
   Function: MV_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/
#if WIN32
char *MV_ErrorString
   (
   int ErrorNumber
   )

   {
   char *ErrorString;

   switch( ErrorNumber )
      {
      case MV_Warning :
      case MV_Error :
         ErrorString = MV_ErrorString( MV_ErrorCode );
         break;

      case MV_Ok :
         ErrorString = "Multivoc ok.\n";
         break;

      case MV_UnsupportedCard :
         ErrorString = "Selected sound card is not supported by Multivoc.\n";
         break;

      case MV_NotInstalled :
         ErrorString = "Multivoc not installed.\n";
         break;

      case MV_NoVoices :
         ErrorString = "No free voices available to Multivoc.\n";
         break;

      case MV_NoMem :
         ErrorString = "Out of memory in Multivoc.\n";
         break;

      case MV_VoiceNotFound :
         ErrorString = "No voice with matching handle found.\n";
         break;

      case MV_DPMI_Error :
         ErrorString = "DPMI Error in Multivoc.\n";
         break;

      case MV_InvalidVOCFile :
         ErrorString = "Invalid VOC file passed in to Multivoc.\n";
         break;

      case MV_InvalidWAVFile :
         ErrorString = "Invalid WAV file passed in to Multivoc.\n";
         break;

      case MV_InvalidMixMode :
         ErrorString = "Invalid mix mode request in Multivoc.\n";
         break;

      case MV_SoundSourceFailure :
         ErrorString = "Sound Source playback failed.\n";
         break;

      case MV_IrqFailure :
         ErrorString = "Playback failed, possibly due to an invalid or conflicting IRQ.\n";
         break;

      case MV_DMAFailure :
         ErrorString = "Playback failed, possibly due to an invalid or conflicting DMA channel.\n";
         break;

      case MV_DMA16Failure :
         ErrorString = "Playback failed, possibly due to an invalid or conflicting DMA channel.  \n"
                       "Make sure the 16-bit DMA channel is correct.\n";
         break;

      case MV_NullRecordFunction :
         ErrorString = "Null record function passed to MV_StartRecording.\n";
         break;

      default :
         ErrorString = "Unknown Multivoc error code.\n";
         break;
      }

   return( ErrorString );
   }
#endif
#if !USE_SOUNDOWNER
int getSoundOwnerFromSoundNum(int soundNum)
{
    for (int v = 0; v < MAX_VOICES; v++)
    {
        if (VoiceArray[v].Playing && VoiceArray[v].soundNum == soundNum)
        {
            return VoiceArray[v].soundOwner;
        }
    }
    return - 1;
}
int getVoiceSoundOwner(int voice)
{
    if (voice > 0 && voice <= MAX_VOICES)
    {
        voice--;
        return VoiceArray[voice].soundOwner;
    }
    else
    {
        return -1;
    }
}
int getVoiceSoundNum(int voice)
{
    if (voice > 0 && voice <= MAX_VOICES)
    {
        voice--;
        return VoiceArray[voice].soundNum;
    }
    return NUM_SOUNDS;
}
int getNumSoundForOwner(int i)
{
    // next-hack: this to emulate the following code in gamedef.c:
    //        for(j=1;j<NUM_SOUNDS;j++)
    //            if( SoundOwner[j][0].i == g_i )
    //                break;
    // this is not 100% equivalent. The code above depends on a priority given by where the sound appears in the array.
    // below is almost random, so we have to prevent early exit. Also we don't check for 0.
    int j = NUM_SOUNDS;
    for (int voice = 0; voice < MAX_VOICES; voice++)
    {
        if (VoiceArray[voice].Playing && VoiceArray[voice].soundOwner == i)
        {
            // find the smallest to simulate the early exit.
            if (j > VoiceArray[voice].soundNum)
            {
                j = VoiceArray[voice].soundNum;
            }
        }
    }
    return j;
}
void  setVoiceSoundOwner(int voice, int owner)
{
    if (voice > 0 && voice <= MAX_VOICES)
        VoiceArray[voice - 1].soundOwner = owner;
}

#endif

/**********************************************************************

   Memory locked functions:

**********************************************************************/


#define MV_LockStart MV_Mix


/*---------------------------------------------------------------------
   Function: MV_Mix

   Mixes the sound into the buffer.
---------------------------------------------------------------------*/
#if WIN32
#define MIX_SAMPLE_SIZE (sizeof(MV_FooBuffer[0]))
static void MV_Mix( VoiceNode *voice )
{
   uint8_t        *start;
   int            length;
   long           voclength;
   unsigned long  position;
   unsigned long  rate;
   unsigned long  FixedPointBufferSize;

#if USE_VOC
   if ( ( voice->length == 0 )
	   && ( voice->GetSound( voice ) != KeepPlaying ) )
      {
      return;
      }
#else
   if ( ( voice->length == 0 )
	   && ( MV_GetNextWAVBlock( voice ) != KeepPlaying ) )
      {
      return;
      }

#endif

   length               = MixBufferSize;
   FixedPointBufferSize = voice->FixedPointBufferSize;

   MV_MixDestination    = (char*)MV_FooBuffer;
   MV_LeftVolume        = voice->LeftVolume;
   MV_RightVolume       = voice->RightVolume;
   MV_GLast             = &voice->GLast;
   MV_GPos              = &voice->GPos;
   MV_GVal              = (int*)&voice->GVal;

#if USE_VOC
   if ( ( MV_Channels == 2 ) && ( IS_QUIET( MV_LeftVolume ) ) )
      {
      MV_LeftVolume      = MV_RightVolume;
      MV_MixDestination += MIX_SAMPLE_SIZE;
      }
#endif
   // Add this voice to the mix
   while( length > 0 )
      {
      start    = voice->sound;
      rate     = voice->RateScale;
      position = voice->position;

      // Check if the last sample in this buffer would be
      // beyond the length of the sample block
      if ( ( position + FixedPointBufferSize ) >= voice->length )
         {
         if ( position < voice->length )
            {
            voclength = ( voice->length - position + rate - 1 ) / rate;
            }
         else
            {
#if USE_VOC
            voice->GetSound( voice );
#else
            MV_GetNextWAVBlock(voice);
#endif
            return;
            }
         }
      else
         {
         voclength = length;
         }
#if USE_VOC
      voice->mix( position, rate, start, voclength );
#else
      MV_MixFPStereo8(position, rate, start, voclength);
#endif
      if ( voclength & 1 )
         {
         MV_MixPosition += rate;
         voclength -= 1;
         }
      voice->position = MV_MixPosition;

      length -= voclength;

      if ( voice->position >= voice->length )
         {
         // Get the next block of sound
#if USE_VOC
         if ( voice->GetSound( voice ) != KeepPlaying )
#else
        if (MV_GetNextWAVBlock(voice) != KeepPlaying)
#endif
            {
            return;
            }

         if ( length > 0 )
            {
            // Get the position of the last sample in the buffer
            FixedPointBufferSize = voice->RateScale * ( length - 1 );
            }
         }
      }
}
#endif

/*---------------------------------------------------------------------
   Function: MV_PlayVoice

   Adds a voice to the play list.
---------------------------------------------------------------------*/

void MV_PlayVoice( VoiceNode *voice )
{
#if WIN32
#if VOICE_LINKED_LIST
   LL_SortedInsertion( &VoiceList, voice, prev, next, VoiceNode, priority );
#endif
   ++sounddebugActiveSounds;
   ++sounddebugAllocateSoundCalls;
#else
   voice->lastAudioBufferIdx = -1;
   voice->position = 0;
#endif
}


/*---------------------------------------------------------------------
   Function: MV_StopVoice

   Removes the voice from the play list and adds it to the free list.
---------------------------------------------------------------------*/

void MV_StopVoice( VoiceNode *voice )
{
#if VOICE_LINKED_LIST
   VoiceNode* pPrev;
   VoiceNode* pNext;

   pPrev = voice->prev;
   pNext = voice->next;

   // move the voice from the play list to the free list
   LL_Remove( voice, next, prev );
   LL_Add( &VoicePool, voice, next, prev );

   if(pPrev == NULL)
   {
	   printf("(MV_StopVoice) pPrev is NULL, this could be a problem.\n");
   }

   if(pNext == NULL)
   {
	   printf("(MV_StopVoice) pNext is NULL, this could be a problem.\n");
   }
#else
    voice->Playing = 0;
#endif

   --sounddebugActiveSounds;
   ++sounddebugDeallocateSoundCalls;
}


/*---------------------------------------------------------------------
   Function: MV_ServiceVoc

   Starts playback of the waiting buffer and mixes the next one.
---------------------------------------------------------------------*/
#if WIN32
void MV_ServiceVoc
(
 void
 )

{
	VoiceNode *voice;


	// Toggle which buffer we'll mix next
	MV_MixPage++;
	if ( MV_MixPage >= MV_NumberOfBuffers )
	{
		MV_MixPage -= MV_NumberOfBuffers;
	}

	{
		ClearBuffer_DW( MV_FooBuffer, 0, sizeof(MV_FLOAT) / 4 * MV_BufferSize / MV_SampleSize * MV_Channels);
		MV_BufferEmpty[ MV_MixPage ] = TRUE;
	}
	#if WIN32
    int numvoicesmixed = 0;
    #endif
	// Play any waiting voices
#if VOICE_LINKED_LIST
	VoiceNode *next;
	for( voice = VoiceList.next; voice != &VoiceList; voice = next )
#else
    for (int voicenum = 0; voicenum < MAX_VOICES; voicenum++)
#endif
	{
#if !VOICE_LINKED_LIST
        voice =  (VoiceNode *) &VoiceArray[voicenum];
        if (!voice->Playing)
        {

            continue;
        }
#endif
#if WIN32
	    numvoicesmixed++;
#endif
		//      if ( ( voice < &MV_Voices[ 0 ] ) || ( voice > &MV_Voices[ 8 ] ) )
		//         {
		//         SetBorderColor(backcolor++);
		//         break;
		//         }
#if USE_VOC
		if(NULL == voice->GetSound)
		{
			#ifdef _DEBUG
				printf("MV_ServiceVoc() voice->GetSound == NULL, break;\n");
			#endif

			// This sound is null, early out, or face a nasty crash.
			break;
		}
#endif
		MV_BufferEmpty[ MV_MixPage ] = FALSE;

//		MV_MixFunction( voice ); // next-hack: this is stupid.
        MV_Mix( voice );
#if VOICE_LINKED_LIST
		next = voice->next;
#endif
		// Is this voice done?
		if ( !voice->Playing )
		{
			MV_StopVoice( voice );
#if WIN32
			if ( MV_CallBackFunc )
            {
				MV_CallBackFunc( voice->soundNum );
            }
#else
		TestCallBack(voice->soundNum);
#endif            
		}
	}
	#if WIN32 & 0
    printf("Number of voices mixed: %d\r\n", numvoicesmixed);
    #endif
	if ( MV_ReverbLevel > 0)
	{
//	    printf("REVERB LEVEL>0\r\n");
		if (MV_ReverbTable != -1) MV_FPReverb(MV_ReverbTable);
	}

	{
		char *dest;
		int   count;

		dest = MV_MixBuffer[ MV_MixPage ];
		count = MV_BufferSize / MV_SampleSize * MV_Channels;
#if USE_VOC
		if ( MV_Bits == 16 )
		{
			MV_16BitDownmix(dest, count);
		}
		else
#endif
		{
			MV_8BitDownmix( (uint8_t*) dest, count);
		}

	}
}
#endif
/*---------------------------------------------------------------------
   Function: MV_GetNextVOCBlock

   Interperate the information of a VOC format sound file.
---------------------------------------------------------------------*/
#if USE_VOC
playbackstatus MV_GetNextVOCBlock
   (
   VoiceNode *voice
   )

   {
   unsigned char *ptr;
   int            blocktype;
   int            lastblocktype;
   uint32_t       blocklength;
   unsigned long  samplespeed;
   unsigned int   tc;
   int            packtype;
   int            voicemode;
   int            done;
   unsigned       BitsPerSample;
   unsigned       Channels;
   unsigned       Format;

   if ( voice->BlockLength > 0 )
      {
      voice->position    -= voice->length;
      voice->sound       += voice->length >> 16;
      if ( voice->bits == 16 )
         {
         voice->sound += voice->length >> 16;
         }
      voice->length       = min( voice->BlockLength, 0x8000 );
      voice->BlockLength -= voice->length;
      voice->length     <<= 16;
      return( KeepPlaying );
      }

   if ( ( voice->length > 0 ) && ( voice->LoopEnd != NULL ) &&
      ( voice->LoopStart != NULL ) )
      {
      voice->BlockLength  = voice->LoopSize;
      voice->sound        = voice->LoopStart;
      voice->position     = 0;
      voice->length       = min( voice->BlockLength, 0x8000 );
      voice->BlockLength -= voice->length;
      voice->length     <<= 16;
      return( KeepPlaying );
      }

   ptr = ( unsigned char * )voice->NextBlock;

   voice->Playing = TRUE;

   voicemode = 0;
   lastblocktype = 0;
   packtype = 0;

   done = FALSE;
   while( !done )
      {
      // Stop playing if we get a NULL pointer
      if ( ptr == NULL )
         {
         voice->Playing = FALSE;
         done = TRUE;
         break;
         }

      blocktype = ( int )*ptr;
      blocklength = ( *( unsigned long * )( ptr + 1 ) ) & 0x00ffffff;
      ptr += 4;

      switch( blocktype )
         {
         case 0 :
            // End of data
            if ( ( voice->LoopStart == NULL ) ||
               ( voice->LoopStart >= ( ptr - 4 ) ) )
               {
               voice->Playing = FALSE;
               done = TRUE;
               }
            else
               {
               voice->BlockLength  = ( ptr - 4 ) - (unsigned char *)voice->LoopStart;
               voice->sound        = voice->LoopStart;
               voice->position     = 0;
               voice->length       = min( voice->BlockLength, 0x8000 );
               voice->BlockLength -= voice->length;
               voice->length     <<= 16;
               return( KeepPlaying );
               }
            break;

         case 1 :
            // Sound data block
            voice->bits  = 8;
            if ( lastblocktype != 8 )
               {
               tc = ( unsigned int )*ptr << 8;
               packtype = *( ptr + 1 );
               }

            ptr += 2;
            blocklength -= 2;

            samplespeed = 256000000L / ( 65536 - tc );

            // Skip packed or stereo data
            if ( ( packtype != 0 ) || ( voicemode != 0 ) )
               {
               ptr += blocklength;
               }
            else
               {
               done = TRUE;
               }
            voicemode = 0;
            break;

         case 2 :
            // Sound continuation block
            samplespeed = voice->SamplingRate;
            done = TRUE;
            break;

         case 3 :
            // Silence
            // Not implimented.
            ptr += blocklength;
            break;

         case 4 :
            // Marker
            // Not implimented.
            ptr += blocklength;
            break;

         case 5 :
            // ASCII string
            // Not implimented.
            ptr += blocklength;
            break;

         case 6 :
            FIXME("ERROR");

            // Repeat begin
            if ( voice->LoopEnd == NULL )
               {
               voice->LoopCount = *( uint16_t * )ptr;
               voice->LoopStart = ptr + blocklength;
               }
            ptr += blocklength;
            break;

         case 7 :
            FIXME("ERROR");

            // Repeat end
            ptr += blocklength;
            if ( lastblocktype == 6 )
               {
               voice->LoopCount = 0;
               }
            else
               {
               if ( ( voice->LoopCount > 0 ) && ( voice->LoopStart != NULL ) )
                  {
                  ptr = voice->LoopStart;
                  if ( voice->LoopCount < 0xffff )
                     {
                     voice->LoopCount--;
                     if ( voice->LoopCount == 0 )
                        {
                        voice->LoopStart = NULL;
                        }
                     }
                  }
               }
            break;

         case 8 :
            FIXME("ERROR");

// next-hack: never reached.
            // Extended block
            voice->bits  = 8;
            tc = *( uint16_t * )ptr;
            packtype = *( ptr + 2 );
            voicemode = *( ptr + 3 );
            ptr += blocklength;
            break;

         case 9 :
            // New sound data block

            samplespeed = *( unsigned long * )ptr;
            BitsPerSample = ( unsigned )*( ptr + 4 );
            Channels = ( unsigned )*( ptr + 5 );
            Format = ( unsigned )*( uint16_t * )( ptr + 6 );

            if ( ( BitsPerSample == 8 ) && ( Channels == 1 ) &&
               ( Format == VOC_8BIT ) )
               {
               ptr         += 12;
               blocklength -= 12;
               voice->bits  = 8;
               done         = TRUE;
               }
            else if ( ( BitsPerSample == 16 ) && ( Channels == 1 ) &&
               ( Format == VOC_16BIT ) )
               {
               ptr         += 12;
               blocklength -= 12;
               voice->bits  = 16;
               done         = TRUE;
               }
            else
               {
               ptr += blocklength;
               }
            break;

         default :
            // Unknown data.  Probably not a VOC file.
            voice->Playing = FALSE;
            done = TRUE;
            break;
         }

      lastblocktype = blocktype;
      }

   if ( voice->Playing )
      {
      voice->NextBlock    = ptr + blocklength;
      voice->sound        = ptr;

      voice->SamplingRate = samplespeed;
      voice->RateScale    = ( voice->SamplingRate * voice->PitchScale ) / MV_MixRate;

      // Multiply by MixBufferSize - 1
      voice->FixedPointBufferSize = ( voice->RateScale * MixBufferSize ) -
         voice->RateScale;

      if ( voice->LoopEnd != NULL )
         {
         if ( blocklength > ( unsigned long )voice->LoopEnd )
            {
            blocklength = ( unsigned long )voice->LoopEnd;
            }
         else
            {
            voice->LoopEnd = ( char * )blocklength;
            }

         voice->LoopStart = voice->sound + ( unsigned long )voice->LoopStart;
         voice->LoopEnd   = voice->sound + ( unsigned long )voice->LoopEnd;
         voice->LoopSize  = voice->LoopEnd - voice->LoopStart;
         }

      if ( voice->bits == 16 )
         {
         blocklength /= 2;
         }

      voice->position     = 0;
      voice->length       = min( blocklength, 0x8000 );
      voice->BlockLength  = blocklength - voice->length;
      voice->length     <<= 16;

      MV_SetVoiceMixMode( voice );

      return( KeepPlaying );
      }

   return( NoMoreData );
   }

#endif
/*---------------------------------------------------------------------
   Function: MV_GetNextDemandFeedBlock

   Controls playback of demand fed data.
---------------------------------------------------------------------*/
#if 0
playbackstatus MV_GetNextDemandFeedBlock
   (
   VoiceNode *voice
   )

   {
   if ( voice->BlockLength > 0 )
      {
      voice->position    -= voice->length;
      voice->sound       += voice->length >> 16;
      voice->length       = min( voice->BlockLength, 0x8000 );
      voice->BlockLength -= voice->length;
      voice->length     <<= 16;

      return( KeepPlaying );
      }

   if ( voice->DemandFeed == NULL )
      {
      return( NoMoreData );
      }

   voice->position     = 0;
   ( voice->DemandFeed )( &voice->sound, &voice->BlockLength );
   voice->length       = min( voice->BlockLength, 0x8000 );
   voice->BlockLength -= voice->length;
   voice->length     <<= 16;

   if ( ( voice->length > 0 ) && ( voice->sound != NULL ) )
      {
      return( KeepPlaying );
      }
   return( NoMoreData );
   }
#endif

/*---------------------------------------------------------------------
   Function: MV_GetNextRawBlock

   Controls playback of demand fed data.
---------------------------------------------------------------------*/
#if 0
playbackstatus MV_GetNextRawBlock
   (
   VoiceNode *voice
   )

   {
   if ( voice->BlockLength <= 0 )
      {
      if ( voice->LoopStart == NULL )
         {
         voice->Playing = FALSE;
         return( NoMoreData );
         }

      voice->BlockLength = voice->LoopSize;
      voice->NextBlock   = voice->LoopStart;
      voice->length = 0;
      voice->position = 0;
      }

   voice->sound        = voice->NextBlock;
   voice->position    -= voice->length;
   voice->length       = min( voice->BlockLength, 0x8000 );
   voice->NextBlock   += voice->length;
   if ( voice->bits == 16 )
      {
      voice->NextBlock += voice->length;
      }
   voice->BlockLength -= voice->length;
   voice->length     <<= 16;

   return( KeepPlaying );
   }
#endif

/*---------------------------------------------------------------------
   Function: MV_GetNextWAVBlock

   Controls playback of demand fed data.
---------------------------------------------------------------------*/
#if WIN32
playbackstatus MV_GetNextWAVBlock
   (
   VoiceNode *voice
   )

   {
   if ( voice->BlockLength <= 0 )
      {
      if ( voice->LoopStart == NULL )
         {
         voice->Playing = FALSE;
         return( NoMoreData );
         }

      voice->BlockLength = voice->LoopSize;
      voice->NextBlock   = voice->LoopStart;
      voice->length      = 0;
      voice->position    = 0;
      }

   voice->sound        = voice->NextBlock;
   voice->position    -= voice->length;
   voice->length       = min( voice->BlockLength, 0x8000 );
   voice->NextBlock   += voice->length;
#if USE_VOC
   if ( voice->bits == 16 )
      {
      voice->NextBlock += voice->length;
      }
#endif
   voice->BlockLength -= voice->length;
   voice->length     <<= 16;

   return( KeepPlaying );
   }

#endif
/*---------------------------------------------------------------------
   Function: MV_GetVoice

   Locates the voice with the specified handle.
---------------------------------------------------------------------*/

VoiceNode *MV_GetVoice
   (
   int handle
   )

   {
#if VOICE_LINKED_LIST
   VoiceNode *voice;

   for( voice = VoiceList.next; voice != &VoiceList; voice = voice->next )
      {
      if ( handle == voice->handle )
         {
         break;
         }
      }

   if ( voice == &VoiceList )
      {
      MV_SetErrorCode( MV_VoiceNotFound );

      // SBF - should this return null?
      return NULL;
      }

   return( voice );
#else
    handle--;
    if (handle < 0 || handle >= MAX_VOICES)
    {
          MV_SetErrorCode( MV_VoiceNotFound );
        return NULL;
    }
    else
    {
        return (VoiceNode *) &VoiceArray[handle];
   }
#endif
}


/*---------------------------------------------------------------------
   Function: MV_VoicePlaying

   Checks if the voice associated with the specified handle is
   playing.
---------------------------------------------------------------------*/

int MV_VoicePlaying
   (
   int handle
   )

   {
   VoiceNode *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( FALSE );
      }

   voice = MV_GetVoice( handle );

   if ( voice == NULL )
      {
      return( FALSE );
      }

   return( voice->Playing );
   }


/*---------------------------------------------------------------------
   Function: MV_KillAllVoices

   Stops output of all currently active voices.
---------------------------------------------------------------------*/

int MV_KillAllVoices(void)

{
   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }
#if VOICE_LINKED_LIST
   // Remove all the voices from the list
   while( VoiceList.next != &VoiceList )
      {
      MV_Kill( VoiceList.next->handle );
      }
#else
    for (int i = 0; i < MAX_VOICES; i++)
    {
        if (VoiceArray[i].Playing)
        {
            MV_Kill(i + 1);
        }
    }
#endif

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_Kill

   Stops output of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int MV_Kill(int handle)

   {
   VoiceNode *voice;
   unsigned  long callbackval;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   voice = MV_GetVoice( handle );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Error );
      }

   callbackval = voice->soundNum;

   MV_StopVoice( voice );
#if WIN32
   if ( MV_CallBackFunc )
      {
      MV_CallBackFunc( callbackval );
      }
#else
   TestCallBack(callbackval);
#endif
   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_VoicesPlaying

   Determines the number of currently active voices.
---------------------------------------------------------------------*/
#if 0
int MV_VoicesPlaying
   (
   void
   )

   {
   VoiceNode   *voice;
   int         NumVoices = 0;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( 0 );
      }

   for( voice = VoiceList.next; voice != &VoiceList; voice = voice->next )
      {
      NumVoices++;
      }

   return( NumVoices );
   }

#endif
/*---------------------------------------------------------------------
   Function: MV_AllocVoice

   Retrieve an inactive or lower priority voice for output.
---------------------------------------------------------------------*/

VoiceNode *MV_AllocVoice
   (
   int priority
   )

   {
#if VOICE_LINKED_LIST
   VoiceNode   *voice;
   VoiceNode   *node;

   // Check if we have any free voices
   if ( LL_Empty( &VoicePool, next, prev ) )
      {
      // check if we have a higher priority than a voice that is playing.
      voice = VoiceList.next;
      for( node = voice->next; node != &VoiceList; node = node->next )
         {
         if ( node->priority < voice->priority )
            {
            voice = node;
            }
         }

      if ( priority >= voice->priority )
         {
         MV_Kill( voice->handle );
         }
      }

   // Check if any voices are in the voice pool
   if ( LL_Empty( &VoicePool, next, prev ) )
      {
      // No free voices
      return( NULL );
      }

   voice = VoicePool.next;
   LL_Remove( voice, next, prev );

   // Find a free voice handle
   #if 0
   do
      {
      MV_VoiceHandle++;
      if ( MV_VoiceHandle < MV_MinVoiceHandle )
         {
         MV_VoiceHandle = MV_MinVoiceHandle;
         }
      }
   while( MV_VoicePlaying( MV_VoiceHandle ) );
#else
    for (MV_VoiceHandle = MV_MinVoiceHandle; MV_VoiceHandle < MAX_VOICES; MV_VoiceHandle++)
    {
        if (!MV_VoicePlaying( MV_VoiceHandle ))
            break;
    }
#endif
   voice->handle = MV_VoiceHandle;
    printf("MV_VoiceHandle is %d\r\n", MV_VoiceHandle);
   return( voice );
#else
    // first check for free voices
    for (int i = 0; i < MAX_VOICES; i++)
    {
        if (VoiceArray[i].Playing == 0)
        {
            VoiceArray[i].Playing = TRUE;
            return (VoiceNode *) &VoiceArray[i];
        }
    }
    // not found. Let's find the lowest prio
    int minIndex = 0;
    for (int i = 1; i < MAX_VOICES; i++)
    {
        if (VoiceArray[i].priority < VoiceArray[minIndex].priority)
        {
            minIndex = i;
        }
    }
    // check if the lowest priority is below our prio
    if (VoiceArray[minIndex].priority <= priority)
    {
        MV_Kill( minIndex + 1 );
        return (VoiceNode *) &VoiceArray[minIndex]; // note, this is for sure playing.
    }
    else
    {
        return NULL;
   }
#endif
}


/*---------------------------------------------------------------------
   Function: MV_VoiceAvailable

   Checks if a voice can be play at the specified priority.
---------------------------------------------------------------------*/

int MV_VoiceAvailable(int priority)

   {
#if VOICE_LINKED_LIST
   VoiceNode   *voice;
   VoiceNode   *node;

//   printf("Voice pool%p, %d\r\n", VoicePool.next, VoicePool.prev);
   // Check if we have any free voices
   if ( !LL_Empty( &VoicePool, next, prev ) )
      {
          printf("VOICE AVAILABLE");
      return( TRUE );
      }

   // check if we have a higher priority than a voice that is playing.
   voice = VoiceList.next;
  printf("Voice: %p next %p,  prev %d\r\n", voice, voice->next, voice->prev);

   for( node = VoiceList.next; node != &VoiceList; node = node->next )
      {
      if ( node->priority < voice->priority )
         {
         voice = node;
         }
      }

   if ( ( voice != &VoiceList ) && ( priority >= voice->priority ) )
      {
          printf("Voice available\r\n");
      return( TRUE );
      }
          printf("Voice NOT available\r\n");

   return( FALSE );
#else
    for (int i = 0; i < MAX_VOICES; i++)
    {
        if (VoiceArray[i].Playing == 0 || VoiceArray[i].priority <= priority)
        {
            return TRUE;
        }
   }
    return FALSE;
#endif
}


/*---------------------------------------------------------------------
   Function: MV_SetVoicePitch

   Sets the pitch for the specified voice.
---------------------------------------------------------------------*/

void MV_SetVoicePitch
   (
   VoiceNode *voice,
   unsigned long rate,
   int pitchoffset
   )

   {
#if WIN32
       voice->SamplingRate = rate;
       voice->PitchScale   = PITCH_GetScale( pitchoffset );   // if pitchoffset is 0, pitchscale is 65536
       voice->RateScale    = ( rate * voice->PitchScale ) / MV_MixRate;

       // Multiply by MixBufferSize - 1
       voice->FixedPointBufferSize = ( voice->RateScale * (MixBufferSize - 1));
#else
      float scale = PITCH_GetScale( pitchoffset ) * (1.0f / 65536.0f);
      voice->mixerToSampleRateMult = scale  * rate * (1.0f / AUDIO_SAMPLE_RATE);
#endif
   }


/*---------------------------------------------------------------------
   Function: MV_GetVolumeTable

   Returns a pointer to the volume table associated with the specified
   volume.
---------------------------------------------------------------------*/

#if 0
static short *MV_GetVolumeTable
   (
   int vol
   )

   {
   int volume;
   short *table;

   volume = MIX_VOLUME( vol );

   table = &MV_VolumeTable[ volume ];

   return( table );
   }
#else
static int MV_GetVolumeTable
   (
   int vol
   )

   {
   int volume;

   volume = MIX_VOLUME( vol );

   return ( volume * MV_TotalVolume ) / MV_MaxTotalVolume;
   }
#endif

/*---------------------------------------------------------------------
   Function: MV_SetVoiceMixMode

   Selects which method should be used to mix the voice.
---------------------------------------------------------------------*/
#if USE_VOC
void MV_SetVoiceMixMode
   (
   VoiceNode *voice
   )

   {
   int test;

   test = T_DEFAULT;
#if USE_VOC
   if ( voice->bits == 16 )
      {
      test |= T_16BITSOURCE;
      }
#endif
   if ( MV_Channels == 1 )
      {
      test |= T_MONO;
      }
   else
      {
      if ( IS_QUIET( voice->RightVolume ) )
         {
         test |= T_RIGHTQUIET;
         }
      else if ( IS_QUIET( voice->LeftVolume ) )
         {
         test |= T_LEFTQUIET;
         }
      }

   switch( test )
      {
#if USE_VOC
      case T_MONO | T_16BITSOURCE :
         voice->mix = MV_MixFPMono16;
         break;

      case T_MONO :
         voice->mix = MV_MixFPMono8;
         break;

      case T_16BITSOURCE | T_LEFTQUIET :
         MV_LeftVolume = MV_RightVolume;
         voice->mix = MV_MixFPMono16;
         break;

      case T_16BITSOURCE | T_RIGHTQUIET :
         voice->mix = MV_MixFPMono16;
         break;
      case T_16BITSOURCE :
         voice->mix = MV_MixFPStereo16;
         break;

#endif
      case T_LEFTQUIET :
         MV_LeftVolume = MV_RightVolume;
         voice->mix = MV_MixFPMono8;
         break;


      case T_RIGHTQUIET :
         voice->mix = MV_MixFPMono8;
         break;


      case T_SIXTEENBIT_STEREO :
         voice->mix = MV_MixFPStereo8;
         break;

//#if USE_VOC
      default :
         voice->mix = MV_MixFPMono8;
//#endif
      }
   }
#endif

/*---------------------------------------------------------------------
   Function: MV_SetVoiceVolume

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

void MV_SetVoiceVolume
   (
   VoiceNode *voice,
   int vol,
   int left,
   int right
   )

   {
   if ( MV_Channels == 1 )
      {
      left  = vol;
      right = vol;
      }

   if ( MV_SwapLeftRight )
      {
      // SBPro uses reversed panning
      voice->LeftVolume  = MV_GetVolumeTable( right );
      voice->RightVolume = MV_GetVolumeTable( left );
      }
   else
      {
      voice->LeftVolume  = MV_GetVolumeTable( left );
      voice->RightVolume = MV_GetVolumeTable( right );
      }
#if USE_VOC
   MV_SetVoiceMixMode( voice );
#endif
   }


/*---------------------------------------------------------------------
   Function: MV_SetPan

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int MV_SetPan
   (
   int handle,
   int vol,
   int left,
   int right
   )

   {
   VoiceNode *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   voice = MV_GetVoice( handle );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Warning );
      }

   MV_SetVoiceVolume( voice, vol, left, right );

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_Pan3D

   Set the angle and distance from the listener of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int MV_Pan3D
   (
   int handle,
   int angle,
   int distance
   )

   {
   int left;
   int right;
   int mid;
   int volume;
   int status;

   if ( distance < 0 )
      {
      distance  = -distance;
      angle    += MV_NumPanPositions / 2;
      }

   volume = MIX_VOLUME( distance );

   // Ensure angle is within 0 - 31
   angle &= MV_MaxPanPosition;

   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );

   status = MV_SetPan( handle, mid, left, right );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_SetReverb

   Sets the level of reverb to add to mix.
---------------------------------------------------------------------*/

void MV_SetReverb
   (
   int reverb
   )

   {
   MV_ReverbLevel = MIX_VOLUME( reverb );
   MV_ReverbTable = ( MV_ReverbLevel * MV_TotalVolume ) / MV_MaxTotalVolume;// &MV_VolumeTable[ MV_ReverbLevel ];
#if WIN32
   if (!reverb) MV_FPReverbFree();
#else
   if (!reverb)
   {
       MV_ReverbClear();
   }
#endif
   }


/*---------------------------------------------------------------------
   Function: MV_GetMaxReverbDelay

   Returns the maximum delay time for reverb.
---------------------------------------------------------------------*/

int MV_GetMaxReverbDelay
   (
   void
   )

   {
   int maxdelay;

   maxdelay = 65536; //MixBufferSize * MV_NumberOfBuffers;

   return maxdelay;
   }


/*---------------------------------------------------------------------
   Function: MV_SetReverbDelay

   Sets the delay level of reverb to add to mix.
---------------------------------------------------------------------*/

void MV_SetReverbDelay
   (
   int delay
   )

   {
   int maxdelay;

   if (!delay) // blah, ignore
      return;
   maxdelay = MV_GetMaxReverbDelay();
   MV_ReverbDelay = max( MixBufferSize, min( delay, maxdelay ) );
   // MV_ReverbDelay *= MV_SampleSize;
   }


/*---------------------------------------------------------------------
   Function: MV_SetMixMode

   Prepares Multivoc to play stereo of mono digitized sounds.
---------------------------------------------------------------------*/

int MV_SetMixMode(int numchannels, int samplebits)
   {
    UNUSED(numchannels);
    UNUSED(samplebits);

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }
#if USE_VOC
    int   mode = 0;
   if ( numchannels == 2 )
      {
      mode |= STEREO;
      }
   if ( samplebits == 16 )
      {
      mode |= SIXTEEN_BIT;
      }

   MV_MixMode = mode;

   MV_Channels = 1;
   if ( MV_MixMode & STEREO )
      {
      MV_Channels = 2;
      }

   MV_Bits = 8;
   if ( MV_MixMode & SIXTEEN_BIT )
      {
      MV_Bits = 16;
      }
#endif
   MV_BuffShift  = 7 + MV_Channels;
   MV_SampleSize = sizeof( MONO8 ) * MV_Channels;

   if ( MV_Bits == 8 )
      {
      MV_Silence = SILENCE_8BIT;
      }
#if USE_VOC
   else
      {
      MV_Silence     = SILENCE_16BIT;
      MV_BuffShift  += 1;
      MV_SampleSize *= 2;
      }
#endif
   MV_BufferSize = MixBufferSize * MV_SampleSize;
   MV_NumberOfBuffers = TotalBufferSize / MV_BufferSize;
   MV_BufferLength = TotalBufferSize;

   MV_RightChannelOffset = MV_SampleSize / 2;

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_StartPlayback

   Starts the sound playback engine.
---------------------------------------------------------------------*/
#if WIN32
int MV_StartPlayback()

   {

   int status;
   int buffer;

   // Initialize the buffers
   ClearBuffer_DW( MV_MixBuffer[ 0 ], MV_Silence, TotalBufferSize >> 2 );
   for( buffer = 0; buffer < MV_NumberOfBuffers; buffer++ )
      {
      MV_BufferEmpty[ buffer ] = TRUE;
      }

   // Set the mix buffer variables
   MV_MixPage = 1;

//   MV_MixFunction = MV_Mix; // next-hack: why wasting this?!?

//JIM
//   MV_MixRate = MV_RequestedMixRate;
//   return( MV_Ok );

   // Start playback
    status = DSL_BeginBufferedPlayback( MV_MixBuffer[ 0 ],
										TotalBufferSize,
										MV_NumberOfBuffers,
										MV_RequestedMixRate,
										MV_MixMode,
										MV_ServiceVoc );

    if ( status != DSL_Ok )
    {
		MV_SetErrorCode( MV_BlasterError );
		return( MV_Error );
    }

    MV_MixRate = DSL_GetPlaybackRate();

   return( MV_Ok );
   }
#endif

/*---------------------------------------------------------------------
   Function: MV_StopPlayback

   Stops the sound playback engine.
---------------------------------------------------------------------*/

void MV_StopPlayback
   (
   void
   )

   {
   VoiceNode   *voice;

   // Make sure all callbacks are done.
#if VOICE_LINKED_LIST

   DSL_StopPlayback();

   VoiceNode   *next;
   for( voice = VoiceList.next; voice != &VoiceList; voice = next )
      {
      next = voice->next;
#else
    for (int i = 0; i < MAX_VOICES; i++)
    {
        voice = (VoiceNode*) &VoiceArray[i];
        if (voice->Playing == 0)
        {
            continue;
        }
#endif
      MV_StopVoice( voice );
#if WIN32
      if ( MV_CallBackFunc )
         {
         MV_CallBackFunc( voice->soundNum );
         }
#else
    TestCallBack(voice->soundNum);
#endif
      }
   }


/*---------------------------------------------------------------------
   Function: MV_PlayWAV

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int MV_PlayWAV
   (
   uint8_t *ptr,
   int   pitchoffset,
   int   vol,
   int   left,
   int   right,
   int   priority,
   unsigned long soundNum
   )

   {
   int status;

   status = MV_PlayLoopedWAV( ptr, -1, -1, pitchoffset, vol, left, right,
      priority, soundNum );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayWAV3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int MV_PlayWAV3D
   (
   uint8_t *ptr,
   int  pitchoffset,
   int  angle,
   int  distance,
   int  priority,
   unsigned long soundNum
   )

   {
   int left;
   int right;
   int mid;
   int volume;
   int status;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   if ( distance < 0 )
      {
      distance  = -distance;
      angle    += MV_NumPanPositions / 2;
      }

   volume = MIX_VOLUME( distance );

   // Ensure angle is within 0 - 31
   angle &= MV_MaxPanPosition;

   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );

   status = MV_PlayWAV( ptr, pitchoffset, mid, left, right, priority,
      soundNum );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayLoopedWAV

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int MV_PlayLoopedWAV
   (
   uint8_t *ptr,
   long  loopstart,
   long  loopend,
   int   pitchoffset,
   int   vol,
   int   left,
   int   right,
   int   priority,
   unsigned long soundNum
   )

   {
   riff_header   riff;
   format_header format;
   data_header   data;
   VoiceNode     *voice;
   int length;
   int absloopend;
#if WIN32
   int absloopstart;
#endif
   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   //riff = ( riff_header * )ptr;
   extMemGetDataFromAddress(&riff, ptr, sizeof(riff));
#if WIN32
   if ( ( strncmp( riff.RIFF, "RIFF", 4 ) != 0 ) ||
      ( strncmp( riff.WAVE, "WAVE", 4 ) != 0 ) ||
      ( strncmp( riff.fmt, "fmt ", 4) != 0 ) )
      {
      MV_SetErrorCode( MV_InvalidWAVFile );
      return( MV_Error );
      }
#endif
   //   format = ( format_header * )( riff + 1 );
   extMemGetDataFromAddress(&format, ptr + sizeof(riff), sizeof(format));
   //  data   = ( data_header * )( ( ( char * )format ) + riff->format_size );
   extMemGetDataFromAddress(&data, ptr + sizeof(riff) + sizeof(format), sizeof(data));


   // Check if it's PCM data.
   if ( format.wFormatTag != 1 )
      {
      MV_SetErrorCode( MV_InvalidWAVFile );
      return( MV_Error );
      }

   if ( format.nChannels != 1 )
      {
      MV_SetErrorCode( MV_InvalidWAVFile );
      return( MV_Error );
      }

   if ( ( format.nBitsPerSample != 8 ) &&
      ( format.nBitsPerSample != 16 ) )
      {
      MV_SetErrorCode( MV_InvalidWAVFile );
      return( MV_Error );
      }
#if WIN32
   if ( strncmp( data.DATA, "data", 4 ) != 0 )
      {
      MV_SetErrorCode( MV_InvalidWAVFile );
      return( MV_Error );
      }
#endif
   // Request a voice from the voice pool
   voice = MV_AllocVoice( priority );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_NoVoices );
      return( MV_Error );
      }
#if USE_VOC
   voice->wavetype    = WAV;
   voice->GetSound    = MV_GetNextWAVBlock;
   voice->bits        = format->nBitsPerSample;

#endif

   length = data.size;
#if WIN32
   absloopstart = loopstart;
#endif
   absloopend   = loopend;
#if USE_VOC
   if ( voice->bits == 16 )
      {
      loopstart  *= 2;
      data->size &= ~1;
      loopend    *= 2;
      length     /= 2;
      }
#endif
   loopend    = min( loopend, (int32_t) data.size );
   absloopend = min( absloopend, length );

   voice->Playing     = TRUE;
#if 0
   voice->DemandFeed  = NULL;
#endif
#if WIN32
   voice->LoopStart   = NULL;
   voice->LoopCount   = 0;
   voice->length      = 0;
   voice->BlockLength = absloopend;
   voice->NextBlock   = ( uint8_t * )( ptr + sizeof(riff) + sizeof(format) + sizeof(data) );
#endif
   voice->position    = 0;
#if VOICE_LINKED_LIST
   voice->next        = NULL;
   voice->prev        = NULL;
#endif
   voice->priority    = priority;
#if WIN32
   voice->GLast       = -1;
   voice->GPos        = 0;
   voice->GVal[0]     = 0;
   voice->GVal[1]     = 0;
   voice->GVal[2]     = 0;
   voice->GVal[3]     = 0;
#endif
   voice->soundNum = soundNum;
#if WIN32
   voice->LoopStart   = voice->NextBlock + loopstart;
   voice->LoopEnd     = voice->NextBlock + loopend;
   voice->LoopSize    = absloopend - absloopstart;
#else
   voice->loopStart = loopstart;
#endif
   if ( ( loopstart >= data.size ) || ( loopstart < 0 ) )
      {
#if WIN32
       voice->LoopStart = NULL;
      voice->LoopEnd   = NULL;
      voice->BlockLength = length;
#else
      voice->loopStart = -1;
#endif
      }

   MV_SetVoicePitch( voice, format.nSamplesPerSec, pitchoffset );
   MV_SetVoiceVolume( voice, vol, left, right );
   MV_PlayVoice( voice );
#if VOICE_LINKED_LIST
   return( voice->handle );
#else
    return (voice - &VoiceArray[0]) + 1;
#endif
}
#if USE_VOC

/*---------------------------------------------------------------------
   Function: MV_PlayVOC3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int MV_PlayVOC3D
   (
   uint8_t *ptr,
   int  pitchoffset,
   int  angle,
   int  distance,
   int  priority,
   unsigned long soundNum
   )

   {
   int left;
   int right;
   int mid;
   int volume;
   int status;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   if ( distance < 0 )
      {
      distance  = -distance;
      angle    += MV_NumPanPositions / 2;
      }

   volume = MIX_VOLUME( distance );

   // Ensure angle is within 0 - 31
   angle &= MV_MaxPanPosition;

   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );

   status = MV_PlayVOC( ptr, pitchoffset, mid, left, right, priority, soundNum );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayVOC

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int MV_PlayVOC
   (
   uint8_t *ptr,
   int   pitchoffset,
   int   vol,
   int   left,
   int   right,
   int   priority,
   unsigned long soundNum
   )

   {
   int status;

   status = MV_PlayLoopedVOC( ptr, -1, -1, pitchoffset, vol, left, right, priority, soundNum );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayLoopedVOC

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int MV_PlayLoopedVOC
   (
   uint8_t *ptr,
   long  loopstart,
   long  loopend,
   int   pitchoffset,
   int   vol,
   int   left,
   int   right,
   int   priority,
   uint32_t soundNum
   )

   {
   VoiceNode   *voice;
   int          status;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   // Make sure it's a valid VOC file.
   status = strncmp( (char*)ptr, "Creative Voice File", 19 );
   if ( status != 0 )
      {
      MV_SetErrorCode( MV_InvalidVOCFile );
      return( MV_Error );
      }

   // Request a voice from the voice pool
   voice = MV_AllocVoice( priority );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_NoVoices );
      return( MV_Error );
      }

   voice->wavetype    = VOC;
   voice->bits        = 8;
   voice->GetSound    = MV_GetNextVOCBlock;
   voice->NextBlock   = ptr + *( uint16_t  * )( ptr + 0x14 );
#if 0
   voice->DemandFeed  = NULL;
#endif
   voice->LoopStart   = NULL;
   voice->LoopCount   = 0;
   voice->BlockLength = 0;
   voice->PitchScale  = PITCH_GetScale( pitchoffset );
   voice->length      = 0;
#if VOICE_LINKED_LIST
   voice->next        = NULL;
   voice->prev        = NULL;
#endif
   voice->priority    = priority;
   voice->GLast       = -1;
   voice->GPos        = 0;
   voice->GVal[0]     = 0;
   voice->GVal[1]     = 0;
   voice->GVal[2]     = 0;
   voice->GVal[3]     = 0;
   voice->soundNum = soundNum;
   voice->LoopStart   = ( uint8_t* )loopstart;
   voice->LoopEnd     = ( uint8_t* )loopend;
   voice->LoopSize    = loopend - loopstart + 1;

   if ( loopstart < 0 )
      {
      voice->LoopStart = NULL;
      voice->LoopEnd   = NULL;
      }

   MV_SetVoiceVolume( voice, vol, left, right );
   MV_PlayVoice( voice );
#if VOICE_LINKED_LIST
   return( voice->handle );
#else
    return (voice - &VoiceArray[0]) + 1;
#endif
   }
#endif

/*---------------------------------------------------------------------
   Function: MV_LockEnd

   Used for determining the length of the functions to lock in memory.
---------------------------------------------------------------------*/
#if 0
static void MV_LockEnd
   (
   void
   )

   {
   }
/*---------------------------------------------------------------------
   Function: MV_CreateVolumeTable

   Create the table used to convert sound data to a specific volume
   level.
---------------------------------------------------------------------*/

void MV_CreateVolumeTable
   (
   int index,
   int volume,
   int MaxVolume
   )

   {
   int val;
   int level;
   int i;

   level = ( volume * MaxVolume ) / MV_MaxTotalVolume;
   if ( MV_Bits == 16 )
      {
      for( i = 0; i < 65536; i += 256 )
         {
         val   = i - 0x8000;
         val  *= level;
         val  /= MV_MaxVolume;
         MV_VolumeTable[ index ][ i / 256 ] = val;
         }
      }
   else
      {
      for( i = 0; i < 256; i++ )
         {
         val   = i - 0x80;
         val  *= level;
         val  /= MV_MaxVolume;
         MV_VolumeTable[ volume ][ i ] = val;
         }
      }
   }
#endif

/*---------------------------------------------------------------------
   Function: MV_CalcVolume

   Create the table used to convert sound data to a specific volume
   level.
---------------------------------------------------------------------*/

void MV_CalcVolume(int MaxVolume)
   {
 UNUSED(MaxVolume);
   }


/*---------------------------------------------------------------------
   Function: MV_CalcPanTable

   Create the table used to determine the stereo volume level of
   a sound located at a specific angle and distance from the listener.
---------------------------------------------------------------------*/

void MV_CalcPanTable( void)
{
#if RUNTIME_PANTABLE
   int   level;
   int   angle;
   int   distance;
   int   HalfAngle;
   int   ramp;

   HalfAngle = ( MV_NumPanPositions / 2 );

   for( distance = 0; distance <= MV_MaxVolume; distance++ )
    {
      level = ( 255 * ( MV_MaxVolume - distance ) ) / MV_MaxVolume;
      for( angle = 0; angle <= HalfAngle / 2; angle++ )
         {
         ramp = level - ( ( level * angle ) /
            ( MV_NumPanPositions / 4 ) );

         MV_PanTable[ angle ][ distance ].left = ramp;
         MV_PanTable[ HalfAngle - angle ][ distance ].left = ramp;
         MV_PanTable[ HalfAngle + angle ][ distance ].left = level;
         MV_PanTable[ MV_MaxPanPosition - angle ][ distance ].left = level;

         MV_PanTable[ angle ][ distance ].right = level;
         MV_PanTable[ HalfAngle - angle ][ distance ].right = level;
         MV_PanTable[ HalfAngle + angle ][ distance ].right = ramp;
         MV_PanTable[ MV_MaxPanPosition - angle ][ distance ].right = ramp;
     }
    }
    #define PRINT_MV_TABLE 1
    #if PRINT_MV_TABLE
    printf("const Pan MV_PanTable[ MV_NumPanPositions ][ 63 + 1 ] =\r\n{\r\n");
    for (int i = 0; i <= MV_NumPanPositions; i++ )
    {
        printf("    {\r\n");
        for (int j = 0; j < 64; j++)
        {
            printf("        {.left = 0x%02x, .right = 0x%02x},\r\n", MV_PanTable[i][j].left & 0xFF, MV_PanTable[i][j].right & 0xFF);
        }
        printf("    },\r\n");
    }
    printf("};\r\n");
    FIXME("Done!");
    #endif
#endif
}


/*---------------------------------------------------------------------
   Function: MV_SetVolume

   Sets the volume of digitized sound playback.
---------------------------------------------------------------------*/

void MV_SetVolume
   (
   int volume
   )

   {
   volume = max( 0, volume );
   volume = min( volume, MV_MaxTotalVolume );

   MV_TotalVolume = volume;

   // Calculate volume table
   MV_CalcVolume( volume );
   }


/*---------------------------------------------------------------------
   Function: MV_GetVolume

   Returns the volume of digitized sound playback.
---------------------------------------------------------------------*/

int MV_GetVolume
   (
   void
   )

   {
   return( MV_TotalVolume );
   }


/*---------------------------------------------------------------------
   Function: MV_SetCallBack

   Set the function to call when a voice stops.
---------------------------------------------------------------------*/
#if WIN32
void MV_SetCallBack
   (
   void ( *function )( int32_t )
   )

   {
   MV_CallBackFunc = function;
   }
#endif

/*---------------------------------------------------------------------
   Function: MV_SetReverseStereo

   Set the orientation of the left and right channels.
---------------------------------------------------------------------*/

void MV_SetReverseStereo
   (
   int setting
   )

   {
   MV_SwapLeftRight = setting;
   }


/*---------------------------------------------------------------------
   Function: MV_GetReverseStereo

   Returns the orientation of the left and right channels.
---------------------------------------------------------------------*/

int MV_GetReverseStereo
   (
   void
   )

   {
   return( MV_SwapLeftRight );
   }


/*---------------------------------------------------------------------
   Function: MV_TestPlayback

   Checks if playback has started.
---------------------------------------------------------------------*/


/*---------------------------------------------------------------------
   Function: MV_Init

   Perform the initialization of variables and memory used by
   Multivoc.
---------------------------------------------------------------------*/
#if WIN32
SDL_mutex * reverbMutex;
int MV_Init
   (
   int soundcard,
   int MixRate,
   int Voices,
   int numchannels,
   int samplebits
   )

   {
    char *ptr;
   int  status;
   int  buffer;


   if ( MV_Installed )
      {
      MV_Shutdown();
      }

   MV_SetErrorCode( MV_Ok );
  // Set number of voices before calculating volume table
   MV_MaxVoices = Voices;

#if VOICE_LINKED_LIST
   int  index;
    printf("Voices: %d\r\n", Voices);
   int MV_TotalMemory = Voices * sizeof( VoiceNode ); // + sizeof( HARSH_CLIP_TABLE_8 );
   FIXME("Total memory %d %d\r\n", Voices, MV_TotalMemory);
   status = USRHOOKS_GetMem( ( void ** )&ptr, MV_TotalMemory );
   if ( status != USRHOOKS_Ok )
      {
      MV_SetErrorCode( MV_NoMem );
      return( MV_Error );
      }

   MV_Voices = ( VoiceNode * )ptr;
//   MV_HarshClipTable = ptr + ( MV_TotalMemory - sizeof( HARSH_CLIP_TABLE_8 ) );


   LL_Reset( &VoiceList, next, prev );
   LL_Reset( &VoicePool, next, prev );

   for( index = 0; index < Voices; index++ )
      {
      LL_Add( &VoicePool, &MV_Voices[ index ], next, prev );
      }
#else
    memset((void *) VoiceArray, 0, sizeof(VoiceArray));
#endif

   // Allocate mix buffer within 1st megabyte
   status = DPMI_GetDOSMemory( ( void ** )&ptr, &MV_BufferDescriptor, 2 * TotalBufferSize);

   if ( status )
      {
#if VOICE_LINKED_LIST
      USRHOOKS_FreeMem( MV_Voices );
      MV_Voices      = NULL;
#endif
//      MV_TotalMemory = 0;
      MV_SetErrorCode( MV_NoMem );
      return( MV_Error );
      }

   MV_SetReverseStereo( FALSE );

   // Initialize the sound card
   status = DSL_Init();
   if ( status != DSL_Ok )
      {
      MV_SetErrorCode( MV_BlasterError );
      }

   if ( MV_ErrorCode != MV_Ok )
      {
      status = MV_ErrorCode;
#if VOICE_LINKED_LIST
      USRHOOKS_FreeMem( MV_Voices );
      MV_Voices      = NULL;
#endif
//      MV_TotalMemory = 0;
      DPMI_FreeDOSMemory( MV_BufferDescriptor );
      MV_SetErrorCode( status );
      return( MV_Error );
      }

   MV_SoundCard    = soundcard;
   MV_Installed    = TRUE;
#if WIN32
   MV_CallBackFunc = NULL;
#endif
   //MV_RecordFunc   = NULL;
   MV_ReverbLevel  = 0;
   MV_ReverbTable  = -1;

   // Set the sampling rate
   MV_RequestedMixRate = MixRate;

   // Set Mixer to play stereo digitized sound
   MV_SetMixMode( numchannels, samplebits );
   MV_ReverbDelay = 14320; // MV_BufferSize * 3;
   //InitializeCriticalSection(&reverbCS);
#ifdef WIN32
   reverbMutex = SDL_CreateMutex();
#endif
   MV_MixBuffer[ MV_NumberOfBuffers ] = ptr;
   for( buffer = 0; buffer < MV_NumberOfBuffers; buffer++ )
      {
      MV_MixBuffer[ buffer ] = ptr;
      ptr += MV_BufferSize;
      }

   // Calculate pan table
   MV_CalcPanTable();

   MV_SetVolume( MV_MaxTotalVolume );


   int MV_FooMemory = sizeof(MV_FLOAT) * MixBufferSize * numchannels + 1024;
   status = USRHOOKS_GetMem( ( void ** )&ptr, MV_FooMemory);
   if ( status != USRHOOKS_Ok )
   {
#if VOICE_LINKED_LIST
       USRHOOKS_FreeMem( MV_Voices );
       MV_Voices      = NULL;
#endif
//       MV_TotalMemory = 0;
	   MV_SetErrorCode( MV_NoMem );
	   return( MV_Error);
   }

   MV_FooBuffer = ptr;

// Start the playback engine
   status = MV_StartPlayback();
   if ( status != MV_Ok )
      {
      // Preserve error code while we shutdown.
      status = MV_ErrorCode;
      MV_Shutdown();
      MV_SetErrorCode( status );
      return( MV_Error );
      }
   return( MV_Ok );
   }
#endif

/*---------------------------------------------------------------------
   Function: MV_Shutdown

   Restore any resources allocated by Multivoc back to the system.
---------------------------------------------------------------------*/
#if WIN32
int MV_Shutdown
   (
   void
   )

   {
   int      buffer;

   if ( !MV_Installed )
      {
      return( MV_Ok );
      }

   MV_KillAllVoices();

   MV_Installed = FALSE;

   // Stop the sound playback engine
   MV_StopPlayback();

   // Free reverb buffer, if allocated
   MV_FPReverbFree();
   //DeleteCriticalSection(&reverbCS);
#ifdef WIN32
   SDL_DestroyMutex(reverbMutex);
#endif
   // Shutdown the sound card
   DSL_Shutdown();

   // Free any voices we allocated
   USRHOOKS_FreeMem( MV_FooBuffer );
   MV_FooBuffer = NULL;
 //  MV_FooMemory = 0;
#if VOICE_LINKED_LIST
   USRHOOKS_FreeMem( MV_Voices );
   MV_Voices      = NULL;
//   MV_TotalMemory = 0;

   LL_Reset( &VoiceList, next, prev );
   LL_Reset( &VoicePool, next, prev );
#endif

   MV_MaxVoices = 1;

   // Release the descriptor from our mix buffer
   DPMI_FreeDOSMemory( MV_BufferDescriptor );
   for( buffer = 0; buffer < NumberOfBuffers; buffer++ )
      {
      MV_MixBuffer[ buffer ] = NULL;
      }

   return( MV_Ok );
   }

#endif

void ClearBuffer_DW( void *ptr, unsigned data, int length )
{
	unsigned *d = (unsigned *)ptr;

	while (length--) {
		*d = data;

		d++;
	}
}

void waitForVoiceEnd(int voiceNum, int maxDelay, void (*f)(void))
{
    (void) maxDelay;
    if (voiceNum > 0 && voiceNum <= MAX_VOICES)
    {
        voiceNum--;
        while (1)
        {
            if (f)
            {
                f();
            }
            if (!VoiceArray[voiceNum].Playing)
            {
                return;
            }
        #if !WIN32
            updateAudio();
        #endif
        }
    }
}
//
#if !WIN32
#include "audio.h"
#define REVERB_DELAY_SAMPLES        768   // delay is around 69 ms
#define REVERB_BUFFER_NUM_SAMPLES   (AUDIO_BUFFER_LENGTH + REVERB_DELAY_SAMPLES - AUDIO_BUFFER_DELAY)
uint16_t reverbSamplesPlayed = 0;
int8_t musBuffer[MUSIC_NUM_SAMPLES];
int8_t reverbBuffer[REVERB_BUFFER_NUM_SAMPLES][2];

void MV_ReverbClear(void)
{
    memset(reverbBuffer, 0, sizeof(reverbBuffer));
}
typedef struct
{
    int left;
    int right;
} portable_samplepair_t;
void SND_PaintChannelFrom8(portable_samplepair_t *paintbuffer, VoiceNode *voice, uint8_t *sampleData, float count, float pos)
{
    int data;
    int i;

    // load data
    unsigned char tsfx[4 * (AUDIO_BUFFER_LENGTH) + 8]; // note: 4x to take into account that some samples might have a faster sample rate, so given the same interval, the count will be larger.
    int offs = ((uint32_t) sampleData + (uint32_t) pos) & 3;
    unsigned char *sfx = extMemGetDataFromAddress((char*) tsfx, (signed char*) (((uint32_t) sampleData + (uint32_t) pos) & ~3), (int) (count + 0.5f + 3 + offs) & ~3);
    sfx += offs;
    // get fractional part
    pos = pos - (int) pos;
    int lvol = voice->LeftVolume << 2;
    int rvol = voice->RightVolume << 2;
    float rate = voice->mixerToSampleRateMult;
    float irate = 1.0f / rate;
    int icount = (count * irate) + 0.5f;
    for (i = 0; i < icount; i++)
    {
        data = (int) sfx[(int)pos] - 128;
        pos += rate;

        paintbuffer[i].left += ((signed char) data) * lvol;
        paintbuffer[i].right += ((signed char) data) * rvol;
    }
}
uint8_t musicVolume = 100;
uint16_t lastMusicIdx = 0;
void MUSIC_SetVolume( int volume )
{
    musicVolume = volume;
}
void MUSIC_StopFade(void)
{
    uint8_t backVolume = musicVolume;
    for (int i = 0; i < 255; i++)
    {
        delay(1);
        if (musicVolume)
        {
            musicVolume--;
        }
        updateAudio();
    }
    memset(musBuffer, 0, sizeof(musBuffer));
    MUSIC_StopSong();
    updateAudio();
    delay(100);
#if !WIN32
    MUSIC_Init(0, 0);
#endif
    updateAudio();
    MUSIC_SetVolume(backVolume);
}
#define TEST_NO_AUDIO 0
#if TEST_AUDIO_CRASH
void updateAudio2(int line, char *file)
{
   printf("L %d, f: %s\r\n", line, file);
#else
void updateAudio(void)
{

#endif

#if TEST_NO_AUDIO
    for (int voiceNum = 0; voiceNum < MAX_VOICES; voiceNum++)
    {
        VoiceNode * voice = &VoiceArray[voiceNum];
        voice->Playing = 0;
    }
    return;
#endif
#if 1
    // where are we in our circular buffer?
#if ESP_PLATFORM
    uint32_t currentIdx = audioGetPlayIndex();
#else
    uint32_t currentIdx = ((LDMA->CH[AUDIO_DMA_CHANNEL_L].SRC - (uint32_t) audioBuffer) / sizeof(audioBuffer[0])) / 2;
#endif
    static uint32_t lastIdx;
    if ((currentIdx - lastIdx) < 20)
    {
        return;
    }
    // update position on the reverb buffer
    reverbSamplesPlayed += currentIdx - lastIdx;
    if (reverbSamplesPlayed >= REVERB_BUFFER_NUM_SAMPLES)
    {
        reverbSamplesPlayed -= REVERB_BUFFER_NUM_SAMPLES;
    }
    //
    lastIdx = currentIdx;
    // we cannot start updating the audio buffer just on next sample (which will occur in about 90us from now).
    // if frame rate is high enough, then the audio buffer is valid several samples after currentIdx.
    // therefore we can start updating the audio buffer AUDIO_BUFFER_DELAY samples after the current one.
    // AUDIO_BUFFER_DELAY must be chosen so that we have enough time to read the sample data from all the lumps
    // and store to the buffer. This might take some time.
    // The index at which we recalculate/update audio samples is startIdx.
    // The buffer is recalculated from startIdx up to the sample before currentIdx
    // As first thing, zeroize buffer;
    // Note that we do not have to end exactly at current index.
    float samplesPlayed = 0;    // how many samples have been played
    uint32_t startIdx = (currentIdx + AUDIO_BUFFER_DELAY) & (AUDIO_BUFFER_LENGTH - 1);
    //
    portable_samplepair_t paintbuffer[AUDIO_BUFFER_LENGTH + 2]; // next-hack: in stack because snd update is done when small amount in stack is used
    // clear paint buffer
    for (int i = 0; i < AUDIO_BUFFER_LENGTH; i++)
    {
        paintbuffer[i].left = 0;
        paintbuffer[i].right = 0;
    }
    // scan through all the voices
    for (int voiceNum = 0; voiceNum < MAX_VOICES; voiceNum++)
    {
         VoiceNode * voice = &VoiceArray[voiceNum];
        if (voice->Playing == 0)
        {
            continue;   // not playing
        }
        if (voice->soundNum >= NUM_SOUNDS)
        {
            continue;   // not valid sound
        }
        // get sound data.
        const SAMPLE *sfx = &Sound[voice->soundNum];
        if (!sfx->ptr)
        {
            continue;   // sound not loaded/not existing.
        }
        // if this is new sample, then we set lastAudioBufferIdx to -1
        if (voice->lastAudioBufferIdx >= 0)
        {
            // otherwise, we need to calculate how many samples do we have played since last call.
            // considering also that we are starting at startIdx.
            //
            samplesPlayed = ((startIdx - voice->lastAudioBufferIdx) & (AUDIO_BUFFER_LENGTH - 1));
            // Because of different sample rate and pitch, we need to know how many actual sound effect samples we played.
            // in fact, if the sample rate is 6000, and mixer rate is 11025 and the pitch rate is 1, for each sample played in the mixer, we have just
            // played 0.544 samples of the sound effect. In other words, we need about 2 samples in the mixer for each sample in the sound.
            samplesPlayed *= voice->mixerToSampleRateMult;
            //
            voice->position += samplesPlayed;
            // if the voice is looping, and we are inside the loop, then it might happen we need to wrap the position inside the loop
            if (voice->loopStart != -1 && voice->position > voice->loopStart)
            {
                // adjust position in loop
                // FIXME: is a subtraction enough ?
                voice->position = voice->loopStart + ((int) voice->position - voice->loopStart) % (sfx->length - voice->loopStart) + (voice->position - (int) voice->position);
            }
        }
        // remember last index.
        voice->lastAudioBufferIdx = startIdx;
        // how many bytes do we need to read?
        // get sample size
        int32_t sfxSize = sfx->length;
        //
        // read through the end of the samples
        float sizeToRead = sfxSize - voice->position;
        float pos = 0;
        if (sizeToRead <= 0) // already outputted all samples?
        {
            // no loop? call callback and stop voice.
            if (voice->loopStart == -1)
            {
                TestCallBack(voice->soundNum);
                voice->Playing = 0;
                continue;
            }
            // Below shall never happen..
            sizeToRead = sfxSize;
            pos = voice->loopStart;
        }
        else
        {
            pos = voice->position;
        }
        // now we know how many samples we can read from the sound. However, based on pitch and different sample rate
        // we might need to read more or less samples.
        float mixerSizeToRead = (AUDIO_BUFFER_LENGTH - AUDIO_BUFFER_DELAY) *  voice->mixerToSampleRateMult;
        if (sizeToRead  > mixerSizeToRead)
        {
            sizeToRead = (AUDIO_BUFFER_LENGTH - AUDIO_BUFFER_DELAY) * voice->mixerToSampleRateMult;
        }
        // the idea is now that first we load and mix until we either fill
        // sizeToRead is for sure strictly positive.
        SND_PaintChannelFrom8(paintbuffer, voice, sfx->ptr + WAV_DATA_OFFSET, sizeToRead, pos);
        //
        if ((sizeToRead < mixerSizeToRead) && voice->loopStart != -1)
        {
            // this is because the previous read up to end of the sample
            float remaining = mixerSizeToRead - sizeToRead;
            // FIXME: what about samples with short loop?
            pos = max(voice->position + sizeToRead - sfxSize, 0) + voice->loopStart;
            SND_PaintChannelFrom8(&paintbuffer[(int)(sizeToRead / voice->mixerToSampleRateMult)], voice, sfx->ptr + WAV_DATA_OFFSET, remaining, pos);
        }
    }
    //
    //
    int *p = (int*) paintbuffer;
    //
#define volume 1
    int snd_vol = volume * 256;
    //
    // Reverb
    //
    if (MV_ReverbLevel)
    {
        // we must add the delay to the reverbBufferIdx
        int revBufferIdx = reverbSamplesPlayed + AUDIO_BUFFER_DELAY;
        if (revBufferIdx >= REVERB_BUFFER_NUM_SAMPLES)
        {
            revBufferIdx -= REVERB_BUFFER_NUM_SAMPLES;
        }
        int reverBufferDelayIdx = revBufferIdx - REVERB_DELAY_SAMPLES;
        if (reverBufferDelayIdx < 0)
        {
            reverBufferDelayIdx += REVERB_BUFFER_NUM_SAMPLES;
        }
        for (uint32_t i = startIdx; i != ((currentIdx - 1) & (AUDIO_BUFFER_LENGTH - 1)); i = (i + 1) & (AUDIO_BUFFER_LENGTH - 1))
        {

#if 0
            // read left value
            int val;
            val = *p >> 8;
            reverbBuffer[revBufferIdx][0] = __SSAT(reverbBuffer[reverBufferDelayIdx][0] * MV_ReverbTable / 64  + val, 8);
            // store it back
            *p = reverbBuffer[revBufferIdx][0] << 8;
            p++;
            //
            // read right value
            val = *p >> 8;
            reverbBuffer[revBufferIdx][1] = __SSAT(reverbBuffer[reverBufferDelayIdx][1] * MV_ReverbTable / 64  + val, 8);
            // store it back
            *p = reverbBuffer[revBufferIdx][1] << 8;
            p++;
#elif 0

            // read left value
            int val;
            *p  = __SSAT(reverbBuffer[reverBufferDelayIdx][0] + *p, 16);
            val = *p *  MV_ReverbTable * 3 / 128;
            reverbBuffer[revBufferIdx][0] = val;
            p++;
            // do the same for the right channel
            *p  = __SSAT(reverbBuffer[reverBufferDelayIdx][1] + *p, 16);
            val = *p *  MV_ReverbTable * 3 / 128;
            reverbBuffer[revBufferIdx][1] = val;
            p++;
#else
            // read left value
            int val;
            *p  = __SSAT((reverbBuffer[reverBufferDelayIdx][0] << 8) + *p, 16);
            val = *p *  MV_ReverbTable * 3 / 128;
            reverbBuffer[revBufferIdx][0] = val >> 8;
            p++;
            // do the same for the right channel
            *p  = __SSAT((reverbBuffer[reverBufferDelayIdx][1] << 8) + *p, 16);
            val = *p *  MV_ReverbTable * 3 / 128;
            reverbBuffer[revBufferIdx][1] = val >> 8;
            p++;

#endif
            // increment indexes
            revBufferIdx++;
            if (revBufferIdx >= REVERB_BUFFER_NUM_SAMPLES)
            {
                revBufferIdx -= REVERB_BUFFER_NUM_SAMPLES;
            }
            reverBufferDelayIdx++;
            if (reverBufferDelayIdx >= REVERB_BUFFER_NUM_SAMPLES)
            {
                reverBufferDelayIdx -= REVERB_BUFFER_NUM_SAMPLES;
            }
        }
    }
    // reset back pointer
    p = (int*) paintbuffer;
#if ENABLE_MUSIC
    if (isMusicPlaying() && musicVolume)
    {
      int32_t musBuffer32[MUSIC_NUM_SAMPLES];
      //
      uint32_t numberOfSamplesToGenerate =  ((currentIdx - 1 - lastMusicIdx))  & (MUSIC_NUM_SAMPLES - 1);
      generateMusicOutput(musBuffer32, numberOfSamplesToGenerate);
      uint32_t m = 0;
      for (; lastMusicIdx != ((currentIdx - 1) & (MUSIC_NUM_SAMPLES - 1)); lastMusicIdx = (lastMusicIdx + 1) & (MUSIC_NUM_SAMPLES - 1))
      {
        int32_t sv =  musBuffer32[m++];
        sv = (sv * musicVolume) >> 16;
        musBuffer[lastMusicIdx] = __SSAT(sv, 8);
      }
    }

#define ZERO_AUDIO_LEVEL                128
    //
    if (isMusicPlaying()  && musicVolume)
    {
      for (uint32_t i = startIdx; i != ((currentIdx - 1) & (AUDIO_BUFFER_LENGTH - 1)); i = (i + 1) & (AUDIO_BUFFER_LENGTH - 1))
      {
          int32_t music = musBuffer[i];
          int val;
          val = (*p++ * snd_vol) >> 16;
          val = __SSAT(music + val, 8) + ZERO_AUDIO_LEVEL;
          audioBuffer[2 * i] = val;
          #if STEREO_AUDIO
              val = (*p++ * snd_vol) >> 16;
              val = __SSAT(music + val, 8) + ZERO_AUDIO_LEVEL;
              audioBuffer[2 * i + 1] = val;
          #endif
      }
    }
    else
    {
      for (uint32_t i = startIdx; i != ((currentIdx - 1) & (AUDIO_BUFFER_LENGTH - 1)); i = (i + 1) & (AUDIO_BUFFER_LENGTH - 1))
      {
          int val;
              val = (*p++ * snd_vol) >> 16;
              val = __SSAT(val, 8) + ZERO_AUDIO_LEVEL;
              audioBuffer[2 * i] = val;
          #if STEREO_AUDIO
              val = (*p++ * snd_vol) >> 16;
              val = __SSAT(val, 8) + ZERO_AUDIO_LEVEL;
              audioBuffer[2 * i + 1] = val;
          #endif
      }
    }
#endif
#endif
}
#endif
