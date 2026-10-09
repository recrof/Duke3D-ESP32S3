/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */

#ifndef _INCLUDE_BUILD_H_
#define _INCLUDE_BUILD_H_
#include "maincfg.h"
#include <inttypes.h>
#ifdef WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
// interpolation types
#define INTERP_TYPE_NOT_ALLOCATED       0
#define INTERP_TYPE_WALL_X              1
#define INTERP_TYPE_WALL_Y              2
#define INTERP_TYPE_SECTOR_CEILINGZ     3
#define INTERP_TYPE_SECTOR_FLOORZ       4
//
#define ORIGINAL_VALUES 0
#if ORIGINAL_VALUES
#error ORIGINAL VALUES!
    #define MAXSECTORS  1024
    #define MAXWALLS 8192
    #define MAXWALLS_TEMP_BUFF      8192
    #define MAXSPRITES 4096 // was 4096. Cannot be too small, because sprites are also transparent walls - 512 is not enough...
    #define MAXTILES 9216//6144 //9216

#else
    #define MAXWALLS_TEMP_BUFF      4096
    #define MAXWALLS                5472//5500// 5471//8192 (crashes at Lunar apocalypse with only 2048)
#if !ATOMIC_EDITION
    #define MAXTILES                3842//6144 //9216
    #define MAXSECTORS              744
    #define MAXSPRITES              1200 // The maximum is 1179 in 1.3d
#else
    #define MAXSPRITES              1296 // The maximum is 1278 in atomic
    #define MAXTILES                5200
    #define MAXSECTORS              856//  (739 max for 1.3d)
#endif
#endif
#define MAXWALLSB 600//512 //2048 TO BE TWEAKED!. Moved from engine.c

#define MAXRAMWALLS             700      // ouch ouch ouch!
#define MAXRAMSPRITES           500      // let's hope it is enough!
#define MAXRAMSECTORS           128      // to be tuned.


#define MAXSTATUS 127//1024                  // probably fine. Saw 15, so 16 would be ok
#define MAXXDIM 320
#define MAXYDIM 240
#define MAXPALOOKUPS 256
#define MAXPSKYTILES 8
#define MAXSPRITESONSCREEN  256//276 ok for 309 //256//128//1024

#define CLIPMASK0 (((1L)<<16)+1L)
#define CLIPMASK1 (((256L)<<16)+64L)


	/*
     * Make all variables in BUILD.H defined in the ENGINE,
	 *  and externed in GAME
     * (dear lord.  --ryan.)
     */
#ifdef ENGINE
	#define EXTERN
#else
	#define EXTERN extern
#endif
#define PACK_ALL 0
#if PACK_ALL
#pragma pack(1)
#endif
/*
 * ceilingstat/floorstat:
 *   bit 0: 1 = parallaxing, 0 = not                                 "P"
 *   bit 1: 1 = groudraw, 0 = not
 *   bit 2: 1 = swap x&y, 0 = not                                    "F"
 *   bit 3: 1 = double smooshiness                                   "E"
 *   bit 4: 1 = x-flip                                               "F"
 *   bit 5: 1 = y-flip                                               "F"
 *   bit 6: 1 = Align texture to first wall of sector                "R"
 *   bits 7-8:                                                       "T"
 *          00 = normal floors
 *          01 = masked floors
 *          10 = transluscent masked floors
 *          11 = reverse transluscent masked floors
 *   bits 9-15: reserved
 */
/* from report
minSector.wallptr is 0x00000000, maxSector.wallptr is 0x0000155b. Minimum field size 13 bits
minSector.wallnum is 0x00000000, maxSector.wallnum is 0x000000e2. Minimum field size 8 bits
minSector.ceilingz is 0xfff3a800, maxSector.ceilingz is 0x000b0800. Minimum field size 21 bits
minSector.floorz is 0xfff9b800, maxSector.floorz is 0x000cac00. Minimum field size 21 bits
minSector.ceilingstat is 0x00000000, maxSector.ceilingstat is 0x0000007e. Minimum field size 7 bits
minSector.floorstat is 0x00000000, maxSector.floorstat is 0x0000007e. Minimum field size 7 bits
minSector.ceilingpicnum is 0x00000000, maxSector.ceilingpicnum is 0x00000d4f. Minimum field size 12 bits
minSector.ceilingheinum is 0x0000e000, maxSector.ceilingheinum is 0x000034ca. Minimum field size 15 bits
minSector.ceilingshade is 0x00000080, maxSector.ceilingshade is 0x00000022. Minimum field size 8 bits
minSector.ceilingpal is 0x00000000, maxSector.ceilingpal is 0x00000017. Minimum field size 5 bits
minSector.ceilingxpanning is 0x00000000, maxSector.ceilingxpanning is 0x000000ff. Minimum field size 8 bits
minSector.ceilingypanning is 0x00000000, maxSector.ceilingypanning is 0x000000ff. Minimum field size 8 bits
minSector.floorpicnum is 0x00000000, maxSector.floorpicnum is 0x00000d58. Minimum field size 12 bits
minSector.floorheinum is 0x0000a000, maxSector.floorheinum is 0x00003e00. Minimum field size 16 bits
minSector.floorshade is 0x00000080, maxSector.floorshade is 0x00000021. Minimum field size 8 bits
minSector.floorpal is 0x00000000, maxSector.floorpal is 0x00000017. Minimum field size 5 bits
minSector.floorxpanning is 0x00000000, maxSector.floorxpanning is 0x000000ff. Minimum field size 8 bits
minSector.floorypanning is 0x00000000, maxSector.floorypanning is 0x000000ff. Minimum field size 8 bits
minSector.visibility is 0x00000000, maxSector.visibility is 0x000000ff. Minimum field size 8 bits
minSector.filler is 0x00000000, maxSector.filler is 0x00000000. Minimum field size 1 bits
minSector.lotag is 0x0000fffe, maxSector.lotag is 0x00007fff. Minimum field size 16 bits
minSector.hitag is 0x00000000, maxSector.hitag is 0x00002710. Minimum field size 14 bits
minSector.extra is 0x0000ffff, maxSector.extra is 0x00000020. Minimum field size 7 bits
*/

	/* 40 bytes */
