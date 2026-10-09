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
#ifdef WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
#include <stdlib.h>

#include "duke3d.h"
#include "../../Game/src/types.h"
#include "audiolib/fx_man.h"
#include "audiolib/music.h"
#include "audiolib/sndcards.h"
#include "control.h"
#include "engine.h"
#include "filesystem.h"
#include "global.h"
#include "sounds.h"
#include "soundefs.h"
//
#include "internalFlash.h"
#include "audiolib/multivoc.h"

#define LOUDESTVOLUME 150

int32_t backflag,numenvsnds;

/*
===================
=
= SoundStartup
=
===================
*/

void SoundStartup(void)
{

    // if they chose None lets return
    if (FXDevice == SC_Unknown) return;
#if WIN32
    int32_t status;

    status = FX_Init(FXDevice, MAX_VOICES, NumChannels, NumBits, MixRate);

    if (status != FX_Ok)
    {
        Error(EXIT_FAILURE, FX_ErrorString(FX_Error));
    }

    FX_SetVolume(FXVolume);
    if (ReverseStereo == 1)
    {
        FX_SetReverseStereo(!FX_GetReverseStereo());
    }

    status = FX_SetCallBack(TestCallBack);

    if (status != FX_Ok)
    {
        Error(EXIT_FAILURE, FX_ErrorString(FX_Error));
    }
#else
    FX_Init(FXDevice, MAX_VOICES, NumChannels, NumBits, MixRate);
    FX_SetVolume(FXVolume);
#endif

    //
#if USE_EXT_MEM
    SAMPLE stack_sound[NUM_SOUNDS] = {0};
    Sound =  stack_sound;
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        loadsound(i, &stack_sound[i]);
    }
    Sound =   storeToInternalFlash(stack_sound, sizeof(stack_sound));
#endif
}

/*
===================
=
= SoundShutdown
=
===================
*/

void SoundShutdown( void )
   {
#if WIN32
   int32_t status;

   // if they chose None lets return
   if (FXDevice == SC_Unknown)
      return;

   status = FX_Shutdown();
   if ( status != FX_Ok )
      {
      Error(EXIT_FAILURE, FX_ErrorString( FX_Error ));
      }
#endif
 }

/*
===================
=
= MusicStartup
=
===================
*/

void MusicStartup(void)
{
    int32_t status;

    // if they chose None lets return
    if (MusicDevice == SC_Unknown)
        return;
    status = MUSIC_Init(MusicDevice, MidiPort);
    if (status == MUSIC_Ok)
    {
        MUSIC_SetVolume(MusicVolume);
    }
    else
    {
        SoundShutdown();
        uninittimer();
        uninitengine();
        CONTROL_Shutdown();
        CONFIG_WriteSetup();
        KB_Shutdown();
#if !USE_EXT_MEM
        uninitgroupfile();
        unlink("duke3d.tmp");
#endif
        Error(EXIT_FAILURE, "Couldn't find selected sound card, or, error w/ sound card itself\n");
    }
}

/*
===================
=
= MusicShutdown
=
===================
*/

void MusicShutdown(void)
{
    int32_t status;

    // if they chose None lets return
    if (MusicDevice == SC_Unknown)
        return;

    status = MUSIC_Shutdown();
    if (status != MUSIC_Ok)
    {
        //Error( MUSIC_ErrorString( MUSIC_ErrorCode ));
    }
}
#if IMPLEMENTED_ELSEWHERE

int USRHOOKS_GetMem(void  **ptr, uint32_t size )
{
   *ptr = malloc(size);

   if (*ptr == NULL)
      return(USRHOOKS_Error);

   return( USRHOOKS_Ok);

}

int USRHOOKS_FreeMem(void  *ptr)
{
   free(ptr);
   return( USRHOOKS_Ok);
}
#endif
uint8_t  menunum=0;

