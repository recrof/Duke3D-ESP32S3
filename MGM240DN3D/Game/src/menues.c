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
	#include <fcntl.h>
#endif
#ifdef WIN32
#include <SDL2/SDL.h>
#endif
#include "duke3d.h"
#include "audiolib/fx_man.h"
#include "audiolib/music.h"
#include "animlib.h"
#include "config.h"
#include "control.h"
#include "display.h"
#include "engine.h"
#include "filesystem.h"
#include "function.h"
#include "mouse.h"
#include "premap.h"
#include "sounds.h"
#include "soundefs.h"
#include "graphics.h"
#include <ctype.h>
#ifdef WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
#if MG24
#include "radio.h"
#endif
#ifdef WIN32
extern SDL_Surface *surface;
#endif
void CopyPreviewScreenshot(uint8_t *dst, uint8_t *src);
extern short inputloc;
extern int recfilep;
extern uint8_t  vgacompatible;
short probey=0,lastprobey=0,last_menu;
#if 0
short globalskillsound=-1;
#else
short globalSkillVoice = -1;
#endif
short sh,onbar,buttonstat,deletespot;
short last_zero,last_fifty,last_threehundred = 0;
extern uint8_t  multiplayerConnection;
//
int8_t capturingForSaveGame = 0;
uint8_t * savegamePreviewPtr = NULL;
uint8_t gameLoaded = 0;
// File tree info
//
//uint8_t  szCurrentDirectory[1024] = {'\0'};

#define FILETYPE_DIRECTORY 0
#define FILETYPE_FILE      1

typedef struct fileentry
{
	uint8_t  filename[16];
	int type;
}FILEENTRY;

typedef struct filelist
{
	FILEENTRY *files;
}FILELIST;

FILELIST m_Files;// = NULL;

//
//

// CTW - REMOVED
/* Error codes */
/*
#define eTenBnNotInWindows 3801
#define eTenBnBadGameIni 3802
#define eTenBnBadTenIni 3803
#define eTenBnBrowseCancel 3804
#define eTenBnBadTenInst 3805

int  tenBnStart(void);
void tenBnSetBrowseRtn(uint8_t  *(*rtn)(uint8_t  *str, int len));
void tenBnSetExitRtn(void (*rtn)(void));
void tenBnSetEndRtn(void (*rtn)(void));*/
// CTW END - REMOVED

#define DEBUG_SAVE 0
#if DEBUG_SAVE
#define SAVE_TO_EXTMEM(idx, property)     do { \
    extMemProgram((uint32_t) &nvm->savegames[idx].s.property, (uint8_t*) &property, (3 + sizeof(nvm->savegames[savegameNumber].s.property)) & ~3);\
    dumpToFile("save\\"#property".txt", &property, sizeof(property)); \
} while(0)
#else
#define SAVE_TO_EXTMEM(idx, property)     extMemProgram((uint32_t) &nvm->savegames[idx].s.property, (uint8_t*) &property, (3 + sizeof(nvm->savegames[savegameNumber].s.property)) & ~3)    //
#endif
#define DEBUG_LOAD 0
#if DEBUG_LOAD
#define LOAD_FROM_EXTMEM(idx, property)     do {  \
        extMemGetDataFromAddress(&property, &nvm->savegames[savegameNumber].s.property , sizeof(property)); \
        printf(">>>>>> size of %s is %d\r\n", #property, sizeof(nvm->savegames[savegameNumber].s.property)); \
        char dummy[sizeof(property) == sizeof(nvm->savegames[savegameNumber].s.property) ? 0 : -1 ];\
         dumpToFile("load\\"#property".txt", &property, sizeof(property)); \
        } while(0)    //
#else
#define LOAD_FROM_EXTMEM(idx, property)     do {extMemGetDataFromAddress((void*) &property, &nvm->savegames[savegameNumber].s.property , sizeof(property)); \
                                            } while(0)    //
#endif // DEBUG_LOAD
#if WIN32
void dummyfunc(void)
{
}

void dummymess(int i,uint8_t  *c)
{
}
#endif
// CTW - REMOVED
/*
void TENtext(void)
{
    int32_t dacount,dalastcount;

    puts("\nDuke Nukem 3D has been licensed exclusively to TEN (Total");
    puts("Entertainment Network) for wide-area networked (WAN) multiplayer");
    puts("games.\n");

    puts("The multiplayer code within Duke Nukem 3D has been highly");
    puts("customized to run best on TEN, where you'll experience fast and");
    puts("stable performance, plus other special benefits.\n");

    puts("We do not authorize or recommend the use of Duke Nukem 3D with");
    puts("gaming services other than TEN.\n");

    puts("Duke Nukem 3D is protected by United States copyright law and");
    puts("international treaty.\n");

    puts("For the best online multiplayer gaming experience, please call TEN");
    puts("at 800-8040-TEN, or visit TEN's Web Site at www.ten.net.\n");

    puts("Press any key to continue.\n");

    _bios_timeofday(0,&dacount);

    while( _bios_keybrd(1) == 0 )
    {
        _bios_timeofday(0,&dalastcount);
        if( (dacount+240) < dalastcount ) break;
    }
}
*/
// CTW END - REMOVED

void cmenu(short cm)
{
    current_menu = cm;

    if( (cm >= MENU_LOAD_GAME0 && cm <= MENU_CANNOT_LOAD_GAME_MESSAGE) )
        return;

    if( cm == 0 )
        probey = last_zero;
    else if(cm == MENU_INGAME_MENU)
        probey = last_fifty;
    else if(cm >= MENU_LOAD_GAME && cm < MENU_HELP0)
        probey = last_threehundred;
    else if(cm == MENU_SKILL_SELECT)
        probey = 1;
    else probey = 0;
    lastprobey = -1;
}

#ifdef WIN32

void savetemp(char  *fn,uint8_t* daptr,int32_t dasiz)
{
    int fp;

    fp = open(fn,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

    write(fp,(uint8_t  *)daptr,dasiz);

    close(fp);

}
#endif

void getangplayers(short snum)
{
    short i,a;

    for(i=connecthead;i>=0;i=connectpoint2[i])
    {
        if(i != snum)
        {
            a = ps[snum].ang+getangle(ps[i].posx-ps[snum].posx,ps[i].posy-ps[snum].posy);
            a = a-1024;
            rotatesprite(
                (320<<15) + (((sintable[(a+512)&2047])>>7)<<15),
                (320<<15) - (((sintable[a&2047])>>8)<<15),
                klabs(sintable[((a>>1)+768)&2047]<<2),0,APLAYER,0,ps[i].palookup,0,0,0,xdim-1,ydim-1);
        }
    }
}
#define PREVIEW_POSY    55
#define PREVIEW_POSX    20
void drawSavegameScreenshot(void)
{
    if (savegamePreviewPtr != NULL)
    {
        for (int y = 0; y < 120; y++)
        {
            int yy = y * (65536 * 100 / 120) >> 16;
            extMemGetDataFromAddress( frameplace + (y + PREVIEW_POSY) * SCREEN_WIDTH + PREVIEW_POSX, savegamePreviewPtr + yy * SCREEN_WIDTH / 2, SCREEN_WIDTH / 2);
        }
    }
}
void CopyPreviewScreenshot(uint8_t *dst, uint8_t *src)
{
    // we can use the same display position instead of stack
    uint8_t *lineptr = frameplace + PREVIEW_POSY * SCREEN_WIDTH + PREVIEW_POSX;
    for (int y = 0; y < 100; y++)
    {
        extMemGetDataFromAddress( lineptr, src + y * (SCREEN_WIDTH / 2) , (SCREEN_WIDTH / 2));
        extMemProgram((uint32_t)(dst + y * (SCREEN_WIDTH / 2)), lineptr, SCREEN_WIDTH / 2);
    }
}
void savePreviewScreenshot(uint8_t *ptr)
{
            // we want to save a 160 x 100 image.
        for (int y = 0; y < PREVIEW_SCREENSHOT_HEIGHT; y++)
        {
            for (int x = 0; x < PREVIEW_SCREENSHOT_WIDTH; x++)
            {
                frameplace[ x + y * PREVIEW_SCREENSHOT_WIDTH] = frameplace[x * 2 + y * 2 * SCREEN_WIDTH];
            }
        }
        extMemProgram((uint32_t) ptr, (uint8_t*) frameplace, PREVIEW_SCREENSHOT_WIDTH * PREVIEW_SCREENSHOT_HEIGHT);
#if WIN32
        savegamePreviewPtr = &ext_memory[(uint32_t) ptr];
#else
        savegamePreviewPtr = ptr;
#endif

}
int loadpheader(uint8_t  savegameNumber,int32_t *vn,int32_t *ln,int32_t *psk,int32_t *nump)
{
#if NOT_YET_IMPLEMENTED
    char  fn[] = "game0.sav";
    int32_t fil;
    int32_t bv;

         fn[4] = spot+'0';

     if ((fil = TCkopen4load(fn,0)) == -1) return(-1);

     tiles[MAXTILES-3].lock = 255;

     kdfread(&bv,4,1,fil);
     if(bv != BYTEVERSION)
     {
        FTA(114,&ps[myconnectindex],1);
        kclose(fil);
        return 1;
     }

     kdfread(nump,sizeof(int32_t),1,fil);

     kdfread(tempbuf,19,1,fil);
         kdfread(vn,sizeof(int32_t),1,fil);
         kdfread(ln,sizeof(int32_t),1,fil);
     kdfread(psk,sizeof(int32_t),1,fil);

     if (tiles[MAXTILES-3].data == NULL)
#error code disabled         allocache(&tiles[MAXTILES-3].data,160*100,&tiles[MAXTILES-3].lock);
    tiles[MAXTILES-3].dim.width = 100;
    tiles[MAXTILES-3].dim.height = 160;
    kdfread(tiles[MAXTILES-3].data,160,100,fil);
    kclose(fil);
    #else
#if WIN32
  nvmData_t *nvm = &ext_memory[EXT_MEMORY_SIZE - sizeof(nvmData_t)];
#else
    nvmData_t *nvm = (nvmData_t*) ((byte*) extMemGetSize() - sizeof(nvmData_t));
#endif

    uint32_t cookie;
    extMemGetDataFromAddress(&cookie, (uint8_t*) &nvm->savegames[savegameNumber].s.cookie, sizeof(cookie));
    //
    savegamePreviewPtr  = NULL;
    //
    if (cookie != SAVEGAME_COOKIE)
    {
        printf("Savegame cookie is 0x%x, not 0x%x\n", cookie, SAVEGAME_COOKIE);
        return -1;
    }
    uint32_t version;
    extMemGetDataFromAddress(&version, &nvm->savegames[savegameNumber].s.version, sizeof(version));
    //
    if (version != SAVEGAME_VERSION)
    {
        printf("Savegame version is %i, not %i\n", version, SAVEGAME_VERSION);
        return -1;
    }
    // read ud
    struct user_defs tud;
    extMemGetDataFromAddress(&tud, &nvm->savegames[savegameNumber].s.ud, sizeof(tud));
    //
    int32_t numplayers;
    extMemGetDataFromAddress(&numplayers, &nvm->savegames[savegameNumber].s.numplayers, sizeof(numplayers));
    // copy num players
    *nump = numplayers;
    // copy save game name
    memcpy(ud.savegame[savegameNumber], tud.savegame[savegameNumber], sizeof(tud.savegame[savegameNumber]));
    ud.savegame[savegameNumber][sizeof(ud.savegame[savegameNumber]) - 1] = 0;
    // copy volume number
    *vn = tud.volume_number;
    // copy level number
    *ln = tud.level_number;
    // copy player skill
    *psk = tud.player_skill;
    //
    savegamePreviewPtr = (uint8_t*)  &nvm->screenshots[savegameNumber];
    #endif
    return(0);
}


int loadplayer(int8_t savegameNumber)
{
    // wait refresh. We might use the display RAM as buffer.
#if !WIN32
    waitForUpdate(-1);
#endif

#if WIN32
  nvmData_t *nvm = &ext_memory[EXT_MEMORY_SIZE - sizeof(nvmData_t)];
#else
    nvmData_t *nvm = (nvmData_t*) ((byte*) extMemGetSize() - sizeof(nvmData_t));
#endif
    if (savegameNumber < 0)
    {
        multiflag = 1;
        multiwhat = 0;
        multipos = -savegameNumber - 1;
        return -1;
    }
    //
    FX_StopAllSounds();
    clearsoundlocks();
    MUSIC_StopSong();

    uint32_t cookie;
    extMemGetDataFromAddress(&cookie, (uint8_t*) &nvm->savegames[savegameNumber].s.cookie, sizeof(cookie));

    if (cookie != SAVEGAME_COOKIE)
    {
        printf("Savegame cookie is 0x%x, not 0x%x\n", cookie, SAVEGAME_COOKIE);
        return -1;
    }
    uint32_t version;
    extMemGetDataFromAddress(&version, &nvm->savegames[savegameNumber].s.version, sizeof(version));
    //
    if (version != SAVEGAME_VERSION)
    {
        printf("Savegame version is %i, not %i\n", version, SAVEGAME_VERSION);
        return -1;
    }
    // read ud
    struct user_defs tud;
    extMemGetDataFromAddress(&tud, &nvm->savegames[savegameNumber].s.ud, sizeof(tud));
    int oldvn = ud.volume_number;
    int oldln = ud.level_number;
    int oldps = ud.player_skill;
    int lastlevel = ud.last_level;
    ud.volume_number = tud.volume_number;
    ud.level_number = tud.level_number;
    ud.player_skill = tud.player_skill;
     //
    extMemGetDataFromAddress(&numplayers, &nvm->savegames[savegameNumber].s.numplayers, sizeof(numplayers));
    //
    if (oldvn != ud.volume_number || oldln != ud.level_number || oldps != ud.player_skill || lastlevel == -1)
    {
        newgame(ud.volume_number,ud.level_number,ud.player_skill);
        enterlevel(MODE_GAME);
    }
    //
    //memcpy(&ud, &tud, sizeof(ud)); // removed, this is part of options
    //  now modify the data for the game.

    LOAD_FROM_EXTMEM(savegameNumber, ps);
    LOAD_FROM_EXTMEM(savegameNumber, po);
    //
    LOAD_FROM_EXTMEM(savegameNumber, msx);
    LOAD_FROM_EXTMEM(savegameNumber, msy);
    //
    // TBD: see if it works with shorts.
    LOAD_FROM_EXTMEM(savegameNumber, numcyclers);
    LOAD_FROM_EXTMEM(savegameNumber, cyclers);
    //
    LOAD_FROM_EXTMEM(savegameNumber, animwall);
    LOAD_FROM_EXTMEM(savegameNumber, numanimwalls);
    //
    LOAD_FROM_EXTMEM(savegameNumber, spriteq);
    LOAD_FROM_EXTMEM(savegameNumber, spriteqloc);
    LOAD_FROM_EXTMEM(savegameNumber, spriteqamount);
    //
    LOAD_FROM_EXTMEM(savegameNumber, mirrorwall);
    LOAD_FROM_EXTMEM(savegameNumber, mirrorsector);
    LOAD_FROM_EXTMEM(savegameNumber, mirrorcnt);
    //
    LOAD_FROM_EXTMEM(savegameNumber, show2dsector);
    LOAD_FROM_EXTMEM(savegameNumber, show2dwall);
    LOAD_FROM_EXTMEM(savegameNumber, show2dsprite);
    //
    LOAD_FROM_EXTMEM(savegameNumber, boardfilename);
    //
    LOAD_FROM_EXTMEM(savegameNumber, pskyoff);
    //
    LOAD_FROM_EXTMEM(savegameNumber, pskybits);
    //
    LOAD_FROM_EXTMEM(savegameNumber, lockclock);
    LOAD_FROM_EXTMEM(savegameNumber, totalclock);
    LOAD_FROM_EXTMEM(savegameNumber, ototalclock);
    LOAD_FROM_EXTMEM(savegameNumber, lastvisinc);
    //
    LOAD_FROM_EXTMEM(savegameNumber, animatecnt);
    LOAD_FROM_EXTMEM(savegameNumber, animategoal);
    LOAD_FROM_EXTMEM(savegameNumber, animatevel);
    LOAD_FROM_EXTMEM(savegameNumber, animateidx);
    LOAD_FROM_EXTMEM(savegameNumber, animatesect);
    //
    LOAD_FROM_EXTMEM(savegameNumber, earthquaketime);
    //
    LOAD_FROM_EXTMEM(savegameNumber, camsprite);
    //
    LOAD_FROM_EXTMEM(savegameNumber, connecthead);
    LOAD_FROM_EXTMEM(savegameNumber, connectpoint2);
    //
    LOAD_FROM_EXTMEM(savegameNumber, numplayersprites);
    //
    LOAD_FROM_EXTMEM(savegameNumber, frags);
    LOAD_FROM_EXTMEM(savegameNumber, randomseed);
    //
    LOAD_FROM_EXTMEM(savegameNumber, global_random);
    //
    LOAD_FROM_EXTMEM(savegameNumber, parallaxyscale);
    //
    LOAD_FROM_EXTMEM(savegameNumber, numsectors);
    LOAD_FROM_EXTMEM(savegameNumber, numwalls);
    //
    LOAD_FROM_EXTMEM(savegameNumber, ramwalls);
    LOAD_FROM_EXTMEM(savegameNumber, ramsprites);
    LOAD_FROM_EXTMEM(savegameNumber, ramspriteIdx);
    #if OPTIMIZE_RAMWALLIDX
        LOAD_FROM_EXTMEM(savegameNumber, ramwallIdx8);
        LOAD_FROM_EXTMEM(savegameNumber, ramwallIdx2);
    #else
        LOAD_FROM_EXTMEM(savegameNumber, ramwallIdx);
    #endif
    LOAD_FROM_EXTMEM(savegameNumber, ramsectors);
    #if MAXRAMSECTORS >= 255
        LOAD_FROM_EXTMEM(savegameNumber, ramsectorIdx);
    #else
        LOAD_FROM_EXTMEM(savegameNumber, ramsectorIdx);
    #endif
    //
    LOAD_FROM_EXTMEM(savegameNumber, numModifiedWalls);
    LOAD_FROM_EXTMEM(savegameNumber, numModifiedSectors);
    LOAD_FROM_EXTMEM(savegameNumber, numModifiedSprites);
    //
    LOAD_FROM_EXTMEM(savegameNumber, ramsprites_allocated_bf);
    //
    LOAD_FROM_EXTMEM(savegameNumber, spr_add_data);
    LOAD_FROM_EXTMEM(savegameNumber, headspritesect);
    LOAD_FROM_EXTMEM(savegameNumber, headspritestat);
    //
    LOAD_FROM_EXTMEM(savegameNumber, wallshade);
    LOAD_FROM_EXTMEM(savegameNumber, wall_hitag9999);
    LOAD_FROM_EXTMEM(savegameNumber, wall_extraIsMinusOne);
    //
    LOAD_FROM_EXTMEM(savegameNumber, ramSectorExtra);
#if SEPARATE_SECTOR_CEILINGSHADE
    LOAD_FROM_EXTMEM(savegameNumber, ramSectorCeilingShade);
#endif
#if SEPARATE_SECTOR_FLOORSHADE
    LOAD_FROM_EXTMEM(savegameNumber, ramSectorFloorShade);
#endif
    LOAD_FROM_EXTMEM(savegameNumber, ramSectorFloorxpanning);
//
    #if USE_SEPARATE_HITTYPE_TEMPDATA
        LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata0_allocated_bf);
        LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata1_allocated_bf);
        LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata2_allocated_bf);
        LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata3_allocated_bf);
        LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata45_allocated_bf);
        //
        LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata0);
        LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata1);
        LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata2);
        LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata3);
        LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata45);
        LOAD_FROM_EXTMEM(savegameNumber,  numHittype_tempdata0);
        LOAD_FROM_EXTMEM(savegameNumber,  numHittype_tempdata1);
        LOAD_FROM_EXTMEM(savegameNumber,  numHittype_tempdata2);
        LOAD_FROM_EXTMEM(savegameNumber,  numHittype_tempdata3);
        LOAD_FROM_EXTMEM(savegameNumber,  numHittype_tempdata45);

    #else
	    LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata_allocated_bf);
	    LOAD_FROM_EXTMEM(savegameNumber,  hittype_tempdata);
	    LOAD_FROM_EXTMEM(savegameNumber,  numHittype_tempdata);

    #endif // USE_SEPARATE_HITTYPE_TEMPDATA
//
    LOAD_FROM_EXTMEM(savegameNumber,  hittype_otherdata_allocated_bf);
    LOAD_FROM_EXTMEM(savegameNumber,  hittype_otherdata);
    #if USE_RAM_HITTYPE
        LOAD_FROM_EXTMEM(savegameNumber,  hittype_allocated_bf);
        LOAD_FROM_EXTMEM(savegameNumber,  numHittypes);
        LOAD_FROM_EXTMEM(savegameNumber,  hittypeIdx);
        LOAD_FROM_EXTMEM(savegameNumber,  ramHittype);
    #else
        #error
    #endif // USE_RAM_HITTYPE
    LOAD_FROM_EXTMEM(savegameNumber,  numHittype_otherdata);
    //
    LOAD_FROM_EXTMEM(savegameNumber, curposidx);
    LOAD_FROM_EXTMEM(savegameNumber, numinterpolations);
    LOAD_FROM_EXTMEM(savegameNumber, startofdynamicinterpolations);
    LOAD_FROM_EXTMEM(savegameNumber, oldipos);
    //
    LOAD_FROM_EXTMEM(savegameNumber, interpolationState);
    LOAD_FROM_EXTMEM(savegameNumber, interpolationSmoothRatio);
    //
    #if USE_BLOCKMAP
        LOAD_FROM_EXTMEM(savegameNumber, sectorModifiedBf);
    #endif

    clearbufbyte(playerquitflag, MAXPLAYERS, 0x01010101);

    resetmys();

    ready2send = 1;

    flushpackets();
    clearfifo();
    waitforeverybody();

    resettimevars();

    return(0);
}
#if !WIN32
void updateAudioBackgroundTask(void)
{
    // update audio every 50 ms so that during extmem writes it does not glitch.
    static uint32_t lastTime = 0;
    sampletimer();
    uint32_t timeNow = totalclock;
    if ((timeNow - lastTime) > 8UL)
    {
        updateAudio();
        lastTime = timeNow;
    }
}
#endif
void menuEnableMultiplayerReady(uint8_t status)
{
    ud.multiReady = status;
}