#if NO_MINIMIZE_SECTORS
typedef struct
{
	short wallptr, wallnum;
	int32_t ceilingz, floorz;   // not constants, and must stay 32 bit
	short ceilingstat, floorstat;
	short ceilingpicnum, ceilingheinum;
	int8_t ceilingshade;
	uint8_t  ceilingpal, ceilingxpanning, ceilingypanning;
	short floorpicnum, floorheinum;
	int8_t floorshade;
	uint8_t  floorpal, floorxpanning, floorypanning;
	uint8_t  visibility, filler;
	short lotag, hitag, extra;
} sectortype;
#else
#define SEPARATE_SECTOR_EXTRA 1
#define SEPARATE_SECTOR_CEILINGSHADE 1
#define SEPARATE_SECTOR_FLOORSHADE 1
#define SEPARATE_SECTOR_FLOORXPANNING 1
typedef struct
{
	int32_t ceilingz :21;      // not constant, and must stay 32 bit due to anim
    uint32_t ceilingstat : 9;     // not constant, but can reduced to 9 bits

	int32_t floorz : 21;        // not constant, and must stay 32 bit due to anim. Even if I have not seen using the top bis (tested but not set?).
	uint32_t floorstat : 9;       // not constant, but can reduced to 9 bits. Even if I have not seen using the top bis (tested but not set?.


	uint32_t floorypanning : 8;
	int32_t ceilingpicnum : 14;
	uint32_t  ceilingpal : 5;
	uint32_t  floorpal : 5;

	int32_t floorpicnum : 14;  //+- 8192
    int32_t lotag : 16;

	int32_t hitag : 16 ;
    int32_t floorheinum : 16;

    // IF NEEDED: 8 bits free.
    uint16_t ceilingxpanning : 8;
	uint16_t ceilingypanning : 8;
#if !SEPARATE_SECTOR_CEILINGSHADE
	int32_t ceilingshade : 8;
#endif // !SEPARATE_SECTOR_CEILINGSHADE
#if !SEPARATE_SECTOR_FLOORSHADE
    int32_t floorshade : 8;
#endif // !SEPARATE_SECTOR_FLOORSHADE
#if !SEPARATE_SECTOR_FLOORXPANNING
	uint32_t floorxpanning : 8;
#endif // SEPARATE_SECTOR_FLOORXPANNING
#if !SEPARATE_SECTOR_EXTRA
	int32_t extra : 16;
#endif // SEPARATE_SECTOR_EXTRA
} __attribute__ ((packed)) sectortype;
typedef struct
{
    #if 0 // let's test
    // constant data
	int16_t ceilingheinum;
	uint8_t  visibility;  // CONST
	uint8_t filler; // CONST
	short wallptr;   // constant and can be decreased in size to 13 bits
	short wallnum;   // constant and can be decreased in size to 8 bits
	#else
    int16_t ceilingheinum;
	short wallptr;   // constant and can be decreased in size to 13 bits
	uint8_t  visibility;  // CONST
//	short wallnum;   // constant and can be decreased in size to 8 bits
    uint8_t wallnum;
	#endif
} const_sectortype;
#endif
EXTERN uint8_t isLoadingLevel;
extern uint8_t wall_extraIsMinusOne[(MAXWALLS  + 7 )/ 8 ];  // 1 if extra is -1
typedef struct
{
	short wallptr, wallnum;
	int32_t ceilingz, floorz;
	short ceilingstat, floorstat;
	short ceilingpicnum, ceilingheinum;
	int8_t ceilingshade;
	uint8_t  ceilingpal, ceilingxpanning, ceilingypanning;
	short floorpicnum, floorheinum;
	int8_t floorshade;
	uint8_t  floorpal, floorxpanning, floorypanning;
	uint8_t  visibility, filler;
	short lotag, hitag, extra;
} disk_sectortype;
#if USE_BLOCKMAP
EXTERN uint8_t sectorModifiedBf[(MAXSECTORS + 31) / 4];
#endif
/*
 * cstat:
 *   bit 0: 1 = Blocking wall (use with clipmove, getzrange)         "B"
 *   bit 1: 1 = bottoms of invisible walls swapped, 0 = not          "2"
 *   bit 2: 1 = align picture on bottom (for doors), 0 = top         "O"
 *   bit 3: 1 = x-flipped, 0 = normal                                "F"
 *   bit 4: 1 = masking wall, 0 = not                                "M"
 *   bit 5: 1 = 1-way wall, 0 = not                                  "1"
 *   bit 6: 1 = Blocking wall (use with hitscan / cliptype 1)        "H"
 *   bit 7: 1 = Transluscence, 0 = not                               "T"
 *   bit 8: 1 = y-flipped, 0 = normal                                "F"
 *   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
 *   bits 10-15: reserved
 */

	/* 32 bytes */
/*
    next-hack stats:
minWall.x is 0xffff0400, maxWall.x is 0x0000ff80. Minimum field size 17 bits
minWall.y is 0xffff0080, maxWall.y is 0x0000ff80. Minimum field size 17 bits
minWall.point2 is 0x00000000, maxWall.point2 is 0x0000155e. Minimum field size 14 bits
minWall.nextwall is 0x0000ffff, maxWall.nextwall is 0x0000155e. Minimum field size 14 bits
minWall.nextsector is 0x0000ffff, maxWall.nextsector is 0x000002e2. Minimum field size 11 bits
minWall.cstat is 0x00000000, maxWall.cstat is 0x000003df. Minimum field size 11 bits
minWall.picnum is 0x00000000, maxWall.picnum is 0x00000d78. Minimum field size 13 bits
minWall.overpicnum is 0x0000ffff, maxWall.overpicnum is 0x00000500. Minimum field size 12 bits
minWall.shade is 0x00000080, maxWall.shade is 0x00000022. Minimum field size 8 bits
minWall.pal is 0x00000000, maxWall.pal is 0x00000017. Minimum field size 5 bits
minWall.xrepeat is 0x00000000, maxWall.xrepeat is 0x000000ff. Minimum field size 8 bits
minWall.yrepeat is 0x00000000, maxWall.yrepeat is 0x00000080. Minimum field size 8 bits
minWall.xpanning is 0x00000000, maxWall.xpanning is 0x000000ff. Minimum field size 8 bits
minWall.ypanning is 0x00000000, maxWall.ypanning is 0x000000ff. Minimum field size 8 bits
minWall.lotag is 0x00000000, maxWall.lotag is 0x000017eb. Minimum field size 14 bits
minWall.hitag is 0x00000000, maxWall.hitag is 0x0000000b. Minimum field size 5 bits
minWall.extra is 0x0000ffff, maxWall.extra is 0x00000000. Minimum field size 1 bits
*/
#define NO_MINIMIZE_WALLS 0
#if NO_MINIMIZE_WALLS
#warning create static walls. Static walls are constant. How to detect static walls ?
typedef struct
{
	int32_t x, y;  // not constant as they might move?
	short point2;   // not constant
	short nextwall; // seems constant
	short nextsector;  //seems constant
	short cstat;        // seen assignment to 65535 - 65 :O
	short picnum;
	short overpicnum;
	int8_t shade;
	uint8_t  pal, xrepeat, yrepeat, xpanning, ypanning;
	short lotag;
	short hitag;
	short extra; // extra shall have as many bits as picnum
} walltype;
#else
#define HITAG_CONSTANT 1
#if !HITAG_CONSTANT

typedef struct
{
    int32_t x;
    int32_t y;           // could be 17 bits each, but we need their ptr :( Later could be changed.
//
	short _lotag;    // 14 bit. But let's keep 16. NOT CONSTANT.
    uint8_t _xpanning, _ypanning; // not constant.
//
	int32_t _cstat : 10;        // not constant 10 bits?
//	int32_t _shade : 8;       // 8 bits
	int32_t _point2: 14;   // not constant. This is a wall ptr, and since it is a wall, then need to go up to MAXWAWLLS
//
    uint32_t  _pal : 5;       // 5  bits
	int32_t _picnum : 14;       // not constant. 12 bits
	int32_t _extra : 13; // extra shall have as many bits as picnum. 13 bits ?

    int32_t _overpicnum : 16;   // not constant. let's keep 13 bit even if 11
    int32_t _hitag : 16;        // can be 9999. Note: wall hitag is ONLY set to 9999. So we could use like a bitfield array if we see that many walls are modified only on hitag.
} walltype;
#else
typedef struct
{
    int32_t x : 17;
    int32_t _cstat : 10;        // not constant 10 bits?
    uint32_t  _pal : 5;       // 5  bits

//
    int32_t y : 17;           // could be 17 bits each, but we need their ptr :( Later could be changed.
	int32_t _picnum : 14;       // not constant. 12 bits signed?

    // Note: the below might be constant!!!
  //  uint32_t _point2: 13;   //  constant. This is a wall ptr, and since it is a wall, then need to go up to MAXWALLS. Seems not negative
    //
    uint32_t _ypanning : 8; // not constant.
	uint32_t _xpanning : 8;
    uint32_t _lotag : 16;    // 13 bit changed to unsigned. But let's keep 16. NOT CONSTANT.
	//
    int32_t _overpicnum : 16;   // can be 12 bits signed?
	int32_t _extra : 16; // extra is set to 1, -1 (handled separately)  or 0, or picnum when picnum is FEMPIC1-FEMPIC3, i.e < 2047
} walltype;

