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
#ifndef _INCL_DUKE3D_H_
#define _INCL_DUKE3D_H_
#include "maincfg.h"
#include "build.h"
#define MAX_SAVEGAMES               10
#define MAX_SAVEGAME_NAME_LENGTH    18 //excluding null terminator
#define BOARD_FILENAME_LENGTH       32
#define PREVIEW_SCREENSHOT_WIDTH    160
#define PREVIEW_SCREENSHOT_HEIGHT   100
// next-hack: changed direct numeric constants with enum, so that it is easier to understand and expand.
enum
{
    MENU_MAIN = 0,
    MENU_INGAME_MENU = 50,
    MENU_EPISODE_SELECT = 100,
    MENU_USER_MAP = 101,
    MENU_SKILL_SELECT = 110,
    MENU_OPTIONS = 200,
    MENU_SAVE_OPTIONS_CONFIRM = 201,
    MENU_OPTIONS_SAVED = 202,
    MENU_LOAD_GAME = 300,
    MENU_PRE_SAVE_GAME = 350,         // it will jump to MENU_SAVE_GAME_SELECT
    MENU_SAVE_GAME_SELECT = 351,
    MENU_ENTER_SAVE_GAME_NAME0 = 360,
    MENU_ENTER_SAVE_GAME_NAME1 = 361,
    MENU_ENTER_SAVE_GAME_NAME2 = 362,
    MENU_ENTER_SAVE_GAME_NAME3 = 363,
    MENU_ENTER_SAVE_GAME_NAME4 = 364,
    MENU_ENTER_SAVE_GAME_NAME5 = 365,
    MENU_ENTER_SAVE_GAME_NAME6 = 366,
    MENU_ENTER_SAVE_GAME_NAME7 = 367,
    MENU_ENTER_SAVE_GAME_NAME8 = 368,
    MENU_ENTER_SAVE_GAME_NAME9 = 369,

    MENU_HELP0 = 400,
    MENU_HELP1 = 401,
    MENU_HELP2 = 402,
    MENU_HELP3 = 403,
    MENU_KEYBOARD_QUIT = 500,   // pressing f10
    MENU_QUIT = 501,
    MENU_QUIT_GAME = 502,
    MENU_QUIT_TO_TITLE = 503,    // from game

    MENU_MULTI_HOST_SETUP = 600,
    MENU_MULTI_WAITING_FOR_OTHER_PLAYERS_READY_MESSAGE = 601,
    MENU_MULTI_READ_USER_MAPS = 602,
    MENU_MULTI_SELECT_USER_MAPS = 603,
    MENU_MULTI_SETUP = 604,
    MENU_MULTI_ENTER_NAME = 605,
    MENU_MULTI_PEER_LOBBY = 606,
    MENU_MULTI_PEER_READY = 607,
    MENU_SOUND_SETUP = 700,
    MENU_STICK_SETUP = 701,
    MENU_GAME_OPTIONS = 702,
    MENU_KEYBOARD_SETUP = 703,
    MENU_MOUSE_BUTTON_SETUP = 704,
    MENU_ANALOG_AXES_SETUP = 705,
    MENU_VIDEO_SETUP = 706,
    MENU_CREDITS0 = 990,
    MENU_CREDITS1 = 991,
    MENU_CREDITS2 = 992,
    MENU_CREDITS3 = 993,
    MENU_CREDITS4 = 994,
    MENU_CREDITS5 = 995,
    MENU_CREDITS6 = 996,
    MENU_CREDITS7 = 997,
    MENU_LOAD_GAME0 = 1000,
    MENU_LOAD_GAME1 = 1001,
    MENU_LOAD_GAME2 = 1002,
    MENU_LOAD_GAME3 = 1003,
    MENU_LOAD_GAME4 = 1004,
    MENU_LOAD_GAME5 = 1005,
    MENU_LOAD_GAME6 = 1006,
    MENU_LOAD_GAME7 = 1007,
    MENU_LOAD_GAME8 = 1008,
    MENU_LOAD_GAME9 = 1009,

    MENU_CANNOT_LOAD_GAME_MESSAGE = 1010,
    MENU_ABORT_THIS_GAME_QUESTION = 1500,
    MENU_OVERWRITE_SAVE_GAME0_QUESTION = 2000,
    MENU_OVERWRITE_SAVE_GAME1_QUESTION = 2001,
    MENU_OVERWRITE_SAVE_GAME2_QUESTION = 2002,
    MENU_OVERWRITE_SAVE_GAME3_QUESTION = 2003,
    MENU_OVERWRITE_SAVE_GAME4_QUESTION = 2004,
    MENU_OVERWRITE_SAVE_GAME5_QUESTION = 2005,
    MENU_OVERWRITE_SAVE_GAME6_QUESTION = 2006,
    MENU_OVERWRITE_SAVE_GAME7_QUESTION = 2007,
    MENU_OVERWRITE_SAVE_GAME8_QUESTION = 2008,
    MENU_OVERWRITE_SAVE_GAME9_QUESTION = 2009,

    MENU_PARENTAL_LOCK = 10000,
    MENU_PARENTAL_ENTER_PASSWORD = 10001,
    MENU_LOAD_LAST_GAME_QUESTION0 = 15000,
    MENU_LOAD_LAST_GAME_QUESTION1 = 15001,
    MENU_MESSAGE_SHAREWARE = 20000,
    MENU_MESSAGE_SELECT_SAVE_SPOT = 25000,
};
#if (!defined MAX_PATH)
  #if (defined MAXPATHLEN)
    #define MAX_PATH MAXPATHLEN
  #elif (defined PATH_MAX)
    #define MAX_PATH PATH_MAX
  #else
    #define MAX_PATH 256
  #endif
#endif

#if UNIX
#include "dukeunix.h"
#endif

#if PLATFORM_MACOSX
#include "dukeunix.h"
#endif

#if WIN32
#include "dukewin.h"
#endif


//  FIX_00022: Automatically recognize the shareware grp (v1.3) + full version (1.3d) +
//             atomic (1.4/1.5 grp) and the con files version (either 1.3 or 1.4) (JonoF's idea)

extern uint8_t  conVersion;
extern uint8_t  grpVersion;


#define RANCID_ID 1
#define	XDUKE_ID  2
#define JONOF_ID  3

//Chocolate DukeNukem3D is a fork of xDuke v17.9

#define DUKE_ID			XDUKE_ID
#define	CHOCOLATE_DUKE_REV_X		1
#define CHOCOLATE_DUKE_REV_DOT_Y	0   // rev is: CHOCOLATE_DUKE_REV_X.CHOCOLATE_DUKE_REV_DOT_Y

#define MAX_KNOWN_GRP 4

enum
{
	UNKNOWN_GRP = 0,
	SHAREWARE_GRP13,
	REGULAR_GRP13D,
	ATOMIC_GRP14_15,
	DUKEITOUTINDC_GRP
};

#define CRC_BASE_GRP_SHAREWARE_13	0x983AD923
#define CRC_BASE_GRP_FULL_13		0xBBC9CE44
#define CRC_BASE_GRP_PLUTONIUM_14	0xF514A6AC
#define CRC_BASE_GRP_ATOMIC_15		0xFD3DCFF1

// implies  conVersion == 14 or conVersion == 15

#define PLUTOPAK  (grpVersion == ATOMIC_GRP14_15 && ATOMIC_EDITION)
#define VOLUMEONE (grpVersion == SHAREWARE_GRP13)
// VOLUMEALL = 1.3d full
#define VOLUMEALL (grpVersion == REGULAR_GRP13D)

#define SCREENSHOTPATH "screenshots"



// #define TEN
// #define BETA

// #define AUSTRALIA

#define MAXSLEEPDIST  16384
#define SLEEPTIME 24*64

extern int BYTEVERSION;

extern int BYTEVERSION_27;
extern int BYTEVERSION_28;
extern int BYTEVERSION_29; // really needed???
extern int BYTEVERSION_116;
extern int BYTEVERSION_117;
extern int BYTEVERSION_118;
extern int BYTEVERSION_1_3;

extern int32_t lastvisinc;

#define NUMPAGES 1

#define AUTO_AIM_ANGLE          48
#define RECSYNCBUFSIZ 2520   //2520 is the (LCM of 1-8)*3
#define MOVEFIFOSIZ 32         // 32 is ok. let's keep small. Was 256

#define FOURSLEIGHT (1<<8)

struct player_struct;

#include "../../Game/src/types.h"
#include "gamedefs.h"


#if WIN32
#define updateAudio()
#else
#define TEST_AUDIO_CRASH 0
#if TEST_AUDIO_CRASH
#define updateAudio() updateAudio2(__LINE__, __FILE__)
void updateAudio2(int line, char *file);
#else
void updateAudio(void);
#endif
#endif


//#define TICRATE (120)
//#define TICSPERFRAME (TICRATE/26)

extern int g_iTickRate;
extern int g_iTicksPerFrame; // the name of this variable is completely nonsense. It should be frames per second.

#define TICRATE g_iTickRate
#define TICSPERFRAME (TICRATE/g_iTicksPerFrame)   // this definition has the correct meaning.

// #define GC (TICSPERFRAME*44)

