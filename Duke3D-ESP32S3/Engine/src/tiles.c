//
//  tiles.c
//  Duke3D
//
//  Created by fabien sanglard on 12-12-22.
//  Copyright (c) 2012 fabien sanglard. All rights reserved.
//
#include "maincfg.h"
#include "internalFlash.h"
#include "duke3d.h"
#include <stdlib.h>
#include <string.h>

#include "tiles.h"
#include "engine.h"
#include "draw.h"
#include "filesystem.h"
#include "names.h"
#if !WIN32
#include "printf.h"
#endif
#if MG24
#include "graphics.h"
#endif
char  artfilename[20];
#if USE_EXT_MEM
const tile_t *tiles;
const uint8_t **extMemTileData;
const uint8_t **levelTileData;
const uint8_t **tileData;
#else
tile_t tiles[MAXTILES];

#endif // USE_EXT_MEM


int32_t artversion;

uint8_t  *pic = NULL;
uint8_t gotMirrorPic = 0;
void setviewtotile(short tilenume, int32_t tileWidth, int32_t tileHeight)
{
  //  FIXME("SetViewToTile %d, w %d h %d\r\n", tilenume, tileWidth, tileHeight);

    /* DRAWROOMS TO TILE BACKUP&SET CODE */
#if !USE_EXT_MEM
    tiles[tilenume].dim.width = tileWidth;
    tiles[tilenume].dim.height = tileHeight;
#endif
    bakxsiz[setviewcnt] = tileWidth;
    bakysiz[setviewcnt] = tileHeight;
    bakvidoption[setviewcnt] = vidoption;
    vidoption = 2;
    bakframeplace[setviewcnt] = frameplace;
    if (tilenume == VIEWSCREEN || tilenume == VIEWSCREEN2)
    {
        frameplace = get_framebuffer() + SCREENWIDTH * (SCREENHEIGHT - STATUS_BAR_HEIGHT);
    }
    else
    {
        frameplace = (uint8_t*) tileData[tilenume];
    }

    bakwindowx1[setviewcnt] = windowx1;
    bakwindowy1[setviewcnt] = windowy1;
    bakwindowx2[setviewcnt] = windowx2;
    bakwindowy2[setviewcnt] = windowy2;
#if OLD
    copybufbyte(&startumost[windowx1],&bakumost[windowx1],(windowx2-windowx1+1)*sizeof(bakumost[0]));
    copybufbyte(&startdmost[windowx1],&bakdmost[windowx1],(windowx2-windowx1+1)*sizeof(bakdmost[0]));
#elif IS_THIS_NECESSARY
    for (int i = 0; i < (windowx2-windowx1+1); i++)
    {
        bakumost[windowx1 + i] = startumost(windowx1 + i);
        bakdmost[windowx1 + i] = startdmost(windowx1 + i);
    }
#endif
    setview(0,0,tileHeight-1,tileWidth-1);
    setaspect(65536,65536);

#if YLOOKUP_ARRAY
    int j = 0;
    for(int i=0; i<=tileWidth; i++) {
        ylookup[i] = j;
        j += tileHeight;
    }
#else
    setYlookup(tileHeight);
#endif
    setBytesPerLine(tileHeight);
    setviewcnt++;
}




void squarerotatetile(short tilenume)
{
    int32_t i, j, k;
    uint8_t  *ptr1, *ptr2;

    dimensions_t tileDim;

    tileDim.width = tiles[tilenume].dim.width;
    tileDim.height = tiles[tilenume].dim.height;

    /* supports square tiles only for rotation part */
    uint8_t *tileBuffer;
    if (useRenderedCamData() && (tilenume == VIEWSCREEN || tilenume == VIEWSCREEN2))
    {
        tileBuffer = (uint8_t*) get_framebuffer() + SCREENWIDTH * (SCREENHEIGHT - STATUS_BAR_HEIGHT);
    }
    else
    {
       //   tileBuffer = tileData[tilenume];
       return;
    }
    if (tileDim.width == tileDim.height)
    {
        k = (tileDim.width<<1);
        for(i=tileDim.width-1; i>=0; i--)
        {
            ptr1 = tileBuffer +i*(tileDim.width+1);
            ptr2 = ptr1;
            if ((i&1) != 0) {
                ptr1--;
                ptr2 -= tileDim.width;
                swapchar(ptr1,ptr2);
            }
            for(j=(i>>1)-1; j>=0; j--)
            {
                ptr1 -= 2;
                ptr2 -= k;
                swapchar2(ptr1,ptr2,tileDim.width);
            }
        }
    }
}