int saveplayer(int8_t savegameNumber)
{
    if (savegameNumber < 0)
    {
        multiflag = 1;
        multiwhat = 1;
        multipos = -savegameNumber - 1;
        return -1;
    }
    // next-hack: we do not save multiplayer.
    if (numplayers > 1)
        return - 1;
    waitforeverybody();
    ready2send = 0;
    MUSIC_StopSong();
    FX_StopAllSounds();
#if !WIN32
    extMemSetMemBusyBackgroundTask(updateAudioBackgroundTask);
#endif
    // next-hack. Ok this is really crappy. We cannot really use the pointer, it's just for offsets!
    nvmData_t *nvm = (nvmData_t*) ((byte*) extMemGetSize() - sizeof(nvmData_t));
    extMemErase((uint32_t) &nvm->savegames[savegameNumber], SAVE_GAME_SIZE);
    extMemErase((uint32_t) &nvm->screenshots[savegameNumber], sizeof(nvm->screenshots[savegameNumber]));
    //
    extMemProgram((uint32_t) &nvm->savegames[savegameNumber].s.ud, (uint8_t*) &ud, sizeof(nvm->savegames[savegameNumber].s.ud));
    //
    extMemProgram((uint32_t) &nvm->savegames[savegameNumber].s.numplayers, (uint8_t*) &numplayers, sizeof(nvm->savegames[savegameNumber].s.numplayers));
    //
#if WIN32
    CopyPreviewScreenshot(&nvm->screenshots[savegameNumber], &ext_memory[(uint32_t) &nvm->tempScreenshot]);

#else
    CopyPreviewScreenshot(nvm->screenshots[savegameNumber], nvm->tempScreenshot);
#endif
    //
    SAVE_TO_EXTMEM(savegameNumber, ps);
    SAVE_TO_EXTMEM(savegameNumber, po);
    //
    SAVE_TO_EXTMEM(savegameNumber, msx);
    SAVE_TO_EXTMEM(savegameNumber, msy);
    //
    // TBD: see if it works with shorts.
    SAVE_TO_EXTMEM(savegameNumber, numcyclers);
    SAVE_TO_EXTMEM(savegameNumber, cyclers);
    //
    SAVE_TO_EXTMEM(savegameNumber, animwall);
    SAVE_TO_EXTMEM(savegameNumber, numanimwalls);
    //
    SAVE_TO_EXTMEM(savegameNumber, spriteq);
    SAVE_TO_EXTMEM(savegameNumber, spriteqloc);
    SAVE_TO_EXTMEM(savegameNumber, spriteqamount);
    //
    SAVE_TO_EXTMEM(savegameNumber, mirrorwall);
    SAVE_TO_EXTMEM(savegameNumber, mirrorsector);
    SAVE_TO_EXTMEM(savegameNumber, mirrorcnt);
    //
    SAVE_TO_EXTMEM(savegameNumber, show2dsector);
    SAVE_TO_EXTMEM(savegameNumber, show2dwall);
    SAVE_TO_EXTMEM(savegameNumber, show2dsprite);
    //
    SAVE_TO_EXTMEM(savegameNumber, boardfilename);
    //
    SAVE_TO_EXTMEM(savegameNumber, pskyoff);
    //
    SAVE_TO_EXTMEM(savegameNumber, pskybits);
    //
    SAVE_TO_EXTMEM(savegameNumber, lockclock);
    SAVE_TO_EXTMEM(savegameNumber, totalclock);
    SAVE_TO_EXTMEM(savegameNumber, ototalclock);
    SAVE_TO_EXTMEM(savegameNumber, lastvisinc);
    //
    SAVE_TO_EXTMEM(savegameNumber, animatecnt);
    SAVE_TO_EXTMEM(savegameNumber, animategoal);
    SAVE_TO_EXTMEM(savegameNumber, animatevel);
    SAVE_TO_EXTMEM(savegameNumber, animateidx);
    SAVE_TO_EXTMEM(savegameNumber, animatesect);
    //
    SAVE_TO_EXTMEM(savegameNumber, earthquaketime);
    //
    SAVE_TO_EXTMEM(savegameNumber, camsprite);
    //
    SAVE_TO_EXTMEM(savegameNumber, connecthead);
    SAVE_TO_EXTMEM(savegameNumber, connectpoint2);
    //
    SAVE_TO_EXTMEM(savegameNumber, numplayersprites);
    //
    SAVE_TO_EXTMEM(savegameNumber, frags);
    SAVE_TO_EXTMEM(savegameNumber, randomseed);
    //
    SAVE_TO_EXTMEM(savegameNumber, global_random);
    //
    SAVE_TO_EXTMEM(savegameNumber, parallaxyscale);
    //
    SAVE_TO_EXTMEM(savegameNumber, numsectors);
    SAVE_TO_EXTMEM(savegameNumber, numwalls);
    //
    SAVE_TO_EXTMEM(savegameNumber, ramwalls);
    SAVE_TO_EXTMEM(savegameNumber, ramsprites);
    SAVE_TO_EXTMEM(savegameNumber, ramspriteIdx);
    #if OPTIMIZE_RAMWALLIDX
        SAVE_TO_EXTMEM(savegameNumber, ramwallIdx8);
        SAVE_TO_EXTMEM(savegameNumber, ramwallIdx2);
    #else
        SAVE_TO_EXTMEM(savegameNumber, ramwallIdx);
    #endif
    SAVE_TO_EXTMEM(savegameNumber, ramsectors);
    #if MAXRAMSECTORS >= 255
        SAVE_TO_EXTMEM(savegameNumber, ramsectorIdx);
    #else
        SAVE_TO_EXTMEM(savegameNumber, ramsectorIdx);
    #endif
    //
    SAVE_TO_EXTMEM(savegameNumber, numModifiedWalls);
    SAVE_TO_EXTMEM(savegameNumber, numModifiedSectors);
    SAVE_TO_EXTMEM(savegameNumber, numModifiedSprites);
    //
    SAVE_TO_EXTMEM(savegameNumber, ramsprites_allocated_bf);
    //
    SAVE_TO_EXTMEM(savegameNumber, spr_add_data);
    SAVE_TO_EXTMEM(savegameNumber, headspritesect);
    SAVE_TO_EXTMEM(savegameNumber, headspritestat);
    //
    SAVE_TO_EXTMEM(savegameNumber, wallshade);
    SAVE_TO_EXTMEM(savegameNumber, wall_hitag9999);
    SAVE_TO_EXTMEM(savegameNumber, wall_extraIsMinusOne);
    //
    SAVE_TO_EXTMEM(savegameNumber, ramSectorExtra);
#if SEPARATE_SECTOR_CEILINGSHADE
    SAVE_TO_EXTMEM(savegameNumber, ramSectorCeilingShade);
#endif
#if SEPARATE_SECTOR_FLOORSHADE
    SAVE_TO_EXTMEM(savegameNumber, ramSectorFloorShade);
#endif
    SAVE_TO_EXTMEM(savegameNumber, ramSectorFloorxpanning);
//
    #if USE_SEPARATE_HITTYPE_TEMPDATA
        SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata0_allocated_bf);
        SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata1_allocated_bf);
        SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata2_allocated_bf);
        SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata3_allocated_bf);
        SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata45_allocated_bf);
        //
        SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata0);
        SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata1);
        SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata2);
        SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata3);
        SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata45);
        SAVE_TO_EXTMEM(savegameNumber,  numHittype_tempdata0);
        SAVE_TO_EXTMEM(savegameNumber,  numHittype_tempdata1);
        SAVE_TO_EXTMEM(savegameNumber,  numHittype_tempdata2);
        SAVE_TO_EXTMEM(savegameNumber,  numHittype_tempdata3);
        SAVE_TO_EXTMEM(savegameNumber,  numHittype_tempdata45);

    #else
	    SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata_allocated_bf);
	    SAVE_TO_EXTMEM(savegameNumber,  hittype_tempdata);
	    SAVE_TO_EXTMEM(savegameNumber,  numHittype_tempdata);

    #endif // USE_SEPARATE_HITTYPE_TEMPDATA
//
    SAVE_TO_EXTMEM(savegameNumber,  hittype_otherdata_allocated_bf);
    SAVE_TO_EXTMEM(savegameNumber,  hittype_otherdata);
    #if USE_RAM_HITTYPE
        SAVE_TO_EXTMEM(savegameNumber,  hittype_allocated_bf);
        SAVE_TO_EXTMEM(savegameNumber,  numHittypes);
        SAVE_TO_EXTMEM(savegameNumber, hittypeIdx);
        SAVE_TO_EXTMEM(savegameNumber,  ramHittype);
    #endif // USE_RAM_HITTYPE
    SAVE_TO_EXTMEM(savegameNumber,  numHittype_otherdata);
    //
    SAVE_TO_EXTMEM(savegameNumber, curposidx);
    SAVE_TO_EXTMEM(savegameNumber, numinterpolations);
    SAVE_TO_EXTMEM(savegameNumber, startofdynamicinterpolations);
    SAVE_TO_EXTMEM(savegameNumber, oldipos);
    //
    SAVE_TO_EXTMEM(savegameNumber, interpolationState);
    SAVE_TO_EXTMEM(savegameNumber, interpolationSmoothRatio);
    SAVE_TO_EXTMEM(savegameNumber, interpolationSmoothRatio);
    //
    #if USE_BLOCKMAP
    SAVE_TO_EXTMEM(savegameNumber, sectorModifiedBf);
    #endif
    //
    uint32_t cookie = SAVEGAME_COOKIE;
    SAVE_TO_EXTMEM(savegameNumber, cookie);
    uint32_t version = SAVEGAME_VERSION;
    SAVE_TO_EXTMEM(savegameNumber, version);
    //
#if !WIN32
    extMemSetMemBusyBackgroundTask(NULL);
    //
#endif
    MUSIC_Continue();
    //
    ready2send = 1;
    waitforeverybody();
    ototalclock = totalclock;
    return(0);
}

#define LMB (buttonstat&1)
#define RMB (buttonstat&2)

// FIX_00036: Mouse wheel can now be used in menu
#define WHEELUP (buttonstat&8)
#define WHEELDOWN (buttonstat&16)

ControlInfo minfo;

int probe(int x,int y,int i,int n)
{
	return( probeXduke(x, y, i, n, 65536L) );
}

int probeXduke(int x,int y,int i,int n, int32_t spriteSize)
{
    short centre;
	int32_t mouseSens;

	static int32_t delay_counter_up = 0, delay_counter_down = 0, delay_up = 50, delay_down = 50;
	static int32_t mi = 0;

	// FIX_00075: Bad Sensitivity along Y axis when using mouse in menu (Thanks to Turrican)
	mouseSens = CONTROL_GetAnalogSensitivity(left_y_axis);
	mouseSens = mouseSens ? mouseSens : 1;

    if( ((ControllerType == controltype_keyboardandmouse)||
		(ControllerType == controltype_joystickandmouse) ||
		(ControllerType == controltype_keyboardandjoystick)) )
		//&& CONTROL_MousePresent )
    {
		memset(&minfo, 0, sizeof(ControlInfo));

        CONTROL_GetInput( &minfo );
		//mouseY = CONTROL_GetMouseY();
		//mi = mouseY;
        mi += minfo.dz / mouseSens;
		mi += minfo.dpitch / mouseSens;
    }

    else
        {
            minfo.dz = minfo.dyaw = 0;
        }

    if( x == (320>>1) )
        centre = 320>>2;
    else centre = 0;

        if( KB_KeyPressed( sc_UpArrow ) || KB_KeyPressed( sc_PgUp ) || KB_KeyPressed( sc_kpad_8 ) ||
            (mi < -1024) || WHEELUP)
        {
			// FIX_00060: Repeat key function was not working in the menu
			if(delay_counter_up==0 || (totalclock-delay_counter_up)>delay_up || (mi < -1024) || WHEELUP)
			{
				mi = 0;
				sound(KICK_HIT);

				probey--;
				if(probey < 0) probey = n-1;
				minfo.dz = 0;
				minfo.dpitch = 0;
				if (delay_counter_up && (totalclock-delay_counter_up)>delay_up)
					delay_up = 10;
				delay_counter_up = totalclock;
			}
        }
		else
		{
            KB_ClearKeyDown( sc_UpArrow );
            KB_ClearKeyDown( sc_kpad_8 );
            KB_ClearKeyDown( sc_PgUp );
			delay_counter_up = 0;
			delay_up = 50;
		}

        if( KB_KeyPressed( sc_DownArrow ) || KB_KeyPressed( sc_PgDn ) || KB_KeyPressed( sc_kpad_2 )
            || (mi > 1024) || WHEELDOWN )
        {
			if(delay_counter_down==0 || (totalclock-delay_counter_down)>delay_down || (mi > 1024) || WHEELDOWN)
			{
				mi = 0;
				sound(KICK_HIT);
				probey++;
				minfo.dz = 0;
				minfo.dpitch = 0;
				if (delay_counter_down && (totalclock-delay_counter_down)>delay_down)
					delay_down = 10;
				delay_counter_down = totalclock;
			}
        }
		else
		{
			KB_ClearKeyDown( sc_DownArrow );
			KB_ClearKeyDown( sc_kpad_2 );
			KB_ClearKeyDown( sc_PgDn );
			delay_counter_down = 0;
			delay_down = 50;
		}

    if(probey >= n)
        probey = 0;

    if(centre)
    {
//        rotatesprite(((320>>1)+(centre)+54)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+6-((6+(totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);
//        rotatesprite(((320>>1)-(centre)-54)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+((totalclock>>3)%7),sh,0,10,0,0,xdim-1,ydim-1);

        rotatesprite(((320>>1)+(centre>>1)+70)<<16,(y+(probey*i)-4)<<16,spriteSize,0,SPINNINGNUKEICON+6-((6+(totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(((320>>1)-(centre>>1)-70)<<16,(y+(probey*i)-4)<<16,spriteSize,0,SPINNINGNUKEICON+((totalclock>>3)%7),sh,0,10,0,0,xdim-1,ydim-1);
    }
    else
        rotatesprite((x-tiles[BIGFNTCURSOR].dim.width-4)<<16,(y+(probey*i)-4)<<16,spriteSize,0,SPINNINGNUKEICON+(((totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);

    if( KB_KeyPressed(sc_Space) || KB_KeyPressed( sc_kpad_Enter ) || KB_KeyPressed( sc_Enter ) || (LMB))// && !onbar) )
    {
        if(current_menu != MENU_SKILL_SELECT)
            sound(PISTOL_BODYHIT);
        KB_ClearKeyDown( sc_Enter );
        KB_ClearKeyDown( sc_Space );
        KB_ClearKeyDown( sc_kpad_Enter );
        return(probey);
    }
    else if( KB_KeyPressed( sc_Escape ) || (RMB) )
    {
        onbar = 0;
        KB_ClearKeyDown( sc_Escape );
        sound(EXITMENUSOUND);
        return(-1);
    }
    else
    {
        if(onbar == 0) return(-probey-2);
        if ( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -128 ) )
            return(probey);
        else if ( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 128 ) )
            return(probey);
        else return(-probey-2);
    }
}

int menutext(int x,int y,short s,short p,const char  *t)
{
    short i, ac, centre;

    y -= 12;

    i = centre = 0;

    if( x == (320>>1) )
    {
        while( *(t+i) )
        {
            if(*(t+i) == ' ')
            {
                centre += 5;
                i++;
                continue;
            }
            ac = 0;
            if(*(t+i) >= '0' && *(t+i) <= '9')
                ac = *(t+i) - '0' + BIGALPHANUM-10;
            else if(*(t+i) >= 'a' && *(t+i) <= 'z')
                ac = toupper(*(t+i)) - 'A' + BIGALPHANUM;
            else if(*(t+i) >= 'A' && *(t+i) <= 'Z')
                ac = *(t+i) - 'A' + BIGALPHANUM;
            else switch(*(t+i))
            {
                case '-':
                    ac = BIGALPHANUM-11;
                    break;
                case '.':
                    ac = BIGPERIOD;
                    break;
                case '\'':
                    ac = BIGAPPOS;
                    break;
                case ',':
                    ac = BIGCOMMA;
                    break;
                case '!':
                    ac = BIGX;
                    break;
                case '?':
                    ac = BIGQ;
                    break;
                case ';':
                    ac = BIGSEMI;
                    break;
                case ':':
                    ac = BIGSEMI;
                    break;
                default:
                    centre += 5;
                    i++;
                    continue;
            }

            centre += tiles[ac].dim.width-1;
            i++;
        }
    }

    if(centre)
        x = (320-centre-10)>>1;

    while(*t)
    {
        if(*t == ' ') {x+=5;t++;continue;}
        ac = 0;
        if(*t >= '0' && *t <= '9')
            ac = *t - '0' + BIGALPHANUM-10;
        else if(*t >= 'a' && *t <= 'z')
            ac = toupper(*t) - 'A' + BIGALPHANUM;
        else if(*t >= 'A' && *t <= 'Z')
            ac = *t - 'A' + BIGALPHANUM;
        else switch(*t)
        {
            case '-':
                ac = BIGALPHANUM-11;
                break;
            case '.':
                ac = BIGPERIOD;
                break;
            case ',':
                ac = BIGCOMMA;
                break;
            case '!':
                ac = BIGX;
                break;
            case '\'':
                ac = BIGAPPOS;
                break;
            case '?':
                ac = BIGQ;
                break;
            case ';':
                ac = BIGSEMI;
                break;
            case ':':
                ac = BIGCOLIN;
                break;
            default:
                x += 5;
                t++;
                continue;
        }
        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,10+16,0,0,xdim-1,ydim-1);

        x += tiles[ac].dim.width;
        t++;
    }
    return (x);
}

int menutextc(int x,int y,short s,short p,char  *t)
{
    short i, ac, centre;

    s += 8;
    y -= 12;

    i = centre = 0;

//    if( x == (320>>1) )
    {
        while( *(t+i) )
        {
            if(*(t+i) == ' ')
            {
                centre += 5;
                i++;
                continue;
            }
            ac = 0;
            if(*(t+i) >= '0' && *(t+i) <= '9')
                ac = *(t+i) - '0' + BIGALPHANUM+26+26;
            if(*(t+i) >= 'a' && *(t+i) <= 'z')
                ac = *(t+i) - 'a' + BIGALPHANUM+26;
            if(*(t+i) >= 'A' && *(t+i) <= 'Z')
                ac = *(t+i) - 'A' + BIGALPHANUM;

            else switch(*t)
            {
                case '-':
                    ac = BIGALPHANUM-11;
                    break;
                case '.':
                    ac = BIGPERIOD;
                    break;
                case ',':
                    ac = BIGCOMMA;
                    break;
                case '!':
                    ac = BIGX;
                    break;
                case '?':
                    ac = BIGQ;
                    break;
                case ';':
                    ac = BIGSEMI;
                    break;
                case ':':
                    ac = BIGCOLIN;
                    break;
            }

            centre += tiles[ac].dim.width-1;
            i++;
        }
    }

    x -= centre>>1;

    while(*t)
    {
        if(*t == ' ') {x+=5;t++;continue;}
        ac = 0;
        if(*t >= '0' && *t <= '9')
            ac = *t - '0' + BIGALPHANUM+26+26;
        if(*t >= 'a' && *t <= 'z')
            ac = *t - 'a' + BIGALPHANUM+26;
        if(*t >= 'A' && *t <= 'Z')
            ac = *t - 'A' + BIGALPHANUM;
        switch(*t)
        {
            case '-':
                ac = BIGALPHANUM-11;
                break;
            case '.':
                ac = BIGPERIOD;
                break;
            case ',':
                ac = BIGCOMMA;
                break;
            case '!':
                ac = BIGX;
                break;
            case '?':
                ac = BIGQ;
                break;
            case ';':
                ac = BIGSEMI;
                break;
            case ':':
                ac = BIGCOLIN;
                break;
        }

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,10+16,0,0,xdim-1,ydim-1);

        x += tiles[ac].dim.width;
        t++;
    }
    return (x);
}


void bar(int x,int y,short *p,short dainc,uint8_t  damodify,short s, short pa)
{
    short xloc;
    uint8_t  rev;

    if(dainc < 0) { dainc = -dainc; rev = 1; }
    else rev = 0;
    y-=2;

    if(damodify)
    {
        if(rev == 0)
        {
            if( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -256 ) ) // && onbar) )
            {
                KB_ClearKeyDown( sc_LeftArrow );
                KB_ClearKeyDown( sc_kpad_4 );

                *p -= dainc;
                if(*p < 0)
                    *p = 0;
                sound(KICK_HIT);
            }
            if( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 256 ) )//&& onbar) )
            {
                KB_ClearKeyDown( sc_RightArrow );
                KB_ClearKeyDown( sc_kpad_6 );

                *p += dainc;
                if(*p > 63)
                    *p = 63;
                sound(KICK_HIT);
            }
        }
        else
        {
            if( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 256 ))//&& onbar ))
            {
                KB_ClearKeyDown( sc_RightArrow );
                KB_ClearKeyDown( sc_kpad_6 );

                *p -= dainc;
                if(*p < 0)
                    *p = 0;
                sound(KICK_HIT);
            }
            if( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -256 ))// && onbar) )
            {
                KB_ClearKeyDown( sc_LeftArrow );
                KB_ClearKeyDown( sc_kpad_4 );

                *p += dainc;
                if(*p > 64)
                    *p = 64;
                sound(KICK_HIT);
            }
        }
    }

    xloc = *p;

    rotatesprite( (x+22)<<16,(y-3)<<16,65536L,0,SLIDEBAR,s,pa,10,0,0,xdim-1,ydim-1);
    if(rev == 0)
        rotatesprite( (x+xloc+1)<<16,(y+1)<<16,65536L,0,SLIDEBAR+1,s,pa,10,0,0,xdim-1,ydim-1);
    else
        rotatesprite( (x+(65-xloc) )<<16,(y+1)<<16,65536L,0,SLIDEBAR+1,s,pa,10,0,0,xdim-1,ydim-1);
}

#define SHX(X) 0
// ((x==X)*(-sh))
#define PHX(X) 0
// ((x==X)?1:2)
#define MWIN(X) rotatesprite( 320<<15,200<<15,X,0,MENUSCREEN,-16,0,10+64,0,0,xdim-1,ydim-1)
#define MWINXY(X,OX,OY) rotatesprite( ( 320+(OX) )<<15, ( 200+(OY) )<<15,X,0,MENUSCREEN,-16,0,10+64,0,0,xdim-1,ydim-1)


int32_t volnum,levnum,plrskl,numplr;
short lastsavedpos = -1;
extern int16_t * lastx;
void dispnames(void)
{
    short x, c = 160;
    #if LASTX_IN_STACK
        int16_t stack_lastx[MAXYDIM] = {0};  // was 32 bit
        lastx = stack_lastx;
    #endif // LASTX_IN_STACK

    c += 64;
    for(x = 0;x <= 108;x += 12)
    rotatesprite((c+91-64)<<16,(x+56)<<16,65536L,0,TEXTBOX,24,0,10,0,0,xdim-1,ydim-1);
    //
    uint8_t stackTile[MAX_RAM_FLAT_SIZE];
    clearBufferedTileImagePtr();
    setTempRamTileData(stackTile);

    rotatesprite(22<<16,97<<16,65536L,0,WINDOWBORDER2,24,0,10,0,0,xdim-1,ydim-1); // left
    rotatesprite(180<<16,97<<16,65536L,1024,WINDOWBORDER2,24,0,10,0,0,xdim-1,ydim-1); // right
    rotatesprite(99<<16,50<<16,65536L,512,WINDOWBORDER1,24,0,10,0,0,xdim-1,ydim-1); // top
    rotatesprite(103<<16,144<<16,65536L,1024+512,WINDOWBORDER1,24,0,10,0,0,xdim-1,ydim-1); // bottom


    minitext(c,48,ud.savegame[0],2,10+16);
    minitext(c,48+12,ud.savegame[1],2,10+16);
    minitext(c,48+12+12,ud.savegame[2],2,10+16);
    minitext(c,48+12+12+12,ud.savegame[3],2,10+16);
    minitext(c,48+12+12+12+12,ud.savegame[4],2,10+16);
    minitext(c,48+12+12+12+12+12,ud.savegame[5],2,10+16);
    minitext(c,48+12+12+12+12+12+12,ud.savegame[6],2,10+16);
    minitext(c,48+12+12+12+12+12+12+12,ud.savegame[7],2,10+16);
    minitext(c,48+12+12+12+12+12+12+12+12,ud.savegame[8],2,10+16);
    minitext(c,48+12+12+12+12+12+12+12+12+12,ud.savegame[9],2,10+16);

}