#if ATOMIC_EDITION
#define NUM_SOUNDS 400
#else
#define NUM_SOUNDS 320 //450
#endif
#define    ALT_IS_PRESSED ( KB_KeyPressed( sc_RightAlt ) || KB_KeyPressed( sc_LeftAlt ) )
#define    SHIFTS_IS_PRESSED ( KB_KeyPressed( sc_RightShift ) || KB_KeyPressed( sc_LeftShift ) )
// next-hack: modified RANDOMSCRAP to be a function-like macro
#define    RANDOMSCRAP(i , spritei)    \
                do           \
                {            \
                        EGS(spritei->_sectnum, \
                            spritei->_x+(TRAND&255)-128, \
                            spritei->_y+(TRAND&255)-128, \
                            spritei->_z-(8<<8)-(TRAND&8191),\
                            SCRAP6+(TRAND&15),-8,48,48,\
                            TRAND&2047,(TRAND&63)+64,-512-(TRAND&2047),i,5);\
                } while(0);
#define    BLACK 0
#define    DARKBLUE 1
#define    DARKGREEN 2
#define    DARKCYAN 3
#define    DARKRED 4
#define    DARKPURPLE 5
#define    BROWN 6
#define    LIGHTGRAY 7

#define    DARKGRAY 8
#define    BLUE 9
#define    GREEN 10
#define    CYAN 11
#define    RED 12
#define    PURPLE 13
#define    YELLOW 14
#define    WHITE 15

#define    PHEIGHT (38<<8)

#define WAIT(X) ototalclock=totalclock+(X);while(totalclock<ototalclock)


#define MODE_MENU       1
#define MODE_DEMO       2
#define MODE_GAME       4
#define MODE_EOL        8
#define MODE_TYPE       16
#define MODE_RESTART    32
#define MODE_SENDTOWHOM 64
#define MODE_END        128
#define MODE_RELOAD_FROM_DEAD       256


#define MAXANIMWALLS 512
#define MAXINTERPOLATIONS 1300//1300//1250// was 2048. 1024 is not enough for E2L8. 1300 needed for E3L3
#define NUMOFFIRSTTIMEACTIVE 128

#define MAXCYCLERS 112// was 256. The limit is 111
#if !ATOMIC_EDITION
#define MAXSCRIPTSIZE   12132
#define LABEL_CNT       1245
#else
#define MAXSCRIPTSIZE 16207
#define LABEL_CNT 1795
#endif
#define MAXANIMATES 64
#if !MMULTI_FILE
#define SP  getSprite(i)->_yvel
#define SX  getSprite_x(i) //getSprite(i)->_x
#define SY  getSprite_y(i) // getSprite(i)->_y
#define SZ  getSprite_z(i) //getSprite(i)->_z
#define SS  getSprite_shade(i) //getSprite(i)->_shade
#define PN  getSprite_picnum(i) //getSprite(i)->_picnum
#define SA  getSprite_ang(i) //getSprite(i)->_ang
#define SV  getSprite(i)->_xvel
#define ZV  getSprite(i)->_zvel
#define RX  getSprite(i)->_xrepeat
#define RY  getSprite(i)->_yrepeat
#define OW  getSprite_owner(i) //getSprite(i)->_owner
#define CS  getSprite_cstat(i) //getSprite(i)->_cstat
#define SH  getSprite_extra(i)
#define CX  getSprite(i)->_xoffset
#define CY  getSprite(i)->_yoffset
#define CD  getSprite(i)->_clipdist
#define PL  getSprite_pal(i)
#define SLT  getSprite_lotag(i) //getSprite(i)->_lotag
#define SHT  getSprite_hitag(i) //getSprite(i)->_hitag
#define SECT getSprite_sectnum(i)//getSprite(i)->_sectnum
#endif
// this is how a macro should have been done:
#define setSprite_xy_repeats(n, x, y) do {setSprite_xrepeat(n, x); setSprite_yrepeat(n, y);} while(0)

#define face_player 1
#define geth 2
#define getv 4
#define random_angle 8
#define face_player_slow 16
#define spin 32
#define face_player_smart 64
#define fleeenemy 128
#define jumptoplayer 257
#define seekplayer 512
#define furthestdir 1024
#define dodgebullet 4096

#define TRAND krand()

#define MAX_WEAPONS  12

#define KNEE_WEAPON          0
#define PISTOL_WEAPON        1
#define SHOTGUN_WEAPON       2
#define CHAINGUN_WEAPON      3
#define RPG_WEAPON           4
#define HANDBOMB_WEAPON      5
#define SHRINKER_WEAPON      6
#define DEVISTATOR_WEAPON    7
#define TRIPBOMB_WEAPON      8
#define FREEZE_WEAPON        9
#define HANDREMOTE_WEAPON    10
#define GROW_WEAPON          11

// next-hack: This is CRAP. Why was it done, at the first place this crime against humanity ?!? Oh boy, let's undo this.
// Reason: All-uppercase tokens, not macro-style should be constants. writing T1 = something; is BAD. Writing something = T1; is bad as well,
// the reviewer misses the fact that actually you are taking something which depends on i.
//#define T1  getHittypeTempPtr(i)[0]
//#define T2  getHittypeTemp(i, 1)
//#define T3  getHittypeTemp(i, 2)
//#define T4  getHittypeTemp(i, 3)
//#define T5  getHittypeTemp(i, 4)
//#define T6  getHittypeTemp(i, 5)

// All these macros are COMPLETE CRAP!
#define IFWITHIN(B,E) if((PN)>=(B) && (PN)<=(E))
#define KILLIT(KX) {deletesprite(KX);goto BOLT;}


#define IFMOVING if(ssp(i,CLIPMASK0))
#define IFHIT j=ifhitbyweapon(i);if(j >= 0)
#define IFHITSECT j=ifhitsectors(s->sectnum);if(j >= 0)

#define AFLAMABLE(X) (X==BOX||X==TREE1||X==TREE2||X==TIRE||X==CONE)


#define IFSKILL1 if(player_skill<1)
#define IFSKILL2 if(player_skill<2)
#define IFSKILL3 if(player_skill<3)
#define IFSKILL4 if(player_skill<4)

#define rnd(X) ((TRAND>>8)>=(255-(X)))
#if USE_SOUNDOWNER
typedef struct
{
#if NO_MINIMIZE_SOUNDS
    short i;
    int voice;
#else
   int16_t i : 12; // can be negative. Can be at most MAX_SPRITES
   uint16_t voice : 4;
#endif
} SOUNDOWNER;
#endif
#include "audiolib/usrhooks.h"
/*


#define __USRHOOKS_H

enum USRHOOKS_Errors
   {
   USRHOOKS_Warning = -2,
   USRHOOKS_Error   = -1,
   USRHOOKS_Ok      = 0
   };
*/

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    int8_t avel, horz;
    short fvel, svel;
    uint32_t bits;
} input;

#pragma pack(pop)

/* !!! FIXME: "sync" is defined in unistd.h ... :(  --ryan. */
#define sync duke_sync
extern input inputfifo[MOVEFIFOSIZ][MAXPLAYERS], sync[MAXPLAYERS];
extern input recsync[RECSYNCBUFSIZ];

extern int32_t movefifosendplc;

typedef struct
{
    uint8_t  *ptr;
    int length;
#if USE_CACHE
    volatile uint8_t  lock; // tanguyf: if not volatile, the game can go into an infinite loop in optimised mode
#endif
} SAMPLE;

struct animwalltype
{
        short wallnum;
        short tag; //int32_t tag;    // next-hack: this is set to picnum or -1. So let's use short!
};
extern struct animwalltype animwall[MAXANIMWALLS];
extern short numanimwalls,probey,lastprobey;

extern char  *mymembuf;
extern uint8_t  typebuflen;
extern char typebuf[41];
extern uint8_t  MusicPtr[72000];
#define MAX_MOVABLE_SECTORS  400 // was 2048. 256 is not ok for E2L8
extern int32_t msx[MAX_MOVABLE_SECTORS],msy[MAX_MOVABLE_SECTORS];
extern short cyclers[MAXCYCLERS][6],numcyclers;
extern char  myname[16]; // myname[2048] c'mon, really ?

#pragma pack(push, 4)
#if LETS_WASTE_RAM_FOR_USERDEFS
struct user_defs
{
    uint8_t  god,warp_on,cashman,eog,showallmap;
    uint8_t  show_help,scrollmode,clipping;
    char  user_name[MAXPLAYERS][32];
    char  ridecule[10][40];
    char  savegame[10][22];
    char  pwlockout[128],rtsname[128];
    uint8_t  overhead_on,last_overhead;

    short pause_on,from_bonus;
    short camerasprite,last_camsprite;
    short last_level,secretlevel;

    int32_t const_visibility,uw_framerate;
    int32_t camera_time,folfvel,folavel,folx,foly,fola;
    int32_t reccnt;

    int32_t entered_name,screen_tilting,shadows,fta_on,executions,auto_run;
    int32_t coords,tickrate,m_coop,coop,screen_size,extended_screen_size,lockout,crosshair,showweapons;
    int32_t mywchoice[MAX_WEAPONS],wchoice[MAXPLAYERS][MAX_WEAPONS],playerai;