//1. Lock a picture in the cache system.
//2. Mark it as used in the bitvector tracker.
void setgotpic(int32_t tilenume)
{
#if USE_CACHE
    if (tiles[tilenume].lock < 200)
        tiles[tilenume].lock = 199;
#endif // USE_CACHE
//    gotpic[tilenume>>3] |= pow2char[tilenume&7];
    if (tilenume == MIRROR)
    {
        gotMirrorPic = 1;   //next-hack: this is the only pic we actually check
    }
}




#if !USE_EXT_MEM
void loadtile(short tilenume)
{
    uint8_t  *ptr;
    int32_t i, tileFilesize;

    if ((uint32_t)tilenume >= (uint32_t)MAXTILES)
        return;

    tileFilesize = tiles[tilenume].dim.width * tiles[tilenume].dim.height;

    if (tileFilesize <= 0)
        return;

    i = tilefilenum[tilenume];
    if (i != artfilnum){
        if (artfil != -1)
            kclose(artfil);
        artfilnum = i;
        artfilplc = 0L;

        artfilename[7] = (i%10)+48;
        artfilename[6] = ((i/10)%10)+48;
        artfilename[5] = ((i/100)%10)+48;
        artfil = TCkopen4load(artfilename,LOAD_ONLY_GRP);
        if (artfil == -1){
            printf("Error, unable to load artfile:'%s'.\n",artfilename);
            getchar();
            EXIT(0);
        }
#if FREQUENT_FAKETIMERHANDLER
        faketimerhandler();
#endif
    }

    if (tiles[tilenume].data == NULL){
        tiles[tilenume].lock = 199;
/*code not used */        allocache(&tiles[tilenume].data,tileFilesize,(uint8_t  *) &tiles[tilenume].lock);
    }

    if (artfilplc != tilefileoffs[tilenume])
    {
        klseek(artfil,tilefileoffs[tilenume]-artfilplc,SEEK_CUR);
      #if FREQUENT_FAKETIMERHANDLER
        faketimerhandler();
    #endif
    }
    ptr = tiles[tilenume].data;

    kread(artfil,ptr,tileFilesize);
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
    artfilplc = tilefileoffs[tilenume]+tileFilesize;
}
#endif
#if CODE_NOT_USED
uint8_t* allocatepermanenttile(short tilenume, int32_t width, int32_t height)
{
    int32_t j;
    uint32_t tileDataSize;

    //Check dimensions are correct.
    if ((width <= 0) || (height <= 0) || ((uint32_t)tilenume >= (uint32_t)MAXTILES))
        return(0);

    tileDataSize = width * height;

    tiles[tilenume].lock = 255;
/*code was not used*/    allocache(&tiles[tilenume].data,tileDataSize,(uint8_t  *) &tiles[tilenume].lock);

    tiles[tilenume].dim.width = width;
    tiles[tilenume].dim.height = height;
    tiles[tilenume].animFlags = 0;

    j = 15;
    while ((j > 1) && (pow2long[j] > width))
        j--;
    picsiz[tilenume] = ((uint8_t )j);

    j = 15;
    while ((j > 1) && (pow2long[j] > height))
        j--;
    picsiz[tilenume] += ((uint8_t )(j<<4));

    return(tiles[tilenume].data);
}
#endif