void intomenusounds(void)
{
    static const short menusnds[] =
    {
        LASERTRIP_EXPLODE,
        DUKE_GRUNT,
        DUKE_LAND_HURT,
        CHAINGUN_FIRE,
        SQUISHED,
        KICK_HIT,
        PISTOL_RICOCHET,
        PISTOL_BODYHIT,
        PISTOL_FIRE,
        SHOTGUN_FIRE,
        BOS1_WALK,
        RPG_EXPLODE,
        PIPEBOMB_BOUNCE,
        PIPEBOMB_EXPLODE,
        NITEVISION_ONOFF,
        RPG_SHOOT,
        SELECT_WEAPON
    };
    sound(menusnds[(int)menunum++]);
    menunum %= 17;
}

void playmusic(const char  *fn)
{
    if(MusicToggle == 0) return;
    if(MusicDevice == SC_Unknown) return;

    // the SDL_mixer version does more or less this same thing.  --ryan.
    PlayMusic(fn);
}

uint8_t  loadsound(uint16_t num, SAMPLE *smpl)  // next-hack: modified so that it will fill ptr and size
{
    #if USE_EXT_MEM
        uint32_t size;
        if(num >= NUM_SOUNDS)
            return 0;
        //
#if !USE_VOC
        // we need to change .VOC in .WAV
        char fileName[sizeof(sounds[0])];
        memcpy(fileName, sounds[num], sizeof(fileName));
        int length = strlen(fileName);   // get length
        if (length <= 4)        // no .voc
            return 0;
        fileName[length - 3] = 'w';
        fileName[length - 2] = 'a';
        fileName[length - 1] = 'v';
//        printf("Loading %s\r\n", fileName);
        void * p =kGetExtMemAddress(fileName, &size);
#else
        void * p =kGetExtMemAddress(sounds[num], &size);
#endif
        if (Sound[num].ptr)  // already loaded
            return 1;
        if (!p)
            return 0;
        //Different audio drivers.
#if WIN32

        smpl->length = size;

#else
        smpl->length = size - WAV_DATA_OFFSET;
#endif
        smpl->ptr = p;

        return 1;
    #else
        int32_t   fp, l;
        if(num >= NUM_SOUNDS || SoundToggle == 0) return 0;
        if (FXDevice == SC_Unknown) return 0;
//printf("load sound %s %d\r\n", sounds[num], num);

/*disabled*/        fp = TCkopen4load(sounds[num], LOAD_ONLY_GRP);
        if(fp == -1)
        {
#if RAM_FTA_QUOTES
            sprintf(&fta_quotes[113][0],"Sound %s(#%d) not found.",sounds[num],num);
            FTA(113,&ps[myconnectindex],1);
#else
            sprintf(get_fta_quotes(CUSTOM_FTA),"Sound %s(#%d) not found.",sounds[num],num);
            FTA(CUSTOM_FTA,&ps[myconnectindex],1);
#endif
            return 0;
        }

        l = kfilelength( fp );
        soundsiz[num] = l;

        Sound[num].lock = 200;

#error: disabled        allocache(&Sound[num].ptr,l,(uint8_t  *)&Sound[num].lock);
        kread( fp, Sound[num].ptr , l);
        kclose( fp );
        return 1;

    #endif // USE_EXT_MEM
}
int fixLoopedSound(int soundnum)
{
    // next-hack: DN3D relied on the engine actually starting the looped sound, as opposed to let the sound loop running
    // this might cause a pause when the sample ends.
    // the list was manually created. It might contain wrong or msising items.
    switch (soundnum)
    {
        default:
            return false;
        case DUKE_JETPACK_IDLE:
        case RECO_ROAM:
        case DRON_JETSND:
        case ENGINE_OPERATING:
        case REACTOR_ON:
        case COMPUTER_AMBIENCE:
        case GEARS_GRINDING:
        case BUBBLE_AMBIENCE:
        case MACHINE_AMBIENCE:
        case SEWER_AMBIENCE:
        case WIND_AMBIENCE:
        case WIND_REPEAT:
        case STEAM_HISSING:
        case BAR_MUSIC:
        case STORE_MUSIC:
        case STORE_MUSIC_BROKE:
        case WATER_GURGLE:
        case GENERIC_AMBIENCE1:
        case GENERIC_AMBIENCE2:
        case GENERIC_AMBIENCE3:
        case GENERIC_AMBIENCE4:
        case GENERIC_AMBIENCE8:
        case GENERIC_AMBIENCE10:
        case GENERIC_AMBIENCE11:
        case GENERIC_AMBIENCE19:
        case FIRE_CRACKLE:
        case SPACE_AMBIENCE1:
        case SPACE_AMBIENCE2:
            return true;
    }
}
#if WIN32
int xyzsound2(short num,short i,int32_t x,int32_t y,int32_t z, const char *fun, const char *file, int line)
#else
int xyzsound(short num,short i,int32_t x,int32_t y,int32_t z)
#endif // WIN32
{
    #if WIN32 && 0
        printf("xyzsound n = %d i = %d x = %d y = %d z = %d fun = %s  file =%s at line %d\r\n",num, i, x, y, z, fun, file, line);
    #endif
    int32_t sndist, cx, cy, cz, j,k;
    short pitche,pitchs,cs;
    int voice, sndang, ca, pitch;

    if( num >= NUM_SOUNDS ||
        FXDevice == SC_Unknown ||
        ( (soundm[num]&8) && ud.lockout ) ||
        SoundToggle == 0 ||
//        Sound[num].num > 3 ||
        getSoundNum(num) > 3 ||
        FX_VoiceAvailable(soundpr[num]) == 0 ||
        (ps[myconnectindex].timebeforeexit > 0 && ps[myconnectindex].timebeforeexit <= 26*3) ||
        ps[myconnectindex].gm&MODE_MENU) return -1;

    if( soundm[num]&128 )
    {
        sound(num);
        return 0;
    }

    if( soundm[num]&4 )
    {
		// FIX_00041: Toggle to hear the opponent sound in DM (like it used to be in v1.3d)
		if(VoiceToggle==0 || (ud.multimode > 1 && PN == APLAYER && getSprite(i)->_yvel != screenpeek && /*ud.coop!=1 &&*/ !OpponentSoundToggle) ) return -1; //xduke : 1.3d Style: makes opponent sound in DM as in COOP

        for(j=0;j<NUM_SOUNDS;j++)
//          for(k=0;k<Sound[j].num;k++)
          for(k = 0; k < getSoundNum(j); k++)
 //           if( (Sound[j].num > 0) && (soundm[j]&4) )
            if( (getSoundNum(j) > 0) && (soundm[j]&4) )
              return -1;
    }

    cx = ps[screenpeek].oposx;
    cy = ps[screenpeek].oposy;
    cz = ps[screenpeek].oposz;
    cs = ps[screenpeek].cursectnum;
    ca = ps[screenpeek].ang+ps[screenpeek].look_ang;

    sndist = FindDistance3D((cx-x),(cy-y),(cz-z)>>4);

    if( i >= 0 && (soundm[num]&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (getSector(SECT)->lotag&0xff) < 9 )
        sndist = divscale14(sndist,(SHT+1));

    pitchs = soundps[num];
    pitche = soundpe[num];
    cx = klabs(pitche-pitchs);

    if(cx)
    {
        if( pitchs < pitche )
             pitch = pitchs + ( rand()%cx );
        else pitch = pitche + ( rand()%cx );
    }
    else pitch = pitchs;

    sndist += soundvo[num];
    if(sndist < 0) sndist = 0;
    if( sndist && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,SX,SY,SZ-(24<<8),SECT) )
        sndist += sndist>>5;

    switch(num)
    {
        case PIPEBOMB_EXPLODE:
        case LASERTRIP_EXPLODE:
        case RPG_EXPLODE:
            if(sndist > (6144) )
                sndist = 6144;
            if(getSector(ps[screenpeek].cursectnum)->lotag == 2)
                pitch -= 1024;
            break;
        default:
            if(getSector(ps[screenpeek].cursectnum)->lotag == 2 && (soundm[num]&4) == 0)
                pitch = -768;
            if( sndist > 31444 && PN != MUSICANDSFX)
                return -1;
            break;
    }


//    if( Sound[num].num > 0 && PN != MUSICANDSFX )
    if( getSoundNum(num) > 0 && PN != MUSICANDSFX )
    {
#if USE_SOUNDOWNER
        if( SoundOwner[num][0].i == i ) stopsound(num);
#else
        if (getSoundOwnerFromSoundNum(num) == i) stopsound(num);
#endif
//        else if( Sound[num].num > 1 ) stopsound(num);
        else if( getSoundNum(num) > 1 ) stopsound(num);
        else if( badguy(getSprite(i)) && getSprite_extra(i) <= 0 ) stopsound(num);
    }

    if( PN == APLAYER && getSprite(i)->_yvel == screenpeek )
    {
        sndang = 0;
        sndist = 0;
    }
    else
    {
        sndang = 2048 + ca - getangle(cx-x,cy-y);
        sndang &= 2047;
    }

//    if(Sound[num].ptr == 0) { if( loadsound(num) == 0 ) return 0; } // next-hack removed. The sound are already all preloaded.
#if USE_CACHE
    else
    {
       if (Sound[num].lock < 200)
          Sound[num].lock = 200;
       else Sound[num].lock++;
    }
#endif
    if( soundm[num]&16 ) sndist = 0;

    if(sndist < ((255-LOUDESTVOLUME)<<6) )
        sndist = ((255-LOUDESTVOLUME)<<6);

    if ((soundm[num] & 1) || fixLoopedSound(num))
    {
        uint16_t start = 0;
//        if(Sound[num].num > 0) return -1;
        if(getSoundNum(num) > 0) return -1;
//        start = *(uint16_t *)(Sound[num].ptr + 0x14); // next-hack: this only works for VOCs!
#if USE_VOC
        if(*Sound[num].ptr == 'C')
            voice = FX_PlayLoopedVOC( Sound[num].ptr, start, start + soundsiz[num],
                    pitch,sndist>>6,sndist>>6,0,soundpr[num],num);
        else
#endif
            voice = FX_PlayLoopedWAV( Sound[num].ptr, start, start + Sound[num].length,
                    pitch,sndist>>6,sndist>>6,0,soundpr[num],num);
    }
    else
    {
#if USE_VOC
        if( *Sound[num].ptr == 'C')
            voice = FX_PlayVOC3D( Sound[ num ].ptr,pitch,sndang>>6,sndist>>6, Sound[num].length, num );
        else
#endif
            voice = FX_PlayWAV3D( Sound[ num ].ptr,pitch,sndang>>6,sndist>>6, Sound[num].length, num );
    }

    if ( voice > FX_Ok )
    {
//        SoundOwner[num][Sound[num].num].i = i;
        unsigned int sn = getSoundNum(num);
#if USE_SOUNDOWNER
        SoundOwner[num][sn].i = i;
        SoundOwner[num][sn].voice = voice;
#else
        setVoiceSoundOwner(voice, i);
#endif
//        SoundOwner[num][Sound[num].num].voice = voice;
   //
        sn++;
//        Sound[num].num++;
        setSoundNum(num, sn);
    }
#if USE_CACHE
    else Sound[num].lock--;
#endif
    return (voice);
}