    int32_t respawn_monsters,respawn_items,respawn_inventory,recstat,monsters_off,brightness;
    int32_t m_respawn_items,m_respawn_monsters,m_respawn_inventory,m_recstat,m_monsters_off,detail;
	// FIX_00082: /q option taken off when playing a demo (multimode_bot)
    int32_t m_ffire,ffire,m_player_skill,m_level_number,m_volume_number,multimode,multimode_bot;
    int32_t player_skill,level_number,volume_number,m_marker,marker,mouseflip;

    int32_t showcinematics, hideweapon;
    int32_t auto_aim, gitdat_mdk; //AutoAim toggle variable.
    int32_t weaponautoswitch;

	// FIX_00015: Backward compliance with older demos (down to demos v27, 28, 116 and 117 only)
	uint8_t  playing_demo_rev;

	uint32_t groupefil_crc32[MAXPLAYERS][4];
	uint16_t conSize[MAXPLAYERS];

#ifdef CHECK_XDUKE_REV
	uint8_t  rev[MAXPLAYERS][10];
#endif
	uint32_t mapCRC[MAXPLAYERS];
	uint32_t exeCRC[MAXPLAYERS];
	uint32_t conCRC[MAXPLAYERS];
};
#else
#define MAX_NAME_LEN                16
struct user_defs
{
    uint8_t  god,warp_on,cashman,eog,showallmap;
    uint8_t  show_help,scrollmode,clipping;
    char  user_name[MAXPLAYERS][MAX_NAME_LEN];
//    char  ridecule[10][40];
    char  savegame[MAX_SAVEGAMES][MAX_SAVEGAME_NAME_LENGTH + 1];
//    char  pwlockout[128],rtsname[128];
    uint8_t  overhead_on,last_overhead;

    short pause_on,from_bonus;
    short camerasprite,last_camsprite;
    short last_level,secretlevel;

    int16_t const_visibility,uw_framerate;
    int32_t camera_time,folfvel,folavel,folx,foly,fola;
    int8_t reccnt;

    int8_t screen_tilting,shadows,fta_on,executions,auto_run;
    int8_t coords,tickrate,m_coop,coop,screen_size,extended_screen_size,lockout,crosshair,showweapons;
    int8_t mywchoice[MAX_WEAPONS],wchoice[MAXPLAYERS][MAX_WEAPONS],playerai;

    int8_t respawn_monsters,respawn_items,respawn_inventory,recstat,monsters_off,brightness;
    int8_t m_respawn_items,m_respawn_monsters,m_respawn_inventory,m_recstat,m_monsters_off,detail;
  // FIX_00082: /q option taken off when playing a demo (multimode_bot)
    int8_t m_ffire,ffire,m_player_skill,m_level_number,m_volume_number,multimode,multimode_bot;
    uint8_t player_skill,level_number,volume_number,m_marker,marker,mouseflip;

    uint8_t showcinematics, hideweapon;
    int8_t auto_aim, gitdat_mdk; //AutoAim toggle variable.
    int8_t weaponautoswitch;
    uint8_t m_channel;          // for multiplayer
    uint8_t multiReady;

  // FIX_00015: Backward compliance with older demos (down to demos v27, 28, 116 and 117 only)
  uint8_t  playing_demo_rev;

  //uint32_t groupefil_crc32[MAXPLAYERS][4];
  //uint16_t conSize[MAXPLAYERS];

#ifdef CHECK_XDUKE_REV
  uint8_t  rev[MAXPLAYERS][10];
#endif
  //uint32_t mapCRC[MAXPLAYERS];
  //uint32_t exeCRC[MAXPLAYERS];
  //uint32_t conCRC[MAXPLAYERS];
};
#endif
#pragma pack(pop)

struct player_orig
{
    int32_t ox,oy,oz;
    short oa,os;
};


extern uint8_t  numplayersprites;


typedef struct
{
	unsigned int crc32;
	char * name;
	uint32_t size;
} crc32_t;

extern const crc32_t crc32lookup[];

void add_ammo( short, short, short, short );


extern int32_t fricxv,fricyv;

#pragma pack(push, 4)
struct player_struct
{
    // it looks like loogiex and loogiey can be much less than 64+64. Like 6+6.
    int16_t zoom;
    int8_t numloogs;
    int8_t aim_mode;  // can be smaller

    int32_t exitx,exity,loogiex[6],loogiey[6],loogcnt;
    int32_t posx, posy, posz, horiz, ohoriz, ohorizoff, invdisptime;
    int32_t bobposx,bobposy,oposx,oposy,oposz,pyoff,opyoff;
    int32_t posxv,posyv,poszv,last_pissed_time,truefz,truecz;
    int32_t player_par, visibility;
    int32_t bobcounter, weapon_sway;
    int32_t pals_time,randomflamex,crack_time;


    short ang,oang,angvel,cursectnum,look_ang,last_extra,subweapon;
    short ammo_amount[MAX_WEAPONS],wackedbyactor,frag,fraggedself;

    short curr_weapon, last_weapon, tipincs, horizoff, wantweaponfire;
    short holoduke_amount,newowner,hurt_delay,hbomb_hold_delay;
    short jumping_counter,airleft,knee_incs,access_incs;
    short fta,ftq,access_wallnum,access_spritenum;
    short kickback_pic,got_access,weapon_ang,firstaid_amount;
    short somethingonplayer,on_crane,i,one_parallax_sectnum;
    short over_shoulder_on,random_club_frame,fist_incs;
    short one_eighty_count,cheat_phase;
    short dummyplayersprite,extra_extra8,quick_kick;
    short heat_amount,actorsqu,timebeforeexit,customexitsound;

    short weaprecs[16],weapreccnt;
	uint32_t interface_toggle_flag;

    short rotscrnang,dead_flag,show_empty_weapon;
    short scuba_amount,jetpack_amount,steroids_amount,shield_amount;
    short holoduke_on,pycount,weapon_pos,frag_ps;
    short transporter_hold,last_full_weapon,footprintshade,boot_amount;

    int scream_voice;
    uint16_t gm;
    uint8_t  on_warping_sector,footprintcount;
    uint8_t  hbomb_on,jumping_toggle,rapid_fire_hold,on_ground;
    uint8_t  name[16],inven_icon,buttonpalette; // name can be smaller

    uint8_t  jetpack_on,spritebridge,lastrandomspot;
    uint8_t  scuba_on,footprintpal,heat_on;

    uint8_t   holster_weapon,falling_counter;
    uint8_t   gotweapon[MAX_WEAPONS],refresh_inventory; // got weapon could be a bitfield of 16 bits

	// tanguyf: fix the palette pointer size being 32/64-bit dependent. For some reason this is serialised into savegames.
    union
    {
	    const uint8_t* palette;
#if WIN32
        uint64_t _palette_padding;  // can be removed for MG24
#endif
    };

    uint8_t  toggle_key_flag,knuckle_incs; // ,select_dir;
    uint8_t  walking_snd_toggle, palookup, hard_landing;
    uint8_t  max_secret_rooms,secret_rooms,/*fire_flag,*/pals[3];
    uint8_t  max_actors_killed,actors_killed,return_to_center;

	// local but synch variables (ud is local but not synch):

	// FIX_00023: Moved Addfaz's autoaim handler to synch variables (to avoid out of synch)
    int32_t auto_aim; //AutoAim toggle variable. Can be 2 bit.

	// FIX_00012: added "weapon autoswitch" toggle allowing to turn the autoswitch off
	//            when picking up new weapons. The weapon sound on pickup will remain on, to not
	//           affect the opponent's gameplay (so he can still hear you picking up new weapons)
    int32_t weaponautoswitch;   // can be 1 bit

	uint8_t fakeplayer; // can be 2 bits
};
#pragma pack(pop)

//extern uint8_t  tempbuf[2048];  // do we need this big?
extern uint8_t packbuf[];
#if LOAD_FROM_CON_FILE
extern int32_t gc,max_player_health,max_armour_amount,max_ammo_amount[MAX_WEAPONS];
extern int32_t impact_damage,respawnactortime,respawnitemtime;

#else
extern const int32_t gc,max_player_health,max_armour_amount,max_ammo_amount[MAX_WEAPONS];
extern const int32_t impact_damage,respawnactortime,respawnitemtime;

#endif


#define MAX_SPRITEQ 64      // to prevent 1024 sprites...
extern short spriteq[MAX_SPRITEQ],spriteqloc,spriteqamount;
extern struct player_struct ps[MAXPLAYERS];
extern struct player_orig po[MAXPLAYERS];
extern struct user_defs ud;

extern short int global_random;
extern int32_t scaredfallz;
extern uint8_t  buf[80]; //My own generic input buffer

extern uint8_t  scantoasc[128],ready2send;
extern uint8_t  scantoascwithshift[128];
#if USE_EXT_MEM
extern const SAMPLE *Sound;
#else
extern SAMPLE Sound[ NUM_SOUNDS ];
#endif
extern int32_t VoiceToggle,AmbienceToggle, OpponentSoundToggle;
#if USE_SOUNDOWNER
extern SOUNDOWNER SoundOwner[NUM_SOUNDS][4];
#endif
extern uint8_t  playerreadyflag[MAXPLAYERS],playerquitflag[MAXPLAYERS];
#if LOAD_FROM_CON_FILE
extern char  fta_quotes[NUMOFFIRSTTIMEACTIVE][64];
extern char  sounds[NUM_SOUNDS][14];
extern int32_t script[MAXSCRIPTSIZE];
#else
extern const char  sounds[NUM_SOUNDS][14];
extern const int32_t script[MAXSCRIPTSIZE];
extern const char  fta_quotes[NUMOFFIRSTTIMEACTIVE][38];