int loadpics(char  *filename, char * gamedir)
{
    UNUSED(filename);
    UNUSED(gamedir);
   int32_t offscount, localtilestart, localtileend, dasiz;
    short i, j, k;


    strcpy(artfilename,filename);


    tile_t *tempTiles = getDisplayFrameBuffer(0);    // let's use the frame buffer
    uint8_t **tempTileData = (uint8_t **) (tempTiles + MAXTILES);
    memset(tempTileData, 0, MAXTILES * sizeof(*tempTileData));
    //
    for(i=0; i<MAXTILES; i++)
    {
        tempTiles[i].dim.width = 0;
        tempTiles[i].dim.height = 0;
        tempTiles[i].animFlags = 0L;
    }

#if WIN32
    static uint8_t tempFrame[320*320 ];
    tempTileData[MAXTILES - 2] = tempFrame;
#else
    tempTileData[MAXTILES - 2] = getDisplayFrameBuffer(0);

    #endif
    tempTiles[MAXTILES - 2].animFlags = 0;
#if BIG_TILT_TILE
    tempTiles[MAXTILES - 2].dim.height = 320;
    tempTiles[MAXTILES - 2].dim.width = 320;
#else
    tempTiles[MAXTILES - 2].dim.height = SCREEN_TILT_TILE_SIZE;
    tempTiles[MAXTILES - 2].dim.width = SCREEN_TILT_TILE_SIZE;

#endif
    artsize = 0L;

    numtilefiles = 0;
    int32_t maxTileIndex = 0;
    //
#if !WIN32
    SET_FLASH_MODE();
#endif
    //
    do
    {
        k = numtilefiles;

        artfilename[7] = (k%10)+48;
        artfilename[6] = ((k/10)%10)+48;
        artfilename[5] = ((k/100)%10)+48;

        uint8_t *p;

        if ((p = kGetExtMemAddress(artfilename, NULL)))
        {
          printf("artifilename is %s, p is %p\r\n", artfilename, p);
            extMemSetCurrentAddress((uint32_t) p);
            extMemGetDataFromCurrentAddress(&artversion, 4);

            if (artversion != 1)
            {
                FIXME("ERROR artversion is %d", artversion);
                return(-1);
            }
            int32_t numTiles;
            extMemGetDataFromCurrentAddress(&numTiles, 4);
            extMemGetDataFromCurrentAddress(&localtilestart, 4);
            extMemGetDataFromCurrentAddress(&localtileend, 4);


            if (localtileend > maxTileIndex)
            {
                maxTileIndex = localtileend;
            }
            if (localtileend >= MAXTILES)
            {
                printf("Error, too many tiles %d, defined %d", localtileend, MAXTILES);
                system("pause");
            }

            for (i = localtilestart; i <= localtileend; i++)
                extMemGetDataFromCurrentAddress(&tempTiles[i].dim.width, 2);

            for (i = localtilestart; i <= localtileend; i++)
                extMemGetDataFromCurrentAddress(&tempTiles[i].dim.height, 2);

            for (i = localtilestart; i <= localtileend; i++)
                extMemGetDataFromCurrentAddress(&tempTiles[i].animFlags, 4);


            offscount = 4+4+4+4+((localtileend-localtilestart+1)<<3);

            for(i=localtilestart; i<=localtileend; i++)
            {

                //tiles[i].data = p + offscount;
                tempTileData[i] = p +offscount;
                dasiz = tempTiles[i].dim.width*tempTiles[i].dim.height;
         //       printf("pic %d siz %d %d %d total %d. Data %p. Offs %d\r\n", i, tempTiles[i].dim.width, tempTiles[i].dim.height, dasiz, artsize, tempTileData[i], offscount );
#if WIN32
            static int maxSize;
            if (dasiz > maxSize)
                maxSize = dasiz;
    //printf("MaxSize: %d\r\n", maxSize);
#endif
                offscount += dasiz;
                artsize += ((dasiz+15)&0xfffffff0);
            }
            tempTiles[MIRROR].dim.width = tempTiles[MIRROR].dim.height = 0;
//            kclose(fil);

            numtilefiles++;

        }
    }
    while (k != numtilefiles);
    tiles = storeToInternalFlash(tempTiles, MAXTILES * sizeof (*tempTiles));
    //

    //
    extMemTileData = storeToInternalFlash (tempTileData, MAXTILES * sizeof(*tempTileData)); // note: *tempTileData is a pointer, so 4-bytes aligned
    tileData = extMemTileData;
    // compare tiles to what is found in extMemoryBackup.
    nvmData_t   *nvm;
#if WIN32
    nvm = &ext_memory[EXT_MEMORY_SIZE - sizeof(nvmData_t)];
#else
   nvm = (nvmData_t*) ((byte*) extMemGetSize() - sizeof(nvmData_t));
#endif // WIN32
    //
    // let reuse tempTileData (which is in the video buffer)
    extMemGetDataFromAddress(tempTileData, nvm->extMemTileDataPtrBackup, MAXTILES * sizeof(*tempTileData));
    // now let's compare it with the internal flash
    if (memcmp (tempTileData, tileData, MAXTILES * sizeof(*tempTileData)))
    {
        printf("storing tile ptr extmem data to external flash\r\n");
#if WIN32
        extMemProgram((uint32_t) nvm->extMemTileDataPtrBackup - (uint32_t) ext_memory, tileData, MAXTILES * sizeof(*tempTileData));
#else
        extMemErase((uint32_t) nvm->extMemTileDataPtrBackup, sizeof(nvm->extMemTileDataPtrBackup));
        extMemProgram((uint32_t) nvm->extMemTileDataPtrBackup, (uint8_t*) tileData, MAXTILES * sizeof(*tempTileData));
#endif // WIN32
    }
    printf("Art files loaded: num tiles: %d. Artsize: %d\n", maxTileIndex + 1, artsize);
   // exit(1);
//    clearbuf(gotpic,(MAXTILES+31)>>5,0L);
    gotMirrorPic = 0;   //next-hack: this is the only pic we actually check

    //
    uint8_t  stack_picsiz[(MAXTILES + 3) & ~(3)] = {0};
    for(i=0; i<MAXTILES - 3; i++)
    {
        j = 15;
        while ((j > 1) && (pow2long[j] > tiles[i].dim.width))
            j--;

        stack_picsiz[i] = ((uint8_t )j);
        j = 15;

        while ((j > 1) && (pow2long[j] > tiles[i].dim.height))
            j--;

        stack_picsiz[i] += ((uint8_t )(j<<4));
    }
    picsiz =  storeToInternalFlash(stack_picsiz, sizeof(stack_picsiz));
    artfil = -1;
    artfilnum = -1;
    artfilplc = 0L;


    return(0);
}

