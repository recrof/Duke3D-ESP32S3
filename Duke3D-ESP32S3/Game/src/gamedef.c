//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
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

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "maincfg.h"

#if WIN32
	#include <conio.h>
#endif

#include <ctype.h>
#ifdef WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "duke3d.h"
#include "engine.h"
#include "keyboard.h"
#include "soundefs.h"
#include "internalFlash.h"
#include "audiolib/multivoc.h"
#include "audiolib/fx_man.h"
#include "port/graphics.h"
extern short otherp;

//static short total_lines;
static short line_number;
//static uint8_t  checking_ifelse,parsing_state;
char *last_used_text;
//static short num_squigilly_brackets;
//static int32_t last_used_size;

static short g_i;           // index of global hittype index used, and sprite as well.
static short g_p;
static int32_t g_x;
#define NUMKEYWORDS     112

//From global.c
void FixFilePath(char  *filename);

//From actors.c
void lotsofmail(const spritetype *s, short n);
void lotsofpaper(const spritetype *s, short n);
#if LOAD_FROM_CON_FILE
static char tempbuf[512];
#endif // LOAD_FROM_CON_FILE
static uint16_t *freeableSprites = NULL;
static const uint16_t freeableSpriteList[] =
{
    BLOODSPLAT1,
    BLOODSPLAT2,
    BLOODSPLAT3,
    BLOODSPLAT4,
    BLOODPOOL,
    PIGCOPDEADSPRITE,
    LIZMANDEADSPRITE,
    WALLBLOOD1,
    WALLBLOOD2,
    WALLBLOOD3,
    WALLBLOOD4,
    WALLBLOOD5,
    WALLBLOOD6,
    WALLBLOOD7,
    WALLBLOOD8,
    FOOTPRINTS,
    FOOTPRINTS2,
    FOOTPRINTS3,
    FOOTPRINTS4,
    BULLETHOLE,
    BLOOD,
    SCRAP6,        // 2390
    SCRAP6 + 1,
    SCRAP6 + 2,
    SCRAP6 + 3,
    SCRAP6 + 4,
    SCRAP6 + 5,
    SCRAP6 + 6,
    SCRAP6 + 7,
    SCRAP6 + 8,
    SCRAP6 + 9,
    SCRAP6 + 10,
    SCRAP6 + 11,
    SCRAP6 + 12,
    SCRAP6 + 13,
    SCRAP6 + 14,
    SCRAP6 + 15,
    SCRAP6 + 16,
    SCRAP6 + 17,
    SCRAP6 + 18,
    SCRAP6 + 19,
    SCRAP6 + 20,
    SCRAP6 + 21,
    SCRAP6 + 22,
    SCRAP6 + 23,
    SCRAP6 + 24,
    SCRAP6 + 25,
    SCRAP6 + 26,
    SCRAP6 + 27,
    SCRAP6 + 28,
    SCRAP6 + 29,  // 2419
    GLASSPIECES,
    GLASSPIECES + 1,
    GLASSPIECES + 2,
    GLASSPIECES + 3,
    MONEY,
    MONEY+1,
    MAIL,
    MAIL+1,
    PAPER,
    PAPER+1,
    JIBS1,
    JIBS1 + 1,
    JIBS1 + 2,
    JIBS1 + 3,
    JIBS1 + 4,
    JIBS2,
    JIBS2 + 1,
    JIBS2 + 2,
    JIBS2 + 3,
    JIBS2 + 4,
    JIBS3,
    JIBS3 + 1,
    JIBS3 + 2,
    JIBS3 + 3,
    JIBS3 + 4,
    JIBS4,
    JIBS4 + 1,
    JIBS4 + 2,
    JIBS4 + 3,
    JIBS4 + 4,
    JIBS5,
    JIBS5 + 1,
    JIBS5 + 2,
    JIBS5 + 3,
    JIBS5 + 4,
    JIBS6,
    JIBS6 + 1,
    JIBS6 + 2,
    JIBS6 + 3,
    JIBS6 + 4,
    HEADJIB1,
    HEADJIB1 + 1,
    HEADJIB1 + 2,
    HEADJIB1 + 3,
    ARMJIB1,
    ARMJIB1 + 1,
    ARMJIB1 + 2,
    ARMJIB1 + 3,
    LEGJIB1,
    LEGJIB1 + 1,
    LEGJIB1 + 2,
    LEGJIB1 + 3,
    FETUSJIB,
};
const char  *const keyw[NUMKEYWORDS] =
{
    "definelevelname",  // 0
    "actor",            // 1    [#]
    "addammo",          // 2    [#]
    "ifrnd",            // 3    [C]
    "enda",             // 4    [:]
    "ifcansee",         // 5    [C]
    "ifhitweapon",      // 6    [#]
    "action",           // 7    [#]
    "ifpdistl",         // 8    [#]
    "ifpdistg",         // 9    [#]
    "else",             // 10   [#]
    "strength",         // 11   [#]
    "break",            // 12   [#]
    "shoot",            // 13   [#]
    "palfrom",          // 14   [#]
    "sound",            // 15   [filename.voc]
    "fall",             // 16   []
    "state",            // 17
    "ends",             // 18
    "define",           // 19
    "//",               // 20
    "ifai",             // 21
    "killit",           // 22
    "addweapon",        // 23
    "ai",               // 24
    "addphealth",       // 25
    "ifdead",           // 26
    "ifsquished",       // 27
    "sizeto",           // 28
    "{",                // 29
    "}",                // 30
    "spawn",            // 31
    "move",             // 32
    "ifwasweapon",      // 33
    "ifaction",         // 34
    "ifactioncount",    // 35
    "resetactioncount", // 36
    "debris",           // 37
    "pstomp",           // 38
    "/*",               // 39
    "cstat",            // 40
    "ifmove",           // 41
    "resetplayer",      // 42
    "ifonwater",        // 43
    "ifinwater",        // 44
    "ifcanshoottarget", // 45
    "ifcount",          // 46
    "resetcount",       // 47
    "addinventory",     // 48
    "ifactornotstayput",// 49
    "hitradius",        // 50
    "ifp",              // 51
    "count",            // 52
    "ifactor",          // 53
    "music",            // 54
    "include",          // 55
    "ifstrength",       // 56
    "definesound",      // 57
    "guts",             // 58
    "ifspawnedby",      // 59
    "gamestartup",      // 60
    "wackplayer",       // 61
    "ifgapzl",          // 62
    "ifhitspace",       // 63
    "ifoutside",        // 64
    "ifmultiplayer",    // 65
    "operate",          // 66
    "ifinspace",        // 67
    "debug",            // 68
    "endofgame",        // 69
    "ifbulletnear",     // 70
    "ifrespawn",        // 71
    "iffloordistl",     // 72
    "ifceilingdistl",   // 73
    "spritepal",        // 74
    "ifpinventory",     // 75
    "betaname",         // 76
    "cactor",           // 77
    "ifphealthl",       // 78
    "definequote",      // 79
    "quote",            // 80
    "ifinouterspace",   // 81
    "ifnotmoving",      // 82
    "respawnhitag",     // 83
    "tip",              // 84
    "ifspritepal",      // 85
    "money",            // 86
    "soundonce",        // 87
    "addkills",         // 88
    "stopsound",        // 89
    "ifawayfromwall",   // 90
    "ifcanseetarget",   // 91
    "globalsound",      // 92
    "lotsofglass",      // 93
    "ifgotweaponce",    // 94
    "getlastpal",       // 95
    "pkick",            // 96
    "mikesnd",          // 97
    "useractor",        // 98
    "sizeat",           // 99
    "addstrength",      // 100   [#]
    "cstator",          // 101
    "mail",             // 102
    "paper",            // 103
    "tossweapon",       // 104
    "sleeptime",        // 105
    "nullop",           // 106
    "definevolumename", // 107
    "defineskillname",  // 108
    "ifnosounds",       // 109
    "clipdist",         // 110
    "ifangdiffl"        // 111
};
// Hittypes. What a mess
#if USE_SEPARATE_HITTYPE_TEMPDATA
uint8_t hittype_tempdata0_allocated_bf[(MAX_HITTYPE_TEMPDATA0 + 7) / 8];
uint8_t hittype_tempdata1_allocated_bf[(MAX_HITTYPE_TEMPDATA1 + 7) / 8];
uint8_t hittype_tempdata2_allocated_bf[(MAX_HITTYPE_TEMPDATA2 + 7) / 8];
uint8_t hittype_tempdata3_allocated_bf[(MAX_HITTYPE_TEMPDATA3 + 7) / 8];
uint8_t hittype_tempdata45_allocated_bf[(MAX_HITTYPE_TEMPDATA45 + 7) / 8];
//
int32_t hittype_tempdata0[MAX_HITTYPE_TEMPDATA0];
int32_t hittype_tempdata1[MAX_HITTYPE_TEMPDATA1];
int32_t hittype_tempdata2[MAX_HITTYPE_TEMPDATA2];
int32_t hittype_tempdata3[MAX_HITTYPE_TEMPDATA3];
tempdata45_t hittype_tempdata45[MAX_HITTYPE_TEMPDATA45];
#else
uint8_t hittype_tempdata_allocated_bf[(MAX_HITTYPE_TEMPDATA + 7) / 8];
weaponhit_tempdata_t hittype_tempdata[MAX_HITTYPE_TEMPDATA];
#endif // USE_SEPARATE_HITTYPE_TEMPDATA
uint8_t hittype_otherdata_allocated_bf[(MAX_HITTYPE_OTHERDATA + 7) / 8];
weaponhit_otherdata_t hittype_otherdata[MAX_HITTYPE_OTHERDATA];
#if USE_RAM_HITTYPE
uint8_t hittype_allocated_bf[(MAX_RAM_HITTYPES + 7) / 8];
struct weaponhit *hittypeRsvdRomPtr = 0;
#endif // USE_RAM_HITTYPE
#if USE_SEPARATE_HITTYPE_TEMPDATA
int numHittype_tempdata0 = 0;
int numHittype_tempdata1 = 0;
int numHittype_tempdata2 = 0;
int numHittype_tempdata3 = 0;
int numHittype_tempdata45 = 0;
#else
int numHittype_tempdata = 0;
#endif
int numHittypes = 0;
int numHittype_otherdata = 0;
void freeUnnecessarySprites(int forWhichSprite, int forWhichHittype)
{
    for (int i = 0; i < MAXSPRITES; i++)
    {
        if (i == forWhichSprite)
            continue;
        if (forWhichSprite >= 0)
        {
            if (spr_add_data[forWhichSprite].nextspritesect == i || spr_add_data[forWhichSprite].nextspritestat == i)
            {
                continue;
            }
        }
        int spriteIdx = getRamSpriteIdx(i) - 1;
        if (spriteIdx >= 0)
        {
// TODO: probably with the new free method, it is better first to check the picture, and later the hittype.
            // this sprite was ram allocated.
            if (forWhichHittype > NO_HITTYPE_REQUIRED)
            {
                // we have to check if this sprite has the required hittype allocated.
                // If has a particular hittype, then
                if (!hittypeIdx[i])
                {
                    // in flash. this is not ok for us.
                    continue;
                }
                // do we need also a hittype tempdata ?
                const struct weaponhit * ht = getHittype(i);
                switch (forWhichHittype)
                {
                    case HITTYPE_FEATURE_ID_tempdata0: if (!ht->tempdata0_idx) continue; else break;
                    case HITTYPE_FEATURE_ID_tempdata1: if (!ht->tempdata1_idx) continue; else break;
                    case HITTYPE_FEATURE_ID_tempdata2: if (!ht->tempdata2_idx) continue; else break;
                    case HITTYPE_FEATURE_ID_tempdata3: if (!ht->tempdata3_idx) continue; else break;
                    case HITTYPE_FEATURE_ID_tempdata45: if (!ht->tempdata45_idx) continue; else break;
                    case HITTYPE_FEATURE_ID_otherdata: if (!ht->otherdata_idx) continue; else break;
                    default: break;         // no hittype temp/otherdata
                }
            }
//            int pn = getSprite_picnum(i);
            int pn = ramsprites[spriteIdx]._picnum;
            #define USE_OLD_FREE_METHOD 0
#if USE_OLD_FREE_METHOD
            switch(pn)
            {
            case BLOODSPLAT1:
            case BLOODSPLAT2:
            case BLOODSPLAT3:
            case BLOODSPLAT4:
            case BLOODPOOL:
            case PIGCOPDEADSPRITE:
            case LIZMANDEADSPRITE:
            case WALLBLOOD1:
            case WALLBLOOD2:
            case WALLBLOOD3:
            case WALLBLOOD4:
            case WALLBLOOD5:
            case WALLBLOOD6:
            case WALLBLOOD7:
            case WALLBLOOD8:
            case FOOTPRINTS:
            case FOOTPRINTS2:
            case FOOTPRINTS3:
            case FOOTPRINTS4:
            case BULLETHOLE:
            case BLOOD:
            case SCRAP6:        // 2390
            case SCRAP6 + 1:
            case SCRAP6 + 2:
            case SCRAP6 + 3:
            case SCRAP6 + 4:
            case SCRAP6 + 5:
            case SCRAP6 + 6:
            case SCRAP6 + 7:
            case SCRAP6 + 8:
            case SCRAP6 + 9:
            case SCRAP6 + 10:
            case SCRAP6 + 11:
            case SCRAP6 + 12:
            case SCRAP6 + 13:
            case SCRAP6 + 14:
            case SCRAP6 + 15:
            case SCRAP6 + 16:
            case SCRAP6 + 17:
            case SCRAP6 + 18:
            case SCRAP6 + 19:
            case SCRAP6 + 20:
            case SCRAP6 + 21:
            case SCRAP6 + 22:
            case SCRAP6 + 23:
            case SCRAP6 + 24:
            case SCRAP6 + 25:
            case SCRAP6 + 26:
            case SCRAP6 + 27:
            case SCRAP6 + 28:
            case SCRAP6 + 29:  // 2419
            case GLASSPIECES:
            case GLASSPIECES + 1:
            case GLASSPIECES + 2:
            case GLASSPIECES + 3:
            case MONEY:
            case MONEY+1:
            case MAIL:
            case MAIL+1:
            case PAPER:
            case PAPER+1:
            case JIBS1:
            case JIBS1 + 1:
            case JIBS1 + 2:
            case JIBS1 + 3:
            case JIBS1 + 4:
            case JIBS2:
            case JIBS2 + 1:
            case JIBS2 + 2:
            case JIBS2 + 3:
            case JIBS2 + 4:
            case JIBS3:
            case JIBS3 + 1:
            case JIBS3 + 2:
            case JIBS3 + 3:
            case JIBS3 + 4:
            case JIBS4:
            case JIBS4 + 1:
            case JIBS4 + 2:
            case JIBS4 + 3:
            case JIBS4 + 4:
            case JIBS5:
            case JIBS5 + 1:
            case JIBS5 + 2:
            case JIBS5 + 3:
            case JIBS5 + 4:
            case JIBS6:
            case JIBS6 + 1:
            case JIBS6 + 2:
            case JIBS6 + 3:
            case JIBS6 + 4:
            case HEADJIB1:
            case HEADJIB1 + 1:
            case HEADJIB1 + 2:
            case HEADJIB1 + 3:
            case ARMJIB1:
            case ARMJIB1 + 1:
            case ARMJIB1 + 2:
            case ARMJIB1 + 3:
            case LEGJIB1:
            case LEGJIB1 + 1:
            case LEGJIB1 + 2:
            case LEGJIB1 + 3:
            case FETUSJIB:
                deletesprite(i);
#if WIN32
                printf("deleting sprite %d, was %d\r\n", i, pn);
#endif
                return;
            default:
                continue;
            }
#else
            if (pn < MAXTILES && pn >= 0)
            {
                if  (freeableSprites[pn / 8] & (1 << (pn % 8)))
                {
                    deletesprite(i);
                    #if WIN32
                        printf("deleting sprite %d, was %d\r\n", i, pn);
                    #endif
                    return;

                }
            }
            else
            {
                #if WIN32
                 printf("pn is %d, larger than numtiles %d\r\n", pn, MAXTILES);
                #endif
            }
#endif
        }
    }
}
/**
 * This function will populate the array (in flash) which determines if a sprite can be freed without worries, e.g. because
 * it is just decorative and temporary. Such as bullet holes, blood, etc.
 */