#endif
extern const int32_t *scriptptr,*insptr,*labelcode;
extern int32_t labelcnt;
extern char  *label,*textptr,error,warning;
extern uint8_t killit_flag;
// note: actorscrptr and type can be as few as 3450
#if USE_ACTORSCRPTR
extern int32_t*actorscrptr[MAXTILES];
#else
#if LOAD_FROM_CON_FILE
extern int16_t actorscridx[MAXTILES];
#else
extern const int16_t actorscridx[MAXTILES];

#endif
#endif
#if LOAD_FROM_CON_FILE
extern uint8_t  actortype[MAXTILES];
#else
#if !USE_ACTORTYPE_ARRAY
// actor type is almost all 0.
extern uint8_t getActorType(int idx);
#else
extern const uint8_t  actortype[MAXTILES];
#endif
#endif
extern int32_t *parsing_actor;
extern uint8_t  *music_pointer;

extern uint8_t  ipath[80],opath[80];
#if LOAD_FROM_CON_FILE
extern char  music_fn[4][11][13];
extern char  env_music_fn[4][13];
#else
extern const char  music_fn[4][11][13];
extern const char  env_music_fn[4][13];
#endif
extern uint8_t music_select;
extern short camsprite;

// extern uint8_t  gotz;
extern uint8_t  inspace(short sectnum);


#pragma pack(push, 4)
#if 0
struct weaponhit
{
    uint8_t  cgg;   // it is 0... 6 : 3 bit
    short picnum,ang,extra,owner,movflag;
    short tempang,actorstayput,dispicnum;
    short timetosleep;
    int32_t floorz,ceilingz,lastvx,lastvy,bposx,bposy,bposz;
    int32_t temp_data[6];
};
#else
// this structure groups temp data, which is often used.

#if USE_SEPARATE_HITTYPE_TEMPDATA
#define MAX_HITTYPE_TEMPDATA0       256
#define MAX_HITTYPE_TEMPDATA1       512 // most used
#define MAX_HITTYPE_TEMPDATA2       256
#define MAX_HITTYPE_TEMPDATA3       256
#define MAX_HITTYPE_TEMPDATA45      256

#else

#define MAX_HITTYPE_TEMPDATA  512//400
#endif
#define MAX_HITTYPE_OTHERDATA 464// 350// might not be enough ?

#if !USE_SEPARATE_HITTYPE_TEMPDATA
typedef struct
{
#if !OPTIMIZED_HITTYPE_TEMPDATA
// 24 bytes
    int32_t temp_data[6];
#else
// 20 bytes
    int32_t temp_data[4];
    int32_t temp_data4: 17;
    int32_t temp_data5: 15;
#endif
} weaponhit_tempdata_t;
#endif
// this structure groups data seldom used.
typedef struct  // 12 bytes
{
    int32_t lastvx; // can be as large as 1<<30
    //
    uint32_t tempang : 11;
    int32_t lastvy : 21;    // min 18 bits

    uint32_t ang : 11;
    int32_t timetosleep : 21; // min 11 bits if unsigned
} weaponhit_otherdata_t;
#if !USE_SEPARATE_HITTYPE_TEMPDATA
extern uint8_t hittype_tempdata_allocated_bf[(MAX_HITTYPE_TEMPDATA + 7) / 8];
extern weaponhit_tempdata_t hittype_tempdata[MAX_HITTYPE_TEMPDATA];
#else
extern uint8_t hittype_tempdata0_allocated_bf[(MAX_HITTYPE_TEMPDATA0 + 7) / 8];
extern uint8_t hittype_tempdata1_allocated_bf[(MAX_HITTYPE_TEMPDATA1 + 7) / 8];
extern uint8_t hittype_tempdata2_allocated_bf[(MAX_HITTYPE_TEMPDATA2 + 7) / 8];
extern uint8_t hittype_tempdata3_allocated_bf[(MAX_HITTYPE_TEMPDATA3 + 7) / 8];
extern uint8_t hittype_tempdata45_allocated_bf[(MAX_HITTYPE_TEMPDATA45 + 7) / 8];
//
extern int32_t hittype_tempdata0[MAX_HITTYPE_TEMPDATA0];
extern int32_t hittype_tempdata1[MAX_HITTYPE_TEMPDATA1];
extern int32_t hittype_tempdata2[MAX_HITTYPE_TEMPDATA2];
extern int32_t hittype_tempdata3[MAX_HITTYPE_TEMPDATA3];
typedef struct
{
    int32_t temp_data4: 17;
    int32_t temp_data5: 15;
}   tempdata45_t;
extern tempdata45_t hittype_tempdata45[MAX_HITTYPE_TEMPDATA45];


#endif
extern uint8_t hittype_otherdata_allocated_bf[(MAX_HITTYPE_OTHERDATA + 7) / 8];
extern weaponhit_otherdata_t hittype_otherdata[MAX_HITTYPE_OTHERDATA];

#define SAVE_SPACE_ON_MOVFLAG 1
struct weaponhit
{
    uint32_t  cgg : 3;   // it is 0... 6 : 3 bit
    uint32_t picnum : 13;  // 13 bit unsigned?
    int32_t extra : 16;

    int32_t dispicnum : 14;        // this can be reduced. 14 bits probably (can be negative)
    int32_t bposx : 18;  // 18?

    int32_t floorz  : 21;       // is 21 enough ?
    int32_t actorstayput : 11; // must be signed.


    int32_t ceilingz : 21;
#if !USE_SEPARATE_HITTYPE_TEMPDATA
    uint32_t tempdata_idx : 10; // max 1023
#else
    uint32_t tempdata1_idx : 10; // max 1023
#endif
#if SAVE_SPACE_ON_MOVFLAG
    uint32_t movflag : 1;       // this is just checked if > 16384. Then why remembering all the bits?
#else
    int32_t movflag : 16;
#endif

    int32_t owner : 12;   //MAXSPRITES? But for sure can be negative.
    int32_t bposy : 18;  // 18?

    int32_t bposz : 21; // 22?
    uint32_t otherdata_idx : 9;

#if USE_SEPARATE_HITTYPE_TEMPDATA
    // less commonly used tempdata
    uint8_t tempdata0_idx;
    uint8_t tempdata2_idx;
    uint8_t tempdata3_idx;
    uint8_t tempdata45_idx;
#endif
    //uint32_t timetosleep : 11;      // up to 1536. Can it be negative ? if not, then 11 bits would be enough, provided the instrptr does not change it.
    //uint32_t ang : 11;   // 0..2047
    //uint32_t tempang : 11; // 0...2047

//
//    int32_t lastvx; // for sure 32 bit (saw 1<<30 or something)
//    int32_t lastvy; // this is same size as bposx 18 ?
     //   int32_t temp_data[6]; // cannot be minimized :( Or can they?
    // temp_data 0: max: z. 21 ?
    // temp_data 1: spritez. So basically could be between any ceilingz and floorz. 21 bits at least.
    // temp_data 2: idem as td1
    // temp_data 3: idem as td1
    // temp_data 4: 32 bit value signed
    // temp_data 5: unsigned short 16 bit ? May need more.
};
#endif

#pragma pack(pop)

#if USE_RAM_HITTYPE
#define MAX_RAM_HITTYPES 772 // 800 is better. But requires 3k more. 690 if using 16-bit reverb audio buffer.
extern struct weaponhit ramHittype[MAX_RAM_HITTYPES];
extern struct weaponhit *hittypeRamPtr;
extern struct weaponhit *hittypes;     //
extern int16_t hittypeIdx[MAXSPRITES];
#else
extern struct weaponhit hittype[MAXSPRITES];
#endif
#define HITTYPE_TRANSITION 0
#if HITTYPE_TRANSITION
static inline const struct weaponhit* getHittype(int index)
{
    return &hittypes[index];
}
#define define_hitypeGettersAndSetters(fieldName) \
static inline void setHittype_##fieldName(int index, int value) \
{   \
    hittypes[index].fieldName = value; \
} \
static inline int getHittype_##fieldName(int index) \
{   \
    return getHittype(index)->fieldName; \
}
#else
static inline const struct weaponhit* getHittype(int index)
{
    int idx = hittypeIdx[index];
    if (!idx) /*was modified ? */
    {
        return &hittypes[index];

    }
    return &hittypeRamPtr[idx];
}
#define define_hitypeGettersAndSetters(fieldName) \
static inline void setHittype_##fieldName(int index, int value) \
{   \
    int idx =  hittypeIdx[index]; \
    if (idx) /*was modified ? */ \
    { \
        struct weaponhit *ht = &hittypeRamPtr[idx]; \
        ht->fieldName = value; \
    } \
    else    /*not yet modified*/ \
    { \
        if (hittypes[index].fieldName != value) \
        { \
            struct weaponhit *ht = &hittypeRamPtr [allocateNewHittype(index)];\
            ht->fieldName = value;            \
        } \
    }\
} \
static inline int getHittype_##fieldName(int index) \
{   \
    return getHittype(index)->fieldName; \
}
#endif