#if WIN32
void TILE_MakeAvailable2(short picID, char *file, int line)
{
    if (tileData[picID] == NULL)
    {
#if !USE_EXT_MEM
        loadtile(picID);
#endif
        FIXME("Error, Pic %d was not cached at file %s, line %d\r\n", picID, file, line);
    }
}
#else
void TILE_MakeAvailable(short picID)
{
    UNUSED(picID);
#if WIN32
    if (tileData[picID] == NULL)
    {

     //   loadtile(picID);
        FIXME("Error, Pic %d was not cached\r\n");
    }
#endif
}
#endif

void copytilepiece(int32_t tilenume1, int32_t sx1, int32_t sy1, int32_t xsiz, int32_t ysiz,
                   int32_t tilenume2, int32_t sx2, int32_t sy2)
{
    const uint8_t  *ptr1;
    uint8_t *ptr2;
    uint8_t dat;
    int32_t xsiz1, ysiz1, xsiz2, ysiz2, i, j, x1, y1, x2, y2;

    xsiz1 = tiles[tilenume1].dim.width;
    ysiz1 = tiles[tilenume1].dim.height;

    xsiz2 = tiles[tilenume2].dim.width;
    ysiz2 = tiles[tilenume2].dim.height;


    if ((xsiz1 > 0) && (ysiz1 > 0) && (xsiz2 > 0) && (ysiz2 > 0))
    {
        TILE_MakeAvailable(tilenume1);
        TILE_MakeAvailable(tilenume2);

        x1 = sx1;
        for(i=0; i<xsiz; i++)
        {
            y1 = sy1;
            for(j=0; j<ysiz; j++)
            {
                x2 = sx2+i;
                y2 = sy2+j;
                if ((x2 >= 0) && (y2 >= 0) && (x2 < xsiz2) && (y2 < ysiz2))
                {
                    ptr1 = tileData[tilenume1] + x1*ysiz1 + y1;
                    ptr2 = (uint8_t *) tileData[tilenume2] + x2*ysiz2 + y2;
                    dat = *ptr1;


                    if (dat != 255)
                        *ptr2 = *ptr1;
                }

                y1++;
                if (y1 >= ysiz1) y1 = 0;
            }
            x1++;
            if (x1 >= xsiz1) x1 = 0;
        }
    }
}



/*
 FCS:   If a texture is animated, this will return the offset to add to tilenum
 in order to retrieve the texture to display.
 */
int animateoffs(int16_t tilenum)
{
    int32_t i, k, offs;

    offs = 0;

    i = (totalclocklock>>((tiles[tilenum].animFlags>>24)&15));

    if ((tiles[tilenum].animFlags&63) > 0){
        switch(tiles[tilenum].animFlags&192)
        {
            case 64:
                k = (i%((tiles[tilenum].animFlags&63)<<1));
                if (k < (tiles[tilenum].animFlags&63))
                    offs = k;
                else
                    offs = (((tiles[tilenum].animFlags&63)<<1)-k);
                break;
            case 128:
                offs = (i%((tiles[tilenum].animFlags&63)+1));
                break;
            case 192:
                offs = -(i%((tiles[tilenum].animFlags&63)+1));
        }
    }

    return(offs);
}
// next-hack: added to get the maximum offset of a tile. Used for caching flats.
void getMaxTileAnimOffset(int16_t tilenum, int *start, int *stop)
{
    int32_t offs;

    offs = 0;

    if ((tiles[tilenum].animFlags&63) > 0){
        switch(tiles[tilenum].animFlags&192)
        {
            case 64: // back-forth
            case 128:
                offs = (tiles[tilenum].animFlags&63);
            case 192:
                *start -= (tiles[tilenum].animFlags&63);
        }
    }
    *start = tilenum;
    *stop = tilenum + offs;
}