int sound(short num)
{
    short pitch,pitche,pitchs,cx;
    int voice;
    int32_t start;

    if (FXDevice == SC_Unknown) return -1;
    if(SoundToggle==0) return 0;
    if(VoiceToggle==0 && (soundm[num]&4) ) return 0;
    if( (soundm[num]&8) && ud.lockout ) return -1;

    if(FX_VoiceAvailable(soundpr[num]) == 0) return 0;
    pitchs = soundps[num];
    pitche = soundpe[num];
    cx = klabs(pitche-pitchs);

    if(cx)
    {
        if( pitchs < pitche )
             pitch = pitchs + ( rand()%cx );
        else pitch = pitche + ( rand()%cx );
    }
    else pitch = pitchs;

//    if(Sound[num].ptr == 0) { if( loadsound(num) == 0 ) return 0; }  // next-hack removed. The sound are already all preloaded.
#if USE_CACHE
    else
    {
       if (Sound[num].lock < 200)
          Sound[num].lock = 200;
       else Sound[num].lock++;
    }
#endif
    if ((soundm[num] & 1) || fixLoopedSound(num))
    {
#if USE_VOC
        if(*Sound[num].ptr == 'C')
        {
            start = (int32_t)*(uint16_t *)(Sound[num].ptr + 0x14);
            voice = FX_PlayLoopedVOC( Sound[num].ptr, start, start + Sound[num].length,
                    pitch,LOUDESTVOLUME,LOUDESTVOLUME,LOUDESTVOLUME,soundpr[num],num);
        }
        else
#endif
        {

            //start = (int32_t)*(uint16_t *)(Sound[num].ptr + 0x14);  next-hack: I say it again. This crap works only for VOCS
            start = 0;
            voice = FX_PlayLoopedWAV( Sound[num].ptr, start, start + Sound[num].length,
                    pitch,LOUDESTVOLUME,LOUDESTVOLUME,LOUDESTVOLUME,soundpr[num],num);
        }
    }
    else
    {
#if USE_VOC
        if(*Sound[num].ptr == 'C')
            voice = FX_PlayVOC3D( Sound[ num ].ptr, pitch,0,255-LOUDESTVOLUME,soundpr[num], num );
        else
#endif
            voice = FX_PlayWAV3D( Sound[ num ].ptr, pitch,0,255-LOUDESTVOLUME,soundpr[num], num );
    }

    if(voice > FX_Ok) return voice;
#if USE_CACHE
    Sound[num].lock--;
#endif
    return 0;
}
#if WIN32
int spritesound2(uint16_t num, short i, const char *fun, const char *file, int line)