#if WIN32
extern struct weaponhit initial_hittype[MAXSPRITES];
#endif
extern input loc;
extern input recsync[RECSYNCBUFSIZ];
extern int32_t avgfvel, avgsvel, avgavel, avghorz, avgbits;

extern short numplayers, myconnectindex;
extern short connecthead, connectpoint2[MAXPLAYERS];   //Player linked list variables (indeces, not connection numbers)
extern short screenpeek;

extern int current_menu;
extern int32_t tempwallptr,animatecnt;
extern int32_t lockclock;
extern uint8_t  display_mirror,rtsplaying;

extern int32_t movefifoend[MAXPLAYERS];
extern int32_t ototalclock;
extern int32_t animategoal[MAXANIMATES];
#if INTERPOLATION_USES_POSPTR
extern int32_t *animateptr[MAXANIMATES];
#else
extern indexedValue_t animateidx[MAXANIMATES];
#endif
extern int32_t animatevel[MAXANIMATES];
// extern int32_t oanimateval[MAXANIMATES];
extern short neartagsector, neartagwall, neartagsprite;
extern int32_t neartaghitdist;
extern short animatesect[MAXANIMATES];
extern int32_t movefifoplc, vel,svel,angvel,horiz;
extern uint8_t interpolationState;
extern int32_t interpolationSmoothRatio;

extern short mirrorwall[64], mirrorsector[64], mirrorcnt;

#define NUMKEYS 19

extern int32_t chainplace, chainnumpages;
extern volatile int32_t checksume;

#include "funct.h"
//#include "engine_protos.h"

extern uint8_t  screencapt;
#if LOAD_FROM_CON_FILE
extern int32_t dukefriction;
extern short soundps[NUM_SOUNDS],soundpe[NUM_SOUNDS],soundvo[NUM_SOUNDS];
extern uint8_t  soundpr[NUM_SOUNDS],soundm[NUM_SOUNDS];
extern char  level_names[44][33];
extern int32_t partime[44],designertime[44];
extern char  volume_names[4][33];
extern char  skill_names[5][33];
extern char  level_file_names[44][128];
#else
extern const int32_t dukefriction;
extern const short soundps[NUM_SOUNDS],soundpe[NUM_SOUNDS],soundvo[NUM_SOUNDS];
extern const uint8_t  soundpr[NUM_SOUNDS],soundm[NUM_SOUNDS];
extern const char  level_names[44][20];
extern const int32_t partime[44];
extern const int32_t designertime[44];
extern const char  volume_names[4][33];
extern const char  skill_names[5][33];
extern const char  level_file_names[44][10];
#endif
extern int32_t SoundToggle,MusicToggle;
extern short last_threehundred,lastsavedpos;
extern uint8_t  restorepalette;

extern short buttonstat;
extern int32_t cachecount;
#if PALETTES_IN_RAM
extern uint8_t  waterpal[768],slimepal[768],titlepal[768],drealms[768],endingpal[768];
#else
extern uint8_t  *waterpal,*slimepal,*titlepal,*drealms,*endingpal;

#endif
extern char boardfilename[32];         // to be removed this useless stuff.
#if USE_BETANAME
extern uint8_t  betaname[80];
#endif
extern uint8_t  earthquaketime;
extern uint8_t  networkmode;
#if USE_RTS
extern uint8_t  lumplockbyte[11];
#endif
    //DUKE3D.H - replace the end "my's" with this
extern int32_t myx, omyx, myxvel, myy, omyy, myyvel, myz, omyz, myzvel;
extern short myhoriz, omyhoriz, myhorizoff, omyhorizoff;
#if 0
extern short globalskillsound;
#else
extern short globalSkillVoice;
#endif
extern short myang, omyang, mycursectnum, myjumpingcounter;
extern uint8_t  myjumpingtoggle, myonground, myhardlanding,myreturntocenter;
extern int32_t fakemovefifoplc;
extern int32_t myxbak[MOVEFIFOSIZ], myybak[MOVEFIFOSIZ], myzbak[MOVEFIFOSIZ];
extern int32_t myhorizbak[MOVEFIFOSIZ];
extern short myangbak[MOVEFIFOSIZ];

extern const short weaponsandammosprites[15];




//DUKE3D.H:
typedef struct
{
        short frag[MAXPLAYERS], got_access, last_extra, shield_amount, curr_weapon;
        short ammo_amount[MAX_WEAPONS], holoduke_on;
        uint8_t  gotweapon[MAX_WEAPONS], inven_icon, jetpack_on, heat_on;
        short firstaid_amount, steroids_amount, holoduke_amount, jetpack_amount;
        short heat_amount, scuba_amount, boot_amount;
        short last_weapon, weapon_pos, kickback_pic;

} STATUSBARTYPE;

extern STATUSBARTYPE sbar;
extern short frags[MAXPLAYERS][MAXPLAYERS];
extern int32_t cameradist, cameraclock,show_shareware;
extern uint8_t  networkmode, movesperpacket;
extern uint8_t  gamequit;

extern uint8_t  pus,pub, lasermode;
extern uint8_t  syncstat, syncval[MAXPLAYERS][MOVEFIFOSIZ];
extern int8_t multiwho, multipos, multiwhat, multiflag;
extern int32_t syncvalhead[MAXPLAYERS], syncvaltail, syncvaltottail;
#if LOAD_FROM_CON_FILE
    extern uint8_t camerashitable,freezerhurtowner;
    extern int32_t numfreezebounces,rpgblastradius,pipebombblastradius,tripbombblastradius,shrinkerblastradius,morterblastradius,bouncemineblastradius,seenineblastradius;
#else
    extern const uint8_t camerashitable,freezerhurtowner;
    extern const int32_t numfreezebounces,rpgblastradius,pipebombblastradius,tripbombblastradius,shrinkerblastradius,morterblastradius,bouncemineblastradius,seenineblastradius;
#endif
// CTW - MODIFICATION
// extern uint8_t  stereo,eightytwofifty,playerswhenstarted,playonten,everyothertime;
extern uint8_t  stereo,playerswhenstarted,everyothertime;
// CTW END - MODIFICATION
extern int32_t myminlag[MAXPLAYERS], mymaxlag, otherminlag, bufferjitter;

extern int32_t numinterpolations, startofdynamicinterpolations;
extern int32_t oldipos[MAXINTERPOLATIONS];
#if INTERPOLATION_HAS_BAKIPOS
extern int32_t bakipos[MAXINTERPOLATIONS];
#endif
#if INTERPOLATION_USES_POSPTR
#error
extern int32_t *curipos[MAXINTERPOLATIONS]; // might be a short ptr.
#else

extern indexedValue_t curposidx[MAXINTERPOLATIONS];
static inline int32_t getIndexedValue(int interpIndex, indexedValue_t *indexes)
{
    int elementIndex = indexes[interpIndex].index;
    int type = indexes[interpIndex].type;
#if WIN32
    if (elementIndex < 0)
    {
        FIXME("Element Index is %d", elementIndex);
        EXIT(0);
    }
#endif

    if (type == INTERP_TYPE_NOT_ALLOCATED)
    {
        FIXME("Interpolation %d was not allocated\r\n", interpIndex);
        EXIT(1);
    }
    else
    {
        switch (type)
        {
            case INTERP_TYPE_WALL_X:
                return ramwalls[elementIndex].x;
            case INTERP_TYPE_WALL_Y:
                return ramwalls[elementIndex].y;
            case INTERP_TYPE_SECTOR_CEILINGZ:
                return ramsectors[elementIndex].ceilingz;
            case INTERP_TYPE_SECTOR_FLOORZ:
                return ramsectors[elementIndex].floorz;
        }
    }
    return 0;   // shut up warnings.
}
static inline int32_t getIndexedValueFromIndexedValue(indexedValue_t indexv)
{
    int elementIndex = indexv.index;
    int type = indexv.type;
#if WIN32
    if (elementIndex < 0)
    {
        FIXME("Element Index is %d", elementIndex);
        EXIT(0);
    }
#endif

    if (type == INTERP_TYPE_NOT_ALLOCATED)
    {
        FIXME("Interpolation was not allocated\r\n");
        EXIT(1);
    }
    else
    {
        switch (type)
        {
            case INTERP_TYPE_WALL_X:
                return ramwalls[elementIndex].x;
            case INTERP_TYPE_WALL_Y:
                return ramwalls[elementIndex].y;
            case INTERP_TYPE_SECTOR_CEILINGZ:
                return ramsectors[elementIndex].ceilingz;
            case INTERP_TYPE_SECTOR_FLOORZ:
                return ramsectors[elementIndex].floorz;

        }
    }
    return 0;   // shut up warnings.
}