// This is the same thing as was in build.c ...
//  We DO have a _dos_findfirst implementation now...maybe use that instead?
//     --ryan.
#if ORIGINAL_DUKE3D_GETFILENAMES
int getfilenames(uint8_t  kind[6])
{
        short type;
        struct find_t fileinfo;

        if (strcmp(kind,"SUBD") == 0)
        {
                strcpy(kind,"*.*");
                if (_dos_findfirst(kind,_A_SUBDIR,&fileinfo) != 0)
                        return(-1);
                type = 1;
        }
        else
        {
                if (_dos_findfirst(kind,_A_NORMAL,&fileinfo) != 0)
                        return(-1);
                type = 0;
        }
        do
        {
                if ((type == 0) || ((fileinfo.attrib&16) > 0))
                        if ((fileinfo.name[0] != '.') || (fileinfo.name[1] != 0))
                        {
                                strcpy(menuname[menunamecnt],fileinfo.name);
                                menuname[menunamecnt][16] = type;
                                menunamecnt++;
                        }
        }
        while (_dos_findnext(&fileinfo) == 0);

        return(0);
}

#else
#if USER_MAP_SUPPORT
int getfilenames(char  kind[6])
{
/* !!! FIXME: Visual C? */
#if (defined __WATCOMC__)
	short type;
	struct find_t fileinfo;

	if (strcmp(kind,"SUBD") == 0)
	{
		strcpy(kind,"*.*");
		if (_dos_findfirst(kind,_A_SUBDIR,&fileinfo) != 0)
			return(-1);
		type = 1;
	}
	else
	{
		if (_dos_findfirst(kind,_A_NORMAL,&fileinfo) != 0)
			return(-1);
		type = 0;
	}
	do
	{
		if ((type == 0) || ((fileinfo.attrib&16) > 0))
			if ((fileinfo.name[0] != '.') || (fileinfo.name[1] != 0))
			{
				strcpy(menuname[menunamecnt],fileinfo.name);
				menuname[menunamecnt][16] = type;
				menunamecnt++;
			}
	}
	while (_dos_findnext(&fileinfo) == 0);

#elif (defined UNIX)

    DIR *dir;
    struct dirent *dent;
    struct stat statbuf;
    int add_this;
    uint8_t  *ptr = NULL;
    int len = 0;
    int subdirs = 0;

	if (strcmp(kind,"SUBD") == 0)
        subdirs = 1;

    dir = opendir(".");
    if (dir == NULL)
        return(-1);

    do
    {
        add_this = 0;
        dent = readdir(dir);
        if (dent != NULL)
        {
            if (stat(dent->d_name, &statbuf) == 0)
            {
                if (subdirs)
                {
                    if (S_ISDIR(statbuf.st_mode))
                    add_this = 1;
                } /* if */
                else
                {
                    /* need to expand support if this assertion ever fails. */
                    assert(stricmp(kind, "*.MAP") == 0);
                    len = strlen(dent->d_name);
                    if (len >= 5)
                    {
                        ptr = ((uint8_t  *) dent->d_name) + len;
                        ptr += strlen(ptr) - 4;
                        if (stricmp(ptr, ".MAP") == 0)
                    add_this = 1;
                    } /* if */
                } /* else */

                if (add_this)
    			{
	    			strcpy(menuname[menunamecnt],dent->d_name);
		    		menuname[menunamecnt][16] = subdirs;
			    	menunamecnt++;
                } /* if */
            } /* if */
        } /* if */
    } while (dent != NULL);

    closedir(dir);

#endif
	return(0);
}
#endif
#endif

#if USER_MAP_SUPPORT
void sortfilenames()
{
        uint8_t  sortbuffer[17];
        int32_t i, j, k;

        for(i=1;i<menunamecnt;i++)
                for(j=0;j<i;j++)
                {
                         k = 0;
                         while ((menuname[i][k] == menuname[j][k]) && (menuname[i][k] != 0) && (menuname[j][k] != 0))
                                 k++;
                        if (menuname[i][k] < menuname[j][k])
                        {
                                memcpy(&sortbuffer[0],&menuname[i][0],sizeof(menuname[0]));
                                memcpy(&menuname[i][0],&menuname[j][0],sizeof(menuname[0]));
                                memcpy(&menuname[j][0],&sortbuffer[0],sizeof(menuname[0]));
                        }
                }
}
#endif
int32_t quittimer = 0;

void gameexitanycase(void)
{
    KB_FlushKeyboardQueue();

    if( gamequit == 0 && ( numplayers > 1 ) )
    {
        if(ps[myconnectindex].gm&MODE_GAME)
        {
            gamequit = 1;
            quittimer = totalclock+120;
			sound(SHORT_CIRCUIT);
        }
        else
        {
            sendlogoff();
            gameexit(" ");
			sound(NITEVISION_ONOFF);
        }
    }
    else if( numplayers < 2 )
        gameexit(" ");

    if( ( totalclock > quittimer ) && ( gamequit == 1) )
	{
        gameexit("Timed out.");
	}
}
int isInLobbyMenu(void)
{
    return MENU_MULTI_PEER_LOBBY == current_menu;
}
void checkSavegameScreenshotRequest(void)
{
    if (capturingForSaveGame)
    {
        nvmData_t *nvm = (nvmData_t*) ((byte*) extMemGetSize() - sizeof(nvmData_t));

        printf("size: %d 0x%p sizeof screenshot: %d addr ss: 0x%p", extMemGetSize(), nvm, sizeof(nvm->tempScreenshot), &nvm->tempScreenshot );
        MUSIC_StopSong();
        FX_StopAllSounds();
#if !WIN32
        extMemSetMemBusyBackgroundTask(updateAudioBackgroundTask);
        extMemErase((uint32_t) &nvm->tempScreenshot, sizeof(nvm->tempScreenshot));
#endif
        savePreviewScreenshot((uint8_t *) &nvm->tempScreenshot);
#if !WIN32
        extMemSetMemBusyBackgroundTask(NULL);
#endif
        MUSIC_Continue();
        capturingForSaveGame = 0;
        cmenu(MENU_SAVE_GAME_SELECT);
    }
}
static uint8_t textInputLoc;
static uint8_t enteringText;
int enterText(short x,short y,char  *t,short dalen,short c)
{
    if (!enteringText)
        return 0;
    short ch,sc;

    if(KB_KeyWaiting())
    {
        sc = KB_LastScan;
        ch = KB_getLastAsciiCode();
        printf("sc %d, ch %d\r\n", sc, ch);

        KB_ClearKeysDown();
        if (sc == 90) // up
        {
            if (textInputLoc < dalen)
            {
                if (t[textInputLoc] < 32)
                    t[textInputLoc] = 'A';
                else
                    if (t[textInputLoc] < 96)
                        t[textInputLoc]++;
                t[textInputLoc+1] = 0;
            }
        }
        if (sc == 106) // down
        {
            if (textInputLoc < dalen)
            {
                if (t[textInputLoc] < 32)
                    t[textInputLoc] = 'A';
                else
                    if (t[textInputLoc] > 32)
                        t[textInputLoc]--;
                t[textInputLoc+1] = 0;
            }
        }
        if (sc == 108) // right
        {
            if (textInputLoc < dalen)
            {
                if (t[textInputLoc] >= 32)
                {
                    textInputLoc++;
                }
            }
        }
        if(sc == 14 || sc == 107) // asc_BackSpace or left
        {
            if( textInputLoc > 0 )
            {
                textInputLoc--;
                *(t+textInputLoc) = 0;
            }
        }
        if(sc == 28 || sc == 102 || sc == 57)
        {
            KB_ClearKeysDown();
            enteringText = 0;
            return (1);
        }
        else if(ch == asc_Escape)
        {
            return (-1);
        }
        else if ( ch >= 32 && textInputLoc < dalen && ch < 127)
        {
            ch = toupper(ch);
            *(t+textInputLoc) = ch;
            *(t+textInputLoc+1) = 0;
            textInputLoc++;
        }
    }
    x = gametext(x,y,t,c,2+8+16);
    c = 4-(sintable[(totalclock<<4)&2047]>>11);
    rotatesprite((x+8)<<16,(y+4)<<16,32768L,0,SPINNINGNUKEICON+((totalclock>>3)%7),c,0,2+8,0,0,xdim-1,ydim-1);
    return (0);
}

void menus(void)
{
    short c,x;
    volatile int32_t l;
	int i,j;

	static int lastkeysetup = 0;
	static int waiting4key = false;
    char text[128]; // should be enough.

    getpackets();

    if(((ControllerType == controltype_keyboardandmouse)||
		(ControllerType == controltype_joystickandmouse))
		//&& CONTROL_MousePresent
		)
    {

        if(buttonstat != 0 && !onbar) // anti-repeat
        {
            x = MOUSE_GetButtons()<<3;
            if( x )
			{
				buttonstat = x<<3;
			}
            else
			{
				buttonstat = 0;
			}
        }
        else

            buttonstat = MOUSE_GetButtons();
    }
    else buttonstat = 0;

    if( (ps[myconnectindex].gm&MODE_MENU) == 0 )
    {
        return;
    }

    ps[myconnectindex].gm &= (0xff-MODE_TYPE);
    ps[myconnectindex].fta = 0;

    x = 0;

    sh = 4-(sintable[(totalclock<<4)&2047]>>11);

    if(!(current_menu >= MENU_LOAD_GAME0 && current_menu <= 2999 && current_menu >= MENU_LOAD_GAME && current_menu <= MENU_ENTER_SAVE_GAME_NAME9))
        vscrn();

	// printf("Current menu=%d, game mode=%d, last menu =%d\n", current_menu, ps[myconnectindex].gm, last_menu);
  //  printf("Current Menu: %d\r\n", current_menu);

    switch(current_menu)
    {
        case MENU_MESSAGE_SELECT_SAVE_SPOT:
            gametext(160,90,"SELECT A SAVE SPOT BEFORE",0,2+8+16);
            gametext(160,90+9,"YOU QUICK RESTORE.",0,2+8+16);

            x = probe(186,124,0,0);
            if(x >= -1)
            {
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
                ps[myconnectindex].gm &= ~MODE_MENU;
            }
            break;

        case MENU_MESSAGE_SHAREWARE:
            x = probe(326,190,0,0);
            gametext(160,50-8,"YOU ARE PLAYING THE SHAREWARE",0,2+8+16);
            gametext(160,59-8,"VERSION OF DUKE NUKEM 3D.  WHILE",0,2+8+16);
            gametext(160,68-8,"THIS VERSION IS REALLY COOL, YOU",0,2+8+16);
            gametext(160,77-8,"ARE MISSING OVER 75% OF THE TOTAL",0,2+8+16);
            gametext(160,86-8,"GAME, ALONG WITH OTHER GREAT EXTRAS",0,2+8+16);
            gametext(160,95-8,"AND GAMES, WHICH YOU'LL GET WHEN",0,2+8+16);
            gametext(160,104-8,"YOU ORDER THE COMPLETE VERSION AND",0,2+8+16);
            gametext(160,113-8,"GET THE FINAL TWO EPISODES.",0,2+8+16);

            gametext(160,113+8,"PLEASE READ THE 'HOW TO ORDER' ITEM",0,2+8+16);
            gametext(160,122+8,"ON THE MAIN MENU IF YOU WISH TO",0,2+8+16);
            gametext(160,131+8,"UPGRADE TO THE FULL REGISTERED",0,2+8+16);
            gametext(160,140+8,"VERSION OF DUKE NUKEM 3D.",0,2+8+16);
            gametext(160,149+16,"PRESS ANY KEY...",0,2+8+16);

            if( x >= -1 ) cmenu(MENU_EPISODE_SELECT);
            break;


        case MENU_LOAD_LAST_GAME_QUESTION1:
        case MENU_LOAD_LAST_GAME_QUESTION0:

            gametext(160,90,"LOAD last game:",0,2+8+16);

            sprintf(text,"\"%s\"",ud.savegame[lastsavedpos]);
            gametext(160,99,text,0,2+8+16);

            gametext(160,99+9,"(Y/N)",0,2+8+16);
#ifdef WIN32
			_handle_events();
#endif
            if( KB_KeyPressed(sc_Escape) || KB_KeyPressed(sc_N) || RMB)
            {
                if(getSprite(ps[myconnectindex].i)->_extra <= 0)
                {
                    enterlevel(MODE_GAME);
                    return;
                }

                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);

                ps[myconnectindex].gm &= ~MODE_MENU;
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }

            if(  KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                KB_FlushKeyboardQueue();
                FX_StopAllSounds();

                if(ud.multimode > 1)
                {
                    loadplayer(-1-lastsavedpos);
                    ps[myconnectindex].gm = MODE_GAME;
                }
                else
                {
                    c = loadplayer(lastsavedpos);
                    if(c == 0)
                        ps[myconnectindex].gm = MODE_GAME;
                }
            }

            probe(186,124+9,0,0);

            break;

        case MENU_PARENTAL_LOCK:  // parental
        case MENU_PARENTAL_ENTER_PASSWORD:

            c = (320>>1)-120;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"ADULT MODE");

            x = probe(c+6,43,16,2);

            if(x == -1)
			{
				cmenu(MENU_GAME_OPTIONS);
				probey = 6;
				break;
			}

            menutext(c,43,SHX(-2),PHX(-2),"ADULT MODE");
			menutext(c+160+40,43,0,0,ud.lockout?"OFF":"ON");

			menutext(c,43+16,SHX(-3),PHX(-3),"ENTER PASSWORD");

            if(current_menu == MENU_PARENTAL_ENTER_PASSWORD)
            {
                gametext(160,43+16+16+16-12,"ENTER PASSWORD",0,2+8+16);
                x = strget((320>>1),43+16+16+16, (char *) buf,19, 998);

                if(x == 1) // user hit enter key
                {
    #if LETS_WASTE_RAM_FOR_USERDEFS

                    if(ud.pwlockout[0] == 0 || ud.lockout == 0 ) // if already unlocked then we set pwd or reset pwd is there is already one
                        strcpy(&ud.pwlockout[0],buf);
                    else if( strcmp(buf,&ud.pwlockout[0]) == 0 ) // if (pwd was up AND lockout is true (locked) AND pwd is good)
                    {
                        ud.lockout = 0;
                        buf[0] = 0;
                        int wallpicnum = getWall_picnum(animwall[x].wallnum);
                        for(x=0;x<numanimwalls;x++)
                            if( wallpicnum != W_SCREENBREAK &&
                                wallpicnum != W_SCREENBREAK+1 &&
                                wallpicnum != W_SCREENBREAK+2 )
                                    if( getWall_extra(animwall[x].wallnum) >= 0 )
                                    {
                                        void *dummy;
                                        dummy = setWall_picnum(animwall[x].wallnum, getWall_extra(animwall[x].wallnum));
                                    }

                    }
#endif
                    current_menu = MENU_PARENTAL_LOCK;
                    KB_ClearKeyDown(sc_Enter);
                    KB_ClearKeyDown(sc_kpad_Enter);
                    KB_FlushKeyboardQueue();
                }
				else if(x==-1) // pressed esc while typing. We discard the text.
				{
					*buf = 0;
					current_menu = MENU_PARENTAL_LOCK;
					KB_ClearKeyDown(sc_Escape);
				}
			}
            else
            {
                if(x == 0)
                {
#if LETS_WASTE_RAM_FOR_USERDEFS
                    if( ud.lockout == 1 )
                    {
                        if(ud.pwlockout[0] == 0)
                        {
                            ud.lockout = 0;
                            int wallpicnum = getWall_picnum(animwall[x].wallnum);
                            for(x=0;x<numanimwalls;x++)
                            if( wallpicnum != W_SCREENBREAK &&
                                wallpicnum != W_SCREENBREAK+1 &&
                                wallpicnum != W_SCREENBREAK+2 )
                                    if( getWall_extra(animwall[x].wallnum) >= 0 )
                                    {
                                        void *dummy;
                                        dummy = setWall_picnum(animwall[x].wallnum, getWall_extra(animwall[x].wallnum));
                                    }
                        }
                        else
                        {
                            buf[0] = 0;
                            current_menu = MENU_PARENTAL_ENTER_PASSWORD;
                            inputloc = 0;
                            KB_FlushKeyboardQueue();
                        }
                    }
                    else
                    {
                        ud.lockout = 1;

                        for(x=0;x<numanimwalls;x++)
                            switch(getWall_picnum(animwall[x].wallnum))
                            {
                                case FEMPIC1:
                                    {
                                        void *dummy;
                                        dummy = setWall_picnum(animwall[x].wallnum, BLANKSCREEN);
                                    }
                                    break;
                                case FEMPIC2:
                                case FEMPIC3:
                                    {
                                        void *dummy;
                                        dummy = setWall_picnum(animwall[x].wallnum, SCREENBREAK6);
                                    }
                                    break;
                            }
                    }
#endif
                }

                else if(x == 1)
                {
                    current_menu = MENU_PARENTAL_ENTER_PASSWORD; // ask for password change
                    inputloc = 0;
					*buf = 0;
                    KB_FlushKeyboardQueue();
                }
            }

            break;

        case MENU_LOAD_GAME0:
        case MENU_LOAD_GAME1:
        case MENU_LOAD_GAME2:
        case MENU_LOAD_GAME3:
        case MENU_LOAD_GAME4:
        case MENU_LOAD_GAME5:
        case MENU_LOAD_GAME6:
        case MENU_LOAD_GAME7:
        case MENU_LOAD_GAME8:
        case MENU_LOAD_GAME9:

            rotatesprite(160<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"LOAD GAME");
//            rotatesprite(101<<16,97<<16,65536,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);
            drawSavegameScreenshot();

            dispnames();

            sprintf((char*)text,"PLAYERS: %-2d                      ",numplr);
            gametext(160,158,(char*)text,0,2+8+16);

            sprintf((char*)text,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+volnum,1+levnum,plrskl);
            gametext(160,170,(char*)text,0,2+8+16);

            gametext(160,90,"LOAD game:",0,2+8+16);
            sprintf((char*)text,"\"%s\"",ud.savegame[current_menu-MENU_LOAD_GAME0]);
            gametext(160,99,(char*)text,0,2+8+16);
            gametext(160,99+9,"(Y/N)",0,2+8+16);
#ifdef WIN32
			_handle_events();
#endif
            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                lastsavedpos = current_menu-MENU_LOAD_GAME0;

                KB_FlushKeyboardQueue();
                //if(ud.multimode < 2 && ud.recstat != 2)
                //{
                //    ready2send = 1;
                //    totalclock = ototalclock;
                //}

                if(ud.multimode > 1)
                {
                    if( ps[myconnectindex].gm&MODE_GAME )
                    {
                        loadplayer(-1-lastsavedpos);
                        ps[myconnectindex].gm = MODE_GAME;
                    }
                    else
                    {
                        text[0] = 126;
                        text[1] = lastsavedpos;
                        //for(x=connecthead;x>=0;x=connectpoint2[x])
                            //    if(x != myconnectindex)
                                //        sendpacket(x,(uint8_t*)text,2);
                        broadcastPacket( (uint8_t*) text, 2, true);

                        getpackets();

                        loadplayer(lastsavedpos);

                        multiflag = 0;
                    }
                }
                else
                {
					// FIX_00084: Various bugs in the load game (single player) option if ESC is hit or if wrong version

                    c = loadplayer(lastsavedpos);
                    if(c == 0)
					{
                        ps[myconnectindex].gm = MODE_GAME;
						if (ud.recstat != 2) // if not playing a demo then ..
							totalclock = ototalclock;
					}
					else
						cmenu(MENU_CANNOT_LOAD_GAME_MESSAGE); // failed loading game
                }

                break;
            }

            if( KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);
                sound(EXITMENUSOUND);
				cmenu(MENU_LOAD_GAME);
				// FIX_00084: Various bugs in the load game (single player) option if ESC is hit or if wrong version
				// simply get back w/o changing anything.

            }

            break;

		case MENU_CANNOT_LOAD_GAME_MESSAGE: //loading a saved game failed

			gametext(160,99,"YOU CAN'T LOAD THIS GAME",0,2+8+16);
			gametext(160,99+9,"EITHER A WRONG VERSION",0,2+8+16);
			gametext(160,99+9+9,"OR BAD # OF PLAYERS OR...",0,2+8+16);
#ifdef WIN32
			_handle_events();
#endif
			if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Escape) || KB_KeyPressed(sc_Enter)
				|| RMB)            {
                KB_ClearKeyDown(sc_Space);
                KB_ClearKeyDown(sc_Escape);
				KB_ClearKeyDown(sc_Enter);
                sound(EXITMENUSOUND);
				cmenu(MENU_LOAD_GAME);
			}

			break;

        case MENU_ABORT_THIS_GAME_QUESTION:
#ifdef WIN32

			_handle_events();
#endif
			if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                KB_FlushKeyboardQueue();
                cmenu(MENU_EPISODE_SELECT);
            }
            if( KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
                ps[myconnectindex].gm &= ~MODE_MENU;
                sound(EXITMENUSOUND);
                break;
            }
            probe(186,124,0,0);
            gametext(160,90,"ABORT this game?",0,2+8+16);
            gametext(160,90+9,"(Y/N)",0,2+8+16);

            break;

        case MENU_OVERWRITE_SAVE_GAME0_QUESTION:
        case MENU_OVERWRITE_SAVE_GAME1_QUESTION:
        case MENU_OVERWRITE_SAVE_GAME2_QUESTION:
        case MENU_OVERWRITE_SAVE_GAME3_QUESTION:
        case MENU_OVERWRITE_SAVE_GAME4_QUESTION:
        case MENU_OVERWRITE_SAVE_GAME5_QUESTION:
        case MENU_OVERWRITE_SAVE_GAME6_QUESTION:
        case MENU_OVERWRITE_SAVE_GAME7_QUESTION:
        case MENU_OVERWRITE_SAVE_GAME8_QUESTION:
        case MENU_OVERWRITE_SAVE_GAME9_QUESTION:

            rotatesprite(160<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"SAVE GAME");