#else
int spritesound(uint16_t num, short i)
#endif // WIN32
{
#if WIN32 && 0
    printf("spritesound: %d %d %s %s %d\r\n", num, i, fun, file, line);
 #endif
    if(num >= NUM_SOUNDS) return -1;
    return xyzsound(num,i,SX,SY,SZ);
}

void stopsound(short num)
{
#if USE_SOUNDOWNER
//    if(Sound[num].num > 0)
    unsigned int sn = getSoundNum(num);
    if(sn > 0)
    {
//      FX_StopSound(SoundOwner[num][Sound[num].num-1].voice);
        FX_StopSound(SoundOwner[num][sn-1].voice);
        testcallback(num);
    }
#else
    // next-hack: fixme this is not functionally the same.
    for (int i = 1; i <= MAX_VOICES; i++)
    {
        if (getVoiceSoundNum(i) == num)
        {
            FX_StopSound(i);
            break;
        }
    }
    testcallback(num);

#endif
}

void stopenvsound(short num,short i)
{
    short k;
    k = getSoundNum(num);
//    if(Sound[num].num > 0)
    if (k > 0)
    {
//        k = Sound[num].num;
#if USE_SOUNDOWNER
        for(j=0;j<k;j++)
           if(SoundOwner[num][j].i == i)
        {
            FX_StopSound(SoundOwner[num][j].voice);
            break;
        }
#else
        for (int v = 1; v <= MAX_VOICES; v++)
        {
            if (getVoiceSoundOwner(v) == i  && getVoiceSoundNum(v) == num)
            {
                FX_StopSound(v);
                break;
            }
        }
#endif
    }
}