static inline void setIndexedValue(int interpIndex, indexedValue_t *indexes,  int value)
{
    int elementIndex = indexes[interpIndex].index;
#if WIN32
    if (elementIndex < 0)
    {
        FIXME("Element Index is %d", elementIndex);
        EXIT(0);
    }
#endif
    int type = indexes[interpIndex].type;
    if (type == INTERP_TYPE_NOT_ALLOCATED)
    {
        FIXME("Interpolation %d was not allocated\r\n", interpIndex);
        EXIT(1);
    }
    else
    {
        switch (type)
        {
            case INTERP_TYPE_WALL_X:
                 ramwalls[elementIndex].x = value;
                 break;
            case INTERP_TYPE_WALL_Y:
                ramwalls[elementIndex].y = value;
                break;
            case INTERP_TYPE_SECTOR_CEILINGZ:
                ramsectors[elementIndex].ceilingz = value;
                break;
            case INTERP_TYPE_SECTOR_FLOORZ:
                ramsectors[elementIndex].floorz = value;
                break;
        }
    }
}
static inline void setIndexedValueFromIndexedValue(indexedValue_t indexv,  int value)
{
    int elementIndex = indexv.index;
#if WIN32
    if (elementIndex < 0)
    {
        FIXME("Element Index is %d", elementIndex);
        EXIT(0);
    }
#endif
    int type = indexv.type;
    if (type == INTERP_TYPE_NOT_ALLOCATED)
    {
        FIXME("Interpolation was not allocated\r\n");
        EXIT(1);
    }
    else
    {
        switch (type)
        {
            case INTERP_TYPE_WALL_X:
                 ramwalls[elementIndex].x = value;
                 break;
            case INTERP_TYPE_WALL_Y:
                ramwalls[elementIndex].y = value;
                break;
            case INTERP_TYPE_SECTOR_CEILINGZ:
                ramsectors[elementIndex].ceilingz = value;
                break;
            case INTERP_TYPE_SECTOR_FLOORZ:
                ramsectors[elementIndex].floorz = value;
                break;
        }
    }
}

#endif
#if INTERPOLATION_USES_POSPTR
extern void setinterpolation(int32_t *posptr);
extern void stopinterpolation(int32_t *posptr);
extern int32_t getanimationgoal(int32_t *animptr);
extern int32_t setanimation(short animsect,int32_t *animptr,int32_t thegoal,int32_t thevel);
#else
#if WIN32
#define setinterpolation(x) setinterpolation2(x, __FILE__, __LINE__)
extern void setinterpolation2(indexedValue_t iv, char *file, int line);
#else
extern void setinterpolation(indexedValue_t iv);
#endif
extern void stopinterpolation(indexedValue_t iv);
extern int32_t getanimationgoal(indexedValue_t animidx);
extern int32_t setanimation(short animsect,indexedValue_t animidx,int32_t thegoal,int32_t thevel);

#endif
#if CLOUD_SUPPORT
#define MAXCLOUDS       40      // was 128, but found max is 35
extern short numclouds,clouds[MAXCLOUDS],cloudx[MAXCLOUDS],cloudy[MAXCLOUDS];
extern int32_t cloudtotalclock;
#endif
extern int32_t totalmemory;



extern uint8_t myaimmode, myaimstat, omyaimstat;

extern uint8_t  nHostForceDisableAutoaim;
extern int allocateNewHittypeTempData(int index);
#if !OPTIMIZED_HITTYPE_TEMPDATA
static inline const int32_t* getHittypeTempPtr(int i)
{
    return &hittype_tempdata[getHittype(i)->tempdata_idx].temp_data[0];
}
#endif
#if WIN32
extern int htmaxData[6], htminData[6];
#endif
#if USE_SEPARATE_HITTYPE_TEMPDATA
void freeHittype_tempdata0(int hittype_index);
void freeHittype_tempdata1(int hittype_index);
void freeHittype_tempdata2(int hittype_index);
void freeHittype_tempdata3(int hittype_index);
void freeHittype_tempdata45(int hittype_index);
void resetAllocHittype_lastIndex_tempdata0();
void resetAllocHittype_lastIndex_tempdata1();
void resetAllocHittype_lastIndex_tempdata2();
void resetAllocHittype_lastIndex_tempdata3();
void resetAllocHittype_lastIndex_tempdata45();
void resetAllocHittype_lastIndex_otherdata();
int allocateNewHittype_tempdata0(uint32_t hittype_index);
int allocateNewHittype_tempdata1(uint32_t hittype_index);
int allocateNewHittype_tempdata2(uint32_t hittype_index);
int allocateNewHittype_tempdata3(uint32_t hittype_index);
int allocateNewHittype_tempdata45(uint32_t hittype_index);
int allocateNewHittype_otherdata(uint32_t hittype_index);
#define NO_HITTYPE_REQUIRED                 -2
#define HITTYPE_FEATURE_ID_NONE             -1
#define HITTYPE_FEATURE_ID_tempdata0        0
#define HITTYPE_FEATURE_ID_tempdata1        1
#define HITTYPE_FEATURE_ID_tempdata2        2
#define HITTYPE_FEATURE_ID_tempdata3        3
#define HITTYPE_FEATURE_ID_tempdata45       4
#define HITTYPE_FEATURE_ID_otherdata        6

#endif
int allocateNewHittype(int hittype_index);
static inline void setHittypeTemp(int i, int n, int value)
{
    #if WIN32 && 0
        int changed = 0;
        if (value > htmaxData[n])
        {
            htmaxData[n] = value;
            changed = 1;
        }
        if (value < htminData[n])
        {
            htminData[n] = value;
            changed = 1;
        }
        if (changed)
        {
            for (int i = 0; i < 6; i++)
            {
                printf("htmin[%d] = %d, htmax[%d] = %d\r\n", i, htminData[i], i, htmaxData[i]);
            }
        }
    #endif
#if USE_SEPARATE_HITTYPE_TEMPDATA
    uint32_t tempdata_idx;
    switch (n)
    {
        case 0: tempdata_idx = getHittype(i)->tempdata0_idx; break;
        case 1: tempdata_idx = getHittype(i)->tempdata1_idx; break;
        case 2: tempdata_idx = getHittype(i)->tempdata2_idx; break;
        case 3: tempdata_idx = getHittype(i)->tempdata3_idx; break;
        case 4: tempdata_idx = getHittype(i)->tempdata45_idx; break;
        case 5: tempdata_idx = getHittype(i)->tempdata45_idx; break;
    }
    if (tempdata_idx) // was a new tempdata allocated ? then update it regardless the value
    {
        if (value == 0)
        {
            // if it is 0, free data. Case 4 and 5 are different, because we must also set data
            switch(n)
            {
                case 0: freeHittype_tempdata0(i); break;
                case 1: freeHittype_tempdata1(i); break;
                case 2: freeHittype_tempdata2(i); break;
                case 3: freeHittype_tempdata3(i); break;
                case 4:
                    // also 5 was 0? free whole thing
                    if (hittype_tempdata45[tempdata_idx].temp_data5 == 0)
                        freeHittype_tempdata45(i);
                    else // just set 4
                        hittype_tempdata45[tempdata_idx].temp_data4 = 0;
                break;
                case 5:
                    // also 4 was 0? free whole thing
                    if (hittype_tempdata45[tempdata_idx].temp_data4 == 0)
                        freeHittype_tempdata45(i);
                    else // just set 5
                        hittype_tempdata45[tempdata_idx].temp_data5 = 0;
                break;
            }
        }
        else
        {
            switch(n)
            {
                case 0: hittype_tempdata0[tempdata_idx] = value; break;
                case 1: hittype_tempdata1[tempdata_idx] = value; break;
                case 2: hittype_tempdata2[tempdata_idx] = value; break;
                case 3: hittype_tempdata3[tempdata_idx] = value; break;
                case 4: hittype_tempdata45[tempdata_idx].temp_data4 = value; break;
                case 5: hittype_tempdata45[tempdata_idx].temp_data5 = value; break;
            }
        }
    }
    else if (value != 0)
    {
//        printf("Allocating because tempData %i is %i\r\n", n, value);
        switch(n)
        {
            case 0: allocateNewHittype_tempdata0(i); break;
            case 1: allocateNewHittype_tempdata1(i); break;
            case 2: allocateNewHittype_tempdata2(i); break;
            case 3: allocateNewHittype_tempdata3(i); break;
            case 4: allocateNewHittype_tempdata45(i); break;
            case 5: allocateNewHittype_tempdata45(i); break;
        }
        switch(n)
        {
            case 0: hittype_tempdata0[getHittype(i)->tempdata0_idx] = value; break;
            case 1: hittype_tempdata1[getHittype(i)->tempdata1_idx] = value; break;
            case 2: hittype_tempdata2[getHittype(i)->tempdata2_idx] = value; break;
            case 3: hittype_tempdata3[getHittype(i)->tempdata3_idx] = value; break;
            case 4: hittype_tempdata45[getHittype(i)->tempdata45_idx].temp_data4 = value; break;
            case 5: hittype_tempdata45[getHittype(i)->tempdata45_idx].temp_data5 = value; break;
        }
    }
#else
    if (getHittype(i)->tempdata_idx) // was a new tempdata allocated ? then update it regardless the value
    {
#if !OPTIMIZED_HITTYPE_TEMPDATA
        hittype_tempdata[getHittype(i)->tempdata_idx].temp_data[n] = value;
#else
        if (n <= 3)
        {
            hittype_tempdata[getHittype(i)->tempdata_idx].temp_data[n] = value;
        }
        else if (n == 4)
        {
            hittype_tempdata[getHittype(i)->tempdata_idx].temp_data4 = value;
        }
        else
        {
            hittype_tempdata[getHittype(i)->tempdata_idx].temp_data5 = value;
        }
#endif
    }
    else if (value != 0)
    {
//        printf("Allocating because tempData %i is %i\r\n", n, value);
        allocateNewHittype_tempdata(i);
#if !OPTIMIZED_HITTYPE_TEMPDATA
        hittype_tempdata[getHittype(i)->tempdata_idx].temp_data[n] = value;
#else
        if (n <= 3)
        {
            hittype_tempdata[getHittype(i)->tempdata_idx].temp_data[n] = value;
        }
        else if (n == 4)
        {
            hittype_tempdata[getHittype(i)->tempdata_idx].temp_data4 = value;
        }
        else
        {
            hittype_tempdata[getHittype(i)->tempdata_idx].temp_data5 = value;
        }
#endif
    }
#endif
}
static inline int32_t getHittypeTemp(int i, int n)
{
#if USE_SEPARATE_HITTYPE_TEMPDATA

    switch(n)
    {
        case 0: return hittype_tempdata0[getHittype(i)->tempdata0_idx];
        case 1: return hittype_tempdata1[getHittype(i)->tempdata1_idx];
        case 2: return hittype_tempdata2[getHittype(i)->tempdata2_idx];
        case 3: return hittype_tempdata3[getHittype(i)->tempdata3_idx];
        case 4: return hittype_tempdata45[getHittype(i)->tempdata45_idx].temp_data4;
        case 5: return hittype_tempdata45[getHittype(i)->tempdata45_idx].temp_data5;
    }
    return 0;
#else
#if !OPTIMIZED_HITTYPE_TEMPDATA
    return getHittypeTempPtr(i)[n];
#else
        if (n <= 3)
        {
            return hittype_tempdata[getHittype(i)->tempdata_idx].temp_data[n];
        }
        else if (n == 4)
        {
            return hittype_tempdata[getHittype(i)->tempdata_idx].temp_data4;
        }
        else
        {
            return hittype_tempdata[getHittype(i)->tempdata_idx].temp_data5;
        }
#endif
#endif
}
#define define_hittypeOtherdataGettersAndSetters(fieldname) \
static inline void setHittype_##fieldname(int i, int value) \
{\
    if (getHittype(i)->otherdata_idx) /*update if already allocated*/ \
    {\
        hittype_otherdata[getHittype(i)->otherdata_idx].fieldname = value;\
    }\
    else if (value != 0)\
    {\
        allocateNewHittype_otherdata(i);\
        hittype_otherdata[getHittype(i)->otherdata_idx].fieldname = value;\
    }\
}\
static inline int32_t getHittype_##fieldname(int i)\
{\
    return hittype_otherdata[getHittype(i)->otherdata_idx].fieldname;\
}
define_hittypeOtherdataGettersAndSetters(lastvx);
define_hittypeOtherdataGettersAndSetters(lastvy);
define_hittypeOtherdataGettersAndSetters(timetosleep);
define_hittypeOtherdataGettersAndSetters(ang);
define_hittypeOtherdataGettersAndSetters(tempang);
//
define_hitypeGettersAndSetters(cgg);
define_hitypeGettersAndSetters(picnum);
define_hitypeGettersAndSetters(extra);
define_hitypeGettersAndSetters(dispicnum);
define_hitypeGettersAndSetters(bposx);
define_hitypeGettersAndSetters(floorz);
define_hitypeGettersAndSetters(actorstayput);
define_hitypeGettersAndSetters(ceilingz);