void initFreeableSprites(void)
{
    #if !USE_OLD_FREE_METHOD
    // use the display buffer to hold the array, whose size is not known.
    // the display buffer is so large we don't really care, it could hold 600k tiles flags!
    freeableSprites = (uint16_t *) getDisplayFrameBuffer(0);
    // clear the buffer, taking care we want to round up to 4 bytes.
    int size = ((MAXTILES + 31) / 32) * 4;
    memset(freeableSprites, 0, size);
    for (unsigned int i = 0; i < sizeof (freeableSpriteList) / sizeof(freeableSpriteList[0]); i++)
    {
        int pn = freeableSpriteList[i];
        freeableSprites[pn / 8] |= 1 << (pn % 8);
    }
    freeableSprites = storeToInternalFlash(freeableSprites, size);
    #endif
}
void resetAllHittypeData(void)
{
    numHittype_otherdata = 0;
    //
#if USE_SEPARATE_HITTYPE_TEMPDATA
    numHittype_tempdata0 = 0;
    numHittype_tempdata1 = 0;
    numHittype_tempdata2 = 0;
    numHittype_tempdata3 = 0;
    numHittype_tempdata45 = 0;

    memset(hittype_tempdata0_allocated_bf, 0, sizeof(hittype_tempdata0_allocated_bf));
    memset(hittype_tempdata1_allocated_bf, 0, sizeof(hittype_tempdata1_allocated_bf));
    memset(hittype_tempdata2_allocated_bf, 0, sizeof(hittype_tempdata2_allocated_bf));
    memset(hittype_tempdata3_allocated_bf, 0, sizeof(hittype_tempdata3_allocated_bf));
    memset(hittype_tempdata45_allocated_bf, 0, sizeof(hittype_tempdata45_allocated_bf));
    memset(hittype_tempdata0, 0, sizeof(hittype_tempdata0));
    memset(hittype_tempdata1, 0, sizeof(hittype_tempdata1));
    memset(hittype_tempdata2, 0, sizeof(hittype_tempdata2));
    memset(hittype_tempdata3, 0, sizeof(hittype_tempdata3));
    memset(hittype_tempdata45, 0, sizeof(hittype_tempdata45));

#else
    numHittype_tempdata = 0;
    memset(hittype_tempdata_allocated_bf, 0, sizeof(hittype_tempdata_allocated_bf));
    memset(hittype_tempdata, 0, sizeof(hittype_tempdata));

#endif // USE_SEPARATE_HITTYPE_TEMPDATA
    //
    memset(hittype_otherdata_allocated_bf, 0, sizeof(hittype_otherdata_allocated_bf));
    memset(hittype_otherdata, 0, sizeof(hittype_otherdata));
#if USE_RAM_HITTYPE
    memset(ramHittype, 0, sizeof(ramHittype));
    memset(hittypeIdx, 0, sizeof(hittypeIdx));
    memset(hittype_allocated_bf, 0, sizeof(hittype_allocated_bf));
    numHittypes = MAXSPRITES - 1;
    hittypeRsvdRomPtr = reserveInternalFlashSize(sizeof (*hittypes) * MAXSPRITES);
#endif

}
#define define_hittype_data_allocators(type) \
void freeHittype_##type(int hittype_index) \
{ \
    int index = getHittype(hittype_index)->type##_idx; \
    if (index) /* was allocated ? free it!*/ \
    { \
            numHittype_##type--; \
            hittype_##type##_allocated_bf[index / 8] &= ~(1 << (index % 8));\
            /*clear it (not needed but let's do it!)*/ \
            memset(&hittype_##type[index], 0, sizeof(hittype_##type[index]));\
            setHittype_##type##_idx(hittype_index, 0);    /* not needed, would be cleared later.*/ \
            /*printf("Freed %s Data for hittype[%d], old index %d. #occupied now = %d\r\n", #type,  hittype_index, index, numHittype_##type);*/ \
    }\
}\
static unsigned short hittype_lastIndex_##type = 0; \
void resetAllocHittype_lastIndex_##type()\
{ \
  hittype_lastIndex_##type = 0;  \
} \
int allocateNewHittype_##type(uint32_t hittype_index) \
{ \
    int wrapped = 0; /* if 1 then we have already tried one full cycle to find an available spot*/ \
    /* we cannot use 0, because at index 0 we must have all-0 data */\
    if (hittype_lastIndex_##type == 0) /* prevent 0 value*/ \
        hittype_lastIndex_##type = 1; \
    unsigned int startIndex = hittype_lastIndex_##type; \
    do \
    { \
        hittype_lastIndex_##type++; \
        if (hittype_lastIndex_##type >= sizeof(hittype_##type) / sizeof(hittype_##type[0])) /*roll over */ \
        { \
            hittype_lastIndex_##type = 1;\
        } \
        if (!(hittype_##type##_allocated_bf[hittype_lastIndex_##type / 8] & (1 << (hittype_lastIndex_##type % 8)))) \
        { \
            numHittype_##type++; \
            hittype_##type##_allocated_bf[hittype_lastIndex_##type / 8] |= (1 << (hittype_lastIndex_##type % 8));\
            /* make a clean start*/ \
            memset(&hittype_##type[hittype_lastIndex_##type], 0, sizeof(hittype_##type[hittype_lastIndex_##type]));\
            setHittype_##type##_idx(hittype_index, hittype_lastIndex_##type); \
            /*if (!strcmp(#type, "tempdata0")) printf("Allocated %s # %d for hittype[%d], new index %d\r\n", #type, numHittype_##type, hittype_index, hittype_lastIndex_##type);*/\
            break;\
        }\
        if (hittype_lastIndex_##type == startIndex)\
        { \
            if (wrapped > 1)  \
            {  \
                FIXME("Error, cannot find an empty slot for %s\r\n", #type);\
                EXIT(0);\
            }\
            wrapped++;\
            freeUnnecessarySprites(hittype_index, HITTYPE_FEATURE_ID_##type);\
        }\
    } while (1); \
    return hittype_lastIndex_##type;\
}
#if USE_SEPARATE_HITTYPE_TEMPDATA
define_hittype_data_allocators(tempdata0);
define_hittype_data_allocators(tempdata1);
define_hittype_data_allocators(tempdata2);
define_hittype_data_allocators(tempdata3);
define_hittype_data_allocators(tempdata45);
#else
define_hittype_data_allocators(tempdata);
#endif
define_hittype_data_allocators(otherdata);
#if USE_RAM_HITTYPE
void freeHittype(int hittype_index)
{
    int index = hittypeIdx[hittype_index];
    if (index) /* was allocated ? free it!*/
    {
            numHittypes--;
            hittype_allocated_bf[index / 8] &= ~(1 << (index % 8));
            /*clear it (not needed but let's do it!)*/
            memset(&hittypeRamPtr[index], 0, sizeof(hittypes[0]));
            hittypeIdx[hittype_index] = 0;
#if WIN32 && 0
            printf("freeing hittype %d, total %d\r\n", hittype_index, numHittypes);
#endif
    }
}
static unsigned short ramHittypeLastIndex = 0;
void resetAllocRamHittypeLastIndex(void)
{
    ramHittypeLastIndex = 0;
}
int allocateNewHittype(int hittype_index)
{
    int wrapped = 0; /* if 1 then we have already tried one full cycle to find an available spot*/ \
    /* we cannot use 0, because at index 0 we must have all-0 data */
    if (ramHittypeLastIndex == 0) /* prevent 0 value*/
        ramHittypeLastIndex = 1;
    int startIndex = ramHittypeLastIndex;
    do
    {
        ramHittypeLastIndex++;
        if (ramHittypeLastIndex >= sizeof(ramHittype) / sizeof(ramHittype[0])) /*roll over */
        {
            ramHittypeLastIndex = 1;
        }
        if (!(hittype_allocated_bf[ramHittypeLastIndex / 8] & (1 << (ramHittypeLastIndex % 8))))
        {
            numHittypes++;
            hittype_allocated_bf[ramHittypeLastIndex / 8] |= (1 << (ramHittypeLastIndex % 8));
            hittypeIdx[hittype_index] = ramHittypeLastIndex;
            //
            memcpy(&hittypeRamPtr[ramHittypeLastIndex], &hittypes[hittype_index], sizeof (hittypes[hittype_index]));
#if WIN32 && 0
            printf("Allocating hittype %d,  total %d\r\n", hittype_index,numHittypes);
#endif
            break;
        }
        if (ramHittypeLastIndex == startIndex)
        {
            if (wrapped > 1)
            {
                FIXME("Error, cannot find an empty slot for hittype\r\n");
                EXIT(0);
            }
            wrapped++;\
            freeUnnecessarySprites(hittype_index, HITTYPE_FEATURE_ID_NONE);\
        }
    } while (1);
    return ramHittypeLastIndex;
}

#endif // USE_RAM_HITTYPE
short getincangle(short a,short na)
{
    a &= 2047;
    na &= 2047;

    if(klabs(a-na) < 1024)
        return (na-a);
    else
    {
        if(na > 1024) na -= 2048;
        if(a > 1024) a -= 2048;

        na -= 2048;
        a -= 2048;
        return (na-a);
    }
}

uint8_t  ispecial(uint8_t  c)
{
    if(c == 0x0a)
    {
        line_number++;
        return 1;
    }

    if(c == ' ' || c == 0x0d)
        return 1;

    return 0;
}

uint8_t  isaltok(uint8_t  c)
{
    return ( isalnum(c) || c == '{' || c == '}' || c == '/' || c == '*' || c == '-' || c == '_' || c == '.');
}

void getglobalz(short i)
{
    int32_t hz,lz,zr;

    if( getSprite(i)->_statnum == 10 || getSprite(i)->_statnum == 6 || getSprite(i)->_statnum == 2 || getSprite(i)->_statnum == 1 || getSprite(i)->_statnum == 4)
    {
        if(getSprite(i)->_statnum == 4)
            zr = 4L;
        else zr = 127L;
        // next-hack: added hittype_ceilingz, hittype_floorz so we can use bitfields.
        int32_t hittype_ceilingz = getHittype(i)->ceilingz;
        int32_t hittype_floorz = getHittype(i)->floorz;
        getzrange(getSprite(i)->_x, getSprite(i)->_y, getSprite(i)->_z-(FOURSLEIGHT), getSprite(i)->_sectnum, &hittype_ceilingz,&hz,&hittype_floorz,&lz,zr,CLIPMASK0);
        setHittype_floorz(i, hittype_floorz);
        setHittype_ceilingz(i, hittype_ceilingz);
#if !NEW_MAXSPRITES
        if( (lz&49152) == 49152 && (sprite[lz&(MAXSPRITES-1)].cstat&48) == 0 )
#else
        CHECK_SPRITE(lz);
        if( (lz&49152) == 49152 && (getSprite(lz & MAX_SPRITESMASK)->_cstat & 48) == 0 )
#endif
        {
#if !NEW_MAXSPRITES
           lz &= (MAXSPRITES-1);
#else
            CHECK_SPRITE(lz);
            lz &= MAX_SPRITESMASK;
#endif
            if( badguy(getSprite(lz)) && getSprite(lz)->_pal != 1)
            {
                if( getSprite(i)->_statnum != 4 )
                {
                    setHittype_dispicnum(i, -4); // No shadows on actors
                    setSprite_xvel(i, -256);
                    ssp(i,CLIPMASK0);
                }
            }
            else if(getSprite(lz)->_picnum == APLAYER && badguy(getSprite(i)) )
            {
                setHittype_dispicnum(i, -4); // No shadows on actors
                setSprite_xvel(i, -256);
                ssp(i,CLIPMASK0);
            }
            else if(getSprite(i)->_statnum == 4 && getSprite(lz)->_picnum == APLAYER)
                if(getSprite(i)->_owner == lz)
            {
                setHittype_ceilingz (i, getSector(getSprite(i)->_sectnum)->ceilingz);
                setHittype_floorz(i, getSector(getSprite(i)->_sectnum)->floorz);
            }
        }
    }
    else
    {
        setHittype_ceilingz (i, getSector(getSprite(i)->_sectnum)->ceilingz);
        setHittype_floorz(i, getSector(getSprite(i)->_sectnum)->floorz);
    }
}


void makeitfall(short i)
{
//    const spritetype *getSprite(i) = getSprite(i);
    int32_t hz,lz,c;

    if( floorspace(getSprite(i)->_sectnum) )
        c = 0;
    else
    {
        if( ceilingspace(getSprite(i)->_sectnum) || getSector(getSprite(i)->_sectnum)->lotag == 2)
            c = gc/6;
        else c = gc;
    }

    if( ( getSprite(i)->_statnum == 1 || getSprite(i)->_statnum == 10 || getSprite(i)->_statnum == 2 || getSprite(i)->_statnum == 6 ) )
    {
        // next-hack: added hittype_ceilingz, hittype_floorz so we can use bitfields.
        int32_t hittype_ceilingz = getHittype(i)->ceilingz;
        int32_t hittype_floorz = getHittype(i)->floorz;
        getzrange(getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z-(FOURSLEIGHT),getSprite(i)->_sectnum,&hittype_ceilingz,&hz,&hittype_floorz,&lz,127L,CLIPMASK0);
        setHittype_floorz(i, hittype_floorz);
        setHittype_ceilingz(i, hittype_ceilingz);
    }
    else
    {
        setHittype_ceilingz (i, getSector(getSprite(i)->_sectnum)->ceilingz);
        setHittype_floorz(i, getSector(getSprite(i)->_sectnum)->floorz);
    }

    if( getSprite(i)->_z < getHittype(i)->floorz-(FOURSLEIGHT) )
    {
        if( getSector(getSprite(i)->_sectnum)->lotag == 2 && getSprite(i)->_zvel > 3122 )
            setSprite_zvel(i, 3144);
        if(getSprite(i)->_zvel < 6144)
            setSprite_zvel(i, getSprite_zvel(i) + c);
        else
            setSprite_zvel(i,  6144);
        setSprite_z(i, getSprite(i)->_z + getSprite(i)->_zvel);   // check this!
    }
    if( getSprite(i)->_z >= getHittype(i)->floorz-(FOURSLEIGHT) )
    {
        setSprite_z(i, getHittype(i)->floorz - FOURSLEIGHT);
        setSprite_zvel(i, 0);
    }
}


void getlabel(void)
{
    int32_t i;

    while( isalnum((int)*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        textptr++;
        if( *textptr == 0)
            return;
    }

    i = 0;
    while( ispecial(*textptr) == 0 )
        label[(labelcnt<<6)+i++] = *(textptr++);

    label[(labelcnt<<6)+i] = 0;
}
#if LOAD_FROM_CON_FILE
int32_t keyword(void)
{
    int32_t i;
    char  *temptextptr;

    temptextptr = textptr;

    while( isaltok(*temptextptr) == 0 )
    {
        temptextptr++;
        if( *temptextptr == 0 )
            return 0;
    }

    i = 0;
    while( isaltok(*temptextptr) )
    {
        tempbuf[i] = *(temptextptr++);
        i++;
    }
    tempbuf[i] = 0;

    for(i=0;i<NUMKEYWORDS;i++)
        if( strcmp( (const char *)tempbuf,keyw[i]) == 0 )
            return i;

    return -1;
}

int32_t transword(void) //Returns its code #
{
    int32_t i, l;

    while( isaltok(*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        if( *textptr == 0 )
            return -1;
        textptr++;
    }

    l = 0;
    while( isaltok(*(textptr+l)) )
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    for(i=0;i<NUMKEYWORDS;i++)
    {
        if( strcmp( (const char *)tempbuf,keyw[i]) == 0 )
        {
            *scriptptr = i;
            textptr += l;
            scriptptr++;
            return i;
        }
    }

    textptr += l;

    if( tempbuf[0] == '{' && tempbuf[1] != 0)
        printf("  * ERROR!(L%hd) Expecting a SPACE or CR between '{' and '%s'.\n",line_number,tempbuf+1);
    else if( tempbuf[0] == '}' && tempbuf[1] != 0)
        printf("  * ERROR!(L%hd) Expecting a SPACE or CR between '}' and '%s'.\n",line_number,tempbuf+1);
    else if( tempbuf[0] == '/' && tempbuf[1] == '/' && tempbuf[2] != 0 )
        printf("  * ERROR!(L%hd) Expecting a SPACE between '//' and '%s'.\n",line_number,tempbuf+2);
    else if( tempbuf[0] == '/' && tempbuf[1] == '*' && tempbuf[2] != 0 )
        printf("  * ERROR!(L%hd) Expecting a SPACE between '/*' and '%s'.\n",line_number,tempbuf+2);
    else if( tempbuf[0] == '*' && tempbuf[1] == '/' && tempbuf[2] != 0 )
        printf("  * ERROR!(L%hd) Expecting a SPACE between '*/' and '%s'.\n",line_number,tempbuf+2);
    else printf("  * ERROR!(L%hd) Expecting key word, but found '%s'.\n",line_number,tempbuf);

    error++;
    return -1;
}

void transnum(void)
{
    int32_t i, l;

    while( isaltok(*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        textptr++;
        if( *textptr == 0 )
            return;
    }


    l = 0;
    while( isaltok(*(textptr+l)) )
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    for(i=0;i<NUMKEYWORDS;i++)
        if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
    {
        error++;
        printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
        textptr+=l;
    }


    for(i=0;i<labelcnt;i++)
    {
        if( strcmp((const char *)tempbuf,label+(i<<6)) == 0 )
        {
            *scriptptr = labelcode[i];
            scriptptr++;
            textptr += l;
            return;
        }
    }

    if( isdigit(*textptr) == 0 && *textptr != '-')
    {
        printf("  * ERROR!(L%hd) Parameter '%s' is undefined.\n",line_number,tempbuf);
        error++;
        textptr+=l;
        return;
    }

    *scriptptr = atol(textptr);
    scriptptr++;

    textptr += l;
}
#endif
/**
 * Encode a scriptptr into a form suitable for portably
 * inserting into bytecode. We store the pointer as minus
 * the offset from the start of the script buffer, just
 * to make it perhaps a little more obvious what is happening.
 */
int32_t encodescriptptr(const int32_t* scptr)
{
    int32_t offs = (int32_t)(scptr - script);

	assert(offs >= 0);
    assert(offs < MAXSCRIPTSIZE);

	return -offs;
}
/* next-hack: added these functions to use indexes instead of pointers.
*/

int32_t getscriptptridx(const int32_t* scptr)
{
    int32_t offs = (int32_t)(scptr - script);

	assert(offs >= 0);
    assert(offs < MAXSCRIPTSIZE);
    // add 1 so that we can use idx 0 as null pointer
	return offs + 1;
}
const int32_t* getscriptptr(int16_t idx)
{
    if (idx == 0)
        return NULL;
    return &script[idx - 1];
}

/**
 * Decode an encoded representation of a scriptptr
 */
const int32_t* decodescriptptr(int32_t scptr)
{
    assert(scptr <= 0);

	int32_t offs = -scptr;

	assert(offs >= 0);
    assert(offs < MAXSCRIPTSIZE);

	return script + offs;
}
#if LOAD_FROM_CON_FILE
uint8_t parsecommand(int readfromGRP)
{
    #ifdef WIN32
        static int maxActorIndex;
    #endif // WIN32
    int32_t i, j, k, *tempscrptr;
    uint8_t  done, temp_ifelse_check;
    int32_t tw;
    char *origtptr;
    short temp_line_number;
    int fp;

    if( error > 12 || ( *textptr == '\0' ) || ( *(textptr+1) == '\0' ) ) return 1;

    tw = transword();

    switch(tw)
    {
        default:
        case -1:
            return 0; //End
        case 39:      //Rem endrem
            scriptptr--;
            j = line_number;
            do
            {
                textptr++;
                if(*textptr == 0x0a) line_number++;
                if( *textptr == 0 )
                {
                    printf("  * ERROR!(L%d) Found '/*' with no '*/'.\n",j);
                    error++;
                    return 0;
                }
            }
            while( *textptr != '*' || *(textptr+1) != '/' );
            textptr+=2;
            return 0;
        case 17:
            if( parsing_actor == 0 && parsing_state == 0 )
            {
                getlabel();
                scriptptr--;
                labelcode[labelcnt] = encodescriptptr(scriptptr);
                labelcnt++;

                parsing_state = 1;

                return 0;
            }

            getlabel();

            for(i=0;i<NUMKEYWORDS;i++)
                if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                {
                    error++;
                    printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
                    return 0;
                }

            for(j=0;j<labelcnt;j++)
            {
                if( strcmp(label+(j<<6),label+(labelcnt<<6)) == 0 )
                {
                    *scriptptr = labelcode[j];
                    break;
                }
            }

            if(j==labelcnt)
            {
                printf("  * ERROR!(L%hd) State '%s' not found.\n",line_number,label+(labelcnt<<6));
                error++;
            }
            scriptptr++;
            return 0;

        case 15:
        case 92:
        case 87:
        case 89:
        case 93:
            transnum();
            return 0;

        case 18:
            if( parsing_state == 0 )
            {
                printf("  * ERROR!(L%hd) Found 'ends' with no 'state'.\n",line_number);
                error++;
            }
//            else
            {
                if( num_squigilly_brackets > 0 )
                {
                    printf("  * ERROR!(L%hd) Found more '{' than '}' before 'ends'.\n",line_number);
                    error++;
                }
                if( num_squigilly_brackets < 0 )
                {
                    printf("  * ERROR!(L%hd) Found more '}' than '{' before 'ends'.\n",line_number);
                    error++;
                }
                parsing_state = 0;
            }
            return 0;
        case 19:
            getlabel();
            // Check to see it's already defined

            for(i=0;i<NUMKEYWORDS;i++)
                if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                {
                    error++;
                    printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
                    return 0;
                }

            for(i=0;i<labelcnt;i++)
            {
                if( strcmp(label+(labelcnt<<6),label+(i<<6)) == 0 )
                {
                    warning++;
                    printf("  * WARNING.(L%hd) Duplicate definition '%s' ignored.\n",line_number,label+(labelcnt<<6));
                    break;
                }
            }

            transnum();
            if(i == labelcnt)
                labelcode[labelcnt++] = *(scriptptr-1);
            scriptptr -= 2;
            return 0;
        case 14:

            for(j = 0;j < 4;j++)
            {
                if( keyword() == -1 )
                    transnum();
                else break;
            }

            while(j < 4)
            {
                *scriptptr = 0;
                scriptptr++;
                j++;
            }
            return 0;

        case 32:
            if( parsing_actor || parsing_state )
            {
                transnum();

                j = 0;
                while(keyword() == -1)
                {
                    transnum();
                    scriptptr--;
                    j |= *scriptptr;
                }
                *scriptptr = j;
                scriptptr++;
            }
            else
            {
                scriptptr--;
                getlabel();
                // Check to see it's already defined

                for(i=0;i<NUMKEYWORDS;i++)
                    if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                {
                    error++;
                    printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
                    return 0;
                }

                for(i=0;i<labelcnt;i++)
                    if( strcmp(label+(labelcnt<<6),label+(i<<6)) == 0 )
                    {
                        warning++;
                        printf("  * WARNING.(L%hd) Duplicate move '%s' ignored.\n",line_number,label+(labelcnt<<6));
                        break;
                    }
                if(i == labelcnt)
                    labelcode[labelcnt++] = encodescriptptr(scriptptr);
                for(j=0;j<2;j++)
                {
                    if(keyword() >= 0) break;
                    transnum();
                }
                for(k=j;k<2;k++)
                {
                    *scriptptr = 0;
                    scriptptr++;
                }
            }
            return 0;

        case 54:
            {
                scriptptr--;
                transnum(); // Volume Number (0/4)
                scriptptr--;

                k = *scriptptr-1;

                if(k >= 0) // if it's background music
                {
                    i = 0;
                    while(keyword() == -1)
                    {
                        while( isaltok(*textptr) == 0 )
                        {
                            if(*textptr == 0x0a) line_number++;
                            textptr++;
                            if( *textptr == 0 ) break;
                        }
                        j = 0;
                        while( isaltok(*(textptr+j)) )
                        {
                            music_fn[k][i][j] = textptr[j];
                            j++;
                        }
                        music_fn[k][i][j] = '\0';
                        textptr += j;
                        if(i > 9) break;
                        i++;
                    }
                }
                else
                {
                    i = 0;
                    while(keyword() == -1)
                    {
                        while( isaltok(*textptr) == 0 )
                        {
                            if(*textptr == 0x0a) line_number++;
                            textptr++;
                            if( *textptr == 0 ) break;
                        }
                        j = 0;
                        while( isaltok(*(textptr+j)) )
                        {
                            env_music_fn[i][j] = textptr[j];
                            j++;
                        }
                        env_music_fn[i][j] = '\0';

                        textptr += j;
                        if(i > 9) break;
                        i++;
                    }
                }
            }
            return 0;
        case 55: // include other con files.
			{
				char  includedconfile[512];
				scriptptr--;
				while( isaltok(*textptr) == 0 )
				{
					if(*textptr == 0x0a) line_number++;
					textptr++;
					if( *textptr == 0 ) break;
				}
				j = 0;
				while( isaltok(*textptr) )
				{
					tempbuf[j] = *(textptr++);
					j++;
				}
				tempbuf[j] = '\0';

				// fix path for unix. (doesn't really matter...)
				FixFilePath((char *)tempbuf);

				sprintf(includedconfile, "%s", tempbuf);
#if !USE_EXT_MEM
				fp = TCkopen4load(includedconfile,readfromGRP);
				if(fp <= 0)
				{
					error++;
					printf("  * ERROR!(ln%hd) Could not find '%s'.\n",line_number,label+(labelcnt<<6));

					printf("ERROR: could not open (%s)\n", includedconfile);
					gameexit("");
					return 0;
				}

				j = kfilelength(fp);

				printf("Including: '%s'.\n", includedconfile);

				temp_line_number = line_number;
				line_number = 1;
				temp_ifelse_check = checking_ifelse;
				checking_ifelse = 0;
				origtptr = textptr;
				textptr = last_used_text+last_used_size;

				*(textptr+j) = 0;

				kread(fp,(uint8_t  *)textptr,j);
				kclose(fp);
//				ud.conCRC[0] = crc32_update((uint8_t  *)textptr, j, ud.conCRC[0]);
#else
                void *ptr = kGetExtMemAddress(includedconfile, &j);

				printf("Including: '%s'.\n", includedconfile);

				temp_line_number = line_number;
				line_number = 1;
				temp_ifelse_check = checking_ifelse;
				checking_ifelse = 0;
				origtptr = textptr;
				textptr = last_used_text+last_used_size;

				*(textptr+j) = 0;

				extMemGetDataFromAddress(textptr, ptr, j);
#endif

				do
					done = parsecommand(readfromGRP);
				while( done == 0 );

				textptr = origtptr;
				total_lines += line_number;
				line_number = temp_line_number;
				checking_ifelse = temp_ifelse_check;

				return 0;
			}
        case 24:
            if( parsing_actor || parsing_state )
                transnum();
            else
            {
                scriptptr--;
                getlabel();

                for(i=0;i<NUMKEYWORDS;i++)
                    if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                    {
                        error++;
                        printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
                        return 0;
                    }

                for(i=0;i<labelcnt;i++)
                    if( strcmp(label+(labelcnt<<6),label+(i<<6)) == 0 )
                    {
                        warning++;
                        printf("  * WARNING.(L%hd) Duplicate ai '%s' ignored.\n",line_number,label+(labelcnt<<6));
                        break;
                    }

                if(i == labelcnt)
                    labelcode[labelcnt++] = encodescriptptr(scriptptr);

                for(j=0;j<3;j++)
                {
                    if(keyword() >= 0) break;
                    if(j == 2)
                    {
                        k = 0;
                        while(keyword() == -1)
                        {
                            transnum();
                            scriptptr--;
                            k |= *scriptptr;
                        }
                        *scriptptr = k;
                        scriptptr++;
                        return 0;
                    }
                    else transnum();
                }
                for(k=j;k<3;k++)
                {
                    *scriptptr = 0;
                    scriptptr++;
                }
            }
            return 0;

        case 7:
            if( parsing_actor || parsing_state )
                transnum();
            else
            {
                scriptptr--;
                getlabel();
                // Check to see it's already defined

                for(i=0;i<NUMKEYWORDS;i++)
                    if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                    {
                        error++;
                        printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
                        return 0;
                    }

                for(i=0;i<labelcnt;i++)
                    if( strcmp(label+(labelcnt<<6),label+(i<<6)) == 0 )
                    {
                        warning++;
                        printf("  * WARNING.(L%hd) Duplicate action '%s' ignored.\n",line_number,label+(labelcnt<<6));
                        break;
                    }

                if(i == labelcnt)
                    labelcode[labelcnt++] = encodescriptptr(scriptptr);

                for(j=0;j<5;j++)
                {
                    if(keyword() >= 0) break;
                    transnum();
                }
                for(k=j;k<5;k++)
                {
                    *scriptptr = 0;
                    scriptptr++;
                }
            }
            return 0;

        case 1:
            if( parsing_state )
            {
                printf("  * ERROR!(L%hd) Found 'actor' within 'state'.\n",line_number);
                error++;
            }

            if( parsing_actor )
            {
                printf("  * ERROR!(L%hd) Found 'actor' within 'actor'.\n",line_number);
                error++;
            }

            num_squigilly_brackets = 0;
            scriptptr--;
            parsing_actor = scriptptr;

            transnum();
            scriptptr--;
#if USE_ACTORSCRPTR
            actorscrptr[*scriptptr] = parsing_actor;
#else
            actorscridx[*scriptptr] = getscriptptridx(parsing_actor);
#endif
#ifdef WIN32
            if (*scriptptr > maxActorIndex)
            {
                maxActorIndex = *scriptptr;
                printf("Max Actor Index: %d\r\n", maxActorIndex);
            }
#endif // WIN32
            for(j=0;j<4;j++)
            {
                *(parsing_actor+j) = 0;
                if(j == 3)
                {
                    j = 0;
                    while(keyword() == -1)
                    {
                        transnum();
                        scriptptr--;
                        j |= *scriptptr;
                    }
                    *scriptptr = j;
                    scriptptr++;
                    break;
                }
                else
                {
                    if(keyword() >= 0)
                    {
                        scriptptr += (4-j);
                        break;
                    }
                    transnum();

                    *(parsing_actor+j) = *(scriptptr-1);
                }
            }

            checking_ifelse = 0;

            return 0;

        case 98:

            if( parsing_state )
            {
                printf("  * ERROR!(L%hd) Found 'useritem' within 'state'.\n",line_number);
                error++;
            }

            if( parsing_actor )
            {
                printf("  * ERROR!(L%hd) Found 'useritem' within 'actor'.\n",line_number);
                error++;
            }

            num_squigilly_brackets = 0;
            scriptptr--;
            parsing_actor = scriptptr;

            transnum();
            scriptptr--;
            j = *scriptptr;

            transnum();
            scriptptr--;
#if USE_ACTORSCRPTR
            actorscrptr[*scriptptr] = parsing_actor;
#else
            actorscridx[*scriptptr] = getscriptptridx(parsing_actor);
#endif
            actortype[*scriptptr] = j;
#ifdef WIN32
            if (*scriptptr > maxActorIndex)
            {
                maxActorIndex = *scriptptr;
                printf("Max Actor Index: %d\r\n", maxActorIndex);
            }
#endif // WIN32

            for(j=0;j<4;j++)
            {
                *(parsing_actor+j) = 0;
                if(j == 3)
                {
                    j = 0;
                    while(keyword() == -1)
                    {
                        transnum();
                        scriptptr--;
                        j |= *scriptptr;
                    }
                    *scriptptr = j;
                    scriptptr++;
                    break;
                }
                else
                {
                    if(keyword() >= 0)
                    {
                        scriptptr += (4-j);
                        break;
                    }
                    transnum();

                    *(parsing_actor+j) = *(scriptptr-1);
                }
            }

            checking_ifelse = 0;

            return 0;



        case 11:
        case 13:
        case 25:
        case 31:
        case 40:
        case 52:
        case 69:
        case 74:
        case 77:
        case 80:
        case 86:
        case 88:
        case 68:
        case 100:
        case 101:
        case 102:
        case 103:
        case 105:
        case 110:
            transnum();
            return 0;

        case 2:
        case 23:
        case 28:
        case 99:
        case 37:
        case 48:
        case 58:
            transnum();
            transnum();
            break;
        case 50:
            transnum();
            transnum();
            transnum();
            transnum();
            transnum();
            break;
        case 10:
            if( checking_ifelse )
            {
                checking_ifelse--;
                tempscrptr = scriptptr;
                scriptptr++; //Leave a spot for the fail location
                parsecommand(readfromGRP);
                *tempscrptr = encodescriptptr(scriptptr);
            }
            else
            {
                scriptptr--;
                error++;
                printf("  * ERROR!(L%hd) Found 'else' with no 'if'.\n",line_number);
            }

            return 0;

        case 75:
            transnum();
        case 3:
        case 8:
        case 9:
        case 21:
        case 33:
        case 34:
        case 35:
        case 41:
        case 46:
        case 53:
        case 56:
        case 59:
        case 62:
        case 72:
        case 73:
//        case 74:
        case 78:
        case 85:
        case 94:
        case 111:
            transnum();
        case 43:
        case 44:
        case 49:
        case 5:
        case 6:
        case 27:
        case 26:
        case 45:
        case 51:
        case 63:
        case 64:
        case 65:
        case 67:
        case 70:
        case 71:
        case 81:
        case 82:
        case 90:
        case 91:
        case 109:

            if(tw == 51)
            {
                j = 0;
                do
                {
                    transnum();
                    scriptptr--;
                    j |= *scriptptr;
                }
                while(keyword() == -1);
                *scriptptr = j;
                scriptptr++;
            }

            tempscrptr = scriptptr;
            scriptptr++; //Leave a spot for the fail location

            do
            {
                j = keyword();
                if(j == 20 || j == 39)
                    parsecommand(readfromGRP);
            } while(j == 20 || j == 39);

            parsecommand(readfromGRP);

            *tempscrptr = encodescriptptr(scriptptr);

            checking_ifelse++;
            return 0;
        case 29:
            num_squigilly_brackets++;
            do
                done = parsecommand(readfromGRP);
            while( done == 0 );
            return 0;
        case 30:
            num_squigilly_brackets--;
            if( num_squigilly_brackets < 0 )
            {
                printf("  * ERROR!(L%hd) Found more '}' than '{'.\n",line_number);
                error++;
            }
            return 1;
        case 76:
            scriptptr--;
            j = 0;
            while( *textptr != 0x0a )
            {
                #if USE_BETANAME
                    betaname[j] = *textptr;
                #endif
                j++; textptr++;
            }
            #if USE_BETANAME
                betaname[j] = 0;
            #endif
            return 0;
        case 20:
            scriptptr--; //Negate the rem
            while( *textptr != 0x0a )
                textptr++;

            // line_number++;
            return 0;

        case 107:
            scriptptr--;
            transnum();
            scriptptr--;
            j = *scriptptr;
            while( *textptr == ' ' ) textptr++;

            i = 0;

            while( *textptr != 0x0a )
            {
                volume_names[j][i] = toupper(*textptr);
                textptr++,i++;
                if(i >= 32)
                {
                    printf("  * ERROR!(L%hd) Volume name exceeds character size limit of 32.\n",line_number);
                    error++;
                    while( *textptr != 0x0a ) textptr++;
                    break;
                }
            }
#ifdef UNIX
            volume_names[j][i] = '\0';
#else
            volume_names[j][i-1] = '\0';
#endif
            return 0;
        case 108:
            scriptptr--;
            transnum();
            scriptptr--;
            j = *scriptptr;
            while( *textptr == ' ' ) textptr++;

            i = 0;

            while( *textptr != 0x0a )
            {
                skill_names[j][i] = toupper(*textptr);
                textptr++,i++;
                if(i >= 32)
                {
                    printf("  * ERROR!(L%hd) Skill name exceeds character size limit of 32.\n",line_number);
                    error++;
                    while( *textptr != 0x0a ) textptr++;
                    break;
                }
            }
#ifdef UNIX
            skill_names[j][i] = '\0';
#else
            skill_names[j][i-1] = '\0';
#endif
            return 0;

        case 0:
            scriptptr--;
            transnum();
            scriptptr--;
            j = *scriptptr;
            transnum();
            scriptptr--;
            k = *scriptptr;
            while( *textptr == ' ' ) textptr++;

            i = 0;
            while( *textptr != ' ' && *textptr != 0x0a )
            {
                level_file_names[j*11+k][i] = *textptr;
                textptr++,i++;
                if(i > 127)
                {
                    printf("  * ERROR!(L%hd) Level file name exceeds character size limit of 128.\n",line_number);
                    error++;
                    while( *textptr != ' ') textptr++;
                    break;
                }
            }
#ifdef UNIX
            level_names[j*11+k][i] = '\0';
#else
            level_names[j*11+k][i-1] = '\0';
#endif

            while( *textptr == ' ' ) textptr++;

            partime[j*11+k] =
                (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*26*60)+
                (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*26);

            textptr += 5;
            while( *textptr == ' ' ) textptr++;

            designertime[j*11+k] =
                (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*26*60)+
                (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*26);

            textptr += 5;
            while( *textptr == ' ' ) textptr++;

            i = 0;

            while( *textptr != 0x0a )
            {
                level_names[j*11+k][i] = toupper(*textptr);
                textptr++,i++;
                if(i >= 32)
                {
                    printf("  * ERROR!(L%hd) Level name exceeds character size limit of 32.\n",line_number);
                    error++;
                    while( *textptr != 0x0a ) textptr++;
                    break;
                }
            }
#ifdef UNIX
            level_names[j*11+k][i] = '\0';
#else
            level_names[j*11+k][i-1] = '\0';
#endif
            return 0;

        case 79:
            scriptptr--;
            transnum();
            k = *(scriptptr-1);
            if(k >= NUMOFFIRSTTIMEACTIVE)
            {
                printf("  * ERROR!(L%hd) Quote amount exceeds limit of %d characters.\n",line_number,NUMOFFIRSTTIMEACTIVE);
                error++;
            }
            scriptptr--;
            i = 0;
            while( *textptr == ' ' )
                textptr++;

            while( *textptr != 0x0a )
            {
                fta_quotes[k][i] = *textptr;
                textptr++,i++;
                if(i >= 64)
                {
                    printf("  * ERROR!(L%hd) Quote exceeds character size limit of 64.\n",line_number);
                    error++;
                    while( *textptr != 0x0a ) textptr++;
                    break;
                }
            }
            fta_quotes[k][i] = '\0';
            return 0;
        case 57:
            scriptptr--;
            transnum();
            k = *(scriptptr-1);
            if(k >= NUM_SOUNDS)
            {
                printf("  * ERROR!(L%hd) Exceeded sound limit of %d.\n",line_number,NUM_SOUNDS);
                error++;
            }
            scriptptr--;
            i = 0;
            while( *textptr == ' ')
                textptr++;

            while( *textptr != ' ' )
            {
                sounds[k][i] = *textptr;
                textptr++,i++;
                if(i >= 13)
                {
                    puts(sounds[k]);
                    printf("  * ERROR!(L%hd) Sound filename exceeded limit of 13 characters.\n",line_number);
                    error++;
                    while( *textptr != ' ' ) textptr++;
                    break;
                }
            }
            sounds[k][i] = '\0';

            transnum();
            soundps[k] = *(scriptptr-1);
            scriptptr--;
            transnum();
            soundpe[k] = *(scriptptr-1);
            scriptptr--;
            transnum();
            soundpr[k] = *(scriptptr-1);
            scriptptr--;
            transnum();
            soundm[k] = *(scriptptr-1);
            scriptptr--;
            transnum();
            soundvo[k] = *(scriptptr-1);
            scriptptr--;
            return 0;

        case 4:
            if( parsing_actor == 0 )
            {
                printf("  * ERROR!(L%hd) Found 'enda' without defining 'actor'.\n",line_number);
                error++;
            }
//            else
            {
                if( num_squigilly_brackets > 0 )
                {
                    printf("  * ERROR!(L%hd) Found more '{' than '}' before 'enda'.\n",line_number);
                    error++;
                }
                parsing_actor = 0;
            }

            return 0;
        case 12:
        case 16:
        case 84:
//        case 21:
        case 22:    //KILLIT
        case 36:
        case 38:
        case 42:
        case 47:
        case 61:
        case 66:
        case 83:
        case 95:
        case 96:
        case 97:
        case 104:
        case 106:
            return 0;
        case 60:
		{
			int32_t params[30];

			scriptptr--;
			for(j = 0; j < 30; j++)
        	{
				transnum();
				scriptptr--;
				params[j] = *scriptptr;

				if (j != 25) continue; // we try to guess if we are using 1.3/1.3d or 1.4/1.5 con files

				if (keyword() != -1) // Is the 26th variable set? If so then it's probably a 1.4/1.5 con file
				{
					break;
				}
				else
				{
					conVersion = 15;
				}
			}

			/* From Jonathon's code --mk
			v1.3d					v1.5

			DEFAULTVISIBILITY		DEFAULTVISIBILITY
			GENERICIMPACTDAMAGE		GENERICIMPACTDAMAGE
			MAXPLAYERHEALTH			MAXPLAYERHEALTH
			STARTARMORHEALTH		STARTARMORHEALTH
			RESPAWNACTORTIME		RESPAWNACTORTIME
			RESPAWNITEMTIME			RESPAWNITEMTIME
			RUNNINGSPEED			RUNNINGSPEED
									GRAVITATIONALCONSTANT
			RPGBLASTRADIUS			RPGBLASTRADIUS
			PIPEBOMBRADIUS			PIPEBOMBRADIUS
			SHRINKERBLASTRADIUS		SHRINKERBLASTRADIUS
			TRIPBOMBBLASTRADIUS		TRIPBOMBBLASTRADIUS
			MORTERBLASTRADIUS		MORTERBLASTRADIUS
			BOUNCEMINEBLASTRADIUS	BOUNCEMINEBLASTRADIUS
			SEENINEBLASTRADIUS		SEENINEBLASTRADIUS
			MAXPISTOLAMMO			MAXPISTOLAMMO
			MAXSHOTGUNAMMO			MAXSHOTGUNAMMO
			MAXCHAINGUNAMMO			MAXCHAINGUNAMMO
			MAXRPGAMMO				MAXRPGAMMO
			MAXHANDBOMBAMMO			MAXHANDBOMBAMMO
			MAXSHRINKERAMMO			MAXSHRINKERAMMO
			MAXDEVISTATORAMMO		MAXDEVISTATORAMMO
			MAXTRIPBOMBAMMO			MAXTRIPBOMBAMMO
			MAXFREEZEAMMO			MAXFREEZEAMMO
									MAXGROWAMMO
			CAMERASDESTRUCTABLE		CAMERASDESTRUCTABLE
			NUMFREEZEBOUNCES		NUMFREEZEBOUNCES
			FREEZERHURTOWNER		FREEZERHURTOWNER
									QSIZE
									TRIPBOMBLASERMODE
			*/

			// Used Jonathon Fowler's parser. Cool to make the code
			// robust to 1.3 con files --mk

			j = 0;

			ud.const_visibility		= params[j++];
			impact_damage			= params[j++];
			max_player_health		= params[j++];
			max_armour_amount		= params[j++];
			respawnactortime		= params[j++];
			respawnitemtime			= params[j++];
			dukefriction			= params[j++];
			if (conVersion == 15)
				gc					= params[j++];
			else
				gc					= 176; // default (guess) when using 1.3d CONs
			rpgblastradius			= params[j++];
			pipebombblastradius		= params[j++];
			shrinkerblastradius		= params[j++];
			tripbombblastradius		= params[j++];
			morterblastradius		= params[j++];
			bouncemineblastradius	= params[j++];
			seenineblastradius		= params[j++];
			max_ammo_amount[PISTOL_WEAPON]		= params[j++];
			max_ammo_amount[SHOTGUN_WEAPON]		= params[j++];
			max_ammo_amount[CHAINGUN_WEAPON]	= params[j++];
			max_ammo_amount[RPG_WEAPON]			= params[j++];
			max_ammo_amount[HANDBOMB_WEAPON]	= params[j++];
			max_ammo_amount[SHRINKER_WEAPON]	= params[j++];
			max_ammo_amount[DEVISTATOR_WEAPON]	= params[j++];
			max_ammo_amount[TRIPBOMB_WEAPON]	= params[j++];
			max_ammo_amount[FREEZE_WEAPON]		= params[j++];
			if (conVersion == 15)
				max_ammo_amount[GROW_WEAPON]	= params[j++];
			else
				max_ammo_amount[GROW_WEAPON]	= 50; // default (guess) when using 1.3d CONs
			camerashitable			= params[j++];
			numfreezebounces		= params[j++];
			freezerhurtowner		= params[j++];
			if (conVersion == 15)
			{
				spriteqamount		= params[j++];

				if(spriteqamount > 1024)
					spriteqamount	= 1024;
				else if(spriteqamount < 0)
					spriteqamount	= 0;
				lasermode			= params[j++];
			}
			else
			{
				// spriteqamount = 64 is the default
				lasermode = 0; // default (guess) when using 1.3d CONs
			}
		}
		return 0;

	} // end of switch(tw)

	return 0;
}

void passone(int readfromGRP)
{

    while( parsecommand(readfromGRP) == 0 );

    if( (error+warning) > 12)
        puts(  "  * ERROR! Too many warnings or errors.");

}
#endif
char  *defaultcons[3] =
{
     "GAME.CON",
     "USER.CON",
     "DEFS.CON"
};
#if CODE_REMOVED
void copydefaultcons(void)
{
    int32_t i, fs, fpi;
    FILE *fpo;
    for(i=0;i<sizeof(defaultcons)/sizeof(defaultcons[0]);i++)
    {
 //       printf("Default cons %i\r\n",i);
        fpi = TCkopen4load(defaultcons[i],1);
        fpo = fopen( defaultcons[i],"wb");

        if(fpi == 0)
        {
// CTW - MODIFICATION
//          if(fpo == -1) fclose(fpo);
            if(fpo == NULL) fclose(fpo);
// CTW END - MODIFICATION
            continue;
        }
// CTW - MODIFICATION
//      if(fpo == -1)
        if(fpo == NULL)
// CTW END - MODIFICATION
        {
            if(fpi == 0) kclose(fpi);
            continue;
        }

        fs = kfilelength(fpi);

        kread(fpi,&hittype[0],fs);
        fwrite(&hittype[0],fs,1,fpo);

        kclose(fpi);
        fclose(fpo);
    }
}
#endif
#if LOAD_FROM_CON_FILE
void loadefs(char  *filenam, char  *mptr, int readfromGRP)
{
    int32_t fs,fp;
	uint8_t  kbdKey;

	memset(script, 0, sizeof(script));

	// FIX_00071: do not ask for internal default con if
	// external con or internal con is buggy
#if !USE_EXT_MEM
    fp = TCkopen4load(filenam,readfromGRP);
    if( fp <= 0 )
    {
		Error(EXIT_SUCCESS, "ERROR: CON(%s) not found.\n", filenam);
    }
    else
    {
        printf("Compiling: '%s'.\n",filenam);

        fs = kfilelength(fp);

        last_used_text = textptr = (char  *) mptr;
        last_used_size = fs;

        kread(fp,(uint8_t  *)textptr,fs);
        kclose(fp);
#if CON_DATA_IN_UD
		ud.conCRC[0]=0;
//		ud.conCRC[0] = crc32_update((uint8_t  *)textptr, fs, ud.conCRC[0]);
#endif
    }
#else

        last_used_text = textptr = (char  *) mptr;
        void *pcon;
        kGetExtMemAddress(filenam, &fs);
        extMemGetDataFromAddress(mptr, pcon, fs);
       last_used_size = fs;
#endif
#ifdef UNIX
    textptr[fs - 1] = 0;
#else
    textptr[fs - 2] = 0;
#endif
#if USE_ACTORSCRPTR
    clearbuf(actorscrptr,MAXSPRITES,0L);
#else
    memset(actorscridx, 0, sizeof(actorscridx));
#endif
    clearbufbyte(actortype,MAXSPRITES,0L);

    labelcnt = 0;
    scriptptr = script+1;
    warning = 0;
    error = 0;
    line_number = 1;
    total_lines = 0;

    passone(readfromGRP); //Tokenize
    *script = encodescriptptr(scriptptr);


    if(warning|error)
        printf("Found %hhd warning(s), '%c' error(s).\n",warning,error);

    if(error)
    {
		Error(EXIT_SUCCESS, "ERROR in CON(%s)\n", filenam);
    }
    else
    {
        total_lines += line_number;
        printf("Code Size:%d bytes(%d labels).\n",(int32_t)((scriptptr-script)<<2)-4,labelcnt);
#if CON_DATA_IN_UD
		ud.conSize[0] = (int32_t)(scriptptr-script)-1;

		// FIX_00062: Better support and identification for GRP and CON files for 1.3/1.3d/1.4/1.5
		if( ud.conSize[0] == 16208 && labelcnt == 1794 && conVersion == 15)
			conVersion = 14;
		printf("Con version: Looks like v%d\n", conVersion);
#endif
			// FIX_00022: Automatically recognize the shareware grp (v1.3) + full version (1.3d) +
			//            atomic (1.4/1.5 grp) and the con files version (either 1.3 or 1.4) (JonoF's idea)

		if(conVersion != 13 && (getGRPcrc32(0)==CRC_BASE_GRP_SHAREWARE_13 ||
			getGRPcrc32(0)==CRC_BASE_GRP_FULL_13) && !getGRPcrc32(1))
		{
			printf(	"\nYou are trying to use a v1.3 Shareware/Full *.GRP with v1.4 or v1.5\n"
					"external *.CON files. You may run in troubles by doing so and/or get\n"
					"Out Of Synch errors. You can safely delete those files so xDuke will\n"
					"always use the GRP internal CON files.\n"
					"\nReload normal GRP internal *.CON files? (Y/N) : ");
#ifdef WIN32
			do
				kbdKey = getch() | ' ';
			while(kbdKey != 'y' && kbdKey != 'n');
#endif
			printf("%c\n", kbdKey);

			if(kbdKey == 'y')
			{
				conVersion = 13;
				loadefs(filenam, mptr, 1); // force GRP con files
			}
		}
		else if(conVersion != 15 && getGRPcrc32(0)==CRC_BASE_GRP_ATOMIC_15 && !getGRPcrc32(1))
		{
			printf(	"\nYou are trying to use a v1.5 ATOMIC *.GRP with v1.4 or v1.3\n"
					"external *.CON files. You may run in troubles by doing so and/or get\n"
					"Out Of Synch errors. You can safely delete those files so xDuke will\n"
					"always use the GRP internal CON files.\n"
					"\nReload normal GRP internal *.CON files? (Y/N) : ");
#ifdef WIN32
			do
				kbdKey = getch() | ' ';
			while(kbdKey != 'y' && kbdKey != 'n');
#endif
			printf("%c\n", kbdKey);

			if(kbdKey == 'y')
			{
				loadefs(filenam, mptr, 1); // force GRP con files
			}
		}else if(conVersion != 14 && getGRPcrc32(0)==CRC_BASE_GRP_PLUTONIUM_14 && !getGRPcrc32(1))
		{
			printf(	"\nYou are trying to use a v1.4 PLUTONIUM *.GRP with v1.3 or v1.5\n"
					"external *.CON files. You may run in troubles by doing so and/or get\n"
					"Out Of Synch errors. You can safely delete those files so xDuke will\n"
					"always use the GRP internal CON files.\n"
					"\nReload normal GRP internal *.CON files? (Y/N) : ");
#ifdef WIN32
			do
				kbdKey = getch() | ' ';
			while(kbdKey != 'y' && kbdKey != 'n');
			printf("%c\n", kbdKey);
#endif
			if(kbdKey == 'y')
			{
				loadefs(filenam, mptr, 1); // force GRP con files
			}
		}
	}
}
#endif
uint8_t  dodge(int spriteindex)
{
//    const spritetype *getSprite(spriteindex) = getSprite(spriteindex);
    short i;
    int32_t bx,by,mx,my,bxvect,byvect,mxvect,myvect,d;

    mx = getSprite(spriteindex)->_x;
    my = getSprite(spriteindex)->_y;
    mxvect = sintable[(getSprite(spriteindex)->_ang+512)&2047]; myvect = sintable[getSprite(spriteindex)->_ang&2047];

    for(i=headspritestat[4];i>=0;i=spr_add_data[i].nextspritestat) //weapons list
    {
        if( getSprite(i)->_owner == i || SECT != getSprite(spriteindex)->_sectnum)
            continue;

        bx = SX-mx;
        by = SY-my;
        bxvect = sintable[(SA+512)&2047]; byvect = sintable[SA&2047];

        if (mxvect*bx + myvect*by >= 0)
            if (bxvect*bx + byvect*by < 0)
        {
            d = bxvect*by - byvect*bx;
            if (klabs(d) < 65536*64)
            {
                setSprite_ang(spriteindex, getSprite(spriteindex)->_ang - 512+(TRAND&1024));
                return 1;
            }
        }
    }
    return 0;
}

short furthestangle(short i,short angs)
{
    short j, hitsect,hitwall,hitspr,furthest_angle, angincs;
    int32_t hx, hy, hz, d, greatestd;
//    const spritetype *getSprite(i) = getSprite(i);

    greatestd = -(1<<30);
    angincs = 2048/angs;

    if(getSprite(i)->_picnum != APLAYER)
        if( (getHittypeTemp(g_i, 0)&63) > 2 ) return( getSprite(i)->_ang + 1024 );

    for(j=getSprite(i)->_ang;j<(2048+getSprite(i)->_ang);j+=angincs)
    {
        hitscan(getSprite(i)->_x, getSprite(i)->_y, getSprite(i)->_z-(8<<8), getSprite(i)->_sectnum,
            sintable[(j+512)&2047],
            sintable[j&2047],0,
            &hitsect,&hitwall,&hitspr,&hx,&hy,&hz,CLIPMASK1);

        d = klabs(hx-getSprite(i)->_x) + klabs(hy-getSprite(i)->_y);

        if(d > greatestd)
        {
            greatestd = d;
            furthest_angle = j;
        }
    }
    return (furthest_angle&2047);
}

short furthestcanseepoint(short i,const spritetype *ts,int32_t *dax,int32_t *day)
{
    short j, hitsect,hitwall,hitspr, angincs;
    int32_t hx, hy, hz, d, da;//, d, cd, ca,tempx,tempy,cx,cy;
//    const spritetype *getSprite(i) = getSprite(i);

    if( (getHittypeTemp(g_i, 0)&63) ) return -1;

    if(ud.multimode < 2 && ud.player_skill < 3)
        angincs = 2048/2;
    else angincs = 2048/(1+(TRAND&1));

    for(j=ts->_ang;j<(2048+ts->_ang);j+=(angincs-(TRAND&511)))
    {
        hitscan(ts->_x, ts->_y, ts->_z-(16<<8), ts->_sectnum,
            sintable[(j+512)&2047],
            sintable[j&2047],16384-(TRAND&32767),
            &hitsect,&hitwall,&hitspr,&hx,&hy,&hz,CLIPMASK1);

        d = klabs(hx-ts->_x)+klabs(hy-ts->_y);
        da = klabs(hx-getSprite(i)->_x)+klabs(hy-getSprite(i)->_y);

        if( d < da )
            if(cansee(hx,hy,hz,hitsect,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z-(16<<8),getSprite(i)->_sectnum))
        {
            *dax = hx;
            *day = hy;
            return hitsect;
        }
    }
    return -1;
}




void alterang(short a)
{
    short aang, angdif, goalang,j;
    int32_t ticselapsed;
    const int32_t *moveptr;

    moveptr = decodescriptptr(getHittypeTemp(g_i, 1));
    ticselapsed = (getHittypeTemp(g_i, 0))&31;
//    const spritetype *getSprite(g_i);
    aang = getSprite(g_i)->_ang;

    setSprite_xvel(g_i, getSprite(g_i)->_xvel + (*moveptr-getSprite(g_i)->_xvel)/5);
    if(getSprite(g_i)->_zvel < 648)
        setSprite_zvel(g_i, getSprite(g_i)->_zvel + ((*(moveptr+1)<<4)-getSprite(g_i)->_zvel)/5);

    if(a&seekplayer)
    {
        j = ps[g_p].holoduke_on;

        if(j >= 0 && cansee(getSprite(j)->_x, getSprite(j)->_y, getSprite(j)->_z, getSprite(j)->_sectnum, getSprite(g_i)->_x, getSprite(g_i)->_y ,getSprite(g_i)->_z, getSprite(g_i)->_sectnum) )
            setSprite_owner(g_i, j);
        else
            setSprite_owner(g_i, ps[g_p].i);

        if (getSprite(getSprite(g_i)->_owner)->_picnum == APLAYER)
            goalang = getangle(getHittype_lastvx(g_i)-getSprite(g_i)->_x,getHittype_lastvy(g_i)-getSprite(g_i)->_y);
        else
            goalang = getangle(getSprite(getSprite(g_i)->_owner)->_x- getSprite(g_i)->_x, getSprite(getSprite(g_i)->_owner)->_y - getSprite(g_i)->_y);

        if(getSprite(g_i)->_xvel && getSprite(g_i)->_picnum != DRONE)
        {
            angdif = getincangle(aang,goalang);

            if(ticselapsed < 2)
            {
                if( klabs(angdif) < 256)
                {
                    j = 128-(TRAND&256);
                    setSprite_ang(g_i, getSprite(g_i)->_ang + j);
                    if( hits(g_i) < 844 )
                        setSprite_ang(g_i, getSprite(g_i)->_ang - j);
                }
            }
            else if(ticselapsed > 18 && ticselapsed < 26) // choose
            {
                if(klabs(angdif>>2) < 128)
                    setSprite_ang(g_i, goalang);
                else
                    setSprite_ang(g_i, getSprite(g_i)->_ang + (angdif>>2));
            }
        }
        else
            setSprite_ang(g_i, goalang);
    }

    if(ticselapsed < 1)
    {
        j = 2;
        if(a&furthestdir)
        {
            goalang = furthestangle(g_i,j);
            setSprite_ang(g_i, goalang);
            setSprite_owner(g_i, ps[g_p].i);
        }

        if(a&fleeenemy)
        {
            goalang = furthestangle(g_i,j);
            setSprite_ang(g_i, goalang); // += angdif; //  = getincangle(aang,goalang)>>1;
        }
    }
}

void move()
{
    int32_t l;
    const int32_t *moveptr;
    short a, goalang, angdif;
    int32_t daxvel;
//    const spritetype *getSprite(g_i);

    a = getSprite(g_i)->_hitag;
    if(a == -1) a = 0;

    setHittypeTemp(g_i, 0, getHittypeTemp(g_i, 0) + 1); //g_t[0]++;

    if(a&face_player)
    {
        if(ps[g_p].newowner >= 0)
            goalang = getangle(ps[g_p].oposx-getSprite(g_i)->_x,ps[g_p].oposy-getSprite(g_i)->_y);
        else goalang = getangle(ps[g_p].posx-getSprite(g_i)->_x,ps[g_p].posy-getSprite(g_i)->_y);
        angdif = getincangle(getSprite(g_i)->_ang, goalang) >> 2;
        if(angdif > -8 && angdif < 0) angdif = 0;
        setSprite_ang(g_i, getSprite(g_i)->_ang + angdif);
    }

    if(a&spin)
        setSprite_ang(g_i, getSprite(g_i)->_ang + (sintable[ ((getHittypeTemp(g_i, 0)<<3)&2047) ]>>6));

    if(a&face_player_slow)
    {
        if(ps[g_p].newowner >= 0)
            goalang = getangle(ps[g_p].oposx-getSprite(g_i)->_x,ps[g_p].oposy-getSprite(g_i)->_y);
        else goalang = getangle(ps[g_p].posx-getSprite(g_i)->_x,ps[g_p].posy-getSprite(g_i)->_y);
        angdif = ksgn(getincangle(getSprite(g_i)->_ang,goalang))<<5;
        if(angdif > -32 && angdif < 0)
        {
            angdif = 0;
            setSprite_ang(g_i, goalang);
        }
        setSprite_ang(g_i, getSprite(g_i)->_ang + angdif);
    }


    if((a&jumptoplayer) == jumptoplayer)
    {
        if(getHittypeTemp(g_i, 0) < 16)
            setSprite_zvel(g_i, getSprite(g_i)->_zvel - (sintable[(512+(getHittypeTemp(g_i, 0)<<4))&2047]>>5));
    }

    if(a&face_player_smart)
    {
        int32_t newx,newy;

        newx = ps[g_p].posx+(ps[g_p].posxv/768);
        newy = ps[g_p].posy+(ps[g_p].posyv/768);
        goalang = getangle(newx-getSprite(g_i)->_x,newy-getSprite(g_i)->_y);
        angdif = getincangle(getSprite(g_i)->_ang,goalang)>>2;
        if(angdif > -8 && angdif < 0) angdif = 0;
        setSprite_ang(g_i, getSprite(g_i)->_ang + angdif);
    }

    if( getHittypeTemp(g_i, 1) == 0 || a == 0 )
    {
        if( ( badguy(getSprite(g_i)) && getSprite(g_i)->_extra <= 0 ) || (getHittype(g_i)->bposx != getSprite(g_i)->_x) || (getHittype(g_i)->bposy != getSprite(g_i)->_y) )
        {
            setHittype_bposx(g_i, getSprite(g_i)->_x);
            setHittype_bposy(g_i, getSprite(g_i)->_y);
            setsprite(g_i,getSprite(g_i)->_x,getSprite(g_i)->_y,getSprite(g_i)->_z);
        }
        return;
    }

    moveptr = decodescriptptr(getHittypeTemp(g_i, 1));

    if(a&geth)
        setSprite_xvel(g_i, getSprite(g_i)->_xvel + ((*moveptr-getSprite(g_i)->_xvel)>>1));
    if(a&getv)
        setSprite_zvel(g_i, getSprite(g_i)->_zvel + (((*(moveptr+1)<<4)-getSprite(g_i)->_zvel)>>1));

    if(a&dodgebullet)
    {
        dodge(g_i);
        getSprite(g_i);
    }

    if(getSprite(g_i)->_picnum != APLAYER)
        alterang(a);

    if(getSprite(g_i)->_xvel > -6 && getSprite(g_i)->_xvel < 6 )
        setSprite_xvel(g_i, 0);

    a = badguy(getSprite(g_i));

    if(getSprite(g_i)->_xvel || getSprite(g_i)->_zvel)
    {
        if(a && getSprite(g_i)->_picnum != ROTATEGUN)
        {
            if( (getSprite(g_i)->_picnum == DRONE || getSprite(g_i)->_picnum == COMMANDER) && getSprite(g_i)->_extra > 0)
            {
                if(getSprite(g_i)->_picnum == COMMANDER)
                {
                    l = getflorzofslope(getSprite(g_i)->_sectnum,getSprite(g_i)->_x,getSprite(g_i)->_y);
                    setHittype_floorz(g_i, l);
                    if( getSprite(g_i)->_z > (l-(8<<8)) )
                    {
                        if( getSprite(g_i)->_z > (l-(8<<8)) )
                            setSprite_z(g_i,  l-(8<<8));
                        setSprite_zvel(g_i, 0);
                    }

                    l = getceilzofslope(getSprite(g_i)->_sectnum,getSprite(g_i)->_x,getSprite(g_i)->_y);
                    setHittype_ceilingz(g_i, l);
                    if( (getSprite(g_i)->_z-l) < (80<<8) )
                    {
                        setSprite_z(g_i, l+(80<<8));
                        setSprite_zvel(g_i, 0);
                    }
                }
                else
                {
                    if( getSprite(g_i)->_zvel > 0 )
                    {
                        l = getflorzofslope(getSprite(g_i)->_sectnum,getSprite(g_i)->_x,getSprite(g_i)->_y);
                        setHittype_floorz(g_i, l);
                        if( getSprite(g_i)->_z > (l-(30<<8)) )
                            setSprite_z(g_i, l-(30<<8));
                    }
                    else
                    {
                        l = getceilzofslope(getSprite(g_i)->_sectnum,getSprite(g_i)->_x,getSprite(g_i)->_y);
                        setHittype_ceilingz(g_i, l);
                        if( (getSprite(g_i)->_z-l) < (50<<8) )
                        {
                            setSprite_z(g_i, l+(50<<8));
                            setSprite_zvel(g_i, 0);
                        }
                    }
                }
            }
            else if(getSprite(g_i)->_picnum != ORGANTIC)
            {
                if(getSprite(g_i)->_zvel > 0 && getHittype(g_i)->floorz < getSprite(g_i)->_z)
                    setSprite_z (g_i, getHittype(g_i)->floorz);
                if( getSprite(g_i)->_zvel < 0)
                {
                    l = getceilzofslope(getSprite(g_i)->_sectnum,getSprite(g_i)->_x,getSprite(g_i)->_y);
                    if( (getSprite(g_i)->_z-l) < (66<<8) )
                    {
                        setSprite_z(g_i,  l+(66<<8));
                        setSprite_zvel(g_i, getSprite(g_i)->_zvel >> 1);
                    }
                }
            }
        }
        else if(getSprite(g_i)->_picnum == APLAYER)
            if( (getSprite(g_i)->_z-getHittype(g_i)->ceilingz) < (32<<8) )
                setSprite_z(g_i, getHittype(g_i)->ceilingz+(32<<8));

        daxvel = getSprite(g_i)->_xvel;
        angdif = getSprite(g_i)->_ang;

        if( a && getSprite(g_i)->_picnum != ROTATEGUN )
        {
            if( g_x < 960 && getSprite(g_i)->_xrepeat > 16 )
            {

                daxvel = -(1024-g_x);
                angdif = getangle(ps[g_p].posx-getSprite(g_i)->_x,ps[g_p].posy-getSprite(g_i)->_y);

                if(g_x < 512)
                {
                    ps[g_p].posxv = 0;
                    ps[g_p].posyv = 0;
                }
                else
                {
                    ps[g_p].posxv = mulscale(ps[g_p].posxv,dukefriction-0x2000,16);
                    ps[g_p].posyv = mulscale(ps[g_p].posyv,dukefriction-0x2000,16);
                }
            }
            else if(getSprite(g_i)->_picnum != DRONE && getSprite(g_i)->_picnum != SHARK && getSprite(g_i)->_picnum != COMMANDER)
            {
                if( getHittype(g_i)->bposz != getSprite(g_i)->_z || ( ud.multimode < 2 && ud.player_skill < 2 ) )
                {
                    if( (getHittypeTemp(g_i, 0)&1) || ps[g_p].actorsqu == g_i ) return;
                    else daxvel <<= 1;
                }
                else
                {
                    if( (getHittypeTemp(g_i, 0)&3) || ps[g_p].actorsqu == g_i ) return;
                    else daxvel <<= 2;
                }
            }
        }
#if SAVE_SPACE_ON_MOVFLAG
        setHittype_movflag(g_i, movesprite(g_i,
            (daxvel*(sintable[(angdif+512)&2047]))>>14,
            (daxvel*(sintable[angdif&2047]))>>14,getSprite(g_i)->_zvel,CLIPMASK0) > 16384 ? 1 : 0);
#else
        setHittype_movflag(g_i, movesprite(g_i,
            (daxvel*(sintable[(angdif+512)&2047]))>>14,
            (daxvel*(sintable[angdif&2047]))>>14,getSprite(g_i)->__zvel,CLIPMASK0));
#endif // SAVE_SPACE_ON_MOVFLAG
   }

   if( a )
   {
       if (getSector(getSprite(g_i)->_sectnum)->ceilingstat&1)
           setSprite_shade(g_i, getSprite(g_i)->_shade + ((getSector_ceilingshade(getSprite(g_i)->_sectnum)-getSprite(g_i)->_shade)>>1));
       else
           setSprite_shade(g_i, getSprite(g_i)->_shade + ((getSector_floorshade(getSprite(g_i)->_sectnum)-getSprite(g_i)->_shade)>>1));

       if( getSector(getSprite(g_i)->_sectnum)->floorpicnum == MIRROR )
           deletesprite(g_i);
   }
}

void parseifelse(int32_t condition)
{
    if( condition )
    {
        insptr+=2;
        parse();
    }
    else
    {
        insptr = decodescriptptr(*(insptr+1));
        if(*insptr == 10)
        {
            insptr+=2;
            parse();
        }
    }
}

// int32_t *it = 0x00589a04;

uint8_t  parse(void)
{
    int32_t j, l, s;

    if(killit_flag) return 1;

//    if(*it == 1668249134L) gameexit("\nERR");

    switch(*insptr)
    {
        case 3:
            insptr++;
            parseifelse( rnd(*insptr));
            break;
        case 45:

            if(g_x > 1024)
            {
                short temphit, sclip, angdif;

                if( badguy(getSprite(g_i)) && getSprite(g_i)->_xrepeat > 56 )
                {
                    sclip = 3084;
                    angdif = 48;
                }
                else
                {
                    sclip = 768;
                    angdif = 16;
                }

                j = hitasprite(g_i,&temphit);
                if(j == (1<<30))
                {
                    parseifelse(1);
                    break;
                }
                if(j > sclip)
                {
                    if(temphit >= 0 && getSprite(temphit)->_picnum == getSprite(g_i)->_picnum)
                        j = 0;
                    else
                    {
                        setSprite_ang(g_i, getSprite_ang(g_i) + angdif);
                        j = hitasprite(g_i,&temphit);
                        setSprite_ang(g_i, getSprite_ang(g_i) - angdif);
                        if(j > sclip)
                        {
                            if(temphit >= 0 && getSprite(temphit)->_picnum == getSprite(g_i)->_picnum)
                                j = 0;
                            else
                            {
                                setSprite_ang(g_i, getSprite(g_i)->_ang - angdif);
                                j = hitasprite(g_i,&temphit);
                                setSprite_ang(g_i, getSprite(g_i)->_ang + angdif);
                                if( j > 768 )
                                {
                                    if(temphit >= 0 && getSprite(temphit)->_picnum == getSprite(g_i)->_picnum)
                                        j = 0;
                                    else j = 1;
                                }
                                else j = 0;
                            }
                        }
                        else j = 0;
                    }
                }
                else j =  0;
            }
            else j = 1;

            parseifelse(j);
            break;
        case 91:
            j = cansee(getSprite(g_i)->_x,getSprite(g_i)->_y,getSprite(g_i)->_z-((TRAND&41)<<8),getSprite(g_i)->_sectnum,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz/*-((TRAND&41)<<8)*/,getSprite(ps[g_p].i)->_sectnum);
            parseifelse(j);
            if ( j )
                setHittype_timetosleep(g_i, SLEEPTIME);
            break;

        case 49:
#if MINIMIZE_ACTORSTAYPUT
            parseifelse(getHittype(g_i)->actorstayput == MAXSECTORS); //next-hack: was -1
 #else
            parseifelse(getHittype(g_i)->actorstayput == -1); //next-hack: was -1

 #endif
            break;
        case 5:
        {
//            const spritetype *getSprite(sn);
            int sn = 0;
            if(ps[g_p].holoduke_on >= 0)
            {
                //getSprite(sn) = getSprite(ps[g_p].holoduke_on);
                sn = ps[g_p].holoduke_on;
                j = cansee(getSprite(g_i)->_x,getSprite(g_i)->_y,getSprite(g_i)->_z-(TRAND&((32<<8)-1)),getSprite(g_i)->_sectnum,
                       getSprite(sn)->_x,getSprite(sn)->_y,getSprite(sn)->_z,getSprite(sn)->_sectnum);
                if(j == 0)
                   sn = ps[g_p].i; // getSprite(sn) = getSprite(ps[g_p].i);
            }
            else
                sn = ps[g_p].i; //getSprite(sn) = getSprite(ps[g_p].i);

            j = cansee(getSprite(g_i)->_x,getSprite(g_i)->_y,getSprite(g_i)->_z-(TRAND&((47<<8))),getSprite(g_i)->_sectnum,
                getSprite(sn)->_x,getSprite(sn)->_y,getSprite(sn)->_z-(24<<8),getSprite(sn)->_sectnum);

            if(j == 0)
            {
                if( ( klabs(getHittype_lastvx(g_i)-getSprite(g_i)->_x)+klabs(getHittype_lastvy(g_i)-getSprite(g_i)->_y) ) <
                    ( klabs(getHittype_lastvx(g_i)-getSprite(sn)->_x)+klabs(getHittype_lastvy(g_i)-getSprite(sn)->_y) ) )
                        j = 0;

                if( j == 0 )
                {
                    int32_t lastvx = getHittype_lastvx(g_i);
                    int32_t lastvy = getHittype_lastvy(g_i);
                    j = furthestcanseepoint(g_i,getSprite(sn),&lastvx,&lastvy);
                    setHittype_lastvx(g_i, lastvx);
                    setHittype_lastvy(g_i, lastvy);


                    if(j == -1) j = 0;
                    else j = 1;
                }
            }
            else
            {
                setHittype_lastvx(g_i, getSprite(sn)->_x);
                setHittype_lastvy(g_i, getSprite(sn)->_y);
            }

            if( j == 1 && ( getSprite(g_i)->_statnum == 1 || getSprite(g_i)->_statnum == 6 ) )
                setHittype_timetosleep(g_i, SLEEPTIME);

            parseifelse(j == 1);
            break;
        }

        case 6:
            parseifelse(ifhitbyweapon(g_i) >= 0);
            break;
        case 27:
            parseifelse( ifsquished(g_i, g_p) == 1);
            break;
        case 26:
            {
                j = getSprite(g_i)->_extra;
                if(getSprite(g_i)->_picnum == APLAYER)
                    j--;
                parseifelse(j < 0);
            }
            break;
        case 24:
            insptr++;
            setHittypeTemp(g_i, 5, *insptr); // g_t[5] = *insptr;
            setHittypeTemp(g_i, 4, *(decodescriptptr(getHittypeTemp(g_i, 5)))  ); // g_t[4] = *(decodescriptptr(getHittypeTemp(g_i, 5)));       // Action
            setHittypeTemp(g_i, 1, *(decodescriptptr(getHittypeTemp(g_i, 5)) + 1)); // g_t[1] = *(decodescriptptr(getHittypeTemp(g_i, 5)) + 1);   // move
            setSprite_hitag(g_i, *(decodescriptptr(getHittypeTemp(g_i, 5)) + 2));    // Ai
            setHittypeTemp(g_i, 0, 0); // g_t[0] = g_t[2] = g_t[3] = 0;
            setHittypeTemp(g_i, 2, 0); //
            setHittypeTemp(g_i, 3, 0); //
            if(getSprite(g_i)->_hitag&random_angle)
                setSprite_ang(g_i, TRAND&2047);
            insptr++;
            break;
        case 7:
            insptr++;
             setHittypeTemp(g_i, 2, 0); // g_t[2] = 0;
             setHittypeTemp(g_i, 3, 0); // g_t[3] = 0;
			// FIX_00093: fixed crashbugs in multiplayer (mine/blimp)
			// This is the blimp bug.
			// *.con code 1.3 and 1.4 are buggy when you try to blow up the
			// blimp in multiplayer. duke3d_w32 /q2 /m /v3 /l9
			// This is because the con code gives a timeout value of 2048
			// as a action address instead of giving a real action address.
			// We simply counter this specific con code bug by resetting
			// the action address to 0 when we get an address "2048":
			 setHittypeTemp(g_i, 4, ((*insptr)==2048)?0:(*insptr)); // g_t[4] = ((*insptr)==2048)?0:(*insptr);
            insptr++;
            break;

        case 8:
            insptr++;
            parseifelse(g_x < *insptr);
            if(g_x > MAXSLEEPDIST && getHittype_timetosleep(g_i) == 0)
                setHittype_timetosleep(g_i, SLEEPTIME);
            break;
        case 9:
            insptr++;
            parseifelse(g_x > *insptr);
            if(g_x > MAXSLEEPDIST && getHittype_timetosleep(g_i) == 0)
                setHittype_timetosleep(g_i, SLEEPTIME);
            break;
        case 10:
            insptr = decodescriptptr(*(insptr + 1));
            break;
        case 100:
            insptr++;
            setSprite_extra(g_i, getSprite_extra(g_i) + *insptr);
            insptr++;
            break;
        case 11:
            insptr++;
            setSprite_extra(g_i, *insptr);
            insptr++;
            break;
        case 94:
            insptr++;

            if(ud.coop >= 1 && ud.multimode > 1)
            {
                if(*insptr == 0)
                {
                    for(j=0;j < ps[g_p].weapreccnt;j++)
                        if( ps[g_p].weaprecs[j] == getSprite(g_i)->_picnum )
                            break;

                    parseifelse(j < ps[g_p].weapreccnt && getSprite(g_i)->_owner == g_i);
                }
                else if(ps[g_p].weapreccnt < 16)
                {
                    ps[g_p].weaprecs[ps[g_p].weapreccnt++] = getSprite(g_i)->_picnum;
                    parseifelse(getSprite(g_i)->_owner == g_i);
                }
            }
            else parseifelse(0);
            break;
        case 95:
            insptr++;
            if(getSprite(g_i)->_picnum == APLAYER)
                setSprite_pal(g_i, ps[getSprite(g_i)->_yvel].palookup);
            else
                setSprite_pal(g_i, getHittype_tempang(g_i));
            setHittype_tempang(g_i, 0);
            break;
        case 104:
            insptr++;
            checkweapons(&ps[getSprite(g_i)->_yvel]);
            break;
        case 106:
            insptr++;
            break;
        case 97:
            insptr++;
//            if(Sound[getSprite(g_i)->_yvel].num == 0)
            if(getSoundNum(getSprite(g_i)->_yvel) == 0)
                spritesound(getSprite(g_i)->_yvel,g_i);
            break;
        case 96:
            insptr++;

            if( ud.multimode > 1 && getSprite(g_i)->_picnum == APLAYER )
            {
                if(ps[otherp].quick_kick == 0)
                    ps[otherp].quick_kick = 14;
            }
            else if(getSprite(g_i)->_picnum != APLAYER && ps[g_p].quick_kick == 0)
                ps[g_p].quick_kick = 14;
            break;
        case 28:
            insptr++;

            j = ((*insptr)-getSprite(g_i)->_xrepeat)<<1;
            setSprite_xrepeat(g_i, getSprite_xrepeat(g_i) + ksgn(j));

            insptr++;

            if( ( getSprite(g_i)->_picnum == APLAYER && getSprite(g_i)->_yrepeat < 36 ) ||
               *insptr < getSprite(g_i)->_yrepeat ||
               ((getSprite(g_i)->_yrepeat*(tiles[getSprite(g_i)->_picnum].dim.height+8))<<2) < (getHittype(g_i)->floorz - getHittype(g_i)->ceilingz) )
            {
                j = ((*insptr)-getSprite(g_i)->_yrepeat)<<1;
                if( klabs(j) )
                    setSprite_yrepeat(g_i, getSprite_yrepeat(g_i) + ksgn(j));
            }

            insptr++;

            break;
        case 99:
            insptr++;
            setSprite_xrepeat(g_i, (uint8_t ) *insptr);
            insptr++;
            setSprite_yrepeat(g_i, (uint8_t ) *insptr);
            insptr++;
            break;
        case 13:
            insptr++;
            shoot(g_i,(short)*insptr);
            insptr++;
            break;
        case 87:
            insptr++;
//            if( Sound[*insptr].num == 0 )
            if( getSoundNum(*insptr) == 0 )
                spritesound((short) *insptr,g_i);
            insptr++;
            break;
        case 89:
            insptr++;
//            if( Sound[*insptr].num > 0 )
            if( getSoundNum(*insptr) > 0 )
                stopsound((short)*insptr);
            insptr++;
            break;
        case 92:
            insptr++;
            if(g_p == screenpeek || ud.coop==1)
                spritesound((short) *insptr,ps[screenpeek].i);
            insptr++;
            break;
        case 15:
            insptr++;
            spritesound((short) *insptr,g_i);
            insptr++;
            break;
        case 84:
            insptr++;
            ps[g_p].tipincs = 26;
            break;
        case 16:
            insptr++;
            setSprite_xoffset(g_i, 0);
            setSprite_yoffset(g_i, 0);
//            if(!gotz)
            {
                int32_t c;

                if( floorspace(getSprite(g_i)->_sectnum) )
                    c = 0;
                else
                {
                    if( ceilingspace(getSprite(g_i)->_sectnum) || getSector(getSprite(g_i)->_sectnum)->lotag == 2)
                        c = gc/6;
                    else c = gc;
                }

                if( getHittype(g_i)->cgg <= 0 || (getSector(getSprite(g_i)->_sectnum)->floorstat&2) )
                {
                    getglobalz(g_i);
                    setHittype_cgg(g_i, 6);
                }
                else setHittype_cgg(g_i, getHittype(g_i)->cgg - 1);

                if( getSprite(g_i)->_z < (getHittype(g_i)->floorz-FOURSLEIGHT) )
                {
                    setSprite_zvel(g_i, getSprite_zvel(g_i) + c);
                    setSprite_z(g_i, getSprite_z(g_i) + getSprite(g_i)->_zvel);

                    if(getSprite(g_i)->_zvel > 6144)
                        setSprite_zvel(g_i, 6144);
                }
                else
                {
                    setSprite_z(g_i, getHittype(g_i)->floorz - FOURSLEIGHT);

                    if( badguy(getSprite(g_i)) || ( getSprite(g_i)->_picnum == APLAYER && getSprite(g_i)->_owner >= 0) )
                    {

                        if( getSprite(g_i)->_zvel > 3084 && getSprite(g_i)->_extra <= 1)
                        {
                            if(getSprite(g_i)->_pal != 1 && getSprite(g_i)->_picnum != DRONE)
                            {
                                if(getSprite(g_i)->_picnum == APLAYER && getSprite(g_i)->_extra > 0)
                                    goto SKIPJIBS;
                                guts(getSprite(g_i),JIBS6,15,g_p);
                                spritesound(SQUISHED,g_i);
                                spawn(g_i,BLOODPOOL);
                            }

                            SKIPJIBS:

                            setHittype_picnum(g_i, SHOTSPARK1);
                            setHittype_extra(g_i, 1);
                            setSprite_zvel(g_i, 0);
                        }
                        else if(getSprite(g_i)->_zvel > 2048 && getSector(getSprite(g_i)->_sectnum)->lotag != 1)
                        {

                            j = getSprite(g_i)->_sectnum;
                            // next-hack: this to let us use bitfields.
                            int32_t sprx = getSprite(g_i)->_x;
                            int32_t spry = getSprite(g_i)->_y;
                            int32_t sprz = getSprite(g_i)->_z;
                            pushmove(&sprx,&spry,&sprz,(short*)&j,128L,(4L<<8),(4L<<8),CLIPMASK0);
                            setSprite_x(g_i, sprx);
                            setSprite_y(g_i, spry);
                            setSprite_z(g_i, sprz);

                            if(j != getSprite(g_i)->_sectnum && j >= 0 && j < MAXSECTORS)
                                changespritesect(g_i,j);

                            spritesound(THUD,g_i);
                        }
                    }
                    if(getSector(getSprite(g_i)->_sectnum)->lotag == 1)
                        switch (getSprite(g_i)->_picnum)
                        {
                            case OCTABRAIN:
                            case COMMANDER:
                            case DRONE:
                                break;
                            default:
                                setSprite_z(g_i, getSprite_z(g_i) + (24<<8));
                                break;
                        }
                    else
                        setSprite_zvel(g_i, 0);
                }
            }

            break;
        case 4:
        case 12:
        case 18:
            return 1;
        case 30:
            insptr++;
            return 1;
        case 2:
            insptr++;
            if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
            {
                killit_flag = 2;
                break;
            }
            addammo( *insptr, &ps[g_p], *(insptr+1) );
            if(ps[g_p].curr_weapon == KNEE_WEAPON)
                if( ps[g_p].gotweapon[*insptr] )
                    addweapon( &ps[g_p], *insptr );
            insptr += 2;
            break;
        case 86:
            insptr++;
            lotsofmoney(getSprite(g_i),*insptr);
            insptr++;
            break;
        case 102:
            insptr++;
            lotsofmail(getSprite(g_i),*insptr);
            insptr++;
            break;
        case 105:
            insptr++;
            setHittype_timetosleep(g_i, (short)*insptr);  // Not sure how much will it be
            insptr++;
            break;
        case 103:
            insptr++;
            lotsofpaper(getSprite(g_i),*insptr);
            insptr++;
            break;
        case 88:
            insptr++;
            ps[g_p].actors_killed += *insptr;
#if MINIMIZE_ACTORSTAYPUT
            getHittype(g_i)->actorstayput = MAXSECTORS; // next-hack: was -1
#else
            setHittype_actorstayput(g_i, -1);
#endif
            insptr++;
            break;
        case 93:
            insptr++;
            spriteglass(g_i,*insptr);
            insptr++;
            break;
        case 22:
            insptr++;
            killit_flag = 1;
            break;
        case 23:
            insptr++;
            if( ps[g_p].gotweapon[*insptr] == 0 ) addweapon( &ps[g_p], *insptr );
            else if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
            {
                 killit_flag = 2;
                 break;
            }
            addammo( *insptr, &ps[g_p], *(insptr+1) );
            if(ps[g_p].curr_weapon == KNEE_WEAPON)
                if( ps[g_p].gotweapon[*insptr] )
                    addweapon( &ps[g_p], *insptr );
            insptr+=2;
            break;
        case 68:
            insptr++;
            printf("%d\n",*insptr);
            insptr++;
            break;
        case 69:
            insptr++;
            ps[g_p].timebeforeexit = *insptr;
            ps[g_p].customexitsound = -1;
            ud.eog = 1;
            insptr++;
            break;
        case 25:
            insptr++;

            if(ps[g_p].newowner >= 0)
            {
                ps[g_p].newowner = -1;
                ps[g_p].posx = ps[g_p].oposx;
                ps[g_p].posy = ps[g_p].oposy;
                ps[g_p].posz = ps[g_p].oposz;
                ps[g_p].ang = ps[g_p].oang;
                updatesector(ps[g_p].posx,ps[g_p].posy,&ps[g_p].cursectnum);
                setpal(&ps[g_p]);

                j = headspritestat[1];
                while(j >= 0)
                {
                    if(getSprite(j)->_picnum==CAMERA1)
                        setSprite_yvel(j, 0);
                    j = spr_add_data[j].nextspritestat;
                }
            }

            j = getSprite(ps[g_p].i)->_extra;

            if(getSprite(g_i)->_picnum != ATOMICHEALTH)
            {
                if( j > max_player_health && *insptr > 0 )
                {
                    insptr++;
                    break;
                }
                else
                {
                    if(j > 0)
                        j += *insptr;
                    if ( j > max_player_health && *insptr > 0 )
                        j = max_player_health;
                }
            }
            else
            {
                if( j > 0 )
                    j += *insptr;
                if ( j > (max_player_health<<1) )
                    j = (max_player_health<<1);
            }

            if(j < 0) j = 0;

            if(ud.god == 0)
            {
                if(*insptr > 0)
                {
                    if( ( j - *insptr ) < (max_player_health>>2) &&
                        j >= (max_player_health>>2) )
                            spritesound(DUKE_GOTHEALTHATLOW,ps[g_p].i);

                    ps[g_p].last_extra = j;
                }

                setSprite_extra(ps[g_p].i, j);
            }

            insptr++;
            break;
        case 17:
            {
				const int32_t *tempscrptr;

                tempscrptr = insptr+2;

                insptr = decodescriptptr(*(insptr + 1));
                while(1) if(parse()) break;
                insptr = tempscrptr;
            }
            break;
        case 29:
            insptr++;
            while(1) if(parse()) break;
            break;
        case 32:
            setHittypeTemp(g_i, 0, 0); //g_t[0]=0;
            insptr++;
            setHittypeTemp(g_i, 1, *insptr); //g_t[1] = *insptr;
            insptr++;
            setSprite_hitag(g_i, *insptr);
            insptr++;
            if(getSprite(g_i)->_hitag&random_angle)
                setSprite_ang(g_i, TRAND&2047);
            break;
        case 31:
            insptr++;
            if(getSprite(g_i)->_sectnum >= 0 && getSprite(g_i)->_sectnum < MAXSECTORS)
                spawn(g_i,*insptr);
            insptr++;
            break;
        case 33:
            insptr++;
            parseifelse( getHittype(g_i)->picnum == *insptr);
            break;
        case 21:
            insptr++;
            parseifelse(getHittypeTemp(g_i, 5) == *insptr);
            break;
        case 34:
            insptr++;
            parseifelse(getHittypeTemp(g_i, 4) == *insptr);
            break;
        case 35:
            insptr++;
            parseifelse(getHittypeTemp(g_i, 2) >= *insptr);
            break;
        case 36:
            insptr++;
            setHittypeTemp(g_i, 2, 0); //g_t[2] = 0;
            break;
        case 37:
            {
                short dnum;

                insptr++;
                dnum = *insptr;
                insptr++;

                if(getSprite(g_i)->_sectnum >= 0 && getSprite(g_i)->_sectnum < MAXSECTORS)
                    for(j=(*insptr)-1;j>=0;j--)
                {
                    if(getSprite(g_i)->_picnum == BLIMP && dnum == SCRAP1)
                        s = 0;
                    else s = (TRAND%3);

                    l = EGS(getSprite(g_i)->_sectnum,
                            getSprite(g_i)->_x+(TRAND&255)-128,getSprite(g_i)->_y+(TRAND&255)-128,getSprite(g_i)->_z-(8<<8)-(TRAND&8191),
                            dnum+s,getSprite(g_i)->_shade,32+(TRAND&15),32+(TRAND&15),
                            TRAND&2047,(TRAND&127)+32,
                            -(TRAND&2047),g_i,5);
                    if(getSprite(g_i)->_picnum == BLIMP && dnum == SCRAP1)
                        setSprite_yvel(l, weaponsandammosprites[j%14]);
                    else
                        setSprite_yvel(l, -1);
                    setSprite_pal(l, getSprite(g_i)->_pal);
                }
                insptr++;
            }
            break;
        case 52:
            insptr++;
            setHittypeTemp(g_i, 2, (short) *insptr); //g_t[0] = (short) *insptr;
            insptr++;
            break;
        case 101:
            insptr++;
            if (*insptr & ORIGINAL_SPRITE_INVISIBLE)    // original sprite invisible
                setSprite_cstat(g_i, getSprite_cstat(g_i) | (short)*insptr | SPRITE_INVISIBLE);
            else
            setSprite_cstat(g_i, getSprite_cstat(g_i) | (short)*insptr);
            insptr++;
            break;
        case 110:
            insptr++;
            spr_add_data[g_i].clipdist = (short) *insptr;
            insptr++;
            break;
        case 40:
            insptr++;
            if (*insptr & ORIGINAL_SPRITE_INVISIBLE)    // original sprite invisible
                setSprite_cstat(g_i,  (short) *insptr | SPRITE_INVISIBLE);
            else
            setSprite_cstat(g_i,  (short) *insptr);
            insptr++;
            break;
        case 41:
            insptr++;
            parseifelse(getHittypeTemp(g_i, 1) == *insptr);
            break;
        case 42:
            insptr++;

            if(ud.multimode < 2)
            {
                if( lastsavedpos >= 0 && ud.recstat != 2 )
                {
                    ps[g_p].gm = MODE_MENU;
                    KB_ClearKeyDown(sc_Space);
                    cmenu(MENU_LOAD_LAST_GAME_QUESTION0);
                }
                else
                {
                    ps[g_p].gm = MODE_RESTART | MODE_RELOAD_FROM_DEAD;
                }
                killit_flag = 2;
            }
            else
            {
                // next-hack: after adding looping sound, we found that looping sound did not stop on death.
                FX_StopAllSounds();
                //
                pickrandomspot(g_p);
                setHittype_bposx(g_i, ps[g_p].bobposx = ps[g_p].oposx = ps[g_p].posx);
                setSprite_x(g_i, ps[g_p].posx);
                setHittype_bposy(g_i, ps[g_p].bobposy = ps[g_p].oposy =ps[g_p].posy);
                setSprite_y(g_i, ps[g_p].posy);
                //FIXME: originally the following was setting to ps[g_p].posy but could be a bug!
                setHittype_bposz(g_i, ps[g_p].oposz = ps[g_p].posz);
                setSprite_z(g_i, ps[g_p].posz);
                updatesector(ps[g_p].posx,ps[g_p].posy,&ps[g_p].cursectnum);
                setsprite(ps[g_p].i,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz+PHEIGHT);
                setSprite_cstat(g_i, 257);

                setSprite_shade(g_i, -12);
                spr_add_data[g_i].clipdist = 64;
                setSprite_xrepeat(g_i, 42);
                setSprite_yrepeat(g_i, 36);
                setSprite_owner(g_i, g_i);
                setSprite_xoffset(g_i, 0);
                setSprite_pal(g_i, ps[g_p].palookup);

                setSprite_extra(g_i, ps[g_p].last_extra = max_player_health);
                ps[g_p].wantweaponfire = -1;
                ps[g_p].horiz = 100;
                ps[g_p].on_crane = -1;
                ps[g_p].frag_ps = g_p;
                ps[g_p].horizoff = 0;
                ps[g_p].opyoff = 0;
                ps[g_p].wackedbyactor = -1;
                ps[g_p].shield_amount = max_armour_amount;
                ps[g_p].dead_flag = 0;
                ps[g_p].pals_time = 0;
                ps[g_p].footprintcount = 0;
                ps[g_p].weapreccnt = 0;
                ps[g_p].fta = 0;
                ps[g_p].ftq = 0;
                ps[g_p].posxv = ps[g_p].posyv = 0;
                ps[g_p].rotscrnang = 0;

                ps[g_p].falling_counter = 0;

                setHittype_extra(g_i, -1);
                setHittype_owner(g_i, g_i);

                setHittype_cgg(g_i, 0);
                setHittype_movflag(g_i, 0);
                setHittype_tempang(g_i, 0);
#if MINIMIZE_ACTORSTAYPUT
                getHittype(g_i)->actorstayput = MAXSECTORS; // next-hack: was -1
#else
                setHittype_actorstayput(g_i, -1); // next-hack: was -1
#endif
                setHittype_dispicnum(g_i, 0);
                setHittype_owner(g_i, ps[g_p].i);

                resetinventory(g_p);
                resetweapons(g_p);

                cameradist = 0;
                cameraclock = totalclock;
            }
            setpal(&ps[g_p]);

            break;
        case 43:
            parseifelse( klabs(getSprite(g_i)->_z-getSector(getSprite(g_i)->_sectnum)->floorz) < (32<<8) && getSector(getSprite(g_i)->_sectnum)->lotag == 1);
            break;
        case 44:
            parseifelse( getSector(getSprite(g_i)->_sectnum)->lotag == 2);
            break;
        case 46:
            insptr++;
            parseifelse(getHittypeTemp(g_i, 0) >= *insptr);
            break;
        case 53:
            insptr++;
            parseifelse(getSprite(g_i)->_picnum == *insptr);
            break;
        case 47:
            insptr++;
            setHittypeTemp(g_i, 0, 0); //g_t[0] = 0;
            break;
        case 48:
            insptr+=2;
            switch(*(insptr-1))
            {
                case 0:
                    ps[g_p].steroids_amount = *insptr;
                    ps[g_p].inven_icon = 2;
                    break;
                case 1:
                    ps[g_p].shield_amount +=          *insptr;// 100;
                    if(ps[g_p].shield_amount > max_player_health)
                        ps[g_p].shield_amount = max_player_health;
                    break;
                case 2:
                    ps[g_p].scuba_amount =             *insptr;// 1600;
                    ps[g_p].inven_icon = 6;
                    break;
                case 3:
                    ps[g_p].holoduke_amount =          *insptr;// 1600;
                    ps[g_p].inven_icon = 3;
                    break;
                case 4:
                    ps[g_p].jetpack_amount =           *insptr;// 1600;
                    ps[g_p].inven_icon = 4;
                    break;
                case 6:
                    switch(getSprite(g_i)->_pal)
                    {
                        case  0: ps[g_p].got_access |= 1;break;
                        case 21: ps[g_p].got_access |= 2;break;
                        case 23: ps[g_p].got_access |= 4;break;
                    }
                    break;
                case 7:
                    ps[g_p].heat_amount = *insptr;
                    ps[g_p].inven_icon = 5;
                    break;
                case 9:
                    ps[g_p].inven_icon = 1;
                    ps[g_p].firstaid_amount = *insptr;
                    break;
                case 10:
                    ps[g_p].inven_icon = 7;
                    ps[g_p].boot_amount = *insptr;
                    break;
            }
            insptr++;
            break;
        case 50:
            hitradius(g_i,*(insptr+1),*(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5));
            insptr+=6;
            break;
        case 51:
            {
                insptr++;

                l = *insptr;
                j = 0;

                s = getSprite(g_i)->_xvel;

                if( (l&8) && ps[g_p].on_ground && (sync[g_p].bits&2) )
                       j = 1;
                else if( (l&16) && ps[g_p].jumping_counter == 0 && !ps[g_p].on_ground &&
                    ps[g_p].poszv > 2048 )
                        j = 1;
                else if( (l&32) && ps[g_p].jumping_counter > 348 )
                       j = 1;
                else if( (l&1) && s >= 0 && s < 8)
                       j = 1;
                else if( (l&2) && s >= 8 && !(sync[g_p].bits&(1<<5)) )
                       j = 1;
                else if( (l&4) && s >= 8 && sync[g_p].bits&(1<<5) )
                       j = 1;
                else if( (l&64) && ps[g_p].posz < (getSprite(g_i)->_z-(48<<8)) )
                       j = 1;
                else if( (l&128) && s <= -8 && !(sync[g_p].bits&(1<<5)) )
                       j = 1;
                else if( (l&256) && s <= -8 && (sync[g_p].bits&(1<<5)) )
                       j = 1;
                else if( (l&512) && ( ps[g_p].quick_kick > 0 || ( ps[g_p].curr_weapon == KNEE_WEAPON && ps[g_p].kickback_pic > 0 ) ) )
                       j = 1;
                else if( (l&1024) && getSprite(ps[g_p].i)->_xrepeat < 32 )
                       j = 1;
                else if( (l&2048) && ps[g_p].jetpack_on )
                       j = 1;
                else if( (l&4096) && ps[g_p].steroids_amount > 0 && ps[g_p].steroids_amount < 400 )
                       j = 1;
                else if( (l&8192) && ps[g_p].on_ground)
                       j = 1;
                else if( (l&16384) && getSprite(ps[g_p].i)->_xrepeat > 32 && getSprite(ps[g_p].i)->_extra > 0 && ps[g_p].timebeforeexit == 0 )
                       j = 1;
                else if( (l&32768) && getSprite(ps[g_p].i)->_extra <= 0)
                       j = 1;
                else if( (l&65536L) )
                {
                    if(getSprite(g_i)->_picnum == APLAYER && ud.multimode > 1)
                        j = getincangle(ps[otherp].ang,getangle(ps[g_p].posx-ps[otherp].posx,ps[g_p].posy-ps[otherp].posy));
                    else
                        j = getincangle(ps[g_p].ang,getangle(getSprite(g_i)->_x-ps[g_p].posx,getSprite(g_i)->_y-ps[g_p].posy));

                    if( j > -128 && j < 128 )
                        j = 1;
                    else
                        j = 0;
                }

                parseifelse((int32_t) j);

            }
            break;
        case 56:
            insptr++;
            parseifelse(getSprite(g_i)->_extra <= *insptr);
            break;
        case 58:
            insptr += 2;
            guts(getSprite(g_i),*(insptr-1),*insptr,g_p);
            insptr++;
            break;
        case 59:
            insptr++;
//            if(getSprite(g_i)->_owner >= 0 && sprite[getSprite(g_i)->_owner].picnum == *insptr)
  //              parseifelse(1);
//            else
            parseifelse( getHittype(g_i)->picnum == *insptr);
            break;
        case 61:
            insptr++;
            forceplayerangle(&ps[g_p]);
            return 0;
        case 62:
            insptr++;
            parseifelse( (( getHittype(g_i)->floorz - getHittype(g_i)->ceilingz ) >> 8 ) < *insptr);
            break;
        case 63:
            parseifelse( sync[g_p].bits&(1<<29));
            break;
        case 64:
            parseifelse(getSector(getSprite(g_i)->_sectnum)->ceilingstat&1);
            break;
        case 65:
            parseifelse(ud.multimode > 1);
            break;
        case 66:
            insptr++;
            if( getSector(getSprite(g_i)->_sectnum)->lotag == 0 )
            {
                neartag(getSprite(g_i)->_x,getSprite(g_i)->_y,getSprite(g_i)->_z-(32<<8),getSprite(g_i)->_sectnum,getSprite(g_i)->_ang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,768L,1);
                if( neartagsector >= 0 && isanearoperator(getSector(neartagsector)->lotag) )
                    if( (getSector(neartagsector)->lotag&0xff) == 23 || getSector(neartagsector)->floorz == getSector(neartagsector)->ceilingz )
                        if( (getSector(neartagsector)->lotag&16384) == 0 )
                            if( (getSector(neartagsector)->lotag&32768) == 0 )
                        {
                            j = headspritesect[neartagsector];
                            while(j >= 0)
                            {
                                if(getSprite(j)->_picnum == ACTIVATOR)
                                    break;
                                j = spr_add_data[j].nextspritesect;
                            }
                            if(j == -1)
                                operatesectors(neartagsector,g_i);
                        }
            }
            break;
        case 67:
            parseifelse(ceilingspace(getSprite(g_i)->_sectnum));
            break;

        case 74:
            insptr++;
            if(getSprite(g_i)->_picnum != APLAYER)
                setHittype_tempang(g_i, getSprite(g_i)->_pal);
            setSprite_pal(g_i, *insptr);
            insptr++;
            break;

        case 77:
            insptr++;
            setSprite_picnum(g_i, *insptr);
            insptr++;
            break;

        case 70:
            parseifelse( dodge(g_i) == 1);
            //getSprite(g_i);
            break;
        case 71:
            if( badguy(getSprite(g_i)) )
                parseifelse( ud.respawn_monsters );
            else if( inventory(getSprite(g_i)) )
                parseifelse( ud.respawn_inventory );
            else
                parseifelse( ud.respawn_items );
            break;
        case 72:
            insptr++;
//            getglobalz(g_i);
            parseifelse( (getHittype(g_i)->floorz - getSprite(g_i)->_z) <= ((*insptr)<<8));
            break;
        case 73:
            insptr++;
//            getglobalz(g_i);
            parseifelse( ( getSprite(g_i)->_z - getHittype(g_i)->ceilingz ) <= ((*insptr)<<8));
            break;
        case 14:

            insptr++;
            ps[g_p].pals_time = *insptr;
            insptr++;
            for(j=0;j<3;j++)
            {
                ps[g_p].pals[j] = *insptr;
                insptr++;
            }
            break;

/*        case 74:
            insptr++;
            getglobalz(g_i);
            parseifelse( (( getHittype(g_i)->floorz - getHittype(g_i)->ceilingz ) >> 8 ) >= *insptr);
            break;
*/
        case 78:
            insptr++;
            parseifelse( getSprite(ps[g_p].i)->_extra < *insptr);
            break;

        case 75:
            {
                insptr++;
                j = 0;
                switch(*(insptr++))
                {
                    case 0:if( ps[g_p].steroids_amount != *insptr)
                           j = 1;
                        break;
                    case 1:if(ps[g_p].shield_amount != max_player_health )
                            j = 1;
                        break;
                    case 2:if(ps[g_p].scuba_amount != *insptr) j = 1;break;
                    case 3:if(ps[g_p].holoduke_amount != *insptr) j = 1;break;
                    case 4:if(ps[g_p].jetpack_amount != *insptr) j = 1;break;
                    case 6:
                        switch(getSprite(g_i)->_pal)
                        {
                            case  0:
                                if(ps[g_p].got_access&1)
                                    j = 1;
                                break;
                            case 21:
                                if(ps[g_p].got_access&2)
                                    j = 1;
                                break;
                            case 23:
                                if(ps[g_p].got_access&4)
                                    j = 1;
                                break;
                        }
                        break;
                    case 7:
                        if(ps[g_p].heat_amount != *insptr)
                            j = 1;
                        break;
                    case 9:
                        if(ps[g_p].firstaid_amount != *insptr)
                        {
                            j = 1;
                        }
                        break;
                    case 10:
                        if(ps[g_p].boot_amount != *insptr)
                            j = 1;
                        break;
                }

                parseifelse(j);
                break;
            }
        case 38:
            insptr++;
            if( ps[g_p].knee_incs == 0 && getSprite(ps[g_p].i)->_xrepeat >= 40 )
                if( cansee(getSprite(g_i)->_x,getSprite(g_i)->_y,getSprite(g_i)->_z-(4<<8),getSprite(g_i)->_sectnum,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz+(16<<8),getSprite(ps[g_p].i)->_sectnum) )
            {
                ps[g_p].knee_incs = 1;
                if(ps[g_p].weapon_pos == 0)
                    ps[g_p].weapon_pos = -1;
                ps[g_p].actorsqu = g_i;
            }
            break;
        case 90:
            {
                short s1;

                s1 = getSprite(g_i)->_sectnum;

                j = 0;

                    updatesector(getSprite(g_i)->_x+108,getSprite(g_i)->_y+108,&s1);
                    if( s1 == getSprite(g_i)->_sectnum )
                    {
                        updatesector(getSprite(g_i)->_x-108,getSprite(g_i)->_y-108,&s1);
                        if( s1 == getSprite(g_i)->_sectnum )
                        {
                            updatesector(getSprite(g_i)->_x+108,getSprite(g_i)->_y-108,&s1);
                            if( s1 == getSprite(g_i)->_sectnum )
                            {
                                updatesector(getSprite(g_i)->_x-108,getSprite(g_i)->_y+108,&s1);
                                if( s1 == getSprite(g_i)->_sectnum )
                                    j = 1;
                            }
                        }
                    }
                    parseifelse( j );
            }

            break;
        case 80:
            insptr++;
            FTA(*insptr,&ps[g_p],0);
            insptr++;
            break;
        case 81:
            parseifelse( floorspace(getSprite(g_i)->_sectnum));
            break;
        case 82:
#if SAVE_SPACE_ON_MOVFLAG
            parseifelse( getHittype(g_i)->movflag);
#else
            parseifelse( (getHittype(g_i)->movflag&49152) > 16384 );
#endif
            break;
        case 83:
            insptr++;
            switch(getSprite(g_i)->_picnum)
            {
                case FEM1:
                case FEM2:
                case FEM3:
                case FEM4:
                case FEM5:
                case FEM6:
                case FEM7:
                case FEM8:
                case FEM9:
                case FEM10:
                case PODFEM1:
                case NAKED1:
                case STATUE:
                    if(getSprite(g_i)->_yvel) operaterespawns(getSprite(g_i)->_yvel);
                    break;
                default:
                    if(getSprite(g_i)->_hitag >= 0) operaterespawns(getSprite(g_i)->_hitag);
                    break;
            }
            break;
        case 85:
            insptr++;
            parseifelse( getSprite(g_i)->_pal == *insptr);
            break;

        case 111:
            insptr++;
            j = klabs(getincangle(ps[g_p].ang,getSprite(g_i)->_ang));
            parseifelse( j <= *insptr);
            break;

        case 109:
#if USE_SOUNDOWNER
            for(j=1;j<NUM_SOUNDS;j++)
                if( SoundOwner[j][0].i == g_i )
                    break;
#else
            j = getNumSoundForOwner(g_i);
#endif
            parseifelse( j == NUM_SOUNDS );
            break;
        default:
            killit_flag = 1;
            break;
    }
    return 0;
}

void execute(short i,short p,int32_t x)
{
    uint8_t  done;

    g_i = i;
    g_p = p;
    g_x = x;
    //&sprite[g_i];
    //g_spoffs = &sprite_offsets[g_i];
    //g_spdist = &sprite_clipdist[g_i];
//    g_t = &getHittype(g_i)->temp_data[0];
#if USE_ACTORSCRPTR
    if( actorscrptr[getSprite(g_i)->picnum] == 0 ) return;
    insptr = 4 + (actorscrptr[getSprite(g_i)->picnum]);
#else
    if( getscriptptr(actorscridx[getSprite(g_i)->_picnum]) == NULL ) return;
    insptr = 4 + (getscriptptr(actorscridx[getSprite(g_i)->_picnum]));

#endif

    killit_flag = 0;

    if(getSprite(g_i)->_sectnum < 0 || getSprite(g_i)->_sectnum >= MAXSECTORS)
    {
        if(badguy(getSprite(g_i)))
            ps[g_p].actors_killed++;
        deletesprite(g_i);
        return;
    }


    if(getHittypeTemp(g_i, 4))
    {
        setSprite_lotag(g_i, getSprite(g_i)->_lotag + TICSPERFRAME);
        if (getSprite(g_i)->_lotag > * (decodescriptptr(getHittypeTemp(g_i, 4)) + 4))
        {
            setHittypeTemp(g_i, 2, getHittypeTemp(g_i, 2) + 1); //g_t[2]++;
            setSprite_lotag(g_i, 0);
            setHittypeTemp(g_i, 3, getHittypeTemp(g_i, 3) + *(decodescriptptr(getHittypeTemp(g_i, 4)) + 3)); // g_t[3] += *(decodescriptptr(getHittypeTemp(g_i, 4)) + 3);
        }
        if( klabs(getHittypeTemp(g_i, 3)) >= klabs( *(decodescriptptr(getHittypeTemp(g_i, 4))+1) * *(decodescriptptr(getHittypeTemp(g_i, 4))+3) ) )
            setHittypeTemp(g_i, 3, 0); //g_t[3] = 0;
    }

    do
        done = parse();
    while( done == 0 );

    if(killit_flag == 1)
    {
        if(ps[g_p].actorsqu == g_i)
            ps[g_p].actorsqu = -1;
        deletesprite(g_i);
    }
    else
    {
        // note: move changes sprite g_i!
        move();

        if( getSprite(g_i)->_statnum == 1)
        {
            if( badguy(getSprite(g_i)) )
            {
                if( getSprite(g_i)->_xrepeat > 60 ) return;
                if( ud.respawn_monsters == 1 && getSprite(g_i)->_extra <= 0 ) return;
            }
            else if( ud.respawn_items == 1 && (getSprite(g_i)->_cstat&SPRITE_INVISIBLE) ) return;

            if(getHittype_timetosleep(g_i) > 1)
                setHittype_timetosleep(g_i, getHittype_timetosleep(g_i) -1);
            else if(getHittype_timetosleep(g_i) == 1)
            {
    // next-hack: added the check that the sprite is not a FIRE or FIRE2 type.
    // this took hours to find out the cause of a bug, for which apparently randomly
    // the two fires inside the box close to the cinema in E1L1 froze. It was later discovered
    // they systematically freeze after about 50 seconds, not randomly.
    // This is because in parse(), case 8-9 the hittype[n].timetosleep is set to non-0 value
    // if the distance is larger than MAXSLEEPDIST (16384), which is always true at the beginning
    // of the level.
    // Therefore, the instructions above will progressively decrement the time, and it will put
    // the sprites in stat 2, which seems to be the sleepy state, where the sprite is not even
    // animated.
    // Noticeably, by using the original chocolate DN3D + the GRP of the Atomic edition, the same
    // bug does not appear. Also using the original DN3D game (1.3 version), this issue does not
    // appear. Using the chocolate DN3D + the GRP of the original DN3D game (1.3), the bug appears
    // as well. We verified that the E1L1 of the original DN3D and the atomic version are different,
    // This suggests that this bug was found there and corrected. However this also means that
    // the source code of DN3D 1.5 is NOT 100% compatible with the original game.
    // Note: this is just a workaround, we are not sure if:
    // a) there are other things that should be included in the check.
    // b) this won't broke the correct game behavior.
    if (getSprite_picnum(g_i) != FIRE && getSprite_picnum(g_i) != FIRE2)
    {
                 changespritestat(g_i,2);
        }
        }
        }
        else if(getSprite(g_i)->_statnum == 6)
            switch(getSprite(g_i)->_picnum)
            {
                case RUBBERCAN:
                case EXPLODINGBARREL:
                case WOODENHORSE:
                case HORSEONSIDE:
                case CANWITHSOMETHING:
                case FIREBARREL:
                case NUKEBARREL:
                case NUKEBARRELDENTED:
                case NUKEBARRELLEAKED:
                case TRIPBOMB:
                case EGG:
                    if(getHittype_timetosleep(g_i) > 1)
                        setHittype_timetosleep(g_i, getHittype_timetosleep(g_i) -1);
                    else if(getHittype_timetosleep(g_i) == 1)
                        changespritestat(g_i,2);
                    break;
            }
    }
}





// "Duke 2000"
// "Virchua Duke"
// "Son of Death
// "Cromium"
// "Potent"
// "Flotsom"

// Volume One
// "Duke is brain dead",
// "BOOT TO THE HEAD"
// Damage too duke
// Weapons are computer cont.  Only logical thinking
// is disappearing.
// " Flips! "
// Flash on screen, inst.
// "BUMS"
// "JAIL"/"MENTAL WARD (Cop code for looney?  T. asks Cop.)"
// "GUTS OR GLORY"

// ( Duke's Mission

// Duke:    "Looks like some kind of transporter...?"
// Byte:    "...YES"

// Duke:    "Waa, here goes nuthin'. "
// (Duke puts r. arm in device)

// Duke:    AAAAHHHHHHHHHHHHHHHHHHHHHHHHH!!!
// (Duke's arm is seved.)
// Byte:    NO.NO.NO.NO.NO.NO.NO...
// ( Byte directs duke to the nearest heat source)
// (Shut Up Mode)
// ( Duke Staggers, end of arm bleeding, usual oozing arm guts. )
// Byte: Left, Left, Left, Left, Right.
// ( Duke, loozing consc, trips on broken pipe, )
// ( hits temple on edge of step. )
// ( Rats everywhere, byte pushing them away with weapon,
// ( eventually covered, show usual groosums, Duke appears dead
// ( Duke wakes up, in hospital, vision less blurry
// ( Hospital doing brain scan, 1/3 cran. mass MISSING!
// Doc: Hummm?  ( Grabbing upper lip to "appear" smart. )

// Stand back boys

// Schrapnel has busted my scull!
// Now I'm insane, Mental ward, got to escape.
// Search light everywhere.

// (M)Mendor, The Tree Dweller.
// (M)BashMan, The Destructor.
// (M)Lash, The Scavenger.
// (F)Mag, The Slut.
// (F)
// NRA OR SOMETHIN'

// Duke Nukem
// 5th Dimention
// Pentagon Man!


// I Hope your not stupid!
// The 70's meet the future.
// Dirty Harry style.  70's music with futuristic edge
// The Instant De-Welder(tm)
// I think I'm going to puke...
// Badge attitude.
// He's got a Badge(LA 3322), a Bulldog, a Bronco (beat up/bondoed).
// Gfx:
// Lite rail systems
// A church.  Large cross
// Sniper Scope,
// Really use the phone
// The Boiler Room
// The IRS, nuking other government buildings?
// You wouldn't have a belt of booz, would ya?
// Slow turning signes
// More persise shooting/descructions
// Faces, use phoneoms and its lookup.  Talking, getting in fights.
// Drug dealers, pimps, and all galore
// Weapons, Anything lying around.
// Trees to clime, burning trees.
// Sledge Hammer, Sledge hammer with Spike
// sancurary, get away from it all.
// Goodlife = ( War + Greed ) / Peace
// Monsterism           (ACTION)
// Global Hunter        (RPG)
// Slick a Wick         (PUZZLE)
// Roach Condo          (FUNNY)
// AntiProfit           (RPG)
// Pen Patrol           (TD SIM)
// 97.5 KPIG! - Wanker County
// "Fauna" - Native Indiginouns Animal Life