#endif

typedef struct
{
#if 0
	short nextwall;          // max MAXWALLS ( -1 admitted)
	short nextsector;       // there are less than 1024 sectors. can be negative, so 11 bits
    uint8_t _xrepeat, _yrepeat;  // CONSTANT!
#if HITAG_CONSTANT
    short _hitag            // max 327 unsigned: 9 bits
#endif
#else
#if !HITAG_CONSTANT
#error
#endif
    uint32_t _xrepeat : 8;
    int32_t nextwall : 14;         // +/- 8192
    uint32_t _hitag : 9;           // max 327 unsigned: 9 bits
    //
    uint32_t _yrepeat : 8;  // CONSTANT!
    int32_t nextsector : 11;         // +/- 1024
    uint32_t _point2 : 13;          // from 0 to MAXWALLS < 8192
#endif
} const_walltype;
typedef struct
{
	int32_t x, y;  // not constant as they might move?
	short _point2;   // not constant
	short nextwall; // seems constant
	short nextsector;  //seems constant
	short _cstat;        // seen assignment to 65535 - 65 :O
	short _picnum;
	short _overpicnum;
	int8_t _shade;
	uint8_t  _pal, _xrepeat, _yrepeat, _xpanning, _ypanning;
	short _lotag;
	short _hitag;   // CONSTANT
	short _extra; // extra shall have as many bits as picnum
} disk_walltype;

#endif

/*
 * cstat:
 *   bit 0: 1 = Blocking sprite (use with clipmove, getzrange)       "B"
 *   bit 1: 1 = transluscence, 0 = normal                            "T"
 *   bit 2: 1 = x-flipped, 0 = normal                                "F"
 *   bit 3: 1 = y-flipped, 0 = normal                                "F"
 *   bits 5-4: 00 = FACE sprite (default)                            "R"
 *             01 = WALL sprite (like masked walls)
 *             10 = FLOOR sprite (parallel to ceilings&floors)
 *   bit 6: 1 = 1-sided sprite, 0 = normal                           "1"
 *   bit 7: 1 = Real centered centering, 0 = foot center             "C"
 *   bit 8: 1 = Blocking sprite (use with hitscan / cliptype 1)      "H"
 *   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
 *   bits 10-14: reserved
 *   bit 15: 1 = Invisible sprite, 0 = not invisible
 */
 #define SPRITE_INVISIBLE      (1 << 10) // was 32768
 #define ORIGINAL_SPRITE_INVISIBLE      (1 << 15) // original value, 32768


	/* 44 bytes */
typedef struct
{
	int32_t x; // : 17;
	int32_t y;// : 17;
	int32_t z;// : 21;        // 17, 17, 21 ?
	short cstat;    // must be 16 :(
	short picnum;// : 14;    // 14?
    int8_t shade;
	uint8_t  pal;// : 5;
	uint8_t clipdist;
	const uint8_t filler; //const an not used. is it pad ?
	uint8_t  xrepeat, yrepeat;
	int8_t xoffset, yoffset;
	short sectnum;// : 11;
	short statnum;// : 7;  //  up to MAXSTATUS, inclusive. 7 bits if MAXSTATUS = 127
	short ang;// : 11;
	short owner;//: 12;
	short xvel;// : 12;
	short yvel;// : 12;
	short zvel;//: 12; // +- 1024 ? 12 bits
	short lotag, hitag, extra;
} disk_spritetype;


#if !NO_MINIMIZE_SPRITE
typedef struct
{
#if SPRITETYPE_HAS_OFFSETS
#define SPRITETYPE_HAS_CLIPDIST 0
	int32_t _z : 21;        //
	int32_t _sectnum : 11;

    int32_t _owner: 12;
	int32_t _xvel  : 12;   // can be smaller
    int32_t _shade : 8;


	int32_t _lotag:16;
	int32_t _hitag : 16;

	int32_t _y : 17;
	uint32_t _statnum: 7;  //  up to MAXSTATUS, inclusive. 7 bits if MAXSTATUS = 127
	uint32_t _yrepeat : 8;


	uint32_t  _pal : 5;
	uint32_t _ang : 11;
	int32_t _zvel : 16; //int32_t zvel : 12; // +- 6144 ? 14 bits


    int32_t _extra:16;
	uint32_t _yoffset : 8;
	uint32_t _xoffset : 8;

	int32_t _x : 17;
	int32_t _picnum : 14;    // 14?

	uint32_t _cstat : 11;    // brought to 11 bit
	int32_t _yvel : 13; // must be at least 13 (12 will make the cinema curtain not move)
    uint32_t _xrepeat : 8;
#if SPRITETYPE_HAS_CLIPDIST
	#error

    uint32_t clipdist : 8;
#endif
    #else
#define SPRITETYPE_HAS_CLIPDIST 1
	int32_t z : 21;        //
	int32_t sectnum : 11;


    int32_t owner: 12;
	int32_t xvel  : 12;   // can be smaller
    int32_t shade : 8;


	int32_t lotag:16;
	int32_t hitag : 16;

	int32_t y : 17;
	uint32_t statnum: 7;  //  up to MAXSTATUS, inclusive. 7 bits if MAXSTATUS = 127
	uint32_t yrepeat : 8;


	uint32_t  pal : 5;
	uint32_t ang : 11;
	int32_t zvel : 16; //int32_t zvel : 12; // +- 6144 ? 14 bits


    int32_t extra:16;
	uint32_t clipdist : 8;
    uint32_t xrepeat : 8;

	int32_t x : 17;
	int32_t picnum : 14;    // 14?

	uint32_t cstat : 11;    // brought to 11 bit
	int32_t yvel : 12;
#endif
} spritetype;

#if !SPRITETYPE_HAS_OFFSETS
typedef struct
{
    uint8_t yoffset;
	uint8_t xoffset;
}  spritetypeoffs;
#else
typedef spritetype spritetypeoffs;

#endif
typedef struct
{
    #if 0
    int32_t x, y, z;
	short cstat;
	short picnum;
	int8_t shade;
	uint8_t  pal;
	// uint8_t clipdist;
	// uint8_t filler;
	uint8_t  xrepeat, yrepeat;
	int8_t xoffset, yoffset;
	short sectnum/*MAXSECTORS*/, statnum;
	short ang;
	short owner;//, xvel, yvel, zvel;
	short lotag,/* hitag, */extra;
	#else
	int32_t z : 21;        //
	int32_t sectnum : 11;      // not sure why shall be different. check mycursectnum and screenpeek.

	int32_t y : 17;
	uint32_t statnum: 7;  //we might create getTsprite_statnum(i) where we check if a bit statnum99 is 1, then we return 99, or we return sprites[tsprites[i].owner].statnum
    uint32_t yrepeat : 8;

    int32_t owner: 12;
    int32_t shade : 8;
	uint32_t cstat : 11;    // brought to 11 bit
#if !TSPRITE_HAS_LOTAG
    uint32_t lotagIs27 : 1;   // 1 if lotag is 27
#endif

	int32_t x : 17;
	int32_t picnum : 14;    // 14?
    uint32_t extraBiggerThanZero : 1;

#if TSPRITE_HAS_LOTAG
    int32_t lotag:16;
#endif
    uint32_t xrepeat : 8;
	uint32_t  pal : 5;
	uint32_t ang : 11;


#if TSPRITE_HAS_EXTRA
    int32_t _extra:16;
#endif


#if TSPRITE_YOFFSET
	uint32_t yoffset : 8;
#endif

#if TSPRITE_XOFFSET
	uint32_t xoffset : 8;
#endif

	// short hitag;
	//int32_t xvel  : 12;   // can be smaller

	//int32_t yvel : 12;

	//int32_t zvel : 14; //int32_t zvel : 12; // +- 6144 ? 14 bits
	// 	uint32_t clipdist : 8;

	#endif
} tspritetype;