#if !USE_SEPARATE_HITTYPE_TEMPDATA
define_hitypeGettersAndSetters(tempdata_idx);
#else
define_hitypeGettersAndSetters(tempdata1_idx);
#endif
define_hitypeGettersAndSetters(movflag);
define_hitypeGettersAndSetters(owner);
define_hitypeGettersAndSetters(bposy);
define_hitypeGettersAndSetters(bposz);
define_hitypeGettersAndSetters(otherdata_idx);
#if USE_SEPARATE_HITTYPE_TEMPDATA
define_hitypeGettersAndSetters(tempdata0_idx);
define_hitypeGettersAndSetters(tempdata2_idx);
define_hitypeGettersAndSetters(tempdata3_idx);
define_hitypeGettersAndSetters(tempdata45_idx);
#endif
// save game data
#define SAVEGAME_COOKIE 0x44334E44
#define SETTINGS_COOKIE 0x44334E44
#define	SAVEGAME_VERSION	1
#define SAVE_GAME_SIZE              131072
#define SAVE_SETTINGS_SIZE          8192


// next-hack: blockmap support
#if USE_BLOCKMAP
#define BLOCKMAP_SIZE    16
typedef struct
{
    int xmax, ymax, xmin, ymin;
} boundingBox_t;
typedef struct
{
    boundingBox_t bb;
    int deltax, deltay;
    int16_t * blockmap[BLOCKMAP_SIZE][BLOCKMAP_SIZE];
} blockmap_data_t;
extern blockmap_data_t *pblockmapData;
#endif

#include "control.h"

// next-hack: we have to align on 4-byte boundary
// because extMemProgram only works on 4 bytes.
// loading 1 byte is fine, though
#define DECLARE_ALIGNED4(type, member) \
    type member; \
    uint8_t padding_##member[4 - (sizeof(type) % 4) % 4]
#define DECLARE_ALIGNED4_ARRAY(type, member, size) \
    type member[size]; \
    uint8_t padding_##member[4 - ((sizeof(type) * (size)) % 4) % 4]