//            rotatesprite(101<<16,97<<16,65536L,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);
            drawSavegameScreenshot();
            sprintf(text,"PLAYERS: %-2d                      ",ud.multimode);
            gametext(160,158,text,0,2+8+16);

            sprintf(text,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
            gametext(160,170,text,0,2+8+16);

            dispnames();

            gametext(160,90,"OVERWRITE previous SAVED game?",0,2+8+16);
            gametext(160,90+9,"(Y/N)",0,2+8+16);
#ifdef WIN32
			_handle_events();
#endif
            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                KB_FlushKeyboardQueue();
                inputloc = strlen(&ud.savegame[current_menu-MENU_OVERWRITE_SAVE_GAME9_QUESTION][0]);

                cmenu(current_menu-MENU_OVERWRITE_SAVE_GAME9_QUESTION+MENU_ENTER_SAVE_GAME_NAME0);

                KB_FlushKeyboardQueue();
                break;
            }
            if( KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);
                cmenu(MENU_SAVE_GAME_SELECT);
                sound(EXITMENUSOUND);
            }

            probe(186,124,0,0);

            break;

        case MENU_CREDITS0:  // credits
        case MENU_CREDITS1:
        case MENU_CREDITS2:
        case MENU_CREDITS3:
        case MENU_CREDITS4:
        case MENU_CREDITS5:
        case MENU_CREDITS6:
        case MENU_CREDITS7:
			c = 160;
			if (!PLUTOPAK) {
            	//rotatesprite(c<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
            	rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            	menutext(c,24,0,0,"CREDITS");

				l = 7;
			} else {
				l = 2;
			}

            if(KB_KeyPressed(sc_Escape)) { cmenu(MENU_MAIN); break; }

            if( KB_KeyPressed( sc_LeftArrow ) ||
                KB_KeyPressed( sc_kpad_4 ) ||
                KB_KeyPressed( sc_UpArrow ) ||
                KB_KeyPressed( sc_PgUp ) ||
                KB_KeyPressed( sc_kpad_8 ) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                KB_ClearKeyDown(sc_UpArrow);
                KB_ClearKeyDown(sc_PgUp);
                KB_ClearKeyDown(sc_kpad_8);

                sound(KICK_HIT);
                current_menu--;
                if(current_menu < MENU_CREDITS0) current_menu = MENU_CREDITS0+l;
            }
            else if(
                KB_KeyPressed( sc_PgDn ) ||
                KB_KeyPressed( sc_Enter ) ||
                KB_KeyPressed( sc_Space ) ||
                KB_KeyPressed( sc_kpad_Enter ) ||
                KB_KeyPressed( sc_RightArrow ) ||
                KB_KeyPressed( sc_DownArrow ) ||
                KB_KeyPressed( sc_kpad_2 ) ||
                KB_KeyPressed( sc_kpad_9 ) ||
                KB_KeyPressed( sc_kpad_6 ) )
            {
                KB_ClearKeyDown(sc_PgDn);
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_ClearKeyDown(sc_kpad_6);
                KB_ClearKeyDown(sc_kpad_9);
                KB_ClearKeyDown(sc_kpad_2);
                KB_ClearKeyDown(sc_DownArrow);
                KB_ClearKeyDown(sc_Space);
                sound(KICK_HIT);
                current_menu++;
                if(current_menu > MENU_CREDITS0+l) current_menu = MENU_CREDITS0;
            }

			if (!PLUTOPAK || 1) { // This is Jonathon Fowler code. Text respects the true 1.3/1.3d --mk
			switch (current_menu) {
				case MENU_CREDITS0:
					gametext(c,40,                      "ORIGINAL CONCEPT",0,2+8+16);
					gametext(c,40+9,                    "TODD REPLOGLE",0,2+8+16);
					gametext(c,40+9+9,                  "ALLEN H. BLUM III",0,2+8+16);

					gametext(c,40+9+9+9+9,              "PRODUCED & DIRECTED BY",0,2+8+16);
					gametext(c,40+9+9+9+9+9,            "GREG MALONE",0,2+8+16);

					gametext(c,40+9+9+9+9+9+9+9,        "EXECUTIVE PRODUCER",0,2+8+16);
					gametext(c,40+9+9+9+9+9+9+9+9,      "GEORGE BROUSSARD",0,2+8+16);

					gametext(c,40+9+9+9+9+9+9+9+9+9+9,  "BUILD ENGINE",0,2+8+16);
					gametext(c,40+9+9+9+9+9+9+9+9+9+9+9,"KEN SILVERMAN",0,2+8+16);
					break;
				case MENU_CREDITS1:
					gametext(c,40,                      "GAME PROGRAMMING",0,2+8+16);
					gametext(c,40+9,                    "TODD REPLOGLE",0,2+8+16);

					gametext(c,40+9+9+9,                "3D ENGINE/TOOLS/NET",0,2+8+16);
					gametext(c,40+9+9+9+9,              "KEN SILVERMAN",0,2+8+16);

					gametext(c,40+9+9+9+9+9+9,          "NETWORK LAYER/SETUP PROGRAM",0,2+8+16);
					gametext(c,40+9+9+9+9+9+9+9,        "MARK DOCHTERMANN",0,2+8+16);
					break;
				case MENU_CREDITS2:
					gametext(c,40,                      "MAP DESIGN",0,2+8+16);
					gametext(c,40+9,                    "ALLEN H BLUM III",0,2+8+16);
					gametext(c,40+9+9,                  "RICHARD GRAY",0,2+8+16);

					gametext(c,40+9+9+9+9,              "3D MODELING",0,2+8+16);
					gametext(c,40+9+9+9+9+9,            "CHUCK JONES",0,2+8+16);
					gametext(c,40+9+9+9+9+9+9,          "SAPPHIRE CORPORATION",0,2+8+16);

					gametext(c,40+9+9+9+9+9+9+9+9,      "ARTWORK",0,2+8+16);
					gametext(c,40+9+9+9+9+9+9+9+9+9,    "DIRK JONES, STEPHEN HORNBACK",0,2+8+16);
					gametext(c,40+9+9+9+9+9+9+9+9+9+9,  "JAMES STOREY, DAVID DEMARET",0,2+8+16);
					gametext(c,40+9+9+9+9+9+9+9+9+9+9+9,"DOUGLAS R WOOD",0,2+8+16);
					break;
				case MENU_CREDITS3:
					gametext(c,40,                      "SOUND ENGINE",0,2+8+16);
					gametext(c,40+9,                    "JIM DOSE",0,2+8+16);

					gametext(c,40+9+9+9,                "SOUND & MUSIC DEVELOPMENT",0,2+8+16);
					gametext(c,40+9+9+9+9,              "ROBERT PRINCE",0,2+8+16);
					gametext(c,40+9+9+9+9+9,            "LEE JACKSON",0,2+8+16);

					gametext(c,40+9+9+9+9+9+9+9,        "VOICE TALENT",0,2+8+16);
					gametext(c,40+9+9+9+9+9+9+9+9,      "LANI MINELLA - VOICE PRODUCER",0,2+8+16);
					gametext(c,40+9+9+9+9+9+9+9+9+9,    "JON ST. JOHN AS \"DUKE NUKEM\"",0,2+8+16);
					break;
				case MENU_CREDITS4:
					gametext(c,60,                      "GRAPHIC DESIGN",0,2+8+16);
					gametext(c,60+9,                    "PACKAGING, MANUAL, ADS",0,2+8+16);
					gametext(c,60+9+9,                  "ROBERT M. ATKINS",0,2+8+16);
					gametext(c,60+9+9+9,                "MICHAEL HADWIN",0,2+8+16);

					gametext(c,60+9+9+9+9+9,            "SPECIAL THANKS TO",0,2+8+16);
					gametext(c,60+9+9+9+9+9+9,          "STEVEN BLACKBURN, TOM HALL",0,2+8+16);
					gametext(c,60+9+9+9+9+9+9+9,        "SCOTT MILLER, JOE SIEGLER",0,2+8+16);
					gametext(c,60+9+9+9+9+9+9+9+9,      "TERRY NAGY, COLLEEN COMPTON",0,2+8+16);
					gametext(c,60+9+9+9+9+9+9+9+9+9,    "HASH INC., FORMGEN, INC.",0,2+8+16);
					break;
				case MENU_CREDITS5:
					gametext(c,49,                      "THE 3D REALMS BETA TESTERS",0,2+8+16);

					gametext(c,49+9+9,                  "NATHAN ANDERSON, WAYNE BENNER",0,2+8+16);
					gametext(c,49+9+9+9,                "GLENN BRENSINGER, ROB BROWN",0,2+8+16);
					gametext(c,49+9+9+9+9,              "ERIK HARRIS, KEN HECKBERT",0,2+8+16);
					gametext(c,49+9+9+9+9+9,            "TERRY HERRIN, GREG HIVELY",0,2+8+16);
					gametext(c,49+9+9+9+9+9+9,          "HANK LEUKART, ERIC BAKER",0,2+8+16);
					gametext(c,49+9+9+9+9+9+9+9,        "JEFF RAUSCH, KELLY ROGERS",0,2+8+16);
					gametext(c,49+9+9+9+9+9+9+9+9,      "MIKE DUNCAN, DOUG HOWELL",0,2+8+16);
					gametext(c,49+9+9+9+9+9+9+9+9+9,    "BILL BLAIR",0,2+8+16);
					break;
				case MENU_CREDITS6:
					gametext(c,32,                      "COMPANY PRODUCT SUPPORT",0,2+8+16);

					gametext(c,32+9+9,                  "THE FOLLOWING COMPANIES WERE COOL",0,2+8+16);
					gametext(c,32+9+9+9,                "ENOUGH TO GIVE US LOTS OF STUFF",0,2+8+16);
					gametext(c,32+9+9+9+9,              "DURING THE MAKING OF DUKE NUKEM 3D.",0,2+8+16);

					gametext(c,32+9+9+9+9+9+9,          "ALTEC LANSING MULTIMEDIA",0,2+8+16);
					gametext(c,32+9+9+9+9+9+9+9,        "FOR TONS OF SPEAKERS AND THE",0,2+8+16);
					gametext(c,32+9+9+9+9+9+9+9+9,      "THX-LICENSED SOUND SYSTEM",0,2+8+16);
					gametext(c,32+9+9+9+9+9+9+9+9+9,    "FOR INFO CALL 1-800-548-0620",0,2+8+16);

					gametext(c,32+9+9+9+9+9+9+9+9+9+9+9,"CREATIVE LABS, INC.",0,2+8+16);

					gametext(c,32+9+9+9+9+9+9+9+9+9+9+9+9+9,"THANKS FOR THE HARDWARE, GUYS.",0,2+8+16);
					break;
				case MENU_CREDITS7:
					gametext(c,50,                      "DUKE NUKEM IS A TRADEMARK OF",0,2+8+16);
					gametext(c,50+9,                    "3D REALMS ENTERTAINMENT",0,2+8+16);

					gametext(c,50+9+9+9,                "DUKE NUKEM",0,2+8+16);
					gametext(c,50+9+9+9+9,              "(C) 1996 3D REALMS ENTERTAINMENT",0,2+8+16);

					if (VOLUMEONE) {
					gametext(c,106,                     "PLEASE READ LICENSE.DOC FOR SHAREWARE",0,2+8+16);
					gametext(c,106+9,                   "DISTRIBUTION GRANTS AND RESTRICTIONS",0,2+8+16);
					}

					gametext(c,VOLUMEONE?134:115,       "MADE IN DALLAS, TEXAS USA",0,2+8+16);
					break;
			}
			}

            switch(current_menu)
            {
                case MENU_CREDITS0:
                case MENU_CREDITS2:
                case MENU_CREDITS3:
                   rotatesprite(160<<16,200<<15,65536L,0,2504+current_menu-MENU_CREDITS0,0,0,10+64,0,0,xdim-1,ydim-1);

                   break;

            }
            break;

        case MENU_MAIN: // main menu
            c = (320>>1);
            rotatesprite(c<<16,28<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
            rotatesprite((c+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8,0,0,xdim-1,ydim-1);

			x = probe(c,67,16,7);

            if(x >= 0)
            {
				if( ud.multimode > 1 && x == 0 && ud.recstat != 2)
                {
                    if( movesperpacket == 4 && myconnectindex != connecthead )
                        break;

                    last_zero = 0;
                    cmenu(MENU_MULTI_HOST_SETUP);
                }
                else
                {
                    last_zero = x;
                    switch(x)
                    {
                        case 0:
                            cmenu(MENU_EPISODE_SELECT);
                            break;
                        case 1: cmenu(MENU_OPTIONS);break;
                        case 2:
                            if ((movesperpacket == 4 && connecthead != myconnectindex) || numplayers > 1)
                                break;
                            cmenu(MENU_LOAD_GAME);
                            break;
                        case 3: KB_FlushKeyboardQueue();cmenu(MENU_HELP0);break;  // help
                        case 4: cmenu(MENU_CREDITS0);break;  // credit
                        case 5: cmenu(MENU_QUIT);break;  // quit
                        case 6: cmenu(MENU_MULTI_SETUP); break;

                    }
                }
            }

            if(KB_KeyPressed(sc_Q)) cmenu(MENU_QUIT);

            if(x == -1)
            {
				// FIX_00069: Hitting Esc at the menu screen shows an empty green screen
				if(ud.recstat == 2) // playing demo
					ps[myconnectindex].gm &= ~MODE_MENU;
            }

            if(movesperpacket == 4)
            {
                if( myconnectindex == connecthead )
                    menutext(c,67,SHX(-2),PHX(-2),"NEW GAME");
                else
                    menutext(c,67,SHX(-2),1,"NEW GAME");
            }
            else
                menutext(c,67,SHX(-2),PHX(-2),"NEW GAME");

            menutext(c,67+16,SHX(-3),PHX(-3),"OPTIONS");

            if(movesperpacket == 4 && connecthead != myconnectindex)
                menutext(c,67+16+16,SHX(-4),1,"LOAD GAME");
            else menutext(c,67+16+16,SHX(-4),numplayers > 1,"LOAD GAME");

            if(VOLUMEONE)
            menutext(c,67+16+16+16,SHX(-5),PHX(-5),"HOW TO ORDER");
            else
            {
            menutext(c,67+16+16+16,SHX(-5),PHX(-5),"HELP");
            }

            menutext(c,67+16+16+16+16,SHX(-6),PHX(-6),"CREDITS");

            menutext(c,67+16+16+16+16+16,SHX(-7),PHX(-7),"QUIT");

            menutext(c,67+16+16+16+16+16+16,SHX(-8),PHX(-8),"MULTIPLAYER");


            break;
// CTW END - MODIFICATION

        case MENU_INGAME_MENU: // general menu as cmenu(MENU_MAIN) but for multiplayer games
            c = (320>>1);
            rotatesprite(c<<16,32<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
            rotatesprite((c+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8,0,0,xdim-1,ydim-1);
            x = probe(c,67,16,7);
            switch(x)
            {
                case 0:
                    if(movesperpacket == 4 && myconnectindex != connecthead)
                        break;
                    if(ud.multimode < 2 || ud.recstat == 2)
                        cmenu(MENU_ABORT_THIS_GAME_QUESTION);
                    else
                    {
                        cmenu(MENU_MULTI_HOST_SETUP);
                        last_fifty = 0;
                    }
                    break;
                case 1:
                    if((movesperpacket == 4 && connecthead != myconnectindex) || numplayers > 1)
                        break;
                    if(ud.recstat != 2)
                    {
                        last_fifty = 1;
                        cmenu(MENU_PRE_SAVE_GAME);
                        setview(0,0,xdim-1,ydim-1);
                    }
                    break;
                case 2:
                    if((movesperpacket == 4 && connecthead != myconnectindex) || numplayers > 1)
                        break;
                    last_fifty = 2;
                    cmenu(MENU_LOAD_GAME);
                    break;
                case 3:
                    last_fifty = 3;
                    cmenu(MENU_OPTIONS);
                    break;
                case 4:
                    last_fifty = 4;
                    KB_FlushKeyboardQueue();
                    cmenu(MENU_HELP0);
                    break;
                case 5:
                    if(numplayers < 2)
                    {
                        last_fifty = 5;
                        cmenu(MENU_QUIT_TO_TITLE);
                    }
                    break;
                case 6:
                    last_fifty = 6;
                    cmenu(MENU_QUIT_GAME);
                    break;
                case -1:
                    ps[myconnectindex].gm &= ~MODE_MENU;
                    if(ud.multimode < 2 && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                    break;
            }

            if( KB_KeyPressed(sc_Q) )
                cmenu(MENU_QUIT_GAME);

            if(movesperpacket == 4 && connecthead != myconnectindex)
            {
                menutext(c,67+16*0 ,SHX(-2),1,"NEW GAME");
                menutext(c,67+16*1 ,SHX(-3),1,"SAVE GAME");
                menutext(c,67+16*2 ,SHX(-4),1,"LOAD GAME");
            }
            else
            {
                menutext(c,67+16*0 ,SHX(-2),PHX(-2),"NEW GAME");
                menutext(c,67+16*1 ,SHX(-3), numplayers > 1 ,"SAVE GAME");
                menutext(c,67+16*2 ,SHX(-4), numplayers > 1,"LOAD GAME");
            }

            menutext(c,67+16*3 ,SHX(-5),PHX(-5),"OPTIONS");
            if(VOLUMEONE)
            menutext(c,67+16*4 ,SHX(-6),PHX(-6),"HOW TO ORDER");
            else
            {
            menutext(c,67+16*4 ,SHX(-6),PHX(-6)," HELP");
            }
            if(numplayers > 1)
                menutext(c,67+16*5 ,SHX(-7),1,"QUIT TO TITLE");
            else menutext(c,67+16*5 ,SHX(-7),PHX(-7),"QUIT TO TITLE");
            menutext(c,67+16*6,SHX(-8),PHX(-8),"QUIT GAME");

            break;

        case MENU_EPISODE_SELECT: // Title menu
            rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"SELECT AN EPISODE");
            if(ATOMIC_EDITION)
            {            //////if(boardfilename[0])
            //
			// uncomment this for user map
			//x = probe(160,60,20,5);
			x = probe(160,60,20,4);

            //////else x = probe(160,60,20,4);
            }
            else
            {
            if(boardfilename[0])
                x = probe(160,60,20,4);
            else x = probe(160,60,20,3);
            }
            if(x >= 0)
            {

				if (VOLUMEONE)
				{
					if(x > 0)
						cmenu(MENU_MESSAGE_SHAREWARE);
					else
					{
						ud.m_volume_number = x;
						ud.m_level_number = 0;
						cmenu(MENU_SKILL_SELECT);
				    }
				}

				else
				{

					if((x == 3 && boardfilename[0])&&!ATOMIC_EDITION)
					{
					    ud.m_volume_number = 0;
					    ud.m_level_number = 7;
					}
					else

					/*
					if(x == 4 && boardfilename[0])
					{
						ud.m_volume_number = 0;
						ud.m_level_number = 7;
					}
					*/

					// USER MAP
					if((x == 4)&&PLUTOPAK)
					{
						//CONSOLE_Printf("MENU_USER_MAP");
						//
                    //[Todo: generate file list starting from .\\maps]")

						cmenu(MENU_USER_MAP);
						break;
					}
					else
					{
						ud.m_volume_number = x;
						ud.m_level_number = 0;
					}
					cmenu(MENU_SKILL_SELECT);
				}
            }
            else if(x == -1)
            {
                if(ps[myconnectindex].gm&MODE_GAME) cmenu(MENU_INGAME_MENU);
                else cmenu(MENU_MAIN);
            }

            menutext(160,60,SHX(-2),PHX(-2),volume_names[0]);

            c = 80;
			if (VOLUMEONE)
			{
				menutext(160,60+20,SHX(-3),1,volume_names[1]);
	            menutext(160,60+20+20,SHX(-4),1,volume_names[2]);
				if(ATOMIC_EDITION)
					menutext(160,60+20+20,SHX(-5),1,volume_names[3]);
			}
			else
			{
				menutext(160,60+20,SHX(-3),PHX(-3),volume_names[1]);
				menutext(160,60+20+20,SHX(-4),PHX(-4),volume_names[2]);
				if(ATOMIC_EDITION)
				{
					menutext(160,60+20+20+20,SHX(-5),PHX(-5),volume_names[3]);
					//if(boardfilename[0])
					//{

					// uncomment this for usermap
					//menutext(160,60+20+20+20+20,SHX(-6),PHX(-6),"USER MAP");

					//gametextpal(160,60+20+20+20+20+3,boardfilename,16+(sintable[(totalclock<<4)&2047]>>11),2);
					//}
				}
				else
				{	/*
		            if(boardfilename[0])
				    {
						menutext(160,60+20+20+20,SHX(-6),PHX(-6),"USER MAP");
						gametext(160,60+20+20+20+6,boardfilename,2,2+8+16);
					}
					*/
				}
			}
            break;

		case MENU_USER_MAP: // MENU_USER_MAP
	            c = (320>>1);
				rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
				menutext(c,24,0,0,"USER MAP");

				// Draw USER MAP background
				{
					int y, x1;
					int32_t xPos, xPos2;
					int32_t yPos; //yPos2;

					xPos =  ( xdim *32) / 320;
					yPos =  ( ydim *30) / 200;

					xPos2 =  ( xdim *282) / 320;
					//yPos2 =  ( ydim *130) / 200;

					for(y=yPos; y < (ydim - (yPos*2)); y+=128)
					{
						for(x1=xPos; x1 < xPos2; x1+=128)
						{
							rotatesprite(x1<<16,y<<16,65536L,0,BIGHOLE,8,0,1+8+16+64+128,0,0, xdim - xPos, ydim - (yPos*2));
						}
					}
				}

				c = (320>>1)-120;
				x = probe(c,70,19,4);

				if(x == -1)
				{
					cmenu(MENU_EPISODE_SELECT);
				}
			break;

        case MENU_SKILL_SELECT:
            c = (320>>1);
            rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(c,24,0,0,"SELECT SKILL");
            x = probe(c,70,19,4);
            if(x >= 0)
            {
#if 0
                switch(x)
                {
                    case 0: globalskillsound = JIBBED_ACTOR6;break;
                    case 1: globalskillsound = BONUS_SPEECH1;break;
                    case 2: globalskillsound = DUKE_GETWEAPON2;break;
                    case 3: globalskillsound = JIBBED_ACTOR5;break;
                }

                sound(globalskillsound);
#else
                int sampleToPlay = 0;
                switch(x)
                {
                    case 0: sampleToPlay = JIBBED_ACTOR6;break;
                    case 1: sampleToPlay = BONUS_SPEECH1;break;
                    case 2: sampleToPlay = DUKE_GETWEAPON2;break;
                    case 3: sampleToPlay = JIBBED_ACTOR5;break;
                }

                globalSkillVoice = sound(sampleToPlay);

#endif
                ud.m_player_skill = x+1;
                if(x == 3) ud.m_respawn_monsters = 1;
                else ud.m_respawn_monsters = 0;

                ud.m_monsters_off = ud.monsters_off = 0;

                ud.m_respawn_items = 0;
                ud.m_respawn_inventory = 0;

                ud.multimode = 1;

				// if (ud.showcinematics)
                //if(ud.m_volume_number == 3) // not needed to play cinematics. Black screen not nice
                //{
                //    flushperms();
                //    setview(0,0,xdim-1,ydim-1);
                //    clearview(0L);
                //    nextpage(SCREEN_HEIGHT);
                //}

                newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
                enterlevel(MODE_GAME);
            }
            else if(x == -1)
            {
                cmenu(MENU_EPISODE_SELECT);
                KB_FlushKeyboardQueue();
            }

            menutext(c,70,SHX(-2),PHX(-2),skill_names[0]);
            menutext(c,70+19,SHX(-3),PHX(-3),skill_names[1]);
            menutext(c,70+19+19,SHX(-4),PHX(-4),skill_names[2]);
            menutext(c,70+19+19+19,SHX(-5),PHX(-5),skill_names[3]);
            break;
        case MENU_OPTIONS_SAVED:
            gametext(160,43+16+16+16+16+16+16+16,"Options saved",0,2+8+16);

        case MENU_OPTIONS:

            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"OPTIONS");

            c = (320>>1)-120;

            x = probe(c+6,43,16,7);

            if(x == -1)
                { if(ps[myconnectindex].gm&MODE_GAME) cmenu(MENU_INGAME_MENU);else cmenu(MENU_MAIN); }

            switch(x)
            {
				case 0:
					cmenu(MENU_GAME_OPTIONS); // game options
					break;

                case 1:
                    cmenu(MENU_KEYBOARD_SETUP); // keybaord setup
					probey = 7;
                    break;

                case 2:
                    cmenu(MENU_STICK_SETUP); // mouse setup
                    break;

				case 3:
                    cmenu(MENU_SOUND_SETUP);  // sound setup
                    break;

				case 4:
					cmenu(MENU_VIDEO_SETUP); // Video setup
					lastkeysetup = 0;
#if USE_MANY_MODES
					for(i=0; i<validmodecnt; i++)
					{
						if(validmodexdim[i] == xdim && validmodeydim[i] == ydim)
							current_resolution = i;
					}
#endif
					break;

                case 5: // record on/off
#if DEMO_RECORD_SUPPORT
                    if( (ps[myconnectindex].gm&MODE_GAME) )
                    {
                        closedemowrite();
                        break;
                    }
                    ud.m_recstat = !ud.m_recstat;
#endif
                    break;
                case 6:
                    cmenu(MENU_SAVE_OPTIONS_CONFIRM);
                    break;
				//case -7:
				//	gametext(320>>1,43+16*6,"*** DISABLED. WILL BE FIXED SOON***",0,2+8+16); // center-i
				//	break;

			}

			menutext(c,43,SHX(-6),PHX(-6),"GAME OPTIONS");

			menutext(c,43+16,SHX(-6),PHX(-6),"SETUP KEYBOARD");

			menutext(c,43+16+16,SHX(-6),PHX(-6),"SETUP ANALOG STICKS");

            menutext(c,43+16+16+16,SHX(-8),PHX(-8),"SETUP SOUND");

            menutext(c,43+16+16+16+16,SHX(-8),PHX(-8),"SETUP VIDEO");
#if DEMO_RECORD_SUPPORT

            if( (ps[myconnectindex].gm&MODE_GAME) && ud.m_recstat != 1 )
            {
                menutext(c,43+16+16+16+16+16,SHX(-10),1,"RECORD");
                menutext(c+160+40,43+16+16+16+16+16,SHX(-10),1,"OFF");
            }
            else
            {
                menutext(c,43+16+16+16+16+16,SHX(-10),PHX(-10),"RECORD");

                if(ud.m_recstat == 1)
                    menutext(c+160+40,43+16+16+16+16+16,SHX(-10),PHX(-10),"ON");
                else menutext(c+160+40,43+16+16+16+16+16,SHX(-10),PHX(-10),"OFF");
            }
#else
                menutext(c,43+16+16+16+16+16,SHX(-10),1,"RECORD");
                menutext(c+160+40,43+16+16+16+16+16,SHX(-10),1,"OFF");
#endif
            menutext(c,43+16+16+16+16+16+16,SHX(-8),PHX(-8),"SAVE OPTIONS");

            break;
        case MENU_SAVE_OPTIONS_CONFIRM:

            gametext(160,90,"Confirm save options?",0,2+8+16);
            gametext(160,90+9,"(Y/N)",0,2+8+16);
#ifdef WIN32
			_handle_events();
#endif
            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_Y))
            {
                saveOptions();
                sound(PISTOL_BODYHIT);
                KB_FlushKeyboardQueue();
                cmenu(MENU_OPTIONS_SAVED);
                break;
            }
            if( KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                KB_ClearKeyDown(sc_Escape);
                sound(EXITMENUSOUND);
                KB_FlushKeyboardQueue();
                cmenu(MENU_OPTIONS);
            }
            break;
        case MENU_SOUND_SETUP:

            c = (320>>1)-120;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"SETUP SOUNDS");
            onbar = ((probey == 2)&&SoundToggle) || ((probey == 3)&&MusicToggle) ;

            x = probe(c+6,43,16,8);

            switch(x)
            {
                case -1:
					cmenu(MENU_OPTIONS);
					probey = 3;
                    break;

                case 0:
                    if (FXDevice != SC_Unknown)
                    {
                        SoundToggle = 1-SoundToggle;
                        if( SoundToggle == 0 )
                        {
                            FX_StopAllSounds();
                            clearsoundlocks();
                        }
                    }
                    break;

				case 1:

                    if(MusicDevice != SC_Unknown)
                    {
                        MusicToggle = 1-MusicToggle;
                        if( MusicToggle == 0 )
							MUSIC_StopSong();
                        else
                        {
                            if(ud.recstat != 2 && ps[myconnectindex].gm&MODE_GAME)
                                playmusic(&music_fn[0][music_select][0]);
                            else playmusic(&env_music_fn[0][0]);

                            MUSIC_Continue();
                        }
                    }
                    break;

                case 4:
                    if(SoundToggle && (FXDevice != SC_Unknown)) VoiceToggle = 1-VoiceToggle;
                    break;

				case 5:
                    if(SoundToggle && (FXDevice != SC_Unknown)) AmbienceToggle = 1-AmbienceToggle;
                    break;

				case 6:
                    if(SoundToggle && (FXDevice != SC_Unknown))
                    {
                        ReverseStereo = 1-ReverseStereo;
                        FX_SetReverseStereo(ReverseStereo);
					}
                    break;

				case 7: // xduke: 1.3d sound style - hear opponent
					if(SoundToggle && (FXDevice != SC_Unknown))
                    {
                        OpponentSoundToggle = 1-OpponentSoundToggle;
                    }
					break;

                default:
                    break;
            }

            if(SoundToggle && FXDevice != SC_Unknown)
                menutext(c+160+40,43,0,(FXDevice == SC_Unknown),"ON");
            else
                menutext(c+160+40,43,0,(FXDevice == SC_Unknown),"OFF");

            if(MusicToggle && MusicDevice != SC_Unknown)
                menutext(c+160+40,43+16,0,(MusicDevice == SC_Unknown),"ON");
            else
                menutext(c+160+40,43+16,0,(MusicDevice == SC_Unknown),"OFF");

            menutext(c,43,SHX(-2),(FXDevice == SC_Unknown),"SOUND");
            menutext(c,43+16+16,SHX(-4),(FXDevice==SC_Unknown)||SoundToggle==0,"SOUND VOLUME");
            {
                l = FXVolume;
                FXVolume >>= 2;
                bar(c+167+40,43+16+16,(short *)&FXVolume,4,(FXDevice!=SC_Unknown)&&x==2,SHX(-4),SoundToggle==0||(FXDevice==SC_Unknown));
                if(l != FXVolume)
                    FXVolume <<= 2;
                if(l != FXVolume)
                    FX_SetVolume( (short) FXVolume );
            }
            menutext(c,43+16,SHX(-3),(MusicDevice==SC_Unknown),"MUSIC");
            menutext(c,43+16+16+16,SHX(-5),(MusicDevice==SC_Unknown)||MusicToggle==0,"MUSIC VOLUME");
            {
                l = MusicVolume;
                MusicVolume >>= 2;
                bar(c+167+40,43+16+16+16,
                    (short *)&MusicVolume,4,
                    (MusicDevice!=SC_Unknown) && x==3,SHX(-5),
                    (MusicToggle==0)||(MusicDevice==SC_Unknown));
                MusicVolume <<= 2;
                if(l != MusicVolume)
                {
#ifdef WIN32
                    STUBBED("Check this");
#endif
                    // !!! FIXME: Used to be Music_ not MUSIC_.  --ryan.
                    MUSIC_SetVolume( (short) MusicVolume );
                }
            }
            menutext(c,43+16+16+16+16,SHX(-6),(FXDevice==SC_Unknown)||SoundToggle==0,"DUKE TALK");
            menutext(c,43+16+16+16+16+16,SHX(-7),(FXDevice==SC_Unknown)||SoundToggle==0,"AMBIENCE");

            menutext(c,43+16+16+16+16+16+16,SHX(-8),(FXDevice==SC_Unknown)||SoundToggle==0,"FLIP STEREO");

            if(VoiceToggle) menutext(c+160+40,43+16+16+16+16,0,(FXDevice==SC_Unknown)||SoundToggle==0,"ON");
            else menutext(c+160+40,43+16+16+16+16,0,(FXDevice==SC_Unknown)||SoundToggle==0,"OFF");

            if(AmbienceToggle) menutext(c+160+40,43+16+16+16+16+16,0,(FXDevice==SC_Unknown)||SoundToggle==0,"ON");
            else menutext(c+160+40,43+16+16+16+16+16,0,(FXDevice==SC_Unknown)||SoundToggle==0,"OFF");

            if(ReverseStereo) menutext(c+160+40,43+16+16+16+16+16+16,0,(FXDevice==SC_Unknown)||SoundToggle==0,"ON");
            else menutext(c+160+40,43+16+16+16+16+16+16,0,(FXDevice==SC_Unknown)||SoundToggle==0,"OFF");

            menutext(c,43+16+16+16+16+16+16+16,SHX(-9),(FXDevice==SC_Unknown)||SoundToggle==0,"OPPONENT SOUND");
			if(OpponentSoundToggle) menutext(c+160+40,43+16+16+16+16+16+16+16,0,(FXDevice==SC_Unknown)||SoundToggle==0,"ON");
			else menutext(c+160+40,43+16+16+16+16+16+16+16,0,(FXDevice==SC_Unknown)||SoundToggle==0,"OFF");

            break;

        case MENU_STICK_SETUP:
            c = (320>>1)-120;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"SETUP ANALOG STICKS");
            onbar = ( probey == 0 || probey == 1 || probey == 2 || probey == 3);

            x = probe(c+6,43,16,8);
            switch(x)
            {
//				case -9:
//					gametext(320>>1,43+16*7+3,"*** SHORTCUT: ALT-M ***",0,2+8+16); // center-i
//					break;

                case -1:
					cmenu(MENU_OPTIONS);
					probey = 2;
                    break;

                case 0:
				case 1:
                case 2:
                case 3:
					break;
				case 4:

 					if ( ((ControllerType == controltype_keyboardandmouse)||
						(ControllerType == controltype_joystickandmouse)) ||
 					   (ControllerType == controltype_keyboardandjoystick))
					{
						MouseAiming = !MouseAiming;
						if(MouseAiming)
							myaimmode = 0;

					}
                    break;

				case 5:

                    if ( ((ControllerType == controltype_keyboardandmouse)||
                        (ControllerType == controltype_joystickandmouse)) ||
                       (ControllerType == controltype_keyboardandjoystick))
					{
						if(!MouseAiming) // means we are in toggle mode
							myaimmode = !myaimmode;
					}
                    break;

                case 6:

                    if ( ((ControllerType == controltype_keyboardandmouse)||
                        (ControllerType == controltype_joystickandmouse)) ||
                       (ControllerType == controltype_keyboardandjoystick))
					{
						ud.mouseflip = 1-ud.mouseflip;
					}
                    break;
#if 0
				case 7:
#ifdef WIN32
					if (SDL_GetRelativeMouseMode() == SDL_TRUE)
					{
                        SDL_SetRelativeMouseMode(SDL_FALSE);
						//SDL_ShowCursor(1);
					}
					else
					{
                        SDL_SetRelativeMouseMode(SDL_TRUE);
						//SDL_ShowCursor(0);
					}
#endif
					break;

				case 8:
					cmenu(MENU_MOUSE_BUTTON_SETUP); // Button setup
                    break;

				case 9:
					cmenu(MENU_ANALOG_AXES_SETUP); // Digital axes setup
                    break;
#else
				case 7:
					cmenu(MENU_ANALOG_AXES_SETUP); // Digital axes setup
                    break;

#endif
                default:
                    break;
            }

			{
			short sense;
            for (int i = 0; i < max_axis; i++)
            {
                sense = CONTROL_GetAnalogSensitivity(i);
                snprintf(text, sizeof(text), "%c-%c SENSITIVTY", i & 2 ? 'R' : 'L',i & 1 ? 'Y' : 'X');
                menutext(c,43+16*i,SHX(-7),PHX(-7),text);
                bar(c+167+40,43+16*i,&sense,1,x==i,SHX(-7),PHX(-7));
                CONTROL_SetAnalogSensitivity(i,  sense );
            }
            //
			menutext(c,43+16*4,SHX(-7),PHX(-7),"AIM TYPE");
			if(MouseAiming) menutext(c+160+40,43+16*4,SHX(-7),PHX(-7),"HELD");
			else menutext(c+160+40,43+16*4,SHX(-7),PHX(-7),"TOGGLE");

			menutext(c,43+16*5,SHX(-7),MouseAiming,"THUMBSTICK AIMING");
			if(myaimmode) menutext(c+160+40,43+16*5,SHX(-7),MouseAiming,"ON");
			else menutext(c+160+40,43+16*5,SHX(-7),MouseAiming,"OFF");

			menutext(c,43+16*6,SHX(-7),PHX(-7),"AIMING FLIP");
			if(ud.mouseflip) menutext(c+160+40,43+16*6,SHX(-7),PHX(-7),"ON");
			else menutext(c+160+40,43+16*6,SHX(-7),PHX(-7),"OFF");

#if 0
			menutext(c,43+16*7,SHX(-7),PHX(-7),"MOUSE CURSOR");
			if(SDL_GetRelativeMouseMode() == SDL_TRUE)
				menutext(c+160+40,43+16*7,SHX(-7),PHX(-7),"TAKEN");
			else
				menutext(c+160+40,43+16*7,SHX(-7),PHX(-7),"FREE'D");
			menutext(c,43+16*8,SHX(-7),PHX(-7),"BUTTON SETUP...");
#endif
			menutext(c,43+16*7,SHX(-7),PHX(-7),"ANALOG AXES SETUP...");
			}

            break;

        case MENU_GAME_OPTIONS:
            c = (320>>1)-120;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"GAME OPTIONS");

            onbar = 0;

			x = probe(c+6,43,16,7);

            switch(x)
            {

                case -1:
					cmenu(MENU_OPTIONS);
                    break;

                case 0:
                    ud.shadows = 1-ud.shadows;
                    break;
                case 1:
                    ud.screen_tilting = 1-ud.screen_tilting;
                    break;
				case 2:
					ud.showcinematics = !ud.showcinematics;
					break;
				case 3:
					ud.hideweapon = !ud.hideweapon;
					vscrn(); // FIX_00056: Refresh issue w/FPS, small Weapon and custom FTA, when screen resized down
					break;
				case 4:
					ud.weaponautoswitch = !ud.weaponautoswitch;
					break;
				case 5:
					// FIX_00045: Autoaim mode can now be toggled on/off from menu
					if( nHostForceDisableAutoaim == 0)
					{
						ud.auto_aim++;
						ud.auto_aim = ((ud.auto_aim-1)%2)+1; // 2 = normal = full; 1 = bullet only
					}
					break;
                case 6: // parental
#ifndef AUSTRALIA
                    cmenu(MENU_PARENTAL_LOCK);
#endif
                    break;

			}


			menutext(c,43+16*0,SHX(-3),PHX(-3),"SHADOWS");
            if(ud.shadows) menutext(c+160+40,43+16*0,0,0,"ON");
            else menutext(c+160+40,43+16*0,0,0,"OFF");

			menutext(c,43+16*1,SHX(-4),PHX(-4),"SCREEN TILTING");
            switch(ud.screen_tilting)
            {
                case 0: menutext(c+160+40,43+16*1,0,0,"OFF");break;
                case 1: menutext(c+160+40,43+16*1,0,0,"ON");break;
                case 2: menutext(c+160+40,43+16*1,0,0,"FULL");break;
            }

			menutext(c,43+16*2,SHX(-3),PHX(-3),"CINEMATICS");
            if(ud.showcinematics) menutext(c+160+40,43+16*2,0,0,"ON");
            else menutext(c+160+40,43+16*2,0,0,"OFF");

			menutext(c,43+16*3,SHX(-3),PHX(-3),"WEAPON MODEL");
			if(ud.hideweapon) menutext(c+160+40,43+16*3,0,0,"OFF");
            else menutext(c+160+40,43+16*3,0,0,"ON");

			menutext(c,43+16*4,SHX(-3),PHX(-3),"WEAPON SWITCH");
			if(ud.weaponautoswitch) menutext(c+160+40,43+16*4,0,0,"OFF");
            else menutext(c+160+40,43+16*4,0,0,"ON");

			switch(ud.auto_aim)
            {
                case 0: menutext(c,43+16*5,0,nHostForceDisableAutoaim,"AUTOAIM DISABLED BY HOST");
						break;
				case 1: menutext(c,43+16*5,0,0,"AUTOAIM BULLET ONLY");
						break;
                case 2: menutext(c,43+16*5,0,0,"AUTOAIM REGULAR FULL");
						break;
            }