#else
#define TSPRITE_HAS_EXTRA 1
#define TSPRITE_HAS_LOTAG 1
typedef struct
{
	int32_t x; // : 17;
	int32_t y;// : 17;
	int32_t z;// : 21;        // 17, 17, 21 ?
	short cstat;    // must be 16 :(
	short picnum;// : 14;    // 14?
    int8_t shade;
	uint8_t  pal;// : 5;
	uint8_t clipdist;
//	const uint8_t filler; //const an not used. is it pad ?
	uint8_t  xrepeat, yrepeat;
	int8_t xoffset, yoffset;
	short sectnum;// : 11;
	short statnum;// : 7;  //  up to MAXSTATUS, inclusive. 7 bits if MAXSTATUS = 127
	short ang;// : 11;
	short owner;//: 12;
	short xvel;// : 12;
	short yvel;// : 12;
	short zvel;//: 12; // +- 1024 ? 12 bits
	short lotag, hitag, extra;
} spritetype;
typedef spritetype tspritetype;

#endif
extern walltype ramwalls[MAXRAMWALLS];
EXTERN spritetype ramsprites[MAXRAMSPRITES];
EXTERN uint8_t ramsprites_allocated_bf[(MAXRAMSPRITES + 7)/8];
EXTERN short ramspriteIdx[MAXSPRITES];          // to be reduced
#define OPTIMIZE_RAMWALLIDX 1
#if OPTIMIZE_RAMWALLIDX
EXTERN uint8_t ramwallIdx8[MAXWALLS];          // associates each wall to a possible modified version.
EXTERN uint8_t ramwallIdx2[(MAXWALLS + 3)/4];          // associates each wall to a possible modified version.
#define TRYING_SPEEDUP_RAMWALLS 1
#if TRYING_SPEEDUP_RAMWALLS
EXTERN uint16_t *stackWallIndexes;
void setStackWallIndexes(uint16_t *stackWall);
#endif
#if WIN32
EXTERN int numberOfGetRamWallIdxCalls;
#endif
static inline unsigned int getRamWallIdx(int n)
{
#if WIN32
    numberOfGetRamWallIdxCalls++;
#endif
#if TRYING_SPEEDUP_RAMWALLS
    if (stackWallIndexes)
        return stackWallIndexes[n];
#endif
    unsigned int idx = ramwallIdx8[n];
    unsigned int idx2 = (ramwallIdx2[n / 4] >> (2 * (n %4))) & 3;
    idx |= idx2 << 8;
    return idx;
}
static inline void setRamWallIdx(int n, unsigned int v)
{
#if TRYING_SPEEDUP_RAMWALLS
    if (stackWallIndexes)
    {
        stackWallIndexes[n] = v;
        return;
    }
#endif
    ramwallIdx8[n] = v;
    int idx2 = ramwallIdx2[n / 4];  // get old value
    idx2 &= ~ (3 << (2 * (n % 4))); // clear bit
    idx2 |= (v >> 8) << (2 * (n % 4));  // set bits
    ramwallIdx2[n / 4] = idx2;          // store back;
 }

#else
EXTERN short ramwallIdx[MAXWALLS];          // associates each wall to a possible modified version.
static inline unsigned int getRamWallIdx(int n)
{
    return ramwallIdx[n];
}
static inline void setRamWallIdx(int n, unsigned int v)
{
    ramwallIdx[n] = v;
}
#endif
extern sectortype ramsectors[MAXRAMSECTORS];
#if MAXRAMSECTORS >= 255
EXTERN short ramsectorIdx[MAXSECTORS];          // associates each sector to a possible modified version.
#else
EXTERN uint8_t ramsectorIdx[MAXSECTORS];          // associates each sector to a possible modified version.
#endif

#pragma pack(4)

extern const sectortype *sectors;//[MAXSECTORS];              // these reside in flash, but their values can be overridden by a ramsector
extern const  const_sectortype *constsector;//[MAXSECTORS];    // these reside in flash and their values are not overridden.

extern int8_t wallshade[MAXWALLS];
#define WALL_IN_INT_FLASH 1
#if WALL_IN_INT_FLASH
EXTERN const walltype *walls;//[MAXWALLS];                    // this contains all data in internal flash which might be overridden by ramwalls.
EXTERN const const_walltype *constwall; //[MAXWALLS];       // to be changed to a pointer!. These values do not change
#else
EXTERN walltype walls;//[MAXWALLS];                    // this contains all data in internal flash which might be overridden by ramwalls.
EXTERN const const_walltype constwall;//[MAXWALLS];       // to be changed to a pointer!. These values do not change

#endif
EXTERN const spritetype *sprites;//[MAXSPRITES];
#if WIN32
EXTERN spritetype debugInitialSpriteValues[MAXSPRITES];
#endif
#if !SPRITETYPE_HAS_OFFSETS
    EXTERN spritetypeoffs sprite_offsets[MAXSPRITES];
    #error
#else
    #define sprite_offsets  sprites
#endif
EXTERN uint16_t mapCRC;

EXTERN int32_t spritesortcnt;
#define TEST_TSPRITE 1
#if TEST_TSPRITE
#define STACK_TSPRITES 1
#if STACK_TSPRITES
EXTERN tspritetype *tsprite; //[MAXSPRITESONSCREEN];
#else
EXTERN tspritetype tsprite[MAXSPRITESONSCREEN];
#endif
#else
EXTERN spritetype tsprite[MAXSPRITESONSCREEN];
#endif
EXTERN uint8_t  vidoption;
EXTERN int32_t xdim, ydim, numpages;

// Fast way to retrive the start of a column in the framebuffer, given a screenspace X coordinate.
//EXTERN int32_t ylookup[MAXYDIM+1];
EXTERN uint16_t ylookupValue;
static inline int32_t ylookup(int y)
{
    return y * ylookupValue;
}
static inline void setYlookup(int v)
{
    ylookupValue = v;
}
EXTERN int32_t yxaspect, viewingrange;
#if USE_MANY_MODES // next-hack: we don't use many modes. Just one
EXTERN int32_t validmodecnt;
EXTERN short validmode[256];
EXTERN int32_t validmodexdim[256], validmodeydim[256];
#endif
EXTERN short numsectors, numwalls;
EXTERN volatile int32_t totalclock;
EXTERN int32_t numframes, randomseed;
#if LOAD_TABLES
EXTERN short sintable[2048];
#else
EXTERN const short sintable[2048];
#endif
#if USE_EXT_MEM
EXTERN const uint8_t * palette;

#else
EXTERN uint8_t  palette[768];
#endif
EXTERN short numpalookups;
#if USE_EXT_MEM
EXTERN const uint8_t  **palookup;
#else
EXTERN uint8_t  *palookup[MAXPALOOKUPS];
#endif
EXTERN uint8_t  parallaxtype, showinvisibility;
EXTERN int32_t parallaxyoffs, parallaxyscale;
EXTERN int32_t visibility, parallaxvisibility;

EXTERN int32_t windowx1, windowy1, windowx2, windowy2;
//EXTERN short startumost[MAXXDIM], startdmost[MAXXDIM];
#define SET_STARTxMOST 0
#if SET_STARTxMOST
EXTERN short _startumost[MAXXDIM], _startdmost[MAXXDIM];
#else
EXTERN short _startumost, _startdmost;