void pan3dsound(void)
{
    int32_t sndist, sx, sy, sz, cx, cy, cz;
    short sndang,ca,j,i,cs;

    numenvsnds = 0;

    if(ud.camerasprite == -1)
    {
        cx = ps[screenpeek].oposx;
        cy = ps[screenpeek].oposy;
        cz = ps[screenpeek].oposz;
        cs = ps[screenpeek].cursectnum;
        ca = ps[screenpeek].ang+ps[screenpeek].look_ang;
    }
    else
    {
        cx = getSprite(ud.camerasprite)->_x;
        cy = getSprite(ud.camerasprite)->_y;
        cz = getSprite(ud.camerasprite)->_z;
        cs = getSprite(ud.camerasprite)->_sectnum;
        ca = getSprite(ud.camerasprite)->_ang;
    }

//    for(j=0;j<NUM_SOUNDS;j++) for(k=0;k<Sound[j].num;k++)
#if USE_SOUNDOWNER
    for(j=0;j<NUM_SOUNDS;j++) for(k = 0; k < getSoundNum(j); k++)
    {
        i = SoundOwner[j][k].i;

        sx = getSprite(i)->_x;
        sy = getSprite(i)->_y;
        sz = getSprite(i)->_z;

        if( PN == APLAYER && getSprite(i)->_yvel == screenpeek)
        {
            sndang = 0;
            sndist = 0;
        }
        else
        {
            sndang = 2048 + ca - getangle(cx-sx,cy-sy);
            sndang &= 2047;
            sndist = FindDistance3D((cx-sx),(cy-sy),(cz-sz)>>4);
            if( i >= 0 && (soundm[j]&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (getSector(SECT)->lotag&0xff) < 9 )
                sndist = divscale14(sndist,(SHT+1));
        }

        sndist += soundvo[j];
        if(sndist < 0) sndist = 0;

        if( sndist && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,sx,sy,sz-(24<<8),SECT) )
            sndist += sndist>>5;

        if(PN == MUSICANDSFX && SLT < 999)
            numenvsnds++;

        switch(j)
        {
            case PIPEBOMB_EXPLODE:
            case LASERTRIP_EXPLODE:
            case RPG_EXPLODE:
                if(sndist > (6144)) sndist = (6144);
                break;
            default:
                if( sndist > 31444 && PN != MUSICANDSFX)
                {
                    stopsound(j);
                    continue;
                }
        }

//        if(Sound[j].ptr == 0 && loadsound(j) == 0 ) continue; // next-hack removed. The sound are already all preloaded.
        if( soundm[j]&16 ) sndist = 0;

        if(sndist < ((255-LOUDESTVOLUME)<<6) )
            sndist = ((255-LOUDESTVOLUME)<<6);

        FX_Pan3D(SoundOwner[j][k].voice,sndang>>6,sndist>>6);
    }
#else
    //for(j=0;j<NUM_SOUNDS;j++) for(k = 0; k < getSoundNum(j); k++)
    for (int voiceNum = 1; voiceNum <= MAX_VOICES; voiceNum++)
    {
        if (!MV_VoicePlaying(voiceNum))
        {
            continue;
        }
        i = getVoiceSoundOwner(voiceNum);
        if (i < 0)
            continue;
        j = getVoiceSoundNum(voiceNum);
        if (j == NUM_SOUNDS)
            continue;
        sx = getSprite(i)->_x;
        sy = getSprite(i)->_y;
        sz = getSprite(i)->_z;

        if( PN == APLAYER && getSprite(i)->_yvel == screenpeek)
        {
            sndang = 0;
            sndist = 0;
        }
        else
        {
            sndang = 2048 + ca - getangle(cx-sx,cy-sy);
            sndang &= 2047;
            sndist = FindDistance3D((cx-sx),(cy-sy),(cz-sz)>>4);
            if( i >= 0 && (soundm[j]&16) == 0 && PN == MUSICANDSFX && SLT < 999 && (getSector(SECT)->lotag&0xff) < 9 )
                sndist = divscale14(sndist,(SHT+1));
        }

        sndist += soundvo[j];
        if(sndist < 0) sndist = 0;

        if( sndist && PN != MUSICANDSFX && !cansee(cx,cy,cz-(24<<8),cs,sx,sy,sz-(24<<8),SECT) )
            sndist += sndist>>5;

        if(PN == MUSICANDSFX && SLT < 999)
            numenvsnds++;

        switch(j)
        {
            case PIPEBOMB_EXPLODE:
            case LASERTRIP_EXPLODE:
            case RPG_EXPLODE:
                if(sndist > (6144)) sndist = (6144);
                break;
            default:
                if( sndist > 31444 && PN != MUSICANDSFX)
                {
                    stopsound(j);
                    continue;
                }
        }

//        if(Sound[j].ptr == 0 && loadsound(j) == 0 ) continue; // next-hack removed. The sound are already all preloaded.
        if( soundm[j]&16 ) sndist = 0;

        if(sndist < ((255-LOUDESTVOLUME)<<6) )
            sndist = ((255-LOUDESTVOLUME)<<6);

        FX_Pan3D(voiceNum, sndang>>6, sndist>>6);
    }

#endif
}