#ifndef AUSTRALIA
            menutext(c,43+16*6,SHX(-9),PHX(-9),"PARENTAL LOCK");
#else
            menutext(c,43+16*6,SHX(-9),1,"PARENTAL LOCK");
#endif


			break;

        case MENU_KEYBOARD_SETUP:

			// black translucent background underneath lists
			rotatesprite(0<<16, 0<<16, 65536l<<5, 0, BLANK, 8, 0, 1+2+8+16+64,
			    scale(0,xdim,320),scale(0,ydim,200),
			    scale(320-0,xdim,320)-1,scale(200-34,ydim,200)-1);

			c = (320>>1)-120-25;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"SETUP KEYBOARD");

			onbar = 0;
			x = probeXduke(c+210+lastkeysetup*62,46+16+16+16,0,NUMGAMEFUNCTIONS,20000);
			if(waiting4key)
			{
				probey = waiting4key-1; // force it to stay at the same location
				x=-(waiting4key-1)-2;
			}

			if (( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_LeftArrow ) ||
				 KB_KeyPressed( sc_kpad_4 ) || KB_KeyPressed( sc_kpad_6 )) &&
				!waiting4key) // set left or right column flag
			{
				lastkeysetup = !lastkeysetup;
				KB_ClearKeyDown( sc_RightArrow ); KB_ClearKeyDown( sc_LeftArrow );
				KB_ClearKeyDown( sc_kpad_4 ); KB_ClearKeyDown( sc_kpad_6 );
				sound(KICK_HIT);
			}

			if (KB_KeyPressed(sc_Delete) && -2>=x && x>=(-NUMGAMEFUNCTIONS-1) && !waiting4key) // clear a key
			{
				if(lastkeysetup)
					CONTROL_MapKey(-x-2, KeyMapping[-x-2].key1, 0);
				else
					CONTROL_MapKey(-x-2, 0, KeyMapping[-x-2].key2);

				KB_ClearKeyDown( sc_Delete ); // Avoid repeating delete
				sound(EXITMENUSOUND);
			}

			if ( (0<=x && x<NUMGAMEFUNCTIONS) || waiting4key) // set a key
			{
				if(!waiting4key)
				{
					waiting4key = x+1; // so it's always true
					KB_ClearLastScanCode(); // clear the enter hit that was just hit
				}

				if(KB_GetLastScanCode())
				{
					if(KB_GetLastScanCode() != sc_Escape)  // ESC is reserved for menu. Using it for controls could discard it completely, eg: AutoRun = "Escape"
					{
						if(lastkeysetup)
							CONTROL_MapKey(waiting4key-1, KeyMapping[waiting4key-1].key1, KB_GetLastScanCode());
						else
							CONTROL_MapKey(waiting4key-1, KB_GetLastScanCode(), KeyMapping[waiting4key-1].key2);

						sound(KICK_HIT);
					}

					KB_ClearLastScanCode();
					KB_FlushKeyboardQueue();
					KB_ClearKeysDown();
					waiting4key = false;
				}
			}

			if(!waiting4key)
				switch(x)
				{
					case -1:
						cmenu(MENU_OPTIONS);
						probey = 1; // back to setup keyboard
						break;
				}

			// display and scroll the whole keyboard list
			j = 7; // -j .. 0 .. j lines => 2*j+1 lines
			if(-2>=x && x>=(-NUMGAMEFUNCTIONS-1)) // -2 to -54 (53 values -> 0->52)
			{
				for(i=-j; i<=+j; i++)
					if(NUMGAMEFUNCTIONS > (-x-2+i) && (-x-2+i) >= 0)
					{
						gametext(c-10,47-16+8*(i+j),gamefunctions[-x-2+i],i?0:0,2+8+16); // disp name
						if (i ||  lastkeysetup || !waiting4key || (totalclock%128 < 64)) // blink 1st key
							gametext(c+185,47-16+8*(i+j),KB_ScanCodeToString( KeyMapping[-x-2+i].key1 )?KB_ScanCodeToString( KeyMapping[-x-2+i].key1 ):"...",i?0:0,2+8+16); // center-i
						if (i || !lastkeysetup || !waiting4key || (totalclock%128 < 64)) // blink 2nd key
							gametext(c+247,47-16+8*(i+j),KB_ScanCodeToString( KeyMapping[-x-2+i].key2 )?KB_ScanCodeToString( KeyMapping[-x-2+i].key2 ):"...",i?0:0,2+8+16); // center-i
					}
			}

			if(waiting4key)
				gametext(320>>1,47-16+8*(2*j+2)-4,"*** HIT A NEW KEY ***",0,2+8+16); // center-i
			else
				gametext(320>>1,47-16+8*(2*j+2)-4,"*** HIT ENTER OR DEL KEY ***",0,2+8+16); // center-i

			break;