#endif
static inline int startumost(int x)
{
    (void) x;
    #if SET_STARTxMOST
        return _startumost[x];
    #else
        return _startumost;
    #endif
}
static inline int startdmost(int x)
{
    (void) x;
#if SET_STARTxMOST
    return _startdmost[x];
#else
    return _startdmost; //SCREENHEIGHT
#endif
}

EXTERN short pskyoff[MAXPSKYTILES], pskybits;

EXTERN short headspritesect[MAXSECTORS+1], headspritestat[MAXSTATUS+1];
#if SPRITES_USE_DOUBLY_LINKED_LIST
#error
EXTERN short prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
#endif
#if 0
#if !SPRITETYPE_HAS_CLIPDIST
typedef struct
{
    uint8_t clipdist;
} spritetypeclipdist;
#endif
EXTERN short nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];
EXTERN spritetypeclipdist sprite_clipdist[MAXSPRITES];
#else
typedef struct
{
    int32_t nextspritesect : 12;
    int32_t nextspritestat : 12;
    uint32_t clipdist : 8;
} sprite_additional_data;
EXTERN sprite_additional_data spr_add_data[MAXSPRITES];
#endif


    /*
	 * These variables are for auto-mapping with the draw2dscreen function.
	 * When you load a new board, these bits are all set to 0 - since
	 * you haven't mapped out anything yet.  Note that these arrays are
	 * bit-mapped.
	 * If you want draw2dscreen() to show sprite #54 then you say:
	 *    spritenum = 54;
	 *    show2dsprite[spritenum>>3] |= (1<<(spritenum&7));
	 * And if you want draw2dscreen() to not show sprite #54 then you say:
	 *    spritenum = 54;
	 *    show2dsprite[spritenum>>3] &= ~(1<<(spritenum&7));
	 * Automapping defaults to 0 (do nothing).  If you set automapping to 1,
	 *    then in 3D mode, the walls and sprites that you see will show up the
	 *    next time you flip to 2D mode.
     */
EXTERN uint8_t  show2dsector[(MAXSECTORS+7)>>3];
EXTERN uint8_t  show2dwall[(MAXWALLS+7)>>3];
EXTERN uint8_t  show2dsprite[(MAXSPRITES+7)>>3];
EXTERN uint8_t  automapping;



//This is the bit vector that marks visited sector during portal flooding. Size is hence (MAXSECTORS / 8)
EXTERN uint8_t  visitedSectors[(MAXSECTORS+7)>>3];

/*************************************************************************
POSITION VARIABLES:

		POSX is your x - position ranging from 0 to 65535
		POSY is your y - position ranging from 0 to 65535
			(the length of a side of the grid in EDITBORD would be 1024)
		POSZ is your z - position (height) ranging from 0 to 65535, 0 highest.
		ANG is your angle ranging from 0 to 2047.  Instead of 360 degrees, or
			 2 * PI radians, I use 2048 different angles, so 90 degrees would
			 be 512 in my system.

SPRITE VARIABLES:

	EXTERN short headspritesect[MAXSECTORS+1], headspritestat[MAXSTATUS+1];
	EXTERN short prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
	EXTERN short nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];

	Example: if the linked lists look like the following:
		 ��������������������������������������������������������������Ŀ
		 �      Sector lists:               Status lists:               �
		 ��������������������������������������������������������������Ĵ
		 �  Sector0:  4, 5, 8             Status0:  2, 0, 8             �
		 �  Sector1:  16, 2, 0, 7         Status1:  4, 5, 16, 7, 3, 9   �
		 �  Sector2:  3, 9                                              �
		 ����������������������������������������������������������������
	Notice that each number listed above is shown exactly once on both the
		left and right side.  This is because any sprite that exists must
		be in some sector, and must have some kind of status that you define.


Coding example #1:
	To go through all the sprites in sector 1, the code can look like this:

		sectnum = 1;
		i = headspritesect[sectnum];
		while (i != -1)
		{
			nexti = nextspritesect[i];

			//your code goes here
			//ex: printf("Sprite %d is in sector %d\n",i,sectnum);

			i = nexti;
		}

Coding example #2:
	To go through all sprites with status = 1, the code can look like this:

		statnum = 1;        //status 1
		i = headspritestat[statnum];
		while (i != -1)
		{
			nexti = spr_add_data[i].nextspritestat;

			//your code goes here
			//ex: printf("Sprite %d has a status of 1 (active)\n",i,statnum);

			i = nexti;
		}

			 insertsprite(short sectnum, short statnum);
			 deletesprite(short spritenum);
			 changespritesect(short spritenum, short newsectnum);
			 changespritestat(short spritenum, short newstatnum);

TILE VARIABLES:
		NUMTILES - the number of tiles found TILES.DAT.
		TILESIZX[MAXTILES] - simply the x-dimension of the tile number.
		TILESIZY[MAXTILES] - simply the y-dimension of the tile number.
		WALOFF[MAXTILES] - the actual 32-bit offset pointing to the top-left
								 corner of the tile.
		PICANM[MAXTILES] - flags for animating the tile.

TIMING VARIABLES:
		TOTALCLOCK - When the engine is initialized, TOTALCLOCK is set to zero.
			From then on, it is incremented 120 times a second by 1.  That
			means that the number of seconds elapsed is totalclock / 120.
		NUMFRAMES - The number of times the draw3dscreen function was called
			since the engine was initialized.  This helps to determine frame
			rate.  (Frame rate = numframes * 120 / totalclock.)

OTHER VARIABLES:

		STARTUMOST[320] is an array of the highest y-coordinates on each column
				that my engine is allowed to write to.  You need to set it only
				once.
		STARTDMOST[320] is an array of the lowest y-coordinates on each column
				that my engine is allowed to write to.  You need to set it only
				once.
		SINTABLE[2048] is a sin table with 2048 angles rather than the
			normal 360 angles for higher precision.  Also since SINTABLE is in
			all integers, the range is multiplied by 16383, so instead of the
			normal -1<sin(x)<1, the range of sintable is -16383<sintable[]<16383
			If you use this sintable, you can possibly speed up your code as
			well as save space in memory.  If you plan to use sintable, 2
			identities you may want to keep in mind are:
				sintable[ang&2047]       = sin(ang * (3.141592/1024)) * 16383
				sintable[(ang+512)&2047] = cos(ang * (3.141592/1024)) * 16383
		NUMSECTORS - the total number of existing sectors.  Modified every time
			you call the loadboard function.
***************************************************************************/

