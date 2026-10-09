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
#define PRINT_MENU_CALLS 0
#ifdef WIN32
#include <stdio.h>
#include <time.h>
#else
#include "graphics.h"
#include <printf.h>
#endif
#include <stdlib.h>
uint8_t sbarUpdated = 0;
#ifdef _WIN32
	#include <conio.h>
	#include <direct.h>
	#include <fcntl.h>
	#include <windows.h>
#elif defined(__APPLE__)
  #include "SDL.h"
#endif

#include "types.h"
#include <ctype.h>
#include "duke3d.h"
#include "audiolib/fx_man.h"
#include "audiolib/music.h"
#include "audiolib/sndcards.h"
#include "config.h"
#include "console.h"
#include "engine.h"
#include "file_lib.h"
#include "gamedefs.h"
#include "global.h"
#include "keyboard.h"
#include "util_lib.h"
#include "function.h"
#include "control.h"
#include "sounds.h"
#include "soundefs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include "internalFlash.h"
#if MG24
#include "radio.h"
#endif
#include "audiolib/multivoc.h"
//
#define MINITEXT_BLUE	0
#define MINITEXT_RED	2
#define MINITEXT_YELLOW	23
#define MINITEXT_GRAY	17

#define COLOR_ON  MINITEXT_YELLOW
#define COLOR_OFF MINITEXT_BLUE

#define IDFSIZE 479985668
// #define IDFSIZE 9961476
// #define IDFSIZE 16384
#define IDFILENAME "DUKE3D.IDF"

#define TIMERUPDATESIZ 32
#define MAX_CONNECTION_DELAY        16              // how many ticks other can be behind us. Was 200 but with fifosize of 256...

char ftaCustomText[MAX_FTA_CUSTOM_TEXT_SIZE];       // next-hack: just to be able to write to FTAs.
int32_t cameradist = 0, cameraclock = 0;
uint8_t  playerswhenstarted;
uint8_t  qe,cp;
uint8_t  multiplayerConnection = 0;
uint8_t  nHostForceDisableAutoaim = 0;

// Game play speed
int g_iTickRate = 120;      // this shall be ticks per  second.
int g_iTicksPerFrame = 26;  // this is frames per seconds

int32_t CommandSoundToggleOff = 0;
int32_t CommandMusicToggleOff = 0;

// For addfaz's stun server. use /stun to activate
uint16_t g_bStun = 0;
#if LOAD_FROM_CON_FILE
char confilename[128] = {"GAME.CON"};
#endif
char boardfilename[BOARD_FILENAME_LENGTH] = {0};
#if PALETTES_IN_RAM
uint8_t  waterpal[768],slimepal[768],titlepal[768],drealms[768],endingpal[768];
#else
uint8_t  *waterpal,*slimepal,*titlepal,*drealms,*endingpal;
#endif
char  firstdemofile[80] = { '\0' };

#define patchstatusbar(x1,y1,x2,y2)                                        \
{                                                                          \
    rotatesprite(0,(200-34)<<16,65536L,0,BOTTOMSTATUSBAR,4,0,10+16+64+128, \
        scale(x1,xdim,320),scale(y1,ydim,200),                             \
        scaleup(x2,xdim,320)-1,scaleup(y2,ydim,200)-1);                    \
}

void newint24( int errval, int ax, int bp, int si );

int recfilep,totalreccnt;
uint8_t  debug_on = 0,actor_tog = 0,memorycheckoveride=0;
uint8_t *rtsptr;
#if WIN32
int htmaxData[6], htminData[6];
#endif


extern uint8_t  syncstate;
#if USE_RTS
extern int32_t numlumps;
#endif
#if WIN32
FILE *frecfilep = (FILE *)NULL;
#endif
void pitch_test( void );

uint8_t  restorepalette,screencapt,nomorelogohack;
int sendmessagecommand = -1;

/*
void timerhandler(void)
{
    totalclock++;
}*/

int gametext(int x,int y,const char  *t,uint8_t  s,short dabits)
{
    short ac,newx;
    const char* oldt;
    uint8_t  centre;

    centre = ( x == (320>>1) );
    newx = 0;
    oldt = t;

    if(centre)
    {
        while(*t)
        {
            if(*t == 32) {newx+=5;t++;continue;}
            else ac = *t - '!' + STARTALPHANUM;

            if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

            if(*t >= '0' && *t <= '9')
                newx += 8;
            else newx += tiles[ac].dim.width;
            t++;
        }

        t = oldt;
        x = (320>>1)-(newx>>1);
    }

    while(*t)
    {
        if(*t == 32) {x+=5;t++;continue;}
        else ac = *t - '!' + STARTALPHANUM;

        if( ac < STARTALPHANUM || ac > ENDALPHANUM )
            break;

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,0,dabits,0,0,xdim-1,ydim-1);

        if(*t >= '0' && *t <= '9')
            x += 8;
        else x += tiles[ac].dim.width;

        t++;
    }

    return (x);
}

int gametextpal(int x,int y,const char  *t,uint8_t  s,uint8_t  p)
{
    short ac,newx;
    uint8_t  centre;
    const char* oldt;

    centre = ( x == (320>>1) );
    newx = 0;
    oldt = t;

    if(centre)
    {
        while(*t)
        {
            if(*t == 32) {newx+=5;t++;continue;}
            else ac = *t - '!' + STARTALPHANUM;

            if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

            if(*t >= '0' && *t <= '9')
                newx += 8;
            else newx += tiles[ac].dim.width;
            t++;
        }

        t = oldt;
        x = (320>>1)-(newx>>1);
    }

    while(*t)
    {
        if(*t == 32) {x+=5;t++;continue;}
        else ac = *t - '!' + STARTALPHANUM;

        if( ac < STARTALPHANUM || ac > ENDALPHANUM )
            break;

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,2+8+16,0,0,xdim-1,ydim-1);
        if(*t >= '0' && *t <= '9')
            x += 8;
        else x += tiles[ac].dim.width;

        t++;
    }

    return (x);
}

int gametextpart(int x,int y, const char  *t,uint8_t  s,short p)
{
    short ac,newx, cnt;
    uint8_t  centre;
    const char * oldt;

    centre = ( x == (320>>1) );
    newx = 0;
    oldt = t;
    cnt = 0;

    if(centre)
    {
        while(*t)
        {
            if(cnt == p) break;

            if(*t == 32) {newx+=5;t++;continue;}
            else ac = *t - '!' + STARTALPHANUM;

            if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

            newx += tiles[ac].dim.width;
            t++;
            cnt++;

        }

        t = oldt;
        x = (320>>1)-(newx>>1);
    }

    cnt = 0;
    while(*t)
    {
        if(*t == 32) {x+=5;t++;continue;}
        else ac = *t - '!' + STARTALPHANUM;

        if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

        if(cnt == p)
        {
            rotatesprite(x<<16,y<<16,65536L,0,ac,s,1,2+8+16,0,0,xdim-1,ydim-1);
            break;
        }
        else
            rotatesprite(x<<16,y<<16,65536L,0,ac,s,0,2+8+16,0,0,xdim-1,ydim-1);

        x += tiles[ac].dim.width;

        t++;
        cnt++;
    }

    return (x);
}

int minitext(int x,int y,const char  *str,uint8_t  p,uint8_t  sb)
{
    short ac;
    char  buf[128];
    char  *t;

    strncpy (buf, str, 128);
    buf[127] = 0;
    t = buf;

    while(*t)
    {
        *t = toupper(*t);
        if(*t == 32) {x+=5;t++;continue;}
        else ac = *t - '!' + MINIFONT;

        rotatesprite(x<<16,y<<16,65536L,0,ac,0,p,sb,0,0,xdim-1,ydim-1);
        x += 4; // tilesizx[ac]+1;

        t++;
    }
    return (x);
}

int minitextshade(int x,int y,char  *str,uint8_t  s,uint8_t  p,uint8_t  sb)
{
    short ac;
    char  buf[128];
    char  *t;

    strncpy (buf, str, 128);
    buf[127] = 0;
    t = buf;

    while(*t)
    {
        *t = toupper(*t);
        if(*t == 32) {x+=5;t++;continue;}
        else ac = *t - '!' + MINIFONT;

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,sb,0,0,xdim-1,ydim-1);
        x += 4; // tilesizx[ac]+1;

        t++;
    }
    return (x);
}

void gamenumber(int32_t x,int32_t y,int32_t n,uint8_t  s)
{
    char  b[10];


    //
    // uint8_t  * ltoa(int32_t l, uint8_t  * buffer, int radix);
    // is NON-STANDARD and equivalent to STANDARD
    // (void) sprintf(buffer, "%ld", l);
    //ltoa(n,b,10);
    sprintf(b,"%d",n);
    gametext(x,y,b,s,2+8+16);
}


char  recbuf[80];

#define MAXUSERQUOTES 4
int32_t quotebot, quotebotgoal;
short user_quote_time[MAXUSERQUOTES];
char  user_quote[MAXUSERQUOTES][32];    // shall be enough 32 instead of 128.
// uint8_t  typebuflen,typebuf[41];

static void adduserquote(char  *daquote)
{
    int32_t i;

    for(i=MAXUSERQUOTES-1;i>0;i--)
    {
        strcpy(user_quote[i],user_quote[i-1]);
        user_quote_time[i] = user_quote_time[i-1];
    }
    strcpy(user_quote[0],daquote);
    user_quote_time[0] = 180;
    pub = NUMPAGES;
}

char  *grpVersion2char_from_crc(unsigned int crc32_grp_to_identify)
{
	char  *id;
	int i=0;

	id = crc32lookup[MAX_KNOWN_GRP].name; // unknown version

	for(i=0; i<MAX_KNOWN_GRP; i++)
	{
		if(crc32lookup[i].crc32==crc32_grp_to_identify)
			id = crc32lookup[i].name;
	}

	return(id);
}

#if 0
char  *grpVersion2char(uint8_t  grp_to_identify)
{
	char  *id;

	switch(grp_to_identify)
	{
		case DUKEITOUTINDC_GRP:
			id = "v1.5 DC PACK";
			break;
		case SHAREWARE_GRP13:
			id = "v1.3 SHAREW.";
			break;
		case ATOMIC_GRP14_15:
			id = "v1.5 ATOMIC";
			break;
		case REGULAR_GRP13D:
			id = "v1.3D FULL";
			break;
		case UNKNOWN_GRP:
			id = "vX.X UNKNOWN";
			break;
		default:
			Error(EXIT_FAILURE,"Failed the GRP Identification\n");
			break;
	}

	return(id);
}
#endif
#if !WIN32
void radioBlockTask(void)
{
    updateAudio();
    totalclock = ototalclock;
}
#endif
//This is a function from the Engine module, used in getpackets.
void sampletimer(void);
#include "delay.h"
void getpackets(void)
{
    int32_t i, j, k, l;
    short other, packbufleng;
    input *osyn, *nsyn;

	sampletimer();
    if(qe == 0 && KB_KeyPressed(sc_LeftControl) && KB_KeyPressed(sc_LeftAlt) && KB_KeyPressed(sc_Delete))
    {
        qe = 1;
        gameexit("Quick Exit.");
    }

	// not a net game
    if (numplayers < 2 || multiplayerConnection)
	{
		//printf("getpackets() numplayers < 2");
		return;
	}

    while ((packbufleng = getpacket(&other,packbuf)) > 0)
    {
#if _DEBUG_NETWORKING_
		printf("RECEIVED PACKET: type: %d : len %d\r\n", packbuf[0], packbufleng);
#endif

        switch(packbuf[0])
        {
			case 253:
				// This should have already been handled by mmulti.cpp so ignore it
				printf("Invalid Packet: %d", packbuf[0]);
			break;

            case 125:
                cp = 0;
                break;

            case 126:
                multiflag = 2;
                multiwhat = 0;
                multiwho = other;
                multipos = packbuf[1];
                loadplayer( multipos );
                multiflag = 0;
                break;
            case 0:  //[0] (receive master sync buffer)
                j = 1;

                if ((movefifoend[other]&(TIMERUPDATESIZ-1)) == 0)
                    for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                    {
                        if (playerquitflag[i] == 0) continue;
                        if (i == myconnectindex)
                            otherminlag = (int32_t)((int8_t  )packbuf[j]);
                        j++;
                    }

                osyn = (input *)&inputfifo[(movefifoend[connecthead]-1)&(MOVEFIFOSIZ-1)][0];
                nsyn = (input *)&inputfifo[(movefifoend[connecthead])&(MOVEFIFOSIZ-1)][0];

                k = j;
                for(i=connecthead;i>=0;i=connectpoint2[i])
                    j += playerquitflag[i];
                for(i=connecthead;i>=0;i=connectpoint2[i])
                {
                    if (playerquitflag[i] == 0) continue;

                    l = packbuf[k++];
                    if (i == myconnectindex)
                        { j += ((l&1)<<1)+(l&2)+((l&4)>>2)+((l&8)>>3)+((l&16)>>4)+((l&32)>>5)+((l&64)>>6)+((l&128)>>7); continue; }

                    copybufbyte(&osyn[i],&nsyn[i],sizeof(input));
                    if (l&1)   nsyn[i].fvel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
                    if (l&2)   nsyn[i].svel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
                    if (l&4)   nsyn[i].avel = (int8_t  )packbuf[j++];
                    if (l&8)   nsyn[i].bits = ((nsyn[i].bits&0xffffff00)|((int32_t)packbuf[j++]));
                    if (l&16)  nsyn[i].bits = ((nsyn[i].bits&0xffff00ff)|((int32_t)packbuf[j++])<<8);
                    if (l&32)  nsyn[i].bits = ((nsyn[i].bits&0xff00ffff)|((int32_t)packbuf[j++])<<16);
                    if (l&64)  nsyn[i].bits = ((nsyn[i].bits&0x00ffffff)|((int32_t)packbuf[j++])<<24);
                    if (l&128) nsyn[i].horz = (int8_t  )packbuf[j++];

                    if (nsyn[i].bits&(1<<26)) playerquitflag[i] = 0;
                    movefifoend[i]++;
                }

                while (j != packbufleng)
                {
                    for(i=connecthead;i>=0;i=connectpoint2[i])
                        if(i != myconnectindex)
                    {
                        syncval[i][syncvalhead[i]&(MOVEFIFOSIZ-1)] = packbuf[j];
                        syncvalhead[i]++;
                    }
                    j++;
                }

                for(i=connecthead;i>=0;i=connectpoint2[i])
                    if (i != myconnectindex)
                        for(j=1;j<movesperpacket;j++)
                        {
                            copybufbyte(&nsyn[i],&inputfifo[movefifoend[i]&(MOVEFIFOSIZ-1)][i],sizeof(input));
                            movefifoend[i]++;
                        }

                 movefifosendplc += movesperpacket;

                break;
            case 1:  //[1] (receive slave sync buffer)
                j = 2; k = packbuf[1];

                osyn = (input *)&inputfifo[(movefifoend[other]-1)&(MOVEFIFOSIZ-1)][0];
                nsyn = (input *)&inputfifo[(movefifoend[other])&(MOVEFIFOSIZ-1)][0];

                copybufbyte(&osyn[other],&nsyn[other],sizeof(input));
                if (k&1)   nsyn[other].fvel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
                if (k&2)   nsyn[other].svel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
                if (k&4)   nsyn[other].avel = (int8_t  )packbuf[j++];
                if (k&8)   nsyn[other].bits = ((nsyn[other].bits&0xffffff00)|((int32_t)packbuf[j++]));
                if (k&16)  nsyn[other].bits = ((nsyn[other].bits&0xffff00ff)|((int32_t)packbuf[j++])<<8);
                if (k&32)  nsyn[other].bits = ((nsyn[other].bits&0xff00ffff)|((int32_t)packbuf[j++])<<16);
                if (k&64)  nsyn[other].bits = ((nsyn[other].bits&0x00ffffff)|((int32_t)packbuf[j++])<<24);
                if (k&128) nsyn[other].horz = (int8_t  )packbuf[j++];
                movefifoend[other]++;

                while (j != packbufleng)
                {
                    syncval[other][syncvalhead[other]&(MOVEFIFOSIZ-1)] = packbuf[j++];
                    syncvalhead[other]++;
                }

                for(i=1;i<movesperpacket;i++)
                {
                    copybufbyte(&nsyn[other],&inputfifo[movefifoend[other]&(MOVEFIFOSIZ-1)][other],sizeof(input));
                    movefifoend[other]++;
                }

                break;

            case 4: // message talk T
                strcpy(recbuf,(char*)packbuf+1);
                recbuf[packbufleng-1] = 0;

                adduserquote(recbuf);
                sound(EXITMENUSOUND);

                pus = NUMPAGES;
                pub = NUMPAGES;

                break;

            case 5:
                ud.m_level_number = ud.level_number = packbuf[1];
                ud.m_volume_number = ud.volume_number = packbuf[2];
                ud.m_player_skill = ud.player_skill = packbuf[3];
                ud.m_monsters_off = ud.monsters_off = packbuf[4];
                ud.m_respawn_monsters = ud.respawn_monsters = packbuf[5];
                ud.m_respawn_items = ud.respawn_items = packbuf[6];
                ud.m_respawn_inventory = ud.respawn_inventory = packbuf[7];
                ud.m_coop = packbuf[8];
                ud.m_marker = ud.marker = packbuf[9];
                ud.m_ffire = ud.ffire = packbuf[10];

                for(i=connecthead;i>=0;i=connectpoint2[i])
                {
                    resetweapons(i);
                    resetinventory(i);
                }

                newgame(ud.volume_number,ud.level_number,ud.player_skill);
                ud.coop = ud.m_coop;

                enterlevel(MODE_GAME);

                break;

            case 6: // get names
                for (i=2;packbuf[i] && i<=11;i++) // limit size of name
                    ud.user_name[other][i-2] = packbuf[i];
                ud.user_name[other][i-2] = 0;

				// we allow the old rancidmeat 19.1 to connect, using the old grpVersion system w/ BYTEVERSION
				if(packbuf[1] == BYTEVERSION_27 || packbuf[1] == BYTEVERSION_117)
				{
					// Old rancid was using either BYTEVERSION_27 or BYTEVERSION_117
					Error(EXIT_SUCCESS,	"STOP: Your opponent is using an obsolete version\n"
										"Please ask him to update to xDuke v%d.%d!\n", CHOCOLATE_DUKE_REV_X, CHOCOLATE_DUKE_REV_DOT_Y);
				}
				break;

            case 9:
                for (i=1;i<packbufleng;i++)
                    ud.wchoice[other][i-1] = packbuf[i];
                break;

            case 7:
#if USE_RTS
                if(numlumps == 0) break;
                if (SoundToggle == 0 || ud.lockout == 1 || FXDevice == SC_Unknown)
                    break;
                rtsptr = RTS_GetSound(packbuf[1]-1);
                if (*rtsptr == 'C')
                    FX_PlayVOC3D(rtsptr,0,0,0,255,-packbuf[1]);
                else
                    FX_PlayWAV3D(rtsptr,0,0,0,255,-packbuf[1]);
                rtsplaying = 7;
#endif

                break;
            case 8:
                ud.m_level_number = ud.level_number = packbuf[1];
                ud.m_volume_number = ud.volume_number = packbuf[2];
                ud.m_player_skill = ud.player_skill = packbuf[3];
                ud.m_monsters_off = ud.monsters_off = packbuf[4];
                ud.m_respawn_monsters = ud.respawn_monsters = packbuf[5];
                ud.m_respawn_items = ud.respawn_items = packbuf[6];
                ud.m_respawn_inventory = ud.respawn_inventory = packbuf[7];
                ud.m_coop = ud.coop = packbuf[8];
                ud.m_marker = ud.marker = packbuf[9];
                ud.m_ffire = ud.ffire = packbuf[10];

                copybufbyte(packbuf+10,boardfilename,packbufleng-11);
                boardfilename[packbufleng-11] = 0;

                for(i=connecthead;i>=0;i=connectpoint2[i])
                {
                    resetweapons(i);
                    resetinventory(i);
                }

                newgame(ud.volume_number,ud.level_number,ud.player_skill);
                enterlevel(MODE_GAME);
                break;

            case 16:
                movefifoend[other] = movefifoplc = movefifosendplc = fakemovefifoplc = 0;
                syncvalhead[other] = syncvaltottail = 0L;

            case 17:
                j = 1;

                if ((movefifoend[other]&(TIMERUPDATESIZ-1)) == 0)
                    if (other == connecthead)
                        for(i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                        {
                            if (i == myconnectindex)
							{
								otherminlag = (int32_t)((int8_t  )packbuf[j]);
							}

                            j++;
                        }

                osyn = (input *)&inputfifo[(movefifoend[other]-1)&(MOVEFIFOSIZ-1)][0];
                nsyn = (input *)&inputfifo[(movefifoend[other])&(MOVEFIFOSIZ-1)][0];

                copybufbyte(&osyn[other],&nsyn[other],sizeof(input));
                k = packbuf[j++];
                if (k&1)   nsyn[other].fvel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
                if (k&2)   nsyn[other].svel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
                if (k&4)   nsyn[other].avel = (int8_t  )packbuf[j++];
                if (k&8)   nsyn[other].bits = ((nsyn[other].bits&0xffffff00)|((int32_t)packbuf[j++]));
                if (k&16)  nsyn[other].bits = ((nsyn[other].bits&0xffff00ff)|((int32_t)packbuf[j++])<<8);
                if (k&32)  nsyn[other].bits = ((nsyn[other].bits&0xff00ffff)|((int32_t)packbuf[j++])<<16);
                if (k&64)  nsyn[other].bits = ((nsyn[other].bits&0x00ffffff)|((int32_t)packbuf[j++])<<24);
                if (k&128) nsyn[other].horz = (int8_t  )packbuf[j++];
                movefifoend[other]++;

                for(i=1;i<movesperpacket;i++)
                {
                    copybufbyte(&nsyn[other],&inputfifo[movefifoend[other]&(MOVEFIFOSIZ-1)][other],sizeof(input));
                    movefifoend[other]++;
                }

                if (j > packbufleng)
				{
					printf("INVALID GAME PACKET!!! (%d too many bytes) (j= %d, packbuflen= %d, type: %d)\n",j-packbufleng, j, packbufleng, packbuf[0]);
					j = 0;
				}

                while (j != packbufleng)
                {
                    syncval[other][syncvalhead[other]&(MOVEFIFOSIZ-1)] = packbuf[j++];
                    syncvalhead[other]++;
                }

                break;
            case 127:
                break;

#ifdef CHECK_XDUKE_REV
			case 131: // xDuke Rev ID
				memcpy(ud.rev[other], packbuf, 10);
#endif

			case 132: // get map CRC of opponents (to debug out of synch)
#if LETS_WASTE_RAM_FOR_USERDEFS
				ud.mapCRC[other] = (uint16_t)packbuf[1] + (uint16_t)(packbuf[2]<<8);
#endif
				break;

			case 133: // client refused to disable the autoaim by host

				Error(EXIT_SUCCESS,	"One or more players refused to play with AutoAim OFF because this breaks\n"
									"the official Duke's gameplay. Please restart without this option...\n");

				break;

			case 134: // Get GRP CRC32 + Con size + exeCRC + conCRC
#if LETS_WASTE_RAM_FOR_USERDEFS
				memcpy(ud.groupefil_crc32[other], packbuf+1, sizeof(groupefil_crc32));
				memcpy(ud.conSize+other, packbuf+1+sizeof(groupefil_crc32), sizeof(ud.conSize[0]));
				memcpy(ud.conCRC+other, packbuf+1+sizeof(groupefil_crc32)+sizeof(ud.conSize[0]), sizeof(ud.conCRC[0]));
				memcpy(ud.exeCRC+other, packbuf+1+sizeof(groupefil_crc32)+sizeof(ud.conSize[0])+sizeof(ud.conCRC[0]), sizeof(ud.exeCRC[0]));
#endif
				break;

            case 250:
                {
					playerreadyflag[other]++;
					printf("Player %d '%s' is ready...\n", other, ud.user_name[other]);
				}
                break;
            case 255:
                gameexitanycase();
                break;
        }
    }
}

//From player.c
#if WIN32
int isRadioTxFifoFull(void)
{
    return 0;
}
#endif
void computergetinput(int32_t snum, input *syn);
void faketimerhandler()
{
    int32_t i, j, k;
    input *osyn, *nsyn;

    //Check if we should quit the game.
    if ((qe == 0 && KB_KeyPressed(sc_LeftControl) && KB_KeyPressed(sc_LeftAlt) && KB_KeyPressed(sc_Delete)) ||
        (qe == 0 && KB_KeyPressed(sc_LeftAlt) && KB_KeyPressed(sc_F4)))
    {
        qe = 1;
        gameexit("Quick Exit.");
    }

    // did we wait at least 4 ticks ?
    // fixme: remove isRadioTxFifoFull()...
    if ((totalclock < ototalclock+TICSPERFRAME) || (ready2send == 0) || isRadioTxFifoFull())
    {
        if (isRadioTxFifoFull())
        {
            totalclock = ototalclock;
        }
		return; // Returns here when playing a demo.
    }
    //YES : Add ticks
    ototalclock += TICSPERFRAME;
    //Check network stuff.
    getpackets(); // this will call also sampletimer()
    if (getoutputcirclesize() >= 16)
        return;
    for(i=connecthead;i>=0;i=connectpoint2[i])
        if (i != myconnectindex)
//            if (movefifoend[i] < movefifoend[myconnectindex]-200) // 200 ?? Another magic number :/
            if (movefifoend[i] < movefifoend[myconnectindex]-MAX_CONNECTION_DELAY)
                return;

     if( !CONSOLE_IsActive())
     {
        getinput(myconnectindex);
     }

     avgfvel += loc.fvel; // x
     avgsvel += loc.svel; // y
     avgavel += loc.avel;
     avghorz += loc.horz;
     avgbits |= loc.bits;
     if (movefifoend[myconnectindex]&(movesperpacket-1))
     {
          copybufbyte(&inputfifo[(movefifoend[myconnectindex]-1)&(MOVEFIFOSIZ-1)][myconnectindex],
                          &inputfifo[movefifoend[myconnectindex]&(MOVEFIFOSIZ-1)][myconnectindex],sizeof(input));
          movefifoend[myconnectindex]++;
          return;
     }

     nsyn = &inputfifo[movefifoend[myconnectindex]&(MOVEFIFOSIZ-1)][myconnectindex];
     nsyn[0].fvel = avgfvel/movesperpacket;
     nsyn[0].svel = avgsvel/movesperpacket;
     nsyn[0].avel = avgavel/movesperpacket;
     nsyn[0].horz = avghorz/movesperpacket;
     nsyn[0].bits = avgbits;
     avgfvel = avgsvel = avgavel = avghorz = avgbits = 0;
     movefifoend[myconnectindex]++;

     if (numplayers < 2)
     {
          if (ud.multimode > 1) for(i=connecthead;i>=0;i=connectpoint2[i])
              if(i != myconnectindex)
              {
                  //clearbufbyte(&inputfifo[movefifoend[i]&(MOVEFIFOSIZ-1)][i],sizeof(input),0L);
                  if(ud.playerai)
                      computergetinput(i,&inputfifo[movefifoend[i]&(MOVEFIFOSIZ-1)][i]);
                  movefifoend[i]++;
              }
          return;
     }

    for(i=connecthead;i>=0;i=connectpoint2[i])
        if (i != myconnectindex)
        {
            k = (movefifoend[myconnectindex]-1)-movefifoend[i];
            myminlag[i] = min(myminlag[i],k);
            mymaxlag = max(mymaxlag,k);
        }

    if (((movefifoend[myconnectindex]-1)&(TIMERUPDATESIZ-1)) == 0)
    {
        i = mymaxlag-bufferjitter; mymaxlag = 0;
        if (i > 0) bufferjitter += ((3+i)>>2);
        else if (i < 0) bufferjitter -= ((1-i)>>2);
    }

    if (networkmode == 1)
    {
        packbuf[0] = 17;

		if ((movefifoend[myconnectindex]-1) == 0)
		{
			packbuf[0] = 16;
		}

        j = 1;

            //Fix timers and buffer/jitter value
        if (((movefifoend[myconnectindex]-1)&(TIMERUPDATESIZ-1)) == 0)
        {
            if (myconnectindex != connecthead)
            {
                i = myminlag[connecthead]-otherminlag;
                if (klabs(i) > 8)
				{
					i >>= 1;
				}
                else
				if (klabs(i) > 2)
				{
					i = ksgn(i);
				}
                else
				{
					i = 0;
				}

                totalclock -= TICSPERFRAME*i;
                myminlag[connecthead] -= i; otherminlag += i;
            }

            if (myconnectindex == connecthead)
                for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                    packbuf[j++] = min(max(myminlag[i],-128),127);

            for(i=connecthead;i>=0;i=connectpoint2[i])
                myminlag[i] = 0x7fffffff;
        }

        osyn = (input *)&inputfifo[(movefifoend[myconnectindex]-2)&(MOVEFIFOSIZ-1)][myconnectindex];
        nsyn = (input *)&inputfifo[(movefifoend[myconnectindex]-1)&(MOVEFIFOSIZ-1)][myconnectindex];

        k = j;
        packbuf[j++] = 0;

        if (nsyn[0].fvel != osyn[0].fvel)
        {
            packbuf[j++] = (uint8_t )nsyn[0].fvel;
            packbuf[j++] = (uint8_t )(nsyn[0].fvel>>8);
            packbuf[k] |= 1;
        }
        if (nsyn[0].svel != osyn[0].svel)
        {
            packbuf[j++] = (uint8_t )nsyn[0].svel;
            packbuf[j++] = (uint8_t )(nsyn[0].svel>>8);
            packbuf[k] |= 2;
        }
        if (nsyn[0].avel != osyn[0].avel)
        {
            packbuf[j++] = (int8_t  )nsyn[0].avel;
            packbuf[k] |= 4;
        }
        if ((nsyn[0].bits^osyn[0].bits)&0x000000ff) packbuf[j++] = (nsyn[0].bits&255), packbuf[k] |= 8;
        if ((nsyn[0].bits^osyn[0].bits)&0x0000ff00) packbuf[j++] = ((nsyn[0].bits>>8)&255), packbuf[k] |= 16;
        if ((nsyn[0].bits^osyn[0].bits)&0x00ff0000) packbuf[j++] = ((nsyn[0].bits>>16)&255), packbuf[k] |= 32;
        if ((nsyn[0].bits^osyn[0].bits)&0xff000000) packbuf[j++] = ((nsyn[0].bits>>24)&255), packbuf[k] |= 64;
        if (nsyn[0].horz != osyn[0].horz)
        {
            packbuf[j++] = (uint8_t )nsyn[0].horz;
            packbuf[k] |= 128;
        }

        while (syncvalhead[myconnectindex] != syncvaltail)
        {
            packbuf[j++] = syncval[myconnectindex][syncvaltail&(MOVEFIFOSIZ-1)];
            syncvaltail++;
        }

       // for(i=connecthead;i>=0;i=connectpoint2[i])
       //     if (i != myconnectindex)
                //         sendpacket(i,packbuf,j);
        broadcastPacket(packbuf, j, true);
        return;
    }
#if MASTER_SLAVE_ENABLED
    if (myconnectindex != connecthead)   //Slave
    {
        FIXME("WE REACHED A BAD POINT: not implemented slave");
            //Fix timers and buffer/jitter value
        if (((movefifoend[myconnectindex]-1)&(TIMERUPDATESIZ-1)) == 0)
        {
            i = myminlag[connecthead]-otherminlag;
            if (klabs(i) > 8) i >>= 1;
            else if (klabs(i) > 2) i = ksgn(i);
            else i = 0;

            totalclock -= TICSPERFRAME*i;
            myminlag[connecthead] -= i; otherminlag += i;

            for(i=connecthead;i>=0;i=connectpoint2[i])
                myminlag[i] = 0x7fffffff;
        }

        packbuf[0] = 1; packbuf[1] = 0; j = 2;

        osyn = (input *)&inputfifo[(movefifoend[myconnectindex]-2)&(MOVEFIFOSIZ-1)][myconnectindex];
        nsyn = (input *)&inputfifo[(movefifoend[myconnectindex]-1)&(MOVEFIFOSIZ-1)][myconnectindex];

        if (nsyn[0].fvel != osyn[0].fvel)
        {
            packbuf[j++] = (uint8_t )nsyn[0].fvel;
            packbuf[j++] = (uint8_t )(nsyn[0].fvel>>8);
            packbuf[1] |= 1;
        }
        if (nsyn[0].svel != osyn[0].svel)
        {
            packbuf[j++] = (uint8_t )nsyn[0].svel;
            packbuf[j++] = (uint8_t )(nsyn[0].svel>>8);
            packbuf[1] |= 2;
        }
        if (nsyn[0].avel != osyn[0].avel)
        {
            packbuf[j++] = (int8_t  )nsyn[0].avel;
            packbuf[1] |= 4;
        }
        if ((nsyn[0].bits^osyn[0].bits)&0x000000ff) packbuf[j++] = (nsyn[0].bits&255), packbuf[1] |= 8;
        if ((nsyn[0].bits^osyn[0].bits)&0x0000ff00) packbuf[j++] = ((nsyn[0].bits>>8)&255), packbuf[1] |= 16;
        if ((nsyn[0].bits^osyn[0].bits)&0x00ff0000) packbuf[j++] = ((nsyn[0].bits>>16)&255), packbuf[1] |= 32;
        if ((nsyn[0].bits^osyn[0].bits)&0xff000000) packbuf[j++] = ((nsyn[0].bits>>24)&255), packbuf[1] |= 64;
        if (nsyn[0].horz != osyn[0].horz)
        {
            packbuf[j++] = (uint8_t )nsyn[0].horz;
            packbuf[1] |= 128;
        }

        while (syncvalhead[myconnectindex] != syncvaltail)
        {
            packbuf[j++] = syncval[myconnectindex][syncvaltail&(MOVEFIFOSIZ-1)];
            syncvaltail++;
        }
//        sendpacket(connecthead,packbuf,j);
        return;
    }

        //This allows allow packet-resends
    for(i=connecthead;i>=0;i=connectpoint2[i])
        if (movefifoend[i] <= movefifosendplc)
        {
            packbuf[0] = 127;
            //for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
            //   sendpacket(i,packbuf,1);
            broadcastPacket(packbuf, 1, false);
            return;
        }
    FIXME("WE REACHED A BAD POINT");
    while (1)  //Master. Not implemented
    {
        for(i=connecthead;i>=0;i=connectpoint2[i])
            if (playerquitflag[i] && (movefifoend[i] <= movefifosendplc)) return;

        osyn = (input *)&inputfifo[(movefifosendplc-1)&(MOVEFIFOSIZ-1)][0];
        nsyn = (input *)&inputfifo[(movefifosendplc  )&(MOVEFIFOSIZ-1)][0];

            //MASTER -> SLAVE packet
        packbuf[0] = 0; j = 1;

            //Fix timers and buffer/jitter value
        if ((movefifosendplc&(TIMERUPDATESIZ-1)) == 0)
        {
            for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
               if (playerquitflag[i])
                packbuf[j++] = min(max(myminlag[i],-128),127);

            for(i=connecthead;i>=0;i=connectpoint2[i])
                myminlag[i] = 0x7fffffff;
        }

        k = j;
        for(i=connecthead;i>=0;i=connectpoint2[i])
           j += playerquitflag[i];
        for(i=connecthead;i>=0;i=connectpoint2[i])
        {
            if (playerquitflag[i] == 0) continue;

            packbuf[k] = 0;
            if (nsyn[i].fvel != osyn[i].fvel)
            {
                packbuf[j++] = (uint8_t )nsyn[i].fvel;
                packbuf[j++] = (uint8_t )(nsyn[i].fvel>>8);
                packbuf[k] |= 1;
            }
            if (nsyn[i].svel != osyn[i].svel)
            {
                packbuf[j++] = (uint8_t )nsyn[i].svel;
                packbuf[j++] = (uint8_t )(nsyn[i].svel>>8);
                packbuf[k] |= 2;
            }
            if (nsyn[i].avel != osyn[i].avel)
            {
                packbuf[j++] = (int8_t  )nsyn[i].avel;
                packbuf[k] |= 4;
            }
            if ((nsyn[i].bits^osyn[i].bits)&0x000000ff) packbuf[j++] = (nsyn[i].bits&255), packbuf[k] |= 8;
            if ((nsyn[i].bits^osyn[i].bits)&0x0000ff00) packbuf[j++] = ((nsyn[i].bits>>8)&255), packbuf[k] |= 16;
            if ((nsyn[i].bits^osyn[i].bits)&0x00ff0000) packbuf[j++] = ((nsyn[i].bits>>16)&255), packbuf[k] |= 32;
            if ((nsyn[i].bits^osyn[i].bits)&0xff000000) packbuf[j++] = ((nsyn[i].bits>>24)&255), packbuf[k] |= 64;
            if (nsyn[i].horz != osyn[i].horz)
            {
                packbuf[j++] = (uint8_t )nsyn[i].horz;
                packbuf[k] |= 128;
            }
            k++;
        }

        while (syncvalhead[myconnectindex] != syncvaltail)
        {
            packbuf[j++] = syncval[myconnectindex][syncvaltail&(MOVEFIFOSIZ-1)];
            syncvaltail++;
        }

        for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
            if (playerquitflag[i])
            {
// TODO: FIXME, DISABLED CODE.
//                 sendpacket(i,packbuf,j);
                 if (nsyn[i].bits&(1<<26))
                    playerquitflag[i] = 0;
            }

        movefifosendplc += movesperpacket;
    }
#endif
}
#if USE_CACHE
extern int32_t cacnum;

typedef struct {
    uint8_t *hand;
    int32_t leng;
    uint8_t  *lock; }
cactype;
extern cactype cac[];

void caches(void)
{
     short i,k;
    char text[512];

     k = 0;
     for(i=0;i<cacnum;i++)
          if ((*cac[i].lock) >= 200)
          {
                sprintf(text,"Locked- %d: Leng:%d, Lock:%d",i,cac[i].leng,*cac[i].lock);
                printext256(0L,k,31,-1,text,1); k += 6;
          }

     k += 6;
#if USE_RTS
     for(i=1;i<11;i++)
          if (lumplockbyte[i] >= 200)
          {
                sprintf(text,"RTS Locked %hd:",i);
                printext256(0L,k,31,-1,text,1); k += 6;
          }
#endif

}
#endif
// FIX_00024: A key can be assigned to the new SHOW_INFO function. Display map CRC when
//             in deathmatch. Usefull to identify who loaded a wrong map in multiplayer.
void dispVersion(void)
{
	int i;
	int offx, offy, stepx, stepy;
    char text[512];

	offx = 21; offy = 30;
	stepx = 73; stepy = 20;

	// black translucent background underneath lists
	rotatesprite(0<<16, 0<<16, 65536l<<5, 0, BLANK, 8, 0, 1+2+8+16+64,
		scale(0,xdim,320),scale(26,ydim,200),
		scale(320-0,xdim,320)-1,scale(200-((ud.multimode>4)?(161-1*7)-stepy:(161-1*7)),ydim,200)-1);

	// FIX_00009: Show map CRC and GRP file version of each player in case of Out Of Synch
	for(i=connecthead;i>=0;i=connectpoint2[i])
	{
		// Disp name
		sprintf(text,"%s", ud.user_name[i]);
		minitext(offx+(stepx*(i&3)),offy+0+((i&4)>>2)*stepy, text, getSprite(ps[i].i)->_pal, 2+8+16);
#if LETS_WASTE_RAM_FOR_USERDEFS
		// Disp MAP CRC
		if(ps[i].fakeplayer)
			sprintf(text,"MAP CRC: (bot)");
		else
			sprintf(text,"MAP CRC: %X", ud.mapCRC[i]);
#endif
		minitext(offx+(stepx*(i&3)),offy+7+((i&4)>>2)*stepy, text, COLOR_ON,2+8+16);

	}
}

void checksync(void)
{
      int32_t i;

      for(i=connecthead;i>=0;i=connectpoint2[i])
            if (syncvalhead[i] == syncvaltottail) break;
      if (i < 0)
      {
             syncstat = 0;
             do
             {
                     for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
					 {
						 if (syncval[i][syncvaltottail&(MOVEFIFOSIZ-1)] != syncval[connecthead][syncvaltottail&(MOVEFIFOSIZ-1)])
						 {
                                 syncstat = 1;
						}
					 }

                     syncvaltottail++;
                     for(i=connecthead;i>=0;i=connectpoint2[i])
                     {
						 if (syncvalhead[i] == syncvaltottail)
						 {
							 break;
						 }
					 }
             } while (i < 0);
      }

      if (connectpoint2[connecthead] < 0)
	  {
		  syncstat = 0;
	  }

      if (syncstat)
      {
			minitext(21,30+35+30, "Out Of Sync - Please restart game", COLOR_ON,2+8+16);
			// FIX_00090: Removed info key. FPS were shown after CRC msg. CRC not always removed. (Turrican)
			for(i=connecthead;i>=0;i=connectpoint2[i])
			{
#if LETS_WASTE_RAM_FOR_USERDEFS
				if (ud.mapCRC[connecthead]!=ud.mapCRC[i])
				{
					minitext(21,30+42+30, "Map CRC mismatching. Please use exactly the same map.", COLOR_ON,2+8+16);
					dispVersion();
				}
				else
				minitext(21,30+42+30, "Verify the con files. Close your P2P if any", COLOR_ON,2+8+16);
#endif
			}
	  }

      if (syncstate)
      {
          //printext256(4L,160L,31,0,"Missed Network packet!",0);
          //printext256(4L,138L,31,0,"RUN DN3DHELP.EXE for information.",0);
          minitext(21,30+35+30, "Missed Network packet!", COLOR_ON,2+8+16);
      }

}


void check_fta_sounds(short i)
{
    if(getSprite_extra(i) > 0) switch(PN)
    {
        case LIZTROOPONTOILET:
        case LIZTROOPJUSTSIT:
        case LIZTROOPSHOOT:
        case LIZTROOPJETPACK:
        case LIZTROOPDUCKING:
        case LIZTROOPRUNNING:
        case LIZTROOP:
            spritesound(PRED_RECOG,i);
            break;
        case LIZMAN:
        case LIZMANSPITTING:
        case LIZMANFEEDING:
        case LIZMANJUMP:
            spritesound(CAPT_RECOG,i);
            break;
        case PIGCOP:
        case PIGCOPDIVE:
            spritesound(PIG_RECOG,i);
            break;
        case RECON:
            spritesound(RECO_RECOG,i);
            break;
        case DRONE:
            spritesound(DRON_RECOG,i);
            break;
        case COMMANDER:
        case COMMANDERSTAYPUT:
            spritesound(COMM_RECOG,i);
            break;
        case ORGANTIC:
            spritesound(TURR_RECOG,i);
            break;
        case OCTABRAIN:
        case OCTABRAINSTAYPUT:
            spritesound(OCTA_RECOG,i);
            break;
        case BOSS1:
            sound(BOS1_RECOG);
            break;
        case BOSS2:
            if(getSprite_pal(i) == 1)
                sound(BOS2_RECOG);
            else sound(WHIPYOURASS);
            break;
        case BOSS3:
            if(getSprite_pal(i) == 1)
                sound(BOS3_RECOG);
            else sound(RIPHEADNECK);
            break;
        case BOSS4:
        case BOSS4STAYPUT:
            if(getSprite_pal(i) == 1)
                sound(BOS4_RECOG);
            sound(BOSS4_FIRSTSEE);
            break;
        case GREENSLIME:
            spritesound(SLIM_RECOG,i);
            break;
    }
}

short inventory(const spritetype *s)
{
    switch(s->_picnum)
    {
        case FIRSTAID:
        case STEROIDS:
        case HEATSENSOR:
        case BOOTS:
        case JETPACK:
        case HOLODUKE:
        case AIRTANK:
            return 1;
    }
    return 0;
}

short badguypic(short pn)
{

    switch(pn)
    {
            case SHARK:
            case RECON:
            case DRONE:
            case LIZTROOPONTOILET:
            case LIZTROOPJUSTSIT:
            case LIZTROOPSTAYPUT:
            case LIZTROOPSHOOT:
            case LIZTROOPJETPACK:
            case LIZTROOPDUCKING:
            case LIZTROOPRUNNING:
            case LIZTROOP:
            case OCTABRAIN:
            case COMMANDER:
            case COMMANDERSTAYPUT:
            case PIGCOP:
            case EGG:
            case PIGCOPSTAYPUT:
            case PIGCOPDIVE:
            case LIZMAN:
            case LIZMANSPITTING:
            case LIZMANFEEDING:
            case LIZMANJUMP:
            case ORGANTIC:
            case BOSS1:
            case BOSS2:
            case BOSS3:
            case BOSS4:
            case GREENSLIME:
            case GREENSLIME+1:
            case GREENSLIME+2:
            case GREENSLIME+3:
            case GREENSLIME+4:
            case GREENSLIME+5:
            case GREENSLIME+6:
            case GREENSLIME+7:
            case RAT:
            case ROTATEGUN:
                return 1;
    }

    if( getActorType(pn) ) return 1;

    return 0;
}


short badguy(const spritetype *s)
{
    return badguypic(s->_picnum);
}





void myos(int32_t x, int32_t y, short tilenum, int8_t shade, uint8_t  orientation)
{
    uint8_t  p;
    short a;

    if(orientation&4)
        a = 1024;
    else a = 0;

    p = getSector(ps[screenpeek].cursectnum)->floorpal;
    rotatesprite(x<<16,y<<16,65536L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);
}

void myospal(int32_t x, int32_t y, short tilenum, int8_t shade, uint8_t  orientation, uint8_t  p)
{
//    uint8_t  fp;
    short a;

    if(orientation&4)
        a = 1024;
    else a = 0;

//    fp = sector[ps[screenpeek].cursectnum].floorpal;
#if !WIN32
    waitForUpdate(DRAW_BUFFER_HEIGHT - STATUS_BAR_HEIGHT);
#endif
    rotatesprite(x<<16,y<<16,65536L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);

}

void invennum(int32_t x,int32_t y,uint8_t  num1,uint8_t  ha,uint8_t  sbits)
{
    char  dabuf[80] = {0};
    sprintf(dabuf,"%d",num1);
    if(num1 > 99)
    {
        rotatesprite((x-4)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        rotatesprite((x)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[1]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        rotatesprite((x+4)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[2]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
    }
    else if(num1 > 9)
    {
        rotatesprite((x)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        rotatesprite((x+4)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[1]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
    }
    else
        rotatesprite((x+4)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
}

void orderweaponnum(short ind,int32_t x,int32_t y,int32_t num1, int32_t num2,uint8_t  ha)
{
    UNUSED(num1);
    UNUSED(num2);
    rotatesprite((x-7)<<16,y<<16,65536L,0,THREEBYFIVE+ind+1,ha-10,7,10+128,0,0,xdim-1,ydim-1);
    rotatesprite((x-3)<<16,y<<16,65536L,0,THREEBYFIVE+10,ha,0,10+128,0,0,xdim-1,ydim-1);

    minitextshade(x+1,y-4,"ORDER",26,6,2+8+16+128);
}

void weaponnum(short ind,int32_t x,int32_t y,int32_t num1, int32_t num2,uint8_t  ha)
{
    char  dabuf[80] = {0};

    rotatesprite((x-7)<<16,y<<16,65536L,0,THREEBYFIVE+ind+1,ha-10,7,10+128,0,0,xdim-1,ydim-1);
    rotatesprite((x-3)<<16,y<<16,65536L,0,THREEBYFIVE+10,ha,0,10+128,0,0,xdim-1,ydim-1);
    rotatesprite((x+9)<<16,y<<16,65536L,0,THREEBYFIVE+11,ha,0,10+128,0,0,xdim-1,ydim-1);

    if(num1 > 99) num1 = 99;
    if(num2 > 99) num2 = 99;

    sprintf(dabuf,"%d",num1);
    if(num1 > 9)
    {
        rotatesprite((x)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
        rotatesprite((x+4)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[1]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
    }
    else rotatesprite((x+4)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);

    sprintf(dabuf,"%d",num2);
    if(num2 > 9)
    {
        rotatesprite((x+13)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
        rotatesprite((x+17)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[1]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
    }
    else rotatesprite((x+13)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
}

void weaponnum999(uint8_t  ind,int32_t x,int32_t y,int32_t num1, int32_t num2,uint8_t  ha)
{
    char  dabuf[80] = {0};

    rotatesprite((x-7)<<16,y<<16,65536L,0,THREEBYFIVE+ind+1,ha-10,7,10+128,0,0,xdim-1,ydim-1);
    rotatesprite((x-4)<<16,y<<16,65536L,0,THREEBYFIVE+10,ha,0,10+128,0,0,xdim-1,ydim-1);
    rotatesprite((x+13)<<16,y<<16,65536L,0,THREEBYFIVE+11,ha,0,10+128,0,0,xdim-1,ydim-1);

    sprintf(dabuf,"%d",num1);
    if(num1 > 99)
    {
        rotatesprite((x)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
        rotatesprite((x+4)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[1]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
        rotatesprite((x+8)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[2]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
    }
    else if(num1 > 9)
    {
        rotatesprite((x+4)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
        rotatesprite((x+8)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[1]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
    }
    else rotatesprite((x+8)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);

    sprintf(dabuf,"%d",num2);
    if(num2 > 99)
    {
        rotatesprite((x+17)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
        rotatesprite((x+21)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[1]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
        rotatesprite((x+25)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[2]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
    }
    else if(num2 > 9)
    {
        rotatesprite((x+17)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
        rotatesprite((x+21)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[1]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
    }
    else rotatesprite((x+25)<<16,y<<16,65536L,0,THREEBYFIVE+dabuf[0]-'0',ha,0,10+128,0,0,xdim-1,ydim-1);
}


    //REPLACE FULLY
void weapon_amounts(struct player_struct *p,int32_t x,int32_t y, uint32_t u) // next_hack: was int32_t u. Bug ?
{
     int cw;

     cw = p->curr_weapon;

     if (u&4)
     {
         if (u != 0xffffffff) patchstatusbar(96,178,96+12,178+6);
         weaponnum999(PISTOL_WEAPON,x,y,
                     p->ammo_amount[PISTOL_WEAPON],max_ammo_amount[PISTOL_WEAPON],
                     12-20*(cw == PISTOL_WEAPON) );
     }
     if (u&8)
     {
         if (u != 0xffffffff) patchstatusbar(96,184,96+12,184+6);
         weaponnum999(SHOTGUN_WEAPON,x,y+6,
                     p->ammo_amount[SHOTGUN_WEAPON],max_ammo_amount[SHOTGUN_WEAPON],
                     (!p->gotweapon[SHOTGUN_WEAPON]*9)+12-18*
                     (cw == SHOTGUN_WEAPON) );
     }
     if (u&16)
     {
         if (u != 0xffffffff) patchstatusbar(96,190,96+12,190+6);
         weaponnum999(CHAINGUN_WEAPON,x,y+12,
                      p->ammo_amount[CHAINGUN_WEAPON],max_ammo_amount[CHAINGUN_WEAPON],
                      (!p->gotweapon[CHAINGUN_WEAPON]*9)+12-18*
                      (cw == CHAINGUN_WEAPON) );
     }
     if (u&32)
     {
         if (u != 0xffffffff) patchstatusbar(135,178,135+8,178+6);
         weaponnum(RPG_WEAPON,x+39,y,
                  p->ammo_amount[RPG_WEAPON],max_ammo_amount[RPG_WEAPON],
                  (!p->gotweapon[RPG_WEAPON]*9)+12-19*
                  (cw == RPG_WEAPON) );
     }
     if (u&64)
     {
         if (u != 0xffffffff) patchstatusbar(135,184,135+8,184+6);
         weaponnum(HANDBOMB_WEAPON,x+39,y+6,
                     p->ammo_amount[HANDBOMB_WEAPON],max_ammo_amount[HANDBOMB_WEAPON],
                     (((!p->ammo_amount[HANDBOMB_WEAPON])|(!p->gotweapon[HANDBOMB_WEAPON]))*9)+12-19*
                     ((cw == HANDBOMB_WEAPON) || (cw == HANDREMOTE_WEAPON)));
     }
     if (u&128)
     {
         if (u != 0xffffffff) patchstatusbar(135,190,135+8,190+6);

		if(VOLUMEONE)
		{
			orderweaponnum(SHRINKER_WEAPON,x+39,y+12,
	                     p->ammo_amount[SHRINKER_WEAPON],max_ammo_amount[SHRINKER_WEAPON],
	                     (!p->gotweapon[SHRINKER_WEAPON]*9)+12-18*
	                     (cw == SHRINKER_WEAPON) );
		}
		else
		{
			if(p->subweapon&(1<<GROW_WEAPON))
	             weaponnum(SHRINKER_WEAPON,x+39,y+12,
	                 p->ammo_amount[GROW_WEAPON],max_ammo_amount[GROW_WEAPON],
	                 (!p->gotweapon[GROW_WEAPON]*9)+12-18*
	                 (cw == GROW_WEAPON) );
	         else
	             weaponnum(SHRINKER_WEAPON,x+39,y+12,
	                 p->ammo_amount[SHRINKER_WEAPON],max_ammo_amount[SHRINKER_WEAPON],
	                 (!p->gotweapon[SHRINKER_WEAPON]*9)+12-18*
	                 (cw == SHRINKER_WEAPON) );
		}
     }
     if (u&256)
     {
		if (u != 0xffffffff) patchstatusbar(166,178,166+8,178+6);

		if(VOLUMEONE)
		{
	        orderweaponnum(DEVISTATOR_WEAPON,x+70,y,
                     p->ammo_amount[DEVISTATOR_WEAPON],max_ammo_amount[DEVISTATOR_WEAPON],
                     (!p->gotweapon[DEVISTATOR_WEAPON]*9)+12-18*
                     (cw == DEVISTATOR_WEAPON) );
		}
		else
		{
	         weaponnum(DEVISTATOR_WEAPON,x+70,y,
	                     p->ammo_amount[DEVISTATOR_WEAPON],max_ammo_amount[DEVISTATOR_WEAPON],
	                     (!p->gotweapon[DEVISTATOR_WEAPON]*9)+12-18*
	                     (cw == DEVISTATOR_WEAPON) );
		}
     }
     if (u&512)
     {
		if (u != 0xffffffff) patchstatusbar(166,184,166+8,184+6);
		if(VOLUMEONE)
		{
	         orderweaponnum(TRIPBOMB_WEAPON,x+70,y+6,
	                 p->ammo_amount[TRIPBOMB_WEAPON],max_ammo_amount[TRIPBOMB_WEAPON],
	                 (!p->gotweapon[TRIPBOMB_WEAPON]*9)+12-18*
	                 (cw == TRIPBOMB_WEAPON) );
		}
		else
		{
			weaponnum(TRIPBOMB_WEAPON,x+70,y+6,
		                     p->ammo_amount[TRIPBOMB_WEAPON],max_ammo_amount[TRIPBOMB_WEAPON],
		                     (!p->gotweapon[TRIPBOMB_WEAPON]*9)+12-18*
		                     (cw == TRIPBOMB_WEAPON) );
		}
     }

     if (u&65536L)
     {
         if (u != 0xffffffff) patchstatusbar(166,190,166+8,190+6);
		if(VOLUMEONE)
		{
			orderweaponnum(-1,x+70,y+12,
	                     p->ammo_amount[FREEZE_WEAPON],max_ammo_amount[FREEZE_WEAPON],
	                     (!p->gotweapon[FREEZE_WEAPON]*9)+12-18*
	                     (cw == FREEZE_WEAPON) );
		}
		else
		{
	         weaponnum(-1,x+70,y+12,
	                     p->ammo_amount[FREEZE_WEAPON],max_ammo_amount[FREEZE_WEAPON],
	                     (!p->gotweapon[FREEZE_WEAPON]*9)+12-18*
	                     (cw == FREEZE_WEAPON) );
		}
     }
}

void digitalnumber(int32_t x,int32_t y,int32_t n,uint8_t  s,uint8_t  cs)
{
    short i, j, k, p, c;
    char  b[10];

    //
    // uint8_t  * ltoa(int32_t l, uint8_t  * buffer, int radix);
    // is NON-STANDARD and equivalent to STANDARD
    // (void) sprintf(buffer, "%ld", l);
    //ltoa(n,b,10);
    sprintf(b,"%d",n);

    i = strlen(b);
    j = 0;

    for(k=0;k<i;k++)
    {
        p = DIGITALNUM+*(b+k)-'0';
        j += tiles[p].dim.width+1;
    }
    c = x-(j>>1);

    j = 0;
    for(k=0;k<i;k++)
    {
        p = DIGITALNUM+*(b+k)-'0';
        rotatesprite((c+j)<<16,y<<16,65536L,0,p,s,0,cs,0,0,xdim-1,ydim-1);
        j += tiles[p].dim.width+1;
    }
}

/*

void scratchmarks(int32_t x,int32_t y,int32_t n,uint8_t  s,uint8_t  p)
{
    int32_t i, ni;

    ni = n/5;
    for(i=ni;i >= 0;i--)
    {
        overwritesprite(x-2,y,SCRATCH+4,s,0,0);
        x += tilesizx[SCRATCH+4]-1;
    }

    ni = n%5;
    if(ni) overwritesprite(x,y,SCRATCH+ni-1,s,p,0);
}
  */
void displayinventory(struct player_struct *p)
{
    short n, j, xoff, y;

    j = xoff = 0;

    n = (p->jetpack_amount > 0)<<3; if(n&8) j++;
    n |= ( p->scuba_amount > 0 )<<5; if(n&32) j++;
    n |= (p->steroids_amount > 0)<<1; if(n&2) j++;
    n |= ( p->holoduke_amount > 0)<<2; if(n&4) j++;
    n |= (p->firstaid_amount > 0); if(n&1) j++;
    n |= (p->heat_amount > 0)<<4; if(n&16) j++;
    n |= (p->boot_amount > 0)<<6; if(n&64) j++;

    xoff = 160-(j*11);

    j = 0;

    if(ud.screen_size > 4)
        y = 154;
    else y = 172;

    if(ud.screen_size == 4)
    {
        if(ud.multimode > 1)
            xoff += 56;
        else xoff += 65;
    }

    while( j <= 9 )
    {
        if( n&(1<<j) )
        {
            switch( n&(1<<j) )
            {
                case   1:
                rotatesprite(xoff<<16,y<<16,65536L,0,FIRSTAID_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);break;
                case   2:
                rotatesprite((xoff+1)<<16,y<<16,65536L,0,STEROIDS_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);break;
                case   4:
                rotatesprite((xoff+2)<<16,y<<16,65536L,0,HOLODUKE_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);break;
                case   8:
                rotatesprite(xoff<<16,y<<16,65536L,0,JETPACK_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);break;
                case  16:
                rotatesprite(xoff<<16,y<<16,65536L,0,HEAT_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);break;
                case  32:
                rotatesprite(xoff<<16,y<<16,65536L,0,AIRTANK_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);break;
                case 64:
                rotatesprite(xoff<<16,(y-1)<<16,65536L,0,BOOT_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);break;
            }

            xoff += 22;

            if(p->inven_icon == j+1)
                rotatesprite((xoff-2)<<16,(y+19)<<16,65536L,1024,ARROW,-32,0,2+16,windowx1,windowy1,windowx2,windowy2);
        }

        j++;
    }
}



void displayfragbar(void)
{
    short i, j;
    char text[512];

    j = 0;

    for(i=connecthead;i>=0;i=connectpoint2[i])
        if(i > j) j = i;

    rotatesprite(0,0,65600L,0,FRAGBAR,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
    if(j >= 4) rotatesprite(319,(8)<<16,65600L,0,FRAGBAR,0,0,10+16+64+128,0,0,xdim-1,ydim-1);
    if(j >= 8) rotatesprite(319,(16)<<16,65600L,0,FRAGBAR,0,0,10+16+64+128,0,0,xdim-1,ydim-1);
    if(j >= 12) rotatesprite(319,(24)<<16,65600L,0,FRAGBAR,0,0,10+16+64+128,0,0,xdim-1,ydim-1);

    for(i=connecthead;i>=0;i=connectpoint2[i])
    {
// next-hack: the following line changed so that it works even if no game has been started.
        minitext(21+(73*(i&3)),2+((i&28)<<1),&ud.user_name[i][0],i + 9,2+8+16+128);

        sprintf(text,"%d",ps[i].frag-ps[i].fraggedself);
// next-hack: idem here
        minitext(17+50+(73*(i&3)),2+((i&28)<<1),text,i + 9,2+8+16+128);
//        minitext(17+50+(73*(i&3)),2+((i&28)<<1),text,getSprite(ps[i].i)->_pal,2+8+16+128);

    }
}

void display_boardfilename_FPS_weapon(short *offx, short *offy, short *stepx, short *stepy)
{
    UNUSED(stepx);
	short i;

	// FIX_00025: Can toggle FPS and map name during a game (use dnrate OR toggle
	//            from menu when in deathmatch).

	// Display boardfilename and FPS
	if((ud.tickrate&1 ) )
	{
		tics(*offx, *offy, COLOR_ON);
		*offy += *stepy;
	}
	if(ud.tickrate&2)
		dispVersion();

	// We display the weapons here instead of changing the function
	// displayweapon() because the display will be much faster
	for(i=connecthead;i>=0;i=connectpoint2[i])
	{
		if (ud.hideweapon && i==screenpeek)
			drawsmallweapon(ps[i].curr_weapon, 1, 130, (ud.screen_size<=4)?170:140);
	}
}


// FIX_00026: Weapon can now be hidden (on your screen only).
void drawsmallweapon(short weapon, float scale, short x, short y)
{
	float t = 60000;
	int s;
	float offsetx, offsety;


	switch(weapon)
	{
		case  KNEE_WEAPON			: s=0;					break;
		case  PISTOL_WEAPON			: s=FIRSTGUNSPRITE;
										offsetx = 8;
										offsety = 7;
										break;
		case  SHOTGUN_WEAPON		: s=SHOTGUNSPRITE;
										t = 45000;
										offsetx = -1;
										offsety = 9;
										break;
		case  CHAINGUN_WEAPON		: s=CHAINGUNSPRITE;
										t = 45000;
										offsetx = -1;
										offsety = 9;
										break;
		case  RPG_WEAPON			: s=RPGSPRITE;
										t = 45000;
										offsetx = 4;
										offsety = 9;
										break;
		case  HANDBOMB_WEAPON		: s=HEAVYHBOMB;
										t=20000;
										offsetx = 16;
										offsety = 13;
										break;
		case  SHRINKER_WEAPON		: s=SHRINKERSPRITE;
										t = 30000;
										offsetx = 6;
										offsety = 14;
										break;
		case  DEVISTATOR_WEAPON		: s=DEVISTATORSPRITE;
										t = 45000;
										offsetx = 3;
										offsety = 9;
										break;
		case  TRIPBOMB_WEAPON		: s=TRIPBOMBSPRITE;
										t = 75000;
										offsetx = 10;
										offsety = 12;
										break;
		case  FREEZE_WEAPON			:	s=FREEZESPRITE;
										t = 45000;
										offsetx = 1;
										offsety = 6;
										break;
		case  HANDREMOTE_WEAPON		: s=0;
										break;
		case  GROW_WEAPON			: s=GROWSPRITEICON;
										t = 30000;
										offsetx = 6;
										offsety = 4;
										break;
		default						: s=0;
	}

	if(s)
		rotatesprite((x+(short)(offsetx*scale))<<16,(y+(short)(offsety*scale))<<16,(int)(t*scale),0,s,0,0,2+8+16,0,0,xdim-1,ydim-1);

	return;
}

void coolgaugetext(short snum)
{
  sbarUpdated = 0;
    struct player_struct *p;
    int32_t i, j, o, ss;
    uint32_t u;
    uint8_t  permbit;
	short offx = 3, offy = 3, stepx=60, stepy=6;
    char text[512];

    p = &ps[snum];

    if (p->invdisptime > 0)
    {
        displayinventory(p);
    }


    if(ps[snum].gm&MODE_MENU)
    {
        if( (current_menu >= MENU_HELP0  && current_menu <= MENU_HELP0+5) )
            return;
    }

	offy += countfragbars(); //add fragbars
	display_boardfilename_FPS_weapon(&offx, &offy, &stepx, &stepy);


    ss = ud.screen_size; if (ss < 4) return;

    // Draw the multi player frag status bar
    if ( ud.multimode > 1 && ud.coop != 1 )
    {
        if (pus)
            {
                displayfragbar();
            }
        else
        {
            for(i=connecthead;i>=0;i=connectpoint2[i])
            {
                if (ps[i].frag != sbar.frag[i])
                {
                    displayfragbar();
                    break;
                }
            }
        }
        for(i=connecthead;i>=0;i=connectpoint2[i])
            if (i != myconnectindex)
                sbar.frag[i] = ps[i].frag;
    }

    if (ss == 4)   //DRAW MINI STATUS BAR:
    {
    	// FIX_00027: Added an extra small statusbar (HUD)
		if(ud.extended_screen_size>0)
		{
			offx = 5; offy = 160;

			sprintf(text,"%d", ps[screenpeek].ammo_amount[ps[screenpeek].curr_weapon]);
			minitext(offx+26,offy+21,text,COLOR_ON,2+8+16); //minitext: 2 red light, 23 yellow
			sprintf(text,"%d", ps[screenpeek].last_extra);
			gametext(offx,offy+20,text,ps[screenpeek].last_extra<=50?15:0,2+8+16); //minitext: 2 red light, 23 yellow
			rotatesprite((offx+0*10)<<16,(offy+28)<<16,20000,0,SHIELD,ps[screenpeek].shield_amount?25:100,0,2+8+16,0,0,xdim-1,ydim-1);
			rotatesprite((offx+0*10)<<16,(offy+28)<<16,ksqrt(ps[screenpeek].shield_amount)*20000/10,0,SHIELD,0,0,2+8+16,0,0,xdim-1,ydim-1);
			rotatesprite((offx+1*10)<<16,(offy+28)<<16,35000,0,JETPACK_ICON,ps[screenpeek].jetpack_amount?25:100,0,2+8+16,0,0,xdim-1,ydim-1);
			rotatesprite((offx+1*10)<<16,(offy+28)<<16,ksqrt(ps[screenpeek].jetpack_amount)*35000/40,0,JETPACK_ICON,0,0,2+8+16,0,0,xdim-1,ydim-1);
			rotatesprite((offx+2*10-1)<<16,(offy+28)<<16,35000,0,STEROIDS_ICON,ps[screenpeek].steroids_amount?25:100,0,2+8+16,0,0,xdim-1,ydim-1);
			rotatesprite((offx+2*10-1)<<16,(offy+28)<<16,ksqrt(ps[screenpeek].steroids_amount)*35000/20,0,STEROIDS_ICON,5,0,2+8+16,0,0,xdim-1,ydim-1);
			rotatesprite((offx+3*10-3)<<16,(offy+28)<<16,40000,0,FIRSTAID_ICON,ps[screenpeek].firstaid_amount?25:100,0,2+8+16,0,0,xdim-1,ydim-1);
			rotatesprite((offx+3*10-3)<<16,(offy+28)<<16,ksqrt(ps[screenpeek].firstaid_amount)*40000/10,0,FIRSTAID_ICON,0,0,2+8+16,0,0,xdim-1,ydim-1);
		}
		else
		{
			if (p->inven_icon)
				rotatesprite(69<<16,(200-30)<<16,65536L,0,INVENTORYBOX,0,21,10+16,0,0,xdim-1,ydim-1);
			rotatesprite(5<<16,(200-28)<<16,65536L,0,HEALTHBOX,0,21,10+16,0,0,xdim-1,ydim-1);

			if(getSprite(p->i)->_pal == 1 && p->last_extra < 2)
				digitalnumber(20,200-17,1,-16,10+16);
			else digitalnumber(20,200-17,p->last_extra,-16,10+16);

			rotatesprite(37<<16,(200-28)<<16,65536L,0,AMMOBOX,0,21,10+16,0,0,xdim-1,ydim-1);

			if (p->curr_weapon == HANDREMOTE_WEAPON) i = HANDBOMB_WEAPON; else i = p->curr_weapon;
			digitalnumber(53,200-17,p->ammo_amount[i],-16,10+16);

			o = 158; permbit = 0;
			if (p->inven_icon)
			{
				switch(p->inven_icon)
				{
					case 1: i = FIRSTAID_ICON; break;
					case 2: i = STEROIDS_ICON; break;
					case 3: i = HOLODUKE_ICON; break;
					case 4: i = JETPACK_ICON; break;
					case 5: i = HEAT_ICON; break;
					case 6: i = AIRTANK_ICON; break;
					case 7: i = BOOT_ICON; break;
					default: i = -1;
				}
				if (i >= 0) rotatesprite((231-o)<<16,(200-21)<<16,65536L,0,i,0,0,10+16+permbit,0,0,xdim-1,ydim-1);

				minitext(292-30-o,190,"%",6,10+16+permbit);

				j = 0x80000000;
				switch(p->inven_icon)
				{
					case 1: i = p->firstaid_amount; break;
					case 2: i = ((p->steroids_amount+3)>>2); break;
					case 3: i = ((p->holoduke_amount+15)/24); j = p->holoduke_on; break;
					case 4: i = ((p->jetpack_amount+15)>>4); j = p->jetpack_on; break;
					case 5: i = p->heat_amount/12; j = p->heat_on; break;
					case 6: i = ((p->scuba_amount+63)>>6); break;
					case 7: i = (p->boot_amount>>1); break;
				}
				invennum(284-30-o,200-6,(uint8_t )i,0,10+permbit);
				if (j > 0) minitext(288-30-o,180,"ON",0,10+16+permbit);
				else if (j != (int) 0x80000000) minitext(284-30-o,180,"OFF",2,10+16+permbit);
				if (p->inven_icon >= 6) minitext(284-35-o,180,"AUTO",2,10+16+permbit);
			}
		}
        return;
    }

        //DRAW/UPDATE FULL STATUS BAR:

    if (pus) { pus = 0; u = 0xffffffff; } else u = 0;

    if (sbar.frag[myconnectindex] != p->frag) { sbar.frag[myconnectindex] = p->frag; u |= 32768; }
    if (sbar.got_access != p->got_access) { sbar.got_access = p->got_access; u |= 16384; }
    if (sbar.last_extra != p->last_extra) { sbar.last_extra = p->last_extra; u |= 1; }
    if (sbar.shield_amount != p->shield_amount) { sbar.shield_amount = p->shield_amount; u |= 2; }
    if (sbar.curr_weapon != p->curr_weapon) { sbar.curr_weapon = p->curr_weapon; u |= (4+8+16+32+64+128+256+512+1024+65536L); }
    for(i=1;i < 10;i++)
    {
        if (sbar.ammo_amount[i] != p->ammo_amount[i]) {
        sbar.ammo_amount[i] = p->ammo_amount[i]; if(i < 9) u |= ((2<<i)+1024); else u |= 65536L+1024; }
        if (sbar.gotweapon[i] != p->gotweapon[i]) { sbar.gotweapon[i] =
        p->gotweapon[i]; if(i < 9 ) u |= ((2<<i)+1024); else u |= 65536L+1024; }
    }
    if (sbar.inven_icon != p->inven_icon) { sbar.inven_icon = p->inven_icon; u |= (2048+4096+8192); }
    if (sbar.holoduke_on != p->holoduke_on) { sbar.holoduke_on = p->holoduke_on; u |= (4096+8192); }
    if (sbar.jetpack_on != p->jetpack_on) { sbar.jetpack_on = p->jetpack_on; u |= (4096+8192); }
    if (sbar.heat_on != p->heat_on) { sbar.heat_on = p->heat_on; u |= (4096+8192); }
    if (sbar.firstaid_amount != p->firstaid_amount) { sbar.firstaid_amount = p->firstaid_amount; u |= 8192; }
    if (sbar.steroids_amount != p->steroids_amount) { sbar.steroids_amount = p->steroids_amount; u |= 8192; }
    if (sbar.holoduke_amount != p->holoduke_amount) { sbar.holoduke_amount = p->holoduke_amount; u |= 8192; }
    if (sbar.jetpack_amount != p->jetpack_amount) { sbar.jetpack_amount = p->jetpack_amount; u |= 8192; }
    if (sbar.heat_amount != p->heat_amount) { sbar.heat_amount = p->heat_amount; u |= 8192; }
    if (sbar.scuba_amount != p->scuba_amount) { sbar.scuba_amount = p->scuba_amount; u |= 8192; }
    if (sbar.boot_amount != p->boot_amount) { sbar.boot_amount = p->boot_amount; u |= 8192; }
    // next-hack: if we need to update anything on the statusbar
    // and the statusbar was garbaged by the camera sprite, then redraw everything.
    if (u != 0xFFFFFFFF && u && useRenderedCamData())
    {
        u = 0xFFFFFFFF;
    }
    if (u == 0)
      return;
    sbarUpdated = 1;
    //0 - update health
    //1 - update armor
    //2 - update PISTOL_WEAPON ammo
    //3 - update SHOTGUN_WEAPON ammo
    //4 - update CHAINGUN_WEAPON ammo
    //5 - update RPG_WEAPON ammo
    //6 - update HANDBOMB_WEAPON ammo
    //7 - update SHRINKER_WEAPON ammo
    //8 - update DEVISTATOR_WEAPON ammo
    //9 - update TRIPBOMB_WEAPON ammo
    //10 - update ammo display
    //11 - update inventory icon
    //12 - update inventory on/off
    //13 - update inventory %
    //14 - update keys
    //15 - update kills
    //16 - update FREEZE_WEAPON ammo

    if (u == 0xfffffffful)
    {
        patchstatusbar(0,0,320,200);
        if (ud.multimode > 1 && ud.coop != 1)
            rotatesprite(277<<16,(200-27)<<16,65536L,0,KILLSICON,0,0,10+16+128,0,0,xdim-1,ydim-1);
    }
    if (ud.multimode > 1 && ud.coop != 1)
    {
        if (u&32768)
        {
            if (u != 0xfffffffful) patchstatusbar(276,183,299,193);
            digitalnumber(287,200-17,max(p->frag-p->fraggedself,0),-16,10+16+128);
        }
    }
    else
    {
        if (u&16384)
        {
            if (u != 0xfffffffful) patchstatusbar(275,182,299,194);
            if (p->got_access&4) rotatesprite(275<<16,182<<16,65536L,0,ACCESS_ICON,0,23,10+16+128,0,0,xdim-1,ydim-1);
            if (p->got_access&2) rotatesprite(288<<16,182<<16,65536L,0,ACCESS_ICON,0,21,10+16+128,0,0,xdim-1,ydim-1);
            if (p->got_access&1) rotatesprite(281<<16,189<<16,65536L,0,ACCESS_ICON,0,0,10+16+128,0,0,xdim-1,ydim-1);
        }
    }
    if (u&(4+8+16+32+64+128+256+512+65536L)) weapon_amounts(p,96,182,u);

    if (u&1)
    {
        if (u != 0xfffffffful) patchstatusbar(20,183,43,193);
        if(getSprite(p->i)->_pal == 1 && p->last_extra < 2)
            digitalnumber(32,200-17,1,-16,10+16+128);
        else digitalnumber(32,200-17,p->last_extra,-16,10+16+128);
    }
    if (u&2)
    {
        if (u != 0xfffffffful) patchstatusbar(52,183,75,193);
        digitalnumber(64,200-17,p->shield_amount,-16,10+16+128);
    }

    if (u&1024)
    {
        if (u != 0xfffffffful) patchstatusbar(196,183,219,193);
        if (p->curr_weapon != KNEE_WEAPON)
        {
            if (p->curr_weapon == HANDREMOTE_WEAPON) i = HANDBOMB_WEAPON; else i = p->curr_weapon;
            digitalnumber(230-22,200-17,p->ammo_amount[i],-16,10+16+128);
        }
    }

    if (u&(2048+4096+8192))
    {
        if (u != 0xfffffffful)
        {
            if (u&(2048+4096)) { patchstatusbar(231,179,265,197); }
                              else { patchstatusbar(250,190,261,195); }
        }
        if (p->inven_icon)
        {
            o = 0; permbit = 128;

            if (u&(2048+4096))
            {
                switch(p->inven_icon)
                {
                    case 1: i = FIRSTAID_ICON; break;
                    case 2: i = STEROIDS_ICON; break;
                    case 3: i = HOLODUKE_ICON; break;
                    case 4: i = JETPACK_ICON; break;
                    case 5: i = HEAT_ICON; break;
                    case 6: i = AIRTANK_ICON; break;
                    case 7: i = BOOT_ICON; break;
                }
                rotatesprite((231-o)<<16,(200-21)<<16,65536L,0,i,0,0,10+16+permbit,0,0,xdim-1,ydim-1);
                minitext(292-30-o,190,"%",6,10+16+permbit);
                if (p->inven_icon >= 6) minitext(284-35-o,180,"AUTO",2,10+16+permbit);
            }
            if (u&(2048+4096))
            {
                switch(p->inven_icon)
                {
                    case 3: j = p->holoduke_on; break;
                    case 4: j = p->jetpack_on; break;
                    case 5: j = p->heat_on; break;
                    default: j = 0x80000000;
                }
                if (j > 0) minitext(288-30-o,180,"ON",0,10+16+permbit);
                else if (j != (int) 0x80000000) minitext(284-30-o,180,"OFF",2,10+16+permbit);
            }
            if (u&8192)
            {
                switch(p->inven_icon)
                {
                    case 1: i = p->firstaid_amount; break;
                    case 2: i = ((p->steroids_amount+3)>>2); break;
                    case 3: i = ((p->holoduke_amount+15)/24); break;
                    case 4: i = ((p->jetpack_amount+15)>>4); break;
                    case 5: i = p->heat_amount/12; break;
                    case 6: i = ((p->scuba_amount+63)>>6); break;
                    case 7: i = (p->boot_amount>>1); break;
                }
                invennum(284-30-o,200-6,(uint8_t )i,0,10+permbit);
            }
        }
    }
}

#if 0
#define AVERAGEFRAMES 16
static int32_t frameval[AVERAGEFRAMES], framecnt = 0;
#endif
void tics(short offx, short offy, short color)
{
#if 0

  int32_t i;
	char  fps[512], mapname[512];
	int32_t currentFps;
	static int32_t fpsAvg = 0, savedFps = 0;
	static boolean toggle = true;
    char text[512];

	strcpy(mapname,boardfilename);
	for(i=0;i<512;i++)
		if(mapname[i]=='.')
			mapname[i]=0;

	if( mapname[0] != 0 && ud.m_level_number == 7 && ud.m_volume_number == 0 )
		sprintf(text, "%s", mapname);
    else
		//sprintf(tempbuf, "%s", level_names[ud.volume_number*11 + ud.level_number]);
		sprintf(text, "e%dl%d", ud.volume_number+1, ud.level_number+1);

    i = totalclock;

    if (i != frameval[framecnt])
    {
		currentFps = (TICRATE*AVERAGEFRAMES)/(i-frameval[framecnt]);
		fpsAvg = ((fpsAvg<<3)+(fpsAvg<<2) + (currentFps<<2))>>4;

		frameval[framecnt] = i;
    }

    framecnt = ((framecnt+1)&(AVERAGEFRAMES-1));

	// refresh screen and update visible FPS. This is to allow a refresh
	// of the screen when the screensize > 4 w/o compromising the FPS.
    if(ud.screen_size>8)
		if ((totalclock%64) < 32)
		{
			if(toggle)
			{
				vscrn();
				savedFps = fpsAvg;
			}
			toggle = false;
		}
		else
		{
			toggle = true;
		}
	else
		savedFps = fpsAvg;

	sprintf(fps," %d", savedFps);
	strcat(text, fps);

	minitext(offx,offy,text,color,2+8+16+128);
#else
	static uint32_t lastTimeUs = 0;
	static uint8_t toggle;
	char  fps[64], mapname[64];
  char text[64];

  strncpy(mapname,boardfilename, sizeof(mapname));
  for(uint32_t i=0; i < sizeof(mapname); i++)
    if(mapname[i]=='.')
      mapname[i]=0;

  if( mapname[0] != 0 && ud.m_level_number == 7 && ud.m_volume_number == 0 )
    snprintf(text, sizeof(text), "%s", mapname);
    else
    //sprintf(tempbuf, "%s", level_names[ud.volume_number*11 + ud.level_number]);
    snprintf(text, sizeof(text), "e%dl%d", ud.volume_number+1, ud.level_number+1);

  // refresh screen and update visible FPS. This is to allow a refresh
  // of the screen when the screensize > 4 w/o compromising the FPS.
    if(ud.screen_size>8)
    {
    if ((totalclock%64) < 32)
    {
      if(toggle)
      {
        vscrn();
      }
      toggle = false;
    }
    else
    {
      toggle = true;
    }
    }
#if MG24
  uint32_t timeNow = I_GetTimeMicrosecs();
#else
    uint32_t timeNow = SDL_GetTicks() * 1000;
#endif
  int fps10 = 999;
  if (timeNow - lastTimeUs) // prevent division by 0
    fps10 = 10000000 / (timeNow - lastTimeUs);
  lastTimeUs = timeNow;
  snprintf(fps, sizeof (fps), " %d.%01d", fps10 / 10, fps10 % 10);
  strncat(text, fps, sizeof(text)-1);
  minitext(offx,offy,text,color,2+8+16+128);
#endif
}
#if PRINT_COORDS_SUPPORT

void coords(short snum)
{
    short x = 200, y = 0;
    char text[512];
    // x = 250 is too much on the right and
    // will make the text going out of the screen
    // if screen <= (320x200)
    // This will also *write beyond the video
    // buffer limit* and will crash the game.

    if(ud.coop != 1)
    {
        if(ud.multimode > 1 && ud.multimode < 5)
            y = 8;
        else if(ud.multimode > 4)
            y = 16;
    }

    sprintf(text,"X= %d",ps[snum].posx);
    printext256(x,y,31,-1,text,1);
    sprintf(text,"Y= %d",ps[snum].posy);
    printext256(x,y+7L,31,-1,text,1);
    sprintf(text,"Z= %d",ps[snum].posz);
    printext256(x,y+14L,31,-1,text,1);
    sprintf(text,"A= %d",ps[snum].ang);
    printext256(x,y+21L,31,-1,text,1);
    sprintf(text,"ZV= %d",ps[snum].poszv);
    printext256(x,y+28L,31,-1,text,1);
    sprintf(text,"OG= %d",ps[snum].on_ground);
    printext256(x,y+35L,31,-1,text,1);
    sprintf(text,"AM= %d",ps[snum].ammo_amount[GROW_WEAPON]);
    printext256(x,y+43L,31,-1,text,1);
    sprintf(text,"LFW= %d",ps[snum].last_full_weapon);
    printext256(x,y+50L,31,-1,text,1);
    sprintf(text,"SECTL= %d",getSector(ps[snum].cursectnum)->lotag);
    printext256(x,y+57L,31,-1,text,1);
    sprintf(text,"SEED= %d",randomseed);
    printext256(x,y+64L,31,-1,text,1);
    sprintf(text,"THOLD= %d",ps[snum].transporter_hold);
    printext256(x,y+64L+7,31,-1,text,1);
}
#endif
char *get_fta_quotes(int q)
{
    if (q == CUSTOM_FTA)
        return ftaCustomText;
    else
        return (char*) &fta_quotes[q];
}
void operatefta(void)
{
     int32_t i, j, k;

     if(ud.screen_size > 0) j = 200-45; else j = 200-8;
     quotebot = min(quotebot,j);
     quotebotgoal = min(quotebotgoal,j);
     if(ps[myconnectindex].gm&MODE_TYPE) j -= 8;
     quotebotgoal = j; j = quotebot;
     for(i=0;i<MAXUSERQUOTES;i++)
     {
         k = user_quote_time[i]; if (k <= 0) break;

         if (k > 4)
              gametext(320>>1,j,user_quote[i],0,2+8+16);
         else if (k > 2) gametext(320>>1,j,user_quote[i],0,2+8+16+1);
             else gametext(320>>1,j,user_quote[i],0,2+8+16+1+32);
         j -= 8;
     }

     if (ps[screenpeek].fta <= 1) return;

     if (ud.coop != 1 && ud.screen_size > 0 && ud.multimode > 1)
     {
         j = 0; k = 8;
         for(i=connecthead;i>=0;i=connectpoint2[i])
             if (i > j) j = i;

         if (j >= 4 && j <= 8) k += 8;
         else if (j > 8 && j <= 12) k += 16;
         else if (j > 12) k += 24;
     }
     else k = 0;

     if (ps[screenpeek].ftq == 115 || ps[screenpeek].ftq == 116)
     {
         k = quotebot;
         for(i=0;i<MAXUSERQUOTES;i++)
         {
             if (user_quote_time[i] <= 0) break;
             k -= 8;
         }
         k -= 4;
     }

     j = ps[screenpeek].fta;
     if (j > 4)
          gametext(320>>1,k,get_fta_quotes(ps[screenpeek].ftq),0,2+8+16);
     else
         if (j > 2) gametext(320>>1,k,get_fta_quotes(ps[screenpeek].ftq),0,2+8+16+1);
     else
         gametext(320>>1,k,get_fta_quotes(ps[screenpeek].ftq),0,2+8+16+1+32);
}

void FTA(short q,struct player_struct *p, int mode)
{
    ud.fta_on = 1;
    if( ud.fta_on == 1 || mode)
    {
        if( p->fta > 0 && q != 115 && q != 116 )
            if( p->ftq == 115 || p->ftq == 116 ) return;

        p->fta = 100;  // this is the time it has to be there.

        if( p->ftq != q || q == 26 || q == CUSTOM_FTA)
        // || q == 26 || q == 115 || q ==116 || q == 117 || q == 122 )
        {
            p->ftq = q;
            pub = NUMPAGES;
            pus = NUMPAGES;
        }
    }
}
#if 0
void showtwoscreens(void)
{
    short i;

	if(VOLUMEONE)
	{
	    setview(0,0,xdim-1,ydim-1);
	    flushperms();
	    ps[myconnectindex].palette = palette;
	    for(i=0;i<64;i+=7) palto(0,0,0,i,true);
	    KB_FlushKeyboardQueue();

	    rotatesprite(0,0,65536L,0,3291,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);

	    nextpage(SCREEN_HEIGHT);
	    waitForUpdate(-1);
		for(i=63;i>0;i-=7) palto(0,0,0,i,true);

	    while( !KB_KeyWaiting() ); // getpackets(); // Net already off. Trying to get packets here makes sporadic crash..

	    for(i=0;i<64;i+=7)
			palto(0,0,0,i,true);

	    KB_FlushKeyboardQueue();

	    rotatesprite(0,0,65536L,0,3290,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
	    nextpage(SCREEN_HEIGHT);
	    waitForUpdate(-1);
		for(i=63;i>0;i-=7)
			palto(0,0,0,i,true);

	    while( !KB_KeyWaiting() ); // getpackets();
	}
	else if(PLUTOPAK)
	{
		setview(0,0,xdim-1,ydim-1);
	    flushperms();
	    ps[myconnectindex].palette = palette;

	    for(i=0;i<64;i+=7)
			palto(0,0,0,i,true);

	    KB_FlushKeyboardQueue();

	    clearview(0L);
	    rotatesprite(0,0,65536L,0,TENSCREEN,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
	    nextpage(SCREEN_HEIGHT);
	    waitForUpdate(-1);
		for(i=63;i>0;i-=7)
			palto(0,0,0,i,true);

	    totalclock = 0;

	    while( !KB_KeyWaiting() );

	}
}
#endif
#if WIN32
void gameexit2(const char *msg, const char *file, const char*function, int line)
#else
void gameexit(const char  *msg)
#endif
{
    #if WIN32
    printf("game exit: %s %s line %i\r\n", file, function, line);
    #endif // WIN32
#if 0
    char  t[256];

    strncpy(t,msg,256); t[255] = 0;

    if(*t != 0) ps[myconnectindex].palette = (uint8_t  *) &palette[0];

    if(numplayers > 1)
        allowtimetocorrecterrorswhenquitting();

    if(ud.recstat == 1)
        closedemowrite();

    if(frecfilep != NULL)
    {
        fclose(frecfilep);
        frecfilep = NULL;
    }

    if(qe || cp)
        goto GOTOHERE;

	// FIX_00089: scoreboard not shown for last player who quits a DM. Only 19.7 affected. (Sarah)
    if( ud.m_recstat != 2 && ud.last_level >= 0 && playerswhenstarted > 1 && ud.coop != 1 && *t == ' ')
    {
        dobonus(1);
// CTW - MODIFICATION
//      setgamemode();
// FIX_00028: No need to call the videodriver on gameexit()
//		setgamemode(ScreenMode,ScreenWidth,ScreenHeight);
// CTW END - MODIFICATION
    }

    if(playerswhenstarted > 1)
        uninitmultiplayers();  /* deinits network transport. */

// CTW - MODIFICATION
/*  if( *t != 0 && *(t+1) != 'V' && *(t+1) != 'Y' && playonten == 0 )
        showtwoscreens();*/
    if( *t != 0 && *(t+1) != 'V' && *(t+1) != 'Y' && true)
    {
		if(ud.showcinematics) // FIX_00029: toggle cinematics on / off
			showtwoscreens();
    }
// CTW END - MODIFICATION

    GOTOHERE:

    Shutdown();

    if(*t != 0)
    {
        setvmode(0x3);

// CTW - MODIFICATION
/*      if(playonten == 0)
        {
            if(*t == ' ' && *(t+1) == 0) *t = 0;
            printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            printf("%s%s","\n",t);
        }*/
        if(true)
        {
            if(*t == ' ' && *(t+1) == 0) *t = 0;
            printf("\n%s\n",t);
        }
// CTW END - MODIFICATION
    }
#if !USE_EXT_MEM
    uninitgroupfile();
#endif
    unlink("duke3d.tmp");
#endif
    Error(EXIT_SUCCESS, msg);

}




short inputloc = 0;
short strget(short x,short y,char  *t,short dalen,short c)
{
    short ch,sc;

    while(KB_KeyWaiting())
    {
        sc = 0;
        ch = KB_Getch();

        if (ch == 0)
        {

            sc = KB_Getch();
            if( sc == 104) return(1);

            continue;
        }
        else
        {
            if(ch == 8) // asc_BackSpace
            {
                if( inputloc > 0 )
                {
                    inputloc--;
                    *(t+inputloc) = 0;
                }
            }
            else
            {
                if(ch == asc_Enter || sc == 104)
                {
                    KB_ClearKeyDown(sc_Enter);
                    KB_ClearKeyDown(sc_kpad_Enter);
                    return (1);
                }
                else if(ch == asc_Escape)
                {
                    KB_ClearKeyDown(sc_Escape);
                    return (-1);
                }
                else if ( ch >= 32 && inputloc < dalen && ch < 127)
                {
                    ch = toupper(ch);
                    *(t+inputloc) = ch;
                    *(t+inputloc+1) = 0;
                    inputloc++;
                }
            }
        }
    }

    if( c == 999 ) return(0);
    if( c == 998 )
    {
        char  b[41];
        int ii;
        for(ii=0;ii<inputloc;ii++)
            b[ii] = '*';
        b[ii] = 0;
        x = gametext(x,y,b,c,2+8+16);
    }
    else x = gametext(x,y,t,c,2+8+16);
    c = 4-(sintable[(totalclock<<4)&2047]>>11);
    rotatesprite((x+8)<<16,(y+4)<<16,32768L,0,SPINNINGNUKEICON+((totalclock>>3)%7),c,0,2+8,0,0,xdim-1,ydim-1);

    return (0);
}

void typemode(void)
{
    #if NOT_YET_IMPLEMENTED
    short ch, hitstate, i, j;
    char text[512];

     if( ps[myconnectindex].gm&MODE_SENDTOWHOM )
     {
          if(sendmessagecommand != -1 || ud.multimode < 3 || movesperpacket == 4)
          {
                text[0] = 4; // message command
                text[1] = 0;
                recbuf[0]  = 0;

                if(ud.multimode < 3)
                     sendmessagecommand = 2;

                strcat(recbuf,ud.user_name[myconnectindex]);
                strcat(recbuf,": ");
                strcat(recbuf,typebuf);
                j = strlen(recbuf);
                recbuf[j] = 0;
                strcat(text+1,recbuf);

                if(sendmessagecommand >= ud.multimode || movesperpacket == 4)
                {
                     for(ch=connecthead;ch >= 0;ch=connectpoint2[ch])
                          if (ch != myconnectindex)
                                sendpacket(ch,(uint8_t*)tempbuf,j+1);

                     adduserquote(recbuf);
                     quotebot += 8;
                     quotebotgoal = quotebot;
                }
                else if(sendmessagecommand >= 0)
                     sendpacket(sendmessagecommand,(uint8_t*)tempbuf,j+1);

                sendmessagecommand = -1;
                ps[myconnectindex].gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
          }
          else if(sendmessagecommand == -1)
          {
                j = 50;
                gametext(320>>1,j,"SEND MESSAGE TO...",0,2+8+16); j += 8;
                for(i=connecthead;i>=0;i=connectpoint2[i])
//                for(i=0;i<ud.multimode;i++)
                {
                     if (i == myconnectindex)
                     {
                         minitextshade((320>>1)-40+1,j+1,"A/ENTER - ALL",26,0,2+8+16);
                         minitext((320>>1)-40,j,"A/ENTER - ALL",0,2+8+16); j += 7;
                     }
                     else
                     {
                         sprintf(buf,"      %d - %s",i+1,ud.user_name[i]);
                         minitextshade((320>>1)-40-6+1,j+1,buf,26,0,2+8+16);
                         minitext((320>>1)-40-6,j,buf,0,2+8+16); j += 7;
                     }
                }
                minitextshade((320>>1)-40-4+1,j+1,"    ESC - Abort",26,0,2+8+16);
                minitext((320>>1)-40-4,j,"    ESC - Abort",0,2+8+16); j += 7;

                //sprintf(buf,"PRESS 1-%ld FOR INDIVIDUAL PLAYER.",ud.multimode);
                //gametext(320>>1,j,buf,0,2+8+16); j += 8;
                //gametext(320>>1,j,"'A' OR 'ENTER' FOR ALL PLAYERS",0,2+8+16); j += 8;
                //gametext(320>>1,j,"ESC ABORTS",0,2+8+16); j += 8;

                if (ud.screen_size > 0) j = 200-45; else j = 200-8;
                gametext(320>>1,j,typebuf,0,2+8+16);

                if( KB_KeyWaiting() )
                {
                     i = KB_Getch();

                     if(i == 'A' || i == 'a' || i == 13)
                          sendmessagecommand = ud.multimode;
                     else if(i >= '1' || i <= (ud.multimode + '1') )
                          sendmessagecommand = i - '1';
                     else
                     {
                        sendmessagecommand = ud.multimode;
                          if(i == 27)
                          {
                              ps[myconnectindex].gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
                              sendmessagecommand = -1;
                          }
                          else
                          typebuf[0] = 0;
                     }

                     KB_ClearKeyDown(sc_1);
                     KB_ClearKeyDown(sc_2);
                     KB_ClearKeyDown(sc_3);
                     KB_ClearKeyDown(sc_4);
                     KB_ClearKeyDown(sc_5);
                     KB_ClearKeyDown(sc_6);
                     KB_ClearKeyDown(sc_7);
                     KB_ClearKeyDown(sc_8);
                     KB_ClearKeyDown(sc_A);
                     KB_ClearKeyDown(sc_Escape);
                     KB_ClearKeyDown(sc_Enter);
                }
          }
     }
     else
     {
          if(ud.screen_size > 0) j = 200-45; else j = 200-8;
          hitstate = strget(320>>1,j,typebuf,30,1);

          if(hitstate == 1)
          {
                KB_ClearKeyDown(sc_Enter);
                ps[myconnectindex].gm |= MODE_SENDTOWHOM;
          }
          else if(hitstate == -1)
                ps[myconnectindex].gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
          else pub = NUMPAGES;
     }
     #endif
}
#if CLOUD_SUPPORT
void moveclouds(void)
{
    if( totalclock > cloudtotalclock || totalclock < (cloudtotalclock-7))
    {
        short i;

        cloudtotalclock = totalclock+6;

        for(i=0;i<numclouds;i++)
        {
            cloudx[i] += (sintable[(ps[screenpeek].ang+512)&2047]>>9);
            cloudy[i] += (sintable[ps[screenpeek].ang&2047]>>9);

            setSector_ceilingxpanning(clouds[i], cloudx[i]>>6);
            setSector_ceilingypanning(clouds[i], cloudy[i]>>6);
        }
    }
}
#endif
#if LASTX_IN_STACK
extern int16_t *lastx;
#endif // LASTX_IN_STACK


void displayrest(int32_t smoothratio)
{
    int32_t a, i, j;

    struct player_struct *pp;
    int32_t cposx,cposy,cang;

    pp = &ps[screenpeek];


    if(ud.show_help)
    {
        switch(ud.show_help)
        {
            case 1:
                rotatesprite(0,0,65536L,0,TEXTSTORY,0,0,10+16+64, 0,0,xdim-1,ydim-1);
                break;
            case 2:
                rotatesprite(0,0,65536L,0,F1HELP,0,0,10+16+64, 0,0,xdim-1,ydim-1);
                break;
        }

        if ( KB_KeyPressed(sc_Escape ) )
        {
            KB_ClearKeyDown(sc_Escape);
            ud.show_help = 0;
            if(ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
            vscrn();
        }

        return;
    }

    i = pp->cursectnum;

    show2dsector[i>>3] |= (1<<(i&7));
    int wallIdx = constsector[i].wallptr;
    for(j=constsector[i].wallnum;j>0;j--, wallIdx++)
    {
        i = constwall[wallIdx].nextsector;
        if (i < 0) continue;
        if (getWall_cstat(wallIdx)&0x0071) continue;
        if (getWall_cstat(constwall[wallIdx].nextwall)&0x0071) continue;
        if (getSector(i)->lotag == 32767) continue;
        if (getSector(i)->ceilingz >= getSector(i)->floorz) continue;
        show2dsector[i>>3] |= (1<<(i&7));
    }

    if(ud.camerasprite == -1)
    {
        if( ud.overhead_on != 2 )
        {
            if(pp->newowner >= 0)
                cameratext(pp->newowner);
            else
            {
                displayweapon(screenpeek);
                if(pp->over_shoulder_on == 0 )
                    displaymasks(screenpeek);
            }
#if CLOUD_SUPPORT
            moveclouds();
#endif
        }

        if( ud.overhead_on > 0 )
        {
                smoothratio = min(max(smoothratio,0),65536);

                dointerpolations(smoothratio);
                if( ud.scrollmode == 0 )
                {
                     if(pp->newowner == -1)
                     {
                         if (screenpeek == myconnectindex && numplayers > 1)
                         {
                             cposx = omyx+mulscale16((int32_t)(myx-omyx),smoothratio);
                             cposy = omyy+mulscale16((int32_t)(myy-omyy),smoothratio);
                             cang = omyang+mulscale16((int32_t)(((myang+1024-omyang)&2047)-1024),smoothratio);
                         }
                         else
                         {
                              cposx = pp->oposx+mulscale16((int32_t)(pp->posx-pp->oposx),smoothratio);
                              cposy = pp->oposy+mulscale16((int32_t)(pp->posy-pp->oposy),smoothratio);
                              cang = pp->oang+mulscale16((int32_t)(((pp->ang+1024-pp->oang)&2047)-1024),smoothratio);
                         }
                    }
                    else
                    {
                        cposx = pp->oposx;
                        cposy = pp->oposy;
                        cang = pp->oang;
                    }
                }
                else
                {

                     ud.fola += ud.folavel>>3;
                     ud.folx += (ud.folfvel*sintable[(512+2048-ud.fola)&2047])>>14;
                     ud.foly += (ud.folfvel*sintable[(512+1024-512-ud.fola)&2047])>>14;

                     cposx = ud.folx;
                     cposy = ud.foly;
                     cang = ud.fola;
                }

                if(ud.overhead_on == 2)
                {
#if MG24
                    waitForUpdate(SCREEN_HEIGHT - STATUS_BAR_HEIGHT);
#endif
                    clearview(0L);

                    drawmapview(cposx,cposy,pp->zoom,cang);
                }
                drawoverheadmap( cposx,cposy,pp->zoom,cang);

                restoreinterpolations();

                if(ud.overhead_on == 2)
                {
                    if(ud.screen_size > 0) a = 147;
                    else a = 182;

                    minitext(1,a+6,volume_names[ud.volume_number],0,2+8+16);
                    minitext(1,a+12,level_names[ud.volume_number*11 + ud.level_number],0,2+8+16);
                }
        }
    }
#if !OLD_UPDATE_PALETTE_POS
    // next-hack: put here, because we need to know when we need to update the palette.
	// FIX_00085: Optimized Video driver. FPS increases by +20%.
    if( pp->pals_time > 0 && pp->loogcnt == 0)
    {
        palto( pp->pals[0],
               pp->pals[1],
               pp->pals[2],
               pp->pals_time|128,
               false);
        restorepalette = 1;
    }
    else if( restorepalette )
    {
        setbrightness(ud.brightness>>2,&pp->palette[0]);
        restorepalette = 0;
    }
    else if(pp->loogcnt > 0) palto(0,64,0,(pp->loogcnt>>1)+128,false);
#endif
    if (isSbPaletteRefreshNeeded())
        pus = 1;
    coolgaugetext(screenpeek);
    operatefta();

    if( KB_KeyPressed(sc_Escape) && ud.overhead_on == 0
        && ud.show_help == 0
        && ps[myconnectindex].newowner == -1)
    {
			if( (ps[myconnectindex].gm&MODE_MENU) != MODE_MENU &&
            ps[myconnectindex].newowner == -1 &&
            (ps[myconnectindex].gm&MODE_TYPE) != MODE_TYPE)
        {
            KB_ClearKeyDown(sc_Escape);
            FX_StopAllSounds();
            clearsoundlocks();

            intomenusounds();

            ps[myconnectindex].gm |= MODE_MENU;

            if(ud.multimode < 2 && ud.recstat != 2) ready2send = 0;

            if(ps[myconnectindex].gm&MODE_GAME) cmenu(MENU_INGAME_MENU);
            else cmenu(MENU_MAIN);
            screenpeek = myconnectindex;
        }
    }

    if(ps[myconnectindex].newowner == -1 && ud.overhead_on == 0 && ud.crosshair && ud.camerasprite == -1)
        rotatesprite((160L-(ps[myconnectindex].look_ang>>1))<<16,100L<<16,65536L,0,CROSSHAIR,0,0,2+1,windowx1,windowy1,windowx2,windowy2);

    if(ps[myconnectindex].gm&MODE_TYPE)
        typemode();
    else
    {
        CONSOLE_HandleInput();
        if( !CONSOLE_IsActive())
        {
#if PRINT_MENU_CALLS
            printf("MENU %d\r\n", __LINE__);
#endif
            menus();
        }
        CONSOLE_Render();
    }

    if( ud.pause_on==1 && (ps[myconnectindex].gm&MODE_MENU) == 0 )
	{
		if (!CONSOLE_IsActive()) //Addfaz Console Pause Game line addition
		{
			menutext(160,100,0,0,"GAME PAUSED");
		}
		else
		{
            menutext(160,120,0,0,"GAME PAUSED");
		}
	}
#if PRINT_COORDS_SUPPORT
    if(ud.coords)
        coords(screenpeek);
#endif
#if OLD_UPDATE_PALETTE_POS
	// FIX_00085: Optimized Video driver. FPS increases by +20%.
    if( pp->pals_time > 0 && pp->loogcnt == 0)
    {
        palto( pp->pals[0],
               pp->pals[1],
               pp->pals[2],
               pp->pals_time|128,
               false);

        restorepalette = 1;
    }
    else if( restorepalette )
    {
        setbrightness(ud.brightness>>2,&pp->palette[0]);
        restorepalette = 0;
    }
    else if(pp->loogcnt > 0) palto(0,64,0,(pp->loogcnt>>1)+128,false);
#endif

}


void updatesectorz(int32_t x, int32_t y, int32_t z, short *sectnum)
{
    int32_t i, j, cz, fz;

    getzsofslope(*sectnum,x,y,&cz,&fz);
    if ((z >= cz) && (z <= fz))
        if (inside(x,y,*sectnum) != 0) return;

    if ((*sectnum >= 0) && (*sectnum < numsectors))
    {
        int wallIdx = constsector[*sectnum].wallptr;
        j = constsector[*sectnum].wallnum;
        do
        {
            i = constwall[wallIdx].nextsector;
            if (i >= 0)
            {
                getzsofslope(i,x,y,&cz,&fz);
                if ((z >= cz) && (z <= fz))
                    if (inside(x,y,(short)i) == 1)
                        { *sectnum = i; return; }
            }
            j--;
            wallIdx++;
        } while (j != 0);
    }

    for(i=numsectors-1;i>=0;i--)
    {
        getzsofslope(i,x,y,&cz,&fz);
        if ((z >= cz) && (z <= fz))
            if (inside(x,y,(short)i) == 1)
                { *sectnum = i; return; }
    }

    *sectnum = -1;
}

void view(struct player_struct *pp, int32_t *vx, int32_t *vy,int32_t *vz,short *vsectnum, short ang, short horiz)
{
//     spritetype *sp;
     int32_t i, nx, ny, nz, hx, hy, hitx, hity, hitz;
     short bakcstat, hitsect, hitwall, hitsprite, daang;

     nx = (sintable[(ang+1536)&2047]>>4);
     ny = (sintable[(ang+1024)&2047]>>4);
     nz = (horiz-100)*128;

//     sp = &sprite[pp->i];

     bakcstat = getSprite(pp->i)->_cstat;
     setSprite_cstat(pp->i, getSprite(pp->i)->_cstat & ((short)~0x101));

     updatesectorz(*vx,*vy,*vz,vsectnum);
     hitscan(*vx,*vy,*vz,*vsectnum,nx,ny,nz,&hitsect,&hitwall,&hitsprite,&hitx,&hity,&hitz,CLIPMASK1);

     if(*vsectnum < 0)
     {
        setSprite_cstat(pp->i, bakcstat);
        return;
     }

     hx = hitx-(*vx); hy = hity-(*vy);
     if (klabs(nx)+klabs(ny) > klabs(hx)+klabs(hy))
     {
         *vsectnum = hitsect;
         if (hitwall >= 0)
         {
             daang = getangle(getWall(getWall_point2(hitwall))->x-getWall(hitwall)->x,
                                    getWall(getWall_point2(hitwall))->y-getWall(hitwall)->y);

             i = nx*sintable[daang]+ny*sintable[(daang+1536)&2047];
             if (klabs(nx) > klabs(ny)) hx -= mulscale28(nx,i);
                                          else hy -= mulscale28(ny,i);
         }
         else if (hitsprite < 0)
         {
             if (klabs(nx) > klabs(ny)) hx -= (nx>>5);
                                          else hy -= (ny>>5);
         }
         if (klabs(nx) > klabs(ny)) i = divscale16(hx,nx);
                                      else i = divscale16(hy,ny);
         if (i < cameradist) cameradist = i;
     }
     *vx = (*vx)+mulscale16(nx,cameradist);
     *vy = (*vy)+mulscale16(ny,cameradist);
     *vz = (*vz)+mulscale16(nz,cameradist);

     cameradist = min(cameradist+((totalclock-cameraclock)<<10),65536);
     cameraclock = totalclock;

     updatesectorz(*vx,*vy,*vz,vsectnum);

     setSprite_cstat(pp->i, bakcstat);
}

    //REPLACE FULLY
void drawbackground(void)
{
     short dapicnum;
     int32_t x,y,x1,y1,x2,y2;

     flushperms();

     switch(ud.m_volume_number)
     {
          default:dapicnum = BIGHOLE;break;
          case 1:dapicnum = BIGHOLE;break;
          case 2:dapicnum = BIGHOLE;break;
     }

     y1 = 0; y2 = ydim;
     if( ready2send || ud.recstat == 2 )
     {
        if(ud.coop != 1)
        {
            if (ud.multimode > 1) y1 += scale(ydim,8,200);
            if (ud.multimode > 4) y1 += scale(ydim,8,200);
        }
        if (ud.screen_size >= 8) y2 = scale(ydim,200-34,200);
     }

     for(y=y1;y<y2;y+=128)
          for(x=0;x<xdim;x+=128)
                rotatesprite(x<<16,y<<16,65536L,0,dapicnum,8,0,8+16+64+128,0,y1,xdim-1,y2-1);

	 // FIX_00081: Screen border in menu
     if(ud.screen_size > 8 && (ps[myconnectindex].gm & MODE_GAME || ud.recstat == 2 )) // ud.recstat == 2 => playing demo
     {
          y = 0;
          if(ud.coop != 1)
          {
             if (ud.multimode > 1) y += 8;
             if (ud.multimode > 4) y += 8;
          }

          x1 = max(windowx1-4,0);
          y1 = max(windowy1-4,y);
          x2 = min(windowx2+4,xdim-1);
          y2 = min(windowy2+4,scale(ydim,200-34,200)-1);

          for(y=y1+4;y<y2-4;y+=64)
          {
                rotatesprite(x1<<16,y<<16,65536L,0,VIEWBORDER,0,0,8+16+64+128,x1,y1,x2,y2);
                rotatesprite((x2+1)<<16,(y+64)<<16,65536L,1024,VIEWBORDER,0,0,8+16+64+128,x1,y1,x2,y2);
          }

          for(x=x1+4;x<x2-4;x+=64)
          {
                rotatesprite((x+64)<<16,y1<<16,65536L,512,VIEWBORDER,0,0,8+16+64+128,x1,y1,x2,y2);
                rotatesprite(x<<16,(y2+1)<<16,65536L,1536,VIEWBORDER,0,0,8+16+64+128,x1,y1,x2,y2);
          }

          rotatesprite(x1<<16,y1<<16,65536L,0,VIEWBORDER+1,0,0,8+16+64+128,x1,y1,x2,y2);
          rotatesprite((x2+1)<<16,y1<<16,65536L,512,VIEWBORDER+1,0,0,8+16+64+128,x1,y1,x2,y2);
          rotatesprite((x2+1)<<16,(y2+1)<<16,65536L,1024,VIEWBORDER+1,0,0,8+16+64+128,x1,y1,x2,y2);
          rotatesprite(x1<<16,(y2+1)<<16,65536L,1536,VIEWBORDER+1,0,0,8+16+64+128,x1,y1,x2,y2);
     }
}
#if ENABLE_SE40  // next-hack: vanilla duke3d did not use this.

// Floor Over Floor

// If standing in sector with SE42
// then draw viewing to SE41 and raise all =hi SE43 cielings.

// If standing in sector with SE43
// then draw viewing to SE40 and lower all =hi SE42 floors.

// If standing in sector with SE44
// then draw viewing to SE40.

// If standing in sector with SE45
// then draw viewing to SE41.

#define FOFTILE 13
#define FOFTILEX 32
#define FOFTILEY 32
//

int32_t tempsectorz[MAXSECTORS];
/*int32_t*/ int16_t tempsectorpicnum[MAXSECTORS]; // why was it 32 bit ?
//short tempcursectnum;

static void SE40_Draw(int spnum,int32_t x,int32_t y,int32_t z,short a,short h,int32_t smoothratio)
{
 int i=0,j=0,k=0;
 int floor1=0,floor2=0,ok=0,fofmode=0;
 int32_t offx,offy;

 if(sprite[spnum].ang!=512) return;

 i = FOFTILE;    //Effect TILE
 if (!(gotpic[i>>3]&(1<<(i&7)))) return;
 gotpic[i>>3] &= ~(1<<(i&7));

 floor1=spnum;

 if(sprite[spnum].lotag==42) fofmode=40;
 if(sprite[spnum].lotag==43) fofmode=41;
 if(sprite[spnum].lotag==44) fofmode=40;
 if(sprite[spnum].lotag==45) fofmode=41;

// fofmode=sprite[spnum].lotag-2;

// sectnum=getSprite(j)->_sectnum;
// sectnum=cursectnum;
 ok++;

/*  recursive?
 for(j=0;j<MAXSPRITES;j++)
 {
  if(
     getSprite(j)->_sectnum==sectnum &&
     getSprite(j)->_picnum==1 &&
     getSprite(j)->_lotag==110
    ) { DrawFloorOverFloor(j); break;}
 }
*/

// if(ok==0) { Message("no fof",RED); return; }

 for(j=0;j<MAXSPRITES;j++)
 {
  if(
     getSprite(j)->_picnum==1 &&
     getSprite(j)->_lotag==fofmode &&
     getSprite(j)->_hitag==sprite[floor1].hitag
    ) { floor1=j; fofmode=getSprite(j)->_lotag; ok++; break;}
 }
// if(ok==1) { Message("no floor1",RED); return; }

 if(fofmode==40) k=41; else k=40;

 for(j=0;j<MAXSPRITES;j++)
 {
	  if(
		 getSprite(j)->_picnum==1 &&
		 getSprite(j)->_lotag==k &&
		 getSprite(j)->_hitag==sprite[floor1].hitag
		)
	  {
		  floor2=j;
		  ok++;
		  break;
	  }
 }

// if(ok==2) { Message("no floor2",RED); return; }

 for(j=0;j<MAXSPRITES;j++)  // raise ceiling or floor
 {
  if(getSprite(j)->_picnum==1 &&
     getSprite(j)->_lotag==k+2 &&
     getSprite(j)->_hitag==sprite[floor1].hitag
    )
    {
     if(k==40)
     {tempsectorz[getSprite(j)->_sectnum]=getSector(getSprite(j)->_sectnum)->floorz;
      setSector_floorz(getSprite(j)->_sectnum, getSector(getSprite(j)->_sectnum)->floorz + (((z-getSector(getSprite(j)->_sectnum)->floorz)/32768)+1)*32768);
      tempsectorpicnum[getSprite(j)->_sectnum]=getSector(getSprite(j)->_sectnum)->floorpicnum;
      setSector_floorpicnum(getSprite(j)->_sectnum, 13);
     }
     if(k==41)
     {tempsectorz[getSprite(j)->_sectnum]=getSector(getSprite(j)->_sectnum)->ceilingz;
      setSector_ceilingz(getSprite(j)->_sectnum, getSector(getSprite(j)->_sectnum)->ceilingz + (((z-getSector(getSprite(j)->_sectnum)->ceilingz)/32768)-1)*32768);
      tempsectorpicnum[getSprite(j)->_sectnum]=getSector(getSprite(j)->_sectnum)->ceilingpicnum;
      setSector_ceilingpicnum(getSprite(j)->_sectnum, 13);
     }
    }
 }

 i=floor1;
 offx=x-getSprite(i)->_x;
 offy=y-getSprite(i)->_y;
 i=floor2;
 drawrooms(offx+getSprite(i)->_x,offy+getSprite(i)->_y,z,a,h,getSprite(i)->_sectnum);
 animatesprites(x,y,a,smoothratio);
 drawmasks();

 for(j=0;j<MAXSPRITES;j++)  // restore ceiling or floor
 {
  if(getSprite(j)->_picnum==1 &&
     getSprite(j)->_lotag==k+2 &&
     getSprite(j)->_hitag==sprite[floor1].hitag
    )
    {
     if(k==40)
     {setSector_floorz(getSprite(j)->_sectnum, tempsectorz[getSprite(j)->_sectnum]);
      setSector_floorpicnum(getSprite(j)->_sectnum, tempsectorpicnum[getSprite(j)->_sectnum]);
     }
     if(k==41)
     {setSector_ceilingz(getSprite(j)->_sectnum, tempsectorz[getSprite(j)->_sectnum]);
      setSector_ceilingpicnum(getSprite(j)->_sectnum, tempsectorpicnum[getSprite(j)->_sectnum]);
     }
    }// end if
 }// end for

} // end SE40




static void se40code(int32_t x,int32_t y,int32_t z,int32_t a,int32_t h, int32_t smoothratio)
{
    int i;

    i = headspritestat[15];
    while(i >= 0)
    {
        switch(getSprite(i)->_lotag)
        {
//            case 40:
//            case 41:
//                SE40_Draw(i,x,y,a,smoothratio);
//                break;
            case 42:
            case 43:
            case 44:
            case 45:
                FIXME("SE40SHIT");
                if(ps[screenpeek].cursectnum == getSprite(i)->_sectnum)
                    SE40_Draw(i,x,y,z,a,h,smoothratio);
                break;
        }
        i = spr_add_data[i].nextspritestat;
    }
}
#endif
static int32_t oyrepeat=-1;
extern char *wallstempbuf;
#if LASTX_IN_STACK
extern int16_t *lastx;
#endif // LASTX_IN_STACK
#define ALIGN4(x) ( (x + 3) & ~3)
int  displayrooms(short snum,int32_t smoothratio)
{
    int32_t cposx,cposy,cposz,dst,j,fz,cz;
    short sect, cang, k, choriz;
    struct player_struct *p;
    int32_t tposx,tposy,i;
    short tang;
    int tiltedScreen = 0;
    p = &ps[snum];
    {
        uint8_t bigstack[ALIGN4(MAXWALLS_TEMP_BUFF) + ALIGN4(MAXWALLSB * (sizeof (uint16_t) + sizeof (pvWall_t))) + ALIGN4(MAXSPRITESONSCREEN * sizeof(tspritetype))];
        uint8_t *stackptr = bigstack ;
        wallstempbuf = (char*) stackptr;
        stackptr += ALIGN4(MAXWALLS_TEMP_BUFF);
    #if STACK_PVWALLS
        setStackPvWalls(stackptr);
        stackptr += ALIGN4(MAXWALLSB * (sizeof (uint16_t) + sizeof (pvWall_t)));
    #endif // STACK_PVWALLS
    #if STACK_TSPRITES
        tsprite = (tspritetype *) stackptr;
    #endif

        if(pub > 0)
        {
            if(ud.screen_size > 8) drawbackground();
            pub = 0;
        }

        if( ud.overhead_on == 2 || ud.show_help || p->cursectnum == -1)
        {
#if STACK_TSPRITES
            tsprite = NULL;
#endif
            return 0;
        }


        smoothratio = min(max(smoothratio,0),65536);

        visibility = p->visibility;

        if(ud.pause_on || ps[snum].on_crane > -1) smoothratio = 65536;

        sect = p->cursectnum;
        if(sect < 0 || sect >= MAXSECTORS)
        {
#if STACK_TSPRITES
            tsprite = NULL;
#endif
            return 0;
        }


        dointerpolations(smoothratio);

        animatecamsprite();

        if(ud.camerasprite >= 0)
        {
    //        spritetype *s;

    //        s = &sprite[ud.camerasprite];

            if(getSprite(ud.camerasprite)->_yvel < 0)
                setSprite_yvel(ud.camerasprite, -100);
            else if(getSprite(ud.camerasprite)->_yvel > 199)
                setSprite_yvel(ud.camerasprite, 300);

            cang = getHittype_tempang(ud.camerasprite)+mulscale16((int32_t)(((getSprite(ud.camerasprite)->_ang+1024-getHittype_tempang(ud.camerasprite))&2047)-1024),smoothratio);
    #if ENABLE_SE40
            se40code(getSprite(ud.camerasprite)->_x,getSprite(ud.camerasprite)->_y,getSprite(ud.camerasprite)->_z,cang,getSprite(ud.camerasprite)->_yvel,smoothratio);
    #endif
            drawrooms(getSprite(ud.camerasprite)->_x,getSprite(ud.camerasprite)->_y,getSprite(ud.camerasprite)->_z-(4<<8),cang,getSprite(ud.camerasprite)->_yvel,getSprite(ud.camerasprite)->_sectnum);
            animatesprites(getSprite(ud.camerasprite)->_x,getSprite(ud.camerasprite)->_y,cang,smoothratio);
            drawmasks();
        }
        else
        {
            i = divscale22(1,getSprite(p->i)->_yrepeat+28);
            if (i != oyrepeat)
            {
                oyrepeat = i;
                //printf("1: %d %d\n", oyrepeat,yxaspect);
                setaspect(oyrepeat,yxaspect);
                //printf("2: %d %d\n", oyrepeat,yxaspect);
            }
    #if NOT_YET_IMPLEMENTED
            if(screencapt)
            {

                tiles[MAXTILES-1].lock = 254;
                if (tiles[MAXTILES-1].data == NULL)
     /*code not implemented*/               allocache(&tiles[MAXTILES-1].data,100*160,&tiles[MAXTILES-1].lock);
                printf("screencapt");

    /*not impl*/            setviewtotile(MAXTILES-1,100L,160L);
            }
            else
    #endif
            if( ( ud.screen_tilting && p->rotscrnang ) || ud.detail==0 )
            {
                    if (ud.screen_tilting) tang = p->rotscrnang; else tang = 0;
    #if !MCU_DUKE
    #error
                    tiles[MAXTILES-2].lock = 255;

                    if (tiles[MAXTILES-2].data == NULL)
                    {
    /*code disabled*/                    allocache(&tiles[MAXTILES-2].data,320L*320L,&tiles[MAXTILES-2].lock);
                    }.
    #endif
                    if ((tang&1023) == 0)
                        setviewtotile(MAXTILES-2,200L>>(1-ud.detail),320L>>(1-ud.detail));
                    else
    #if BIG_TILT_TILE
                        setviewtotile(MAXTILES-2,320L>>(1-ud.detail),320L>>(1-ud.detail));
    #else
                        setviewtotile(MAXTILES-2,SCREEN_TILT_TILE_SIZE>>(1-ud.detail),SCREEN_TILT_TILE_SIZE>>(1-ud.detail));
    #endif
                    if ((tang&1023) == 512)
                    {     //Block off unscreen section of 90� tilted screen
                        j = ((320-60)>>(1-ud.detail));
    #if SET_STARTxMOST
    #error
                        for(i=(60>>(1-ud.detail))-1;i>=0;i--)
                        {
                            _startumost[i] = 1; _startumost[i+j] = 1;
                            _startdmost[i] = 0; _startdmost[i+j] = 0;
                        }
    #endif
                    }

                    i = (tang&511); if (i > 256) i = 512-i;
                    // next-hack: where do these number come from ? Simple 320:200 is 8:5.
                    // This is 8*cos(angle) + 5 * sin(angle)
                    i = sintable[i+512]*8 + sintable[i]*5L;
                    setaspect(i>>1,yxaspect);
              }

              if ( (snum == myconnectindex) && (numplayers > 1) )
                      {
                                    cposx = omyx+mulscale16((int32_t)(myx-omyx),smoothratio);
                                    cposy = omyy+mulscale16((int32_t)(myy-omyy),smoothratio);
                                    cposz = omyz+mulscale16((int32_t)(myz-omyz),smoothratio);
                                    cang = omyang+mulscale16((int32_t)(((myang+1024-omyang)&2047)-1024),smoothratio);
                                    choriz = omyhoriz+omyhorizoff+mulscale16((int32_t)(myhoriz+myhorizoff-omyhoriz-omyhorizoff),smoothratio);
                                    sect = mycursectnum;
                      }
                      else
                      {
                                    cposx = p->oposx+mulscale16((int32_t)(p->posx-p->oposx),smoothratio);
                                    cposy = p->oposy+mulscale16((int32_t)(p->posy-p->oposy),smoothratio);
                                    cposz = p->oposz+mulscale16((int32_t)(p->posz-p->oposz),smoothratio);
                                    cang = p->oang+mulscale16((int32_t)(((p->ang+1024-p->oang)&2047)-1024),smoothratio);
                                    choriz = p->ohoriz+p->ohorizoff+mulscale16((int32_t)(p->horiz+p->horizoff-p->ohoriz-p->ohorizoff),smoothratio);
                      }
                      cang += p->look_ang;

                      if (p->newowner >= 0)
                      {
                                    cang = p->ang+p->look_ang;
                                    choriz = p->horiz+p->horizoff;
                                    cposx = p->posx;
                                    cposy = p->posy;
                                    cposz = p->posz;
                                    sect = getSprite(p->newowner)->_sectnum;
                                    smoothratio = 65536L;
                      }

                      else if( p->over_shoulder_on == 0 )
                                    cposz += p->opyoff+mulscale16((int32_t)(p->pyoff-p->opyoff),smoothratio);
                      else view(p,&cposx,&cposy,&cposz,&sect,cang,choriz);

            cz = getHittype(p->i)->ceilingz;
            fz = getHittype(p->i)->floorz;

            if(earthquaketime > 0 && p->on_ground == 1)
            {
                cposz += 256-(((earthquaketime)&1)<<9);
                cang += (2-((earthquaketime)&2))<<2;
            }

            if(getSprite(p->i)->_pal == 1) cposz -= (18<<8);

            if(p->newowner >= 0)
                choriz = 100+getSprite(p->newowner)->_shade;
            else if(p->spritebridge == 0)
            {
                if( cposz < ( p->truecz + (4<<8) ) ) cposz = cz + (4<<8);
                else if( cposz > ( p->truefz - (4<<8) ) ) cposz = fz - (4<<8);
            }

            if (sect >= 0)
            {
                getzsofslope(sect,cposx,cposy,&cz,&fz);
                if (cposz < cz+(4<<8)) cposz = cz+(4<<8);
                if (cposz > fz-(4<<8)) cposz = fz-(4<<8);
            }

            if(choriz > 299) choriz = 299;
            else if(choriz < -99) choriz = -99;
    #if ENABLE_SE40
            se40code(cposx,cposy,cposz,cang,choriz,smoothratio);
    #endif
            //if ((gotpic[MIRROR>>3]&(1<<(MIRROR&7))) > 0)
            if (gotMirrorPic)
            {
                dst = 0x7fffffff; i = 0;
                for(k=0;k<mirrorcnt;k++)
                {
                    j = klabs(getWall(mirrorwall[k])->x-cposx);
                    j += klabs(getWall(mirrorwall[k])->y-cposy);
                    if (j < dst) dst = j, i = k;
                }

                if( getWall_overpicnum(mirrorwall[i]) == MIRROR )
                {
                    preparemirror(cposx,cposy,cposz,cang,choriz,mirrorwall[i],mirrorsector[i],&tposx,&tposy,&tang);

                    j = visibility;
                    visibility = (j>>1) + (j>>2);

                    drawrooms(tposx,tposy,cposz,tang,choriz,mirrorsector[i]+MAXSECTORS);

                    display_mirror = 1;
                    animatesprites(tposx,tposy,tang,smoothratio);
                    display_mirror = 0;

                    drawmasks();
                    completemirror();   //Reverse screen x-wise in this function
                    visibility = j;
                }
                //gotpic[MIRROR>>3] &= ~(1<<(MIRROR&7));
                gotMirrorPic = 0;
            }
    #if WIN32
        profileGetWallPerFrame = 0;
    #endif // WIN32
            drawrooms(cposx,cposy,cposz,cang,choriz,sect);
    #if WIN32 && PROFILE_GETWALLS
        printf("GetWallInDrawRooms: %d\r\n", profileGetWallPerFrame);
        profileGetWallPerFrame = 0;
    #endif // WIN32

            animatesprites(cposx,cposy,cang,smoothratio);
            drawmasks();
    #if CODE_DISABLED
            if(screencapt == 1)
            {
                setviewback();
                tiles[MAXTILES-1].lock = 1;
                screencapt = 0;
            }
            else
    #endif
            if( ( ud.screen_tilting && p->rotscrnang) || ud.detail==0 )
            {
                if (ud.screen_tilting) tang = p->rotscrnang; else tang = 0;
                setviewback();
                    // next-hack: why was this necessary?:            tiles[MAXTILES-2].animFlags &= 0xff0000ff;
                tiltedScreen = true;
    #if !MCU_DUKE
                tiles[MAXTILES-2].lock = 199;
    #endif
            }
        }
    }
    restoreinterpolations();

    if (totalclock < lastvisinc)
    {
        if (klabs(p->visibility-ud.const_visibility) > 8)
            p->visibility += (ud.const_visibility-p->visibility)>>2;
    }
    else p->visibility = ud.const_visibility;

#if STACK_TSPRITES
    tsprite = NULL;
#endif
    return tiltedScreen;
}
void tiltScreen(int snum)
{
    struct player_struct *p;
    int tang;;
    int i;
    if (numplayers > 1)
        pus = 1; // next-hack: fix for fragbar display.

    p = &ps[snum];

    int32_t buffer[SCREEN_TILT_TILE_SIZE * SCREEN_TILT_TILE_SIZE / 4];
    int32_t *s = (int32_t*)  tileData[MAXTILES - 2];
    for (int i = 0; i < SCREEN_TILT_TILE_SIZE * SCREEN_TILT_TILE_SIZE / 4; i++)
    {
        buffer[i] = *s++;
    }
    if (ud.screen_tilting) tang = p->rotscrnang; else tang = 0;
    i = (tang&511); if (i > 256) i = 512-i;
    i = (sintable[i+512]*8 + sintable[i]*5L) * ((int) ((float) SCREEN_WIDTH / SCREEN_TILT_TILE_SIZE + 0.5f));
    if ((1-ud.detail) == 0) i >>= 1;
    //
    rotateRAMsprite(160<<16,100<<16,i ,tang+512,(uint8_t*) buffer,0,0,4+2+64,windowx1,windowy1,windowx2,windowy2);
}



short LocateTheLocator(short n,short sn)
{
    short i;

    i = headspritestat[7];
    while(i >= 0)
    {
        if( (sn == -1 || sn == SECT) && n == SLT )
            return i;
        i = spr_add_data[i].nextspritestat;
    }
    return -1;
}
#if WIN32
short EGS2(short whatsect,int32_t s_x,int32_t s_y,int32_t s_z,short s_pn,int8_t s_s,int8_t s_xr,int8_t s_yr,short s_a,short s_ve,int32_t s_zv,short s_ow,int8_t s_ss, const char *funct, const char *file, int line)
{
    #define DEBUG_EGS 0
    #if DEBUG_EGS
      SYSTEMTIME st;
      GetLocalTime(&st);
    printf("Current time: %02d:%02d:%02d ", st.wHour, st.wMinute, st.wSecond);
    printf("EGS Function: %s, file %s, line %d, picnum %d\r\n", funct, file, line, s_pn);
    #endif
#else
short EGS(short whatsect,int32_t s_x,int32_t s_y,int32_t s_z,short s_pn,int8_t s_s,int8_t s_xr,int8_t s_yr,short s_a,short s_ve,int32_t s_zv,short s_ow,int8_t s_ss)
{
#endif // WIN32

    short i;
//    spritetype *s;

    i = insertsprite(whatsect,s_ss);
    // next-hack: remove one unncessary sprites
    if (i < 0)
    {
        freeUnnecessarySprites(-1, NO_HITTYPE_REQUIRED);
        i = insertsprite(whatsect,s_ss);
    }
    if( i < 0 )
        gameexit(" Too many sprites spawned. This may happen (for any duke port) if you have hacked the steroids trail in the *.con files. If so, delete your *.con files to use the internal ones and try again.");
#if !SPRITE_TRANSITION
    // next-hack: initialize dynamic sprites
    if (!getRamSpriteIdx(i))
    {
        allocateNewRamSprite(i);
    }
    // now for sure it is in RAM
    spritetype *rs = (spritetype *) getSprite(i);
    memset(rs, 0, sizeof(spritetype));
    // now we must restore stat and sect.
    setSprite_statnum(i, s_ss);
    setSprite_sectnum(i, whatsect);
#endif
#if USE_RAM_HITTYPE
    // now we need to allocate a hittype for this sprite. Let's free if it has one
    freeHittype(i);
    int hittypenumber = allocateNewHittype(i);
    memset(&hittypeRamPtr[hittypenumber], 0 , sizeof(hittypeRamPtr[hittypenumber]));
#endif
    setHittype_bposx(i, s_x);
    setHittype_bposy(i, s_y);
    setHittype_bposz(i, s_z);


    setSprite_x(i, s_x);
    setSprite_y(i, s_y);
    setSprite_z(i, s_z);
    setSprite_cstat(i, 0);
    setSprite_picnum(i, s_pn);
    setSprite_shade(i, s_s);
    setSprite_xrepeat(i, s_xr);
    setSprite_yrepeat(i, s_yr);
    setSprite_pal(i, 0);

    setSprite_ang(i, s_a);
    setSprite_xvel(i, s_ve);
    setSprite_zvel(i, s_zv);
    setSprite_owner(i, s_ow);
#if SPRITETYPE_HAS_OFFSETS
    setSprite_xoffset(i, 0);
    setSprite_yoffset(i, 0);
#else
    sprite_offsets[i].xoffset = 0;
    sprite_offsets[i].yoffset = 0;
#endif
    setSprite_yvel(i, 0);
#if SPRITE_HAS_CLIPDIST
    getSprite(i)->_clipdist = 0;
#else
    spr_add_data[i].clipdist = 0;
#endif
    setSprite_pal(i, 0);
    setSprite_lotag(i, 0);

    setHittype_picnum(i, getSprite(s_ow)->_picnum);

    setHittype_lastvx(i, 0);
    setHittype_lastvy(i, 0);
    setHittype_tempang(i, 0);
    setHittype_timetosleep(i, 0);
    if (getHittype_ang(i) == 0)  // also ang was 0? free.
        freeHittype_otherdata(i);
#if MINIMIZE_ACTORSTAYPUT
    getHittype(i)->actorstayput = MAXSECTORS; //next-hack: was -1
 #else
    setHittype_actorstayput(i, -1); //next-hack: was -1
 #endif
    setHittype_extra(i, -1);
    setHittype_owner(i, s_ow);
    setHittype_cgg(i, 0);
    setHittype_movflag(i, 0);
    setHittype_dispicnum(i, 0);
    setHittype_floorz(i, getHittype(s_ow)->floorz);
    setHittype_ceilingz(i, getHittype(s_ow)->ceilingz);

    setHittypeTemp(i, 0, 0);
    setHittypeTemp(i, 2, 0);
    setHittypeTemp(i, 3, 0);
    setHittypeTemp(i, 5, 0);
#if USE_ACTORSCRPTR
    if( actorscrptr[s_pn] )
    {
        getSprite(i)->_extra = *actorscrptr[s_pn];
        getHittypeTemp(i, 4) = *(actorscrptr[s_pn]+1);
        getHittypeTemp(i, 1)  = *(actorscrptr[s_pn]+2);
        getSprite(i)->_hitag = *(actorscrptr[s_pn]+3);
    }
#else
    if( getscriptptr(actorscridx[s_pn]) )
    {
        setSprite_extra(i, *getscriptptr(actorscridx[s_pn]));
        setHittypeTemp(i, 4, *(getscriptptr(actorscridx[s_pn]) + 1));
        setHittypeTemp(i, 1, *(getscriptptr(actorscridx[s_pn]) + 2));
        setSprite_hitag(i, *(getscriptptr(actorscridx[s_pn]) + 3));
    }
#endif
    else
    {
        setHittypeTemp(i, 1, 0);
        setHittypeTemp(i, 4, 0);
        setSprite_extra(i, 0);
        setSprite_hitag(i, 0);
    }

    if (show2dsector[SECT>>3]&(1<<(SECT&7)))
        show2dsprite[i>>3] |= (1<<(i&7));
    else
        show2dsprite[i>>3] &= ~(1<<(i&7));
/*
    if(getSprite(i)->_sectnum < 0)
    {
        setSprite_xy_repeats(i, 0, 0);
        changespritestat(i,5);
    }
*/
    return(i);
}

uint8_t  wallswitchcheck(short i)
{
    switch(PN)
    {
        case HANDPRINTSWITCH:
        case HANDPRINTSWITCH+1:
        case ALIENSWITCH:
        case ALIENSWITCH+1:
        case MULTISWITCH:
        case MULTISWITCH+1:
        case MULTISWITCH+2:
        case MULTISWITCH+3:
        case ACCESSSWITCH:
        case ACCESSSWITCH2:
        case PULLSWITCH:
        case PULLSWITCH+1:
        case HANDSWITCH:
        case HANDSWITCH+1:
        case SLOTDOOR:
        case SLOTDOOR+1:
        case LIGHTSWITCH:
        case LIGHTSWITCH+1:
        case SPACELIGHTSWITCH:
        case SPACELIGHTSWITCH+1:
        case SPACEDOORSWITCH:
        case SPACEDOORSWITCH+1:
        case FRANKENSTINESWITCH:
        case FRANKENSTINESWITCH+1:
        case LIGHTSWITCH2:
        case LIGHTSWITCH2+1:
        case POWERSWITCH1:
        case POWERSWITCH1+1:
        case LOCKSWITCH1:
        case LOCKSWITCH1+1:
        case POWERSWITCH2:
        case POWERSWITCH2+1:
        case DIPSWITCH:
        case DIPSWITCH+1:
        case DIPSWITCH2:
        case DIPSWITCH2+1:
        case TECHSWITCH:
        case TECHSWITCH+1:
        case DIPSWITCH3:
        case DIPSWITCH3+1:
            return 1;
    }
    return 0;
}


int32_t tempwallptr;
#if WIN32
short spawn2( short j, short pn, char *file, const char *function, int line )
#else
short spawn( short j, short pn )
#endif
{
    #if WIN32
    static int bonuses, possibleStatic;
    #endif // WIN32
    short i, s, startwall, endwall, sect, clostest;
    int32_t x, y, d;
//    spritetype *sp;
    char text[512];

    if(j >= 0)
    {
        i = EGS(getSprite(j)->_sectnum,getSprite(j)->_x,getSprite(j)->_y,getSprite(j)->_z
            ,pn,0,0,0,0,0,0,j,0);
        setHittype_picnum(i, getSprite(j)->_picnum);
    }
    else
    { // if i < 0, then it is spawn from prelevel.
        i = pn;

        setHittype_picnum(i, PN);
        setHittype_timetosleep(i, 0);
        setHittype_extra(i, -1);

        setHittype_bposx(i, SX);
        setHittype_bposy(i, SY);
        setHittype_bposz(i, SZ);

        setSprite_owner(i, i); // OW = setHittype_owner(i, i);  <---- BLEAH
        setHittype_owner(i, i);
        setHittype_cgg(i, 0);
        setHittype_movflag(i, 0);
        setHittype_tempang(i, 0);
        setHittype_dispicnum(i, 0);
        setHittype_floorz(i, getSector(SECT)->floorz);
        setHittype_ceilingz(i, getSector(SECT)->ceilingz);

        setHittype_lastvx(i, 0);
        setHittype_lastvy(i, 0);
#if MINIMIZE_ACTORSTAYPUT
         setHittype_actorstayput(i, MAXSECTORS); //next-hack: was -1
#else
        setHittype_actorstayput(i, -1);
#endif
// #warning HERE WE CAN DEALLOCATE THIS. DONE
        setHittypeTemp(i, 0, 0);
        setHittypeTemp(i, 1, 0);
        setHittypeTemp(i, 2, 0);
        setHittypeTemp(i, 3, 0);
        setHittypeTemp(i, 4, 0);
        setHittypeTemp(i, 5, 0);
#if USE_SEPARATE_HITTYPE_TEMPDATA
        freeHittype_tempdata0(i);
        freeHittype_tempdata1(i);
        freeHittype_tempdata2(i);
        freeHittype_tempdata3(i);
        freeHittype_tempdata45(i);
#else
        freeHittype_tempdata(i);

#endif // USE_SEPARATE_HITTYPE_TEMPDATA

        if( PN != SPEAKER && PN != LETTER && PN != DUCK && PN != TARGET && PN != TRIPBOMB && PN != VIEWSCREEN && PN != VIEWSCREEN2 && (CS&48) )
            if( !(PN >= CRACK1 && PN <= CRACK4) )
        {
            if(SS == 127) return i;
            if( wallswitchcheck(i) == 1 && (CS&16) )
            {
                if( PN != ACCESSSWITCH && PN != ACCESSSWITCH2 && getSprite_pal(i))
                {
                    if( (ud.multimode < 2) || (ud.multimode > 1 && ud.coop==1) )
                    {
                        setSprite_xrepeat(i, 0);
                        setSprite_yrepeat(i, 0);
                        setSprite_lotag(i, 0);
                        setSprite_hitag(i, 0);
                        setSprite_cstat(i, 0);
                        return i;
                    }
                }
                setSprite_cstat(i, getSprite_cstat(i) | 257); // CS |=  257; CRAP CODE!
                if( getSprite_pal(i) && PN != ACCESSSWITCH && PN != ACCESSSWITCH2)
                    setSprite_pal(i, 0);
                return i;
            }

            if( SHT )
            {
                changespritestat(i,12);
                setSprite_cstat(i, getSprite_cstat(i) | 257); // CS |=  257; CRAP CODE!
                setSprite_extra(i, impact_damage);//SH = impact_damage; // again!
                return i;
            }
        }

        s = PN;

        if( CS&1 )
            setSprite_cstat(i, getSprite_cstat(i) | 256); //CS |= 256; damn you
#if USE_ACTORSCRPTR
        if( actorscrptr[s] )
        {
            SH = *(actorscrptr[s]);
            getHittypeTemp(i, 4) = *(actorscrptr[s]+1);
            getHittypeTemp(i, 1)  = *(actorscrptr[s]+2);
            if( *(actorscrptr[s]+3) && SHT == 0 )
                SHT = *(actorscrptr[s]+3);
        }
#else
        if( getscriptptr(actorscridx[s]) )
        {
            setSprite_extra(i, *(getscriptptr(actorscridx[s])));//SH = *(getscriptptr(actorscridx[s]));
            setHittypeTemp(i, 4, *(getscriptptr(actorscridx[s]) + 1));
            setHittypeTemp(i, 1, *(getscriptptr(actorscridx[s]) + 2));
            if( *(getscriptptr(actorscridx[s]) + 3) && SHT == 0 )
                setSprite_hitag(i, *(getscriptptr(actorscridx[s]) + 3));//SHT = *(getscriptptr(actorscridx[s]) + 3);
        }

#endif
        else
        {
            setHittypeTemp(i, 1, 0);
            setHittypeTemp(i, 4, 0);
        }
    }

    //sp = &sprite[i];
    sprite_additional_data *sp_d = &spr_add_data[i];
    sect = getSprite(i)->_sectnum;

    switch(getSprite(i)->_picnum)
    {
            default:
#if USE_ACTORSCRPTR
                if( actorscrptr[getSprite(i)->_picnum] )
#else
                if( getscriptptr(actorscridx[getSprite(i)->_picnum]) )
#endif
                {
                    if( j == -1 && getSprite(i)->_lotag > ud.player_skill )
                    {
                        setSprite_xy_repeats(i, 0, 0);
                        changespritestat(i,5);
                        break;
                    }
                        //  Init the size
                    if(getSprite(i)->_xrepeat == 0 || getSprite(i)->_yrepeat == 0)
                        setSprite_xy_repeats(i, 1, 1);

                    if( getActorType(getSprite(i)->_picnum) & 3)
                    {
                        if( ud.monsters_off == 1 )
                        {
                            setSprite_xy_repeats(i, 0, 0);
                            changespritestat(i,5);
                            break;
                        }

                        makeitfall(i);

                        if( getActorType(getSprite(i)->_picnum) & 2)
#if MINIMIZE_ACTORSTAYPUT
                            setHittype_actorstayput(i, getSprite(i)->_sectnum >= 0 ? getSprite(i)->_sectnum : MAXSECTORS); // next-hack: added to make sure it is not -1
#else
                            setHittype_actorstayput(i, getSprite(i)->_sectnum);
#endif
                        ps[myconnectindex].max_actors_killed++;
                        sp_d->clipdist = 80;
                        if(j >= 0)
                        {
                            if(getSprite(j)->_picnum == RESPAWN)
                            {
                                setSprite_pal(i, getSprite(j)->_pal);
                                setHittype_tempang(i, getSprite_pal(i));
                            }
                            changespritestat(i,1);
                        }
                        else changespritestat(i,2);
                    }
                    else
                    {
                        sp_d->clipdist = 40;
                        setSprite_owner(i, i);
                        changespritestat(i,1);
                    }

                    setHittype_timetosleep(i, 0);

                    if(j >= 0)
                        setSprite_ang(i, getSprite(j)->_ang);
                }
                break;
            case FOF:
                setSprite_xy_repeats(i, 0, 0);
                changespritestat(i,5);
                break;
            case WATERSPLASH2:
                if(j >= 0)
                {
                    setsprite(i,getSprite(j)->_x,getSprite(j)->_y,getSprite(j)->_z);
                    int rv = 8+(TRAND&7);
                    setSprite_xy_repeats(i, rv, rv);
                }
                else
                {
                    int rv =  16+(TRAND&15);
                    setSprite_xy_repeats(i, rv, rv);
                }
                setSprite_shade(i, -16);
                setSprite_cstat(i, getSprite(i)->_cstat | 128);
                if(j >= 0)
                {
                    if(getSector(getSprite(j)->_sectnum)->lotag == 2)
                    {
                        setSprite_z(i, getceilzofslope(SECT,SX,SY)+(16<<8));
                        setSprite_cstat(i, getSprite(i)->_cstat | 8);
                    }
                    else if( getSector(getSprite(j)->_sectnum)->lotag == 1)
                        setSprite_z(i, getflorzofslope(SECT,SX,SY));
                }

                if(getSector(sect)->floorpicnum == FLOORSLIME ||
                    getSector(sect)->ceilingpicnum == FLOORSLIME)
                        setSprite_pal(i, 7);
            case NEON1:
            case NEON2:
            case NEON3:
            case NEON4:
            case NEON5:
            case NEON6:
            case DOMELITE:
                if(getSprite(i)->_picnum != WATERSPLASH2)
                    setSprite_cstat(i, getSprite(i)->_cstat | 257);
            case NUKEBUTTON:
                if(getSprite(i)->_picnum == DOMELITE)
                    setSprite_cstat(i, getSprite(i)->_cstat | 257);
            case JIBS1:
            case JIBS2:
            case JIBS3:
            case JIBS4:
            case JIBS5:
            case JIBS6:
            case HEADJIB1:
            case ARMJIB1:
            case LEGJIB1:
            case LIZMANHEAD1:
            case LIZMANARM1:
            case LIZMANLEG1:
            case DUKETORSO:
            case DUKEGUN:
            case DUKELEG:
                changespritestat(i,5);
                break;
            case TONGUE:
                if(j >= 0)
                    setSprite_ang(i, getSprite(j)->_ang);
                setSprite_z(i, getSprite(i)->_z - (38<<8));
                setSprite_zvel(i, 256-(TRAND&511));
                setSprite_xvel(i, 64-(TRAND&127));
                changespritestat(i,4);
                break;
            case NATURALLIGHTNING:
                setSprite_cstat(i, getSprite(i)->_cstat & ~257);
                setSprite_cstat(i, getSprite(i)->_cstat | SPRITE_INVISIBLE);
                break;
            case TRANSPORTERSTAR:
            case TRANSPORTERBEAM:
                if(j == -1) break;
                if(getSprite(i)->_picnum == TRANSPORTERBEAM)
                {
                    setSprite_xy_repeats(i, 31, 1);
                    setSprite_z(i, getSector(getSprite(j)->_sectnum)->floorz-(40<<8));
                }
                else
                {
                    if(getSprite(j)->_statnum == 4)
                    {
                        setSprite_xy_repeats(i, 8, 8);
                    }
                    else
                    {
                        setSprite_xy_repeats(i, 48, 64);
                        if(getSprite(j)->_statnum == 10 || badguy(getSprite(j)) )
                            setSprite_z(i, getSprite(i)->_z - (32<<8));
                    }
                }

                setSprite_shade(i, -127);
                setSprite_cstat(i, 128|2);
                setSprite_ang(i, getSprite(j)->_ang);

                setSprite_xvel(i, 128);
                changespritestat(i,5);
                ssp(i,CLIPMASK0);
                setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                break;

            case FRAMEEFFECT1:
			case FRAMEEFFECT1_13CON:
				if(j >= 0)
                {
                    setSprite_xy_repeats(i, getSprite_xrepeat(j), getSprite_yrepeat(j));
                    setHittypeTemp(i, 1, getSprite(j)->_picnum);
                }
                else
                    setSprite_xy_repeats(i, 0, 0);

                changespritestat(i,5);

                break;

            case LASERLINE:
                setSprite_xy_repeats(i, 32, 6);

                if(lasermode == 1)
                    setSprite_cstat(i, 16 + 2);
                else if(lasermode == 0 || lasermode == 2)
                    setSprite_cstat(i, 16);
                else
                {
                    setSprite_xy_repeats(i, 0, 0);
                }

                if(j >= 0)
                    setSprite_ang(i, getHittypeTemp(j, 5)+512);
                changespritestat(i,5);
                break;

            case FORCESPHERE:
                if(j == -1 )
                {
                    setSprite_cstat(i, SPRITE_INVISIBLE);
                    changespritestat(i,2);
                }
                else
                {
                    setSprite_xy_repeats(i, 1, 1);
                    changespritestat(i,5);
                }
                break;

            case BLOOD:
                setSprite_xy_repeats(i, 16, 16);
               setSprite_z(i, getSprite(i)->_z - (26<<8));
               if( j >= 0 && getSprite(j)->_pal == 6 )
                   setSprite_pal(i, 6);
               changespritestat(i,5);
               break;
            case BLOODPOOL:
            case PUKE:
                {
                    short s1;
                    s1 = getSprite(i)->_sectnum;

                    updatesector(getSprite(i)->_x+108,getSprite(i)->_y+108,&s1);
                    if(s1 >= 0 && getSector(s1)->floorz == getSector(getSprite(i)->_sectnum)->floorz)
                    {
                        updatesector(getSprite(i)->_x-108,getSprite(i)->_y-108,&s1);
                        if(s1 >= 0 && getSector(s1)->floorz == getSector(getSprite(i)->_sectnum)->floorz)
                        {
                            updatesector(getSprite(i)->_x+108,getSprite(i)->_y-108,&s1);
                            if(s1 >= 0 && getSector(s1)->floorz == getSector(getSprite(i)->_sectnum)->floorz)
                            {
                                updatesector(getSprite(i)->_x-108,getSprite(i)->_y+108,&s1);
                                if(s1 >= 0 && getSector(s1)->floorz != getSector(getSprite(i)->_sectnum)->floorz)
                                {
                                   setSprite_xy_repeats(i, 0, 0);
                                    changespritestat(i,5);
                                    break;
                                }
                            }
                            else { setSprite_xy_repeats(i, 0, 0);changespritestat(i,5);break;}
                        }
                        else { setSprite_xy_repeats(i, 0, 0);;changespritestat(i,5);break;}
                    }
                    else { setSprite_xy_repeats(i, 0, 0);;changespritestat(i,5);break;}
                }

                if( getSector(SECT)->lotag == 1 )
                {
                    changespritestat(i,5);
                    break;
                }

                if(j >= 0 && getSprite(i)->_picnum != PUKE)
                {
                    if( getSprite(j)->_pal == 1)
                        setSprite_pal(i, 1);
                    else if( getSprite(j)->_pal != 6 && getSprite(j)->_picnum != NUKEBARREL && getSprite(j)->_picnum != TIRE )
                    {
                        if(getSprite(j)->_picnum == FECES)
                            setSprite_pal(i, 7); // Brown
                        else
                            setSprite_pal(i, 2); // Red
                    }
                    else
                        setSprite_pal(i, 0);  // green

                    if(getSprite(j)->_picnum == TIRE)
                        setSprite_shade(i, 127);
                }
                setSprite_cstat(i, getSprite(i)->_cstat | 32);
            case FECES:
                if( j >= 0)
                    setSprite_xy_repeats(i, 1, 1);
                changespritestat(i,5);
                break;

            case BLOODSPLAT1:
            case BLOODSPLAT2:
            case BLOODSPLAT3:
            case BLOODSPLAT4:
                setSprite_cstat(i, getSprite(i)->_cstat | 16);
                setSprite_xy_repeats(i, 7+(TRAND&7), 7+(TRAND&7));
                setSprite_z(i, getSprite(i)->_z - (16<<8));
                if(j >= 0 && getSprite(j)->_pal == 6)
                    setSprite_pal(i, 6);
                insertspriteq(i);
                changespritestat(i,5);
                break;

            case TRIPBOMB:
                if( getSprite(i)->_lotag > ud.player_skill )
                {
                    setSprite_xy_repeats(i, 0, 0);
                    changespritestat(i,5);
                    break;
                }
                setSprite_xy_repeats(i, 4, 5);

                setSprite_owner(i, i);
                setSprite_hitag(i, i);

                setSprite_xvel(i, 16);
                ssp(i,CLIPMASK0);
                setHittypeTemp(i, 0, 17);
                setHittypeTemp(i, 2, 0);
                setHittypeTemp(i, 5, getSprite(i)->_ang);

            case SPACEMARINE:
                if(getSprite(i)->_picnum == SPACEMARINE)
                {
                    setSprite_extra(i, 20);
                    setSprite_cstat(i, getSprite(i)->_cstat | 257);
                }
                changespritestat(i,2);
                break;

            case HYDRENT:
            case PANNEL1:
            case PANNEL2:
            case SATELITE:
            case FUELPOD:
            case SOLARPANNEL:
            case ANTENNA:
            case GRATE1:
            case CHAIR1:
            case CHAIR2:
            case CHAIR3:
            case BOTTLE1:
            case BOTTLE2:
            case BOTTLE3:
            case BOTTLE4:
            case BOTTLE5:
            case BOTTLE6:
            case BOTTLE7:
            case BOTTLE8:
            case BOTTLE10:
            case BOTTLE11:
            case BOTTLE12:
            case BOTTLE13:
            case BOTTLE14:
            case BOTTLE15:
            case BOTTLE16:
            case BOTTLE17:
            case BOTTLE18:
            case BOTTLE19:
            case OCEANSPRITE1:
            case OCEANSPRITE2:
            case OCEANSPRITE3:
            case OCEANSPRITE5:
            case MONK:
            case INDY:
            case LUKE:
            case JURYGUY:
            case SCALE:
            case VACUUM:
            case FANSPRITE:
            case CACTUS:
            case CACTUSBROKE:
            case HANGLIGHT:
            case FETUS:
            case FETUSBROKE:
            case CAMERALIGHT:
            case MOVIECAMERA:
            case IVUNIT:
            case POT1:
            case POT2:
            case POT3:
            case TRIPODCAMERA:
            case SUSHIPLATE1:
            case SUSHIPLATE2:
            case SUSHIPLATE3:
            case SUSHIPLATE4:
            case SUSHIPLATE5:
            case WAITTOBESEATED:
            case VASE:
            case PIPE1:
            case PIPE2:
            case PIPE3:
            case PIPE4:
            case PIPE5:
            case PIPE6:
#if WIN32
            possibleStatic++;
            printf("Possible static: %d\r\n", possibleStatic);
#endif // WIN32
                sp_d->clipdist = 32;
                setSprite_cstat(i, getSprite(i)->_cstat | 257);
            case OCEANSPRITE4:
                changespritestat(i,0);
                break;
            case FEMMAG1:
            case FEMMAG2:
                setSprite_cstat(i, getSprite(i)->_cstat & ~257);
                changespritestat(i,0);
                break;
            case DUKETAG:
            case SIGN1:
            case SIGN2:
                if(ud.multimode < 2 && getSprite(i)->_pal)
                {
                    setSprite_xy_repeats(i, 0, 0);
                    changespritestat(i,5);
                }
                else setSprite_pal(i, 0);
                break;
            case MASKWALL1:
            case MASKWALL2:
            case MASKWALL3:
            case MASKWALL4:
            case MASKWALL5:
            case MASKWALL6:
            case MASKWALL7:
            case MASKWALL8:
            case MASKWALL9:
            case MASKWALL10:
            case MASKWALL11:
            case MASKWALL12:
            case MASKWALL13:
            case MASKWALL14:
            case MASKWALL15:
                j = getSprite(i)->_cstat&60;
                setSprite_cstat(i, j | 1);
                changespritestat(i,0);
#if WIN32
            possibleStatic++;
            printf("Possible static: %d\r\n", possibleStatic);
#endif // WIN32

                break;
            case FOOTPRINTS:
            case FOOTPRINTS2:
            case FOOTPRINTS3:
            case FOOTPRINTS4:
                if(j >= 0)
                {
                    short s1;
                    s1 = getSprite(i)->_sectnum;

                    updatesector(getSprite(i)->_x+84,getSprite(i)->_y+84,&s1);
                    if(s1 >= 0 && getSector(s1)->floorz == getSector(getSprite(i)->_sectnum)->floorz)
                    {
                        updatesector(getSprite(i)->_x-84,getSprite(i)->_y-84,&s1);
                        if(s1 >= 0 && getSector(s1)->floorz == getSector(getSprite(i)->_sectnum)->floorz)
                        {
                            updatesector(getSprite(i)->_x+84,getSprite(i)->_y-84,&s1);
                            if(s1 >= 0 && getSector(s1)->floorz == getSector(getSprite(i)->_sectnum)->floorz)
                            {
                                updatesector(getSprite(i)->_x-84,getSprite(i)->_y+84,&s1);
                                if(s1 >= 0 && getSector(s1)->floorz != getSector(getSprite(i)->_sectnum)->floorz)
                                { setSprite_xy_repeats(i, 0, 0);changespritestat(i,5);break;}
                            }
                            else { setSprite_xy_repeats(i, 0, 0);break;}
                        }
                        else { setSprite_xy_repeats(i, 0, 0);break;}
                    }
                    else { setSprite_xy_repeats(i, 0, 0);break;}

                    setSprite_cstat(i, 32+((ps[getSprite(j)->_yvel].footprintcount&1)<<2));
                    setSprite_ang(i, getSprite(j)->_ang);
                }

                setSprite_z(i, getSector(sect)->floorz);
                if(getSector(sect)->lotag != 1 && getSector(sect)->lotag != 2)
                    setSprite_xy_repeats(i, 32, 32);

                insertspriteq(i);
                changespritestat(i,5);
                break;

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
            case TOUGHGAL:
                setSprite_yvel(i, getSprite(i)->_hitag);
                setSprite_hitag(i, -1);
                if(getSprite(i)->_picnum == PODFEM1)
                    setSprite_extra(i, getSprite(i)->_extra << 1);
            case BLOODYPOLE:

            case QUEBALL:
            case STRIPEBALL:

                if(getSprite(i)->_picnum == QUEBALL || getSprite(i)->_picnum == STRIPEBALL)
                {
                    setSprite_cstat(i, 256);
                    sp_d->clipdist = 8;
                }
                else
                {
                    setSprite_cstat(i, getSprite(i)->_cstat | 257);
                    sp_d->clipdist = 32;
                }

                changespritestat(i,2);
                break;

            case DUKELYINGDEAD:
                if(j >= 0 && getSprite(j)->_picnum == APLAYER)
                {
                    setSprite_xy_repeats(i, getSprite_xrepeat(j), getSprite_yrepeat(j));
                    setSprite_shade(i, getSprite(j)->_shade);
                    setSprite_pal(i, ps[getSprite(j)->_yvel].palookup);
                }
            case DUKECAR:
            case HELECOPT:
//                if(getSprite(i)->_picnum == HELECOPT || getSprite(i)->_picnum == DUKECAR) getSprite(i)->_xvel = 1024;
                setSprite_cstat(i, 0);
                setSprite_extra(i, 1);
                setSprite_xvel(i, 292);
                setSprite_zvel(i, 360);
            case RESPAWNMARKERRED:
            case BLIMP:

                if(getSprite(i)->_picnum == RESPAWNMARKERRED)
                {
                    setSprite_xy_repeats(i, 24, 24);
                    if(j >= 0)
                        setSprite_z(i, getHittype(j)->floorz); // -(1<<4);
                }
                else
                {
                    setSprite_cstat(i, getSprite(i)->_cstat | 257);
                    sp_d->clipdist = 128;
                }
            case MIKE:
                if(getSprite(i)->_picnum == MIKE)
                    setSprite_yvel(i, getSprite(i)->_hitag);
            case WEATHERWARN:
                changespritestat(i,1);
                break;

            case SPOTLITE:
                setHittypeTemp(i, 0, getSprite(i)->_x);
                setHittypeTemp(i, 1, getSprite(i)->_y);
                break;
            case BULLETHOLE:
                setSprite_xy_repeats(i, 3, 3);
                setSprite_cstat(i, 16+(TRAND&12));
                insertspriteq(i);
            case MONEY:
            case MAIL:
            case PAPER:
                if( getSprite(i)->_picnum == MONEY || getSprite(i)->_picnum == MAIL || getSprite(i)->_picnum == PAPER )
                {
                    setHittypeTemp(i, 0, TRAND&2047);
                    setSprite_cstat(i, TRAND&12);
                    setSprite_xy_repeats(i, 8, 8);
                    setSprite_ang(i, TRAND&2047);
                }
                changespritestat(i,5);
                break;

            case VIEWSCREEN:
            case VIEWSCREEN2:
                setSprite_owner(i, i);
                setSprite_lotag(i, 1);
                setSprite_extra(i, 1);
                changespritestat(i,6);
                break;

            case SHELL: //From the player
            case SHOTGUNSHELL:
                if( j >= 0 )
                {
                    short snum,a;

                    if(getSprite(j)->_picnum == APLAYER)
                    {
                        snum = getSprite(j)->_yvel;
                        a = ps[snum].ang-(TRAND&63)+8;  //Fine tune

                        setHittypeTemp(i, 0, TRAND&1);
                        if(getSprite(i)->_picnum == SHOTGUNSHELL)
                            setSprite_z(i, (6<<8)+ps[snum].pyoff+ps[snum].posz-((ps[snum].horizoff+ps[snum].horiz-100)<<4));
                        else
                            setSprite_z(i, (3<<8)+ps[snum].pyoff+ps[snum].posz-((ps[snum].horizoff+ps[snum].horiz-100)<<4));
                        setSprite_zvel(i, -(TRAND&255));
                    }
                    else
                    {
                        a = getSprite(i)->_ang;
                        setSprite_z(i, getSprite(j)->_z-PHEIGHT+(3<<8));
                    }

                    setSprite_x(i, getSprite(j)->_x+(sintable[(a+512)&2047]>>7));
                    setSprite_y(i, getSprite(j)->_y+(sintable[a&2047]>>7));

                    setSprite_shade(i, -8);

                    setSprite_ang(i, a-512);
                    setSprite_xvel(i, 20);

					//  do not try to make it 0 when ud.hideweapon Will make OOS when shooting in water
					setSprite_xy_repeats(i, 4, 4);

                    changespritestat(i,5);
                }
                break;

            case RESPAWN:
                setSprite_extra(i, 66-13);
            case MUSICANDSFX:
                if( ud.multimode < 2 && getSprite(i)->_pal == 1)
                {
                    setSprite_xy_repeats(i, 0, 0);
                    changespritestat(i,5);
                    break;
                }
                setSprite_cstat(i, (short)SPRITE_INVISIBLE);
                changespritestat(i,11);
                break;

            case EXPLOSION2:
            case EXPLOSION2BOT:
            case BURNING:
            case BURNING2:
            case SMALLSMOKE:
            case SHRINKEREXPLOSION:
            case COOLEXPLOSION1:

                if(j >= 0)
                {
                    setSprite_ang(i, getSprite(j)->_ang);
                    setSprite_shade(i, -64);
                    setSprite_cstat(i, 128|(TRAND&4));
                }

                if(getSprite(i)->_picnum == EXPLOSION2 || getSprite(i)->_picnum == EXPLOSION2BOT)
                {
                    setSprite_xy_repeats(i, 48, 48);
                    setSprite_shade(i, -127);
                    setSprite_cstat(i, getSprite(i)->_cstat | 128);
                }
                else if(getSprite(i)->_picnum == SHRINKEREXPLOSION )
                {
                    setSprite_xy_repeats(i, 32, 32);
                }
                else if( getSprite(i)->_picnum == SMALLSMOKE )
                {
                    // 64 "money"
                    setSprite_xy_repeats(i, 24, 24);
                }
                else if(getSprite(i)->_picnum == BURNING || getSprite(i)->_picnum == BURNING2)
                {
                    setSprite_xy_repeats(i, 4, 4);
                }

                if(j >= 0)
                {
                    x = getflorzofslope(getSprite(i)->_sectnum,getSprite(i)->_x,getSprite(i)->_y);
                    if(getSprite(i)->_z > x-(12<<8) )
                        setSprite_z(i, x-(12<<8));
                }

                changespritestat(i,5);

                break;

            case PLAYERONWATER:
                if(j >= 0)
                {
                    setSprite_xy_repeats(i, getSprite_xrepeat(j), getSprite_yrepeat(j));
                    setSprite_zvel(i, 128);
                    if(getSector(getSprite(i)->_sectnum)->lotag != 2)
                        setSprite_cstat(i, getSprite(i)->_cstat | SPRITE_INVISIBLE);
                }
                changespritestat(i,13);
                break;

            case APLAYER:
                setSprite_xy_repeats(i, 0, 0);
                j = ud.coop;
                if(j == 2) j = 0;

                if( ud.multimode < 2 || (ud.multimode > 1 && j != getSprite(i)->_lotag) )
                    changespritestat(i,5);
                else
                {
#if WIN32
        printf("Spawn: file %s function %s line %i\r\n", file, function, line);
#endif // WIN32
//                    FIXME("CSS10");
                    changespritestat(i,10);
                }
                break;
            case WATERBUBBLE:
                if(j >= 0 && getSprite(j)->_picnum == APLAYER)
                    setSprite_z(i, getSprite(i)->_z - (16<<8));
                if( getSprite(i)->_picnum == WATERBUBBLE)
                {
                    if( j >= 0 )
                        setSprite_ang(i, getSprite(j)->_ang);
                    setSprite_xy_repeats(i, 4, 4);
                }
                else
                    setSprite_xy_repeats(i, 32, 32);

                changespritestat(i,5);
                break;

            case CRANE:

                setSprite_cstat(i, getSprite(i)->_cstat | 64|257);

                setSprite_picnum(i, getSprite(i)->_picnum + 2);
                setSprite_z(i, getSector(sect)->ceilingz+(48<<8));
                setHittypeTemp(i, 4, tempwallptr);

                msx[tempwallptr] = getSprite(i)->_x;
                msy[tempwallptr] = getSprite(i)->_y;
                msx[tempwallptr+2] = getSprite(i)->_z;

                s = headspritestat[0];
                while(s >= 0)
                {
                    if( getSprite(s)->_picnum == CRANEPOLE && SHT == (getSprite(s)->_hitag) )
                    {
                        msy[tempwallptr+2] = s;

                        setHittypeTemp(i, 1, getSprite(s)->_sectnum);

                        setSprite_xy_repeats(s, 64, 128);

                        msx[tempwallptr+1] = getSprite(s)->_x;
                        msy[tempwallptr+1] = getSprite(s)->_y;

                        setSprite_x(s, getSprite(i)->_x);
                        setSprite_y(s, getSprite(i)->_y);
                        setSprite_z(s, getSprite(i)->_z);
                        setSprite_shade(s, getSprite(i)->_shade);

                        setsprite(s,getSprite(s)->_x,getSprite(s)->_y,getSprite(s)->_z);
                        break;
                    }
                    s = spr_add_data[s].nextspritestat;
                }

                tempwallptr += 3;
                if (tempwallptr >= MAX_MOVABLE_SECTORS)
                {
                    FIXME("Too many movable sectors");
                    EXIT(1);
                }
                setSprite_owner(i, -1);
                setSprite_extra(i, 8);
                changespritestat(i,6);
                break;

            case WATERDRIP:
                if((j >= 0 && getSprite(j)->_statnum == 10) || getSprite(j)->_statnum == 1)
                {
                    setSprite_shade(i, 32);
                    if(getSprite(j)->_pal != 1)
                    {
                        setSprite_pal(i, 2);
                        setSprite_z(i, getSprite(i)->_z - (18<<8));
                    }
                    else
                        setSprite_z(i, getSprite(i)->_z - (13<<8));
                    setSprite_ang(i, getangle(ps[connecthead].posx-getSprite(i)->_x,ps[connecthead].posy-getSprite(i)->_y));
                    setSprite_xvel(i, 48-(TRAND&31));
                    ssp(i,CLIPMASK0);
                }
                else if(j == -1)
                {
                    setSprite_z(i, getSprite(i)->_z + (4<<8));
                    setHittypeTemp(i, 0, getSprite(i)->_z);
                    setHittypeTemp(i, 1, TRAND&127);
                }
            case TRASH:

                if(getSprite(i)->_picnum != WATERDRIP)
                    setSprite_ang(i, TRAND&2047);

            case WATERDRIPSPLASH:

                setSprite_xy_repeats(i, 24, 24);

                changespritestat(i,6);
                break;

            case PLUG:
                setSprite_lotag(i, 9999);
                changespritestat(i,6);
                break;
            case TOUCHPLATE:
                setHittypeTemp(i, 2, getSector(sect)->floorz);
                if(getSector(sect)->lotag != 1 && getSector(sect)->lotag != 2)
                    setSector_floorz(sect, getSprite(i)->_z);
                if(getSprite(i)->_pal && ud.multimode > 1)
                {
                    setSprite_xy_repeats(i, 0, 0);
                    changespritestat(i,5);
                    break;
                }
            case WATERBUBBLEMAKER:
                setSprite_cstat(i, getSprite(i)->_cstat | SPRITE_INVISIBLE);
                changespritestat(i,6);
                break;
            case BOLT1:
            case BOLT1+1:
            case BOLT1+2:
            case BOLT1+3:
            case SIDEBOLT1:
            case SIDEBOLT1+1:
            case SIDEBOLT1+2:
            case SIDEBOLT1+3:
                setHittypeTemp(i, 0, getSprite(i)->_xrepeat);
                setHittypeTemp(i, 1, getSprite(i)->_yrepeat);
            case MASTERSWITCH:
                if(getSprite(i)->_picnum == MASTERSWITCH)
                    setSprite_cstat(i, getSprite(i)->_cstat | SPRITE_INVISIBLE);
                setSprite_yvel(i, 0);
                changespritestat(i,6);
                break;
            case TARGET:
            case DUCK:
            case LETTER:
                setSprite_extra(i, 1);
                setSprite_cstat(i, getSprite(i)->_cstat | 257);
                changespritestat(i,1);
                break;
            case OCTABRAINSTAYPUT:
            case LIZTROOPSTAYPUT:
            case PIGCOPSTAYPUT:
            case LIZMANSTAYPUT:
            case BOSS1STAYPUT:
            case PIGCOPDIVE:
            case COMMANDERSTAYPUT:
            case BOSS4STAYPUT:
#if MINIMIZE_ACTORSTAYPUT
                getHittype(i)->actorstayput = getSprite(i)->_sectnum >= 0 ? getSprite(i)->_sectnum : MAXSECTORS; //next-hack: added to make sure it is not -1
#else
                setHittype_actorstayput(i, getSprite(i)->_sectnum); //next-hack: added to make sure it is not -1
#endif
            case BOSS1:
            case BOSS2:
            case BOSS3:
            case BOSS4:
            case ROTATEGUN:
            case GREENSLIME:
                if(getSprite(i)->_picnum == GREENSLIME)
                    setSprite_extra(i, 1);
            case DRONE:
            case LIZTROOPONTOILET:
            case LIZTROOPJUSTSIT:
            case LIZTROOPSHOOT:
            case LIZTROOPJETPACK:
            case LIZTROOPDUCKING:
            case LIZTROOPRUNNING:
            case LIZTROOP:
            case OCTABRAIN:
            case COMMANDER:
            case PIGCOP:
            case LIZMAN:
            case LIZMANSPITTING:
            case LIZMANFEEDING:
            case LIZMANJUMP:
            case ORGANTIC:
            case RAT:
            case SHARK:

                if(getSprite(i)->_pal == 0)
                {
                    switch(getSprite(i)->_picnum)
                    {
                        case LIZTROOPONTOILET:
                        case LIZTROOPSHOOT:
                        case LIZTROOPJETPACK:
                        case LIZTROOPDUCKING:
                        case LIZTROOPRUNNING:
                        case LIZTROOPSTAYPUT:
                        case LIZTROOPJUSTSIT:
                        case LIZTROOP:
                            setSprite_pal(i, 22);
                            break;
                    }
                }

                if( getSprite(i)->_picnum == BOSS4STAYPUT || getSprite(i)->_picnum == BOSS1 || getSprite(i)->_picnum == BOSS2 || getSprite(i)->_picnum == BOSS1STAYPUT || getSprite(i)->_picnum == BOSS3 || getSprite(i)->_picnum == BOSS4 )
                {
                    if(j >= 0 && getSprite(j)->_picnum == RESPAWN)
                        setSprite_pal(i, getSprite(j)->_pal);
                    if(getSprite(i)->_pal)
                    {
                        sp_d->clipdist = 80;
                        setSprite_xy_repeats(i, 40, 40);
                    }
                    else
                    {
                        setSprite_xy_repeats(i, 80, 80);
                        sp_d->clipdist = 164;
                    }
                }
                else
                {
                    if(getSprite(i)->_picnum != SHARK)
                    {
                        setSprite_xy_repeats(i, 40, 40);
                        sp_d->clipdist = 80;
                    }
                    else
                    {
                        setSprite_xy_repeats(i, 60, 60);
                        sp_d->clipdist = 40;
                    }
                }

                if(j >= 0)
                    setSprite_lotag(i, 0);

                if( ( getSprite(i)->_lotag > ud.player_skill ) || ud.monsters_off == 1 )
                {
                    setSprite_xy_repeats(i, 0, 0);
                    changespritestat(i,5);
                    break;
                }
                else
                {
                    makeitfall(i);

                    if(getSprite(i)->_picnum == RAT)
                    {
                        setSprite_ang(i, TRAND&2047);
                        setSprite_xy_repeats(i, 48, 48);
                        setSprite_cstat(i, 0);
                    }
                    else
                    {
                        setSprite_cstat(i, getSprite(i)->_cstat | 257);

                        if(getSprite(i)->_picnum != SHARK)
                            ps[myconnectindex].max_actors_killed++;
                    }

                    if(getSprite(i)->_picnum == ORGANTIC) setSprite_cstat(i, getSprite(i)->_cstat | 128);

                    if(j >= 0)
                    {
                        setHittype_timetosleep(i, 0);
                        check_fta_sounds(i);
                        changespritestat(i,1);
                    }
                    else changespritestat(i,2);
                }

                if(getSprite(i)->_picnum == ROTATEGUN)
                    setSprite_zvel(i, 0);

                break;

            case LOCATORS:
                setSprite_cstat(i, getSprite(i)->_cstat | SPRITE_INVISIBLE);
                changespritestat(i,7);
                break;

            case ACTIVATORLOCKED:
            case ACTIVATOR:
                setSprite_cstat(i, SPRITE_INVISIBLE);
                if(getSprite(i)->_picnum == ACTIVATORLOCKED)
                    setSector_lotag(getSprite(i)->_sectnum, getSector(getSprite(i)->_sectnum)->lotag | 16384);
                changespritestat(i,8);
                break;

            case DOORSHOCK:
                setSprite_cstat(i, getSprite(i)->_cstat | (1+256));
                setSprite_shade(i, -12);
                changespritestat(i,6);
                break;

            case OOZ:
            case OOZ2:
                setSprite_shade(i, -12);

                if(j >= 0)
                {
                    if( getSprite(j)->_picnum == NUKEBARREL )
                        setSprite_pal(i, 8);
                    insertspriteq(i);
                }

                changespritestat(i,1);

                getglobalz(i);

                j = (getHittype(i)->floorz-getHittype(i)->ceilingz)>>9;

                setSprite_xy_repeats(i, 25-(j>>1), j);
                setSprite_cstat(i, getSprite(i)->_cstat | (TRAND&4));

                break;

            case HEAVYHBOMB:
                if(j >= 0)
                    setSprite_owner(i, j);
                else
                    setSprite_owner(i, i);
                setSprite_xy_repeats(i, 9, 9);
                setSprite_yvel(i, 4);
            case REACTOR2:
            case REACTOR:
            case RECON:

                if(getSprite(i)->_picnum == RECON)
                {
                    if( getSprite(i)->_lotag > ud.player_skill )
                    {
                        setSprite_xy_repeats(i, 0, 0);
                        changespritestat(i,5);
                        return i;
                    }
                    ps[myconnectindex].max_actors_killed++;
                    setHittypeTemp(i, 5, 0);
                    if(ud.monsters_off == 1)
                    {
                        setSprite_xy_repeats(i, 0, 0);
                        changespritestat(i,5);
                        break;
                    }
                    setSprite_extra(i, 130);
                }

                if(getSprite(i)->_picnum == REACTOR || getSprite(i)->_picnum == REACTOR2)
                    setSprite_extra(i, impact_damage);

                setSprite_cstat(i, getSprite_cstat(i) | 257); // CS |= 257; // Make it hitable   <--- shitty code.

                if( ud.multimode < 2 && getSprite(i)->_pal != 0)
                {
                    setSprite_xy_repeats(i, 0, 0);
                    changespritestat(i,5);
                    break;
                }
                setSprite_pal(i, 0);
                setSprite_shade(i, -17); //SS = -17; // crap code

                changespritestat(i,2);
                break;

            case ATOMICHEALTH:
            case STEROIDS:
            case HEATSENSOR:
            case SHIELD:
            case AIRTANK:
            case TRIPBOMBSPRITE:
            case JETPACK:
            case HOLODUKE:

            case FIRSTGUNSPRITE:
            case CHAINGUNSPRITE:
            case SHOTGUNSPRITE:
            case RPGSPRITE:
            case SHRINKERSPRITE:
            case FREEZESPRITE:
            case DEVISTATORSPRITE:

            case SHOTGUNAMMO:
            case FREEZEAMMO:
            case HBOMBAMMO:
            case CRYSTALAMMO:
            case GROWAMMO:
            case BATTERYAMMO:
            case DEVISTATORAMMO:
            case RPGAMMO:
            case BOOTS:
            case AMMO:
            case AMMOLOTS:
            case COLA:
            case FIRSTAID:
            case SIXPAK:
                if(j >= 0)
                {   // next-hack: spawned during game ?
                    setSprite_lotag(i, 0);
                    setSprite_z(i, getSprite(i)->_z - (32<<8));
                    setSprite_zvel(i, -1024);
                    ssp(i,CLIPMASK0);
                    setSprite_cstat(i, TRAND&4);
                }
                else
                {   // next-hack: spawned in prelevel ?
#if WIN32
            bonuses++;
            printf("Bonuses: %d\r\n", bonuses);
#endif // WIN32

                    setSprite_owner(i, i);
                    setSprite_cstat(i, 0);
                }

                if( ( ud.multimode < 2 && getSprite(i)->_pal != 0) || (getSprite(i)->_lotag > ud.player_skill) )
                {
                    setSprite_xy_repeats(i, 0, 0);
                    changespritestat(i,5);
                    break;
                }

                setSprite_pal(i, 0);

            case ACCESSCARD:

                if(getSprite(i)->_picnum == ATOMICHEALTH) // why do you put it here? That's very unreadable!
                    setSprite_cstat(i, getSprite(i)->_cstat | 128);

                if(ud.multimode > 1 && ud.coop != 1 && getSprite(i)->_picnum == ACCESSCARD)
                {
                    setSprite_xy_repeats(i, 0, 0);
                    changespritestat(i,5);
                    break;
                }
                else
                {
                    if(getSprite(i)->_picnum == AMMO)
                        setSprite_xy_repeats(i, 16, 16);
                    else
                        setSprite_xy_repeats(i, 32, 32);
                }

                setSprite_shade(i, -17);

                if(j >= 0)
                    changespritestat(i,1);
                else
                {
                    changespritestat(i,2);
                    makeitfall(i);
                }
                break;

            case WATERFOUNTAIN:
                setSprite_lotag(i, 1); //SLT = 1; // crapcode

            case TREE1:
            case TREE2:
            case TIRE:
            case CONE:
            case BOX:
                setSprite_cstat(i, 257); //CS = 257; // Make it hitable // crapcode
                setSprite_extra(i, 1);
                changespritestat(i,6);
                break;

            case FLOORFLAME:
                setSprite_shade(i, -127);
                changespritestat(i,6);
                break;

            case BOUNCEMINE:
                setSprite_owner(i, i);
                setSprite_cstat(i, getSprite(i)->_cstat | (1+256)); //Make it hitable
                setSprite_xy_repeats(i, 24, 24);
                setSprite_shade(i, -127);
                setSprite_extra(i, impact_damage<<2);
                changespritestat(i,2);
                break;

            case CAMERA1:
            case CAMERA1+1:
            case CAMERA1+2:
            case CAMERA1+3:
            case CAMERA1+4:
            case CAMERAPOLE:
                setSprite_extra(i, 1);

                if(camerashitable)
                    setSprite_cstat(i, 257);
                else
                    setSprite_cstat(i, 0);

            case GENERICPOLE:

                if( ud.multimode < 2 && getSprite(i)->_pal != 0 )
                {
                    setSprite_xy_repeats(i, 0, 0);
                    changespritestat(i,5);
                    break;
                }
                else setSprite_pal(i, 0);
                if(getSprite(i)->_picnum == CAMERAPOLE || getSprite(i)->_picnum == GENERICPOLE) break;
                    setSprite_picnum(i, CAMERA1);
                changespritestat(i,1);
                break;
            case STEAM:
                if(j >= 0)
                {
                    setSprite_ang(i, getSprite(j)->_ang);
                    setSprite_cstat(i, 16+128+2);
                    setSprite_xy_repeats(i, 1, 1);
                    setSprite_xvel(i, -8);
                    ssp(i,CLIPMASK0);
                }
            case CEILINGSTEAM:
                changespritestat(i,6);
                break;

            case SECTOREFFECTOR:
                setSprite_yvel(i, getSector_extra(sect));
                setSprite_cstat(i, getSprite(i)->_cstat | SPRITE_INVISIBLE);
                setSprite_xy_repeats(i, 0, 0);

                switch(getSprite(i)->_lotag)
                {
                    case 28:
                        setHittypeTemp(i, 5, 65);// Delay for lightning
                        break;
                    case 7: // Transporters!!!!
                    case 23:// XPTR END
                        if(getSprite(i)->_lotag != 23)
                        {
                            for(j=0;j<MAXSPRITES;j++)
                                if(getSprite(j)->_statnum < MAXSTATUS && getSprite(j)->_picnum == SECTOREFFECTOR && ( getSprite(j)->_lotag == 7 || getSprite(j)->_lotag == 23 ) && i != j && getSprite(j)->_hitag == SHT)
                                {
                                    setSprite_owner(i, j);//OW = j; // I hate this code.
                                    break;
                                }
                        }
                        else
                            setSprite_owner(i, i); //OW = i; idem

                        setHittypeTemp(i, 4, getSector(sect)->floorz == SZ);
                        setSprite_cstat(i, 0);
                        changespritestat(i,9);
                        return i;
                    case 1:
                        setSprite_owner(i, -1);
                        setHittypeTemp(i, 0, 1);
                        break;
                    case 18:

                        if(getSprite(i)->_ang == 512)
                        {
                            setHittypeTemp(i, 1, getSector(sect)->ceilingz);
                            if(getSprite(i)->_pal)
                                setSector_ceilingz(sect, getSprite(i)->_z);
                        }
                        else
                        {
                            setHittypeTemp(i, 1, getSector(sect)->floorz);
                            if(getSprite(i)->_pal)
                                setSector_floorz(sect, getSprite(i)->_z);
                        }

                        setSprite_hitag(i, getSprite(i)->_hitag << 2);
                        break;

                    case 19:
                        setSprite_owner(i, -1);
                        break;
                    case 25: // Pistons
                        setHittypeTemp(i, 3, getSector(sect)->ceilingz);
                        setHittypeTemp(i, 4, 1);
                        setSector_ceilingz(sect, getSprite(i)->_z);
#if INTERPOLATION_USES_POSPTR
                        setinterpolation(getSector_ceilingzptr(sect));
#else
                        setinterpolation(getSector_ceilingzidx(sect));
#endif
                        break;
                    case 35:
                        setSector_ceilingz(sect, getSprite(i)->_z);
                        break;
                    case 27:
                        if(ud.recstat == 1)
                        {
                            setSprite_xy_repeats(i, 64, 64);
                            setSprite_cstat(i, getSprite(i)->_cstat & ~SPRITE_INVISIBLE); // was 32767
                        }
                        break;
                    case 12:

                        setHittypeTemp(i, 1, getSector_floorshade(sect));
                        setHittypeTemp(i, 2, getSector_ceilingshade(sect));
                        break;

                    case 13:

                        setHittypeTemp(i, 0, getSector(sect)->ceilingz);
                        setHittypeTemp(i, 1, getSector(sect)->floorz);

                        if( klabs(getHittypeTemp(i, 0)-getSprite(i)->_z) < klabs(getHittypeTemp(i, 1)-getSprite(i)->_z) )
                            setSprite_owner(i, 1);
                        else
                            setSprite_owner(i, 0);

                        if(getSprite(i)->_ang == 512)
                        {
                            if(getSprite(i)->_owner)
                                setSector_ceilingz(sect, getSprite(i)->_z);
                            else
                                setSector_floorz(sect, getSprite(i)->_z);
                        }
                        else
                        {
                            setSector_ceilingz(sect, getSprite(i)->_z);
                            setSector_floorz(sect, getSprite(i)->_z);
                        }

                        if( getSector(sect)->ceilingstat&1 )
                        {
                            setSector_ceilingstat(sect, getSector(sect)->ceilingstat ^ 1);
                            setHittypeTemp(i, 3, 1);

                            if(!getSprite(i)->_owner && getSprite(i)->_ang==512)
                            {
                                setSector_ceilingstat(sect, getSector(sect)->ceilingstat ^ 1);
                                setHittypeTemp(i, 3, 0);
                            }

                            setSector_ceilingshade(sect, getSector_floorshade(sect));

                            if(getSprite(i)->_ang==512)
                            {
                                startwall = constsector[sect].wallptr;
                                endwall = startwall+constsector[sect].wallnum;
                                for(j=startwall;j<endwall;j++)
                                {
                                    x = constwall[j].nextsector;
                                    if(x >= 0)
                                        if( !(getSector(x)->ceilingstat&1) )
                                    {
                                        setSector_ceilingpicnum(sect, getSector(x)->ceilingpicnum);
                                        setSector_ceilingshade(sect, getSector_ceilingshade(x));
                                        break; //Leave early
                                    }
                                }
                            }
                        }

                        break;

                    case 17:

                        setHittypeTemp(i, 2, getSector(sect)->floorz); //Stopping loc

                        j = nextsectorneighborz(sect,getSector(sect)->floorz,-1,-1);
                        setHittypeTemp(i, 3, getSector(j)->ceilingz);

                        j = nextsectorneighborz(sect,getSector(sect)->ceilingz,1,1);
                        setHittypeTemp(i, 4, getSector(j)->floorz);

                        if(numplayers < 2)
                        {
#if INTERPOLATION_USES_POSPTR
                            setinterpolation(getSector_floorzptr(sect));
                            setinterpolation(getSector_ceilingzptr(sect));
#else
                            setinterpolation(getSector_floorzidx(sect));
                            setinterpolation(getSector_ceilingzidx(sect));
#endif
                        }

                        break;

                    case 24:
                        setSprite_yvel(i, getSprite(i)->_yvel << 1);
                    case 36:
                        break;

                    case 20:
                    {
                        int32_t q;

                        startwall = constsector[sect].wallptr;
                        endwall = startwall+constsector[sect].wallnum;

                        //find the two most clostest wall x's and y's
                        q = 0x7fffffff;

                        for(s=startwall;s<endwall;s++)
                        {
                            x = getWall(s)->x;
                            y = getWall(s)->y;

                            d = FindDistance2D(getSprite(i)->_x-x,getSprite(i)->_y-y);
                            if( d < q )
                            {
                                q = d;
                                clostest = s;
                            }
                        }

                        setHittypeTemp(i, 1, clostest);

                        q = 0x7fffffff;

                        for(s=startwall;s<endwall;s++)
                        {
                            x = getWall(s)->x;
                            y = getWall(s)->y;

                            d = FindDistance2D(getSprite(i)->_x-x,getSprite(i)->_y-y);
                            if(d < q && s != getHittypeTemp(i, 1) )
                            {
                                q = d;
                                clostest = s;
                            }
                        }

                        setHittypeTemp(i, 2, clostest);
                    }

                    break;

                    case 3:

                        setHittypeTemp(i, 3, getSector_floorshade(sect));

                        setSector_floorshade(sect, getSprite(i)->_shade);
                        setSector_ceilingshade(sect, getSprite(i)->_shade);

                        setSprite_owner(i, getSector(sect)->ceilingpal<<8);
                        setSprite_owner(i, getSprite(i)->_owner | getSector(sect)->floorpal);

                        //fix all the walls;

                        startwall = constsector[sect].wallptr;
                        endwall = startwall+constsector[sect].wallnum;

                        for(s=startwall;s<endwall;s++)
                        {
                            if(!(getWall_hitag(s)&1))
                            {
                                setWall_shade(s, getSprite(i)->_shade);
//                                            WARNING("SHADE\r\n");
                            }
                            if( (getWall_cstat(s)&2) && constwall[s].nextwall >= 0)
                            {
                                setWall_shade(constwall[s].nextwall, getSprite(i)->_shade);
//                                            WARNING("SHADE\r\n");
                            }
                        }
                        break;

                    case 31:
                        setHittypeTemp(i, 1, getSector(sect)->floorz);
                    //    getHittypeTemp(i, 2) = getSprite(i)->_hitag;
                        if(getSprite(i)->_ang != 1536)
                            setSector_floorz(sect, getSprite(i)->_z);

                        startwall = constsector[sect].wallptr;
                        endwall = startwall+constsector[sect].wallnum;

                        for(s=startwall;s<endwall;s++)
                            if(getWall_hitag(s) == 0)
                            {
#if HITAG_CONSTANT
                                setWall_hitagTo9999(s);
#else
                                void *dummy;
                                dummy = setWall_hitag(s, 9999);
#endif
                            }
#if INTERPOLATION_USES_POSPTR
                        setinterpolation(getSector_floorzptr(sect));
#else
                        setinterpolation(getSector_floorzidx(sect));
#endif
                        break;
                    case 32:
                        setHittypeTemp(i, 1, getSector(sect)->ceilingz);
                        setHittypeTemp(i, 2, getSprite(i)->_hitag);
                        if(getSprite(i)->_ang != 1536)
                            setSector_ceilingz(sect, getSprite(i)->_z);

                        startwall = constsector[sect].wallptr;
                        endwall = startwall+constsector[sect].wallnum;

                        for(s=startwall;s<endwall;s++)
                            if(getWall_hitag(s) == 0)
                            {
#if HITAG_CONSTANT
                                setWall_hitagTo9999(s);
#else
                                void *dummy;
                                dummy = setWall_hitag(s, 9999);
#endif
                            }
#if INTERPOLATION_USES_POSPTR
                        setinterpolation(getSector_ceilingzptr(sect));
#else
                        setinterpolation(getSector_ceilingzidx(sect));
#endif
                        break;

                    case 4: //Flashing lights

                        setHittypeTemp(i, 2, getSector_floorshade(sect));

                        startwall = constsector[sect].wallptr;
                        endwall = startwall+constsector[sect].wallnum;

                        setSprite_owner(i, getSector(sect)->ceilingpal<<8);
                        setSprite_owner(i, getSprite(i)->_owner | getSector(sect)->floorpal);

                        for(s=startwall;s<endwall;s++)
                            if(getWall_shade(s) > getHittypeTemp(i, 3))
                                setHittypeTemp(i, 3, getWall_shade(s));

                        break;

                    case 9:
                        if( getSector(sect)->lotag &&
                            labs(getSector(sect)->ceilingz-getSprite(i)->_z) > 1024)
                                setSector_lotag(sect, getSector(sect)->lotag | 32768); //If its open
                    case 8:
                        //First, get the ceiling-floor shade

                        setHittypeTemp(i, 0, getSector_floorshade(sect));
                        setHittypeTemp(i, 1, getSector_ceilingshade(sect));

                        startwall = constsector[sect].wallptr;
                        endwall = startwall+constsector[sect].wallnum;

                        for(s=startwall;s<endwall;s++)
                            if(getWall_shade(s) > getHittypeTemp(i, 2))
                                setHittypeTemp(i, 2, getWall_shade(s));

                        setHittypeTemp(i, 3, 1); //Take Out;

                        break;

                    case 11://Pivitor rotater
                        if(getSprite(i)->_ang>1024)
                            setHittypeTemp(i, 3, 2);
                        else
                            setHittypeTemp(i, 3, -2);
                    case 0:
                    case 2://Earthquakemakers
                    case 5://Boss Creature
                    case 6://Subway
                    case 14://Caboos
                    case 15://Subwaytype sliding door
                    case 16://That rotating blocker reactor thing
                    case 26://ESCELATOR
                    case 30://No rotational subways

                        if(getSprite(i)->_lotag == 0)
                        {
                            if( getSector(sect)->lotag == 30 )
                            {
                                if(getSprite(i)->_pal)
                                    spr_add_data[i].clipdist = 1;
                                else
                                    spr_add_data[i].clipdist = 0;
                                setHittypeTemp(i, 3, getSector(sect)->floorz);
                                setSector_hitag(sect, i);
                            }

                            for(j = 0;j < MAXSPRITES;j++)
                            {
                                if( getSprite(j)->_statnum < MAXSTATUS )
                                if( getSprite(j)->_picnum == SECTOREFFECTOR &&
                                    getSprite(j)->_lotag == 1 &&
                                    getSprite(j)->_hitag == getSprite(i)->_hitag)
                                {
                                    if( getSprite(i)->_ang == 512 )
                                    {
                                        setSprite_x(i, getSprite(j)->_x);
                                        setSprite_y(i, getSprite(j)->_y);
                                    }
                                    break;
                                }
                            }
                            if(j == MAXSPRITES)
                            {
                                sprintf(text,"Found lonely Sector Effector (lotag 0) at (%d,%d)\n",getSprite(i)->_x,getSprite(i)->_y);
                                gameexit(text);
                            }
                            setSprite_owner(i, j);
                        }

                        startwall = constsector[sect].wallptr;
                        endwall = startwall+constsector[sect].wallnum;

                        setHittypeTemp(i, 1, tempwallptr);
                        for(s=startwall;s<endwall;s++)
                        {
                            msx[tempwallptr] = getWall(s)->x-getSprite(i)->_x;
                            msy[tempwallptr] = getWall(s)->y-getSprite(i)->_y;
                            tempwallptr++;
                            if(tempwallptr > MAX_MOVABLE_SECTORS - 1)
                            {
                                sprintf(text,"Too many moving sectors at (%d,%d).\n",getWall(s)->x,getWall(s)->y);
                                gameexit(text);
                            }
                        }
                        if( getSprite(i)->_lotag == 30 || getSprite(i)->_lotag == 6 || getSprite(i)->_lotag == 14 || getSprite(i)->_lotag == 5 )
                        {

                            startwall = constsector[sect].wallptr;
                            endwall = startwall+constsector[sect].wallnum;

                            if(getSector(sect)->hitag == -1)
                                setSprite_extra(i, 0);
                            else setSprite_extra(i, 1);

                            setSector_hitag(sect, i);

                            j = 0;

                            for(s=startwall;s<endwall;s++)
                            {
                                if( constwall[s].nextsector >= 0 &&
                                    getSector(constwall[s].nextsector)->hitag == 0 &&
                                        getSector(constwall[s].nextsector)->lotag < 3 )
                                    {
                                        s = constwall[s].nextsector;
                                        j = 1;
                                        break;
                                    }
                            }

                            if(j == 0)
                            {
                                sprintf(text,"Subway found no zero'd sectors with locators\nat (%d,%d).\n",getSprite(i)->_x,getSprite(i)->_y);
                                gameexit(text);
                            }

                            setSprite_owner(i, -1);
                            setHittypeTemp(i, 0, s);    // max: MAXWALLS

                            if(getSprite(i)->_lotag != 30)
                                setHittypeTemp(i, 3, getSprite(i)->_hitag);
                        }

                        else if(getSprite(i)->_lotag == 16)
                            setHittypeTemp(i, 3, getSector(sect)->ceilingz);

                        else if( getSprite(i)->_lotag == 26 )
                        {
                            setHittypeTemp(i, 3, getSprite(i)->_x);
                            setHittypeTemp(i, 4, getSprite(i)->_y);
                            if(getSprite(i)->_shade==getSector_floorshade(sect)) //UP
                                setSprite_zvel(i, -256);
                            else
                                setSprite_zvel(i, 256);

                            setSprite_shade(i, 0);
                        }
                        else if( getSprite(i)->_lotag == 2)
                        {
                            setHittypeTemp(i, 5, getSector(getSprite(i)->_sectnum)->floorheinum);
                            setSector_floorheinum(getSprite(i)->_sectnum, 0);
                        }
                }

                switch(getSprite(i)->_lotag)
                {
                    case 6:
                    case 14:
                        j = callsound(sect,i);
                        if(j == -1) j = SUBWAY;
                            setHittype_lastvx(i, j);
                    case 30:
                        if(numplayers > 1) break;
                    case 0:
                    case 1:
                    case 5:
                    case 11:
                    case 15:
                    case 16:
                    case 26:
                        setsectinterpolate(i);
                        break;
                }

                switch(getSprite(i)->_lotag)
                {
                    case 40:
                    case 41:
                    case 43:
                    case 44:
                    case 45:
                        changespritestat(i,15);
                        break;
                    default:
                        changespritestat(i,3);
                        break;
                }

                break;


            case SEENINE:
            case OOZFILTER:

                setSprite_shade(i, -16);
                if(getSprite(i)->_xrepeat <= 8)
                {
                    setSprite_cstat(i, (short)SPRITE_INVISIBLE);
                    setSprite_xy_repeats(i, 0, 0);
                }
                else
                    setSprite_cstat(i, 1+256);
                setSprite_extra(i, impact_damage<<2);
                setSprite_owner(i, i);

                changespritestat(i,6);
                break;

            case CRACK1:
            case CRACK2:
            case CRACK3:
            case CRACK4:
            case FIREEXT:
                if(getSprite(i)->_picnum == FIREEXT)
                {
                    setSprite_cstat(i, 257);
                    setSprite_extra(i, impact_damage<<2);
                }
                else
                {
                    setSprite_cstat(i, getSprite(i)->_cstat | 17);
                    setSprite_extra(i, 1);
                }

                if( ud.multimode < 2 && getSprite(i)->_pal != 0)
                {
                    setSprite_xy_repeats(i, 0, 0);
                    changespritestat(i,5);
                    break;
                }

                setSprite_pal(i, 0);
                setSprite_owner(i, i);
                changespritestat(i,6);
                setSprite_xvel(i, 8);
                ssp(i,CLIPMASK0);
                break;

            case TOILET:
            case STALL:
                setSprite_lotag(i, 1);
                setSprite_cstat(i, getSprite(i)->_cstat | 257);
                sp_d->clipdist = 8;
                setSprite_owner(i, i);
                break;
            case CANWITHSOMETHING:
            case CANWITHSOMETHING2:
            case CANWITHSOMETHING3:
            case CANWITHSOMETHING4:
            case RUBBERCAN:
                setSprite_extra(i, 0);
            case EXPLODINGBARREL:
            case HORSEONSIDE:
            case FIREBARREL:
            case NUKEBARREL:
            case FIREVASE:
            case NUKEBARRELDENTED:
            case NUKEBARRELLEAKED:
            case WOODENHORSE:

                if(j >= 0)
                    setSprite_xy_repeats(i, 32, 32);
                sp_d->clipdist = 72;
                makeitfall(i);
                if(j >= 0)
                    setSprite_owner(i, j);
                else setSprite_owner(i, i);
            case EGG:
                if( ud.monsters_off == 1 && getSprite(i)->_picnum == EGG )
                {
                    setSprite_xy_repeats(i, 0, 0);
                    changespritestat(i,5);
                }
                else
                {
                    if(getSprite(i)->_picnum == EGG)
                        sp_d->clipdist = 24;
                    setSprite_cstat(i, 257|(TRAND&4));
                    changespritestat(i,2);
                }
                break;
            case TOILETWATER:
                setSprite_shade(i, -16);
                changespritestat(i,6);
                break;
    }
    return i;
}

void animatesprites(int32_t x,int32_t y,short a,int32_t smoothratio)
{
    short i, j, k, p, sect;
    int32_t l, t1,t3,t4;
//    spritetype *s;
    tspritetype *t;

    for(j=0;j < spritesortcnt; j++)
    {
        t = &tsprite[j];
        i = t->owner;
//        s = &sprite[t->owner];

        switch(t->picnum)
        {
            case BLOODPOOL:
            case PUKE:
            case FOOTPRINTS:
            case FOOTPRINTS2:
            case FOOTPRINTS3:
            case FOOTPRINTS4:
                if(t->shade == 127) continue;
                break;
            case RESPAWNMARKERRED:
            case RESPAWNMARKERYELLOW:
            case RESPAWNMARKERGREEN:
                if(ud.marker == 0)
                    t->xrepeat = t->yrepeat = 0;
                continue;
            case CHAIR3:

                k = (((t->ang+3072+128-a)&2047)>>8)&7;
                if(k>4)
                {
                    k = 8-k;
                    t->cstat |= 4;
                }
                else t->cstat &= ~4;
                t->picnum = getSprite(i)->_picnum+k;
                break;
            case BLOODSPLAT1:
            case BLOODSPLAT2:
            case BLOODSPLAT3:
            case BLOODSPLAT4:
                if(ud.lockout) t->xrepeat = t->yrepeat = 0;
                else if(t->pal == 6)
                {
                    t->shade = -127;
                    continue;
                }
            case BULLETHOLE:
            case CRACK1:
            case CRACK2:
            case CRACK3:
            case CRACK4:
                t->shade = 16;
                continue;
            case NEON1:
            case NEON2:
            case NEON3:
            case NEON4:
            case NEON5:
            case NEON6:
                continue;
            case GREENSLIME:
            case GREENSLIME+1:
            case GREENSLIME+2:
            case GREENSLIME+3:
            case GREENSLIME+4:
            case GREENSLIME+5:
            case GREENSLIME+6:
            case GREENSLIME+7:
                break;
            default:
#if TSPRITE_HAS_EXTRA
                if( ( (t->cstat&16) ) || ( badguypic(t->picnum) && t->extra > 0) || t->statnum == 10)
                    continue;
#else
                if( ( (t->cstat&16) ) || ( badguypic(t->picnum) && t->extraBiggerThanZero) || t->statnum == 10)
                    continue;
#endif
        }

        if (getSector(t->sectnum)->ceilingstat&1)
            l = getSector_ceilingshade(t->sectnum);
        else
            l = getSector_floorshade(t->sectnum);

        if(l < -127) l = -127;
        if(l > 128) l =  127;
        t->shade = l;
    }


    for(j=0;j < spritesortcnt; j++ )  //Between drawrooms() and drawmasks()
    {                             //is the perfect time to animate sprites
        t = &tsprite[j];
        i = t->owner;
       // s = &sprite[i];
        //spritetypeoffs *so = &sprite_offsets[i];
        switch(getSprite(i)->_picnum)
        {
            case SECTOREFFECTOR:
#if TSPRITE_HAS_LOTAG
                if(t->lotag == 27 && ud.recstat == 1)
#else
                if(t->lotagIs27 && ud.recstat == 1)
#endif
                {
                    t->picnum = 11+((totalclock>>3)&1);
                    t->cstat |= 128;
                }
                else
                    t->xrepeat = t->yrepeat = 0;
                break;
            case NATURALLIGHTNING:
               t->shade = -127;
               break;
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
            case MAN:
            case MAN2:
            case WOMAN:
            case NAKED1:
            case PODFEM1:
            case FEMMAG1:
            case FEMMAG2:
            case FEMPIC1:
            case FEMPIC2:
            case FEMPIC3:
            case FEMPIC4:
            case FEMPIC5:
            case FEMPIC6:
            case FEMPIC7:
            case BLOODYPOLE:
            case FEM6PAD:
            case STATUE:
            case STATUEFLASH:
            case OOZ:
            case OOZ2:
            case WALLBLOOD1:
            case WALLBLOOD2:
            case WALLBLOOD3:
            case WALLBLOOD4:
            case WALLBLOOD5:
            case WALLBLOOD7:
            case WALLBLOOD8:
            case SUSHIPLATE1:
            case SUSHIPLATE2:
            case SUSHIPLATE3:
            case SUSHIPLATE4:
            case FETUS:
            case FETUSJIB:
            case FETUSBROKE:
            case HOTMEAT:
            case FOODOBJECT16:
            case DOLPHIN1:
            case DOLPHIN2:
            case TOUGHGAL:
            case TAMPON:
            case XXXSTACY:
            case 4946:
            case 4947:
            case 693:
            case 2254:
            case 4560:
            case 4561:
            case 4562:
            case 4498:
            case 4957:
                if(ud.lockout)
                {
                    t->xrepeat = t->yrepeat = 0;
                    continue;
                }
        }
        if( t->statnum == 99 ) continue;
        if( getSprite(i)->_statnum != 1 && getSprite(i)->_picnum == APLAYER && ps[getSprite(i)->_yvel].newowner == -1 && getSprite(i)->_owner >= 0 )
        {
            t->x -= mulscale16(65536-smoothratio,ps[getSprite(i)->_yvel].posx-ps[getSprite(i)->_yvel].oposx);
            t->y -= mulscale16(65536-smoothratio,ps[getSprite(i)->_yvel].posy-ps[getSprite(i)->_yvel].oposy);
            t->z = ps[getSprite(i)->_yvel].oposz + mulscale16(smoothratio,ps[getSprite(i)->_yvel].posz-ps[getSprite(i)->_yvel].oposz);
            t->z += (40<<8);
        }
        else if( ( getSprite(i)->_statnum == 0 && getSprite(i)->_picnum != CRANEPOLE) || getSprite(i)->_statnum == 10 || getSprite(i)->_statnum == 6 || getSprite(i)->_statnum == 4 || getSprite(i)->_statnum == 5 || getSprite(i)->_statnum == 1 )
        {
            t->x -= mulscale16(65536-smoothratio,getSprite(i)->_x-getHittype(i)->bposx);
            t->y -= mulscale16(65536-smoothratio,getSprite(i)->_y-getHittype(i)->bposy);
            t->z -= mulscale16(65536-smoothratio,getSprite(i)->_z-getHittype(i)->bposz);
        }

        sect = getSprite(i)->_sectnum;
        t1 = getHittypeTemp(i, 1) ;t3 = getHittypeTemp(i, 3);t4 = getHittypeTemp(i, 4);

        switch(getSprite(i)->_picnum)
        {
            case DUKELYINGDEAD:
                t->z += (24<<8);
                break;
            case BLOODPOOL:
            case FOOTPRINTS:
            case FOOTPRINTS2:
            case FOOTPRINTS3:
            case FOOTPRINTS4:
                if(t->pal == 6)
                    t->shade = -127;
            case PUKE:
            case MONEY:
            case MONEY+1:
            case MAIL:
            case MAIL+1:
            case PAPER:
            case PAPER+1:
                if(ud.lockout && getSprite(i)->_pal == 2)
                {
                    t->xrepeat = t->yrepeat = 0;
                    continue;
                }
                break;
            case TRIPBOMB:
                continue;
            case FORCESPHERE:
                if(t->statnum == 5)
                {
                    short sqa,sqb;

                    sqa =
                        getangle(
                            getSprite(getSprite(i)->_owner)->_x-ps[screenpeek].posx,
                            getSprite(getSprite(i)->_owner)->_y-ps[screenpeek].posy);
                    sqb =
                        getangle(
                            getSprite(getSprite(i)->_owner)->_x-t->x,
                            getSprite(getSprite(i)->_owner)->_y-t->y);

                    if( klabs(getincangle(sqa,sqb)) > 512 )
                        if( ldist_sprite_tsprite(getSprite(getSprite(i)->_owner),t) < ldist(getSprite(ps[screenpeek].i),getSprite(getSprite(i)->_owner)) )
                            t->xrepeat = t->yrepeat = 0;
                }
                continue;
            case BURNING:
            case BURNING2:
                if( getSprite(getSprite(i)->_owner)->_statnum == 10 )
                {
                    if( display_mirror == 0 && getSprite(getSprite(i)->_owner)->_yvel == screenpeek && ps[getSprite(getSprite(i)->_owner)->_yvel].over_shoulder_on == 0 )
                        t->xrepeat = 0;
                    else
                    {
                        t->ang = getangle(x-t->x,y-t->y);
                        t->x = getSprite(getSprite(i)->_owner)->_x;
                        t->y = getSprite(getSprite(i)->_owner)->_y;
                        t->x += sintable[(t->ang+512)&2047]>>10;
                        t->y += sintable[t->ang&2047]>>10;
                    }
                }
                break;

            case ATOMICHEALTH:
                t->z -= (4<<8);
                break;
            case CRYSTALAMMO:
                t->shade = (sintable[(totalclock<<4)&2047]>>10);
                continue;
            case VIEWSCREEN:
            case VIEWSCREEN2:
                if(camsprite >= 0 && getHittypeTemp(OW, 0) == 1)
                {
                    t->picnum = STATIC;
                    t->cstat |= (rand()&12);
                    t->xrepeat += 8;
                    t->yrepeat += 8;
                }
                break;

            case SHRINKSPARK:
                t->picnum = SHRINKSPARK+( (totalclock>>4)&3 );
                break;
            case GROWSPARK:
                t->picnum = GROWSPARK+( (totalclock>>4)&3 );
                break;
            case RPG:
                 k = getangle(getSprite(i)->_x-x,getSprite(i)->_y-y);
                 k = (((getSprite(i)->_ang+3072+128-k)&2047)/170);
                 if(k > 6)
                 {
                    k = 12-k;
                    t->cstat |= 4;
                 }
                 else t->cstat &= ~4;
                 t->picnum = RPG+k;
                 break;

            case RECON:

                k = getangle(getSprite(i)->_x-x,getSprite(i)->_y-y);
                if( getHittypeTemp(i, 0) < 4 )
                    k = (((getSprite(i)->_ang+3072+128-k)&2047)/170);
                else k = (((getSprite(i)->_ang+3072+128-k)&2047)/170);

                if(k>6)
                {
                    k = 12-k;
                    t->cstat |= 4;
                }
                else t->cstat &= ~4;

                if( klabs(t3) > 64 ) k += 7;
                t->picnum = RECON+k;

                break;

            case APLAYER:

                p = getSprite(i)->_yvel;

                if(t->pal == 1) t->z -= (18<<8);

                if(ps[p].over_shoulder_on > 0 && ps[p].newowner < 0 )
                {
                    t->cstat |= 2;
                    if ( screenpeek == myconnectindex && numplayers >= 2 )
                    {
                        t->x = omyx+mulscale16((int32_t)(myx-omyx),smoothratio);
                        t->y = omyy+mulscale16((int32_t)(myy-omyy),smoothratio);
                        t->z = omyz+mulscale16((int32_t)(myz-omyz),smoothratio)+(40<<8);
                        t->ang = omyang+mulscale16((int32_t)(((myang+1024-omyang)&2047)-1024),smoothratio);
                        t->sectnum = mycursectnum;
                    }
                }

                if( ( display_mirror == 1 || screenpeek != p || getSprite(i)->_owner == -1 ) && ud.multimode > 1 && ud.showweapons && getSprite(ps[p].i)->_extra > 0 && ps[p].curr_weapon > 0 )
                {

#if !TEST_TSPRITE
                    memcpy((spritetype *)&tsprite[spritesortcnt],(spritetype *)t,sizeof(spritetype));
                    tsprite[spritesortcnt].statnum = 99;
#else
                    tsprite[spritesortcnt].statnum = 99;
                    fastmemcpy(&tsprite[spritesortcnt],t, sizeof(tspritetype));
                  //  copySpriteToTsprite(&tsprite[spritesortcnt],t);
#endif
                    tsprite[spritesortcnt].yrepeat = ( t->yrepeat>>3 );
                    if(t->yrepeat < 4) t->yrepeat = 4;

                    tsprite[spritesortcnt].shade = t->shade;
                    tsprite[spritesortcnt].cstat = 0;

                    switch(ps[p].curr_weapon)
                    {
                        case PISTOL_WEAPON:      tsprite[spritesortcnt].picnum = FIRSTGUNSPRITE;       break;
                        case SHOTGUN_WEAPON:     tsprite[spritesortcnt].picnum = SHOTGUNSPRITE;        break;
                        case CHAINGUN_WEAPON:    tsprite[spritesortcnt].picnum = CHAINGUNSPRITE;       break;
                        case RPG_WEAPON:         tsprite[spritesortcnt].picnum = RPGSPRITE;            break;
                        case HANDREMOTE_WEAPON:
                        case HANDBOMB_WEAPON:    tsprite[spritesortcnt].picnum = HEAVYHBOMB;           break;
                        case TRIPBOMB_WEAPON:    tsprite[spritesortcnt].picnum = TRIPBOMBSPRITE;       break;
                        case GROW_WEAPON:        tsprite[spritesortcnt].picnum = GROWSPRITEICON;       break;
                        case SHRINKER_WEAPON:    tsprite[spritesortcnt].picnum = SHRINKERSPRITE;       break;
                        case FREEZE_WEAPON:      tsprite[spritesortcnt].picnum = FREEZESPRITE;         break;
                        case DEVISTATOR_WEAPON:  tsprite[spritesortcnt].picnum = DEVISTATORSPRITE;     break;
                    }

                    if(getSprite(i)->_owner >= 0)
                        tsprite[spritesortcnt].z = ps[p].posz-(12<<8);
                    else tsprite[spritesortcnt].z = getSprite(i)->_z-(51<<8);
                    if(ps[p].curr_weapon == HANDBOMB_WEAPON)
                    {
                        tsprite[spritesortcnt].xrepeat = 10;
                        tsprite[spritesortcnt].yrepeat = 10;
                    }
                    else
                    {
                        tsprite[spritesortcnt].xrepeat = 16;
                        tsprite[spritesortcnt].yrepeat = 16;
                    }
                    tsprite[spritesortcnt].pal = 0;
                    spritesortcnt++;
                }

                if(getSprite(i)->_owner == -1)
                {
                    k = (((getSprite(i)->_ang+3072+128-a)&2047)>>8)&7;
                    if(k>4)
                    {
                        k = 8-k;
                        t->cstat |= 4;
                    }
                    else t->cstat &= ~4;

                    if(getSector(t->sectnum)->lotag == 2) k += 1795-1405;
                    else if( (getHittype(i)->floorz-getSprite(i)->_z) > (64<<8) ) k += 60;

                    t->picnum += k;
                    t->pal = ps[p].palookup;

                    goto PALONLY;
                }

                if( ps[p].on_crane == -1 && (getSector(getSprite(i)->_sectnum)->lotag&0x7ff) != 1 )
                {
                    l = getSprite(i)->_z-getHittype(ps[p].i)->floorz+(3<<8);
                    if( l > 1024 && getSprite(i)->_yrepeat > 32 && getSprite(i)->_extra > 0 )
                        setSprite_yoffset(i, (int8_t  )(l/(getSprite(i)->_yrepeat<<2)));
                    else
                        setSprite_yoffset(i, 0);
                }

                if(ps[p].newowner > -1)
                {
#if USE_ACTORSCRPTR
                    t4 = *(actorscrptr[APLAYER]+1);
                    t3 = 0;
                    t1 = *(actorscrptr[APLAYER]+2);
#else
                    t4 = *(getscriptptr(actorscridx[APLAYER]) + 1);
                    t3 = 0;
                    t1 = *(getscriptptr(actorscridx[APLAYER]) + 2);

#endif
                }

                if(ud.camerasprite == -1 && ps[p].newowner == -1)
                    if(getSprite(i)->_owner >= 0 && display_mirror == 0 && ps[p].over_shoulder_on == 0 )
                        if( ud.multimode < 2 || ( ud.multimode > 1 && p == screenpeek ) )
                {
                    t->owner = -1;
                    t->xrepeat = t->yrepeat = 0;
                    continue;
                }

                PALONLY:

                if( getSector(sect)->floorpal )
                    t->pal = getSector(sect)->floorpal;

                if(getSprite(i)->_owner == -1) continue;

                if( t->z > getHittype(i)->floorz && t->xrepeat < 32 )
                    t->z = getHittype(i)->floorz;

                break;

            case JIBS1:
            case JIBS2:
            case JIBS3:
            case JIBS4:
            case JIBS5:
            case JIBS6:
            case HEADJIB1:
            case LEGJIB1:
            case ARMJIB1:
            case LIZMANHEAD1:
            case LIZMANARM1:
            case LIZMANLEG1:
            case DUKELEG:
            case DUKEGUN:
            case DUKETORSO:
                if(ud.lockout)
                {
                    t->xrepeat = t->yrepeat = 0;
                    continue;
                }
                if(t->pal == 6) t->shade = -120;

            case SCRAP1:
            case SCRAP2:
            case SCRAP3:
            case SCRAP4:
            case SCRAP5:
            case SCRAP6:
            case SCRAP6+1:
            case SCRAP6+2:
            case SCRAP6+3:
            case SCRAP6+4:
            case SCRAP6+5:
            case SCRAP6+6:
            case SCRAP6+7:

                if(getHittype(i)->picnum == BLIMP && t->picnum == SCRAP1 && getSprite(i)->_yvel >= 0)
                    t->picnum = getSprite(i)->_yvel;
                else t->picnum += getHittypeTemp(i, 0);
                t->shade -= 6;

                if( getSector(sect)->floorpal )
                    t->pal = getSector(sect)->floorpal;
                break;

            case WATERBUBBLE:
                if(getSector(t->sectnum)->floorpicnum == FLOORSLIME)
                {
                    t->pal = 7;
                    break;
                }
            default:

                if( getSector(sect)->floorpal )
                    t->pal = getSector(sect)->floorpal;
                break;
        }
#if USE_ACTORSCRPTR
        if( actorscrptr[getSprite(i)->_picnum] )
#else
        if( getscriptptr(actorscridx[getSprite(i)->_picnum]) )
#endif
        {
            if(t4<0)
				// FIX_00093: fixed crashbugs in multiplayer (mine/blimp)
				// This is the mine issue (confusion bug in getHittypeTemp(i, 4) usage)
				// close to blimp bug (search for BLIMP)
				// -> t4 aka macro getHittypeTemp(i, 4) is incremented at DETONATEB: in actor.c
				// for a time counter. Instead we want an address.
				// Issue happens in confessn.map (do a dnclip + dnkroz + dncoords,
				// start with duke3d_w32 /m /q2 -map confessn.map)
				// go through the Guilty logo till x = -2932, y = 42174, z = 18416.
				// blow up the bomb. Wait in the water. Look at the respawn sign
				// at the bottom of the chain. Crashes when it's about to respawn.
				// Lame fix. ok for w32. Doesn't work for other plateform.
				// How to make a differene between a timer and an address??
				//
				// tanguyf: encoded script ptr are now negative.
            {
                l = *(decodescriptptr(t4) + 2);

                switch( l )
                {
                    case 2:
                        k = (((getSprite(i)->_ang+3072+128-a)&2047)>>8)&1;
                        break;

                    case 3:
                    case 4:
                        k = (((getSprite(i)->_ang+3072+128-a)&2047)>>7)&7;
                        if(k > 3)
                        {
                            t->cstat |= 4;
                            k = 7-k;
                        }
                        else t->cstat &= ~4;
                        break;

                    case 5:
                        k = getangle(getSprite(i)->_x-x,getSprite(i)->_y-y);
                        k = (((getSprite(i)->_ang+3072+128-k)&2047)>>8)&7;
                        if(k>4)
                        {
                            k = 8-k;
                            t->cstat |= 4;
                        }
                        else t->cstat &= ~4;
                        break;
                    case 7:
                        k = getangle(getSprite(i)->_x-x,getSprite(i)->_y-y);
                        k = (((getSprite(i)->_ang+3072+128-k)&2047)/170);
                        if(k>6)
                        {
                            k = 12-k;
                            t->cstat |= 4;
                        }
                        else t->cstat &= ~4;
                        break;
                    case 8:
                        k = (((getSprite(i)->_ang+3072+128-a)&2047)>>8)&7;
                        t->cstat &= ~4;
                        break;
                    default:
                        k = 0;
                        break;
                }

                t->picnum += k + (*decodescriptptr(t4)) + l * t3;

                if(l > 0)
                    while(tiles[t->picnum].dim.width == 0 && t->picnum > 0 )
                    t->picnum -= l;       //Hack, for actors

                if( getHittype(i)->dispicnum >= 0)
                    setHittype_dispicnum(i, t->picnum);
            }
            else if(display_mirror == 1)
                t->cstat |= 4;
        }

        if( getSprite(i)->_statnum == 13 || badguy(getSprite(i)) || (getSprite(i)->_picnum == APLAYER && getSprite(i)->_owner >= 0) )
            if(t->statnum != 99 && getSprite(i)->_picnum != EXPLOSION2 && getSprite(i)->_picnum != HANGLIGHT && getSprite(i)->_picnum != DOMELITE)
                if(getSprite(i)->_picnum != HOTMEAT)
        {
            if( getHittype(i)->dispicnum < 0 )
            {
                setHittype_dispicnum(i, getHittype(i)->dispicnum + 1);
                continue;
            }
            else if( ud.shadows && spritesortcnt < (MAXSPRITESONSCREEN-2))
            {
                int32_t daz,xrep,yrep;

                if( (getSector(sect)->lotag&0xff) > 2 || getSprite(i)->_statnum == 4 || getSprite(i)->_statnum == 5 || getSprite(i)->_picnum == DRONE || getSprite(i)->_picnum == COMMANDER )
                    daz = getSector(sect)->floorz;
                else
                    daz = getHittype(i)->floorz;

                if( (getSprite(i)->_z-daz) < (8<<8) )
                    if( ps[screenpeek].posz < daz )
                {


#if !TEST_TSPRITE
                    memcpy((spritetype *)&tsprite[spritesortcnt],(spritetype *)t,sizeof(spritetype));

                    tsprite[spritesortcnt].statnum = 99;
#else
                  fastmemcpy(&tsprite[spritesortcnt],t,sizeof(tspritetype));
                   // copySpriteToTsprite(&tsprite[spritesortcnt], t);
                    tsprite[spritesortcnt].statnum = 99;

#endif
                    tsprite[spritesortcnt].yrepeat = ( t->yrepeat>>3 );
                    if(t->yrepeat < 4) t->yrepeat = 4;

                    tsprite[spritesortcnt].shade = 127;
                    tsprite[spritesortcnt].cstat |= 2;

                    tsprite[spritesortcnt].z = daz;
                    // next-hack: what's this shit ?
                    xrep = tsprite[spritesortcnt].xrepeat;// - (klabs(daz-t->z)>>11);
                    tsprite[spritesortcnt].xrepeat = xrep;
                    tsprite[spritesortcnt].pal = 4;

                    yrep = tsprite[spritesortcnt].yrepeat;// - (klabs(daz-t->z)>>11);
                    tsprite[spritesortcnt].yrepeat = yrep;
                    spritesortcnt++;
                }
            }

            if( ps[screenpeek].heat_amount > 0 && ps[screenpeek].heat_on )
            {
                t->pal = 6;
                t->shade = 0;
            }
        }


        switch(getSprite(i)->_picnum)
        {
            case LASERLINE:
                if(getSector(t->sectnum)->lotag == 2) t->pal = 8;
                t->z = getSprite(getSprite(i)->_owner)->_z-(3<<8);
                if(lasermode == 2 && ps[screenpeek].heat_on == 0 )
                    t->yrepeat = 0;
            case EXPLOSION2:
            case EXPLOSION2BOT:
            case FREEZEBLAST:
            case ATOMICHEALTH:
            case FIRELASER:
            case SHRINKSPARK:
            case GROWSPARK:
            case CHAINGUN:
            case SHRINKEREXPLOSION:
            case RPG:
            case FLOORFLAME:
                if(t->picnum == EXPLOSION2)
                {
                    ps[screenpeek].visibility = -127;
                    lastvisinc = totalclock+32;
                    restorepalette = 1;
                }
                t->shade = -127;
                break;
            case FIRE:
            case FIRE2:
            case BURNING:
            case BURNING2:
                if( getSprite(getSprite(i)->_owner)->_picnum != TREE1 && getSprite(getSprite(i)->_owner)->_picnum != TREE2 )
                    t->z = getSector(t->sectnum)->floorz;
                t->shade = -127;
                break;
            case COOLEXPLOSION1:
                t->shade = -127;
                t->picnum += (getSprite(i)->_shade>>1);
                break;
            case PLAYERONWATER:

                k = (((t->ang+3072+128-a)&2047)>>8)&7;
                if(k>4)
                {
                    k = 8-k;
                    t->cstat |= 4;
                }
                else t->cstat &= ~4;

                t->picnum = getSprite(i)->_picnum+k+((getHittypeTemp(i, 0)<4)*5);
                t->shade = getSprite(getSprite(i)->_owner)->_shade;

                break;

            case WATERSPLASH2:
                t->picnum = WATERSPLASH2+t1;
                break;
            case REACTOR2:
                t->picnum = getSprite(i)->_picnum + getHittypeTemp(i, 2);
                break;
            case SHELL:
                t->picnum = getSprite(i)->_picnum+(getHittypeTemp(i, 0)&1);
            case SHOTGUNSHELL:
                t->cstat |= 12;
                if(getHittypeTemp(i, 0) > 1) t->cstat &= ~4;
                if(getHittypeTemp(i, 0) > 2) t->cstat &= ~12;
                break;
            case FRAMEEFFECT1:
			case FRAMEEFFECT1_13CON:
			if(getSprite(i)->_owner >= 0 && getSprite(getSprite(i)->_owner)->_statnum < MAXSTATUS)
                {
                    if(getSprite(getSprite(i)->_owner)->_picnum == APLAYER)
                        if(ud.camerasprite == -1)
                            if(screenpeek == getSprite(getSprite(i)->_owner)->_yvel && display_mirror == 0)
                    {
                        t->owner = -1;
                        break;
                    }
                    if( (getSprite(getSprite(i)->_owner)->_cstat&SPRITE_INVISIBLE) == 0 )
                    {
                        t->picnum = getHittype(getSprite(i)->_owner)->dispicnum;
                        t->pal = getSprite(getSprite(i)->_owner)->_pal;
                        t->shade = getSprite(getSprite(i)->_owner)->_shade;
                        t->ang = getSprite(getSprite(i)->_owner)->_ang;
                        t->cstat = 2|getSprite(getSprite(i)->_owner)->_cstat;
                    }
                }
                break;

            case CAMERA1:
            case RAT:
                k = (((t->ang+3072+128-a)&2047)>>8)&7;
                if(k>4)
                {
                    k = 8-k;
                    t->cstat |= 4;
                }
                else t->cstat &= ~4;
                t->picnum = getSprite(i)->_picnum+k;
                break;
        }

        setHittype_dispicnum(i, t->picnum);
        if(getSector(t->sectnum)->floorpicnum == MIRROR)
            t->xrepeat = t->yrepeat = 0;
    }
}



#define NUMCHEATCODES 26
const uint8_t  cheatquotes[NUMCHEATCODES][14] = {
    {"cornholio"},	// 0
    {"stuff"},		// 1
    {"scotty###"},	// 2
    {"coords"},		// 3
    {"view"},		// 4
    {"time"},		// 5
    {"unlock"},		// 6
    {"cashman"},	// 7
    {"items"},		// 8
    {"rate"},		// 9
    {"skill#"},		// 10
    {"beta"},		// 11
    {"hyper"},		// 12
    {"monsters"},	// 13
    {"<RESERVED>"},	// 14
    {"<RESERVED>"},	// 15
    {"todd"},		// 16
    {"showmap"},	// 17
    {"kroz"},		// 18
    {"allen"},		// 19
    {"clip"},		// 20
    {"weapons"},	// 21
    {"inventory"},	// 22
    {"keys"},		// 23
    {"debug"}		// 24
//    {"ending"}

};


char  cheatbuf[10];
uint8_t cheatbuflen;
void activateCheat(int cheatNumber)
{
    char tempbuf[16];
    int i, j, weapon;
    int k = cheatNumber;
    switch(k)
    {
        case 0: // cornholio
        case 18: // kroz

            ud.god = 1-ud.god;

            if(ud.god)
            { // set on
                pus = 1;
                pub = 1;
                setSprite_cstat(ps[myconnectindex].i, 257);

                setHittypeTemp(ps[myconnectindex].i, 0 ,0);
                setHittypeTemp(ps[myconnectindex].i, 1, 0);
                setHittypeTemp(ps[myconnectindex].i, 2, 0);
                setHittypeTemp(ps[myconnectindex].i, 3, 0);
                setHittypeTemp(ps[myconnectindex].i, 4, 0);
                setHittypeTemp(ps[myconnectindex].i, 5, 0);

                setSprite_hitag(ps[myconnectindex].i, 0);
                setSprite_lotag(ps[myconnectindex].i, 0);
                setSprite_pal(ps[myconnectindex].i, ps[myconnectindex].palookup);

                FTA(17,&ps[myconnectindex],1);
            }
            else // set off
            {
                ud.god = 0;
                setSprite_extra(ps[myconnectindex].i, max_player_health);
                setHittype_extra(ps[myconnectindex].i, -1);
                ps[myconnectindex].last_extra = max_player_health;
                FTA(18,&ps[myconnectindex],1);
            }

            setSprite_extra(ps[myconnectindex].i, max_player_health);
            setHittype_extra(ps[myconnectindex].i, 0);
            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();

            return;

        case 1: // stuff

            if(VOLUMEONE)
                j = 6;
            else
                j = 0;

            for ( weapon = PISTOL_WEAPON;weapon < MAX_WEAPONS-j;weapon++ )
               ps[myconnectindex].gotweapon[weapon]  = 1;

            for ( weapon = PISTOL_WEAPON;
                  weapon < (MAX_WEAPONS-j);
                  weapon++ )
                addammo( weapon, &ps[myconnectindex], max_ammo_amount[weapon] );

            ps[myconnectindex].ammo_amount[GROW_WEAPON] = 50;

            ps[myconnectindex].steroids_amount =         400;
            ps[myconnectindex].heat_amount     =        1200;
            ps[myconnectindex].boot_amount          =    200;
            ps[myconnectindex].shield_amount =           100;
            ps[myconnectindex].scuba_amount =            6400;
            ps[myconnectindex].holoduke_amount =         2400;
            ps[myconnectindex].jetpack_amount =          1600;
            ps[myconnectindex].firstaid_amount =         max_player_health;

            ps[myconnectindex].got_access =              7;
            FTA(5,&ps[myconnectindex],1);
            ps[myconnectindex].cheat_phase = 0;

            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            ps[myconnectindex].inven_icon = 1;
            return;

        case 2:  // dnscotty###
        case 10: // skill#

            if(k == 2)
            {
                short volnume,levnume;
                volnume = cheatbuf[6] - '0';
                levnume = (cheatbuf[7] - '0')*10+(cheatbuf[8]-'0');

                volnume--;
                levnume--;
                if (VOLUMEONE)
                {
                    if( volnume > 0 )
                    {
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;
                    }
                }

                if((volnume > 4)&&PLUTOPAK)
                {
                    ps[myconnectindex].cheat_phase = 0;
                    KB_FlushKeyboardQueue();
                    return;
                }
                else

                if((volnume > 3)&&!PLUTOPAK)
                {
                    ps[myconnectindex].cheat_phase = 0;
                    KB_FlushKeyboardQueue();
                    return;
                }
                else

                if(volnume == 0)
                {
                    if(levnume > 5)
                    {
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;
                    }
                }
                else
                {
                    if(levnume >= 11)
                    {
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;
                    }
                }

                ud.m_volume_number = ud.volume_number = volnume;
                ud.m_level_number = ud.level_number = levnume;

            }
            else ud.m_player_skill = ud.player_skill =
                cheatbuf[5] - '1';

            if(numplayers > 1 && myconnectindex == connecthead)
            {
                tempbuf[0] = 5;
                tempbuf[1] = ud.m_level_number;
                tempbuf[2] = ud.m_volume_number;
                tempbuf[3] = ud.m_player_skill;
                tempbuf[4] = ud.m_monsters_off;
                tempbuf[5] = ud.m_respawn_monsters;
                tempbuf[6] = ud.m_respawn_items;
                tempbuf[7] = ud.m_respawn_inventory;
                tempbuf[8] = ud.m_coop;
                tempbuf[9] = ud.m_marker;
                tempbuf[10] = ud.m_ffire;

                //for(i=connecthead;i>=0;i=connectpoint2[i])
                //    sendpacket(i,(uint8_t*)tempbuf,11);
                broadcastPacket((uint8_t*) tempbuf, 11, false);
            }
            else ps[myconnectindex].gm |= MODE_RESTART;

            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            return;

        case 3: // coords
            ps[myconnectindex].cheat_phase = 0;
            ud.coords = 1-ud.coords;
            KB_FlushKeyboardQueue();
            return;

        case 4: // view
            if( ps[myconnectindex].over_shoulder_on )
                ps[myconnectindex].over_shoulder_on = 0;
            else
            {
                ps[myconnectindex].over_shoulder_on = 1;
                cameradist = 0;
                cameraclock = totalclock;
            }
            // FTA(22,&ps[myconnectindex],1);
            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            return;

        case 5: // time
            // FTA(21,&ps[myconnectindex]);
            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            return;

        case 6: // unlock
            for(i=numsectors-1;i>=0;i--) //Unlock
            {
                j = getSector(i)->lotag;
                if(j == -1 || j == 32767) continue;
                if( (j & 0x7fff) > 2 )
                {
                    if( j&(0xffff-16384) )
                        setSector_lotag(i, getSector(i)->lotag & (0xffff-16384));
                    operatesectors(i,ps[myconnectindex].i);
                }
            }
            operateforcefields(ps[myconnectindex].i,-1);

            FTA(100,&ps[myconnectindex],1);
            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            return;

        case 7: // cashman
            ud.cashman = 1-ud.cashman;
            KB_ClearKeyDown(sc_N);
            ps[myconnectindex].cheat_phase = 0;
            return;

        case 8: // items
            ps[myconnectindex].steroids_amount =         400;
            ps[myconnectindex].heat_amount     =        1200;
            ps[myconnectindex].boot_amount          =    200;
            ps[myconnectindex].shield_amount =           100;
            ps[myconnectindex].scuba_amount =            6400;
            ps[myconnectindex].holoduke_amount =         2400;
            ps[myconnectindex].jetpack_amount =          1600;

            ps[myconnectindex].firstaid_amount =         max_player_health;
            ps[myconnectindex].got_access =              7;
            FTA(5,&ps[myconnectindex],1);
            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            return;

        case 9: // rate
            ud.tickrate ^= 1;
            vscrn(); // FIX_00056: Refresh issue w/FPS, small Weapon and custom FTA, when screen resized down
            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            return;

        case 11: // beta
            FTA(105,&ps[myconnectindex],1);
            KB_ClearKeyDown(sc_H);
            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            return;

        case 12: // hyper
            ps[myconnectindex].steroids_amount = 399;
            ps[myconnectindex].heat_amount = 1200;
            ps[myconnectindex].cheat_phase = 0;
            FTA(37,&ps[myconnectindex],1);
            KB_FlushKeyboardQueue();
            return;

        case 13: // monsters
            if(actor_tog == 3) actor_tog = 0;
            actor_tog++;
            ps[screenpeek].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            return;

        case 14: // <RESERVED>
        case 25: // ??
            ud.eog = 1;
            ps[myconnectindex].gm |= MODE_EOL;
            KB_FlushKeyboardQueue();
            return;

        case 15: // <RESERVED>
            ps[myconnectindex].gm = MODE_EOL;
            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            return;

        case 16: // todd
            FTA(99,&ps[myconnectindex],1);
            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            return;

       case 17: // showmap
            ud.showallmap = 1-ud.showallmap;
            if(ud.showallmap)
            {
                for(i=0;i<(MAXSECTORS>>3);i++)
                    show2dsector[i] = 255;
                for(i=0;i<(MAXWALLS>>3);i++)
                    show2dwall[i] = 255;
                FTA(111,&ps[myconnectindex],1);
            }
            else
            {
                for(i=0;i<(MAXSECTORS>>3);i++)
                    show2dsector[i] = 0;
                for(i=0;i<(MAXWALLS>>3);i++)
                    show2dwall[i] = 0;
                FTA(1,&ps[myconnectindex],1);
            }
            ps[myconnectindex].cheat_phase = 0;
            KB_FlushKeyboardQueue();
            return;

        case 19: // allen
            FTA(79,&ps[myconnectindex],1);
            ps[myconnectindex].cheat_phase = 0;
            KB_ClearKeyDown(sc_N);
            return;

        case 20: // clip
            ud.clipping = 1-ud.clipping;
            KB_FlushKeyboardQueue();
            ps[myconnectindex].cheat_phase = 0;
            FTA(112+ud.clipping,&ps[myconnectindex],1);
            return;

        case 21: // weapons
            if(VOLUMEONE)
                j = 6;
            else
                j = 0;

            for ( weapon = PISTOL_WEAPON;weapon < MAX_WEAPONS-j;weapon++ )
            {
                addammo( weapon, &ps[myconnectindex], max_ammo_amount[weapon] );
                ps[myconnectindex].gotweapon[weapon]  = 1;
            }

            KB_FlushKeyboardQueue();
            ps[myconnectindex].cheat_phase = 0;
            FTA(119,&ps[myconnectindex],1);
            return;

        case 22: // inventory
            KB_FlushKeyboardQueue();
            ps[myconnectindex].cheat_phase = 0;
            ps[myconnectindex].steroids_amount =         400;
            ps[myconnectindex].heat_amount     =        1200;
            ps[myconnectindex].boot_amount          =    200;
            ps[myconnectindex].shield_amount =           100;
            ps[myconnectindex].scuba_amount =            6400;
            ps[myconnectindex].holoduke_amount =         2400;
            ps[myconnectindex].jetpack_amount =          1600;
            ps[myconnectindex].firstaid_amount =         max_player_health;
            FTA(120,&ps[myconnectindex],1);
            ps[myconnectindex].cheat_phase = 0;
            return;

        case 23: // keys
            ps[myconnectindex].got_access =              7;
            KB_FlushKeyboardQueue();
            ps[myconnectindex].cheat_phase = 0;
            FTA(121,&ps[myconnectindex],1);
            return;

        case 24: // debug
            debug_on = 1-debug_on;
            KB_FlushKeyboardQueue();
            ps[myconnectindex].cheat_phase = 0;
            break;
    }
}
void checkForCheats(int ch)
{
  int k, j;
  for(k = 0;k < NUMCHEATCODES;k++)
  {
      for(j = 0;j<cheatbuflen;j++)
      {
          if( toupper(cheatbuf[j]) == toupper(cheatquotes[k][j]) || (cheatquotes[k][j] == '#' && ch >= '0' && ch <= '9') )
          {
              if( cheatquotes[k][j+1] == 0 )
              {
                  activateCheat(k);
                  return;
              }
              if(j == cheatbuflen-1) return;
          }
          else
          {
              break;
          }
      }
  }
  ps[myconnectindex].cheat_phase = 0;
  return;
}
void cheats(int enteredByConsole, char *cmd)
{
//    char tempbuf[16];
    short ch; //, i, j, k, weapon;

    if( (ps[myconnectindex].gm&MODE_TYPE) || (ps[myconnectindex].gm&MODE_MENU))
        return;

    if (enteredByConsole)
    {
        strncpy(cheatbuf, cmd, sizeof(cheatbuf)-1);
        cheatbuf[sizeof(cheatbuf)-1] = 0;
        cheatbuflen = strlen(cheatbuf);
        checkForCheats(0);
    }

    if ( ps[myconnectindex].cheat_phase == 1)
    {
       while (KB_KeyWaiting())
       {
          ch = KB_Getch();
          ch = tolower(ch);

          if( !( (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ) )
          {
             ps[myconnectindex].cheat_phase = 0;
//             FTA(46,&ps[myconnectindex]);
             return;
          }

          cheatbuf[cheatbuflen++] = ch;
          cheatbuf[cheatbuflen] = 0;

          if(cheatbuflen > 11)
          {
              ps[myconnectindex].cheat_phase = 0;
              return;
          }
#if 1
          checkForCheats(ch);
#else
          char tempbuf[16];
          for(k = 0;k < NUMCHEATCODES;k++)
          {
              for(j = 0;j<cheatbuflen;j++)
              {
                  if( cheatbuf[j] == cheatquotes[k][j] || (cheatquotes[k][j] == '#' && ch >= '0' && ch <= '9') )
                  {
                      if( cheatquotes[k][j+1] == 0 ) goto FOUNDCHEAT;
                      if(j == cheatbuflen-1) return;
                  }
                  else break;
              }
          }

          ps[myconnectindex].cheat_phase = 0;
          return;

          FOUNDCHEAT:
          {
                switch(k)
                {
                    case 0: // cornholio
                    case 18: // kroz

                        ud.god = 1-ud.god;

                        if(ud.god)
                        { // set on
                            pus = 1;
                            pub = 1;
                            setSprite_cstat(ps[myconnectindex].i, 257);

                            setHittypeTemp(ps[myconnectindex].i, 0 ,0);
                            setHittypeTemp(ps[myconnectindex].i, 1, 0);
                            setHittypeTemp(ps[myconnectindex].i, 2, 0);
                            setHittypeTemp(ps[myconnectindex].i, 3, 0);
                            setHittypeTemp(ps[myconnectindex].i, 4, 0);
                            setHittypeTemp(ps[myconnectindex].i, 5, 0);

                            setSprite_hitag(ps[myconnectindex].i, 0);
                            setSprite_lotag(ps[myconnectindex].i, 0);
                            setSprite_pal(ps[myconnectindex].i, ps[myconnectindex].palookup);

                            FTA(17,&ps[myconnectindex],1);
                        }
                        else // set off
                        {
                            ud.god = 0;
                            setSprite_extra(ps[myconnectindex].i, max_player_health);
                            setHittype_extra(ps[myconnectindex].i, -1);
                            ps[myconnectindex].last_extra = max_player_health;
                            FTA(18,&ps[myconnectindex],1);
                        }

                        setSprite_extra(ps[myconnectindex].i, max_player_health);
                        setHittype_extra(ps[myconnectindex].i, 0);
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();

                        return;

                    case 1: // stuff

						if(VOLUMEONE)
                        	j = 6;
						else
                        	j = 0;

                        for ( weapon = PISTOL_WEAPON;weapon < MAX_WEAPONS-j;weapon++ )
                           ps[myconnectindex].gotweapon[weapon]  = 1;

                        for ( weapon = PISTOL_WEAPON;
                              weapon < (MAX_WEAPONS-j);
                              weapon++ )
                            addammo( weapon, &ps[myconnectindex], max_ammo_amount[weapon] );

                        ps[myconnectindex].ammo_amount[GROW_WEAPON] = 50;

                        ps[myconnectindex].steroids_amount =         400;
                        ps[myconnectindex].heat_amount     =        1200;
                        ps[myconnectindex].boot_amount          =    200;
                        ps[myconnectindex].shield_amount =           100;
                        ps[myconnectindex].scuba_amount =            6400;
                        ps[myconnectindex].holoduke_amount =         2400;
                        ps[myconnectindex].jetpack_amount =          1600;
                        ps[myconnectindex].firstaid_amount =         max_player_health;

                        ps[myconnectindex].got_access =              7;
                        FTA(5,&ps[myconnectindex],1);
                        ps[myconnectindex].cheat_phase = 0;

                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        ps[myconnectindex].inven_icon = 1;
                        return;

                    case 2:  // dnscotty###
                    case 10: // skill#

                        if(k == 2)
                        {
                            short volnume,levnume;
                            volnume = cheatbuf[6] - '0';
                            levnume = (cheatbuf[7] - '0')*10+(cheatbuf[8]-'0');

                            volnume--;
                            levnume--;
							if (VOLUMEONE)
							{
								if( volnume > 0 )
	                            {
	                                ps[myconnectindex].cheat_phase = 0;
	                                KB_FlushKeyboardQueue();
	                                return;
	                            }
							}

                            if((volnume > 4)&&PLUTOPAK)
                            {
                                ps[myconnectindex].cheat_phase = 0;
                                KB_FlushKeyboardQueue();
                                return;
                            }
                            else

							if((volnume > 3)&&!PLUTOPAK)
                            {
                                ps[myconnectindex].cheat_phase = 0;
                                KB_FlushKeyboardQueue();
                                return;
                            }
                            else

                            if(volnume == 0)
                            {
                                if(levnume > 5)
                                {
                                    ps[myconnectindex].cheat_phase = 0;
                                    KB_FlushKeyboardQueue();
                                    return;
                                }
                            }
                            else
                            {
                                if(levnume >= 11)
                                {
                                    ps[myconnectindex].cheat_phase = 0;
                                    KB_FlushKeyboardQueue();
                                    return;
                                }
                            }

                            ud.m_volume_number = ud.volume_number = volnume;
                            ud.m_level_number = ud.level_number = levnume;

                        }
                        else ud.m_player_skill = ud.player_skill =
                            cheatbuf[5] - '1';

                        if(numplayers > 1 && myconnectindex == connecthead)
                        {
                            tempbuf[0] = 5;
                            tempbuf[1] = ud.m_level_number;
                            tempbuf[2] = ud.m_volume_number;
                            tempbuf[3] = ud.m_player_skill;
                            tempbuf[4] = ud.m_monsters_off;
                            tempbuf[5] = ud.m_respawn_monsters;
                            tempbuf[6] = ud.m_respawn_items;
                            tempbuf[7] = ud.m_respawn_inventory;
                            tempbuf[8] = ud.m_coop;
                            tempbuf[9] = ud.m_marker;
                            tempbuf[10] = ud.m_ffire;

                            //for(i=connecthead;i>=0;i=connectpoint2[i])
                            //    sendpacket(i,(uint8_t*)tempbuf,11);
                            broadcastPacket((uint8_t*) tempbuf, 11, false);
                        }
                        else ps[myconnectindex].gm |= MODE_RESTART | MODE_CHEAT ;

                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;

                    case 3: // coords
                        ps[myconnectindex].cheat_phase = 0;
                        ud.coords = 1-ud.coords;
                        KB_FlushKeyboardQueue();
                        return;

                    case 4: // view
                        if( ps[myconnectindex].over_shoulder_on )
                            ps[myconnectindex].over_shoulder_on = 0;
                        else
                        {
                            ps[myconnectindex].over_shoulder_on = 1;
                            cameradist = 0;
                            cameraclock = totalclock;
                        }
                        // FTA(22,&ps[myconnectindex],1);
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;

                    case 5: // time
                        // FTA(21,&ps[myconnectindex]);
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;

					case 6: // unlock
                        for(i=numsectors-1;i>=0;i--) //Unlock
                        {
                            j = getSector(i)->lotag;
                            if(j == -1 || j == 32767) continue;
                            if( (j & 0x7fff) > 2 )
                            {
                                if( j&(0xffff-16384) )
                                    setSector_lotag(i, getSector(i)->lotag & (0xffff-16384));
                                operatesectors(i,ps[myconnectindex].i);
                            }
                        }
                        operateforcefields(ps[myconnectindex].i,-1);

                        FTA(100,&ps[myconnectindex],1);
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;

                    case 7: // cashman
                        ud.cashman = 1-ud.cashman;
                        KB_ClearKeyDown(sc_N);
                        ps[myconnectindex].cheat_phase = 0;
                        return;

                    case 8: // items
                        ps[myconnectindex].steroids_amount =         400;
                        ps[myconnectindex].heat_amount     =        1200;
                        ps[myconnectindex].boot_amount          =    200;
                        ps[myconnectindex].shield_amount =           100;
                        ps[myconnectindex].scuba_amount =            6400;
                        ps[myconnectindex].holoduke_amount =         2400;
                        ps[myconnectindex].jetpack_amount =          1600;

                        ps[myconnectindex].firstaid_amount =         max_player_health;
                        ps[myconnectindex].got_access =              7;
                        FTA(5,&ps[myconnectindex],1);
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;

                    case 9: // rate
                        ud.tickrate ^= 1;
						vscrn(); // FIX_00056: Refresh issue w/FPS, small Weapon and custom FTA, when screen resized down
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;

                    case 11: // beta
                        FTA(105,&ps[myconnectindex],1);
                        KB_ClearKeyDown(sc_H);
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;

                    case 12: // hyper
                        ps[myconnectindex].steroids_amount = 399;
                        ps[myconnectindex].heat_amount = 1200;
                        ps[myconnectindex].cheat_phase = 0;
                        FTA(37,&ps[myconnectindex],1);
                        KB_FlushKeyboardQueue();
                        return;

                    case 13: // monsters
                        if(actor_tog == 3) actor_tog = 0;
                        actor_tog++;
                        ps[screenpeek].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;

                    case 14: // <RESERVED>
                    case 25: // ??
                        ud.eog = 1;
                        ps[myconnectindex].gm |= MODE_EOL;
                        KB_FlushKeyboardQueue();
                        return;

                    case 15: // <RESERVED>
                        ps[myconnectindex].gm = MODE_EOL;
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;

                    case 16: // todd
                        FTA(99,&ps[myconnectindex],1);
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;

                   case 17: // showmap
                        ud.showallmap = 1-ud.showallmap;
                        if(ud.showallmap)
                        {
                            for(i=0;i<(MAXSECTORS>>3);i++)
                                show2dsector[i] = 255;
                            for(i=0;i<(MAXWALLS>>3);i++)
                                show2dwall[i] = 255;
                            FTA(111,&ps[myconnectindex],1);
                        }
                        else
                        {
                            for(i=0;i<(MAXSECTORS>>3);i++)
                                show2dsector[i] = 0;
                            for(i=0;i<(MAXWALLS>>3);i++)
                                show2dwall[i] = 0;
                            FTA(1,&ps[myconnectindex],1);
                        }
                        ps[myconnectindex].cheat_phase = 0;
                        KB_FlushKeyboardQueue();
                        return;

                    case 19: // allen
                        FTA(79,&ps[myconnectindex],1);
                        ps[myconnectindex].cheat_phase = 0;
                        KB_ClearKeyDown(sc_N);
                        return;

					case 20: // clip
                        ud.clipping = 1-ud.clipping;
                        KB_FlushKeyboardQueue();
                        ps[myconnectindex].cheat_phase = 0;
                        FTA(112+ud.clipping,&ps[myconnectindex],1);
                        return;

					case 21: // weapons
						if(VOLUMEONE)
                        	j = 6;
						else
                        	j = 0;

                        for ( weapon = PISTOL_WEAPON;weapon < MAX_WEAPONS-j;weapon++ )
                        {
                            addammo( weapon, &ps[myconnectindex], max_ammo_amount[weapon] );
                            ps[myconnectindex].gotweapon[weapon]  = 1;
                        }

                        KB_FlushKeyboardQueue();
                        ps[myconnectindex].cheat_phase = 0;
                        FTA(119,&ps[myconnectindex],1);
                        return;

                    case 22: // inventory
                        KB_FlushKeyboardQueue();
                        ps[myconnectindex].cheat_phase = 0;
                        ps[myconnectindex].steroids_amount =         400;
                        ps[myconnectindex].heat_amount     =        1200;
                        ps[myconnectindex].boot_amount          =    200;
                        ps[myconnectindex].shield_amount =           100;
                        ps[myconnectindex].scuba_amount =            6400;
                        ps[myconnectindex].holoduke_amount =         2400;
                        ps[myconnectindex].jetpack_amount =          1600;
                        ps[myconnectindex].firstaid_amount =         max_player_health;
                        FTA(120,&ps[myconnectindex],1);
                        ps[myconnectindex].cheat_phase = 0;
                        return;

                    case 23: // keys
                        ps[myconnectindex].got_access =              7;
                        KB_FlushKeyboardQueue();
                        ps[myconnectindex].cheat_phase = 0;
                        FTA(121,&ps[myconnectindex],1);
                        return;

                    case 24: // debug
                        debug_on = 1-debug_on;
                        KB_FlushKeyboardQueue();
                        ps[myconnectindex].cheat_phase = 0;
                        break;
                }
             }
#endif
          }
       }

    else
    {
        if( KB_KeyPressed(sc_D) )
        {
            if( ps[myconnectindex].cheat_phase >= 0 && numplayers < 2 && ud.recstat == 0)
                ps[myconnectindex].cheat_phase = -1;
        }

        if( KB_KeyPressed(sc_N) )
        {
            if( ps[myconnectindex].cheat_phase == -1 )
            {
                if(ud.player_skill == 4)
                {
                    FTA(22,&ps[myconnectindex],1);
                    ps[myconnectindex].cheat_phase = 0;
                }
                else
                {
                    ps[myconnectindex].cheat_phase = 1;
//                    FTA(25,&ps[myconnectindex]);
                    cheatbuflen = 0;
                }
                KB_FlushKeyboardQueue();
            }
            else if(ps[myconnectindex].cheat_phase != 0)
            {
                ps[myconnectindex].cheat_phase = 0;
                KB_ClearKeyDown(sc_D);
                KB_ClearKeyDown(sc_N);
            }
        }
    }
}

void deletehittype( int n)
{
    // deallocate hittype temp data as well
#if USE_SEPARATE_HITTYPE_TEMPDATA
    freeHittype_tempdata0(n);
    freeHittype_tempdata1(n);
    freeHittype_tempdata2(n);
    freeHittype_tempdata3(n);
    freeHittype_tempdata45(n);
#else
    freeHittype_tempdata(n);
#endif // USE_SEPARATE_HITTYPE_TEMPDATA
    // deallocate hittype other data as well
    freeHittype_otherdata(n);
#if USE_RAM_HITTYPE
        freeHittype(n);
#else
    memset(&hittype[n], 0, sizeof(hittype[n]));

#endif // USE_RAM_HITTYPE
}

int32_t nonsharedtimer;
#define PRINT_SPRITESTATS 0
#if WIN32 && PRINT_SPRITESTATS
extern int maxSprites;
void printSpriteStats(void)
{
    static int frame = 0;
    frame++;
    int tmp[6] = {0};
    int spritesChanged = 0;
#if 1
    int32_t sc_floorz = 0, sc_ceilingz = 0, sc_lastvx = 0,sc_lastvy = 0,sc_bposx=0,sc_bposy = 0, sc_bposz = 0;
    static int msc_floorz = 0, msc_ceilingz = 0, msc_lastvx = 0, msc_lastvy = 0, msc_bposx=0, msc_bposy=0, msc_bposz = 0;
#else
    int sc_picnum = 0, sc_ang = 0, sc_extra= 0, sc_owner = 0;
    int sc_tempang = 0, sc_actorstayput = 0, sc_dispicnum = 0;
    int sc_timetosleep = 0;
#endif
#define check_change(fieldname) (msc_##fieldname < sc_##fieldname)
#define update_change(fieldname) (msc_#fieldname = sc_##fieldname)
#define print_change(fieldname) printf("(%s = %d) ", #fieldname, sc_##fieldname)
#define count_sc(i,fieldname) if (getHittype(i)->fieldname) sc_##fieldname++
    for (int i =0; i < MAXSPRITES; i++)
    {
#if 0

        int modified = 0;
        for (int j = 0; j < 6; j++)
        {
            if (getHittypeTempPtr(i)[j]
                || getHittype(i)->lastvx
                || getHittype(i)->lastvy)
//                || getHittype_timetosleep(i) //
 //               || getHittype(i)->dispicnum) // 400 -> 500
            {
                modified = 1;
                break;
            }
        }
        spritesChanged += modified;
#elif 1
    count_sc(i, floorz);
    count_sc(i, ceilingz);
    //count_sc(i, lastvx);
    //count_sc(i, lastvy);
    count_sc(i, bposx);
    count_sc(i, bposy);
    count_sc(i, bposz);
#elif 0
    count_sc(i, picnum);
    count_sc(i, ang);
    count_sc(i, extra);
    count_sc(i, owner);
    count_sc(i, tempang);
    count_sc(i, actorstayput);
//sc_actorstayput += getHittype(i)->actorstayput != MAXSECTORS;
    count_sc(i, dispicnum);
    count_sc(i, timetosleep);
#else
        for (int j = 0; j < 6; j++)
        {
            if (getHittypeTempPtr(i)[j])
                tmp[j]++;
        }

#endif
    }
    #if 0
    static int oldSpritesChanged = 0;
    if (oldSpritesChanged != spritesChanged)
    {
        printf("Num Spr: %d, mod %d\r\n", maxSprites, spritesChanged);
        oldSpritesChanged = spritesChanged;
    }
    #elif 0
        if (frame % 30 == 0)
    {
        print_change( floorz);
        print_change( ceilingz);
        print_change( lastvx);
        print_change( lastvy);
        print_change( bposx);
        print_change( bposy);
        print_change( bposz);
        printf("\r\n");
    }
    #elif 0
    print_change( picnum);
    print_change( ang);
    print_change( extra);
    print_change( owner);
    print_change( tempang);
    print_change( actorstayput);
    print_change( dispicnum);
    print_change( timetosleep);
    printf("\r\n");
    #elif 0
    printf("Mod: %d %d %d %d %d %d\r\n", tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);
    #endif
}
#endif
void nonsharedkeys(void)
{
    short i;//,ch;
    int32_t j;
    char text[64];
//    char tempbuf[16];
#if WIN32
#if PRINT_SPRITESTATS
    printSpriteStats();
#endif // PRINT_SPRITESTAT
#endif
    if(ud.recstat == 2)
    {
        ControlInfo noshareinfo;
        CONTROL_GetInput( &noshareinfo );
    }
#if ENABLE_SCREENSHOTS
    if( KB_KeyPressed( sc_F12 ) )
    {
        KB_ClearKeyDown( sc_F12 );
		takescreenshot();
        // FTA(103,&ps[myconnectindex]); done better in takescreenshot()
    }
#endif
    if( !ALT_IS_PRESSED && ud.overhead_on == 0)
        {
            if( ACTION( gamefunc_Enlarge_Screen ) )
            {
                CONTROL_ClearAction( gamefunc_Enlarge_Screen );
                if(ud.screen_size > 0)
                    sound(THUD);

				// FIX_00027: Added an extra small statusbar (HUD)
				if (ud.screen_size==4)
				{
					ud.extended_screen_size++;
					if(ud.extended_screen_size==2)
					{
						ud.extended_screen_size = 1;
						ud.screen_size -= 4;
					}
				}
				else
	                ud.screen_size -= 4;
                vscrn();
            }
            if( ACTION( gamefunc_Shrink_Screen ) )
            {
                CONTROL_ClearAction( gamefunc_Shrink_Screen );
                if(ud.screen_size < 64) sound(THUD);

				// FIX_00027: Added an extra small statusbar (HUD)
				if (ud.screen_size==4)
				{
					ud.extended_screen_size--;
					if(ud.extended_screen_size<0)
					{
						ud.extended_screen_size=0;
						ud.screen_size += 4;
					}
				}
				else
	                ud.screen_size += 4;
                vscrn();
            }

			if(ud.screen_size < 4)
				ud.extended_screen_size = 1;
			else if(ud.screen_size > 4)
				ud.extended_screen_size = 0;

        }

    if( ps[myconnectindex].cheat_phase == 1 || ps[myconnectindex].gm&(MODE_MENU|MODE_TYPE)) return;

    if( ACTION(gamefunc_See_Coop_View) && ( ud.coop == 1 || ud.recstat == 2) )
    {
        CONTROL_ClearAction( gamefunc_See_Coop_View );
        screenpeek = connectpoint2[screenpeek];
        if(screenpeek == -1) screenpeek = connecthead;
        restorepalette = 1;
    }

    if( ud.multimode > 1 && ACTION(gamefunc_Show_Opponents_Weapon) )
    {
        CONTROL_ClearAction(gamefunc_Show_Opponents_Weapon);
        ud.showweapons = 1-ud.showweapons;
        FTA(82-ud.showweapons,&ps[screenpeek],1);
    }

    if( ACTION(gamefunc_Toggle_Crosshair) )
    {
        CONTROL_ClearAction(gamefunc_Toggle_Crosshair);
        ud.crosshair = 1-ud.crosshair;
        FTA(21-ud.crosshair,&ps[screenpeek],1);
    }

    if(ud.overhead_on && ACTION(gamefunc_Map_Follow_Mode) )
    {
        CONTROL_ClearAction(gamefunc_Map_Follow_Mode);
        ud.scrollmode = 1-ud.scrollmode;
        if(ud.scrollmode)
        {
            ud.folx = ps[screenpeek].oposx;
            ud.foly = ps[screenpeek].oposy;
            ud.fola = ps[screenpeek].oang;
        }
        FTA(83+ud.scrollmode,&ps[myconnectindex],1);
    }

    if( SHIFTS_IS_PRESSED || ALT_IS_PRESSED )
    {
        i = 0;
        if( KB_KeyPressed( sc_F1) ) { KB_ClearKeyDown(sc_F1);i = 1; }
        if( KB_KeyPressed( sc_F2) ) { KB_ClearKeyDown(sc_F2);i = 2; }
        if( KB_KeyPressed( sc_F3) ) { KB_ClearKeyDown(sc_F3);i = 3; }
        if( KB_KeyPressed( sc_F4) ) { KB_ClearKeyDown(sc_F4);i = 4; }
        if( KB_KeyPressed( sc_F5) ) { KB_ClearKeyDown(sc_F5);i = 5; }
        if( KB_KeyPressed( sc_F6) ) { KB_ClearKeyDown(sc_F6);i = 6; }
        if( KB_KeyPressed( sc_F7) ) { KB_ClearKeyDown(sc_F7);i = 7; }
        if( KB_KeyPressed( sc_F8) ) { KB_ClearKeyDown(sc_F8);i = 8; }
        if( KB_KeyPressed( sc_F9) ) { KB_ClearKeyDown(sc_F9);i = 9; }
        if( KB_KeyPressed( sc_F10) ) {KB_ClearKeyDown(sc_F10);i = 10; }

        if(i)
        {
            if(SHIFTS_IS_PRESSED)
            {
                if(i == 5 && ps[myconnectindex].fta > 0 && ps[myconnectindex].ftq == 26)
                {
                    music_select++;

					// FIX_00065: Music cycling with F5 and SHIFT-F5 messed up
					if(VOLUMEALL) // Then its 1.3d reg
					{
						if(music_select == 33) music_select = 0;
					}
					else if (VOLUMEONE)
					{
						if(music_select == 6) music_select = 0;
					}
					else // assume 1.5 or plutopak
					{
						if(music_select == 44) music_select = 0;
					}

                    strcpy(text,"PLAYING ");
                    strcat(text,&music_fn[0][music_select][0]);
					MUSIC_StopSong(); // FIX_00074: Shift f5 doesn't change hi-res tunes, but only midi tunes.
                    playmusic(&music_fn[0][music_select][0]);
#if RAM_FTA_QUOTES
                    strcpy(&fta_quotes[26][0],text);
                    FTA(26,&ps[myconnectindex],1);
#else
                    strcpy(get_fta_quotes(CUSTOM_FTA),text);
                    FTA(CUSTOM_FTA,&ps[myconnectindex],1);
#endif
                    return;
                }
#if LETS_WASTE_RAM_FOR_USERDEFS
                adduserquote(ud.ridecule[i-1]);
                ch = 0;

                tempbuf[ch] = 4;
                tempbuf[ch+1] = 0;
                strcat((char*)tempbuf+1,ud.ridecule[i-1]);

                i = 1+strlen(ud.ridecule[i-1]);

                if(ud.multimode > 1)
//                    for(ch=connecthead;ch>=0;ch=connectpoint2[ch])
//                        if (ch != myconnectindex)
//                            sendpacket(ch,tempbuf,i);
                    broadcastPacket(tempbuf, i, true);
#endif


                pus = NUMPAGES;
                pub = NUMPAGES;

                return;

            }
#if USE_RTS
            if(ud.lockout == 0)
                if(SoundToggle && ALT_IS_PRESSED && ( RTS_NumSounds() > 0 ) && rtsplaying == 0 && VoiceToggle )
            {
                rtsptr = RTS_GetSound (i-1);
                if(*rtsptr == 'C')
                    FX_PlayVOC3D( rtsptr,0,0,0,255,-i);
                else FX_PlayWAV3D( rtsptr,0,0,0,255,-i);

                rtsplaying = 7;

                if(ud.multimode > 1)
                {
                    tempbuf[0] = 7;
                    tempbuf[1] = i;

//                    for(ch=connecthead;ch>=0;ch=connectpoint2[ch])
//                        if(ch != myconnectindex)
//                            sendpacket(ch,(uint8_t*)tempbuf,2);
                      broadcastPacket(tempbuf, 2, true);
                }

                pus = NUMPAGES;
                pub = NUMPAGES;

                return;
            }
#endif
        }
    }

    if(!ALT_IS_PRESSED && !SHIFTS_IS_PRESSED)
    {

        if( ud.multimode > 1 && ACTION(gamefunc_SendMessage) )
        {
            KB_FlushKeyboardQueue();
            CONTROL_ClearAction( gamefunc_SendMessage );
            ps[myconnectindex].gm |= MODE_TYPE;
            typebuf[0] = 0;
            inputloc = 0;
        }

        if( KB_KeyPressed(sc_F1) || ( ud.show_help && ( KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) ) ) )
        {
            KB_ClearKeyDown(sc_F1);
            KB_ClearKeyDown(sc_Space);
            KB_ClearKeyDown(sc_kpad_Enter);
            KB_ClearKeyDown(sc_Enter);
            ud.show_help ++;

            if( ud.show_help > 2 )
            {
                ud.show_help = 0;
                if(ud.multimode < 2 && ud.recstat != 2) ready2send = 1;
                vscrn();
            }
            else
            {
                setview(0,0,xdim-1,ydim-1);
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 0;
                    totalclock = ototalclock;
                }
            }
        }

//        if(ud.multimode < 2)
        {
            if(ud.recstat != 2 && KB_KeyPressed( sc_F2 ) )
            {
                KB_ClearKeyDown( sc_F2 );
#if ENABLE_SCREENSHOT

                if(movesperpacket == 4 && connecthead != myconnectindex)
                    return;

                FAKE_F2:
                if(getSprite(ps[myconnectindex].i)->_extra <= 0)
                {
                    FTA(118,&ps[myconnectindex],1);
                    return;
                }
                cmenu(MENU_PRE_SAVE_GAME);
                screencapt = 1;
                displayrooms(myconnectindex,65536);
//                savetemp("duke3d.tmp",tiles[MAXTILES-1].data,160*100);
                screencapt = 0;
                FX_StopAllSounds();
                clearsoundlocks();

//                setview(0,0,xdim-1,ydim-1);
                ps[myconnectindex].gm |= MODE_MENU;

                if(ud.multimode < 2)
                {
                    ready2send = 0;
                    totalclock = ototalclock;
                    screenpeek = myconnectindex;
                }
#endif
            }

            if(KB_KeyPressed( sc_F3 ))
            {
                KB_ClearKeyDown( sc_F3 );

                if(movesperpacket == 4 && connecthead != myconnectindex)
                    return;

                cmenu(MENU_LOAD_GAME);
                FX_StopAllSounds();
                clearsoundlocks();

//                setview(0,0,xdim-1,ydim-1);
                ps[myconnectindex].gm |= MODE_MENU;
                if(ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 0;
                    totalclock = ototalclock;
                }
                screenpeek = myconnectindex;
            }
        }

        if(KB_KeyPressed( sc_F4 ) && FXDevice != SC_Unknown )
        {
            KB_ClearKeyDown( sc_F4 );
            FX_StopAllSounds();
            clearsoundlocks();

            ps[myconnectindex].gm |= MODE_MENU;
            if(ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
            cmenu(MENU_SOUND_SETUP);

        }

        if( KB_KeyPressed( sc_F6 ) && (ps[myconnectindex].gm&MODE_GAME))
        {
            KB_ClearKeyDown( sc_F6 );
#if ENABLE_SCREENSHOT
            if(movesperpacket == 4 && connecthead != myconnectindex)
                return;

            if(lastsavedpos == -1) goto FAKE_F2;

            KB_FlushKeyboardQueue();

            if(getSprite(ps[myconnectindex].i)->_extra <= 0)
            {
                FTA(118,&ps[myconnectindex],1);
                return;
            }
            screencapt = 1;
            displayrooms(myconnectindex,65536);
//            savetemp("duke3d.tmp",tiles[MAXTILES-1].data,160*100);
            screencapt = 0;
            if( lastsavedpos >= 0 )
            {
                inputloc = strlen(&ud.savegame[lastsavedpos][0]);
                current_menu = MENU_ENTER_SAVE_GAME_NAME0+lastsavedpos;
                probey = lastsavedpos;
            }
            FX_StopAllSounds();
            clearsoundlocks();

            setview(0,0,xdim-1,ydim-1);
            ps[myconnectindex].gm |= MODE_MENU;
            if(ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
#endif
        }

        if(KB_KeyPressed( sc_F7 ) )
        {
            KB_ClearKeyDown(sc_F7);
            if( ps[myconnectindex].over_shoulder_on )
                ps[myconnectindex].over_shoulder_on = 0;
            else
            {
                ps[myconnectindex].over_shoulder_on = 1;
                cameradist = 0;
                cameraclock = totalclock;
            }
            FTA(109+ps[myconnectindex].over_shoulder_on,&ps[myconnectindex],1);
        }
#if 0
    // we can't modify.
        if( KB_KeyPressed( sc_F5 ) && MusicDevice != SC_Unknown )
        {
            KB_ClearKeyDown( sc_F5 );
            strcpy(text,&music_fn[0][music_select][0]);
            strcat(text,".  USE SHIFT-F5 TO CHANGE.");
            strcpy(fta_quotes[26],text);
            FTA(26,&ps[myconnectindex],1);

        }
#endif
        if(KB_KeyPressed( sc_F8 ))
        {
            KB_ClearKeyDown( sc_F8 );
            ud.fta_on = !ud.fta_on;
            FTA(24-ud.fta_on,&ps[myconnectindex],1);
        }

        if(KB_KeyPressed( sc_F9 ) && (ps[myconnectindex].gm&MODE_GAME) )
        {
            KB_ClearKeyDown( sc_F9 );

            if(movesperpacket == 4 && myconnectindex != connecthead)
                return;

            if( lastsavedpos >= 0 ) cmenu(MENU_LOAD_LAST_GAME_QUESTION1);
            else cmenu(MENU_MESSAGE_SELECT_SAVE_SPOT);
            FX_StopAllSounds();
            clearsoundlocks();
            ps[myconnectindex].gm |= MODE_MENU;
            if(ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
        }

        if(KB_KeyPressed( sc_F10 ))
        {
            KB_ClearKeyDown( sc_F10 );
			cmenu(MENU_KEYBOARD_QUIT);
            FX_StopAllSounds();
            clearsoundlocks();
            ps[myconnectindex].gm |= MODE_MENU;
            if(ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
        }


        if( ud.overhead_on != 0)
        {

            j = totalclock-nonsharedtimer; nonsharedtimer += j;
            if ( ACTION( gamefunc_Enlarge_Screen ) )
                ps[myconnectindex].zoom += mulscale6(j,max(ps[myconnectindex].zoom,256));
            if ( ACTION( gamefunc_Shrink_Screen ) )
                ps[myconnectindex].zoom -= mulscale6(j,max(ps[myconnectindex].zoom,256));

            if( (ps[myconnectindex].zoom > 2048) )
                ps[myconnectindex].zoom = 2048;
            if( (ps[myconnectindex].zoom < 48) )
                ps[myconnectindex].zoom = 48;

        }
    }

    if( KB_KeyPressed(sc_Escape) && ud.overhead_on && ps[myconnectindex].newowner == -1 )
    {
        KB_ClearKeyDown( sc_Escape );
        ud.last_overhead = ud.overhead_on;
        ud.overhead_on = 0;
        ud.scrollmode = 0;
        vscrn();
    }

    if( ACTION(gamefunc_AutoRun) )
    {
        CONTROL_ClearAction(gamefunc_AutoRun);
        ud.auto_run = 1-ud.auto_run;
        FTA(85+ud.auto_run,&ps[myconnectindex],1);
    }

    if( ACTION(gamefunc_Map) )
    {
        CONTROL_ClearAction( gamefunc_Map );
        if( ud.last_overhead != ud.overhead_on && ud.last_overhead)
        {
            ud.overhead_on = ud.last_overhead;
            ud.last_overhead = 0;
        }
        else
        {
            ud.overhead_on++;
            if(ud.overhead_on == 3 ) ud.overhead_on = 0;
            ud.last_overhead = ud.overhead_on;
        }
        restorepalette = 1;
        vscrn();
    }

    if(KB_KeyPressed( sc_F11 ))
    {
        KB_ClearKeyDown( sc_F11 );
		// FIX_00030: Brightness step was not the same from the keys vs menu
        if(SHIFTS_IS_PRESSED) ud.brightness-=8; // Keyboard step must be 8, as the brightness cursor step.
        else ud.brightness+=8;

        if (ud.brightness > 56 )
            ud.brightness = 0;
        else if(ud.brightness < 0)
            ud.brightness = 56;

        setbrightness(ud.brightness>>2,&ps[myconnectindex].palette[0]);
        if(ud.brightness < 40) FTA( 29 + (ud.brightness>>3) ,&ps[myconnectindex],1);
        else if(ud.brightness < 80) FTA( 96 + (ud.brightness>>3) - 5,&ps[myconnectindex],1);
    }
}


#if WIN32
void comlinehelp(char  **argv)
{

    printf("Command line help.  %s [/flags...]\n",argv[0]);
    puts(" ?, /?         This help message");
    puts(" /l##          Level (1-11)");
    puts(" /v#           Volume (1-4)");
    puts(" /s#           Skill (1-4)");
    puts(" /r            Record demo");
    puts(" /dFILE        Start to play demo FILE");
    puts(" /m            No monsters");
    puts(" /ns           No sound");
    puts(" /nm           No music");
    puts(" /t#           Respawn, 1 = Monsters, 2 = Items, 3 = Inventory, x = All");
    puts(" /c#           MP mode, 1 = DukeMatch(spawn), 2 = Coop, 3 = Dukematch(no spawn)");
    puts(" /q#           Fake multiplayer (2-8 players)");
    puts(" /a            Use player AI (fake multiplayer only)");
    puts(" /i#           Network mode (1/0) (multiplayer only) (default == 1)");
    puts(" /f#           Send fewer packets (1, 2, 4) (multiplayer only)");
    puts(" /gFILE, /g... Use multiple group files (must be last on command line)");
    puts(" /xFILE        Compile FILE (default GAME.CON)");
    puts(" /u#########   User's favorite weapon order (default: 3425689071)");
    puts(" /#            Load and run a game (slot 0-9)");
    puts(" /z            Skip memory check");
    puts(" -map FILE     Use a map FILE");
    puts(" -name NAME    Foward NAME");
    puts(" -net          Net mode game");
    printf("\n");
}
#endif
#if WIN32
void checkcommandline(int argc,char  **argv)
{
    #if NOT_YET_IMPLEMENTED
    short i, j;
    char  *c;
	char  kbdKey;

    ud.fta_on = 1;
    ud.god = 0;
    ud.m_respawn_items = 0;
    ud.m_respawn_monsters = 0;
    ud.m_respawn_inventory = 0;
    ud.warp_on = 0;
    ud.cashman = 0;
    ud.m_player_skill = ud.player_skill = 2;
	ud.multimode_bot = 0;

#ifdef BETA
    return;
#endif

	printf("Commands: ");
	i=1;
	while(i < argc)
	{
   		printf("%s ",argv[i]);
   		i++;
	}
	printf("\n");

	i = 1;

    if(argc > 1)
    {
        while(i < argc)
        {
            c = argv[i];

			if (stricmp(c, "-map") == 0)
            {
				i++;
				strcpy(boardfilename,argv[i]);
				if( strchr(boardfilename,'.') == 0)
					strcat(boardfilename,".map");
				printf("Using level: '%s'.\n",boardfilename);
				continue;

			}

			if (stricmp(c, "-net") == 0)
            {
                i += 2;  // skip filename.
				// FIX_00044: Markers are now on by default in netgames (as real DOS duke3d)
				ud.m_marker = ud.marker = 1;
                continue;
            }

            if (stricmp(c, "-game_dir") == 0)
            {
				// Get the file name
				i++;
				c = argv[i];
				setGameDir(c);

				// skip over the file name now that we have it.
				i++;

                continue;
            }

            if (stricmp(c, "-stun") == 0)
            {
				g_bStun = 1;

				i++;
                continue;
            }

            if (stricmp(c, "/disableautoaim") == 0)
            {

				printf(	"\nThe Host used the /disableautoaim option to turn your Weapon AutoAim OFF\n"
						"Playing without AutoAim is usually extremely difficult and can make boring\n"
						"games ruining Duke's playability. Duke3D was not designed to play with\n"
						"AutoAim OFF like in modern FPS.\n\n"
						"Do you authorize the HOST to turn your AutoAim OFF (Y/N)? ");

				do
					kbdKey = getch()|' ';
				while(kbdKey != 'n' && kbdKey != 'y');

				printf("%c \n", kbdKey);

				if(kbdKey == 'y')
					nHostForceDisableAutoaim = 1;
				else
					nHostForceDisableAutoaim = 2;

				i++;
                continue;
            }

            if(*c == '?')
            {
                comlinehelp(argv);
                Error(EXIT_SUCCESS, "");
            }

            if(*c == '/')
            {
                c++;
                switch(*c)
                {
                    case 'x':
                    case 'X':
                        c++;
                        if(*c)
                        {
							if(getGameDir()[0] != '\0'){
								sprintf(confilename, "%s\\%s", getGameDir(), c);
							}
							else{
								strcpy(confilename,c);
							}

                            if(SafeFileExists(confilename) == 0){
                                Error(EXIT_SUCCESS, "Could not find con file '%s'.\n",confilename);
                            }
                            else printf("Using con file: '%s'\n",confilename);
                        }
                        break;
                    case 'g':
                    case 'G':
                        c++;
                        if(*c){
							char  fullpathgrpfile[16]; // 16 not enough
							memset(fullpathgrpfile, 0, 16);

                            if( strchr(c,'.') == 0){
								strcat(c,".grp"); // crap!
							}

							if(getGameDir()[0] != '\0'){
								sprintf(fullpathgrpfile, "%s\\%s", getGameDir(), c);
							}
							else{
								strcpy(fullpathgrpfile, c);
							}

                            j = initgroupfile(fullpathgrpfile);
                            if( j == -1 )
                                printf("Could not find group file %s.\n",fullpathgrpfile);
                        }

                        break;
                    case 'a':
                    case 'A':
                        ud.playerai = 1;
                        puts("Other player AI.");
                        break;
                    case 'n':
                    case 'N':
                        c++;
                        if(*c == 's' || *c == 'S'){
                            CommandSoundToggleOff = 2;
                            puts("Sound off.");
                        }
                        else
                            if(*c == 'm' || *c == 'M'){
                            CommandMusicToggleOff = 1;
                            puts("Music off.");
                            }
                            else{
                                comlinehelp(argv);
                                Error(EXIT_SUCCESS, "");
                            }
                        break;
                    case 'i':
                    case 'I':
                        c++;
                        if(*c == '0')
                            networkmode = 0;
                        if(*c == '1')
                            networkmode = 1;
                        printf("Network Mode %d\n",networkmode);
                        break;
                    case 'c':
                    case 'C':
                        c++;
                        if(*c == '1' || *c == '2' || *c == '3' )  // C1 = DM; C2 =COOP
                            ud.m_coop = *c - '0' - 1; // 0 = DM   1 = COOP
                        else ud.m_coop = 0;

                        switch(ud.m_coop)
                        {
                            case 0:
                                puts("Dukematch (spawn).");
                                break;
                            case 1:
                                puts("Cooperative play.");
                                break;
                            case 2:
                                puts("Dukematch (no spawn).");
                                break;
                        }

                        break;
                    case 'z':
                    case 'Z':
                        memorycheckoveride = 1;
                        break;
                    case 'f':
                    case 'F':
                        c++;
                        if(*c == '1')
                            movesperpacket = 1;
                        if(*c == '2')
                            movesperpacket = 2;
                        if(*c == '4')
                        {
                            movesperpacket = 4;
                            setpackettimeout(0x3fffffff,0x3fffffff);
                        }
                        break;
                    case 't':
                    case 'T':
                        c++;
                        if(*c == '1') ud.m_respawn_monsters = 1;
                        else if(*c == '2') ud.m_respawn_items = 1;
                        else if(*c == '3') ud.m_respawn_inventory = 1;
                        else
                        {
                            ud.m_respawn_monsters = 1;
                            ud.m_respawn_items = 1;
                            ud.m_respawn_inventory = 1;
                        }
                        puts("Respawn on.");
                        break;
                    case 'm':
                    case 'M':
                        if( *(c+1) != 'a' && *(c+1) != 'A' )
                        {
                            ud.m_monsters_off = 1;
                            ud.m_player_skill = ud.player_skill = 0;
                            puts("Monsters off.");
                        }
                        break;
                    case 'w':
                    case 'W':
                        ud.coords = 1;
                        break;
                    case 'q':
                    case 'Q':
                        puts("Fake multiplayer mode.");
                        if( *(++c) == 0) ud.multimode_bot = 1;
                        else ud.multimode_bot = atol(c)%17;
						ud.multimode = ud.multimode_bot;
                        ud.m_coop = ud.coop = 0;
                        ud.m_marker = ud.marker = 1;
                        ud.m_respawn_monsters = ud.respawn_monsters = 1;
                        ud.m_respawn_items = ud.respawn_items = 1;
                        ud.m_respawn_inventory = ud.respawn_inventory = 1;

                        break;
                    case 'r':
                    case 'R':
                        ud.m_recstat = 1;
                        puts("Demo record mode on.");
                        break;
                    case 'd':
                    case 'D':
                        c++;
                        if( strchr(c,'.') == 0)
                            strcat(c,".dmo");
                        printf("Play demo %s.\n",c);
                        strcpy(firstdemofile,c);
                        break;
                    case 'l':
                    case 'L':
                        ud.warp_on = 1;
                        c++;
                        ud.m_level_number = ud.level_number = (atol(c)-1)%11;
                        break;
                    case 'j':
                    case 'J':
                        Error(EXIT_SUCCESS, "This port has a auto adaptive version system. All versions supported, simply change your duke3d.grp\n");
						break;

                    case 'v':
                    case 'V':
                        c++;
                        ud.warp_on = 1;
                        ud.m_volume_number = ud.volume_number = atol(c)-1;
                        break;
                    case 's':
                    case 'S':
                        c++;
                        ud.m_player_skill = ud.player_skill = (atol(c)%5);
                        if(ud.m_player_skill == 4)
                            ud.m_respawn_monsters = ud.respawn_monsters = 1;
                        break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        ud.warp_on = 2 + (*c) - '0';
                        break;
                    case 'u':
                    case 'U':
                        c++;
                        j = 0;
                        if(*c)
                        {
                            puts("Using favorite weapon order(s).");
                            while(*c)
                            {
                                ud.mywchoice[j] = *c-'0';
                                c++;
                                j++;
                            }
                            while(j < 10)
                            {
                                if(j == 9)
                                    ud.mywchoice[9] = 1;
                                else
                                    ud.mywchoice[j] = 2;

                                j++;
                            }
                        }
                        else
                        {
                            puts("Using default weapon orders.");
                        }

                        break;

                    case '?': // Show help
					default:

						comlinehelp(argv);
                        Error(EXIT_SUCCESS, "");
						break;
                }
            }
            i++;
        }
    }
    #endif
}
#endif


#if 0
void printstr(short x, short y, uint8_t  string[81], uint8_t  attribute)
{
        uint8_t  character;
        short i, pos;

        pos = (y*80+x)<<1;
        i = 0;
        while (string[i] != 0)
        {
                character = string[i];
                printchrasm(0xb8000+(int32_t)pos,1L,(((int32_t)attribute<<8)+(int32_t)character));
                i++;
                pos+=2;
        }
}
#endif
void Logo(void)
{
    short i,soundanm;

    soundanm = 0;

    ready2send = 0;

    KB_FlushKeyboardQueue();

    setview(0,0,xdim-1,ydim-1);
    clearview(0L);
    palto(0,0,0,63,false);

    flushperms();
    nextpage(SCREEN_HEIGHT);
    waitForUpdate(-1);
    MUSIC_StopSong();

	if(ud.showcinematics && numplayers < 2)
	{
		////This plays the explosion from the nuclear sign at the beginning.
		if(!VOLUMEONE)
		{
		    if(!KB_KeyWaiting() && nomorelogohack == 0)
		    {
		        getpackets();

		        playanm("logo.anm",5);
		        palto(0,0,0,63,false);
		        KB_FlushKeyboardQueue();
		    }

		    clearview(0L);
		    nextpage(SCREEN_HEIGHT);
		    waitForUpdate(-1);
		}

		//MIDI start here
		playmusic(&env_music_fn[0][0]);

		//"REALITY IS OUR GAME" Screen
	    for(i=0;i<64;i+=7)
	    {
			palto(0,0,0,i,true);
	    }
		ps[myconnectindex].palette = drealms;
	    palto(0,0,0,63,false);
	    rotatesprite(0,0,65536L,0,DREALMS,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
	    nextpage(SCREEN_HEIGHT);
	    waitForUpdate(-1);
		for(i=63;i>0;i-=7)
		{
			palto(0,0,0,i,true);
		}


	    totalclock = 0;
	    while( totalclock < (120*7) && !KB_KeyWaiting() )
	    {
#if MG24
	      keyhandler();
#endif
	        getpackets();
	        updateAudio();

	    }




	    for(i=0;i<64;i+=7)
			palto(0,0,0,i,true);
	    clearview(0L);
	    nextpage(SCREEN_HEIGHT);
	    waitForUpdate(-1);
	    ps[myconnectindex].palette = titlepal;
	    flushperms();
	    rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);
	    KB_FlushKeyboardQueue();
	    nextpage(SCREEN_HEIGHT);
	    waitForUpdate(-1);
	    for(i=63;i>0;i-=7)
			palto(0,0,0,i,true);

	    totalclock = 0;

		//Animate screen (Duke picture wiht "DUKE" "NUKEM 3D" coming from far away and hitting the screen"
	    while(totalclock < (860+120) && !KB_KeyWaiting())
	    {
	        rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);

	        if( totalclock > 120 && totalclock < (120+60) )
	        {
	            if(soundanm == 0)
	            {
	                soundanm = 1;
	                sound(PIPEBOMB_EXPLODE);
	            }
	            rotatesprite(160<<16,104<<16,(totalclock-120)<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
	        }
	        else if( totalclock >= (120+60) )
	            rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);

	        if( totalclock > 220 && totalclock < (220+30) )
	        {
	            if( soundanm == 1)
	            {
	                soundanm = 2;
	                sound(PIPEBOMB_EXPLODE);
	            }

	            rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
	            rotatesprite(160<<16,(129)<<16,(totalclock - 220 )<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
	        }
	        else if( totalclock >= (220+30) )
	            rotatesprite(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);

			if(PLUTOPAK) // FIX_00064: Cinematics explosions were not right for 1.3/1.3d grp.
			{

				if( totalclock >= 280 && totalclock < 395 )
				{
					rotatesprite(160<<16,(151)<<16,(410-totalclock)<<12,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);
					if(soundanm == 2)
					{
						soundanm = 3;
						sound(FLY_BY);
					}
				}
				else if( totalclock >= 395 )
				{
					if(soundanm == 3)
					{
						soundanm = 4;
						sound(PIPEBOMB_EXPLODE);
					}
					rotatesprite(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);
				}
			}

	        getpackets();
	        updateAudio();

	        nextpage(SCREEN_HEIGHT);
	        waitForUpdate(-1);
	    }
		// FIX_00077: Menu goes directly to the "NEW GAME" sub-menu when starting new game (Turrican)
		KB_FlushKeyboardQueue();
	}
	else if(numplayers > 1)
    {
		// FIX_00079: "waiting player" screen not showing up (black screen)
		playmusic(&env_music_fn[0][0]);

	    ps[myconnectindex].palette = titlepal;
	    for(i=63;i>0;i-=7) palto(0,0,0,i,true);

        rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);

        gametext(160,190,"WAITING FOR PLAYERS",14,2);
        nextpage(SCREEN_HEIGHT);
        waitForUpdate(-1);
    }
	else
	{
		// FIX_00091: Main theme starting too early (Bryzian/Turrican)
		playmusic(&env_music_fn[0][0]);
	}

    waitforeverybody();

    flushperms();
    clearview(0L);
    nextpage(SCREEN_HEIGHT);
    waitForUpdate(-1);

    ps[myconnectindex].palette = palette;
    sound(NITEVISION_ONOFF);

    palto(0,0,0,0,false);
    clearview(0L);
}


/*
===================
=
= ShutDown
=
===================
*/

void ShutDown( void )
{
    SoundShutdown();
    MusicShutdown();
    uninittimer();
    uninitengine();
    CONTROL_Shutdown();
    CONFIG_WriteSetup();
    KB_Shutdown();
    CONSOLE_Term();
}


/*
===================
=
= Startup
=
===================
*/
void printCstring(char *str)
{
    while(*str > 0)
    {
        if (*str == '\r')
        {
            printf("\\r");
        }
        else  if (*str == '\n')
        {
            printf("\\n");
        }
        else  if (*str == '\"')
        {
            printf("\\\"");
        }
        else
            printf("%c", *str);
        str++;
    }
}
void compilecons(void)
{
    #define PRINT_CON_FILES 0
    #if PRINT_CON_FILES
    int size;
    char *p;
    //p = kGetExtMemAddress("USER.CON", &size);
    //printf("%s", p);
    //FIXME("------------------END OF USER CON");
    //p = kGetExtMemAddress("GAME.CON", &size);
    //printf("%s", p);
    //FIXME("------------------END OF GAME CON");
    p = kGetExtMemAddress("DEFS.CON", &size);
    printf("%s", p);
    FIXME("------------------END OF DEFS CON");

    EXIT(0);
    #endif // PRINT_CON_FILES
#if LOAD_FROM_CON_FILE

    char  userconfilename[512];
#define LET_US_DO_LIKE_NORMAL_PEOPLE 1
#if LET_US_DO_LIKE_NORMAL_PEOPLE
    char mymembuf_stack[262144], labelcode_stack[262144], label_stack[262144];
    mymembuf = mymembuf_stack;
    labelcode = (int32_t*)labelcode_stack;
    label = (char*)label_stack;
#else
// next-hack: this is utterly shit.

    mymembuf = (char*)hittype;
    labelcode = (int32_t*)&sector[0];
    label = (char*)sprite;


#endif // LET_US_DO_LIKE_NORMAL_PEOPLE
    sprintf(userconfilename, "%s", confilename);

    loadefs(userconfilename, mymembuf, 0);

    printf("Code Size:%d bytes(%d labels).\n",(int32_t)((scriptptr-script)<<2)-4,labelcnt);
    int size = (scriptptr-script);
    printf("#define MAXSCRIPTSIZE %d\r\n", size);
    printf("#define LABEL_CNT %d\r\n", labelcnt);
    printf("const int32_t script [MAXSCRIPTSIZE] = \r\n{\r\n    ");
    for (int i = 0; i < size; i++)
    {
        printf("0x%08X, ", script[i]);
        if (i % MAX_WORDS_PER_LINE == MAX_WORDS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    // char  music_fn[4][11][13];
    printf("const char  music_fn[4][11][13] =\r\n{\r\n");
    for (int i = 0; i < 4; i++)
    {
        printf("    {\r\n        ");
        for (int j = 0; j < 11; j++)
        {
            printf("\"%s\", ", music_fn[i][j]);
        }
        printf("\r\n    },\r\n");
    }
    printf("};\r\n");
    // char  env_music_fn[4][13];
    printf("const char  env_music_fn[4][13] =\r\n{\r\n");
    for (int i = 0; i < 4; i++)
    {
            printf("\"%s\", ", env_music_fn[i]);
    }
    printf("\r\n};\r\n");
    #warning to be reduced.

    //int16_t  actorsscridx[MAXTILES]
    printf("const int16_t  actorscridx[MAXTILES] =\r\n{\r\n");
    for (int i = 0; i < MAXTILES; i++)
    {
        printf("0x%04X, ", actorscridx[i]);
        if (i % MAX_SHORTS_PER_LINE == MAX_SHORTS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    // uint8_t  actortype[MAXTILES];

    printf("const uint8_t  actortype[MAXTILES] =\r\n{\r\n");
    for (int i = 0; i < MAXTILES; i++)
    {
        printf("0x%02X, ", actortype[i]);
        if (i % MAX_BYTES_PER_LINE == MAX_BYTES_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    //

    //
    // skill_names: NOT NEEDED, ALREADY PRESENT (unless we use > 1.3)
    // volume_names: NOT NEEDED, ALREADY PRESENT (unless we use >1.3. Todo anyway.
    // betaname  // not used.

    // char level_names[44][33];
    printf("const char level_names[44][33] =\r\n{\r\n");
    for (int i = 0; i < 44; i++)
    {
            printf("    \"%s\",\r\n", level_names[i]);
    }
    printf("};\r\n");
    // char level_file_names[44][128];
    printf("const char level_file_names[44][128] =\r\n{\r\n");
    for (int i = 0; i < 44; i++)
    {
            printf("    \"%s\",\r\n", level_file_names[i]);
    }
    printf("};\r\n");
    // int32_t partime[44];
    printf("const int32_t partime[44] = \r\n{\r\n    ");
    for (int i = 0; i < 44; i++)
    {
        printf("0x%08X, ", partime[i]);
        if (i % MAX_WORDS_PER_LINE == MAX_WORDS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    printf("const int32_t designertime[44] = \r\n{\r\n    ");
    for (int i = 0; i < 44; i++)
    {
        printf("0x%08X, ", designertime[i]);
        if (i % MAX_WORDS_PER_LINE == MAX_WORDS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    // char fta_quotes[NUMOFFIRSTTIMEACTIVE][64];
    printf("const char fta_quotes[NUMOFFIRSTTIMEACTIVE][64]=\r\n{\r\n");
    for (int i = 0; i < NUMOFFIRSTTIMEACTIVE; i++)
    {
            printf("    \"");
            printCstring(fta_quotes[i]);
            printf("\",\r\n");
    }
    printf("};\r\n");
    // char  sounds[NUM_SOUNDS][14];
    printf("const char sounds[NUM_SOUNDS][14] = \r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
            printf("    \"%s\",\r\n", sounds[i]);
    }
    printf("\r\n};\r\n");
    //short soundps[NUM_SOUNDS]
    printf("const short soundps[NUM_SOUNDS] =\r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        printf("0x%04X, ", soundps[i] & 0xFFFF);
        if (i % MAX_SHORTS_PER_LINE == MAX_SHORTS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    //short soundpe[NUM_SOUNDS]
    printf("const short soundpe[NUM_SOUNDS] =\r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        printf("0x%04X, ", soundpe[i] & 0xFFFF);
        if (i % MAX_SHORTS_PER_LINE == MAX_SHORTS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    //short soundvo[NUM_SOUNDS]
    printf("const short soundvo[NUM_SOUNDS] =\r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        printf("0x%04X, ", soundvo[i] & 0xFFFF);
        if (i % MAX_SHORTS_PER_LINE == MAX_SHORTS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    //uint8_t soundm[NUM_SOUNDS]
    printf("const uint8_t soundm[NUM_SOUNDS] =\r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        printf("0x%02X, ", soundm[i] & 0xFF);
        if (i % MAX_SHORTS_PER_LINE == MAX_SHORTS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    //uint8_t soundpr[NUM_SOUNDS]
    printf("const uint8_t soundpr[NUM_SOUNDS] =\r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        printf("0x%02X, ", soundpr[i] & 0xFF);
        if (i % MAX_BYTES_PER_LINE == MAX_BYTES_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    // int32_t max_ammo_amount[MAX_WEAPONS]
    printf("const int32_t max_ammo_amount[MAX_WEAPONS] =\r\n{\r\n    ");
    for (int i = 0; i < MAX_WEAPONS; i++)
    {
        printf("0x%08X, ", max_ammo_amount[i]);
        if (i % MAX_WORDS_PER_LINE == MAX_WORDS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
#define PRINT_VARIABLE(type, name) printf("%s %s = %d;\r\n", type, #name, name);
    PRINT_VARIABLE("const int32_t", impact_damage);
    PRINT_VARIABLE("const int32_t", max_armour_amount);
    PRINT_VARIABLE("const int32_t", max_player_health);
    PRINT_VARIABLE("const int32_t", gc);
    PRINT_VARIABLE("const int32_t", dukefriction);
    PRINT_VARIABLE("const int32_t", respawnitemtime);
    PRINT_VARIABLE("const int32_t", respawnactortime);

    PRINT_VARIABLE("const int32_t", rpgblastradius);
    PRINT_VARIABLE("const int32_t", pipebombblastradius);
    PRINT_VARIABLE("const int32_t", shrinkerblastradius);
    PRINT_VARIABLE("const int32_t", tripbombblastradius);
    PRINT_VARIABLE("const int32_t", morterblastradius);
    PRINT_VARIABLE("const int32_t", bouncemineblastradius);
    PRINT_VARIABLE("const int32_t", seenineblastradius);
    PRINT_VARIABLE("const int32_t", numfreezebounces);
    PRINT_VARIABLE("const uint8_t", camerashitable);
    PRINT_VARIABLE("const uint8_t", freezerhurtowner);
    system("pause");
#else
    scriptptr = &script[MAXSCRIPTSIZE];
    labelcnt = LABEL_CNT;
    conVersion = 13;
#endif
#if LETS_WASTE_RAM_FOR_USERDEFS
		ud.conSize[0] = (int32_t)(scriptptr-script)-1;
#endif
}


void Startup(void)
{
   int i;
   CONFIG_SetDefaults(); // ADDED here

   // Init the Console
   CONSOLE_Init();

   KB_Startup();

   CONFIG_GetSetupFilename();
   CONFIG_ReadSetup();

   compilecons();
#if !LOAD_FROM_CON_FILE
        ud.const_visibility = 512;
#endif
#ifdef AUSTRALIA
  ud.lockout = 1;
#endif

   if(CommandSoundToggleOff) SoundToggle = 0;
   if(CommandMusicToggleOff) MusicToggle = 0;

//if(VOLUMEONE)
//{
//   printf("\n*** You have run Duke Nukem 3D %ld times. ***\n",ud.executions);
//   if(ud.executions >= 50) puts("IT IS NOW TIME TO UPGRADE TO THE COMPLETE VERSION!!!\n");
//}

   CONTROL_Startup( ControllerType, &GetTime, TICRATE );

// CTW - MODIFICATION
// initengine(ScreenMode,ScreenWidth,ScreenHeight);
   initengine();
// CTW END - MODIFICATION

   printf("Loading art header.");

  loadpics("tiles000.art", "\0");

  // this is to speed up sprite freeing.
  initFreeableSprites();

   readsavenames();
#if !USE_EXT_MEM
   tiles[MIRROR].dim.width = tiles[MIRROR].dim.height = 0;
#endif

   for(i=0;i<MAXPLAYERS;i++) playerreadyflag[i] = 0;
#if ORIGINAL_MULTIPLAYER
   initmultiplayers();
   if(numplayers > 1)
    puts("Multiplayer initialized.");
#endif

   ps[myconnectindex].palette = (uint8_t  *) &palette[0];
   SetupGameButtons();

   if(networkmode == 255)
       networkmode = 1;

   /* SBF - wasn't sure if swapping them would harm anything. */
   printf("Checking sound inits.");
   SoundStartup();
   //

   //
   printf("Checking music inits.");
   MusicStartup();
//
   // AutoAim
	if(nHostForceDisableAutoaim)
		ud.auto_aim = 0;
#if !WIN32
   loadtmb("d3dtimbr.tmb");
#else
    // this is just to take into account that we store to flash 13*256 bytes.
    // In this way we can simulate accurately in Win32 how much flash we have free.
    char dummy[13 * 256];
    storeToInternalFlash(dummy, sizeof(dummy));
#endif
}


void sendscore(char  *s)
{
    if(numplayers > 1)
      genericmultifunction(-1, (uint8_t *) s,strlen(s)+1,5);
}


void getnames(void)
{
    short i,l;

	// FIX_00031: Names now limited to 10 chars max that is the fragbar field limit.
    for(l=0; l<=9 && myname[l];l++)
    {
        ud.user_name[myconnectindex][l] = toupper(myname[l]);
        buf[l+2] = toupper(myname[l]);
    }

#ifdef CHECK_XDUKE_REV // must not be under "if(numplayers > 1)" so it runs in any case
	ud.rev[myconnectindex][0] = true; // always true. Used to check who we validated
	ud.rev[myconnectindex][1] = DUKE_ID;
	ud.rev[myconnectindex][2] = CHOCOLATE_DUKE_REV_X;
	ud.rev[myconnectindex][3] = CHOCOLATE_DUKE_REV_DOT_Y;
#endif
#if LETS_WASTE_RAM_FOR_USERDEFS
	memcpy(ud.groupefil_crc32[myconnectindex],groupefil_crc32, sizeof(groupefil_crc32));
	ud.conSize[myconnectindex] = ud.conSize[0]; // [0] still containing the original value
	ud.conCRC[myconnectindex] = ud.conCRC[0];
	ud.exeCRC[myconnectindex] = ud.exeCRC[0];
#endif
    if(numplayers > 1)
    {

        buf[0] = 6;
        buf[1] = grpVersion;

        buf[l+2] = 0;
        l += 3;

        //for(i=connecthead;i>=0;i=connectpoint2[i])
        //    if( i != myconnectindex )
        //        sendpacket(i,(uint8_t*)buf,l);
        broadcastPacket(buf, l, true);
		if(nHostForceDisableAutoaim==2) // user doesn't want AA off.
			{
				buf[0] = 133; // request to stop the game.
			//for(i=connecthead;i>=0;i=connectpoint2[i])
			//{
			//	sendpacket(i,(uint8_t*)buf,l);
			//}
			broadcastPacket(buf, l, false);
			}


#ifdef CHECK_XDUKE_REV
        buf[0] = 131; // xDuke TAG ID
        buf[1] = ud.rev[myconnectindex][1];
		buf[2] = ud.rev[myconnectindex][2]; // version x
		buf[3] = ud.rev[myconnectindex][3]; // version .y

		buf[4] = 0;		// reserved
		buf[5] = 0;		// reserved...
        buf[9] = 0;		// reserved.
		// See below for single player mode.

		//for(i=connecthead;i>=0;i=connectpoint2[i])
		//	if( i != myconnectindex )
        //        sendpacket(i,&buf[0],10);
        broadcastPacket(&buf[0], 10, true);
#endif

  //      getpackets();


        l = 1;
        buf[0] = 9; // send weapon order

        for(i=0;i<10;i++)
        {
            ud.wchoice[myconnectindex][i] = ud.mywchoice[i];
            buf[l] = (uint8_t ) ud.mywchoice[i];
            l++;
        }

        //for(i=connecthead;i>=0;i=connectpoint2[i])
        //    if(i != myconnectindex)
        //        sendpacket(i,(uint8_t*)&buf[0],11);
        broadcastPacket((uint8_t*)&buf[0], 11, true);
		buf[0] = 134;	// GRP CRC + CON SIZE + conCRC + exeCRC
#if LETS_WASTE_RAM_FOR_USERDEFS

		memcpy(buf+1, groupefil_crc32, sizeof(groupefil_crc32));
		memcpy(buf+1+sizeof(groupefil_crc32), ud.conSize, sizeof(ud.conSize[0]));
		memcpy(buf+1+sizeof(groupefil_crc32)+sizeof(ud.conSize[0]), ud.conCRC, sizeof(ud.conCRC[0]));
		memcpy(buf+1+sizeof(groupefil_crc32)+sizeof(ud.conSize[0])+sizeof(ud.conCRC[0]), ud.exeCRC, sizeof(ud.exeCRC[0]));
		//for(i=connecthead;i>=0;i=connectpoint2[i])
		//	if( i != myconnectindex )
		//		sendpacket(i,(uint8_t*)buf,1+sizeof(groupefil_crc32)+sizeof(ud.conSize[0])+sizeof(ud.conCRC[0])+ sizeof(ud.exeCRC[0]));
        broadcastPacket((uint8_t*)buf,1+sizeof(groupefil_crc32)+sizeof(ud.conSize[0])+sizeof(ud.conCRC[0])+ sizeof(ud.exeCRC[0]), true);)
#endif

//        getpackets();

        buf[0] = 10;
        buf[1] = ps[0].aim_mode;
        ps[myconnectindex].aim_mode = ps[0].aim_mode;

//        for(i=connecthead;i>=0;i=connectpoint2[i])
//            if(i != myconnectindex)
//                sendpacket(i,(uint8_t*)buf,2);
        broadcastPacket(buf, 2, true);
//        getpackets();

        if(cp == 0)
        {
            buf[0] = 125;

//            for(i=connecthead;i>=0;i=connectpoint2[i])
//                if(i != myconnectindex)
//                    sendpacket(i,(uint8_t*)buf,1);
            broadcastPacket(buf, 1, true);
        }

//        getpackets();

        waitforeverybody();

#ifdef CHECK_XDUKE_REV
		// from command "case 131:"
		for(l=0,i=connecthead;i>=0;i=connectpoint2[i])
			if(((ud.rev[i][2]<<8)+ud.rev[i][3]) != ((CHOCOLATE_DUKE_REV_X<<8)+CHOCOLATE_DUKE_REV_DOT_Y))
				l=1;
			else
				ud.rev[i][0] = true; // means we validated this guy

		if(l)
		{
			printf("\n*** One or more players do not have the same xDuke version:\n\n");
			for(l=0,i=connecthead;i>=0;i=connectpoint2[i])
				printf("Player [%-10s] is using Chocolate DukeNukem3D v%d.%d\n", ud.user_name[i],
				ud.rev[i][2],ud.rev[i][3]);
			Error(EXIT_SUCCESS, "");
		}
#endif

		// checking GRP/CON size from "case 134"
#if LETS_WASTE_RAM_FOR_USERDEFS
		for(l=0,i=connecthead;i>=0;i=connectpoint2[i])
			for(j=0; j<MAXGROUPFILES; j++)
			{
				if(ud.groupefil_crc32[i][j]!=ud.groupefil_crc32[myconnectindex][j] || ud.conSize[i] != ud.conSize[myconnectindex])
					l=1;
			}

		if(l)
		{
			printf("\n*** One or more players do not have the same GRP/CON version:\n\n");
			for(i=connecthead;i>=0;i=connectpoint2[i])
			{
				for(j=0; j<MAXGROUPFILES && ud.groupefil_crc32[i][j]; j++)
				{
					if(j)
						printf("                    GRP (Add-on) : %s CRC=%X\n",
						grpVersion2char_from_crc(ud.groupefil_crc32[i][j]),
						ud.groupefil_crc32[i][j]);
					else
						printf("Player [%-10s] GRP (base)   : %s CRC=%X\n", ud.user_name[i],
						grpVersion2char_from_crc(ud.groupefil_crc32[i][j]),
						ud.groupefil_crc32[i][j]);
				}
				printf("                    CON code size: %d\n\n",ud.conSize[i]);
			}
			Error(EXIT_SUCCESS,	"");
		}
#endif
    }
	else if(nHostForceDisableAutoaim==2)
	{
		nHostForceDisableAutoaim=0;
		ud.auto_aim = 2;
	}

    if(cp == 1)
        gameexit("Please put Duke Nukem 3D Atomic Edition CD in drive.");
}


const char* const baseDir="duke3d*.grp";
#if _WIN32

void findGRPToUse(char * groupfilefullpath)
{
    WIN32_FIND_DATA FindFileData;
	HANDLE hFind =  INVALID_HANDLE_VALUE;
    int i=0,kbdKey ;
	char  groupfile[9][512];
	int grpID ;

	if(getGameDir()[0] != '\0')
	{
		sprintf(groupfilefullpath, "%s\\%s", getGameDir(), baseDir);
		hFind = FindFirstFile(groupfilefullpath, &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			sprintf(groupfilefullpath, "%s", baseDir);
		}
		else
			FindClose(hFind);
	}
	else
		sprintf(groupfilefullpath, "%s", baseDir);

	printf("Searching '%s':\n\n",groupfilefullpath);
	hFind = FindFirstFile(groupfilefullpath,&FindFileData);

	if ( hFind==INVALID_HANDLE_VALUE )
		Error(EXIT_SUCCESS, "Can't find '%s'\n", groupfilefullpath);

	do
	{
		i++;
		sprintf(groupfile[i-1], "%s", FindFileData.cFileName);
		printf("Found GRP #%d:\t%lu Bytes\t %s \n", i, FindFileData.nFileSizeLow, groupfile[i-1]);
	} while ( FindNextFile(hFind, &FindFileData) && i < 9 );

	if(i==1)
		grpID = 0;
	else
	{
		printf("\n-> Choose a base GRP file from 1 to %c: ",'0' + i);
		do
			kbdKey = getch();
		while(kbdKey < '1' || kbdKey > ('0' + i));
		printf("%c\n", kbdKey);
		grpID = kbdKey-'1';

	}

	FindClose(hFind);
	if (strlen(getGameDir()) == 0)
		 sprintf(groupfilefullpath, "./%s", groupfile[grpID]);
	else
	   sprintf(groupfilefullpath, "%s//%s", getGameDir(), groupfile[grpID]);
}

#else
#if WIN32

int dukeGRP_Match(char* filename,int length)
{
    char* cursor = filename+length-4;

    if (strncasecmp(cursor,".grp",4))
        return 0;

    return !strncasecmp(filename,"duke3d",6);
}


#include <dirent.h>
void findGRPToUse(char * groupfilefullpath){
#ifdef WIN32
    char directoryToScan[512];
    struct dirent* dirEntry ;

    directoryToScan[0] = '\0';

    if (getGameDir()[0] != '\0')
    {
        strcat(directoryToScan,getGameDir());
        if (directoryToScan[strlen(directoryToScan)-1] != '/')
            strcat(directoryToScan,"/");
    }
    else{
        strcat(directoryToScan, "./");
    }

    printf("Scanning directory '%s' for a GRP file like '%s'.\n",directoryToScan,baseDir);

    DIR* dir =  opendir(directoryToScan);

    while ((dirEntry = readdir(dir)) != NULL)
    {

#ifdef __linux__
        if (dukeGRP_Match(dirEntry->d_name, _D_EXACT_NAMLEN(dirEntry)))
#else
        if (dukeGRP_Match(dirEntry->d_name,dirEntry->d_namlen))
#endif
        {
            sprintf(groupfilefullpath,"%s",dirEntry->d_name);
            return;
        }

    }
#endif
}
#endif

#endif
#if WIN32
extern int hlineasm4calls;
extern int hlineasm4pix;
#endif // WIN32
static int load_duke3d_groupfile(void)
{
	// FIX_00032: Added multi base GRP manager. Use duke3d*.grp to handle multiple grp.
#ifdef WIN32
	char  groupfilefullpath[512];
    groupfilefullpath[0] = '\0';

    findGRPToUse(groupfilefullpath);

    if (groupfilefullpath[0] == '\0')
    {
        return false;
    }


	FixFilePath(groupfilefullpath);

	return(initgroupfile(groupfilefullpath) != -1);
#else
  return(initgroupfile("") != -1);
#endif
}

void renderView(int i)
{
    #if LASTX_IN_STACK
        int16_t stack_lastx[MAXYDIM] = {0};  // was 32 bit
        lastx = stack_lastx;
    #endif // LASTX_IN_STACK
    int tilt;
    tilt = displayrooms(screenpeek,i);
    if (tilt)
    {
        tiltScreen(screenpeek);
    }
    faketimerhandler();
    //  next-hack: added here the switch to go to save menu
    checkSavegameScreenshotRequest();
    //
    displayrest(i);

}
uint8_t lineYoffs;  // next-hack: for multiplayer
#if WIN32
int divsPerFrame;
int profileGetWallPerFrame;
extern int paletteLoadedPerFrame;
int main(int argc,char  **argv)
#else
int dn3dmain(int argc,char  **argv)
#endif
{
    int32_t i, j;

#if WIN32
	uint8_t  kbdKey;
#endif

	//printf(	"This is a debug version 19.7.1 only Based on 19.7\n"
	//		"Fully compliant with v19.7. Added the following:\n\n"
	//		"FIX_00086: grp loaded by smaller sucessive chunks to avoid\n"
	//		"           overloading low ram computers (Spanator)\n"
	//		"FIX_00087: intro in 1024x768 mode being slow. Undone FIX_00070\n"
	//		"           and fixed font issue again (Bryzian)\n"
	//		"FIX_00088: crash on maps using a bad palette index like the end\n"
	//		"           of roch3.map (NY00123)\n"
	//		"FIX_00089: scoreboard not shown for last player who quits a DM.\n"
	//		"           Only 19.7 affected. (Sarah)\n"
	//		"FIX_00090: Removed showinfo key. FPS were shown after CRC msg. \n"
	//		"           CRC not always removed. (Turrican)\n"
	//		"FIX_00091: Main theme starting too early (Bryzian/Turrican)\n"
	//		"FIX_00092: corrupted saved files making the following saved\n"
	//		"           files invisible (Bryzian)\n\n"
	//		"This version should not be distributed. It's not secret but it\n"
	//		"would create a bad mess in the duke community if people start\n"
	//		"using it as it may contain new unsuspected bugs. Only a select\n"
	//		"group of known dukers who know what they are doing should be using\n"
	//		"it. Please report new bugs at xd@m-klein.com or on DX forums. Thx!\n\n");

	printf("*** Chocolate DukeNukem3D v%d.%d ***\n\n", CHOCOLATE_DUKE_REV_X, CHOCOLATE_DUKE_REV_DOT_Y);


	// FIX_00033: Fake multi and AI are now fully working
	ud.multimode = 1;  // xduke: must be done before checkcommandline or that will prevent Fakeplayer and AI
//
    internalFlashInit();
//
#if !WIN32
    setRadioTxBlockCallback(radioBlockTask);
#endif
    if (!load_duke3d_groupfile())
    {

        Error(EXIT_SUCCESS, "Could not initialize any original BASE duke3d*.grp file\n"
#if 0
							"Even if you are playing a custom GRP you still need\n"
							"an original base GRP file as Shareware/Full 1.3D GRP or\n"
							"the v1.5 ATOMIC GRP file. Such a file seems to be missing\n"
							"or is corrupted\n"
#endif
    );
    }
    if (NULL == kGetExtMemAddress("E2L1.map", NULL))
    {
        // shareware
        grpVersion = SHAREWARE_GRP13;
    }
    else if (NULL == kGetExtMemAddress("E4L1.map", NULL))
    {
        // full
        grpVersion = REGULAR_GRP13D;
    }
    else
    {
        grpVersion = ATOMIC_GRP14_15;
    }
    // next-hack: let's check what we got
	// FIX_00022: Automatically recognize the shareware grp (v1.3) + full version (1.3d) +
	//            atomic (1.4/1.5 grp) and the con files version (either 1.3 or 1.4) (JonoF's idea)

	// Detecting grp version
	// We keep the old GRP scheme detection for 19.6 compliance. Will be obsolete.
#if 0
	filehandle = kopen4load("DUKEDC9.MAP",1);
	kclose(filehandle);

	if (filehandle == -1) // not DC pack
	{
		filehandle = kopen4load("DUKESW.BIN",1);
		kclose(filehandle);

		if (filehandle == -1) // not Shareware version 1.3
		{
			filehandle = kopen4load("E4L11.MAP",1);
			kclose(filehandle);

			if (filehandle == -1) // not Atomic Edition 1.4/1.5
			{
				filehandle = kopen4load("E3L11.MAP",1);
				kclose(filehandle);

				if (filehandle == -1) // not Regular version 1.3d
				{
					grpVersion = UNKNOWN_GRP;
				}
				else
				{
					grpVersion = REGULAR_GRP13D;
				}
			}
			else
			{
				grpVersion = ATOMIC_GRP14_15;
			}
		}
		else
		{
			grpVersion = SHAREWARE_GRP13;
		}
	}
	else
	{
		grpVersion = DUKEITOUTINDC_GRP;
	}
#endif
#if WIN32
	// FIX_00062: Better support and identification for GRP and CON files for 1.3/1.3d/1.4/1.5
	if (	groupefil_crc32[0]==CRC_BASE_GRP_SHAREWARE_13 ||
				groupefil_crc32[0]==CRC_BASE_GRP_FULL_13 ||
				groupefil_crc32[0]==CRC_BASE_GRP_PLUTONIUM_14 ||
				groupefil_crc32[0]==CRC_BASE_GRP_ATOMIC_15 )
	{
		printf("GRP identified as: %s\n", grpVersion2char_from_crc(groupefil_crc32[0]));
	}
	else
	{

		printf(	"The content of your original BASE *.GRP is corrupted. CRC=%X\n"
			"You may run in troubles. Official GRP are:\n\n", groupefil_crc32[0]);

		for(i=0; i<MAX_KNOWN_GRP; i++)
			printf("%s -> CRC32=%X  Size=%d bytes\n", crc32lookup[i].name, crc32lookup[i].crc32, crc32lookup[i].size);

		printf(	"\nYou should try to get one of these GRP only as a base GRP\n"
				"Do you want to continue anyway? (Y/N): ");
#if WIN32
		do
			kbdKey = getch() | ' ';
		while(kbdKey != 'y' && kbdKey != 'n');
		printf("%c\n", kbdKey);
#endif
		if(kbdKey == 'n')
			Error(EXIT_SUCCESS,"");
	}
#endif
	// computing exe crc
#if LETS_WASTE_RAM_FOR_USERDEFS
	ud.exeCRC[0] = 0;
#endif
#if WIN32
#if USE_CRC
	filehandle = open(argv[0],O_BINARY|O_RDONLY);
	if(filehandle!=-1)
	{
		exe = malloc(filelength(filehandle));
		if(exe)
		{
			read(filehandle, exe, filelength(filehandle));
			ud.exeCRC[0] = crc32_update(exe, filelength(filehandle), ud.exeCRC[0]);
			free(exe);
		}
		close(filehandle);
	}
#endif
	checkcommandline(argc,argv);
#endif

    _platform_init(argc, argv, "Duke Nukem 3D", "Duke3D");
#ifdef WIN32
    totalmemory = Z_AvailHeap();

    if(memorycheckoveride == 0)
    {
        if(totalmemory < (3162000-350000))
        {
            puts("You don't have enough free memory to run Duke Nukem 3D.");
            puts("The DOS \"mem\" command should report 6,800K (or 6.8 megs)");
            puts("of \"total memory free\".\n");
            printf("Duke Nukem 3D requires %d more bytes to run.\n",3162000-350000-totalmemory);
            Error(EXIT_SUCCESS, "");
        }
    }
    else
        printf("Using %d bytes for heap.\n",totalmemory);

    RegisterShutdownFunction( ShutDown );
#endif

   inittimer(TICRATE);
    if( setgamemode(ScreenWidth,ScreenHeight) < 0 )
    {
        printf("\nVESA driver for ( %i * %i ) not found/supported!\n",xdim,ydim);
        ScreenWidth = 320;
        ScreenHeight = 200;
        setgamemode(ScreenWidth,ScreenHeight);
    }
    Startup();

#if OLD_MULTIPLAYER
    if(g_bStun)
	{
		waitforeverybody();
	}

    if(numplayers > 1) // if multimode > 1 and numplayer == 1 => fake player mode on
    {
        ud.multimode = numplayers;
        sendlogon();
    }
    else if(boardfilename[0] != 0)
    {
        ud.m_level_number = 7;
        ud.m_volume_number = 0;
        ud.warp_on = 1;
    }

    getnames();

    if(ud.multimode > 1)
    {
        playerswhenstarted = ud.multimode;

		// AddFaz fix.
		// This would cause monsters not to spawn when loading a usermap
		/*
        if(ud.warp_on == 0)
        {
            ud.m_monsters_off = 1;
            ud.m_player_skill = 0;
        }
		*/
    }
#else
    getnames();
#endif
    ud.last_level = -1;
#if USE_RTS
   RTS_Init(ud.rtsname);
   if(numlumps)
   {
	   printf("Using .RTS file:%s\n",ud.rtsname);
        system("pause");
   }
#endif
//   if (CONTROL_JoystickEnabled)
//       CONTROL_CenterJoystick(CenterCenter,UpperLeft,LowerRight,CenterThrottle,CenterRudder);
#if BOARD == BOARD_THEGAMEPAD_ARDUINONANOMATTER || ESP_PLATFORM
   CONTROL_JoystickEnabled = 1;
#endif
   printf("Loading palette/lookups.");

  /*  if( setgamemode(ScreenWidth,ScreenHeight) < 0 )
    {
        printf("\nVESA driver for ( %i * %i ) not found/supported!\n",xdim,ydim);
        ScreenWidth = 320;
        ScreenHeight = 200;
        setgamemode(ScreenWidth,ScreenHeight);
    }*/
    postDisplaySetup();

    printf("genspriteremaps()\n");
    genspriteremaps();


    setbrightness(ud.brightness>>2,&ps[myconnectindex].palette[0]);

    if(KB_KeyPressed( sc_Escape ) )
		gameexit(" ");

    FX_StopAllSounds();
    clearsoundlocks();

// now every level-independent data shall be loaded. Let the internal flash know
// where this data ends.
    internalFlashSetCommonZone();

    if(ud.warp_on > 1 && ud.multimode < 2)
    {
        clearview(0L);
        ps[myconnectindex].palette = palette;
        palto(0,0,0,0,false);
        rotatesprite(320<<15,200<<15,65536L,0,LOADSCREEN,0,0,2+8+64,0,0,xdim-1,ydim-1);
        menutext(160,105,0,0,"LOADING SAVED GAME...");
        nextpage(SCREEN_HEIGHT);
        waitForUpdate(-1);
        j = loadplayer(ud.warp_on-2);
        if(j)
            ud.warp_on = 0;
    }

    MAIN_LOOP_RESTART:

    if(ud.warp_on == 0) //if game is loaded without /V or /L cmd arguments.
	{

		if (numplayers > 1 && boardfilename[0] != 0) //check if a user map is loaded and in multiplayer.
		{
			int c;

			ud.level_number = ud.m_level_number = 7; // 7 = usermap.
            ud.volume_number = ud.m_volume_number;
			ud.player_skill = ud.m_player_skill;

            switch(ud.m_coop) //set item spawn options, as they would be if
			{      //game was started via main menu.
				case 0:
					ud.respawn_inventory = ud.m_respawn_inventory = 1;
					ud.respawn_items = ud.m_respawn_items = 1;
					break;
				case 1:
					ud.respawn_inventory = ud.m_respawn_inventory = 1;
					ud.respawn_items = ud.m_respawn_items = 0;
					break;
				case 2:
					ud.respawn_inventory = ud.m_respawn_inventory = 0;
					ud.respawn_items = ud.m_respawn_items = 0;
				break;
            }

			if( ud.m_player_skill == 4 )
			{
				ud.m_respawn_monsters = 1; //set skill
			}
			else
			{
				ud.m_respawn_monsters = 0;
			}

			waitforeverybody();

			for(c=connecthead;c>=0;c=connectpoint2[c])
            {
				resetweapons(c);
				resetinventory(c);
            }

			newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);

            enterlevel(MODE_GAME); //start game.

		}
		else
		{
			Logo(); //play logo, (game must be started via menus).
		}
	}



	else if(ud.warp_on == 1) //if cmd arguments /V and /L are given.
    {

		if (numplayers > 1) //if in multiplayer reset everyones weapon status.
		{
			int c;

            switch(ud.m_coop)	//set item spawn options, as they would be if
			{					//game was started via main menu.
				case 0:
					ud.respawn_inventory = ud.m_respawn_inventory = 1;
					ud.respawn_items = ud.m_respawn_items = 1;
					break;
				case 1:
					ud.respawn_inventory = ud.m_respawn_inventory = 1;
					ud.respawn_items = ud.m_respawn_items = 0;
					break;
				case 2:
					ud.respawn_inventory = ud.m_respawn_inventory = 0;
					ud.respawn_items = ud.m_respawn_items = 0;
					break;
			}

			if( ud.m_player_skill == 4 )
			{
				ud.m_respawn_monsters = 1; //set skill
			}
			else
			{
				ud.m_respawn_monsters = 0;
			}

			waitforeverybody();

			for(c=connecthead;c>=0;c=connectpoint2[c])
            {
			    resetweapons(c); //without this players would spawn with no weapon.
				resetinventory(c);
            }

		}

		newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
		enterlevel(MODE_GAME); //start game.

    }
    else
	{
		vscrn();
	}

    if( ud.warp_on == 0 && playback() )
    {
        FX_StopAllSounds();
        clearsoundlocks();
        nomorelogohack = 1;
        goto MAIN_LOOP_RESTART;
    }

    ud.warp_on = 0;
#if !WIN32
    int timemicrosec = I_GetTimeMicrosecs();
#endif
	//The main game loop is here, before starting game.
    // next-hack: reset timer here.
    sampletimer();
    ototalclock = totalclock;
    while ( !(ps[myconnectindex].gm&MODE_END) )
    {
        #define TEST_ROM_HITTYPES 0
        #if WIN32 && TEST_ROM_HITTYPES
        //
            int modified = memcmp(hittypes, initial_hittype, sizeof(initial_hittype));
            if (modified)
            {
                FIXME("MODIFIED!");
            }
        #endif // WIN32
        #if WIN32 && 0
        int equal = 0;
        for (int sprn = 0; sprn < MAXSPRITES; sprn++)
        {
            equal += (0 == memcmp(&sprite[sprn], &debugInitialSpriteValues[sprn], sizeof(spritetype)));
        }
        printf("Number equal sprites this frame: %d\r\n", equal);
        #endif // WIN
        #if WIN32 & 0
        #if USE_SEPARATE_HITTYPE_TEMPDATA
            int nchanged = 0; // number of temp changed. cumulative
            int htchanged[5] = {0}; // changes by hittype temp
            for (int i = 0; i < MAXSPRITES; i++)
            {
                for (int j = 0; j < 6; j++)
                {
                    if (getHittypeTemp(i, j))
                    {
                        if (j >= 4)
                        {
                            htchanged[4]++;
                        }
                        else
                        {
                            htchanged[j]++;
                        }
                        nchanged ++;
                    }
                }
            }
            printf("N of changed hittypeTemp %d. 0: %d, 1: %d, 2: %d, 3: %d, 4-5: %d\r\n",
                   nchanged,
                    htchanged[0], htchanged[1], htchanged[2], htchanged[3], htchanged[4]);

        #else
        extern int numHittype_tempdata;
            int nchanged = 0; // number of temp changed. cumulative
            int htchanged[5] = {0}; // changes by hittype temp
            int onlyHt[5] = {0};    // for each temp, number they are unique
            int numTemps[6] = {0}; // Tell how many sprites have n temps not 0.
            for (int i = 0; i < MAXSPRITES; i++)
            {
                int mod = 0;
                int mod45 = 0;
                int found = 0;
                int lastfound = 0;
                for (int j = 0; j < 6; j++)
                {
                    if (getHittypeTemp(i, j))
                    {
                        if (j >= 4)
                        {
                            mod45 = 1;
                            lastfound = 4;
                            htchanged[4]++;
                            found |= 1 << 4;
                        }
                        else
                        {
                            mod++;
                            lastfound = j;
                            htchanged[j]++;
                            found |= 1 << j;
                        }
                        nchanged ++;
                    }
                }
                numTemps[mod + mod45]++;
                if ((found & (found - 1)) == 0 && found)
                {
                    // found one hittype only
                    onlyHt[lastfound]++;
                }
            }
            printf("N of changed hittypeTemp %d. Tot Str: %d 0: %d, 1: %d, 2: %d, 3: %d, 4-5: %d - ",
                   nchanged, numHittype_tempdata,
                    htchanged[0], htchanged[1], htchanged[2], htchanged[3], htchanged[4]);
            printf("unique (tot: %d) 0: %d, 1: %d, 2: %d, 3: %d, 4-5: %d -- ", onlyHt[0] + onlyHt[1] + onlyHt[2] + onlyHt[3] + onlyHt[4],
                    onlyHt[0], onlyHt[1], onlyHt[2], onlyHt[3], onlyHt[4]);
            for (int i = 0; i < 6; i++)
            printf("%d-Mod: %d ", i, numTemps[i]);
            printf("\r\n");
#endif
        #endif // WIN32
        #if WIN32 & 0
             #define CHECK_DIFFERENCE(s1, s2, field) do {if (s1.field != s2.field) printf("Field %s is %d and %d\r\n", #field, s1.field, s2.field ); }while(0)

            int c1, c2, c3;
            if (sectors != NULL)
            {

                #if CHECK_SECTOR | 1
                for (int i = 0; i < MAXSECTORS; i++)
                {
                    c1 = memcmp(&sectors[i], &initial_sectors[i], sizeof(sectortype));
                    if (c1)
                    {

                        printf("Sector %i modified\r\n", i);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], floorheinum);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], ceilingstat);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], extra);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], hitag);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], lotag);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], floorpicnum);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], floorpal);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], ceilingpal);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], ceilingpicnum);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], floorstat);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], floorshade);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], ceilingypanning);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], ceilingxpanning);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], ceilingshade);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], floorypanning);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], floorz);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], floorxpanning);
                        CHECK_DIFFERENCE(sectors[i], initial_sectors[i], ceilingz);
                    }
                }
                #endif
                    c1 = memcmp(sectors, &initial_sectors, sizeof(initial_sectors));

                for (int i = 0; i < MAXSPRITES; i++)
                {

                    c2 = memcmp(&sprites[i], &initial_sprites[i], sizeof(spritetype));
                    if (c2)
                    {

                        printf("Sprite %i modified\r\n", i);

                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _x);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _y);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _z);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _picnum);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _yvel);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _xvel);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _cstat);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _xrepeat);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _yrepeat);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _xoffset);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _yoffset);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _extra);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _ang);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _pal);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _zvel);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _statnum);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _hitag);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _lotag);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _shade);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _owner);
                        CHECK_DIFFERENCE(initial_sprites[i], sprites[i], _sectnum);

                    }
                }
                c2 = memcmp(walls, initial_walls, sizeof(initial_walls));
                c3 = memcmp(sprites, initial_sprites, sizeof(initial_sprites));
                if (c1 || c2 || c3) printf("Comparisons %d %d %d\r\n", c1, c2, c3);
            }
        #endif // WIN32
        #if WIN32 & !USE_RAM_HITTYPE
            int hitchanged = 0;
            for (int i = 0; i < MAXSPRITES; i++)
            {
                if (memcmp(&hittype[i], &initial_hittype[i], sizeof(hittype[i])))
                {
                    hitchanged ++;
                }
            }
            printf("Ht changed: %d\r\n", hitchanged);
        #endif // WIN32
      faketimerhandler();
      updateAudio();
      sampletimer();
    	if( ud.recstat == 2 || ud.multimode > 1 || ( ud.show_help == 0 && (ps[myconnectindex].gm&MODE_MENU) != MODE_MENU ) )
    	{
    	    if( ps[myconnectindex].gm&MODE_GAME )
			{
				if( moveloop() )
				{
					continue;
				}
			}
    	}
        if( ps[myconnectindex].gm&MODE_EOL || ps[myconnectindex].gm&MODE_RESTART )
        {

            if( ps[myconnectindex].gm&MODE_EOL )
            {
                closedemowrite();

                ready2send = 0;

                i = ud.screen_size;
                ud.screen_size = 0;
                vscrn();
                ud.screen_size = i;
                dobonus(0);

                if(ud.eog)
                {
                    ud.eog = 0;
                    if(ud.multimode < 2)
                    {
						if(VOLUMEONE)
                        	doorders();

                        ps[myconnectindex].gm = MODE_MENU;
                        cmenu(MENU_MAIN);
                        probey = 0;
                        goto MAIN_LOOP_RESTART;
                    }
                    else
                    {
                        ud.m_level_number = 0;
                        ud.level_number = 0;
                    }
                }
            }

            ready2send = 0;
            if(numplayers > 1) ps[myconnectindex].gm = MODE_GAME;
            // next-hack: added the following lines to speedup level reload
            if ((ps[myconnectindex].gm  & MODE_RESTART) && (ps[myconnectindex].gm & MODE_RELOAD_FROM_DEAD))
            {
                loadplayer(MAX_SAVEGAMES);
            }
            else
            {
            enterlevel(ps[myconnectindex].gm);
            }
            continue;
        }

        if( !CONSOLE_IsActive() ) // added by next-hack, so cheats can be entered in console.
            cheats(false, NULL);

        if( !CONSOLE_IsActive() )
          nonsharedkeys();

        faketimerhandler();


        if( (ud.show_help == 0 && ud.multimode < 2 && !(ps[myconnectindex].gm&MODE_MENU) ) || ud.multimode > 1 || ud.recstat == 2)
        {
            i = min(max((totalclock-ototalclock)*(65536L/TICSPERFRAME),0),65536);
        //   FIXME("I %i, TICSPF %i g_iTickRate %i g_iTicksPerFrame %i\r\n", i, TICSPERFRAME, g_iTickRate, g_iTicksPerFrame);
        }
        else
        {
            i = 65536;

         //   FIXME("Imsx %i", i);

        }

        // let's keep stack_lastx in a separate scope
#if 0
        {
            #if LASTX_IN_STACK
                int16_t stack_lastx[MAXYDIM] = {0};  // was 32 bit
                lastx = stack_lastx;
            #endif // LASTX_IN_STACK
            int tilt;
            tilt = displayrooms(screenpeek,i);
            if (tilt)
            {
                tiltScreen(screenpeek);
            }
            faketimerhandler();
            //  next-hack: added here the switch to go to save menu
            checkSavegameScreenshotRequest();
            //
            displayrest(i);

        }
#else
#if WIN32
        uint32_t renderViewNumberOfGetRamWallIdxCalls = numberOfGetRamWallIdxCalls;
#endif
        renderView(i);
#if WIN32 && 0
        printf("numberOfGetRamWallIdxCalls for engine: %d\r\n", numberOfGetRamWallIdxCalls - renderViewNumberOfGetRamWallIdxCalls);
#endif
#endif
        faketimerhandler();

        if(ps[myconnectindex].gm&MODE_DEMO)
            goto MAIN_LOOP_RESTART;

#if USE_CACHE
        if(debug_on)
			caches();
#endif // USE_CACHE

        checksync();

		if (VOLUMEONE)
        	if(ud.show_help == 0 && show_shareware > 0 && (ps[myconnectindex].gm&MODE_MENU) == 0 )
            	rotatesprite((320-50)<<16,9<<16,65536L,0,BETAVERSION,0,0,2+8+16+128,0,0,xdim-1,ydim-1);
		//
#if WIN32 && PROFILE_HLINECALLS
        printf("hline4 Calls: %d, pix %d\r\n", hlineasm4calls, hlineasm4pix);
    hlineasm4calls = 0;
    hlineasm4pix = 0;
#endif // WIN32
#if !WIN32
#if EXCLUDE_PRINTF_FROM_FRAME_TIME_CALCULATION
     // rendering time calculation, excluding printf.
     int timenow = I_GetTimeMicrosecs();
     printf("TF: %05d\r\n", timenow - timemicrosec);
     timemicrosec = I_GetTimeMicrosecs();
#else
     // this includes printf, but at least you get the same number as displayed.
     int timenow = I_GetTimeMicrosecs();
     printf("%05d\r\n", timenow - timemicrosec);
     if ((timenow - timemicrosec ) > 47000)
     {
//       printf("TOO SLOW\r\n");
     }
#define FIND_SLOW_FRAMES 0
#if FIND_SLOW_FRAMES
     if (timenow - timemicrosec > 50000)
     {
         printf("SLOW FRAME\r\n");
         __asm volatile ("BKPT\r\t");
     }
#warning find slow frames active! This will block if the frame rate gets below 20 fps!
#endif
     timemicrosec = timenow;
#endif
#elif SHOW_DIVSPERFRAME
        printf("Div per frame: %d\r\n", divsPerFrame);
        divsPerFrame = 0;
#endif
#define SHOW_PALETTE_COPY_PER_FRAME 0
#if WIN32 && SHOW_PALETTE_COPY_PER_FRAME
    printf("Palettes copied: %d\r\n", paletteLoadedPerFrame);
    paletteLoadedPerFrame = 0;
#endif // WIN32
#define SHOW_GETWALL_PER_FRAME 0
#if WIN32 && SHOW_GETWALL_PER_FRAME
    printf("GetWallPerFrame: %d\r\n", profileGetWallPerFrame);
    profileGetWallPerFrame = 0;
#endif // WIN32
		if((ps[myconnectindex].gm & MODE_MENU))
		 {
		   nextpage(SCREEN_HEIGHT);
		   waitForUpdate(-1);
		 }
		 else
		 {
		   if (numplayers > 1)
		       lineYoffs = 9; //next-hack: the frag-bar is 9 pixel tall
		   else
		       lineYoffs = 0;
		   nextpage((ud.screen_size < 8 || sbarUpdated) ? SCREEN_HEIGHT : SCREEN_HEIGHT - STATUS_BAR_HEIGHT);
		 }

    }
    gameexit(" ");
	return(0);
}

#if DEMO_SUPPORT
uint8_t  opendemoread(uint8_t  which_demo) // 0 = mine
{
#if CODE_DISABLED

    char  d[] = "demo_.dmo";
    char  *fname = d;
    uint8_t  ver;
    short i,j;

	int32_t dummy;
	int32_t groupefil_crc32_from_demo[MAXGROUPFILES];

    if(which_demo == 10)
        d[4] = 'x';
    else
        d[4] = '0' + which_demo;

    ud.reccnt = 0;

     if(which_demo == 1 && firstdemofile[0] != 0)
     {
		fname = firstdemofile;
		if ((recfilep = TCkopen4load(firstdemofile,0)) == -1)
		{
			return(0);
		}
     }
     else
	 {
		 if ((recfilep = TCkopen4load(d,0)) == -1)
		 {
			return(0);
		 }
	 }

     kread(recfilep,&ud.reccnt,sizeof(int32_t));
     kread(recfilep,&ver,sizeof(uint8_t ));

	 printf("%s has version = %d\n", fname, ver);

	// FIX_00015: Backward compliance with older demos (down to demos v27, 28, 116, 117 and 118)
	if (PLUTOPAK)
	{
		if( (ver != BYTEVERSION && ver != BYTEVERSION_116 && ver != BYTEVERSION_117 && ver != BYTEVERSION_118) ) // || (ud.reccnt < 512) )
		{
			printf("%s is a demo version %d. We want v. %d, %d, %d, or %d (1.5 Atomic versions)\n",
					fname, (int) ver, BYTEVERSION_116, BYTEVERSION_117, BYTEVERSION_118, BYTEVERSION);
			kclose(recfilep);
			return 0;
		}
	}
	else // 1.3/1.3d style
	{
		if( (ver != BYTEVERSION && ver != BYTEVERSION_27 && ver != BYTEVERSION_28 && ver != BYTEVERSION_29) ) // || (ud.reccnt < 512) )
		{
			printf("%s is a demo version %d. We want v. %d, %d, %d or %d (1.3/1.3d versions)\n",
					fname, (int) ver, BYTEVERSION_27, BYTEVERSION_28, BYTEVERSION_29, BYTEVERSION);
			kclose(recfilep);
			return 0;
		}
	}

	// FIX_00062: Better support and identification for GRP and CON files for 1.3/1.3d/1.4/1.5
	if(ver==BYTEVERSION)
	{
		kread(recfilep, (int32_t *)groupefil_crc32_from_demo, sizeof(groupefil_crc32_from_demo));

		for(i=0; i<MAXGROUPFILES; i++)
			if(groupefil_crc32_from_demo[i]!=groupefil_crc32[i])
			{
				for(j=0; j<=i; j++)
				{
					printf("You have GRP #%d:  %s (CRC32=%X)\n"
						   "this demo expects %s (CRC32=%X)\n",
						j, grpVersion2char_from_crc(groupefil_crc32[j]),groupefil_crc32[j],
						grpVersion2char_from_crc(groupefil_crc32_from_demo[j]),groupefil_crc32_from_demo[j]);

				}
				kclose(recfilep);
				return 0;
			}


	}

	 ud.playing_demo_rev = ver;

	 kread(recfilep,(uint8_t  *)&ud.volume_number,sizeof(uint8_t ));
     kread(recfilep,(uint8_t  *)&ud.level_number,sizeof(uint8_t ));
     kread(recfilep,(uint8_t  *)&ud.player_skill,sizeof(uint8_t ));
     kread(recfilep,(uint8_t  *)&ud.m_coop,sizeof(uint8_t ));
     kread(recfilep,(uint8_t  *)&ud.m_ffire,sizeof(uint8_t ));
     kread(recfilep,(short *)&ud.multimode,sizeof(short));
     kread(recfilep,(short *)&ud.m_monsters_off,sizeof(short));
     kread(recfilep,(int32_t *)&ud.m_respawn_monsters,sizeof(int32_t));
     kread(recfilep,(int32_t *)&ud.m_respawn_items,sizeof(int32_t));
     kread(recfilep,(int32_t *)&ud.m_respawn_inventory,sizeof(int32_t));
     kread(recfilep,(int32_t *)&ud.playerai,sizeof(int32_t));
     kread(recfilep,(uint8_t  *)&ud.user_name[0][0],sizeof(ud.user_name));
	 // FIX_00034: Demos do not turn your run mode off anymore:
     kread(recfilep,(int32_t *)&dummy /*ud.auto_run*/,sizeof(int32_t)); // not needed and would affect autorun status in duke3d.cfg when quitting duke from a demo
     kread(recfilep,(uint8_t  *)boardfilename,sizeof(boardfilename));
     if( boardfilename[0] != 0 )
     {
        ud.m_level_number = 7;
        ud.m_volume_number = 0;
     }

     for(i=0;i<ud.multimode;i++)
	 {
        kread(recfilep,(int32_t *)&ps[i].aim_mode,sizeof(uint8_t ));

		// FIX_00080: Out Of Synch in demos. Tries recovering OOS in old demos v27/28/29/116/117/118. New: v30/v119.
		if(ver==BYTEVERSION)
			kread(recfilep,ud.wchoice[i],sizeof(ud.wchoice[0]));
	 }

     ud.god = ud.cashman = ud.eog = ud.showallmap = 0;
     ud.clipping = ud.scrollmode = ud.overhead_on = 0;
	 // FIX_00034: Demos do not turn your run mode off anymore:
     /* ud.showweapons =  */ ud.pause_on /*= ud.auto_run */ = 0; // makes no sense to reset those 2 value!

         newgame(ud.volume_number,ud.level_number,ud.player_skill);
         return(1);
#else
return 0;
 #endif

}

void opendemowrite(void)
{
#if CODE_DISABLED
    char  d[] = "demo1.dmo";
    int32_t dummylong = 0;
    uint8_t  ver;
    short i;
	char  fullpathdemofilename[16];

    if(ud.recstat == 2) kclose(recfilep);

    ver = BYTEVERSION;

	// Are we loading a TC?
	if(getGameDir()[0] != '\0'){
		// Yes
		sprintf(fullpathdemofilename, "%s\\%s", getGameDir(), d);
	}
	else{
		// No
		sprintf(fullpathdemofilename, "%s", d);
	}

// CTW - MODIFICATION
//  if ((frecfilep = fopen(d,"wb")) == -1) return;
    if ((frecfilep = fopen(fullpathdemofilename,"wb")) == NULL) return;
// CTW END - MODIFICATION
    fwrite(&dummylong,4,1,frecfilep);
    fwrite(&ver,sizeof(uint8_t ),1,frecfilep);
	// FIX_00062: Better support and identification for GRP and CON files for 1.3/1.3d/1.4/1.5
	fwrite((int32_t *)groupefil_crc32,sizeof(groupefil_crc32),1,frecfilep);
    fwrite((uint8_t  *)&ud.volume_number,sizeof(uint8_t ),1,frecfilep);
    fwrite((uint8_t  *)&ud.level_number,sizeof(uint8_t ),1,frecfilep);
    fwrite((uint8_t  *)&ud.player_skill,sizeof(uint8_t ),1,frecfilep);
    fwrite((uint8_t  *)&ud.m_coop,sizeof(uint8_t ),1,frecfilep);
    fwrite((uint8_t  *)&ud.m_ffire,sizeof(uint8_t ),1,frecfilep);
    fwrite((short *)&ud.multimode,sizeof(short),1,frecfilep);
    fwrite((short *)&ud.m_monsters_off,sizeof(short),1,frecfilep);
    fwrite((int32_t *)&ud.m_respawn_monsters,sizeof(int32_t),1,frecfilep);
    fwrite((int32_t *)&ud.m_respawn_items,sizeof(int32_t),1,frecfilep);
    fwrite((int32_t *)&ud.m_respawn_inventory,sizeof(int32_t),1,frecfilep);
    fwrite((int32_t *)&ud.playerai,sizeof(int32_t),1,frecfilep);
    fwrite((uint8_t  *)&ud.user_name[0][0],sizeof(ud.user_name),1,frecfilep);
    fwrite((int32_t *)&ud.auto_run,sizeof(int32_t),1,frecfilep);
    fwrite((uint8_t  *)boardfilename,sizeof(boardfilename),1,frecfilep);

    for(i=0;i<ud.multimode;i++)
	{
		fwrite((int32_t *)&ps[i].aim_mode,sizeof(uint8_t ),1,frecfilep); // seems wrong; prolly not needed anyway
		// FIX_00080: Out Of Synch in demos. Tries recovering OOS in old demos v27/28/29/116/117/118. New: v30/v119.
		fwrite(ud.wchoice[i],sizeof(ud.wchoice[0]),1,frecfilep);
	}

    totalreccnt = 0;
    ud.reccnt = 0;
#endif
}
void record(void)
{
    #if NOT_YET_IMPLEMENTED
    short i;
    for(i=connecthead;i>=0;i=connectpoint2[i])
         {
         copybufbyte(&sync[i],&recsync[ud.reccnt],sizeof(input));
			   ud.reccnt++;
                 totalreccnt++;
                 if (ud.reccnt >= RECSYNCBUFSIZ)
                 {
              dfwrite(recsync,sizeof(input)*ud.multimode,ud.reccnt/ud.multimode,frecfilep);
                          ud.reccnt = 0;
                 }
         }
    #else
        FIXME("Not yet implemented");
    #endif
}
#endif
void closedemowrite(void)
{
    #if NOT_YET_IMPLEMENTED
         if (ud.recstat == 1)
         {
        if (ud.reccnt > 0)
        {
            dfwrite(recsync,sizeof(input)*ud.multimode,ud.reccnt/ud.multimode,frecfilep);

            fseek(frecfilep,SEEK_SET,0L);
            fwrite(&totalreccnt,sizeof(int32_t),1,frecfilep);
            ud.recstat = ud.m_recstat = 0;
        }
        fclose(frecfilep);
        frecfilep = NULL;
    }
        #else
     //   FIXME("Not yet implemented");
    #endif
}

// CTW - MODIFICATION
// On my XP machine, demo playback causes the game to crash shortly in.
// Only bug found so far, not sure if it's OS dependent or compiler or what.
// Seems to happen when player input starts being simulated, but just guessing.
// This change effectively disables it. The related code is still enabled.
// (This is working on Linux, so I flipped it back to '1'. --ryan.)
 uint8_t  which_demo = 1;
// CTW END - MODIFICATION

uint8_t  in_menu = 0;
// extern int32_t syncs[];
int32_t playback(void)
{
    #if DEMO_SUPPORT
    int32_t i,j,k,l,t;
    uint8_t  foundemo;

    if( ready2send )
	{
		return 0;
	}

	/*
	if(numplayers > 1)
		return 1;
	*/

    foundemo = 0;

    RECHECK:

    in_menu = ps[myconnectindex].gm&MODE_MENU;

    pub = NUMPAGES;
    pus = NUMPAGES;

    flushperms();
#if DEMO_SUPPORT
	if(numplayers < 2 && ud.multimode_bot<2) foundemo = opendemoread(which_demo);
#endif

    if(foundemo == 0)
    {
#if DEMO_SUPPORT
        if(which_demo > 1)
        {
            which_demo = 1;
            goto RECHECK;
        }
#endif
        for(t=0;t<63;t+=7) palto(0,0,0,t,true);
        drawbackground();

        CONSOLE_HandleInput();
        if( !CONSOLE_IsActive())
        {
#if PRINT_MENU_CALLS
            printf("MENU %d\r\n", __LINE__);
#endif
            menus();
        }
        // note: will need to declare lastx in stack
        CONSOLE_Render();
        ps[myconnectindex].palette = palette;
        nextpage(SCREEN_HEIGHT);
        waitForUpdate(-1);
        for(t=63;t>0;t-=7)
		{
			palto(0,0,0,t,true);
		}

        ud.reccnt = 0;
    }
#if DEMO_SUPPORT
    else
    {
        ud.recstat = 2;
        which_demo++;
        if(which_demo == 10)
		{
			which_demo = 1;
		}

        enterlevel(MODE_DEMO);
    }
#endif
    if(foundemo == 0 || in_menu || KB_KeyWaiting() || numplayers > 1)
    {
        FX_StopAllSounds();
        clearsoundlocks();
        ps[myconnectindex].gm |= MODE_MENU;
    }

    ready2send = 0;
    i = 0;

    KB_FlushKeyboardQueue();

    k = 0;

    while (ud.reccnt > 0 || foundemo == 0)
    {
#if DEMO_SUPPORT
        if(foundemo) while ( totalclock >= (lockclock+TICSPERFRAME) )
        {
            if ((i == 0) || (i >= RECSYNCBUFSIZ))
            {
                i = 0;
                l = min(ud.reccnt,RECSYNCBUFSIZ);
                kdfread(recsync,sizeof(input)*ud.multimode,l/ud.multimode,recfilep);
            }

            for(j=connecthead;j>=0;j=connectpoint2[j])
            {
               copybufbyte(&recsync[i],&inputfifo[movefifoend[j]&(MOVEFIFOSIZ-1)][j],sizeof(input));

			   movefifoend[j]++;
               i++;
               ud.reccnt--;
            }
            domovethings();
        }
#endif
        if(foundemo == 0)
            drawbackground();
#if DEMO_SUPPORT
        else
        {
            if( !CONSOLE_IsActive() )
            {
                nonsharedkeys();
            }

            j = min(max((totalclock-lockclock)*(65536/TICSPERFRAME),0),65536);
            FIXME("demo not enabled");
            displayrooms(screenpeek,j);
            displayrest(j);

            if(ud.multimode > 1 && ps[myconnectindex].gm )
                getpackets();
        }

        if( (ps[myconnectindex].gm&MODE_MENU) && (ps[myconnectindex].gm&MODE_EOL) )
		{
			printf("playback(1) :: goto RECHECK:\n");
			goto RECHECK;
		}
#endif
        if(ps[myconnectindex].gm&MODE_TYPE)
        {
            typemode();
            if((ps[myconnectindex].gm&MODE_TYPE) != MODE_TYPE)
                ps[myconnectindex].gm = MODE_MENU;
        }
        else
        {
            CONSOLE_HandleInput();
            if( !CONSOLE_IsActive())
            {
#if PRINT_MENU_CALLS
            printf("MENU %d\r\n", __LINE__);
#endif
                menus();
            }

            CONSOLE_Render();
            if( ud.multimode > 1 )
            {
                ControlInfo noshareinfo;
                if( !CONSOLE_IsActive() )
                {
                    CONTROL_GetInput( &noshareinfo );
                    if( ACTION(gamefunc_SendMessage) )
                    {
                        KB_FlushKeyboardQueue();
                        CONTROL_ClearAction( gamefunc_SendMessage );
                        ps[myconnectindex].gm = MODE_TYPE;
                        typebuf[0] = 0;
                        inputloc = 0;
                    }
                }

            }
        }

        operatefta();

        if(ud.last_camsprite != ud.camerasprite)
        {
            ud.last_camsprite = ud.camerasprite;
            ud.camera_time = totalclock+(TICRATE*2);
        }

		if (VOLUMEONE)
			if( ud.show_help == 0 && (ps[myconnectindex].gm&MODE_MENU) == 0 )
				rotatesprite((320-50)<<16,9<<16,65536L,0,BETAVERSION,0,0,2+8+16+128,0,0,xdim-1,ydim-1);

		getpackets();
        updateAudio();
        nextpage(SCREEN_HEIGHT);
        waitForUpdate(-1);
        if( ps[myconnectindex].gm==MODE_END || ps[myconnectindex].gm==MODE_GAME )
        {
#if !USE_EXT_MEM
            if(foundemo)
                kclose(recfilep);
#endif
            ud.playing_demo_rev = 0;
			return 0;
        }
    }
 #if !USE_EXT_MEM
    kclose(recfilep);
#endif
	ud.playing_demo_rev = 0;
    if(ps[myconnectindex].gm&MODE_MENU)
	{
		goto RECHECK;
	}

    return 1;
#else
    int32_t i,t;
    uint8_t  foundemo;

    foundemo = 0;
#if DEMO_SUPPORT
    RECHECK:
#endif
    in_menu = ps[myconnectindex].gm&MODE_MENU;

    pub = NUMPAGES;
    pus = NUMPAGES;

    flushperms();
#if LASTX_IN_STACK
    int16_t stack_lastx[MAXYDIM] = {0};  // was 32 bit
    lastx = stack_lastx;
#endif // LASTX_IN_STACK

    if(foundemo == 0)
    {
        for(t=0;t<63;t+=7) palto(0,0,0,t,true);
        drawbackground();

        CONSOLE_HandleInput();
        if( !CONSOLE_IsActive())
        {
#if PRINT_MENU_CALLS
            printf("MENU %d\r\n", __LINE__);
#endif
            menus();
        }
        CONSOLE_Render();
        ps[myconnectindex].palette = palette;

        nextpage(SCREEN_HEIGHT);
        waitForUpdate(-1);
        for(t=63;t>0;t-=7)
		{
			palto(0,0,0,t,true);
		}

        ud.reccnt = 0;
    }

    FX_StopAllSounds();
    clearsoundlocks();
    ps[myconnectindex].gm |= MODE_MENU;

    ready2send = 0;
    i = 0;

    KB_FlushKeyboardQueue();

    while (ud.reccnt > 0 || foundemo == 0)
    {
        drawbackground();
        if(ps[myconnectindex].gm&MODE_TYPE)
        {
            typemode();
            if((ps[myconnectindex].gm&MODE_TYPE) != MODE_TYPE)
                ps[myconnectindex].gm = MODE_MENU;
        }
        else
        {
            CONSOLE_HandleInput();
            if( !CONSOLE_IsActive())
            {
#if PRINT_MENU_CALLS
            printf("MENU %d\r\n", __LINE__);
#endif
                menus();
                // next-hack: fix me
                if (multiplayerConnection)
                {
                    cmenu(MENU_MULTI_PEER_LOBBY);
                    menuEnableMultiplayerReady(false);
                    //cmenu(MENU_MAIN);
                    for(i=0;i<MAXPLAYERS;i++) playerreadyflag[i] = 0;
                    initmultiplayers();
                    if(networkmode == 255)
                        networkmode = 1;

                    multiplayerConnection = 0;
                    cmenu(MENU_MULTI_WAITING_FOR_OTHER_PLAYERS_READY_MESSAGE);
                    menus();
                    nextpage(SCREEN_HEIGHT);

                    getnames();

                    if(numplayers > 1) // if multimode > 1 and numplayer == 1 => fake player mode on
                    {
                        ud.multimode = numplayers;
                        sendlogon();    // actually empty function...
                    }
                    else if(boardfilename[0] != 0)
                    {
                        ud.m_level_number = 7;
                        ud.m_volume_number = 0;
                        ud.warp_on = 1;
                    }
                }
            }
            CONSOLE_Render();
        }

        operatefta();

        if(ud.last_camsprite != ud.camerasprite)
        {
            ud.last_camsprite = ud.camerasprite;
            ud.camera_time = totalclock+(TICRATE*2);
        }

		getpackets();
        updateAudio();
        nextpage(SCREEN_HEIGHT);
        waitForUpdate(-1);
        if( ps[myconnectindex].gm==MODE_END || ps[myconnectindex].gm==MODE_GAME )
        {
            ud.playing_demo_rev = 0;
			return 0;
        }
    }
    return 1;
#endif
}

uint8_t  moveloop()
{
    int32_t i;

    if (numplayers > 1)
	{
		while (fakemovefifoplc < movefifoend[myconnectindex])
		{
			fakedomovethings();
		}
	}


    getpackets();

    if (numplayers < 2) bufferjitter = 0;
    while (movefifoend[myconnectindex]-movefifoplc > bufferjitter)
    {
        for(i=connecthead;i>=0;i=connectpoint2[i])
            if (movefifoplc == movefifoend[i]) break;
        if (i >= 0) break;
        if( domovethings() ) return 1;
    }
    return 0;
}

void fakedomovethingscorrect(void)
{
     int32_t i;
     struct player_struct *p;

     if (numplayers < 2) return;

     i = ((movefifoplc-1)&(MOVEFIFOSIZ-1));
     p = &ps[myconnectindex];

     if (p->posx == myxbak[i] && p->posy == myybak[i] && p->posz == myzbak[i]
          && p->horiz == myhorizbak[i] && p->ang == myangbak[i]) return;

     myx = p->posx; omyx = p->oposx; myxvel = p->posxv;
     myy = p->posy; omyy = p->oposy; myyvel = p->posyv;
     myz = p->posz; omyz = p->oposz; myzvel = p->poszv;
     myang = p->ang; omyang = p->oang;
     mycursectnum = p->cursectnum;
     myhoriz = p->horiz; omyhoriz = p->ohoriz;
     myhorizoff = p->horizoff; omyhorizoff = p->ohorizoff;
     myjumpingcounter = p->jumping_counter;
     myjumpingtoggle = p->jumping_toggle;
     myonground = p->on_ground;
     myhardlanding = p->hard_landing;
     myreturntocenter = p->return_to_center;

     fakemovefifoplc = movefifoplc;
     while (fakemovefifoplc < movefifoend[myconnectindex])
          fakedomovethings();

}

void fakedomovethings(void)
{
        input *syn;
        struct player_struct *p;
        int32_t i, j, k, doubvel, fz, cz, hz, lz, x, y;
        uint32_t sb_snum;
        short psect, psectlotag, tempsect, backcstat;
        uint8_t  shrunk, spritebridge;

        syn = (input *)&inputfifo[fakemovefifoplc&(MOVEFIFOSIZ-1)][myconnectindex];

        p = &ps[myconnectindex];

        backcstat = getSprite(p->i)->_cstat;
        setSprite_cstat(p->i, getSprite(p->i)->_cstat & ~257);

        sb_snum = syn->bits;

        psect = mycursectnum;
        psectlotag = getSector(psect)->lotag;
        spritebridge = 0;

        shrunk = (getSprite(p->i)->_yrepeat < 32);

        if( ud.clipping == 0 && ( getSector(psect)->floorpicnum == MIRROR || psect < 0 || psect >= MAXSECTORS) )
        {
            myx = omyx;
            myy = omyy;
        }
        else
        {
            omyx = myx;
            omyy = myy;
        }

        omyhoriz = myhoriz;
        omyhorizoff = myhorizoff;
        omyz = myz;
        omyang = myang;

        getzrange(myx,myy,myz,psect,&cz,&hz,&fz,&lz,163L,CLIPMASK0);

        j = getflorzofslope(psect,myx,myy);

        if( (lz&49152) == 16384 && psectlotag == 1 && klabs(myz-j) > PHEIGHT+(16<<8) )
            psectlotag = 0;

        if( p->aim_mode == 0 && myonground && psectlotag != 2 && (getSector(psect)->floorstat&2) )
        {
                x = myx+(sintable[(myang+512)&2047]>>5);
                y = myy+(sintable[myang&2047]>>5);
                tempsect = psect;
                updatesector(x,y,&tempsect);
                if (tempsect >= 0)
                {
                     k = getflorzofslope(psect,x,y);
                     if (psect == tempsect)
                          myhorizoff += mulscale16(j-k,160);
                     else if (klabs(getflorzofslope(tempsect,x,y)-k) <= (4<<8))
                          myhorizoff += mulscale16(j-k,160);
                }
        }
        if (myhorizoff > 0) myhorizoff -= ((myhorizoff>>3)+1);
        else if (myhorizoff < 0) myhorizoff += (((-myhorizoff)>>3)+1);

        if(hz >= 0 && (hz&49152) == 49152)
        {
#if !NEW_MAXSPRITES

                hz &= (MAXSPRITES-1);
#else
                        CHECK_SPRITE(hz);

                hz &= MAX_SPRITESMASK;
#endif
                if (getSprite(hz)->_statnum == 1 && getSprite(hz)->_extra >= 0)
                {
                    hz = 0;
                    cz = getceilzofslope(psect,myx,myy);
                }
        }

        if(lz >= 0 && (lz&49152) == 49152)
        {
#if !NEW_MAXSPRITES
                 j = lz&(MAXSPRITES-1);
#else
                        CHECK_SPRITE(lz);
                 j = lz & MAX_SPRITESMASK;
#endif
                 if ((getSprite(j)->_cstat&33) == 33)
                 {
                        psectlotag = 0;
                        spritebridge = 1;
                 }
                 if(badguy(getSprite(j)) && getSprite(j)->_xrepeat > 24 && klabs(getSprite(p->i)->_z-getSprite(j)->_z) < (84<<8) )
                 {
                    j = getangle( getSprite(j)->_x-myx,getSprite(j)->_y-myy);
                    myxvel -= sintable[(j+512)&2047]<<4;
                    myyvel -= sintable[j&2047]<<4;
                }
        }

        if( getSprite(p->i)->_extra <= 0 )
        {
                 if( psectlotag == 2 )
                 {
                            if(p->on_warping_sector == 0)
                            {
                                     if( klabs(myz-fz) > (PHEIGHT>>1))
                                             myz += 348;
                            }
                            clipmove(&myx,&myy,&myz,&mycursectnum,0,0,164L,(4L<<8),(4L<<8),CLIPMASK0);
                 }

                 updatesector(myx,myy,&mycursectnum);
                 pushmove(&myx,&myy,&myz,&mycursectnum,128L,(4L<<8),(20L<<8),CLIPMASK0);

                myhoriz = 100;
                myhorizoff = 0;

                 goto ENDFAKEPROCESSINPUT;
        }

        doubvel = TICSPERFRAME;

        if(p->on_crane >= 0) goto FAKEHORIZONLY;

        if(p->one_eighty_count < 0) myang += 128;

        i = 40;

        if( psectlotag == 2)
        {
                 myjumpingcounter = 0;

                 if ( sb_snum&1 )
                 {
                            if(myzvel > 0) myzvel = 0;
                            myzvel -= 348;
                            if(myzvel < -(256*6)) myzvel = -(256*6);
                 }
                 else if (sb_snum&(1<<1))
                 {
                            if(myzvel < 0) myzvel = 0;
                            myzvel += 348;
                            if(myzvel > (256*6)) myzvel = (256*6);
                 }
                 else
                 {
                    if(myzvel < 0)
                    {
                        myzvel += 256;
                        if(myzvel > 0)
                            myzvel = 0;
                    }
                    if(myzvel > 0)
                    {
                        myzvel -= 256;
                        if(myzvel < 0)
                            myzvel = 0;
                    }
                }

                if(myzvel > 2048) myzvel >>= 1;

                 myz += myzvel;

                 if(myz > (fz-(15<<8)) )
                            myz += ((fz-(15<<8))-myz)>>1;

                 if(myz < (cz+(4<<8)) )
                 {
                            myz = cz+(4<<8);
                            myzvel = 0;
                 }
        }

        else if(p->jetpack_on)
        {
                 myonground = 0;
                 myjumpingcounter = 0;
                 myhardlanding = 0;

                 if(p->jetpack_on < 11)
                            myz -= (p->jetpack_on<<7); //Goin up

                 if(shrunk) j = 512;
                 else j = 2048;

                 if (sb_snum&1)                            //A
                            myz -= j;
                 if (sb_snum&(1<<1))                       //Z
                            myz += j;

                 if(shrunk == 0 && ( psectlotag == 0 || psectlotag == 2 ) ) k = 32;
                 else k = 16;

                 if(myz > (fz-(k<<8)) )
                            myz += ((fz-(k<<8))-myz)>>1;
                 if(myz < (cz+(18<<8)) )
                            myz = cz+(18<<8);
        }
        else if( psectlotag != 2 )
        {
            if (psectlotag == 1 && p->spritebridge == 0)
            {
                 if(shrunk == 0) i = 34;
                 else i = 12;
            }
                 if(myz < (fz-(i<<8)) && (floorspace(psect)|ceilingspace(psect)) == 0 ) //falling
                 {
                            if( (sb_snum&3) == 0 && myonground && (getSector(psect)->floorstat&2) && myz >= (fz-(i<<8)-(16<<8) ) )
                                     myz = fz-(i<<8);
                            else
                            {
                                     myonground = 0;

                                     myzvel += (gc+80);

                                     if(myzvel >= (4096+2048)) myzvel = (4096+2048);
                            }
                 }

                 else
                 {
                            if(psectlotag != 1 && psectlotag != 2 && myonground == 0 && myzvel > (6144>>1))
                                 myhardlanding = myzvel>>10;
                            myonground = 1;

                            if(i==40)
                            {
                                     //Smooth on the ground

                                     k = ((fz-(i<<8))-myz)>>1;
                                     if( klabs(k) < 256 ) k = 0;
                                     myz += k; // ((fz-(i<<8))-myz)>>1;
                                     myzvel -= 768; // 412;
                                     if(myzvel < 0) myzvel = 0;
                            }
                            else if(myjumpingcounter == 0)
                            {
                                myz += ((fz-(i<<7))-myz)>>1; //Smooth on the water
                                if(p->on_warping_sector == 0 && myz > fz-(16<<8))
                                {
                                    myz = fz-(16<<8);
                                    myzvel >>= 1;
                                }
                            }

                            if( sb_snum&2 )
                                     myz += (2048+768);

                            if( (sb_snum&1) == 0 && myjumpingtoggle == 1)
                                     myjumpingtoggle = 0;

                            else if( (sb_snum&1) && myjumpingtoggle == 0 )
                            {
                                     if( myjumpingcounter == 0 )
                                             if( (fz-cz) > (56<<8) )
                                             {
                                                myjumpingcounter = 1;
                                                myjumpingtoggle = 1;
                                             }
                            }
                            if( myjumpingcounter && (sb_snum&1) == 0 )
                                myjumpingcounter = 0;
                 }

                 if(myjumpingcounter)
                 {
                            if( (sb_snum&1) == 0 && myjumpingtoggle == 1)
                                     myjumpingtoggle = 0;

                            if( myjumpingcounter < (1024+256) )
                            {
                                     if(psectlotag == 1 && myjumpingcounter > 768)
                                     {
                                             myjumpingcounter = 0;
                                             myzvel = -512;
                                     }
                                     else
                                     {
                                             myzvel -= (sintable[(2048-128+myjumpingcounter)&2047])/12;
                                             myjumpingcounter += 180;

                                             myonground = 0;
                                     }
                            }
                            else
                            {
                                     myjumpingcounter = 0;
                                     myzvel = 0;
                            }
                 }

                 myz += myzvel;

                 if(myz < (cz+(4<<8)) )
                 {
                            myjumpingcounter = 0;
                            if(myzvel < 0) myxvel = myyvel = 0;
                            myzvel = 128;
                            myz = cz+(4<<8);
                 }

        }

        if ( p->fist_incs ||
                     p->transporter_hold > 2 ||
                     myhardlanding ||
                     p->access_incs > 0 ||
                     p->knee_incs > 0 ||
                     (p->curr_weapon == TRIPBOMB_WEAPON &&
                      p->kickback_pic > 1 &&
                      p->kickback_pic < 4 ) )
        {
                 doubvel = 0;
                 myxvel = 0;
                 myyvel = 0;
        }
        else if ( syn->avel )          //p->ang += syncangvel * constant
        {                         //ENGINE calculates angvel for you
            int32_t tempang;

            tempang = syn->avel<<1;

            if(psectlotag == 2)
                myang += (tempang-(tempang>>3))*sgn(doubvel);
            else myang += (tempang)*sgn(doubvel);
            myang &= 2047;
        }

        if ( myxvel || myyvel || syn->fvel || syn->svel )
        {
                 if(p->steroids_amount > 0 && p->steroids_amount < 400)
                     doubvel <<= 1;

                 myxvel += ((syn->fvel*doubvel)<<6);
                 myyvel += ((syn->svel*doubvel)<<6);

                 if( ( p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 10 && myonground ) || ( myonground && (sb_snum&2) ) )
                 {
                            myxvel = mulscale16(myxvel,dukefriction-0x2000);
                            myyvel = mulscale16(myyvel,dukefriction-0x2000);
                 }
                 else
                 {
                    if(psectlotag == 2)
                    {
                        myxvel = mulscale16(myxvel,dukefriction-0x1400);
                        myyvel = mulscale16(myyvel,dukefriction-0x1400);
                    }
                    else
                    {
                        myxvel = mulscale16(myxvel,dukefriction);
                        myyvel = mulscale16(myyvel,dukefriction);
                    }
                 }

                 if( abs(myxvel) < 2048 && abs(myyvel) < 2048 )
                     myxvel = myyvel = 0;

                 if( shrunk )
                 {
                     myxvel =
                         mulscale16(myxvel,(dukefriction)-(dukefriction>>1)+(dukefriction>>2));
                     myyvel =
                         mulscale16(myyvel,(dukefriction)-(dukefriction>>1)+(dukefriction>>2));
                 }
        }

FAKEHORIZONLY:
        if(psectlotag == 1 || spritebridge == 1) i = (4L<<8); else i = (20L<<8);

        clipmove(&myx,&myy,&myz,&mycursectnum,myxvel,myyvel,164L,4L<<8,i,CLIPMASK0);
        pushmove(&myx,&myy,&myz,&mycursectnum,164L,4L<<8,4L<<8,CLIPMASK0);

        if( p->jetpack_on == 0 && psectlotag != 1 && psectlotag != 2 && shrunk)
            myz += 30<<8;

        if ((sb_snum&(1<<18)) || myhardlanding)
            myreturntocenter = 9;

        if (sb_snum&(1<<13))
        {
                myreturntocenter = 9;
                if (sb_snum&(1<<5)) myhoriz += 6;
                myhoriz += 6;
        }
        else if (sb_snum&(1<<14))
        {
                myreturntocenter = 9;
                if (sb_snum&(1<<5)) myhoriz -= 6;
                myhoriz -= 6;
        }
        else if (sb_snum&(1<<3))
        {
                if (sb_snum&(1<<5)) myhoriz += 6;
                myhoriz += 6;
        }
        else if (sb_snum&(1<<4))
        {
                if (sb_snum&(1<<5)) myhoriz -= 6;
                myhoriz -= 6;
        }

        if (myreturntocenter > 0)
            if ((sb_snum&(1<<13)) == 0 && (sb_snum&(1<<14)) == 0)
        {
             myreturntocenter--;
             myhoriz += 33-(myhoriz/3);
        }

        if(p->aim_mode)
            myhoriz += syn->horz>>1;
        else
        {
            if( myhoriz > 95 && myhoriz < 105) myhoriz = 100;
            if( myhorizoff > -5 && myhorizoff < 5) myhorizoff = 0;
        }

        if (myhardlanding > 0)
        {
            myhardlanding--;
            myhoriz -= (myhardlanding<<4);
        }

        if (myhoriz > 299) myhoriz = 299;
        else if (myhoriz < -99) myhoriz = -99;

        if(p->knee_incs > 0)
        {
            myhoriz -= 48;
            myreturntocenter = 9;
        }


ENDFAKEPROCESSINPUT:

        myxbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myx;
        myybak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myy;
        myzbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myz;
        myangbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myang;
        myhorizbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myhoriz;
        fakemovefifoplc++;

        setSprite_cstat(p->i, backcstat);
}


uint8_t  domovethings(void)
{
    short i, j;
    uint8_t  ch;
#if TRYING_SPEEDUP_RAMWALLS
        uint16_t stackWallIndexes[MAXWALLS];
#define FASTER_COPY 1
#if FASTER_COPY
        union
        {
           uint32_t wi;
           struct
           {
                   uint32_t low : 8;
                   uint32_t high : 2;
                   uint32_t filler : (32 - 8 - 2);
           } swi;
        } wi;
        uint32_t wi2;
        uint32_t i2 = 0;
        for (int i = 0; i < numwalls; )
        {
            wi.wi = ramwallIdx8[i];
            wi2 = ramwallIdx2[i2];
            wi.swi.high = wi2; //hopefully BFI is used.
            stackWallIndexes[i] = wi.wi;
            i++;
            //

            wi.wi = ramwallIdx8[i];
            wi2 = wi2 >> 2;
            wi.swi.high = wi2; //hopefully BFI is used.
            stackWallIndexes[i] = wi.wi;
            i++;
            //
            wi.wi = ramwallIdx8[i];
            wi2 = wi2 >> 2;
            wi.swi.high = wi2; //hopefully BFI is used.
            stackWallIndexes[i] = wi.wi;
            i++;
            //
            wi.wi = ramwallIdx8[i];
            wi2 = wi2 >> 2;
            wi.swi.high = wi2; //hopefully BFI is used.
            stackWallIndexes[i] = wi.wi;
            i++;
            //
            // last
            i2++;
        }
#else

        for (int i = 0; i < numwalls; i++)
            stackWallIndexes[i] = getRamWallIdx(i);
#endif
        setStackWallIndexes(stackWallIndexes);
#endif


    for(i=connecthead;i>=0;i=connectpoint2[i])
        if( sync[i].bits&(1<<17) )
    {
        multiflag = 2;
        multiwhat = (sync[i].bits>>18)&1;
        multipos = (uint32_t) (sync[i].bits>>19)&15;
        multiwho = i;
#if ENABLE_MULTIPLAYER_SAVES
        if( multiwhat )
        {
			// FIX_00058: Save/load game crash in both single and multiplayer
#if ENABLE_SCREESHOT
            screencapt = 1;
            displayrooms(myconnectindex,65536);
//            savetemp("duke3d.tmp",tiles[MAXTILES-1].data,160*100);
            screencapt = 0;
#endif
            saveplayer( multipos );
            multiflag = 0;
#if RAM_FTA_QUOTES
            if(multiwho != myconnectindex)
            {
                strcpy(fta_quotes[122],&ud.user_name[multiwho][0]);
                strcat(fta_quotes[122]," SAVED A MULTIPLAYER GAME");
                FTA(122,&ps[myconnectindex],1);
            }
            else
            {
                strcpy(fta_quotes[122],"MULTIPLAYER GAME SAVED");
                FTA(122,&ps[myconnectindex],1);
            }
#else
            if(multiwho != myconnectindex)
            {
                strcpy(get_fta_quotes(CUSTOM_FTA),&ud.user_name[multiwho][0]);
                strcat(get_fta_quotes(CUSTOM_FTA)," SAVED A MULTIPLAYER GAME");
                FTA(CUSTOM_FTA,&ps[myconnectindex],1);
            }
            else
            {
                strcpy(get_fta_quotes(CUSTOM_FTA),"MULTIPLAYER GAME SAVED");
                FTA(CUSTOM_FTA,&ps[myconnectindex],1);
            }

#endif
            break;
        }
        else
        {
//            waitforeverybody();
            j = loadplayer( multipos );

            multiflag = 0;

            if(j == 0)
            {
#if RAM_FTA_QUOTES
                if(multiwho != myconnectindex)
                {
                    strcpy(fta_quotes[122],&ud.user_name[multiwho][0]);
                    strcat(fta_quotes[122]," LOADED A MULTIPLAYER GAME");
                    FTA(122,&ps[myconnectindex],1);
                }
                else
                {
                    strcpy(fta_quotes[122],"MULTIPLAYER GAME LOADED");
                    FTA(122,&ps[myconnectindex],1);
                }
#endif
                return 1;
            }

        }
#endif

    }

    ud.camerasprite = -1;
    lockclock += TICSPERFRAME;

    if(earthquaketime > 0) earthquaketime--;
    if(rtsplaying > 0) rtsplaying--;

    for(i=0;i<MAXUSERQUOTES;i++)
         if (user_quote_time[i])
         {
             user_quote_time[i]--;
             if (!user_quote_time[i]) pub = NUMPAGES;
         }
     if ((klabs(quotebotgoal-quotebot) <= 16) && (ud.screen_size <= 8))
         quotebot += ksgn(quotebotgoal-quotebot);
     else
         quotebot = quotebotgoal;

    if( show_shareware > 0 )
    {
        show_shareware--;
        if(show_shareware == 0)
        {
            pus = NUMPAGES;
            pub = NUMPAGES;
        }
    }

    everyothertime++;

    for(i=connecthead;i>=0;i=connectpoint2[i])
        copybufbyte(&inputfifo[movefifoplc&(MOVEFIFOSIZ-1)][i],&sync[i],sizeof(input));
    movefifoplc++;

    updateinterpolations();

    j = -1;
    for(i=connecthead;i>=0;i=connectpoint2[i])
     {
          if ((sync[i].bits&(1<<26)) == 0) { j = i; continue; }

          closedemowrite();

          if (i == myconnectindex) gameexit(" ");
          if (screenpeek == i)
          {
                screenpeek = connectpoint2[i];
                if (screenpeek < 0) screenpeek = connecthead;
          }

          if (i == connecthead) connecthead = connectpoint2[connecthead];
          else connectpoint2[j] = connectpoint2[i];

          numplayers--;
          ud.multimode--;

          if (numplayers < 2)
              sound(GENERIC_AMBIENCE17);

          pub = NUMPAGES;
          pus = NUMPAGES;
          vscrn();

          sprintf((char *)buf,"%s is history!",ud.user_name[i]);

          quickkill(&ps[i]);
          deletesprite(ps[i].i);

          adduserquote((char *)buf);

          if(j < 0 && networkmode == 0 )
              gameexit( " \nThe 'MASTER/First player' just quit the game.  All\nplayers are returned from the game. This only happens in 5-8\nplayer mode as a different network scheme is used.");
      }

      if ((numplayers >= 2) && ((movefifoplc&7) == 7))
      {
            ch = (uint8_t )(randomseed&255);
            for(i=connecthead;i>=0;i=connectpoint2[i])
                 ch += ((ps[i].posx+ps[i].posy+ps[i].posz+ps[i].ang+ps[i].horiz)&255);
            syncval[myconnectindex][syncvalhead[myconnectindex]&(MOVEFIFOSIZ-1)] = ch;
            syncvalhead[myconnectindex]++;


      }
#if DEMO_SUPPORT
    if(ud.recstat == 1) record();
#endif
    if( ud.pause_on == 0 )
    {
        global_random = TRAND;
        movedummyplayers();//ST 13
    }

    for(i=connecthead;i>=0;i=connectpoint2[i])
    {
        cheatkeys(i);

        if( ud.pause_on == 0 )
        {
            processinput(i);
            checksectors(i);
        }
    }
    if( ud.pause_on == 0 )
    {

#if MG24
        uint32_t t1 = I_GetTimeMicrosecs();
        movefta();//ST 2
        uint32_t t2 = I_GetTimeMicrosecs();
        moveweapons();          //ST 5 (must be last)
        uint32_t  t3 = I_GetTimeMicrosecs();
        movetransports();       //ST 9
        uint32_t  t4 = I_GetTimeMicrosecs();
        moveplayers();          //ST 10
        uint32_t  t5 = I_GetTimeMicrosecs();
        movefallers();          //ST 12
        uint32_t  t6 = I_GetTimeMicrosecs();
        moveexplosions();       //ST 4
        uint32_t  t7 = I_GetTimeMicrosecs();
        moveactors();           //ST 1
        uint32_t  t8 = I_GetTimeMicrosecs();
        moveeffectors();        //ST 3 This is the slowest.
        uint32_t  t9 = I_GetTimeMicrosecs();
        movestandables();       //ST 6. THIS CAN BE VERY SLOW
        uint32_t  t10 = I_GetTimeMicrosecs();
        doanimations();
        uint32_t  t11 = I_GetTimeMicrosecs();
        movefx();               //ST 11
        uint32_t  t12 = I_GetTimeMicrosecs();
    /*    printf("%d %d %d %d %d %d %d %d %d %d %d\r\n",
            t2-t1,
            t3-t2,
            t4-t3,
            t5-t4,
            t6-t5,
            t7-t6,
            t8-t7,
            t9-t8,
            t10-t9,
            t11-t10,
            t12-t11
        );*/
        UNUSED(t1);
        UNUSED(t2);
        UNUSED(t3);
        UNUSED(t4);
        UNUSED(t5);
        UNUSED(t6);
        UNUSED(t7);
        UNUSED(t8);
        UNUSED(t9);
        UNUSED(t10);
        UNUSED(t11);
        UNUSED(t12);

        #else
        movefta();//ST 2
        moveweapons();          //ST 5 (must be last)
        movetransports();       //ST 9
        moveplayers();          //ST 10
        movefallers();          //ST 12
        moveexplosions();       //ST 4
        moveactors();           //ST 1
        moveeffectors();        //ST 3 This is the slowest.
        movestandables();       //ST 6. THIS CAN BE VERY SLOW
        doanimations();
        movefx();               //ST 11
        #endif

    }

    fakedomovethingscorrect();

    if( (everyothertime&1) == 0)
    {
        animatewalls();
        movecyclers();
        pan3dsound();
    }
#if TRYING_SPEEDUP_RAMWALLS
        setStackWallIndexes(NULL);
#if FASTER_COPY
        i2 = 0;
        union
        {
                uint32_t wi2;
                struct
                {
                        uint32_t w0 : 2;
                        uint32_t w1 : 2;
                        uint32_t w2 : 2;
                        uint32_t w3 : 2;
                        uint32_t filler : 32 - 8;

                } bf;
        } w2;
        i2 = 0;
        for (int i = 0; i < numwalls; )
        {
            wi.wi = stackWallIndexes[i]; // get index
            ramwallIdx8[i] = wi.swi.low; // save low byte
            w2.bf.w0 = wi.swi.high;      // save high part to temporary register
            i++;                         // next wall
            wi.wi = stackWallIndexes[i]; // get index
            ramwallIdx8[i] = wi.swi.low; // save low byte
            w2.bf.w1 = wi.swi.high;      // save high part to temporary register
            i++;                         // next wall
            wi.wi = stackWallIndexes[i]; // get index
            ramwallIdx8[i] = wi.swi.low; // save low byte
            w2.bf.w2 = wi.swi.high;      // save high part to temporary register
            i++;                         // next wall
            wi.wi = stackWallIndexes[i]; // get index
            ramwallIdx8[i] = wi.swi.low; // save low byte
            w2.bf.w3 = wi.swi.high;      // save high part to temporary register
            i++;
            // save index 2
            ramwallIdx2[i2] = w2.wi2;
            //
            i2++;
         }

#else
        for (int i = 0; i < numwalls; i++)
            setRamWallIdx(i, stackWallIndexes[i]);
#endif
#endif

    return 0;
}


void doorders(void)
{
    short i;

    setview(0,0,xdim-1,ydim-1);

    for(i=0;i<63;i+=7) palto(0,0,0,i,true);
    ps[myconnectindex].palette = palette;
    totalclock = 0;
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    nextpage(SCREEN_HEIGHT);
    waitForUpdate(-1);
    for(i=63;i>0;i-=7) palto(0,0,0,i,true);
    totalclock = 0;while( !KB_KeyWaiting() ) {getpackets(); updateAudio();}

    for(i=0;i<63;i+=7) palto(0,0,0,i,true);
    totalclock = 0;
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING+1,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    nextpage(SCREEN_HEIGHT);
    waitForUpdate(-1);
    for(i=63;i>0;i-=7) palto(0,0,0,i,true);
    totalclock = 0;while( !KB_KeyWaiting() ) {getpackets(); updateAudio();}

    for(i=0;i<63;i+=7) palto(0,0,0,i,true);
    totalclock = 0;
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING+2,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    nextpage(SCREEN_HEIGHT);
    waitForUpdate(-1);
    for(i=63;i>0;i-=7) palto(0,0,0,i,true);
    totalclock = 0;while( !KB_KeyWaiting() ) {getpackets(); updateAudio();}

    for(i=0;i<63;i+=7) palto(0,0,0,i,true);
    totalclock = 0;
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING+3,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    nextpage(SCREEN_HEIGHT);
    waitForUpdate(-1);
    for(i=63;i>0;i-=7) palto(0,0,0,i,true);
    totalclock = 0;while( !KB_KeyWaiting() ) {getpackets(); updateAudio();}
}

void dobonus(uint8_t  bonusonly)
{
    uint8_t tempStack[MAX_RAM_FLAT_SIZE];
    setTempRamTileData(tempStack);
#if LASTX_IN_STACK
    int16_t stack_lastx[MAXYDIM] = {0};  // was 32 bit
    lastx = stack_lastx;
#endif // LASTX_IN_STACK
    short t, gfx_offset;
//    short tinc;
    int32_t i, y,xfragtotal,yfragtotal;
    short bonuscnt;
    char text[64];

    const int32_t breathe[] =
    {
         0,  30,VICTORY1+1,176,59,
        30,  60,VICTORY1+2,176,59,
        60,  90,VICTORY1+1,176,59,
        90, 120,0         ,176,59
    };

    const int32_t bossmove[] =
    {
         0, 120,VICTORY1+3,86,59,
       220, 260,VICTORY1+4,86,59,
       260, 290,VICTORY1+5,86,59,
       290, 320,VICTORY1+6,86,59,
       320, 350,VICTORY1+7,86,59,
       350, 380,VICTORY1+8,86,59,
       0, 0, 0, 0, 0          // next-hack: added otherwise undefined behavior.
    };

    bonuscnt = 0;

    for(t=0;t<64;t+=7) { palto(0,0,0,t,true);  updateAudio();}
    setview(0,0,xdim-1,ydim-1);
    clearview(0L);
    nextpage(SCREEN_HEIGHT);
    waitForUpdate(-1);
    flushperms();

    FX_StopAllSounds();
    clearsoundlocks();
    FX_SetReverb(0L);


    if(bonusonly) goto FRAGBONUS;

    if(numplayers < 2 && ud.eog && ud.from_bonus == 0)
        switch(ud.volume_number)
    {
        case 0:
            if(ud.lockout == 0)
            {
                clearview(0L);
                rotatesprite(0,50<<16,65536L,0,VICTORY1,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                nextpage(SCREEN_HEIGHT);
                waitForUpdate(-1);
                ps[myconnectindex].palette = endingpal;
                for(t=63;t>=0;t--) {palto(0,0,0,t,true); updateAudio();}

                KB_FlushKeyboardQueue();
                totalclock = 0; //tinc = 0;
                while( 1 )
                {
                    clearview(0L);
                    rotatesprite(0,50<<16,65536L,0,VICTORY1,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

                    // boss
                    if( totalclock > 390 && totalclock < 780 )
                    {
                        for(t=0;t<35;t+=5)
                        {
                            if( bossmove[t+2] && (totalclock%390) > bossmove[t] && (totalclock%390) <= bossmove[t+1] )
                            {
                                if(t==10 && bonuscnt == 1)
                                {
                                    sound(SHOTGUN_FIRE);
                                    sound(SQUISHED);
                                    bonuscnt++;
                                }
                                rotatesprite(bossmove[t+3]<<16,bossmove[t+4]<<16,65536L,0,bossmove[t+2],0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                            }
                            updateAudio();
                        }
                    }
                    // Breathe
                    if( totalclock < 450 || totalclock >= 750 )
                    {
                        if(totalclock >= 750)
                        {
                            rotatesprite(86<<16,59<<16,65536L,0,VICTORY1+8,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                            if(totalclock >= 750 && bonuscnt == 2) { sound(DUKETALKTOBOSS); bonuscnt++; }
                        }
                        for(t=0;t<20;t+=5)
                        {
                            if( breathe[t+2] && (totalclock%120) > breathe[t] && (totalclock%120) <= breathe[t+1] )
                            {
                                if(t==5 && bonuscnt == 0)
                                {
                                    sound(BOSSTALKTODUKE);
                                    bonuscnt++;
                                }
                                rotatesprite(breathe[t+3]<<16,breathe[t+4]<<16,65536L,0,breathe[t+2],0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                            }
                            updateAudio();
                        }
                    }

                    updateAudio();
                    getpackets();
                    nextpage(SCREEN_HEIGHT);
                    waitForUpdate(-1);
                    if( KB_KeyWaiting() ) break;
                }
            }

            for(t=0;t<64;t++) { palto(0,0,0,t,true);  updateAudio();}


            KB_FlushKeyboardQueue();
            ps[myconnectindex].palette = palette;

            rotatesprite(0,0,65536L,0,3292,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
            nextpage(SCREEN_HEIGHT);
            waitForUpdate(-1);
            for(t=63;t>0;t--) {palto(0,0,0,t,true);  updateAudio();}
            while( !KB_KeyWaiting() ) {getpackets(); updateAudio(); keyhandler();}
            for(t=0;t<64;t++) {palto(0,0,0,t,true); updateAudio();}
            MUSIC_StopSong();
            FX_StopAllSounds();
            clearsoundlocks();
            break;
        case 1:
            MUSIC_StopSong();
            clearview(0L);
            nextpage(SCREEN_HEIGHT);
            waitForUpdate(-1);
            if(ud.lockout == 0)
            {
                playanm("cineov2.anm",1);
                KB_FlushKeyboardQueue();
                clearview(0L);
                nextpage(SCREEN_HEIGHT);
                waitForUpdate(-1);
            }

            sound(PIPEBOMB_EXPLODE);

            for(t=0;t<64;t++) { palto(0,0,0,t,true);  updateAudio();}
            setview(0,0,xdim-1,ydim-1);
            KB_FlushKeyboardQueue();
            ps[myconnectindex].palette = palette;
            rotatesprite(0,0,65536L,0,3293,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
            nextpage(SCREEN_HEIGHT);
            waitForUpdate(-1);
            for(t=63;t>0;t--) {palto(0,0,0,t,true); updateAudio();}
            while( !KB_KeyWaiting() ) {getpackets(); updateAudio(); keyhandler();}
            for(t=0;t<64;t++) {palto(0,0,0,t,true); updateAudio();}

            break;

        case 3:

            setview(0,0,xdim-1,ydim-1);

            MUSIC_StopSong();
            clearview(0L);
            nextpage(SCREEN_HEIGHT);
            waitForUpdate(-1);
            if(ud.lockout == 0)
            {
                KB_FlushKeyboardQueue();
                playanm("vol4e1.anm",8);
                clearview(0L);
                nextpage(SCREEN_HEIGHT);
                waitForUpdate(-1);
                playanm("vol4e2.anm",10);
                clearview(0L);
                nextpage(SCREEN_HEIGHT);
                waitForUpdate(-1);
                playanm("vol4e3.anm",11);
                clearview(0L);
                nextpage(SCREEN_HEIGHT);
                waitForUpdate(-1);
            }

            FX_StopAllSounds();
            clearsoundlocks();
            sound(ENDSEQVOL3SND4);
            KB_FlushKeyboardQueue();

            ps[myconnectindex].palette = palette;
            palto(0,0,0,63,false);
            clearview(0L);
            menutext(160,60,0,0,"THANKS TO ALL OUR");
            menutext(160,60+16,0,0,"FANS FOR GIVING");
            menutext(160,60+16+16,0,0,"US BIG HEADS.");
            menutext(160,70+16+16+16,0,0,"LOOK FOR A DUKE NUKEM 3D");
            menutext(160,70+16+16+16+16,0,0,"SEQUEL SOON.");
            nextpage(SCREEN_HEIGHT);
            waitForUpdate(-1);

            for(t=63;t>0;t-=3) {palto(0,0,0,t,true); updateAudio();}
            KB_FlushKeyboardQueue();
            while(!KB_KeyWaiting()) {getpackets(); updateAudio(); keyhandler();}
            for(t=0;t<64;t+=3) {palto(0,0,0,t,true); updateAudio();}

            clearview(0L);
            nextpage(SCREEN_HEIGHT);
            waitForUpdate(-1);

            playanm("DUKETEAM.ANM",4);

            KB_FlushKeyboardQueue();
            while(!KB_KeyWaiting()) {getpackets(); updateAudio(); keyhandler();}

            clearview(0L);
            nextpage(SCREEN_HEIGHT);
            waitForUpdate(-1);
            palto(0,0,0,63,false);

            FX_StopAllSounds();
            clearsoundlocks();
            KB_FlushKeyboardQueue();

            break;

        case 2:

            MUSIC_StopSong();
            clearview(0L);
            nextpage(SCREEN_HEIGHT);
            waitForUpdate(-1);
            if(ud.lockout == 0)
            {
                for(t=63;t>=0;t--) {palto(0,0,0,t,true); updateAudio();}
                playanm("cineov3.anm",2);
                KB_FlushKeyboardQueue();
                ototalclock = totalclock+200;
                while(totalclock < ototalclock) {getpackets(); updateAudio();}
                clearview(0L);
                nextpage(SCREEN_HEIGHT);
                waitForUpdate(-1);
                FX_StopAllSounds();
                clearsoundlocks();
            }

            playanm("RADLOGO.ANM",3);

            if( ud.lockout == 0 && !KB_KeyWaiting() )
            {
                #if USE_CACHE
                sound(ENDSEQVOL3SND5);
                while(Sound[ENDSEQVOL3SND5].lock>=200) getpackets();
                if(KB_KeyWaiting()) goto ENDANM;
                sound(ENDSEQVOL3SND6);
                while(Sound[ENDSEQVOL3SND6].lock>=200) getpackets();
                if(KB_KeyWaiting()) goto ENDANM;
                sound(ENDSEQVOL3SND7);
                while(Sound[ENDSEQVOL3SND7].lock>=200) getpackets();
                if(KB_KeyWaiting()) goto ENDANM;
                sound(ENDSEQVOL3SND8);
                while(Sound[ENDSEQVOL3SND8].lock>=200) getpackets();
                if(KB_KeyWaiting()) goto ENDANM;
                sound(ENDSEQVOL3SND9);
                while(Sound[ENDSEQVOL3SND9].lock>=200) getpackets();
                #else
                    int voiceNum;
                    voiceNum = sound(ENDSEQVOL3SND5);
                    waitForVoiceEnd(voiceNum, 0, getpackets);
                    if(KB_KeyWaiting()) goto ENDANM;
                    voiceNum = sound(ENDSEQVOL3SND6);
                    waitForVoiceEnd(voiceNum, 0, getpackets);
                    if(KB_KeyWaiting()) goto ENDANM;
                    voiceNum = sound(ENDSEQVOL3SND7);
                    waitForVoiceEnd(voiceNum, 0, getpackets);
                    if(KB_KeyWaiting()) goto ENDANM;
                    voiceNum = sound(ENDSEQVOL3SND8);
                    waitForVoiceEnd(voiceNum, 0, getpackets);
                    if(KB_KeyWaiting()) goto ENDANM;
                    voiceNum = sound(ENDSEQVOL3SND9);
                    waitForVoiceEnd(voiceNum, 0, getpackets);
                #endif
            }

            KB_FlushKeyboardQueue();
            totalclock = 0;
            while(!KB_KeyWaiting() && totalclock < 120) {getpackets(); updateAudio();   keyhandler();}

            ENDANM:

            FX_StopAllSounds();
            clearsoundlocks();

            KB_FlushKeyboardQueue();

            clearview(0L);

            break;
    }

    FRAGBONUS:

    ps[myconnectindex].palette = palette;
    KB_FlushKeyboardQueue();
    totalclock = 0; //tinc = 0;
    bonuscnt = 0;

    MUSIC_StopSong();
    FX_StopAllSounds();
    clearsoundlocks();

    if(playerswhenstarted > 1 && ud.coop != 1 )
    {
        if(!(MusicToggle == 0 || MusicDevice == SC_Unknown))
            sound(BONUSMUSIC);

        rotatesprite(0,0,65536L,0,MENUSCREEN,16,0,2+8+16+64,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,34<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
        rotatesprite((260)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,0,0,2+8,0,0,xdim-1,ydim-1);
        gametext(160,58+2,"MULTIPLAYER TOTALS",0,2+8+16);
        gametext(160,58+10,level_names[(ud.volume_number*11)+ud.last_level-1],0,2+8+16);

        gametext(160,165,"PRESS ANY KEY TO CONTINUE",0,2+8+16);


        t = 0;
        minitext(23,80,"   NAME                                           KILLS",8,2+8+16+128);
        for(i=0;i<playerswhenstarted;i++)
        {
            snprintf(text, sizeof(text),"%-4d",i+1);
            minitext(92+(i*23),80,text,3,2+8+16+128);
        }

        for(i=0;i<playerswhenstarted;i++)
        {
            xfragtotal = 0;
            snprintf(text, sizeof(text),"%d",i+1);

            minitext(30,90+t,text,0,2+8+16+128);
            minitext(38,90+t,ud.user_name[i],ps[i].palookup,2+8+16+128);

            for(y=0;y<playerswhenstarted;y++)
            {
                if(i == y)
                {
                    snprintf(text, sizeof(text),"%-4d",ps[y].fraggedself);
                    minitext(92+(y*23),90+t,text,2,2+8+16+128);
                    xfragtotal -= ps[y].fraggedself;
                }
                else
                {
                    snprintf(text, sizeof(text),"%-4d",frags[i][y]);
                    minitext(92+(y*23),90+t,text,0,2+8+16+128);
                    xfragtotal += frags[i][y];
                }

                if(myconnectindex == connecthead)
                {
                    snprintf(text, sizeof(text),"stats %d killed %d %d\n",i+1,y+1,frags[i][y]);
                    sendscore(text);
                }
            }

            snprintf(text, sizeof(text),"%-4d",xfragtotal);
            minitext(101+(8*23),90+t,text,2,2+8+16+128);

            t += 7;
        }

        for(y=0;y<playerswhenstarted;y++)
        {
            yfragtotal = 0;
            for(i=0;i<playerswhenstarted;i++)
            {
                if(i == y)
                    yfragtotal += ps[i].fraggedself;
                yfragtotal += frags[i][y];
            }
            snprintf(text, sizeof(text),"%-4d",yfragtotal);
            minitext(92+(y*23),96+(8*7),text,2,2+8+16+128);
        }

        minitext(45,96+(8*7),"DEATHS",8,2+8+16+128);
        nextpage(SCREEN_HEIGHT);
        waitForUpdate(-1);

        for(t=0;t<64;t+=7)
        {
            palto(0,0,0,63-t,true);
            updateAudio();
        }
        KB_FlushKeyboardQueue();
        while(KB_KeyWaiting()==0) {getpackets(); updateAudio(); keyhandler();}
#if ENABLE_SCREENSHOT
        if( KB_KeyPressed( sc_F12 ) )
        {
            KB_ClearKeyDown( sc_F12 );
            takescreenshot();
        }
#endif
        if(bonusonly || ud.multimode > 1) return;

        for(t=0;t<64;t+=7) {palto(0,0,0,t,true); updateAudio();}
    }

    if(bonusonly || ud.multimode > 1) return;

    switch(ud.volume_number)
    {
        case 1:
            gfx_offset = 5;
            break;
        default:
            gfx_offset = 0;
            break;
    }

    rotatesprite(0,0,65536L,0,BONUSSCREEN+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

    menutext(160,20-6,0,0,&level_names[(ud.volume_number*11)+ud.last_level-1][0]);
    menutext(160,36-6,0,0,"COMPLETED");

    gametext(160,192,"PRESS ANY KEY TO CONTINUE",16,2+8+16);

    if(!(MusicToggle == 0 || MusicDevice == SC_Unknown))
        sound(BONUSMUSIC);

    nextpage(SCREEN_HEIGHT);
    waitForUpdate(-1);

    KB_FlushKeyboardQueue();
    for(t=0;t<64;t++) {palto(0,0,0,63-t,true); updateAudio();}
    bonuscnt = 0;
    totalclock = 0; //tinc = 0;

    while( 1 )
    {
        waitForUpdate(-1);
        sampletimer();
        updateAudio();
        keyhandler();
        if(ps[myconnectindex].gm&MODE_EOL)
        {
            rotatesprite(0,0,65536L,0,BONUSSCREEN+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

            if( totalclock > (1000000000L) && totalclock < (1000000320L) )
            {
                switch( (totalclock>>4)%15 )
                {
                    case 0:
                        if(bonuscnt == 6)
                        {
                            bonuscnt++;
                            sound(SHOTGUN_COCK);
                            switch(rand()&3)
                            {
                                case 0:
                                    sound(BONUS_SPEECH1);
                                    break;
                                case 1:
                                    sound(BONUS_SPEECH2);
                                    break;
                                case 2:
                                    sound(BONUS_SPEECH3);
                                    break;
                                case 3:
                                    sound(BONUS_SPEECH4);
                                    break;
                            }
                        }
                    case 1:
                    case 4:
                    case 5:
                        rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+3+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                        break;
                    case 2:
                    case 3:
                       rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+4+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                       break;
                }
            }
            else if( totalclock > (10240+120L) ) break;
            else
            {
                switch( (totalclock>>5)&3 )
                {
                    case 1:
                    case 3:
                        rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+1+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                        break;
                    case 2:
                        rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+2+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                        break;
                }
            }

            menutext(160,20-6,0,0,&level_names[(ud.volume_number*11)+ud.last_level-1][0]);
            menutext(160,36-6,0,0,"COMPLETED");

            gametext(160,192,"PRESS ANY KEY TO CONTINUE",16,2+8+16);

            if( totalclock > (60*3) )
            {
                gametext(10,59+9,"Your Time:",0,2+8+16);
                gametext(10,69+9,"Par time:",0,2+8+16);
                gametext(10,78+9,"3D Realms' Time:",0,2+8+16);
                if(bonuscnt == 0)
                    bonuscnt++;

                if( totalclock > (60*4) )
                {
                    if(bonuscnt == 1)
                    {
                        bonuscnt++;
                        sound(PIPEBOMB_EXPLODE);
                    }
                    snprintf(text, sizeof(text),"%02d:%02d",
                        (ps[myconnectindex].player_par/(26*60))%60,
                        (ps[myconnectindex].player_par/26)%60);
                    gametext((320>>2)+71,60+9,text,0,2+8+16);

                    snprintf(text, sizeof(text),"%02d:%02d",
                        (partime[ud.volume_number*11+ud.last_level-1]/(26*60))%60,
                        (partime[ud.volume_number*11+ud.last_level-1]/26)%60);
                    gametext((320>>2)+71,69+9,text,0,2+8+16);

                    snprintf(text, sizeof(text),"%02d:%02d",
                        (designertime[ud.volume_number*11+ud.last_level-1]/(26*60))%60,
                        (designertime[ud.volume_number*11+ud.last_level-1]/26)%60);
                    gametext((320>>2)+71,78+9,text,0,2+8+16);

                }
            }
            if( totalclock > (60*6) )
            {
                gametext(10,94+9,"Enemies Killed:",0,2+8+16);
                gametext(10,99+4+9,"Enemies Left:",0,2+8+16);

                if(bonuscnt == 2)
                {
                    bonuscnt++;
                    sound(FLY_BY);
                }

                if( totalclock > (60*7) )
                {
                    if(bonuscnt == 3)
                    {
                        bonuscnt++;
                        sound(PIPEBOMB_EXPLODE);
                    }
                    snprintf(text, sizeof(text),"%-3hhd",ps[myconnectindex].actors_killed);
                    gametext((320>>2)+70,93+9,text,0,2+8+16);
                    if(ud.player_skill > 3 )
                    {
                        snprintf(text, sizeof(text),"N/A");
                        gametext((320>>2)+70,99+4+9,text,0,2+8+16);
                    }
                    else
                    {
                        if( (ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed) < 0 )
                            snprintf(text, sizeof(text),"%-3d",0);
                        else snprintf(text, sizeof(text),"%-3d",ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed);
                        gametext((320>>2)+70,99+4+9,text,0,2+8+16);
                    }
                }
            }
            if( totalclock > (60*9) )
            {
                gametext(10,120+9,"Secrets Found:",0,2+8+16);
                gametext(10,130+9,"Secrets Missed:",0,2+8+16);
                if(bonuscnt == 4) bonuscnt++;

                if( totalclock > (60*10) )
                {
                    if(bonuscnt == 5)
                    {
                        bonuscnt++;
                        sound(PIPEBOMB_EXPLODE);
                    }
                    snprintf(text, sizeof(text),"%-3d",ps[myconnectindex].secret_rooms);
                    gametext((320>>2)+70,120+9,text,0,2+8+16);
                    if( ps[myconnectindex].secret_rooms > 0 )
                        snprintf(text, sizeof(text),"%-3d",(100*ps[myconnectindex].secret_rooms/ps[myconnectindex].max_secret_rooms));
                    snprintf(text, sizeof(text),"%-3d",ps[myconnectindex].max_secret_rooms-ps[myconnectindex].secret_rooms);
                    gametext((320>>2)+70,130+9,text,0,2+8+16);
                }
            }

            if(totalclock > 10240 && totalclock < 10240+10240)
                totalclock = 1024;

            if( KB_KeyWaiting() && totalclock > (60*2) )
            {
#if ENABLE_SCREENSHOT
                if( KB_KeyPressed( sc_F12 ) )
                {
                    KB_ClearKeyDown( sc_F12 );
                    takescreenshot();
                }
#endif

                if( totalclock < (60*13) )
                {
                    KB_FlushKeyboardQueue();
                    totalclock = (60*13);
                }
                else if( totalclock < (1000000000L))
                   totalclock = (1000000000L);
            }
        }
        else break;
        nextpage(SCREEN_HEIGHT);
        waitForUpdate(-1);
    }
#if LASTX_IN_STACK
    lastx = NULL;
    FX_StopAllSounds();
    MUSIC_StopSong();
    updateAudio();
#endif
}


void cameratext(short i)
{
    uint8_t  flipbits;
    int32_t x , y;
    if(!getHittypeTemp(i, 0))
    {
        uint8_t stackTile[MAX_RAM_FLAT_SIZE];
        clearBufferedTileImagePtr();
        setTempRamTileData(stackTile);
        rotatesprite(24<<16,33<<16,65536L,0,CAMCORNER,0,0,2,windowx1,windowy1,windowx2,windowy2);
        rotatesprite((320-26)<<16,34<<16,65536L,0,CAMCORNER+1,0,0,2,windowx1,windowy1,windowx2,windowy2);
        rotatesprite(22<<16,163<<16,65536L,512,CAMCORNER+1,0,0,2+4,windowx1,windowy1,windowx2,windowy2);
        rotatesprite((310-10)<<16,163<<16,65536L,512,CAMCORNER+1,0,0,2,windowx1,windowy1,windowx2,windowy2);
        if(totalclock&16)
            rotatesprite(46<<16,32<<16,65536L,0,CAMLIGHT,0,0,2,windowx1,windowy1,windowx2,windowy2);
        clearBufferedTileImagePtr();
    }
    else
    {
        flipbits = (totalclock<<1)&48;
        for(x=0;x<394;x+=64)
            for(y=0;y<200;y+=64)
                rotatesprite(x<<16,y<<16,65536L,0,STATIC,0,0,2+flipbits,windowx1,windowy1,windowx2,windowy2);
    }
}

void vglass(int32_t x,int32_t y,short a,short wn,short n)
{
    int32_t z, zincs;
    short sect;

    sect = constwall[wn].nextsector;
    if(sect == -1) return;
    zincs = ( getSector(sect)->floorz-getSector(sect)->ceilingz ) / n;

    for(z = getSector(sect)->ceilingz;z < getSector(sect)->floorz; z += zincs )
        EGS(sect,x,y,z-(TRAND&8191),GLASSPIECES+(z&(TRAND%3)),-32,36,36,a+128-(TRAND&255),16+(TRAND&31),0,-1,5);
}

void lotsofglass(short i,short wallnum,short n)
{
     int32_t j, xv, yv, z, x1, y1;
     short sect, a;

     sect = -1;

     if(wallnum < 0)
     {
        for(j=n-1; j >= 0 ;j--)
        {
            a = SA-256+(TRAND&511)+1024;
            EGS(SECT,SX,SY,SZ,GLASSPIECES+(j%3),-32,36,36,a,32+(TRAND&63),1024-(TRAND&1023),i,5);
        }
        return;
     }

     j = n+1;

     x1 = getWall(wallnum)->x;
     y1 = getWall(wallnum)->y;

     xv = getWall(getWall_point2(wallnum))->x-x1;
     yv = getWall(getWall_point2(wallnum))->y-y1;

     x1 -= ksgn(yv);
     y1 += ksgn(xv);

     xv /= j;
     yv /= j;

     for(j=n;j>0;j--)
         {
                  x1 += xv;
                  y1 += yv;

          updatesector(x1,y1,&sect);
          if(sect >= 0)
          {
              z = getSector(sect)->floorz-(TRAND&(klabs(getSector(sect)->ceilingz-getSector(sect)->floorz)));
              if( z < -(32<<8) || z > (32<<8) )
                  z = SZ-(32<<8)+(TRAND&((64<<8)-1));
              a = SA-1024;
              EGS(SECT,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,32+(TRAND&63),-(TRAND&1023),i,5);
          }
         }
}

void spriteglass(short i,short n)
{
    int32_t j, k, a, z;

    for(j=n;j>0;j--)
    {
        a = TRAND&2047;
        z = SZ-((TRAND&16)<<8);
        k = EGS(SECT,SX,SY,z,GLASSPIECES+(j%3),TRAND&15,36,36,a,32+(TRAND&63),-512-(TRAND&2047),i,5);
        setSprite_pal(k, getSprite_pal(i));
    }
}

void ceilingglass(short i,short sectnum,short n)
{
     int32_t j, xv, yv, z, x1, y1;
     short a,s, startwall,endwall;

     startwall = constsector[sectnum].wallptr;
     endwall = startwall+constsector[sectnum].wallnum;

     for(s=startwall;s<(endwall-1);s++)
     {
         x1 = getWall(s)->x;
         y1 = getWall(s)->y;

         xv = (getWall(s+1)->x-x1)/(n+1);
         yv = (getWall(s+1)->y-y1)/(n+1);

         for(j=n;j>0;j--)
         {
              x1 += xv;
              y1 += yv;
              a = TRAND&2047;
              z = getSector(sectnum)->ceilingz+((TRAND&15)<<8);
              EGS(sectnum,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,(TRAND&31),0,i,5);
          }
     }
}



void lotsofcolourglass(short i,short wallnum,short n)
{
     int32_t j, xv, yv, z, x1, y1;
     short sect = -1, a, k;

     if(wallnum < 0)
     {
        for(j=n-1; j >= 0 ;j--)
        {
            a = TRAND&2047;
            k = EGS(SECT,SX,SY,SZ-(TRAND&(63<<8)),GLASSPIECES+(j%3),-32,36,36,a,32+(TRAND&63),1024-(TRAND&2047),i,5);
            setSprite_pal(k, TRAND&15);
        }
        return;
     }

     j = n+1;
     x1 = getWall(wallnum)->x;
     y1 = getWall(wallnum)->y;

     xv = (getWall(getWall_point2(wallnum))->x-getWall(wallnum)->x)/j;
     yv = (getWall(getWall_point2(wallnum))->y-getWall(wallnum)->y)/j;

     for(j=n;j>0;j--)
         {
                  x1 += xv;
                  y1 += yv;

          updatesector(x1,y1,&sect);
          z = getSector(sect)->floorz-(TRAND&(klabs(getSector(sect)->ceilingz-getSector(sect)->floorz)));
          if( z < -(32<<8) || z > (32<<8) )
              z = SZ-(32<<8)+(TRAND&((64<<8)-1));
          a = SA-1024;
          k = EGS(SECT,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,32+(TRAND&63),-(TRAND&2047),i,5);
          setSprite_pal(k, TRAND&7);
         }
}

void SetupGameButtons( void )
{
   CONTROL_DefineFlag(gamefunc_Move_Forward,false);
   CONTROL_DefineFlag(gamefunc_Move_Backward,false);
   CONTROL_DefineFlag(gamefunc_Turn_Left,false);
   CONTROL_DefineFlag(gamefunc_Turn_Right,false);
   CONTROL_DefineFlag(gamefunc_Strafe,false);
   CONTROL_DefineFlag(gamefunc_Fire,false);
   CONTROL_DefineFlag(gamefunc_Open,false);
   CONTROL_DefineFlag(gamefunc_Run,false);
   CONTROL_DefineFlag(gamefunc_AutoRun,false);
   CONTROL_DefineFlag(gamefunc_Jump,false);
   CONTROL_DefineFlag(gamefunc_Crouch,false);
   CONTROL_DefineFlag(gamefunc_Look_Up,false);
   CONTROL_DefineFlag(gamefunc_Look_Down,false);
   CONTROL_DefineFlag(gamefunc_Look_Left,false);
   CONTROL_DefineFlag(gamefunc_Look_Right,false);
   CONTROL_DefineFlag(gamefunc_Strafe_Left,false);
   CONTROL_DefineFlag(gamefunc_Strafe_Right,false);
   CONTROL_DefineFlag(gamefunc_Aim_Up,false);
   CONTROL_DefineFlag(gamefunc_Aim_Down,false);
   CONTROL_DefineFlag(gamefunc_Weapon_1,false);
   CONTROL_DefineFlag(gamefunc_Weapon_2,false);
   CONTROL_DefineFlag(gamefunc_Weapon_3,false);
   CONTROL_DefineFlag(gamefunc_Weapon_4,false);
   CONTROL_DefineFlag(gamefunc_Weapon_5,false);
   CONTROL_DefineFlag(gamefunc_Weapon_6,false);
   CONTROL_DefineFlag(gamefunc_Weapon_7,false);
   CONTROL_DefineFlag(gamefunc_Weapon_8,false);
   CONTROL_DefineFlag(gamefunc_Weapon_9,false);
   CONTROL_DefineFlag(gamefunc_Weapon_10,false);
   CONTROL_DefineFlag(gamefunc_Inventory,false);
   CONTROL_DefineFlag(gamefunc_Inventory_Left,false);
   CONTROL_DefineFlag(gamefunc_Inventory_Right,false);
   CONTROL_DefineFlag(gamefunc_Holo_Duke,false);
   CONTROL_DefineFlag(gamefunc_Jetpack,false);
   CONTROL_DefineFlag(gamefunc_NightVision,false);
   CONTROL_DefineFlag(gamefunc_MedKit,false);
   CONTROL_DefineFlag(gamefunc_TurnAround,false);
   CONTROL_DefineFlag(gamefunc_SendMessage,false);
   CONTROL_DefineFlag(gamefunc_Map,false);
   CONTROL_DefineFlag(gamefunc_Shrink_Screen,false);
   CONTROL_DefineFlag(gamefunc_Enlarge_Screen,false);
   CONTROL_DefineFlag(gamefunc_Center_View,false);
   CONTROL_DefineFlag(gamefunc_Holster_Weapon,false);
   CONTROL_DefineFlag(gamefunc_Show_Opponents_Weapon,false);
   CONTROL_DefineFlag(gamefunc_Map_Follow_Mode,false);
   CONTROL_DefineFlag(gamefunc_See_Coop_View,false);
   CONTROL_DefineFlag(gamefunc_Mouse_Aiming,false);
   CONTROL_DefineFlag(gamefunc_Toggle_Crosshair,false);
   CONTROL_DefineFlag(gamefunc_Steroids,false);
   CONTROL_DefineFlag(gamefunc_Quick_Kick,false);
   CONTROL_DefineFlag(gamefunc_Next_Weapon,false);
   CONTROL_DefineFlag(gamefunc_Previous_Weapon,false);
   CONTROL_DefineFlag(gamefunc_Console,false);
}

/*
===================
=
= GetTime
=
===================
*/

int32_t GetTime(void)
   {
   return totalclock;
   }


/*
===================
=
= CenterCenter
=
===================
*/

void CenterCenter(void)
   {
   printf("Center the joystick and press a button\n");
   }

/*
===================
=
= UpperLeft
=
===================
*/

void UpperLeft(void)
   {
   printf("Move joystick to upper-left corner and press a button\n");
   }

/*
===================
=
= LowerRight
=
===================
*/

void LowerRight(void)
   {
   printf("Move joystick to lower-right corner and press a button\n");
   }

/*
===================
=
= CenterThrottle
=
===================
*/

void CenterThrottle(void)
   {
   printf("Center the throttle control and press a button\n");
   }

/*
===================
=
= CenterRudder
=
===================
*/

void CenterRudder(void)
{
   printf("Center the rudder control and press a button\n");
}

// FIX_00006: better naming system for screenshots + message when pic is taken.
//            Use ./screenshots folder. Screenshot code rerwritten. Faster and
//            makes smaller files. Doesn't freeze or lag the game anymore.
#if ENABLE_SCREENSHOT
void takescreenshot(void)
{
	char  szFilename[256];
	int i;
	char  score[20];
	time_t time4file;
	struct tm *tmHMS;
    char text[512];

	// xduke: Build a nice name w/ date and players name if in multi mode.
	time(&time4file);
	tmHMS = localtime(&time4file);

	sprintf(text, "Chocolate DukeNukem3D(v%d.%d) %.4d.%.2d.%.2d %.2dh%.2dm%.2ds",
			CHOCOLATE_DUKE_REV_X,
			CHOCOLATE_DUKE_REV_DOT_Y,
			tmHMS->tm_year+1900,
			tmHMS->tm_mon+1,
			tmHMS->tm_mday,
			tmHMS->tm_hour,
			tmHMS->tm_min,
			tmHMS->tm_sec);

	if(ud.multimode>1) // if more than 1 player, we add name. Then add score if DM
	{
        text[0] = '\0';
		strcat((char *)text, " [");
		for(i=connecthead;i>=0;i=connectpoint2[i])
		{
			if(!ud.user_name[i][0])
				strcat(text, "NoName");
			else
				strcat(text, &ud.user_name[i][0]);

			if(ud.m_coop==0 || ud.m_coop==2)  // if DM or DM No spawn. Add Score as well
			{
				strcat(text, "(");
                snprintf(score, sizeof(score), "%d",ps[i].frag-ps[i].fraggedself);
				strcat(text, score);
				strcat(text, ") vs ");
			}
			else
				strcat(text, " vs ");
		}
        text[strlen(text)-4]=0; // remove last vs
		strcat(text, "]");
	}
	strcat(text, ".bmp");


	// If this is a TC save it to the TC's directory
	if(getGameDir()[0] != '\0')
	{
		sprintf(szFilename, "%s\\%s", getGameDir(), SCREENSHOTPATH);
		mkdir(szFilename);
		sprintf(szFilename, "%s\\%s\\%s", getGameDir(), SCREENSHOTPATH, text);
	}
	// otherwise let's save it to the root.
	else
	{
		mkdir(SCREENSHOTPATH);
		sprintf(szFilename, "%s\\%s", SCREENSHOTPATH, text);
	}
#if 0
	if(SafeFileExists(szFilename) == 0)
	{
		screencapture(szFilename);
		sprintf(fta_quotes[103],"SCREEN SAVED");
		sound(EXITMENUSOUND);
	}
	else
		sprintf(fta_quotes[103],"CAN'T WRITE FILE! FILE ALREADY EXISTS!");

	FTA(103,&ps[screenpeek],1);
#endif
}
#endif

// Rare Multiplayer, when dead, total screen screwup back again!
// E3l1 (Coop /w monsters) sprite list corrupt 50%
// Univbe exit, instead, default to screen buffer.
// Check all caches bounds and memory usages
// Fix enlarger weapon selections to perfection
// Need sounds.c
// Spawning a couple of sounds at the same time
// Check Weapon Switching
// FIRE and FIRE2
// Where should I flash the screen white???
// Jittery on subs in mp?
// Check accurate memory amounts!
// Why squish sound at hit space when dead?
// Falling Counter Not reset in mp
// Wierd small freezer
// Double freeze on player?, still firing
// Do Mouse Flip option
// Save mouse aiming
// Laser bounce off mirrors
// GEORGE:   Ten in text screen.
// Alien:
// Freeze: change
// Press space holding player
// Press space
// tank broke
// 2d mode fucked in fake mp mode
// 207
// Mail not rolling up on conveyers
// Fix all alien animations
// Do episode names in .CONS
// do syntak check for "{?????"
// Make commline parms set approiate multiplayer flags

// Check all breakables to see if they are exploding properly
// Fix freezing palette on Alien

// Do a demo make run overnite
// Fix Super Duck
// Slime Guies, use quickkick.

// Make Lasers from trip bombs reflect off mirrors
// Remember for lockout of sound swears
// Pass sender in packed, NOT
// Fatal sync give no message for TEN
// Hitting TEN BUTTON(OPTION) no TEN SCreen
// Check multioperateswitches for se 31,32
// Fix pal for ceilings (SE#18)
// case 31: sprites up one high
// E1l1 No Kill All troops in room, sleep time

// Fifo for message list

// Bloodsplat on conveyers

// Meclanical
// Increase sound
// Mouse Delay at death
// Wierd slowdown

// Footprints on stuff floating

// Ken, The inside function is called a lot in -1 sectors
// No loading Univbe message rewrite
// Expander must cycle with rest of weapons
// Duck SHOOT PIPEBOMB, red wall

// Get commit source from mark

/*
1. fix pipebomb bug
2. check george maps
4. Save/Restore check (MP and SP)
5. Check TEN
6. Get Commit fixed
8. Is mail slow?
9. Cacheing
10. Blue out "PLAY ON TEN" in MULTIPLAYER
11. Eight Player test
12. Postal.voc not found.
13. All Monsters explode in arcade,
    check SEENINE STRENGTH,
    Change 28<<8 back to 16<<8 in hitradius
    Compare 1.3d to 1.4
14. Check sounds/gfx for for parr lock
15. Player # Loaded a game
16. Replace Crane code 1.3d to 1.4
17. Fix Greenslime
18. Small Freeze sprite,below floor
19. Vesa message auto abort in mp?
20. Fucked Palette in my skip ahead in MP
21. Load in main menu
22. Rotated frag screen no game screen
23. Jibs sounds when killed other dukes
24. Ten code and /f4 mode
25. Fix All MP Glitches!!
26. Unrem Menues anim tenbn
27. buy groc,clothes,scanner
28. Why Double Defs in global and game, is so at work
29. Check that all .objs are erased
30. Check why 1.3ds gotweapon gamedef coop code no workie
31. Heavy mods to net code
32. Make sure all commline stuff works,
33. killed all waitfor???
34. 90k stack
35. double door probs
36: copy protection
* when you start a game the duke saying that is played when you choose a skill the sound is cut off.
* NEWBEASTJUMPING is not deleted at premap in multi-play
if(*c == '4') no work need objs ask ken, commit
{
movesperpacket = 4;
setpackettimeout(0x3fffffff,0x3fffffff);
}
remember, netcode load
*/
//  Ai Problem in god mode.
// Checkplayerhurtwall for forcefields bigforce
// Nuddie, posters. IMF
// Release commit.c to public?
// Document Save bug with mp
// Check moves per packet /f4 waitforeverybody over net?
// Kill IDF OBJ
// No shotguns under water @ tanker
// Unrem copyprotect
// Look for printf and puts
// Check con rewrites
// erase mmulti.c, or get newest objs
// Why nomonsters screwy in load menu in mp
// load last > 'y' == NOT
// Check xptr oos when dead rising to surface.
//    diaginal warping with shotguns
// Test white room.  Lasertripbomb arming crash
// The Bog
// Run Duke Out of windows
// Put Version number in con files
// Test diff. version playing together
// Reorganize dukecd
// Put out patch w/ two weeks testing
// Print draw3d
// Double Klick

/*
Duke Nukem V

Layout:

      Settings:
        Suburbs
          Duke inflitrating neighborhoods inf. by aliens
        Death Valley:
          Sorta like a western.  Bull-skulls half buried in the sand
          Military compound:  Aliens take over nuke-missle silo, duke
            must destroy.
          Abondend Aircraft field
        Vegas:
          Blast anything bright!  Alien lights camoflauged.
          Alien Drug factory. The Blue Liquid
        Mountainal Cave:
          Interior cave battles.
        Jungle:
          Trees, canopee, animals, a mysterious hole in the earth with
          gas seaping thru.
        Penetencury:
          Good use of spotlights:
        Mental ward:
          People whom have claimed to be slowly changing into an
          alien species

      Inventory:
        Wood,
        Metal,
        Torch,
        Rope,
        Plastique,
        Cloth,
        Wiring,
        Glue,
        Cigars,
        Food,
        Duck Tape,
        Nails,
        Piping,
        Petrol,
        Uranium,
        Gold,
        Prism,
        Power Cell,

        Hand spikes (Limited usage, they become dull)
        Oxygent     (Oxygen mixed with stimulant)


      Player Skills:
        R-Left,R-Right,Foward,Back
        Strafe, Jump, Double Flip Jump for distance
        Help, Escape
        Fire/Use
        Use Menu

    After a brief resbit, Duke decides to get back to work.

Cmdr:   "Duke, we've got a lot of scared people down there.
         Some reports even claim that people are already
         slowly changing into aliens."
Duke:   "No problem, my speciality is in croud control."
Cmdr:   "Croud control, my ass!  Remember that incident
         during the war?  You created nuthin' but death and
         destruction."
Duke:   "Not destruction, justice."
Cmdr:   "I'll take no responsibility for your actions.  Your on
         your own!  Behave your self, damnit!  You got that,
         soldger?"
Duke:   "I've always been on my own...   Face it, it's ass kickin' time,
         SIR!"
Cmdr:   "Get outta here...!"
        (Duke gives the Cmdr a hard stair, then cocks his weapon and
         walks out of the room)
Cmdr:   In a wisper: "Good luck, my friend."

        (Cut to a scene where aliens are injecting genetic material
         into an unconcious subject)

Programming:   ( the functions I need )
     Images: Polys
     Actors:
       Multi-Object sections for change (head,arms,legs,torsoe,all change)
       Facial expressions.  Pal lookup per poly?

     struct imagetype
        {
            int *itable; // AngX,AngY,AngZ,Xoff,Yoff,Zoff;
            int *idata;
            struct imagetype *prev, *next;
        }

*/


// Test frag screen name fuckup
// Test all xptrs
// Make Jibs stick to ceiling
// Save Game menu crash
// Cache len sum err
// Loading in main (MP), reset totalclock?
// White Room
// Sound hitch with repeat bits
// Rewrite saved menues so no crash
// Put a getpackets after loadplayer in menus
// Put "loading..." before waitfor in loadpla
// No ready2send = 0 for loading
// Test Joystick
// Ten
// Bog
// Test Blimp respawn
// move 1 in player???