#if 0
        case MENU_MOUSE_BUTTON_SETUP: // mouse button setup, from menu MENU_STICK_SETUP
            c = (320>>1)-120-25;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"SETUP ANALOG STICKS");

			// black translucent background underneath lists
			rotatesprite(0<<16, 0<<16, 65536l<<5, 0, BLANK, 8, 0, 1+2+8+16+64,
			    scale(0,xdim,320),scale(40,ydim,200),
			    scale(320-0,xdim,320)-1,scale(200-75,ydim,200)-1);

			onbar = 0;
			x = probeXduke(c+146,46+8,8,MAXMOUSEBUTTONS,20000);
			lastkeysetup = 0;

			if ( KB_KeyPressed( sc_kpad_4 ) || KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed (sc_BackSpace) )
			{
				lastkeysetup = 1; // reversed;
				KB_ClearKeyDown( sc_kpad_4 );
				KB_ClearKeyDown( sc_LeftArrow );
				KB_ClearKeyDown(sc_BackSpace);
				sound(KICK_HIT);
				x = -x-2;
			}
			else if ( KB_KeyPressed( sc_kpad_6 ) || KB_KeyPressed( sc_RightArrow ) )
			{
				KB_ClearKeyDown( sc_kpad_6 );
				KB_ClearKeyDown( sc_RightArrow );
				sound(KICK_HIT);
				x = -x-2;
			}

			if (KB_KeyPressed(sc_Delete) && -2>=x && x>=(-MAXMOUSEBUTTONS-1)) // clear a key
			{
				MouseMapping[-x-2] = -1;
				KB_ClearKeyDown( sc_Delete ); // Avoid repeating delete
				sound(EXITMENUSOUND);
			}

			if (0<=x && x<MAXMOUSEBUTTONS) // set a function
			{
				if (lastkeysetup) // going up
				{
					MouseMapping[x]--;
					if(MouseMapping[x]==-2)
						MouseMapping[x] = NUMGAMEFUNCTIONS-1;

				}
				else
				{
					MouseMapping[x]++;
					if(MouseMapping[x]==NUMGAMEFUNCTIONS)
						MouseMapping[x] = -1; // Discard
				}
			}

			switch(x)
			{
				case -1:
					cmenu(MENU_STICK_SETUP);
					probey = 6; // back to the general mouse setup menu
					break;
			}

			// display the button list
			for(i=0; i<MAXMOUSEBUTTONS; i++)
			{
				sprintf(text, "#%d",i);
				switch(i)
				{
					case 0:
						strcat(text, " Left");
						break;
					case 1:
						strcat(text, " Right");
						break;
					case 2:
						strcat(text, " Middle");
						break;
					case 3:
						strcat(text, " Wheel up");
						break;
					case 4:
						strcat(text, " Wheel down");
						break;
					default:
						strcat(text, " (Extra)");
						break;
				}

				gametext(c,47+i*8,text,0,2+8+16);
				gametext(c+121,47+i*8,(MouseMapping[i]!=-1)?CONFIG_FunctionNumToName(MouseMapping[i]):"...",0,2+8+16);
			}

			gametext(320>>1,47+(MAXMOUSEBUTTONS+1)*8,"*** USE ARROWS OR DEL KEY ***",0,2+8+16);

			break;
#endif
        case MENU_ANALOG_AXES_SETUP: // digital axes setup, from menu MENU_STICK_SETUP
            c = (320>>1)-120-25;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"ANALOG AXES");

			// black translucent background underneath lists
			rotatesprite(0<<16, 0<<16, 65536l<<5, 0, BLANK, 8, 0, 1+2+8+16+64,
			    scale(0,xdim,320),scale(40,ydim,200),
			    scale(320-0,xdim,320)-1,scale(200-100,ydim,200)-1);

			onbar = 0;
			x = probeXduke(c+146,46+8,8,MAXMOUSEAXES*2,20000);
			lastkeysetup = 0;

			if ( KB_KeyPressed( sc_kpad_4 ) || KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed (sc_BackSpace) )
			{
				lastkeysetup = 1; // reversed;
				KB_ClearKeyDown( sc_kpad_4 );
				KB_ClearKeyDown( sc_LeftArrow );
				KB_ClearKeyDown(sc_BackSpace);
				sound(KICK_HIT);
				x = -x-2;
			}
			else if ( KB_KeyPressed( sc_kpad_6 ) || KB_KeyPressed( sc_RightArrow ) )
			{
				KB_ClearKeyDown( sc_kpad_6 );
				KB_ClearKeyDown( sc_RightArrow );
				sound(KICK_HIT);
				x = -x-2;
			}

			if (KB_KeyPressed(sc_Delete) && -2>=x && x>=(-(max_axis)-1)) // clear a key
			{
//				MouseDigitalAxeMapping[(-x-2)>>1][(-x-2)&1] = -1;
                JoyAxisMapping[-x-2] = -1;
				KB_ClearKeyDown( sc_Delete ); // Avoid repeating delete
				sound(EXITMENUSOUND);
			}

			if (0<=x && x<(max_axis)) // set a function
			{
				if (lastkeysetup) // going up
				{
//					MouseDigitalAxeMapping[x>>1][x&1]--;
//					if(MouseDigitalAxeMapping[x>>1][x&1]==-2)
//						MouseDigitalAxeMapping[x>>1][x&1] = NUMGAMEFUNCTIONS-1;
                    JoyAxisMapping[x]--;
                    if (JoyAxisMapping[x] < 0)
                        JoyAxisMapping[x] = analog_maxtype - 1;
				}
				else
				{
//					MouseDigitalAxeMapping[x>>1][x&1]++;
//					if(MouseDigitalAxeMapping[x>>1][x&1]==NUMGAMEFUNCTIONS)
//						MouseDigitalAxeMapping[x>>1][x&1] = -1; // Discard
                    JoyAxisMapping[x]++;
                    if (JoyAxisMapping[x] >= analog_maxtype)
                        JoyAxisMapping[x] = 0;

				}
			}

			switch(x)
			{
				case -1:
					cmenu(MENU_STICK_SETUP);
					probey = 7; // back to the general mouse setup menu
					break;
			}

			// display the button list
			for(i=0; i<(max_axis); i++)
			{
				//*tempbuf = 0;
				text[0] = i & 2 ? 'R' : 'L';
				text[1] = '-';
				text[2] = i & 1 ? 'Y' : 'X';
				text[3] = ':';
				text[4] =  0;

				gametext(c,47+i*8,text,0,2+8+16);
				gametext(c+121,47+i*8,(JoyAxisMapping[i]!=-1) ? analogFunctions[JoyAxisMapping[i]]:"...",0,2+8+16);
			}

			gametext(320>>1,47+(4+1)*8,"*** USE ARROWS OR DEL KEY ***",0,2+8+16);

			break;

        case MENU_VIDEO_SETUP: // Video setup
			// FIX_00042: Build in Video setup.
			c = (320>>1)-120;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"VIDEO SETUP");

			onbar = (probey == 3 || probey == 4);
			x = probe(c+6,43,16,6);

			switch(x)
			{
				case -7: // cursor idle on the FPS option (5)
					gametext(320>>1,43+16*7,"*** SHORTCUT: TYPE DNRATE ***",0,2+8+16); // center-i

					break;

				case -3: // cursor idle on the togglefullscreen option (1)
					gametext(320>>1,43+16*7,"*** SHORTCUT: ALT-ENTER ***",0,2+8+16); // center-i

					break;

				case -2: // cursor idle on the resolution option (0)
#if USE_MANY_MODES
					if ( KB_KeyPressed( sc_kpad_4 ) || KB_KeyPressed( sc_LeftArrow ) )
					{
						current_resolution--;
						if(current_resolution == -1)
							current_resolution = 0;
						lastkeysetup = 1; // indicates we changed
						KB_ClearKeyDown( sc_kpad_4 );
						KB_ClearKeyDown( sc_LeftArrow );
						sound(PISTOL_BODYHIT);
					}
					else if ( KB_KeyPressed( sc_kpad_6 ) || KB_KeyPressed( sc_RightArrow ) )
					{
						current_resolution++; // reversed;
						if(current_resolution == validmodecnt)
							current_resolution = validmodecnt-1;
						lastkeysetup = 1; // indicates we changed
						KB_ClearKeyDown( sc_kpad_6 );
						KB_ClearKeyDown( sc_RightArrow );
						sound(PISTOL_BODYHIT);
					}
					if(lastkeysetup)
						gametext(320>>1,43+16*7,"*** HIT ENTER TO VALIDATE ***",0,2+8+16); // center-i
					else
						gametext(320>>1,43+16*7,"*** LEFT/RIGHT ARROW TO SELECT ***",0,2+8+16); // center-i
#endif
					break;

				case -1:
					cmenu(MENU_OPTIONS);
					probey = 4; // back to the general option menu
					break;

				case 0:
#if USE_MANY_MODES
					if(lastkeysetup)
						setgamemode(validmodexdim[current_resolution],validmodeydim[current_resolution]);
#endif
					lastkeysetup = 0; // indicating changes are done
					break;

				case 1:
#if USE_MANY_MODES
					FullScreen = !FullScreen;
					_platform_init(0, NULL, "Duke Nukem 3D", "Duke3D");
					_setgamemode(validmodexdim[current_resolution],validmodeydim[current_resolution]);
#endif
					break;

                case 2:
//                    ud.detail = 1-ud.detail;
                    // next-hack: always high details.
                      ud.detail = 1;
                    break;

				case 5:
					ud.tickrate ^= 1;
					vscrn(); // FIX_00056: Refresh issue w/FPS, small Weapon and custom FTA, when screen resized down
					break;
			}

			menutext(c,43,0,!USE_MANY_MODES,"RESOLUTION");
#if USE_MANY_MODES
			sprintf(text, "%d x %d", validmodexdim[current_resolution],validmodeydim[current_resolution]);
#else
			sprintf(text, "%d x %d", 320, 240);