void TestCallBack(int32_t num)
{
    short spritenum, soundNum;

        if(num < 0)
        {
#if USE_RTS
            if(lumplockbyte[-num] >= 200)
                lumplockbyte[-num]--;
#endif
            return;
        }
#if USE_SOUNDOWNER
//        tempk = Sound[num].num;

        soundNum = getSoundNum(num);
        if(soundNum > 0)
        {
            if( (soundm[num]&16) == 0)  // i.e. not some warambient sounds etc.
            {
                for(tempj=0; tempj < soundNum; tempj++)
                {
                    spritenum = SoundOwner[num][tempj].i;
                    if(getSprite(spritenum)->_picnum == MUSICANDSFX && getSector(getSprite(spritenum)->_sectnum)->lotag < 3 && getSprite(spritenum)->_lotag < 999)
                    {
                        setHittypeTemp(spritenum, 0, 0);
                        // remove this voice by copying the last one (the soundnum will be decremented)
                        if( (tempj + 1) < soundNum )
                        {
                            SoundOwner[num][tempj].voice = SoundOwner[num][soundNum-1].voice;
                            SoundOwner[num][tempj].i     = SoundOwner[num][soundNum-1].i;
                    }
                    break;
                }
            }
            }
//            Sound[num].num--;
//            SoundOwner[num][tempk-1].i = -1;
            soundNum--;getSprite
            setSoundNum(num, soundNum);        // decrement number of entries
            SoundOwner[num][soundNum].i = -1;  //invalidate owner of last entry + 1.
        }
#else
//        tempk = Sound[num].num;

        soundNum = getSoundNum(num);
        if(soundNum > 0)
        {
            if( (soundm[num]&16) == 0)  // i.e. not some warambient sounds etc.
            {

                for(int voiceNum = 1; voiceNum < MAX_VOICES; voiceNum++)
                {
                    // check if the voice is playing the sound
                    if (getVoiceSoundNum(voiceNum) != num)
                    {
                        continue;
                    }
                    spritenum = getVoiceSoundOwner(voiceNum);
                    if (spritenum < 0)
                        continue;
                    if(getSprite(spritenum)->_picnum == MUSICANDSFX && getSector(getSprite(spritenum)->_sectnum)->lotag < 3 && getSprite(spritenum)->_lotag < 999)
                    {
                        setHittypeTemp(spritenum, 0, 0);
                        setVoiceSoundOwner(voiceNum, -1); //needed?
                        break;
                    }
                }
            }
            soundNum--;
            setSoundNum(num, soundNum);        // decrement number of entries
        }

#endif
#if USE_CACHE
        Sound[num].lock--;
#endif
}


// no idea if this is right. I added this function.  --ryan.
void testcallback(uint32_t num)
{
//    STUBBED("wtf?");
    TestCallBack(num);
}
volatile int soundVoiceActive;
int isSoundVoiceActive(int n)
{
    if (n < 0)
        return 0;
    if (n > 32)
        return 0;
    return 0 != (soundVoiceActive & (1 << n ));
}


void clearsoundlocks(void)
{
#if !USE_EXT_MEM

    for(i=0;i<NUM_SOUNDS;i++)
            Sound[i].lock = 199;
#else
   soundVoiceActive = 0;
#endif
#if USE_RTS
    for(i=0;i<11;i++)

            lumplockbyte[i] = 199;
#endif
}

