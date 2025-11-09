//
//  tiles.h
//  Duke3D
//
//  Created by fabien sanglard on 12-12-22.
//  Copyright (c) 2012 fabien sanglard. All rights reserved.
//

#ifndef Duke3D_tiles_h
#define Duke3D_tiles_h

#include "build.h"



/*
// The dimension of the tile in texels unit. The sizes can be obtained for
// any tile by doing a tilesizx * tilesizy
//EXTERN short tilesizx[MAXTILES], tilesizy[MAXTILES];
typedef struct dimensions_s{
    short width;
    short height;
} dimensions_t;
dimensions_t tiles[MAXTILES];

// An array of locks for each pic: Used to check if a texture is in RAM or in the GRP.
EXTERN uint8_t  walock[MAXTILES];
EXTERN int32_t numtiles, picanm[MAXTILES];

//The wall texture data.
EXTERN uint8_t* waloff[MAXTILES];
*/




typedef struct dimensions_s{
    short width;
    short height;
} dimensions_t;

typedef struct tile_s{
    dimensions_t dim;
#if USE_CACHE
    uint8_t lock;
#endif
    int32_t animFlags;
    //uint8_t* data;
} tile_t;

#if USE_EXT_MEM
extern const tile_t *tiles;   // tile dimensions and animflags
extern const uint8_t **extMemTileData; // pointers to tile data, all of them is in external flash
extern const uint8_t **levelTileData;  // pointer to level tile data, some of them in internal flash for speedup
extern const uint8_t **tileData;              // current pointer to tile data. Might be extMemTileData or levelTileData
#else
extern tile_t tiles[MAXTILES];
#endif // USE_EXT_MEM
void setviewtotile(short tilenume, int32_t tileWidth, int32_t tileHeight);
void squarerotatetile(short tilenume);

void loadtile(short tilenume);
uint8_t* allocatepermanenttile(short tilenume, int32_t width, int32_t height);
int loadpics(char  *filename, char * gamedir);
void copytilepiece(int32_t tilenume1, int32_t sx1, int32_t sy1, int32_t xsiz, int32_t ysiz,int32_t tilenume2, int32_t sx2, int32_t sy2);


//Bitvector marking picture used for rendition.
//extern uint8_t  gotpic[(MAXTILES+7)>>3];
extern uint8_t gotMirrorPic;
void setgotpic(int32_t tilenume);


int animateoffs(int16_t tilenum);

extern uint8_t  *pic ;
#if WIN32
#define TILE_MakeAvailable(id) TILE_MakeAvailable2 (id, __FILE__, __LINE__)
void TILE_MakeAvailable2(short picID, char *file, int line);

#else
void TILE_MakeAvailable(short picID);
#endif
void getMaxTileAnimOffset(int16_t tilenum, int *start, int *stop);

#endif