#endif
			if (lastkeysetup == 0 || (totalclock%64 < 32)) // blink color after change
				menutext(c+150,43,0,!USE_MANY_MODES,text);
			else
				menutext(c+150,43,0,1,text);

			menutext(c,43+16*1,SHX(-3),PHX(-3),"FULLSCREEN");
			menutext(c+160+40,43+16*1,0,0,FullScreen?"ON":"OFF");

			menutext(c,43+16*2,SHX(-2), !ALLOW_LOW_DETAIL,"DETAIL");
			menutext(c+160+40,43+16*2,0,!ALLOW_LOW_DETAIL,ud.detail?"HIGH":"LOW");
			{
				// next-hack: reducing screen size would work, but why?
				// increasing screen size would work, except when there is a camera sprite.
                int32_t screen_size = ud.screen_size;

			    // FIX_00027: Added an extra small statusbar (HUD)
	            menutext(c,43+16*3,SHX(-5),PHX(-5),"SCREEN SIZE");
	            bar(c+167+40,43+16*3,(short *)&screen_size,-4,x==3,SHX(-5),PHX(-5));

#if FULL_SCREEN_SUPPORT
				if(ud.screen_size==4)
				{
					if(screen_size==0)
					{
						ud.extended_screen_size++;
						if(ud.extended_screen_size==2)
						{
							ud.extended_screen_size = 1;
							ud.screen_size -= 4;
						}
					}
					else if(screen_size==8)
					{
						ud.extended_screen_size--;
						if(ud.extended_screen_size<0)
						{
							ud.extended_screen_size=0;
							ud.screen_size += 4;
						}
					}
				}
				else
					ud.screen_size = screen_size;
#else
                ud.screen_size = screen_size >= 8 ? screen_size : 8;
#endif
			}

            menutext(c,43+16*4,SHX(-6),PHX(-6),"BRIGHTNESS");
            bar(c+167+40,43+16*4,(short *)&ud.brightness,8,x==4,SHX(-6),PHX(-6));
            if(x==4)
            {
                    setbrightness(ud.brightness>>2,&ps[myconnectindex].palette[0]);
                    // next-hack: notify we'll need to refresh the whole screen
                    setFullPaletteChanged();
            }

			menutext(c,43+16*5,SHX(-2),PHX(-2),"SHOW FPS");
			menutext(c+160+40,43+16*5,0,0,(ud.tickrate&1)?"ON":"OFF");

			break;

        case MENU_PRE_SAVE_GAME:
            capturingForSaveGame = 1;
            break;

        case MENU_ENTER_SAVE_GAME_NAME0:
        case MENU_ENTER_SAVE_GAME_NAME1:
        case MENU_ENTER_SAVE_GAME_NAME2:
        case MENU_ENTER_SAVE_GAME_NAME3:
        case MENU_ENTER_SAVE_GAME_NAME4:
        case MENU_ENTER_SAVE_GAME_NAME5:
        case MENU_ENTER_SAVE_GAME_NAME6:
        case MENU_ENTER_SAVE_GAME_NAME7:
        case MENU_ENTER_SAVE_GAME_NAME8:
        case MENU_ENTER_SAVE_GAME_NAME9:
        case MENU_SAVE_GAME_SELECT:
        case MENU_LOAD_GAME:

            c = 320>>1;
            rotatesprite(c<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
            rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);

            if(current_menu == MENU_LOAD_GAME) menutext(c,24,0,0,"LOAD GAME");
            else menutext(c,24,0,0,"SAVE GAME");

            if(current_menu >= MENU_ENTER_SAVE_GAME_NAME0 && current_menu <= MENU_ENTER_SAVE_GAME_NAME9 )
            {
                sprintf(text,"PLAYERS: %-2d                      ",ud.multimode);
                gametext(160,158,text,0,2+8+16);
                sprintf(text,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
                gametext(160,170,text,0,2+8+16);
#if FILENAME_SELECTION_IMPLEMENTED
                x = strget((320>>1),184,&ud.savegame[current_menu-MENU_ENTER_SAVE_GAME_NAME0][0],19, 999 );

                if(x == -1)
                {
            //        readsavenames();
                    ps[myconnectindex].gm = MODE_GAME;
                    if(ud.multimode < 2  && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                    goto DISPLAYNAMES;
                }
#else
                x = 1;
                // TODO: maybe use better name
                snprintf(ud.savegame[current_menu-MENU_ENTER_SAVE_GAME_NAME0],
                         sizeof(ud.savegame[current_menu-MENU_ENTER_SAVE_GAME_NAME0]),
                          "E%.1dL%.2d S: %.1d T: %.4d", ud.volume_number, ud.level_number, ud.player_skill,
                         (ototalclock / 120)% 100000 );
#endif
                if( x == 1 )
                {
                    if( ud.savegame[current_menu-MENU_ENTER_SAVE_GAME_NAME0][0] == 0 )
                    {
                        KB_FlushKeyboardQueue();
                        cmenu(MENU_SAVE_GAME_SELECT);
                    }
                    else
                    {
                        if(ud.multimode > 1)
                            saveplayer(-1-(current_menu-MENU_ENTER_SAVE_GAME_NAME0));
                        else saveplayer(current_menu-MENU_ENTER_SAVE_GAME_NAME0);
                        lastsavedpos = current_menu-MENU_ENTER_SAVE_GAME_NAME0;
                        ps[myconnectindex].gm = MODE_GAME;

                        if(ud.multimode < 2  && ud.recstat != 2)
                        {
                            ready2send = 1;
                            totalclock = ototalclock;
                        }
                        KB_ClearKeyDown(sc_Escape);
                        sound(EXITMENUSOUND);
                    }
                }
// The following was the pic
//                rotatesprite(101<<16,97<<16,65536,512,MAXTILES-1,-32,0,2+4+8+64,0,0,xdim-1,ydim-1);
                #if WIN32
                        savegamePreviewPtr = &ext_memory[EXT_MEMORY_SIZE  - sizeof(nvmData_t) + offsetof(nvmData_t, tempScreenshot) ];
                #else
                        savegamePreviewPtr =(uint8_t *) (extMemGetSize() - sizeof(nvmData_t) + offsetof(nvmData_t, tempScreenshot));
                #endif
                drawSavegameScreenshot();
                dispnames();
                rotatesprite((c+67+strlen(&ud.savegame[current_menu-MENU_ENTER_SAVE_GAME_NAME0][0])*4)<<16,(50+12*probey)<<16,32768L-10240,0,SPINNINGNUKEICON+(((totalclock)>>3)%7),0,0,10,0,0,xdim-1,ydim-1);
                break;
            }

           last_threehundred = probey;

            x = probe(c+68,54,12,10);

          if(current_menu == MENU_LOAD_GAME)
          {
              if( ud.savegame[probey][0] )
              {
                  if( lastprobey != probey )
                  {
                     loadpheader(probey,&volnum,&levnum,&plrskl,&numplr);
                     lastprobey = probey;
                  }

//                  rotatesprite(101<<16,97<<16,65536L,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);
                  drawSavegameScreenshot();
                  sprintf(text,"PLAYERS: %-2d                      ",numplr);
                  gametext(160,158,text,0,2+8+16);
                  sprintf(text,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+volnum,1+levnum,plrskl);
                  gametext(160,170,text,0,2+8+16);
              }
              else menutext(69,70,0,0,"EMPTY");
          }
          else
          {
              if( ud.savegame[probey][0] )
              {
                  if(lastprobey != probey)
                      loadpheader(probey,&volnum,&levnum,&plrskl,&numplr);
                  lastprobey = probey;
//                  rotatesprite(101<<16,97<<16,65536L,512,MAXTILES-3,-32,0,4+10+64,0,0,xdim-1,ydim-1);
                  drawSavegameScreenshot();
              }
              else menutext(69,70,0,0,"EMPTY");
              sprintf(text,"PLAYERS: %-2d                      ",ud.multimode);
              gametext(160,158,text,0,2+8+16);
              sprintf(text,"EPISODE: %-2d / LEVEL: %-2d / SKILL: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
              gametext(160,170,text,0,2+8+16);
          }

            switch( x )
            {
                case -1:
                    if(current_menu == MENU_LOAD_GAME) //load game
                    {
                        if( (ps[myconnectindex].gm&MODE_GAME) != MODE_GAME)
                        {
                            cmenu(MENU_MAIN);
                            break;
                        }
                        else
                            ps[myconnectindex].gm &= ~MODE_MENU;
                    }
                    else // save game
                        ps[myconnectindex].gm = MODE_GAME;

                    if(ud.multimode < 2 && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }

                    break;
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                    if( current_menu == MENU_LOAD_GAME)
                    {
                        if( ud.savegame[x][0] )
                            current_menu = (MENU_LOAD_GAME0+x);
                    }
                    else
                    {
                        if( ud.savegame[x][0] != 0)
                            current_menu = MENU_OVERWRITE_SAVE_GAME9_QUESTION+x;
                        else
                        {
                            KB_FlushKeyboardQueue();
                            current_menu = (MENU_ENTER_SAVE_GAME_NAME0+x);
                            ud.savegame[x][0] = 0;
                            inputloc = 0;
                        }
                    }
                    break;
            }
#if FILENAME_SELECTION_IMPLEMENTED
            DISPLAYNAMES:
#endif
            dispnames();
            break;

        case MENU_HELP0: // help
        case MENU_HELP1:
		if(!VOLUMEONE)
		{
			c = 320>>1;

            if( KB_KeyPressed( sc_LeftArrow ) ||
                KB_KeyPressed( sc_kpad_4 ) ||
                KB_KeyPressed( sc_UpArrow ) ||
                KB_KeyPressed( sc_PgUp ) ||
                KB_KeyPressed( sc_kpad_8 ) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                KB_ClearKeyDown(sc_UpArrow);
                KB_ClearKeyDown(sc_PgUp);
                KB_ClearKeyDown(sc_kpad_8);

                sound(KICK_HIT);
                current_menu--;
                if(current_menu < MENU_HELP0) current_menu = MENU_HELP1;
            }
            else if(
                KB_KeyPressed( sc_PgDn ) ||
                KB_KeyPressed( sc_Enter ) ||
                KB_KeyPressed( sc_kpad_Enter ) ||
                KB_KeyPressed( sc_RightArrow ) ||
                KB_KeyPressed( sc_DownArrow ) ||
                KB_KeyPressed( sc_kpad_2 ) ||
                KB_KeyPressed( sc_kpad_9 ) ||
                KB_KeyPressed( sc_Space ) ||
                KB_KeyPressed( sc_kpad_6 ) )
            {
                KB_ClearKeyDown(sc_PgDn);
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_ClearKeyDown(sc_kpad_6);
                KB_ClearKeyDown(sc_kpad_9);
                KB_ClearKeyDown(sc_kpad_2);
                KB_ClearKeyDown(sc_DownArrow);
                KB_ClearKeyDown(sc_Space);
                sound(KICK_HIT);
                current_menu++;
                if(current_menu > MENU_HELP1) current_menu = MENU_HELP0;
            }

            if( KB_KeyPressed(sc_Escape) )
            {
				KB_ClearKeyDown(sc_Escape); // or else ESC will be activated in cmenu(MENU_MAIN)
                if(ps[myconnectindex].gm&MODE_GAME)
                    cmenu(MENU_INGAME_MENU);
                else cmenu(MENU_MAIN);
                return;
            }

            flushperms();
            switch(current_menu)
            {
                case MENU_HELP0:
                    rotatesprite(0,0,65536L,0,TEXTSTORY,0,0,10+16+64, 0,0,xdim-1,ydim-1);
                    break;
                case MENU_HELP1:
                    rotatesprite(0,0,65536L,0,F1HELP,0,0,10+16+64, 0,0,xdim-1,ydim-1);
                    break;
            }
		break;
		}

		case MENU_HELP2:
        case MENU_HELP3:
		if(VOLUMEONE)
		{
            c = 320>>1;

            if( KB_KeyPressed( sc_LeftArrow ) ||
                KB_KeyPressed( sc_kpad_4 ) ||
                KB_KeyPressed( sc_UpArrow ) ||
                KB_KeyPressed( sc_PgUp ) ||
                KB_KeyPressed( sc_kpad_8 ) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                KB_ClearKeyDown(sc_UpArrow);
                KB_ClearKeyDown(sc_PgUp);
                KB_ClearKeyDown(sc_kpad_8);

                sound(KICK_HIT);
                current_menu--;
                if(current_menu < MENU_HELP0) current_menu = MENU_HELP3;
            }
            else if(
                KB_KeyPressed( sc_PgDn ) ||
                KB_KeyPressed( sc_Enter ) ||
                KB_KeyPressed( sc_kpad_Enter ) ||
                KB_KeyPressed( sc_RightArrow ) ||
                KB_KeyPressed( sc_DownArrow ) ||
                KB_KeyPressed( sc_kpad_2 ) ||
                KB_KeyPressed( sc_kpad_9 ) ||
                KB_KeyPressed( sc_Space ) ||
                KB_KeyPressed( sc_kpad_6 ) )
            {
                KB_ClearKeyDown(sc_PgDn);
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_ClearKeyDown(sc_kpad_6);
                KB_ClearKeyDown(sc_kpad_9);
                KB_ClearKeyDown(sc_kpad_2);
                KB_ClearKeyDown(sc_DownArrow);
                KB_ClearKeyDown(sc_Space);
                sound(KICK_HIT);
                current_menu++;
                if(current_menu > MENU_HELP3) current_menu = MENU_HELP0;
            }

            if( KB_KeyPressed(sc_Escape) )
            {
                if(ps[myconnectindex].gm&MODE_GAME)
                    cmenu(MENU_INGAME_MENU);
                else cmenu(MENU_MAIN);
                return;
            }

            flushperms();
            rotatesprite(0,0,65536L,0,ORDERING+current_menu-MENU_HELP0,0,0,10+16+64,0,0,xdim-1,ydim-1);
		}
		break;

        case MENU_KEYBOARD_QUIT: // from f10
		case MENU_QUIT: // from menu MENU_MAIN
		case MENU_QUIT_GAME: // from menu MENU_INGAME_MENU

            c = 320>>1;

            gametext(c,90,"Are you sure you want to quit?",0,2+8+16);
            gametext(c,99,"(Y/N)",0,2+8+16);
#ifdef WIN32
			_handle_events();
#endif
			if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
				gameexitanycase();
            }

            x = probe(186,124,0,0);
            if(x == -1 || KB_KeyPressed(sc_N) || RMB)
            {
                KB_ClearKeyDown(sc_N);
                quittimer = 0;
				// FIX_00073: menu off messed up. While in game hit Esc -> select quit -> press esc => stuck in menu
				if (current_menu==MENU_KEYBOARD_QUIT)
					ps[myconnectindex].gm &= ~MODE_MENU;
				else if(current_menu==MENU_QUIT)
					cmenu(MENU_MAIN);
				else
					cmenu(MENU_INGAME_MENU);
            }
            break;

		case MENU_QUIT_TO_TITLE:
            c = 320>>1;
            gametext(c,90,"Quit to Title?",0,2+8+16);
            gametext(c,99,"(Y/N)",0,2+8+16);
#ifdef WIN32
			_handle_events();
#endif
            if( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB )
            {
                KB_FlushKeyboardQueue();
                ps[myconnectindex].gm = MODE_DEMO;
                if(ud.recstat == 1)
                    closedemowrite();
				if( ud.m_recstat != 2 && ud.last_level >= 0 && ud.multimode > 1 && ud.coop != 1)
					dobonus(1);
				ud.last_level = -1;
                cmenu(MENU_MAIN);
            }

            x = probe(186,124,0,0);

            if(x == -1 || KB_KeyPressed(sc_N) || RMB)
            {
                cmenu(MENU_INGAME_MENU);
                if(ud.multimode < 2  && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }

            break;

        case MENU_MULTI_WAITING_FOR_OTHER_PLAYERS_READY_MESSAGE:
            rotatesprite(160<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
//            displayfragbar();
            rotatesprite(160<<16,29<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,34,0,0,&ud.user_name[myconnectindex][0]);

            sprintf((char*)text,"Waiting for other players");
            gametext(160,50,(char*)text,0,2+8+16);
            gametext(160,59,"to be ready",0,2+8+16);
            // change immediately.
            cmenu(MENU_MULTI_PEER_READY);
            if( KB_KeyPressed(sc_Escape) )
            {
                KB_ClearKeyDown(sc_Escape);
                sound(EXITMENUSOUND);
                cmenu(MENU_MAIN);
            }
            break;
#if USER_MAP_SUPPORT
        case MENU_MULTI_READ_USER_MAPS:
            if(menunamecnt == 0)
            {
        //        getfilenames("SUBD");
                getfilenames("*.MAP");
                sortfilenames();
                if (menunamecnt == 0)
                    cmenu(MENU_MULTI_HOST_SETUP);
            }
        case MENU_MULTI_SELECT_USER_MAPS:
            c = (320>>1) - 120;
            displayfragbar();
            rotatesprite(320>>1<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"USER MAPS");
            for(x=0;x<menunamecnt;x++)
            {
                if(x == fileselect)
                    minitext(15 + (x/15)*54,32 + (x%15)*8,menuname[x],0,26);
                else minitext(15 + (x/15)*54,32 + (x%15)*8,menuname[x],16,26);
            }

            fileselect = probey;
            if( KB_KeyPressed( sc_LeftArrow ) || KB_KeyPressed( sc_kpad_4 ) || ((buttonstat&1) && minfo.dyaw < -256 ) )
            {
                KB_ClearKeyDown( sc_LeftArrow );
                KB_ClearKeyDown( sc_kpad_4 );
                probey -= 15;
                if(probey < 0) probey += 15;
                else sound(KICK_HIT);
            }
            if( KB_KeyPressed( sc_RightArrow ) || KB_KeyPressed( sc_kpad_6 ) || ((buttonstat&1) && minfo.dyaw > 256 ) )
            {
                KB_ClearKeyDown( sc_RightArrow );
                KB_ClearKeyDown( sc_kpad_6 );
                probey += 15;
                if(probey >= menunamecnt)
                    probey -= 15;
                else sound(KICK_HIT);
            }

            onbar = 0;
            x = probe(0,0,0,menunamecnt);

            if(x == -1) cmenu(MENU_MULTI_HOST_SETUP);
            else if(x >= 0)
            {
                text[0] = 8;
                text[1] = ud.m_level_number = 6;
                text[2] = ud.m_volume_number = 0;
                text[3] = ud.m_player_skill+1;

                if(ud.player_skill == 3)
                    ud.m_respawn_monsters = 1;
                else ud.m_respawn_monsters = 0;

                if(ud.m_coop == 0) ud.m_respawn_items = 1;
                else ud.m_respawn_items = 0;

                ud.m_respawn_inventory = 1;

                text[4] = ud.m_monsters_off;
                text[5] = ud.m_respawn_monsters;
                text[6] = ud.m_respawn_items;
                text[7] = ud.m_respawn_inventory;
                text[8] = ud.m_coop;
                text[9] = ud.m_marker;

                x = strlen(menuname[probey]);

                copybufbyte(menuname[probey],text+10,x);
                copybufbyte(menuname[probey],boardfilename,x+1);

              //  for(c=connecthead;c>=0;c=connectpoint2[c])
              //      if(c != myconnectindex)
              //          sendpacket(c,(uint8_t*)text,x+10);
                broadcastPacket(text, x + 10, true);
                newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill+1);
                enterlevel(MODE_GAME);
            }
            break;
        // next-hack: added these menu entries.
#endif
        case MENU_MULTI_SETUP:
            if (ud.m_channel < 11)
            {
                ud.m_channel = 11;
            }
            else if (ud.m_channel > 26)
            {
                ud.m_channel = 26;
            }
            c = (SCREEN_WIDTH / 2) - 120;
            rotatesprite((SCREEN_WIDTH / 2)<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(SCREEN_WIDTH / 2,24,0,0,"MULTIPLAYER SETUP");
            //snprintf(text, sizeof(text), "YOUR NAME: %s", &ud.user_name[myconnectindex][0]);
            menutext(c,57,SHX(-8),PHX(-8),"YOUR NAME:");
            menutext(c,57+16,SHX(-8),PHX(-8),"CHANNEL:");
            menutext(c,57+16+16,SHX(-8),PHX(-8),"CONNECT TO OTHERS");
            snprintf(text, sizeof(text), "%d",ud.m_channel);
            gametext(c+130,57-7 + 16,text,0,2+8+16);
            if (enteringText)
            {
                 enterText(c + 130 ,57-7, myname, MAX_NAME_LEN - 1, 998);
            }
            else
            {
                gametext(c+130,57-7,myname,0,2+8+16);
                x = probe(c,57,16,3);
                KB_ClearKeysDown();
            //printf("X: %d\r\n", x);
            switch (x)
            {
                    case 2:
                        multiplayerConnection = 1;
                    break;
                    case 1:
                        ud.m_channel++;
                        if (ud.m_channel > 26)
                            ud.m_channel = 11;
                        #if !WIN32
                        radioSetChannel(ud.m_channel);
                        #endif
                        break;
                    case 0:
                        enteringText = x + 1;
                        textInputLoc = strlen(myname);
                        break;
                case -1:
                    cmenu(MENU_MAIN);
                    break;
            }
            }
            break;
        case MENU_MULTI_PEER_READY:
        case MENU_MULTI_PEER_LOBBY:
//            printf("LOBBY\r\n");
            c = (SCREEN_WIDTH / 2) - 120;
            rotatesprite((SCREEN_WIDTH / 2)<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(SCREEN_WIDTH / 2,24,0,0,"PEER LOBBY");
            for (int i = 0; i < MAXPLAYERS; i++)
            {
                char tmps[32];
                snprintf(tmps, sizeof(tmps), "Player %i: %s", i, ud.user_name[i]);
                menutext(c,57 + i * 16,SHX(-8),PHX(-8), tmps);
            }
            if (isInLobbyMenu())
            {
                menutext(c,57 + MAXPLAYERS * 16,SHX(-8), !ud.multiReady, "Ready");
            }
            else
                menutext(c,57 + MAXPLAYERS * 16,SHX(-8),!ud.multiReady, "Start New Match");
            x = probe(c,57 + MAXPLAYERS * 16,16,1);
            switch (x)
            {
                case 0:
                    if (ud.multiReady)
                    {
                       if (isInLobbyMenu())
                       {
                            cmenu(MENU_MULTI_PEER_READY);
                       }
                       else
                       {
                           cmenu(MENU_MULTI_HOST_SETUP);
                       }
                    }
                    break;
                case -1:
                    cmenu(MENU_MAIN);
                    break;
            }
        break;
        case MENU_MULTI_HOST_SETUP: // multiplayer and bot menu
            c = (320>>1) - 120;
            if((ps[myconnectindex].gm&MODE_GAME) != MODE_GAME)
            {
                displayfragbar();
            }
            rotatesprite(160<<16,26<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(160,31,0,0,&ud.user_name[myconnectindex][0]);

            x = probe(c,57-8,16,8);

            switch(x)
            {
				// FIX_00068: menu "New Game" in multiplayer mode now allowing left/right arrow for selection
				case -7: // idle on case 5
					if ( KB_KeyPressed( sc_kpad_4 ) || KB_KeyPressed( sc_LeftArrow ) ||
						 KB_KeyPressed( sc_kpad_6 ) || KB_KeyPressed( sc_RightArrow ))
					{
						if(ud.m_coop == 1)
							ud.m_ffire = !ud.m_ffire;

						KB_ClearKeyDown( sc_kpad_4 );
						KB_ClearKeyDown( sc_LeftArrow );
						KB_ClearKeyDown( sc_kpad_6 );
						KB_ClearKeyDown( sc_RightArrow );
						sound(PISTOL_BODYHIT);
					}
                    break;

				case -6: // idle on case 4
					if ( KB_KeyPressed( sc_kpad_4 ) || KB_KeyPressed( sc_LeftArrow ) ||
						 KB_KeyPressed( sc_kpad_6 ) || KB_KeyPressed( sc_RightArrow ))
					{
						if(ud.m_coop == 0)
							ud.m_marker = !ud.m_marker;

						KB_ClearKeyDown( sc_kpad_4 );
						KB_ClearKeyDown( sc_LeftArrow );
						KB_ClearKeyDown( sc_kpad_6 );
						KB_ClearKeyDown( sc_RightArrow );
						sound(PISTOL_BODYHIT);
					}
                    break;

				case -5: // idle on case 3
					if ( KB_KeyPressed( sc_kpad_4 ) || KB_KeyPressed( sc_LeftArrow ) )
					{
						if(ud.m_monsters_off == 1 && ud.m_player_skill > 0)
							ud.m_monsters_off = 0;

						if(ud.m_monsters_off == 1)
						{
							ud.m_monsters_off = 0;
							ud.m_player_skill = 3;
						}
						else
						{
							ud.m_player_skill--;
							if(ud.m_player_skill < 0)
							{
								ud.m_player_skill = 0;
								ud.m_monsters_off = 1;
							}
						}

						KB_ClearKeyDown( sc_kpad_4 );
						KB_ClearKeyDown( sc_LeftArrow );
						sound(PISTOL_BODYHIT);
					}
					else if( KB_KeyPressed( sc_kpad_6 ) || KB_KeyPressed( sc_RightArrow ) )
					{
						if(ud.m_monsters_off == 1 && ud.m_player_skill > 0)
							ud.m_monsters_off = 0;

						if(ud.m_monsters_off == 0)
						{
							ud.m_player_skill++;
							if(ud.m_player_skill > 3)
							{
								ud.m_player_skill = 0;
								ud.m_monsters_off = 1;
							}
						}
						else
						{
							ud.m_monsters_off = 0;
							ud.m_player_skill = 0;
						}

						KB_ClearKeyDown( sc_kpad_6 );
						KB_ClearKeyDown( sc_RightArrow );
						sound(PISTOL_BODYHIT);
					}
                    break;

#ifndef ONELEVELDEMO

				case -4: // idle on case 2
					if ( KB_KeyPressed( sc_kpad_4 ) || KB_KeyPressed( sc_LeftArrow ) )
					{
						ud.m_level_number--;
if (!VOLUMEONE)
{
						if(ud.m_volume_number == 0 && ud.m_level_number < 0)
							ud.m_level_number = 6;
}
else
{
						if(ud.m_volume_number == 0 && ud.m_level_number < 0)
							ud.m_level_number = 5;
}
						if(ud.m_level_number < 0) ud.m_level_number = 10;

						KB_ClearKeyDown( sc_kpad_4 );
						KB_ClearKeyDown( sc_LeftArrow );
						sound(PISTOL_BODYHIT);
					}
					else if ( KB_KeyPressed( sc_kpad_6 ) || KB_KeyPressed( sc_RightArrow ) )
					{
						ud.m_level_number++;
if (!VOLUMEONE)
{
						if(ud.m_volume_number == 0 && ud.m_level_number > 6)
							ud.m_level_number = 0;
}
else
{
						if(ud.m_volume_number == 0 && ud.m_level_number > 5)
							ud.m_level_number = 0;
}
						if(ud.m_level_number > 10) ud.m_level_number = 0;

						KB_ClearKeyDown( sc_kpad_6 );
						KB_ClearKeyDown( sc_RightArrow );
						sound(PISTOL_BODYHIT);
					}
					break;
#endif

				case -3: // Idle on case 1
if (!VOLUMEONE)
{
					if ( KB_KeyPressed( sc_kpad_4 ) || KB_KeyPressed( sc_LeftArrow ) )
					{
						ud.m_volume_number--;
if(PLUTOPAK)
						{	if(ud.m_volume_number < 0 ) ud.m_volume_number = 3; }
else
						{	if(ud.m_volume_number < 0) ud.m_volume_number = 2; }

						if(ud.m_volume_number == 0 && ud.m_level_number > 6)
							ud.m_level_number = 0;
						if(ud.m_level_number > 10) ud.m_level_number = 0;

						KB_ClearKeyDown( sc_kpad_4 );
						KB_ClearKeyDown( sc_LeftArrow );
						sound(PISTOL_BODYHIT);
					}
					else if( KB_KeyPressed( sc_kpad_6 ) || KB_KeyPressed( sc_RightArrow ) )
					{
						ud.m_volume_number++;
if(PLUTOPAK)
						{	if(ud.m_volume_number > 3) ud.m_volume_number = 0; }
else
						{	if(ud.m_volume_number > 2) ud.m_volume_number = 0; }

						if(ud.m_volume_number == 0 && ud.m_level_number > 6)
							ud.m_level_number = 0;
						if(ud.m_level_number > 10) ud.m_level_number = 0;

						KB_ClearKeyDown( sc_kpad_6 );
						KB_ClearKeyDown( sc_RightArrow );
						sound(PISTOL_BODYHIT);
					}
}
					break;

				case -2: // Idle on case 0
					if ( KB_KeyPressed( sc_kpad_4 ) || KB_KeyPressed( sc_LeftArrow ) )
					{
						ud.m_coop--;
						if(ud.m_coop == -1) ud.m_coop = 2;

						KB_ClearKeyDown( sc_kpad_4 );
						KB_ClearKeyDown( sc_LeftArrow );
						sound(PISTOL_BODYHIT);
					}
					else if( KB_KeyPressed( sc_kpad_6 ) || KB_KeyPressed( sc_RightArrow ) )
					{
						ud.m_coop++;
						if(ud.m_coop == 3) ud.m_coop = 0;

						KB_ClearKeyDown( sc_kpad_6 );
						KB_ClearKeyDown( sc_RightArrow );
						sound(PISTOL_BODYHIT);
					}
                    break;

                case -1:
                    ud.m_recstat = 0;
                    if(ps[myconnectindex].gm&MODE_GAME) cmenu(MENU_INGAME_MENU);
                    else cmenu(MENU_MAIN);
                    break;
                case 0:
                    ud.m_coop++;
                    if(ud.m_coop == 3) ud.m_coop = 0;
                    break;
                case 1:
if (!VOLUMEONE)
{                    ud.m_volume_number++;
if(PLUTOPAK)
					{	if(ud.m_volume_number > 3) ud.m_volume_number = 0; }
else
					{	if(ud.m_volume_number > 2) ud.m_volume_number = 0; }

                    if(ud.m_volume_number == 0 && ud.m_level_number > 6)
                        ud.m_level_number = 0;
                    if(ud.m_level_number > 10) ud.m_level_number = 0;
}
                    break;
                case 2:
#ifndef ONELEVELDEMO
                    ud.m_level_number++;
if (!VOLUMEONE)
{
                    if(ud.m_volume_number == 0 && ud.m_level_number > 6)
                        ud.m_level_number = 0;
}
else
{
                    if(ud.m_volume_number == 0 && ud.m_level_number > 5)
                        ud.m_level_number = 0;
}
                    if(ud.m_level_number > 10) ud.m_level_number = 0;
#endif
                    break;
                case 3:
                    if(ud.m_monsters_off == 1 && ud.m_player_skill > 0)
                        ud.m_monsters_off = 0;

                    if(ud.m_monsters_off == 0)
                    {
                        ud.m_player_skill++;
                        if(ud.m_player_skill > 3)
                        {
                            ud.m_player_skill = 0;
                            ud.m_monsters_off = 1;
                        }
                    }
                    else
					{
						ud.m_monsters_off = 0;
						ud.m_player_skill = 0;
					}

                    break;

                case 4:
                    if(ud.m_coop == 0)
                        ud.m_marker = !ud.m_marker;
                    break;

                case 5:
                    if(ud.m_coop == 1)
                        ud.m_ffire = !ud.m_ffire;
                    break;

                case 6: // select user map
if(!VOLUMEONE)
{                    if(boardfilename[0] == 0) break;

                    text[0] = 5;
                    text[1] = ud.m_level_number = 7;
                    text[2] = ud.m_volume_number = 0;
                    text[3] = ud.m_player_skill+1;

                    ud.level_number = ud.m_level_number;
                    ud.volume_number = ud.m_volume_number;

                    if( ud.m_player_skill == 3 ) ud.m_respawn_monsters = 1;
                    else ud.m_respawn_monsters = 0;

                    if(ud.m_coop == 0) ud.m_respawn_items = 1;
                    else ud.m_respawn_items = 0;

                    ud.m_respawn_inventory = 1;

                    text[4] = ud.m_monsters_off;
                    text[5] = ud.m_respawn_monsters;
                    text[6] = ud.m_respawn_items;
                    text[7] = ud.m_respawn_inventory;
                    text[8] = ud.m_coop;
                    text[9] = ud.m_marker;
                    text[10] = ud.m_ffire;

                    for(c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        resetweapons(c);
                        resetinventory(c);

                       // if(c != myconnectindex)
                       //     sendpacket(c,(uint8_t*)text,11);
                    }
                    broadcastPacket((uint8_t*) text, 11, true);
                    //
                    newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill+1);
                    enterlevel(MODE_GAME);

                    return;
}
                case 7: // start game

                    text[0] = 5;
                    text[1] = ud.m_level_number;
                    text[2] = ud.m_volume_number;
                    text[3] = ud.m_player_skill+1;

                    if( ud.m_player_skill == 3 ) ud.m_respawn_monsters = 1;
                    else ud.m_respawn_monsters = 0;

                    if(ud.m_coop == 0) ud.m_respawn_items = 1;
                    else ud.m_respawn_items = 0;

                    ud.m_respawn_inventory = 1;

                    text[4] = ud.m_monsters_off;
                    text[5] = ud.m_respawn_monsters;
                    text[6] = ud.m_respawn_items;
                    text[7] = ud.m_respawn_inventory;
                    text[8] = ud.m_coop;
                    text[9] = ud.m_marker;
                    text[10] = ud.m_ffire;

                    for(c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        resetweapons(c);
                        resetinventory(c);

//                        if(c != myconnectindex)
//                            sendpacket(c,(uint8_t*)text,11);
                    }
                    broadcastPacket((uint8_t*) text, 11, true);

                    newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill+1);
                    enterlevel(MODE_GAME);

                    return;

            }

            c += 40;

            if(ud.m_coop==1) gametext(c+70,57-7-9,"COOPERATIVE PLAY",0,2+8+16);
            else if(ud.m_coop==2) gametext(c+70,57-7-9,"DUKEMATCH (NO SPAWN)",0,2+8+16);
            else gametext(c+70,57-7-9,"DUKEMATCH (SPAWN)",0,2+8+16);

 if(VOLUMEONE)
            gametext(c+70,57+16-7-9,volume_names[ud.m_volume_number],0,2+8+16);
 else
            {
            gametext(c+70,57+16-7-9,volume_names[ud.m_volume_number],0,2+8+16);
            }

            gametext(c+70,57+16+16-7-9,&level_names[11*ud.m_volume_number+ud.m_level_number][0],0,2+8+16);

            if(ud.m_monsters_off == 0 || ud.m_player_skill > 0)
                gametext(c+70,57+16+16+16-7-9,skill_names[ud.m_player_skill],0,2+8+16);
            else gametext(c+70,57+16+16+16-7-9,"NONE",0,2+8+16);

            if(ud.m_coop == 0)
            {
                if(ud.m_marker)
                    gametext(c+70,57+16+16+16+16-7-9,"ON",0,2+8+16);
                else gametext(c+70,57+16+16+16+16-7-9,"OFF",0,2+8+16);
            }

            if(ud.m_coop == 1)
            {
                if(ud.m_ffire)
                    gametext(c+70,57+16+16+16+16+16-7-9,"ON",0,2+8+16);
                else gametext(c+70,57+16+16+16+16+16-7-9,"OFF",0,2+8+16);
            }

            c -= 44;

            menutext(c,57-9,SHX(-2),PHX(-2),"GAME TYPE");

if (VOLUMEONE)
{            sprintf(text,"EPISODE %d",ud.m_volume_number+1);
            menutext(c,57+16-9,SHX(-3),1,text);
}
else
{
            sprintf(text,"EPISODE %d",ud.m_volume_number+1);
            menutext(c,57+16-9,SHX(-3),PHX(-3),text);
}

#ifndef ONELEVELDEMO
            sprintf(text,"LEVEL %d",ud.m_level_number+1);
            menutext(c,57+16+16-9,SHX(-4),PHX(-4),text);
#else
            sprintf(text,"LEVEL %d",ud.m_level_number+1);
            menutext(c,57+16+16-9,SHX(-4),1,text);
#endif
            menutext(c,57+16+16+16-9,SHX(-5),PHX(-5),"MONSTERS");

            if(ud.m_coop == 0)
                menutext(c,57+16+16+16+16-9,SHX(-6),PHX(-6),"MARKERS");
            else
                menutext(c,57+16+16+16+16-9,SHX(-6),1,"MARKERS");

            if(ud.m_coop == 1)
                menutext(c,57+16+16+16+16+16-9,SHX(-6),PHX(-6),"FR. FIRE");
            else menutext(c,57+16+16+16+16+16-9,SHX(-6),1,"FR. FIRE");

if(!VOLUMEONE)
{            menutext(c,57+16+16+16+16+16+16-9,SHX(-7),boardfilename[0] == 0,"USER MAP");
            if( boardfilename[0] != 0 )
                gametext(c+70+44,57+16+16+16+16+16,boardfilename,0,2+8+16);
}
else
{
			menutext(c,57+16+16+16+16+16+16-9,SHX(-7),1,"USER MAP");
}

            menutext(c,57+16+16+16+16+16+16+16-9,SHX(-8),PHX(-8),"START GAME");

    break;
    }

    if( (ps[myconnectindex].gm&MODE_MENU) != MODE_MENU)
    {
        vscrn();
        cameraclock = totalclock;
        cameradist = 65536L;
    }
}

void palto(uint8_t  r,uint8_t  g,uint8_t  b,int32_t e, int present)
{
    int i;
    uint8_t  temparray[768];

    for(i=0;i<(256*3);i+=3)
    {
        temparray[i  ] =ps[myconnectindex].palette[i+0]+((((int32_t)r-(int32_t)ps[myconnectindex].palette[i+0])*(int32_t)(e&127))>>6);
        temparray[i+1] =ps[myconnectindex].palette[i+1]+((((int32_t)g-(int32_t)ps[myconnectindex].palette[i+1])*(int32_t)(e&127))>>6);
        temparray[i+2] =ps[myconnectindex].palette[i+2]+((((int32_t)b-(int32_t)ps[myconnectindex].palette[i+2])*(int32_t)(e&127))>>6);
    }

    setbrightness(ud.brightness>>2,temparray);
    setFullPaletteChanged();
	if (present)
	{
#if !WIN32
        updateAudio();
#endif
        VBE_presentPalette();
	}
#if MG24
//	setFramePalette(temparray);
#endif
}


void drawoverheadmap(int32_t cposx, int32_t cposy, int32_t czoom, short cang)
{
        int32_t i, j, k, l, x1, y1, x2, y2, x3, y3, x4, y4, ox, oy, xoff, yoff;
        int32_t dax, day, cosang, sinang, xspan, yspan, sprx, spry;
        int32_t xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
        int32_t xvect, yvect, xvect2, yvect2;
        short p;
        uint8_t  col;
        walltype *wal, *wal2;
//        spritetype *spr;

        xvect = sintable[(-cang)&2047] * czoom;
        yvect = sintable[(1536-cang)&2047] * czoom;
        xvect2 = mulscale16(xvect,yxaspect);
        yvect2 = mulscale16(yvect,yxaspect);

                //Draw red lines
        for(i=0;i<numsectors;i++)
        {
                if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

                startwall = constsector[i].wallptr;
                endwall = constsector[i].wallptr + constsector[i].wallnum;

                z1 = getSector(i)->ceilingz; z2 = getSector(i)->floorz;

                for(j=startwall,wal=getWall(startwall);j<endwall;j++, wal = getWall(j))
                {
                        k = constwall[j].nextwall; if (k < 0) continue;

                        //if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;
                        //if ((k > j) && ((show2dwall[k>>3]&(1<<(k&7))) > 0)) continue;

                        if (getSector(constwall[j].nextsector)->ceilingz == z1)
                                if (getSector(constwall[j].nextsector)->floorz == z2)
                                        if (( (getWall_cstat(j)|getWall_cstat(constwall[j].nextwall)) &(16+32)) == 0) continue;

                        col = 139; //red
                        if ( (getWall_cstat(j)|getWall_cstat(constwall[j].nextwall)) &1) col = 234; //magenta

                        if (!(show2dsector[constwall[j].nextsector>>3]&(1<<(constwall[j].nextsector&7))))
                                col = 24;
            else continue;

                        ox = wal->x-cposx; oy = wal->y-cposy;
                        x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                        y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

                        wal2 = getWall(getWall_point2(j));
                        ox = wal2->x-cposx; oy = wal2->y-cposy;
                        x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                        y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

                        drawline256(x1,y1,x2,y2,col);
                }
        }

                //Draw sprites
        k = ps[screenpeek].i;
        for(i=0;i<numsectors;i++)
        {
                if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;
                for(j=headspritesect[i];j>=0;j=spr_add_data[j].nextspritesect)
                        //if ((show2dsprite[j>>3]&(1<<(j&7))) > 0)
                        {
                //spr = getSprite(j);
                //spritetypeoffs *spr_o = &sprite_offsets[j];

                if (j == k || (getSprite(j)->_cstat & SPRITE_INVISIBLE) || getSprite(j)->_cstat == 257 || getSprite(j)->_xrepeat == 0) continue;

                                col = 71; //cyan
                                if (getSprite(j)->_cstat&1) col = 234; //magenta

                                sprx = getSprite(j)->_x;
                                spry = getSprite(j)->_y;

                if( (getSprite(j)->_cstat&257) != 0) switch (getSprite(j)->_cstat&48)
                                {
                    case 0: break;
                                                ox = sprx-cposx; oy = spry-cposy;
                                                x1 = dmulscale16(ox,xvect,-oy,yvect);
                                                y1 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = (sintable[(getSprite(j)->_ang+512)&2047]>>7);
                                                oy = (sintable[(getSprite(j)->_ang)&2047]>>7);
                                                x2 = dmulscale16(ox,xvect,-oy,yvect);
                                                y2 = dmulscale16(oy,xvect,ox,yvect);

                                                x3 = mulscale16(x2,yxaspect);
                                                y3 = mulscale16(y2,yxaspect);

                                                drawline256(x1-x2+(xdim<<11),y1-y3+(ydim<<11),
                                                                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                                                drawline256(x1-y2+(xdim<<11),y1+x3+(ydim<<11),
                                                                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                                                drawline256(x1+y2+(xdim<<11),y1-x3+(ydim<<11),
                                                                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                        break;

                                        case 16:
                        if( getSprite(j)->_picnum == LASERLINE )
                        {
                            x1 = sprx; y1 = spry;
                            tilenum = getSprite(j)->_picnum;
                            xoff = (int32_t)((int8_t  )((tiles[tilenum].animFlags>>8)&255))+((int32_t)getSprite(j)->_xoffset);
                            if ((getSprite(j)->_cstat&4) > 0)
                                xoff = -xoff;
                            k = getSprite(j)->_ang; l = getSprite(j)->_xrepeat;
                            dax = sintable[k&2047]*l; day = sintable[(k+1536)&2047]*l;
                            l = tiles[tilenum].dim.width;
                            k = (l>>1)+xoff;
                            x1 -= mulscale16(dax,k);
                            x2 = x1+mulscale16(dax,l);
                            y1 -= mulscale16(day,k);
                            y2 = y1+mulscale16(day,l);

                            ox = x1-cposx; oy = y1-cposy;
                            x1 = dmulscale16(ox,xvect,-oy,yvect);
                            y1 = dmulscale16(oy,xvect2,ox,yvect2);

                            ox = x2-cposx; oy = y2-cposy;
                            x2 = dmulscale16(ox,xvect,-oy,yvect);
                            y2 = dmulscale16(oy,xvect2,ox,yvect2);

                            drawline256(x1+(xdim<<11),y1+(ydim<<11),
                                                                                x2+(xdim<<11),y2+(ydim<<11),col);
                        }

                        break;

                    case 32:

                                                tilenum = getSprite(j)->_picnum;
                                                xoff = (int32_t)((int8_t  )((tiles[tilenum].animFlags>>8)&255))+((int32_t)getSprite(j)->_xoffset);
                                                yoff = (int32_t)((int8_t  )((tiles[tilenum].animFlags>>16)&255))+((int32_t)getSprite(j)->_yoffset);
                                                if ((getSprite(j)->_cstat&4) > 0) xoff = -xoff;
                                                if ((getSprite(j)->_cstat&8) > 0) yoff = -yoff;

                                                k = getSprite(j)->_ang;
                                                cosang = sintable[(k+512)&2047];
                                        sinang = sintable[k];
                                                xspan = tiles[tilenum].dim.width;
                                        xrepeat = getSprite(j)->_xrepeat;
                                                yspan = tiles[tilenum].dim.height;
                                        yrepeat = getSprite(j)->_yrepeat;

                                                dax = ((xspan>>1)+xoff)*xrepeat; day = ((yspan>>1)+yoff)*yrepeat;
                                                x1 = sprx + dmulscale16(sinang,dax,cosang,day);
                                                y1 = spry + dmulscale16(sinang,day,-cosang,dax);
                                                l = xspan*xrepeat;
                                                x2 = x1 - mulscale16(sinang,l);
                                                y2 = y1 + mulscale16(cosang,l);
                                                l = yspan*yrepeat;
                                                k = -mulscale16(cosang,l); x3 = x2+k; x4 = x1+k;
                                                k = -mulscale16(sinang,l); y3 = y2+k; y4 = y1+k;

                                                ox = x1-cposx; oy = y1-cposy;
                                                x1 = dmulscale16(ox,xvect,-oy,yvect);
                                                y1 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = x2-cposx; oy = y2-cposy;
                                                x2 = dmulscale16(ox,xvect,-oy,yvect);
                                                y2 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = x3-cposx; oy = y3-cposy;
                                                x3 = dmulscale16(ox,xvect,-oy,yvect);
                                                y3 = dmulscale16(oy,xvect2,ox,yvect2);

                                                ox = x4-cposx; oy = y4-cposy;
                                                x4 = dmulscale16(ox,xvect,-oy,yvect);
                                                y4 = dmulscale16(oy,xvect2,ox,yvect2);

                                                drawline256(x1+(xdim<<11),y1+(ydim<<11),
                                                                                x2+(xdim<<11),y2+(ydim<<11),col);

                                                drawline256(x2+(xdim<<11),y2+(ydim<<11),
                                                                                x3+(xdim<<11),y3+(ydim<<11),col);

                                                drawline256(x3+(xdim<<11),y3+(ydim<<11),
                                                                                x4+(xdim<<11),y4+(ydim<<11),col);

                                                drawline256(x4+(xdim<<11),y4+(ydim<<11),
                                                                                x1+(xdim<<11),y1+(ydim<<11),col);

                                                break;
                                }
                        }
        }

                //Draw white lines
        for(i=0;i<numsectors;i++)
        {
                if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

                startwall = constsector[i].wallptr;
                endwall = constsector[i].wallptr + constsector[i].wallnum;

                k = -1;
                for(j=startwall,wal=getWall(startwall);j<endwall;j++,wal = getWall(j))
                {
                        if (constwall[j].nextwall >= 0) continue;

                        //if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;

                        if (tiles[getWall_picnum(j)].dim.width == 0)
                            continue;
                        if (tiles[getWall_picnum(j)].dim.height== 0)
                            continue;

                        if (j == k)
                                { x1 = x2; y1 = y2; }
                        else
                        {
                                ox = wal->x-cposx; oy = wal->y-cposy;
                                x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                                y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);
                        }

                        k = getWall_point2(j); wal2 = getWall(k);
                        ox = wal2->x-cposx; oy = wal2->y-cposy;
                        x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                        y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

                        drawline256(x1,y1,x2,y2,24);
                }
        }

         for(p=connecthead;p >= 0;p=connectpoint2[p])
         {
          if(ud.scrollmode && p == screenpeek) continue;

          ox = getSprite(ps[p].i)->_x-cposx; oy = getSprite(ps[p].i)->_y-cposy;
                  daang = (getSprite(ps[p].i)->_ang-cang)&2047;
                  if (p == screenpeek) { ox = 0; oy = 0; daang = 0; }
                  x1 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
                  y1 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

          if(p == screenpeek || ud.coop == 1 )
          {
                if(getSprite(ps[p].i)->_xvel > 16 && ps[p].on_ground)
                    i = APLAYERTOP+((totalclock>>4)&3);
                else
                    i = APLAYERTOP;

                j = klabs(ps[p].truefz-ps[p].posz)>>8;
                j = mulscale(czoom*(getSprite(ps[p].i)->_yrepeat+j),yxaspect,16);

                if(j < 22000) j = 22000;
                else if(j > (65536<<1)) j = (65536<<1);

                rotatesprite((x1<<4)+(xdim<<15),(y1<<4)+(ydim<<15),j,
                    daang,i,getSprite(ps[p].i)->_shade,getSprite(ps[p].i)->_pal,
                    (getSprite(ps[p].i)->_cstat&2)>>1,windowx1,windowy1,windowx2,windowy2);
          }
         }
}



void endanimsounds(int32_t fr)
{
    switch(ud.volume_number)
    {
        case 0:break;
        case 1:
            switch(fr)
            {
                case 1:
                    sound(WIND_AMBIENCE);
                    break;
                case 26:
                    sound(ENDSEQVOL2SND1);
                    break;
                case 36:
                    sound(ENDSEQVOL2SND2);
                    break;
                case 54:
                    sound(THUD);
                    break;
                case 62:
                    sound(ENDSEQVOL2SND3);
                    break;
                case 75:
                    sound(ENDSEQVOL2SND4);
                    break;
                case 81:
                    sound(ENDSEQVOL2SND5);
                    break;
                case 115:
                    sound(ENDSEQVOL2SND6);
                    break;
                case 124:
                    sound(ENDSEQVOL2SND7);
                    break;
            }
            break;
        case 2:
            switch(fr)
            {
                case 1:
                    sound(WIND_REPEAT);
                    break;
                case 98:
                    sound(DUKE_GRUNT);
                    break;
                case 82+20:
                    sound(THUD);
                    sound(SQUISHED);
                    break;
                case 104+20:
                    sound(ENDSEQVOL3SND3);
                    break;
                case 114+20:
                    sound(ENDSEQVOL3SND2);
                    break;
                case 158:
                    sound(PIPEBOMB_EXPLODE);
                    break;
            }
            break;
    }
}

void logoanimsounds(int32_t fr)
{
    switch(fr)
    {
        case 1:
            sound(FLY_BY);
            break;
        case 19:
            sound(PIPEBOMB_EXPLODE);
            break;
    }
}

void intro4animsounds(int32_t fr)
{
    switch(fr)
    {
        case 1:
            sound(INTRO4_B);
            break;
        case 12:
        case 34:
            sound(SHORT_CIRCUIT);
            break;
        case 18:
            sound(INTRO4_5);
            break;
    }
}

void first4animsounds(int32_t fr)
{
    switch(fr)
    {
        case 1:
            sound(INTRO4_1);
            break;
        case 12:
            sound(INTRO4_2);
            break;
        case 7:
            sound(INTRO4_3);
            break;
        case 26:
            sound(INTRO4_4);
            break;
    }
}

void intro42animsounds(int32_t fr)
{
    switch(fr)
    {
        case 10:
            sound(INTRO4_6);
            break;
    }
}




void endanimvol41(int32_t fr)
{
    switch(fr)
    {
        case 3:
            sound(DUKE_UNDERWATER);
            break;
        case 35:
            sound(VOL4ENDSND1);
            break;
    }
}

void endanimvol42(int32_t fr)
{
    switch(fr)
    {
        case 11:
            sound(DUKE_UNDERWATER);
            break;
        case 20:
            sound(VOL4ENDSND1);
            break;
        case 39:
            sound(VOL4ENDSND2);
            break;
        case 50:
            FX_StopAllSounds();
            break;
    }
}

void endanimvol43(int32_t fr)
{
    switch(fr)
    {
        case 1:
            sound(BOSS4_DEADSPEECH);
            break;
        case 40:
            sound(VOL4ENDSND1);
            sound(DUKE_UNDERWATER);
            break;
        case 50:
            sound(BIGBANG);
            break;
    }
}
void drawBufferToFrameVscale(uint8_t *data, int tileWidth, int tileHeight, int drawH)
{
    float coeff = (float) tileHeight / drawH;
    int yy = 0;

    for (int y = drawH - 1; y >= 0; y--)
    {
        yy = coeff * y;
            memcpy(&frameplace[SCREENWIDTH * y], &data[yy * tileWidth], tileWidth);
    }
}

int32_t lastanimhack=0;
void playanm(char  *fn,uint8_t  t)
{
#if MANY_FRAMES
    #define TILE_FRAME (MAXTILES-3-t)
#else
    #define TILE_FRAME (MAXTILES-3)
#endif // MANY_FRAMES
#if USE_EXT_MEM  && MCU_DUKE
    uint8_t  *animbuf, *palptr;
    int32_t i, j, k, numframes=0;
    uint32_t length=0;
    if(t != 7 && t != 9 && t != 10 && t != 11)
        KB_FlushKeyboardQueue();

    if( KB_KeyWaiting() )
    {
        FX_StopAllSounds();
        return; //goto ENDOFANIMLOOP;
    }

    uint8_t *p = kGetExtMemAddress(fn, &length);
    if (p == NULL)
		return;

    printf("Anin length is %d\r\n", length);

    animbuf = p;
#if !REUSE_ARRAYS
    anim_t stackAnim;
    anim = &stackAnim;
#else
// next-hack: yes I know. This is horrible, but anim is not played when sprites are used. So we can use it here.
    anim = (anim_t*) ramsprites;    //
#endif

    lastanimhack = (TILE_FRAME);

   ANIM_LoadAnim (animbuf);


    numframes = anim->lpheader.nRecords;

    palptr = anim->pal;
    uint8_t tempbuf[256 * 4];
    for(i=0;i<256;i++)
    {
            j = 4 * i;
            k = j-i;
            tempbuf[j+0] = (palptr[k+2]>>2);
            tempbuf[j+1] = (palptr[k+1]>>2);
            tempbuf[j+2] = (palptr[k+0]>>2);
            tempbuf[j+3] = 0;
    }

    VBE_setPalette(tempbuf);

    ototalclock = totalclock + 10;

    for(i=1;i<numframes;i++)
    {
       while(totalclock < ototalclock)
       {
          if( KB_KeyWaiting() )
              return; //goto ENDOFANIMLOOP;
          updateAudio();
          getpackets();
       }

       if(t == 10) ototalclock += 14;
       else if(t == 9) ototalclock += 10;
       else if(t == 7) ototalclock += 18;
       else if(t == 6) ototalclock += 14;
       else if(t == 5) ototalclock += 9;
       else if(ud.volume_number == 3) ototalclock += 10;
       else if(ud.volume_number == 2) ototalclock += 10;
       else if(ud.volume_number == 1) ototalclock += 18;
       else                           ototalclock += 10;

#if WIN32
       uint8_t *backFrame = frameplace;
#else
       uint8_t *backFrame =  getDisplayFrameBuffer(0);  // this will be the other frame.
#endif
        anim->imagebuffer = backFrame;
        //
        ANIM_DrawFrame(i);
       nextpage(SCREEN_HEIGHT);
       waitForUpdate(-1);

       if(t == 8) endanimvol41(i);
       else if(t == 10) endanimvol42(i);
       else if(t == 11) endanimvol43(i);
       else if(t == 9) intro42animsounds(i);
       else if(t == 7) intro4animsounds(i);
       else if(t == 6) first4animsounds(i);
       else if(t == 5) logoanimsounds(i);
       else if(t < 4) endanimsounds(i);
        }

   // ENDOFANIMLOOP:
   // ;
//    ANIM_FreeAnim ();
//    tiles[TILE_FRAME].lock = 1;

#else
    uint8_t  *animbuf, *palptr;
    int32_t i, j, k, length=0, numframes=0;
    int32_t handle=-1;

    if(t != 7 && t != 9 && t != 10 && t != 11)
        KB_FlushKeyboardQueue();

    if( KB_KeyWaiting() )
    {
        FX_StopAllSounds();
        goto ENDOFANIMLOOP;
    }

/*disabled*/    handle = TCkopen4load(fn, LOAD_ONLY_GRP);

    if(handle == -1)
		return;

    length = kfilelength(handle);
    printf("File Length %d\r\n", length);
    tiles[TILE_FRAME].lock = 219+t;

    if(anim == 0 || lastanimhack != (TILE_FRAME))
/*not with EXT_MEM*/        allocache((uint8_t**)&anim,length+sizeof(anim_t),&tiles[TILE_FRAME].lock);

    animbuf = (uint8_t  *)(anim)+sizeof(anim_t);

    lastanimhack = (TILE_FRAME);

    tiles[TILE_FRAME].dim.width = 200;
    tiles[TILE_FRAME].dim.height = 320;

    kread(handle,animbuf,length);
    kclose(handle);

    ANIM_LoadAnim (animbuf);
    numframes = ANIM_NumFrames();


    palptr = ANIM_GetPalette();
    for(i=0;i<256;i++)
    {
            j = (i<<2); k = j-i;
            tempbuf[j+0] = (palptr[k+2]>>2);
            tempbuf[j+1] = (palptr[k+1]>>2);
            tempbuf[j+2] = (palptr[k+0]>>2);
            tempbuf[j+3] = 0;
    }

    VBE_setPalette(tempbuf);

    ototalclock = totalclock + 10;

    for(i=1;i<numframes;i++)
    {
       while(totalclock < ototalclock)
       {
          if( KB_KeyWaiting() )
              goto ENDOFANIMLOOP;
          getpackets();
       }

       if(t == 10) ototalclock += 14;
       else if(t == 9) ototalclock += 10;
       else if(t == 7) ototalclock += 18;
       else if(t == 6) ototalclock += 14;
       else if(t == 5) ototalclock += 9;
       else if(ud.volume_number == 3) ototalclock += 10;
       else if(ud.volume_number == 2) ototalclock += 10;
       else if(ud.volume_number == 1) ototalclock += 18;
       else                           ototalclock += 10;
#if !MCU_DUKE

       tiles[TILE_FRAME].data = ANIM_DrawFrame(i);
//       Let's do like normal people, shall we?
//       rotatesprite(0<<16,0<<16,65536L,512,TILE_FRAME,0,0,2+4+8+16+64, 0,0,xdim-1,ydim-1);

       drawBufferToFrameVscale(frameplace, 320, 200, ScreenHeight);
      // memcpy(frameplace, tiles[TILE_FRAME].data, 320 * 200);
#else
#if WIN32
    uint8_t backFrame[320 * 200];   // this will be the other frame.
#endif
    anim->imagebuffer = backFrame;
    ANIM_DrawFrame(i);
    drawBufferToFrameVscale(backFrame, 320, 200, ScreenHeight);
#endif
       nextpage(SCREEN_HEIGHT);

       if(t == 8) endanimvol41(i);
       else if(t == 10) endanimvol42(i);
       else if(t == 11) endanimvol43(i);
       else if(t == 9) intro42animsounds(i);
       else if(t == 7) intro4animsounds(i);
       else if(t == 6) first4animsounds(i);
       else if(t == 5) logoanimsounds(i);
       else if(t < 4) endanimsounds(i);
        }

    ENDOFANIMLOOP:

    ANIM_FreeAnim ();
    tiles[TILE_FRAME].lock = 1;

#endif // USE_EXT_MEM
}