typedef struct
{
    DECLARE_ALIGNED4(uint32_t, dummy);
    DECLARE_ALIGNED4(uint32_t, cookie);
    DECLARE_ALIGNED4(int32_t, version);

    DECLARE_ALIGNED4(struct user_defs, ud);
    DECLARE_ALIGNED4(int, numplayers);
    DECLARE_ALIGNED4_ARRAY(short, spriteq, MAX_SPRITEQ);
    DECLARE_ALIGNED4(short, spriteqloc);
    DECLARE_ALIGNED4(short, spriteqamount);
    DECLARE_ALIGNED4_ARRAY(struct player_struct, ps, MAXPLAYERS);
    DECLARE_ALIGNED4_ARRAY(struct player_orig, po, MAXPLAYERS);

    DECLARE_ALIGNED4_ARRAY(int32_t, msx, MAX_MOVABLE_SECTORS);
    DECLARE_ALIGNED4_ARRAY(int32_t, msy, MAX_MOVABLE_SECTORS);
    DECLARE_ALIGNED4_ARRAY(short, cyclers, MAXCYCLERS * 6);
    DECLARE_ALIGNED4(short, numcyclers);

    DECLARE_ALIGNED4_ARRAY(struct animwalltype, animwall, MAXANIMWALLS);
    DECLARE_ALIGNED4(short, numanimwalls);

    DECLARE_ALIGNED4_ARRAY(short, mirrorwall, 64);
    DECLARE_ALIGNED4_ARRAY(short, mirrorsector, 64);
    DECLARE_ALIGNED4(short, mirrorcnt);

    DECLARE_ALIGNED4_ARRAY(uint8_t, show2dsector, (MAXSECTORS + 7) >> 3);
    DECLARE_ALIGNED4_ARRAY(uint8_t, show2dwall, (MAXWALLS + 7) >> 3);
    DECLARE_ALIGNED4_ARRAY(uint8_t, show2dsprite, (MAXSPRITES + 7) >> 3);

    DECLARE_ALIGNED4_ARRAY(char, boardfilename, BOARD_FILENAME_LENGTH);

    DECLARE_ALIGNED4_ARRAY(short, pskyoff, MAXPSKYTILES);
    DECLARE_ALIGNED4(short, pskybits);

    DECLARE_ALIGNED4(int32_t, lockclock);
    DECLARE_ALIGNED4(int32_t, ototalclock);
    DECLARE_ALIGNED4(int32_t, totalclock);
    DECLARE_ALIGNED4(int32_t, lastvisinc);

    DECLARE_ALIGNED4(int32_t, animatecnt);
    DECLARE_ALIGNED4_ARRAY(int32_t, animategoal, MAXANIMATES);
    DECLARE_ALIGNED4_ARRAY(int32_t, animatevel, MAXANIMATES);
    DECLARE_ALIGNED4_ARRAY(indexedValue_t, animateidx, MAXANIMATES);
    DECLARE_ALIGNED4_ARRAY(short, animatesect, MAXANIMATES);

    DECLARE_ALIGNED4(uint8_t, earthquaketime);

    DECLARE_ALIGNED4(short, camsprite);

    DECLARE_ALIGNED4(short, connecthead);
    DECLARE_ALIGNED4_ARRAY(short, connectpoint2, MAXPLAYERS);

    DECLARE_ALIGNED4(uint8_t, numplayersprites);

    DECLARE_ALIGNED4_ARRAY(short, frags, MAXPLAYERS * MAXPLAYERS);

    DECLARE_ALIGNED4(int32_t, randomseed);

    DECLARE_ALIGNED4(short, global_random);

    DECLARE_ALIGNED4(int32_t, parallaxyscale);

    DECLARE_ALIGNED4(short, numsectors);
    DECLARE_ALIGNED4(short, numwalls);

    DECLARE_ALIGNED4_ARRAY(walltype, ramwalls, MAXRAMWALLS);
    DECLARE_ALIGNED4_ARRAY(spritetype, ramsprites, MAXRAMSPRITES);
    DECLARE_ALIGNED4_ARRAY(short, ramspriteIdx, MAXSPRITES);

    #if OPTIMIZE_RAMWALLIDX
        DECLARE_ALIGNED4_ARRAY(uint8_t, ramwallIdx8, MAXWALLS);
        DECLARE_ALIGNED4_ARRAY(uint8_t, ramwallIdx2, (MAXWALLS + 3) / 4);
    #else
        DECLARE_ALIGNED4_ARRAY(short, ramwallIdx, MAXWALLS);
    #endif

    DECLARE_ALIGNED4_ARRAY(sectortype, ramsectors, MAXRAMSECTORS);

    #if MAXRAMSECTORS >= 255
        DECLARE_ALIGNED4_ARRAY(short, ramsectorIdx, MAXSECTORS);
    #else
        DECLARE_ALIGNED4_ARRAY(uint8_t, ramsectorIdx, MAXSECTORS);
    #endif

    DECLARE_ALIGNED4(short, numModifiedWalls);
    DECLARE_ALIGNED4(short, numModifiedSectors);
    DECLARE_ALIGNED4(short, numModifiedSprites);

    DECLARE_ALIGNED4_ARRAY(uint8_t, ramsprites_allocated_bf, (MAXRAMSPRITES + 7) / 8);

    DECLARE_ALIGNED4_ARRAY(sprite_additional_data, spr_add_data, MAXSPRITES);
    DECLARE_ALIGNED4_ARRAY(short, headspritesect, MAXSECTORS + 1);
    DECLARE_ALIGNED4_ARRAY(short, headspritestat, MAXSTATUS + 1);

    DECLARE_ALIGNED4_ARRAY(int8_t, wallshade, MAXWALLS);
    DECLARE_ALIGNED4_ARRAY(uint8_t, wall_hitag9999, (MAXWALLS + 7) / 8);
    DECLARE_ALIGNED4_ARRAY(uint8_t, wall_extraIsMinusOne, (MAXWALLS + 7) / 8);

    DECLARE_ALIGNED4_ARRAY(int16_t, ramSectorExtra, MAXSECTORS);
    DECLARE_ALIGNED4_ARRAY(int8_t, ramSectorCeilingShade, MAXSECTORS);
    DECLARE_ALIGNED4_ARRAY(int8_t, ramSectorFloorShade, MAXSECTORS);
    DECLARE_ALIGNED4_ARRAY(uint8_t, ramSectorFloorxpanning, MAXSECTORS);

    #if USE_SEPARATE_HITTYPE_TEMPDATA
        DECLARE_ALIGNED4_ARRAY(uint8_t, hittype_tempdata0_allocated_bf, (MAX_HITTYPE_TEMPDATA0 + 7) / 8);
        DECLARE_ALIGNED4_ARRAY(uint8_t, hittype_tempdata1_allocated_bf, (MAX_HITTYPE_TEMPDATA1 + 7) / 8);
        DECLARE_ALIGNED4_ARRAY(uint8_t, hittype_tempdata2_allocated_bf, (MAX_HITTYPE_TEMPDATA2 + 7) / 8);
        DECLARE_ALIGNED4_ARRAY(uint8_t, hittype_tempdata3_allocated_bf, (MAX_HITTYPE_TEMPDATA3 + 7) / 8);
        DECLARE_ALIGNED4_ARRAY(uint8_t, hittype_tempdata45_allocated_bf, (MAX_HITTYPE_TEMPDATA45 + 7) / 8);

        DECLARE_ALIGNED4_ARRAY(int32_t, hittype_tempdata0, MAX_HITTYPE_TEMPDATA0);
        DECLARE_ALIGNED4_ARRAY(int32_t, hittype_tempdata1, MAX_HITTYPE_TEMPDATA1);
        DECLARE_ALIGNED4_ARRAY(int32_t, hittype_tempdata2, MAX_HITTYPE_TEMPDATA2);
        DECLARE_ALIGNED4_ARRAY(int32_t, hittype_tempdata3, MAX_HITTYPE_TEMPDATA3);
        DECLARE_ALIGNED4_ARRAY(tempdata45_t, hittype_tempdata45, MAX_HITTYPE_TEMPDATA45);
        DECLARE_ALIGNED4(int, numHittype_tempdata0);
        DECLARE_ALIGNED4(int, numHittype_tempdata1);
        DECLARE_ALIGNED4(int, numHittype_tempdata2);
        DECLARE_ALIGNED4(int, numHittype_tempdata3);
        DECLARE_ALIGNED4(int, numHittype_tempdata45);
    #else
        DECLARE_ALIGNED4_ARRAY(uint8_t, hittype_tempdata_allocated_bf, (MAX_HITTYPE_TEMPDATA + 7) / 8);
        DECLARE_ALIGNED4_ARRAY(weaponhit_tempdata_t, hittype_tempdata, MAX_HITTYPE_TEMPDATA);
        DECLARE_ALIGNED4(int, numHittype_tempdata);
    #endif // USE_SEPARATE_HITTYPE_TEMPDATA

    DECLARE_ALIGNED4_ARRAY(uint8_t, hittype_otherdata_allocated_bf, (MAX_HITTYPE_OTHERDATA + 7) / 8);
    DECLARE_ALIGNED4_ARRAY(weaponhit_otherdata_t, hittype_otherdata, MAX_HITTYPE_OTHERDATA);

    #if USE_RAM_HITTYPE
        DECLARE_ALIGNED4_ARRAY(uint8_t, hittype_allocated_bf, (MAX_RAM_HITTYPES + 7) / 8);
        DECLARE_ALIGNED4(int, numHittypes);
        DECLARE_ALIGNED4_ARRAY(int16_t, hittypeIdx, MAXSPRITES);
        DECLARE_ALIGNED4_ARRAY(struct weaponhit, ramHittype, MAX_RAM_HITTYPES);
    #endif // USE_RAM_HITTYPE

    DECLARE_ALIGNED4(int, numHittype_otherdata);

    DECLARE_ALIGNED4_ARRAY(indexedValue_t, curposidx, MAXINTERPOLATIONS);
    DECLARE_ALIGNED4(int32_t, numinterpolations);
    DECLARE_ALIGNED4(int32_t, startofdynamicinterpolations);
    DECLARE_ALIGNED4_ARRAY(int32_t, oldipos, MAXINTERPOLATIONS);
    DECLARE_ALIGNED4(uint8_t, interpolationState);
    DECLARE_ALIGNED4(int32_t, interpolationSmoothRatio);
    #if USE_BLOCKMAP
        DECLARE_ALIGNED4_ARRAY(uint8_t, sectorModifiedBf, (MAXSECTORS + 31) / 4);
    #endif
} savegame_t;


typedef struct
{
    savegame_t s;
    uint8_t padding[SAVE_GAME_SIZE - sizeof(savegame_t)];
} padded_savegame_t;
typedef struct
{
    uint32_t cookie;
    DECLARE_ALIGNED4(struct user_defs, ud);
    DECLARE_ALIGNED4_ARRAY(KeyMapping_t, KeyMapping, MAXGAMEBUTTONS);
    DECLARE_ALIGNED4_ARRAY(uint8_t, JoyAxisMapping, max_axis);
    int32_t axisSensitivities[max_axis];
    //
    DECLARE_ALIGNED4_ARRAY(char, myname, MAX_NAME_LEN);
    //
    int32_t SoundToggle;
    int32_t MusicToggle;
    int32_t VoiceToggle;
    int32_t AmbienceToggle;
    int32_t OpponentSoundToggle;
    int32_t FXVolume;
    int32_t MusicVolume;
    int32_t FXDevice;
    int32_t MusicDevice;
    int32_t ReverseStereo;

    uint32_t crc32;
} settings_t;
typedef struct
{
    settings_t s;
    uint8_t padding[SAVE_SETTINGS_SIZE - sizeof(settings_t)];
} padded_settings_t;
typedef struct
{
    padded_settings_t settings;
    uint8_t tempScreenshot[16384];
    uint8_t screenshots[MAX_SAVEGAMES+1][16384];   //+1 because to speedup reloading after death, we save on slot MAX_SAVEGAME
    padded_savegame_t savegames[MAX_SAVEGAMES+1];  // idem.
    // store here the tiledata ptrs to external memory
    uint8_t extMemTileDataPtrBackup[(MAXTILES * sizeof(void*) + 65535) & ~65535];
} nvmData_t;

#if USE_SEPARATE_HITTYPE_TEMPDATA
extern int numHittype_tempdata0;
extern int numHittype_tempdata1;
extern int numHittype_tempdata2;
extern int numHittype_tempdata3;
extern int numHittype_tempdata45;
#else
extern int numHittype_tempdata;
#endif
#if USE_RAM_HITTYPE
    extern uint8_t hittype_allocated_bf[(MAX_RAM_HITTYPES + 7) / 8];
    extern int numHittypes;
#endif // USE_RAM_HITTYPE
extern int numHittype_otherdata;

#ifdef WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
// other commonly used functions.
void deletehittype( int n);
void freeUnnecessarySprites(int forWhichSprite, int forWhichHittype);
void resetAllocRamHittypeLastIndex(void);
void resetAllHittypeData(void);
void initFreeableSprites(void);
#endif  // include-once header.