#define PORTSIG  "Port by Ryan C. Gordon, Andrew Henderson, Dan Olson, Fabien Sanglard and a cast of thousands."
#if !INTERPOLATION_USES_POSPTR
typedef union
{
    struct
    {
        int16_t index : 12; // max 2048. -1 is for none
        uint16_t type : 4;
    };
    uint16_t value;
} indexedValue_t;
#endif
//Global.c
#if WIN32
#define Error(t, f, ...) Error2(t, __FILE__, __FUNCTION__, __LINE__,  f,  ##__VA_ARGS__)
void   Error2 (int errorType, const char *file, const char *function, int line, const char  *error, ...);
#else
void Error (int errorType, const char *error, ...);
#endif
int FindDistance2D(int ix, int iy);
extern short numModifiedWalls;
extern short numModifiedSectors;
extern short numModifiedSprites;
extern const uint16_t *wallSectorId;
/*#define WALL_IS_MODIFIED(index) ( 0 != ramwallIdx[(index)])
#define WALL_SET_MODIFIED(index, ramindex) do {ramwallIdx[(index)] = ramindex; } while(0)

#define COUNT_MOD_WALL(index, fieldName, value)  if (!WALL_IS_MODIFIED(index)) { WALL_SET_MODIFIED(index, numModifiedWalls); numModifiedWalls++; printf("Modified W# %d field %s to %d, file %s, line %d\r\n", numModifiedWalls, #fieldName, value, __FILE__, __LINE__); }
*/
#define VERBOSE_WALL_ALLOC 0
#if VERBOSE_WALL_ALLOC
#define NOTIFY_MOD_WALL(index, fieldName, value)  printf("Modified W# %d field %s to %d, file %s, line %d\r\n", numModifiedWalls, #fieldName, value, __FILE__, __LINE__)
#else
#define NOTIFY_MOD_WALL(index, fieldName, value)
#endif // VERBOSE_WALL_ALLOC
#define VERBOSE_SECT_ALLOC 0
#if VERBOSE_SECT_ALLOC
#define NOTIFY_MOD_SECT(index, fieldName, value)  printf("Modified S# %d field %s to %d, file %s, line %d\r\n", numModifiedSectors, #fieldName, value, __FILE__, __LINE__)
#else
#define NOTIFY_MOD_SECT(index, fieldName, value)
#endif
#define VERBOSE_SPRITE_ALLOC 0
#if VERBOSE_SPRITE_ALLOC
#define NOTIFY_LOAD_LEVEL(s, i, v) printf("Not allocating ram sprites for field %s, sprite %i, value %d\r\n", s, i, v)
#define NOTIFY_MOD_SPRITE(index, fieldName, value, oldvalue, ptr)  printf("Modified Spr# %d (total %d) field %s to %d (was %d), pointer %p\r\n", \
                                                                index, numModifiedSprites, #fieldName, value, oldvalue, ptr)
#else
#define NOTIFY_LOAD_LEVEL(s, i, v)
#define NOTIFY_MOD_SPRITE(index, fieldName, value, oldvalue, ptr)
#endif // VERBOSE_WALL_ALLOC

extern sectortype * allocateNewRamSector(int sectorIndex);
extern walltype * allocateNewRamWall(int wallIndex);
extern spritetype * allocateNewRamSprite(int spriteIndex);

static inline int getRamSpriteIdx(int index)
{
    return ramspriteIdx[index];
}
#define SPRITE_TRANSITION 0
#if SPRITE_TRANSITION
static inline const spritetype* getSprite(int index)
{
    return &sprites[index];
}
#define defineSpritesSettersAndGetters(fieldName) \
static inline spritetype * setSprite##fieldName(int index, int value) \
{   \
    sprites[index].fieldName = value;\
    return &sprites[index];\
} \
static inline int getSprite##fieldName(int index) \
{   \
    return getSprite(index)->fieldName; \
}
#else
static inline const spritetype* getSprite(int index)
{
    int idx = getRamSpriteIdx(index);
    if (!idx) /*was modified ? */
        return &sprites[index];
    return &ramsprites[idx - 1];
}
#define defineSpritesSettersAndGetters(fieldName) \
static inline const spritetype * setSprite##fieldName(int index, int value) \
{   \
    if (isLoadingLevel) \
    {                   \
        NOTIFY_LOAD_LEVEL(#fieldName, index, value);\
        ((spritetype*) &sprites[index])->fieldName = value;\
        return (spritetype*) &sprites[index];\
    }   \
    int idx = getRamSpriteIdx(index); \
    if (idx) /*was modified ? */ \
    { \
        spritetype *spr = &ramsprites[idx - 1]; \
        spr->fieldName = value; \
        return spr; \
    } \
    else    /*not yet modified*/ \
    { \
        if (sprites[index].fieldName != value) \
        { \
            spritetype *spr = allocateNewRamSprite(index);\
            NOTIFY_MOD_SPRITE(index, fieldName, value, sprites[index].fieldName, spr); \
            spr->fieldName = value;            \
            return spr; \
        } \
        else /*not modified. Do not allocate. Return pointer to flash*/ \
        { \
            return (spritetype*) &sprites[index]; \
        }\
    }\
} \
static inline int getSprite##fieldName(int index) \
{   \
    return getSprite(index)->fieldName; \
}
#endif


#define defineWallSetterAndGetters(fieldName) \
/*__attribute__ ((warn_unused_result))*/ static inline const walltype * setWall##fieldName(int index, int value) \
{   \
    int idx = getRamWallIdx(index); \
    if (idx) /*was modified ? */ \
    { \
        walltype *wal = &ramwalls[idx - 1]; \
        wal->fieldName = value; \
        return wal; \
    } \
    else    /*not yet modified*/ \
    { \
        if (walls[index].fieldName != value) \
        { \
            walltype *wal = allocateNewRamWall(index);\
            NOTIFY_MOD_WALL(index, fieldName, value); \
            wal->fieldName = value;            \
            return wal; \
        } \
        else /*not modified. Do not allocate. Return pointer to flash*/ \
        { \
            return (walltype*) &walls[index]; \
        }\
    }\
} \
static inline int getWall##fieldName(int index) \
{   \
    return getWall(index)->fieldName; \
}
#define defineWallSetterAndGettersXY(fieldName) \
/*__attribute__ ((warn_unused_result))*/ static inline const walltype * setWall_##fieldName(int index, int value) \
{   \
    int idx = getRamWallIdx(index); \
    if (idx) /*was modified ? */ \
    { \
        walltype *wal = &ramwalls[idx - 1]; \
        wal->fieldName = value; \
        return wal; \
    } \
    else    /*not yet modified*/ \
    { \
        if (walls[index].fieldName != value) \
        { \
            walltype *wal = allocateNewRamWall(index);\
            NOTIFY_MOD_WALL(index, fieldName, value); \
            wal->fieldName = value;            \
            return wal; \
        } \
        else /*not modified. Do not allocate. Return pointer to flash*/ \
        { \
            return (walltype*) &walls[index]; \
        }\
    }\
} \
static inline int getWall_##fieldName(int index) \
{   \
    return getWall(index)->fieldName; \
}
static inline const sectortype *getSector(int index)
{
    int idx = ramsectorIdx[(index)];
    if (!idx) /*was modified ? */
        return (sectortype*) &sectors[index];
    return &ramsectors[ramsectorIdx[index] - 1];
}
#if INTERPOLATION_USES_POSPTR
static inline int* getSector_ceilingzptr(int index)
{
    int idx = ramsectorIdx[index];
    if (!idx) /*was modified ? */
    {
        allocateNewRamSector(index);
        NOTIFY_MOD_SECT(index, ceilingz, -1);
    }
    return &ramsectors[ramsectorIdx[index] - 1].ceilingz;
}
static inline int* getSector_floorzptr(int index)
{
    int idx = ramsectorIdx[index];
    if (!idx) /*was modified ? */
    {
        allocateNewRamSector(index);
        NOTIFY_MOD_SECT(index, floorz, -1);
    }
    return &ramsectors[ramsectorIdx[index] - 1].floorz;
}
#else
static inline indexedValue_t getSector_ceilingzidx(int index)
{
    int idx = ramsectorIdx[index];
    if (!idx) /*was modified ? */
    {
        allocateNewRamSector(index);
        NOTIFY_MOD_SECT(index, ceilingz, -1);
    }
    indexedValue_t idxv;
    idxv.index = ramsectorIdx[index] - 1;
    idxv.type = INTERP_TYPE_SECTOR_CEILINGZ;
    return idxv;
}
static inline indexedValue_t getSector_floorzidx(int index)
{
    int idx = ramsectorIdx[index];
    if (!idx) /*was modified ? */
    {
        allocateNewRamSector(index);
        NOTIFY_MOD_SECT(index, floorz, -1);
    }
    indexedValue_t idxv;
    idxv.index = ramsectorIdx[index] - 1;
    idxv.type = INTERP_TYPE_SECTOR_FLOORZ;
    return idxv;
}
static inline indexedValue_t getSector_ceilingzidx_from_ptr(const sectortype *s)
{
//    printf("%s\r\n", __FUNCTION__);
    // already allocated ?
    if ((uint32_t) s >= (uint32_t)&ramsectors[0] && (uint32_t) s < (uint32_t) ramsectors + sizeof(ramsectors))
    {
        int idx = s - &ramsectors[0];
        //
//        printf("Ramsector, idx %d\r\n", idx);
        indexedValue_t idxv;
        idxv.index = idx;
        idxv.type = INTERP_TYPE_SECTOR_CEILINGZ;
        return idxv;
    }
    else    // not allocated.
    {
        int idx = s - &sectors[0];
#if WIN32 && 0
        if (idx < 0 || idx >= MAXSECTORS)
        {
            FIXME("Error, idx %d is out of range\r\n");
            EXIT(1);
        }
        int oldRamSector = ramsectorIdx[idx];
#endif
        indexedValue_t iv = getSector_ceilingzidx(idx);
#if WIN32 && 0
        printf(" Ramsector %d found for sector %d, old(raw, not with -1) ramsectoridx %d\r\n", iv.index, idx, oldRamSector);
#endif
        return iv;
    }
}
//
static inline indexedValue_t getSector_floorzidx_from_ptr(const sectortype *s)
{
    // already allocated ?
    if ((uint32_t) s >= (uint32_t)&ramsectors[0] && (uint32_t) s < (uint32_t) ramsectors + sizeof(ramsectors))
    {
        int idx = s - &ramsectors[0];
//        printf("Ramsector, idx %d\r\n", idx);
        //
        indexedValue_t idxv;
        idxv.index = idx;
        idxv.type = INTERP_TYPE_SECTOR_FLOORZ;
        return idxv;
    }
    else    // not allocated.
    {
        int idx = s - &sectors[0];
#if WIN32
        if (idx < 0 || idx >= MAXSECTORS)
        {
            FIXME("Error, idx %d is out of range\r\n", idx);
            EXIT(1);
        }
#endif
        indexedValue_t iv = getSector_floorzidx(idx);
 //       printf(" Ramsector allocated: %d\r\n", iv.index);
        return iv;
    }
}
#endif
#if WIN32
extern int profileGetWallPerFrame;
#endif
static inline walltype * getWall(int index)
{
    #if WIN32
        profileGetWallPerFrame++;
    #endif
    int idx = getRamWallIdx(index);
    if (!idx) /*was modified ? */
        return (walltype*) &walls[index];
    return &ramwalls[idx - 1];
}
#if INTERPOLATION_USES_POSPTR
static inline int* getWall_xptr(int index)
{
    int idx = ramwallIdx[(index)];
    if (!idx) /*was modified ? */
    {
        allocateNewRamWall(index);
        NOTIFY_MOD_WALL(index, xptr, -1);
    }
    return &ramwalls[ramwallIdx[index] - 1].x;
}
static inline int* getWall_yptr(int index)
{
    int idx = ramwallIdx[index];
    if (!idx) /*was modified ? */
    {
        allocateNewRamWall(index);
        NOTIFY_MOD_WALL(index, yptr, -1);
    }
    return &ramwalls[ramwallIdx[index] - 1].y;
}
#else

static inline indexedValue_t getWall_xidx(int index)
{
    int idx = getRamWallIdx(index);
    if (!idx) /*was modified ? */
    {
        allocateNewRamWall(index);
        NOTIFY_MOD_WALL(index, xptr, -1);
    }
    indexedValue_t idxv;
    idxv.index = getRamWallIdx(index) - 1;
    idxv.type = INTERP_TYPE_WALL_X;
    return idxv;
}
static inline indexedValue_t getWall_yidx(int index)
{
    int idx = getRamWallIdx(index);
    if (!idx) /*was modified ? */
    {
        allocateNewRamWall(index);
        NOTIFY_MOD_WALL(index, yptr, -1);
    }
    indexedValue_t idxv;
    idxv.index = getRamWallIdx(index) - 1;
    idxv.type = INTERP_TYPE_WALL_Y;
    return idxv;
}
#endif
defineWallSetterAndGetters(_xpanning);
defineWallSetterAndGetters(_ypanning);
defineWallSetterAndGetters(_pal);
defineWallSetterAndGetters(_lotag); // hitag is constant!
defineWallSetterAndGetters(_overpicnum);
#if 0
defineWallSetterAndGetters(_extra); // Custom for extra
#else
static inline int getWall_extra(int index)
{
    if (wall_extraIsMinusOne[index / 8] & (1 << (index % 8)))
        return -1;
    return getWall(index)->_extra;
}
/*__attribute__ ((warn_unused_result))*/ static inline const walltype * setWall_extra(int index, int value)
{
    int idx = getRamWallIdx(index);
    if (value == -1)
    {
        wall_extraIsMinusOne[index / 8] |=  (1 << (index % 8));
    }
    else
    {
        wall_extraIsMinusOne[index / 8] &= ~(1 << (index % 8));
    }
    if (idx) /*was modified ? */
    {
        walltype *wal = &ramwalls[idx - 1];
        wal->_extra = value;
        return wal;
    }
    else    /*not yet modified*/
    {
        if (walls[index]._extra != value && value != -1)
        {
            walltype *wal = allocateNewRamWall(index);
            NOTIFY_MOD_WALL(index, _extra, value);
            wal->_extra = value;
            return wal;
        }
        else /*not modified or value is -1. Do not allocate. Return pointer to flash*/ \
        {
            return &walls[index];
        }
    }
}
#endif
//defineWallSetterAndGetters(_shade);

// custom for shade
static inline void setWall_shade(int index, int value)
{
    wallshade[index] = value;
}
static inline int getWall_shade(int index)
{

    return wallshade[index];
}
//
defineWallSetterAndGetters(_cstat);
//defineWallSetterAndGetters(_point2);
static inline int getWall_point2(int idx)
{
    return constwall[idx]._point2;
}
defineWallSetterAndGetters(_picnum);
#if !HITAG_CONSTANT
defineWallSetterAndGetters(_hitag);
#else
extern uint8_t wall_hitag9999[(MAXWALLS  + 7 )/ 8 ];        // 1 if hitag is 9999
static inline void setWall_hitagTo9999(int index)
{
    wall_hitag9999[index / 8] = (1 << (index % 8));
}
static inline int getWall_hitag(int index)
{
    if (wall_hitag9999[index / 8] & (1 << (index % 8)))
        return 9999;
    else
        return constwall[index]._hitag;
}
#endif
#if !NO_RAMSECTORS
#define defineSectorSetter(fieldName) \
/*__attribute__ ((warn_unused_result)) */static inline sectortype* setSector_##fieldName(int index, int value) \
{   \
    int idx = ramsectorIdx[(index)]; \
    if (idx) /*was modified ? */ \
    { \
        sectortype *sec = &ramsectors[idx - 1]; \
        sec->fieldName = value; \
        return sec; \
    } \
    else    /*not yet modified*/ \
    { \
        if (sectors[index].fieldName != value) \
        { \
            sectortype *sec = allocateNewRamSector(index);\
            NOTIFY_MOD_SECT(index, fieldName, value); \
            sec->fieldName = value;            \
            return sec; \
        } \
        else /*not modified. Do not allocate. Return pointer to flash*/ \
        { \
            return (sectortype*)&sectors[index]; \
        }\
    }\
}
#else
#define defineSectorSetter(fieldName) \
/*__attribute__ ((warn_unused_result)) */static inline sectortype* setSector_##fieldName(int index, int value) \
{   \
    ((sectortype*)&sectors[index])->fieldName = value; \
    return ((sectortype*)&sectors[index]);\
}
#endif
defineWallSetterAndGettersXY(x);
defineWallSetterAndGettersXY(y);
// sprites getters and setters
defineSpritesSettersAndGetters(_z);
defineSpritesSettersAndGetters(_sectnum);
defineSpritesSettersAndGetters(_owner);
defineSpritesSettersAndGetters(_xvel);
defineSpritesSettersAndGetters(_shade);
defineSpritesSettersAndGetters(_lotag);
defineSpritesSettersAndGetters(_hitag);
defineSpritesSettersAndGetters(_y);
defineSpritesSettersAndGetters(_statnum);
defineSpritesSettersAndGetters(_yrepeat);
defineSpritesSettersAndGetters(_pal);
defineSpritesSettersAndGetters(_ang);
defineSpritesSettersAndGetters(_zvel);
defineSpritesSettersAndGetters(_extra);
defineSpritesSettersAndGetters(_yoffset);
defineSpritesSettersAndGetters(_xoffset);
defineSpritesSettersAndGetters(_x);
defineSpritesSettersAndGetters(_picnum);
defineSpritesSettersAndGetters(_cstat);
defineSpritesSettersAndGetters(_yvel);
defineSpritesSettersAndGetters(_xrepeat);
//
defineSectorSetter(ceilingz);
defineSectorSetter(floorz);
#if SEPARATE_SECTOR_CEILINGSHADE
EXTERN int8_t ramSectorCeilingShade[MAXSECTORS];
static inline void setSector_ceilingshade(int index, int value)
{
    //sectortype* s = &sectors[index];
    ramSectorCeilingShade[index] = value;
    //return &sectors[index];
}
static inline int getSector_ceilingshade(int index)
{
    return ramSectorCeilingShade[index];
}
#else
defineSectorSetter(ceilingshade);
#endif
defineSectorSetter(ceilingxpanning);
defineSectorSetter(ceilingypanning);
#if SEPARATE_SECTOR_FLOORSHADE
EXTERN int8_t ramSectorFloorShade[MAXSECTORS];
static inline void setSector_floorshade(int index, int value)
{
    ramSectorFloorShade[index]= value;
}
static inline int getSector_floorshade(int index)
{
    return ramSectorFloorShade[index];
}
#else
defineSectorSetter(floorshade);
#endif
defineSectorSetter(floorheinum);
#if SEPARATE_SECTOR_FLOORXPANNING
EXTERN uint8_t ramSectorFloorxpanning[MAXSECTORS];
static inline void setSector_floorxpanning(int index, int value)
{
    ramSectorFloorxpanning[index] = value;
}
static inline int getSector_floorxpanning(int index)
{
    return ramSectorFloorxpanning[index];
}
#else
defineSectorSetter(floorxpanning);
#endif
defineSectorSetter(floorypanning);
defineSectorSetter(floorstat);
defineSectorSetter(ceilingpicnum);
defineSectorSetter(ceilingpal);
defineSectorSetter(floorpal);
defineSectorSetter(floorpicnum);
defineSectorSetter(lotag);
defineSectorSetter(hitag);

#if SEPARATE_SECTOR_EXTRA
EXTERN int16_t ramSectorExtra[MAXSECTORS];
static inline void setSector_extra(int index, int value)
{
    ramSectorExtra[index] = value;
}
static inline int getSector_extra(int index)
{
    return ramSectorExtra[index];
}
#else
defineSectorSetter(extra);
#endif
defineSectorSetter(ceilingstat);
// pvWalls
enum vector_index_e {VEC_X=0,VEC_Y=1};
enum screenSpaceCoo_index_e {VEC_COL=0,VEC_DIST=1};
typedef int32_t vector_t[2];
#define REDUCE_PVWALLS 1
//typedef int32_t coo2D_t[2];
#if REDUCE_PVWALLS
#define N_BITS_VEC_COL    9
#define N_BITS_VEC_DIST (32 - N_BITS_VEC_COL)
typedef union
{
    struct
    {
        uint32_t vec_col: N_BITS_VEC_COL;    // 320, MUST BE SIGNED FOR MAPS
        int32_t vec_dist_ : N_BITS_VEC_DIST;
    };
    struct
    {
        int32_t vec_colmap: N_BITS_VEC_COL + 1;    // 320, MUST BE SIGNED FOR MAPS
        int32_t vec_distmap : N_BITS_VEC_DIST - 1;
    };

} coo2D_t;
#if (WIN32 || ESP_PLATFORM) && REDUCE_PVWALLS
static inline int SSAT_N_BITS_VEC_DIST(int x)
{
    if (x > ((1 << (N_BITS_VEC_DIST - 1)) - 1))
    {
//        printf("Saturating %x to %x\r\n", x, (1 << (nbits - 1)) - 1);
        return (1 << (N_BITS_VEC_DIST - 1)) - 1;
    }
    else if (x < - (1 << (N_BITS_VEC_DIST - 1)))
    {
  //      printf("Saturating %x to %x\r\n", x, -(1 << (nbits - 1)) );
        return - (1 << (N_BITS_VEC_DIST - 1));
    }
    return x;
}
#else
static inline int SSAT_N_BITS_VEC_DIST(int x)
{
#if REDUCE_PVWALLS
    // TODO: CHECK
    __asm__ volatile ("ssat %0, %1, %2" : "=r" (x) :  "I" (N_BITS_VEC_DIST), "r" (x) : "cc" );
#endif
    return x;
}
#endif // WIN32

static inline int getVecDist(coo2D_t *c)
{
    return c->vec_dist_ << 1;
}
static inline void setVecDist(coo2D_t *c, int value)
{
    c->vec_dist_ = SSAT_N_BITS_VEC_DIST(value >> 1);
}

#else
typedef union
{
    struct
    {
        int32_t vec_col;    // 320, MUST BE SIGNED FOR MAPS
        int32_t vec_dist;
    };
    struct
    {
        int32_t vec_colmap;    // 320, MUST BE SIGNED FOR MAPS
        int32_t vec_distmap;
    };

} coo2D_t;
#endif
// This is the structure emitted for each wall that is potentially visible.
// A stack of those is populated when the sectors are scanned.
typedef struct pvWall_s{
    vector_t cameraSpaceCoo[2]; //Camera space coordinates of the wall endpoints. Access with vector_index_e.
   // int16_t sectorId;        //The index of the sector this wall belongs to in the map database.
    // note next-hack: if you know the worldWallId you can use a LUT to get the sector ID. Also worldWallId is limited to less than 8192.
#if PVWALL_HAS_WORLDWALLID
int16_t worldWallId;     //The index of the wall in the map database.
#endif
    coo2D_t screenSpaceCoo[2]; //Screen space coordinate of the wall endpoints. Access with screenSpaceCoo_index_e.
} pvWall_t;
#if WIN32
EXTERN sectortype initial_sectors[MAXSECTORS];
EXTERN walltype initial_walls[MAXWALLS];
EXTERN spritetype initial_sprites[MAXSPRITES];
#endif
void clearBufferedTileImagePtr(void);
void setTempRamTileData(uint8_t *buffer);
void setHorizycent(int32_t v);
void setStackPvWalls(uint8_t *ptr);
void rotateRAMsprite(int32_t sx, int32_t sy, int32_t z, short a, uint8_t *buffer,
                  int8_t dashade, uint8_t  dapalnum, uint8_t  dastat,
                  int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2);
#endif  /* defined _INCLUDE_BUILD_H_ */

/* end of build.h ... */



