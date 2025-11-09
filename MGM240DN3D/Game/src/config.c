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

#ifdef WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "duke3d.h"
#include "audiolib/sndcards.h"
#include "build.h"
#include "config.h"
#include "control.h"
#include "engine.h"
#include "file_lib.h"
#include "function.h"
#include "keyboard.h"
#include "scriplib.h"
#include "util_lib.h"
#if MG24
#include "crc.h"
#endif
// we load this in to get default button and key assignments
// as well as setting up function mappings

#include "_functio.h"
#include "strings.h"
#include "audiolib/music.h"
#include "audiolib/fx_man.h"
//
#define LOAD_OPTION_FROM_EXTMEM(property)     do {extMemGetDataFromAddress((void*) &property, &nvm->settings.s.property , sizeof(property)); \
                                            } while(0)    //
#define SAVE_OPTION_TO_EXTMEM(property)  extMemProgram((uint32_t) &nvm->settings.s.property, (uint8_t*) &property, (3 + sizeof(nvm->settings.s.property)) & ~3)    //

//
// Sound variables
//
int32_t FXDevice;
int32_t MusicDevice;
int32_t FXVolume;
int32_t MusicVolume;
int32_t SoundToggle;
int32_t MusicToggle;
int32_t VoiceToggle;
int32_t AmbienceToggle;
int32_t OpponentSoundToggle; // xduke to toggle opponent's sounds on/off in DM (duke 1.3d scheme)
//int32_t NumVoices = 8;
const int32_t NumChannels = 2;
#if WIN32
const int32_t NumBits = 8;
#else
const int32_t NumBits = 8;
#endif
const int32_t MixRate = 11025;
int32_t MidiPort;
int32_t ReverseStereo;

uint8_t ControllerType;
uint8_t MouseAiming = 0;
uint8_t FullScreen = 0;

//
// Screen variables
//

int32_t ScreenWidth = 320;
int32_t ScreenHeight = 240;

//
// Mouse variables
//
//int32_t mouseSensitivity_X;
//int32_t mouseSensitivity_Y;
#if MOD_SUPPORT
static char  setupfilename[512];//={SETUPFILENAME};
#else
#define setupfilename SETUPFILENAME
#endif
#if USE_SCRIPTLIB
static int32_t scripthandle;
static int32_t setupread=0;
#endif
/*
===================
=
= CONFIG_GetSetupFilename
=
===================
*/
#define MAXSETUPFILES 20
void CONFIG_GetSetupFilename( void )
   {

#if MOD_SUPPORT
    int32_t i;
    setupfilename[0] = '\0';

   // Are we trying to load a mod?
   if(getGameDir()[0] != '\0'){
		FILE *fp = NULL;

	   //Yes
		sprintf(setupfilename, "%s\\%s", getGameDir(), SETUPFILENAME);

		// let's make sure it's actually there
		fp = fopen(setupfilename, "r");
		if(fp)
			fclose(fp);
        else{
			// It doesn't exist, so revert to the main one.
			printf("Config file: %s does not exist, using main config.\n", setupfilename);
			sprintf(setupfilename, "%s", SETUPFILENAME);
		}

   }else{
	   //No
		strcpy (setupfilename, SETUPFILENAME);
   }
#endif

   printf("Using Setup file: '%s'\n",setupfilename);
#if WIN32
   int i = clock()+(3*CLOCKS_PER_SEC/4);
   while (clock()<i){
      ;
   }
#endif
}

/*
===================
=
= CONFIG_FunctionNameToNum
=
===================
*/

int32_t CONFIG_FunctionNameToNum( char  * func )
   {
   int32_t i;

   for (i=0;i<NUMGAMEFUNCTIONS;i++)
      {
      if (!strcasecmp(func,gamefunctions[i]))
         {
         return i;
         }
      }
   return -1;
   }

/*
===================
=
= CONFIG_FunctionNumToName
=
===================
*/

const char  * CONFIG_FunctionNumToName( int32_t func )
{
	if (-1 < func && func < NUMGAMEFUNCTIONS)
	{
		return gamefunctions[func];
	}
	else
	{
		return NULL;
	}
}

/*
===================
=
= CONFIG_AnalogNameToNum
=
===================
*/
#if WIN32
int32_t CONFIG_AnalogNameToNum( char  * func )
   {

   if (!stricmp(func,"analog_turning"))
      {
      return analog_turning;
      }
   if (!stricmp(func,"analog_strafing"))
      {
      return analog_strafing;
      }
   if (!stricmp(func,"analog_moving"))
      {
      return analog_moving;
      }
   if (!stricmp(func,"analog_lookingupanddown"))
      {
      return analog_lookingupanddown;
      }

   return -1;
   }
#endif
/*
===================
=
= CONFIG_SetDefaults
=
===================
*/
#if WIN32
#include <stdio.h>
void dumpToFile(char *name, void *ptr, int size)
{
    FILE *fil;
    fil = fopen(name, "wb");
    fwrite(ptr, size, 1, fil);
    fclose(fil);
}

#define QUIET_TEST 1
#define NO_MUSIC 1
#else
#define QUIET_TEST 0
#define NO_MUSIC 0
#endif

void CONFIG_SetDefaults( void )
{
   // sound
   SoundToggle = !QUIET_TEST;
   MusicToggle = !NO_MUSIC;
   VoiceToggle = !QUIET_TEST;
   AmbienceToggle = 1;
   OpponentSoundToggle = 1;
   FXVolume = QUIET_TEST ? 0 : 220;
   MusicVolume = QUIET_TEST ? 0 : 200;
   FXDevice = SC_SoundScape;
   MusicDevice = -1;
   ReverseStereo = 0;

   // mouse
   for (int i = 0; i < max_axis; i++)
    CONTROL_SetAnalogSensitivity(i, 16);

   // game
   ps[0].aim_mode = 1;
   ud.screen_size = 8;
   ud.extended_screen_size = 0;
   ud.screen_tilting = 1;
   ud.brightness = 16;
   ud.auto_run = 1;
   ud.showweapons = 0;
   ud.tickrate = 1;
   ud.scrollmode = 0;
   ud.shadows = 1;
   ud.detail = 1;
#if LETS_WASTE_RAM_FOR_USERDEFS
   ud.lockout = 0;
   ud.pwlockout[0] = '\0';
#endif
   ud.crosshair = 1;
   ud.m_marker = 1; // for multiplayer
   ud.m_ffire = 1;
   ud.showcinematics = 1;
   ud.weaponautoswitch = 0;
   ud.hideweapon = 0;
   ud.auto_aim = 2; // full by default
   ud.gitdat_mdk = 0;
   ud.playing_demo_rev = 0;
   ud.mouseflip = 1;
    // next-hack: keybd

   kb_scancode key1,key2;
	// set default keys if we don't use scriptlib
    for(int i=0; i<NUMKEYENTRIES; i++){
        int32_t function = CONFIG_FunctionNameToNum(keydefaults[i].entryKey);
        key1 = (byte) KB_StringToScanCode( keydefaults[i].keyname1 );
        key2 = (byte) KB_StringToScanCode( keydefaults[i].keyname2 );
        CONTROL_MapKey( function, key1, key2 );
	}

#if 0
   // com
   strcpy(ud.rtsname,"DUKE.RTS");
   strcpy(ud.ridecule[0],"An inspiration for birth control.");
   strcpy(ud.ridecule[1],"You're gonna die for that!");
   strcpy(ud.ridecule[2],"It hurts to be you.");
   strcpy(ud.ridecule[3],"Lucky Son of a Bitch.");
   strcpy(ud.ridecule[4],"Hmmm....Payback time.");
   strcpy(ud.ridecule[5],"You bottom dwelling scum sucker.");
   strcpy(ud.ridecule[6],"Damn, you're ugly.");
   strcpy(ud.ridecule[7],"Ha ha ha...Wasted!");
   strcpy(ud.ridecule[8],"You suck!");
   strcpy(ud.ridecule[9],"AARRRGHHHHH!!!");
#endif

   // Controller
#if WIN32
	ControllerType = controltype_keyboardandmouse;
#else
    ControllerType = controltype_keyboardandjoystick;
#endif
}
int loadOptions(void)
{
    #if WIN32
        nvmData_t *nvm = &ext_memory[EXT_MEMORY_SIZE - sizeof(nvmData_t)];
    #else
        nvmData_t *nvm = (nvmData_t*) ((byte*) extMemGetSize() - sizeof(nvmData_t));
    #endif
    uint32_t cookie;
    LOAD_OPTION_FROM_EXTMEM(cookie);
    if (cookie != SAVEGAME_COOKIE)
    {
        printf("Settings cookie missing, not loading\r\n");
        return 0;
    }
    #if !WIN32
        settings_t stackSettings;
        extMemGetDataFromAddress(&stackSettings, &nvm->settings.s, sizeof(stackSettings));
        uint32_t calcCrc32 = crc32Compute((uint8_t *)&stackSettings, sizeof(stackSettings) - sizeof(uint32_t));
        uint32_t crc32;
        LOAD_OPTION_FROM_EXTMEM(crc32);

        if (calcCrc32 != crc32)
        {
            printf("Bad CRC32 on settings, read %x, computed %x, not loading\r\n", crc32, calcCrc32);
            return 0;
        }
    #endif
    // Todo: settings are already loaded, we could simply memcpy.
    LOAD_OPTION_FROM_EXTMEM(ud);
    LOAD_OPTION_FROM_EXTMEM(KeyMapping);
    LOAD_OPTION_FROM_EXTMEM(JoyAxisMapping);
    LOAD_OPTION_FROM_EXTMEM(axisSensitivities);
    //
    LOAD_OPTION_FROM_EXTMEM(myname);
    //
    // fix name
    myname[MAX_NAME_LEN - 1] = 0;
    for (uint32_t i = 0; i < sizeof(myname); i++)
    {
        char c = myname[i];
        if (( c == 0 && i == 0) || (i != 0 && c != 0 && (c < 32 || c > 96)))
        {
            strcpy(myname, "Player");
            break;
        }
    }
    //
    LOAD_OPTION_FROM_EXTMEM(SoundToggle);
    LOAD_OPTION_FROM_EXTMEM(MusicToggle);
    LOAD_OPTION_FROM_EXTMEM(VoiceToggle);
    LOAD_OPTION_FROM_EXTMEM(AmbienceToggle);
    LOAD_OPTION_FROM_EXTMEM(OpponentSoundToggle);
    LOAD_OPTION_FROM_EXTMEM(FXVolume);
    LOAD_OPTION_FROM_EXTMEM(MusicVolume);
    LOAD_OPTION_FROM_EXTMEM(FXDevice);
    LOAD_OPTION_FROM_EXTMEM(MusicDevice);
    LOAD_OPTION_FROM_EXTMEM(ReverseStereo);
    //
    //
    ototalclock = totalclock;
    return(1);

}

int saveOptions(void)
{
    MUSIC_StopSong();
    FX_StopAllSounds();
#if !WIN32
    updateAudio();
#endif
    // next-hack. Ok this is really crappy. We cannot really use the pointer, it's just for offsets!
    nvmData_t *nvm = (nvmData_t*) ((byte*) extMemGetSize() - sizeof(nvmData_t));
    extMemErase((uint32_t) &nvm->settings, sizeof(&nvm->settings));
    //
    SAVE_OPTION_TO_EXTMEM(ud);
    SAVE_OPTION_TO_EXTMEM(KeyMapping);
    SAVE_OPTION_TO_EXTMEM(JoyAxisMapping);
    SAVE_OPTION_TO_EXTMEM(axisSensitivities);
    //
    SAVE_OPTION_TO_EXTMEM(myname);
    //
    SAVE_OPTION_TO_EXTMEM(SoundToggle);
    SAVE_OPTION_TO_EXTMEM(MusicToggle);
    SAVE_OPTION_TO_EXTMEM(VoiceToggle);
    SAVE_OPTION_TO_EXTMEM(AmbienceToggle);
    SAVE_OPTION_TO_EXTMEM(OpponentSoundToggle);
    SAVE_OPTION_TO_EXTMEM(FXVolume);
    SAVE_OPTION_TO_EXTMEM(MusicVolume);
    SAVE_OPTION_TO_EXTMEM(FXDevice);
    SAVE_OPTION_TO_EXTMEM(MusicDevice);
    SAVE_OPTION_TO_EXTMEM(ReverseStereo);


    uint32_t cookie = SAVEGAME_COOKIE;
    SAVE_OPTION_TO_EXTMEM(cookie);
    uint32_t crc32;
    #if WIN32
        crc32 = 0;
    #else
        settings_t stackSettings;
        extMemGetDataFromAddress(&stackSettings, &nvm->settings.s, sizeof(stackSettings));
        crc32 = crc32Compute((uint8_t*) &stackSettings, sizeof(stackSettings) - sizeof(nvm->settings.s.crc32));
    #endif // WIN32
    SAVE_OPTION_TO_EXTMEM(crc32);
    //
    MUSIC_Continue();
    //
    ototalclock = totalclock;
    return(0);
}


/*
===================
=
= CONFIG_ReadKeys
=
===================
*/
#if USE_SCRIPTLIB
void CONFIG_ReadKeys( void )
   {
   int32_t i;
   int32_t numkeyentries;
   int32_t function;
   char  keyname1[80];
   char  keyname2[80];
   kb_scancode key1,key2;

	// set default keys in case duke3d.cfg was not found

	// FIX_00011: duke3d.cfg not needed anymore to start the game. Will create a default one
	//            if not found and use default keys.


	for(i=0; i<NUMKEYENTRIES; i++){
        function = CONFIG_FunctionNameToNum(keydefaults[i].entryKey);
        key1 = (byte) KB_StringToScanCode( keydefaults[i].keyname1 );
        key2 = (byte) KB_StringToScanCode( keydefaults[i].keyname2 );
        CONTROL_MapKey( function, key1, key2 );
	}


       numkeyentries = SCRIPT_NumberEntries( scripthandle, "KeyDefinitions" );

   for (i=0;i<numkeyentries;i++)  // i = number in which the functions appear in duke3d.cfg
      {
      function = CONFIG_FunctionNameToNum(SCRIPT_Entry( scripthandle, "KeyDefinitions", i ));
      if (function != -1)  // ensure it is in the list gamefunctions[function]
         {
         memset(keyname1,0,sizeof(keyname1));
         memset(keyname2,0,sizeof(keyname2));
         SCRIPT_GetDoubleString
            (
            scripthandle,
            "KeyDefinitions",
            SCRIPT_Entry( scripthandle,"KeyDefinitions", i ),
            keyname1,
            keyname2
            );
         key1 = 0;
         key2 = 0;
         if (keyname1[0])
            {
            key1 = (byte) KB_StringToScanCode( keyname1 );
            }
         if (keyname2[0])
            {
            key2 = (byte) KB_StringToScanCode( keyname2 );
            }
         CONTROL_MapKey( function, key1, key2 );
         }
      }
   }


/*
===================
=
= CONFIG_SetupMouse
=
===================
*/
void CONFIG_SetupMouse( int32_t scripthandle )
   {
   int32_t i;
   char  str[80];
   char  temp[80];
   int32_t function, scale;

   for (i=0;i<MAXMOUSEBUTTONS;i++)
      {
      sprintf(str,"MouseButton%d",i);
      memset(temp,0,sizeof(temp));
      SCRIPT_GetString( scripthandle,"Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      CONTROL_MapButton( function, i, false );
      sprintf(str,"MouseButtonClicked%d",i);
      memset(temp,0,sizeof(temp));
      SCRIPT_GetString( scripthandle,"Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      CONTROL_MapButton( function, i, true );
      }
   // map over the axes
   for (i=0;i<MAXMOUSEAXES;i++)
      {
      sprintf(str,"MouseAnalogAxes%d",i);
      memset(temp,0,sizeof(temp));
      SCRIPT_GetString(scripthandle, "Controls", str,temp);
      function = CONFIG_AnalogNameToNum(temp);
      if (function != -1)
         {
         //TODO Fix the Analog mouse axis issue. Just make a new function for registering them.
         //CONTROL_MapAnalogAxis(i,function);
         }
      sprintf(str,"MouseDigitalAxes%d_0",i);
      memset(temp,0,sizeof(temp));
      SCRIPT_GetString(scripthandle, "Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      CONTROL_MapDigitalAxis( i, function, 0 );
      sprintf(str,"MouseDigitalAxes%d_1",i);
      memset(temp,0,sizeof(temp));
      SCRIPT_GetString(scripthandle, "Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      CONTROL_MapDigitalAxis( i, function, 1 );
      sprintf(str,"MouseAnalogScale%d",i);
      SCRIPT_GetNumber(scripthandle, "Controls", str,&scale);
      //TODO: Fix the Analog mouse scale issue. Just make a new function for registering them.
	  //CONTROL_SetAnalogAxisScale( i, scale );
      }

	SCRIPT_GetNumber( scripthandle, "Controls","MouseSensitivity_X_Rancid",&mouseSensitivity_X);
	if(mouseSensitivity_X>63 || mouseSensitivity_X < 0)
		mouseSensitivity_X  = 15;
	// FIX_00014: Added Y cursor setup for mouse sensitivity in the menus
	// Copy Sensitivity_X into Sensitivity_Y in case it is not set.
	mouseSensitivity_Y = mouseSensitivity_X;
	SCRIPT_GetNumber( scripthandle, "Controls","MouseSensitivity_Y_Rancid",&mouseSensitivity_Y);
	if(mouseSensitivity_Y>63 || mouseSensitivity_Y < 0)
		mouseSensitivity_Y  = 15;

   }

/*
===================
=
= CONFIG_SetupGamePad
=
===================
*/

void CONFIG_SetupGamePad( int32_t scripthandle )
   {
   int32_t i;
   char  str[80];
   char  temp[80];
   int32_t function;


   for (i=0;i<MAXJOYBUTTONS;i++)
      {
      sprintf(str,"JoystickButton%d",i);
      memset(temp,0,sizeof(temp));
      SCRIPT_GetString( scripthandle,"Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      if (function != -1)
         CONTROL_MapButton( function, i, false );
      sprintf(str,"JoystickButtonClicked%d",i);
      memset(temp,0,sizeof(temp));
      SCRIPT_GetString( scripthandle,"Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      if (function != -1)
         CONTROL_MapButton( function, i, true );
      }
   // map over the axes
   for (i=0;i<MAXGAMEPADAXES;i++)
      {
      sprintf(str,"GamePadDigitalAxes%d_0",i);
      memset(temp,0,sizeof(temp));
      SCRIPT_GetString(scripthandle, "Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      if (function != -1)
         CONTROL_MapDigitalAxis( i, function, 0 );
      sprintf(str,"GamePadDigitalAxes%d_1",i);
      memset(temp,0,sizeof(temp));
      SCRIPT_GetString(scripthandle, "Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      if (function != -1)
         CONTROL_MapDigitalAxis( i, function, 1 );
      }
   SCRIPT_GetNumber( scripthandle, "Controls","JoystickPort",&function);
   CONTROL_JoystickPort = function;
   }

/*
===================
=
= CONFIG_SetupJoystick
=
===================
*/

void CONFIG_SetupJoystick( int32_t scripthandle )
{

   int32_t i, j;
   char  str[80];
   char  temp[80];
   int32_t function, deadzone;
   float scale;

   for (i=0;i<MAXJOYBUTTONS;i++)
      {
      sprintf(str,"JoystickButton%d",i);
      memset(temp,0,sizeof(temp));
      SCRIPT_GetString( scripthandle,"Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      if (function != -1)
         CONTROL_MapJoyButton( function, i, false );
      sprintf(str,"JoystickButtonClicked%d",i);
      memset(temp,0,sizeof(temp));
      SCRIPT_GetString( scripthandle,"Controls", str,temp);
      function = CONFIG_FunctionNameToNum(temp);
      if (function != -1)
         CONTROL_MapJoyButton( function, i, true );
      }
   // map over the axes
   for (i=0;i<MAXJOYAXES;i++)
      {
        sprintf(str,"JoystickAnalogAxes%d",i);
        memset(temp,0,sizeof(temp));
        SCRIPT_GetString(scripthandle, "Controls", str,temp);
        function = CONFIG_AnalogNameToNum(temp);
        //if (function != -1)
            //{
            CONTROL_MapAnalogAxis(i,function);
            //}
        sprintf(str,"JoystickDigitalAxes%d_0",i);
        memset(temp,0,sizeof(temp));
        SCRIPT_GetString(scripthandle, "Controls", str,temp);
        function = CONFIG_FunctionNameToNum(temp);
        if (function != -1)
            CONTROL_MapDigitalAxis( i, function, 0 );
        sprintf(str,"JoystickDigitalAxes%d_1",i);
        memset(temp,0,sizeof(temp));
        SCRIPT_GetString(scripthandle, "Controls", str,temp);
        function = CONFIG_FunctionNameToNum(temp);
        if (function != -1)
            CONTROL_MapDigitalAxis( i, function, 1 );
        sprintf(str,"JoystickAnalogScale%d",i);
        SCRIPT_GetFloat(scripthandle, "Controls", str,&scale);
        CONTROL_SetAnalogAxisScale( i, scale );
        deadzone = 0;
        sprintf(str,"JoystickAnalogDeadzone%d",i);
        SCRIPT_GetNumber(scripthandle, "Controls", str, &deadzone);
        CONTROL_SetAnalogAxisDeadzone( i, deadzone);
      }

   // map over the "top hats"
   for (i=0; i < MAXJOYHATS; i++)
   {
	  for(j=0; j < 8; j++) // 8? because hats can have 8 different values
	  {
		  sprintf(str,"JoystickHat%d_%d",i, j);
		  memset(temp,0,sizeof(temp));
		  SCRIPT_GetString( scripthandle,"Controls", str,temp);
		  function = CONFIG_FunctionNameToNum(temp);
		  if (function != -1)
		  {
			  CONTROL_MapJoyHat( function, i, j);
		  }
	  }
   }

   // read in JoystickPort
   SCRIPT_GetNumber( scripthandle, "Controls","JoystickPort",&function);
   CONTROL_JoystickPort = function;
   // read in rudder state
   SCRIPT_GetNumber( scripthandle, "Controls","EnableRudder",(int32_t*)&CONTROL_RudderEnabled);
}
#endif
void readsavenames(void)
{
    #if NOT_YET_IMPLEMENTED
    int32_t dummy;
    short i;
    uint8_t  fn[] = "game_.sav";
    FILE *fil;
	char  fullpathsavefilename[16];


    for (i=0;i<10;i++)
    {

        fn[4] = i+'0';

		// Are we loading a TC?
		if(getGameDir()[0] != '\0')
		{
			// Yes
			sprintf(fullpathsavefilename, "%s\\%s", getGameDir(), fn);
		}
		else
		{
			// No
			sprintf(fullpathsavefilename, "%s", fn);
		}

        if ((fil = fopen(fullpathsavefilename,"rb")) == NULL ) continue;
        dfread(&dummy,4,1,fil);

		//	FIX_00015: Backward compliance with older demos (down to demos v27, 28, 116 and 117 only)
        if(	dummy != BYTEVERSION	 &&
			dummy != BYTEVERSION_27  &&
			dummy != BYTEVERSION_28  &&
			dummy != BYTEVERSION_116 &&
			dummy != BYTEVERSION_117) continue;
        // FIX_00092: corrupted saved files making the following saved files invisible (Bryzian)
		dfread(&dummy,4,1,fil);
        dfread(&ud.savegame[i][0],19,1,fil);
        fclose(fil);
    }
    #else
    int32_t dummy;
        for (int i = 0; i < MAX_SAVEGAMES; i++)
            loadpheader(i, &dummy, &dummy, &dummy, &dummy);
    #endif
}

/*
===================
=
= CONFIG_ReadSetup
=
===================
*/

//int32 dukever13;

void CONFIG_ReadSetup( void )
{
#if WIN32
   FILE* setup_file_hdl;

   printf("CONFIG_ReadSetup...\n");

   if (!SafeFileExists(setupfilename))
      {
		// FIX_00011: duke3d.cfg not needed anymore to start the game. Will create a default one
		//            if not found and use default keys.
      printf("%s does not exist. Don't forget to set it up!\n" ,setupfilename);
	  setup_file_hdl = fopen (setupfilename, "w"); // create it...
	  if(setup_file_hdl)
		  fclose(setup_file_hdl);
      }
#endif
#if !WIN32
   crc32Init();
#endif
   loadOptions();
   //CONFIG_SetDefaults();
#if USE_SCRIPTLIB
   int32_t dummy;
   char  commmacro[] = COMMMACRO;
   scripthandle = SCRIPT_Load( setupfilename );

   for(dummy = 0;dummy < 10;dummy++)
   {
       commmacro[13] = dummy+'0';
       SCRIPT_GetString( scripthandle, "Comm Setup",commmacro,ud.ridecule[dummy]);
   }



   SCRIPT_GetString( scripthandle, "Comm Setup","PlayerName",&myname[0]);

   dummy = CheckParm("NAME");
   if( dummy ) strncpy(myname,_argv[dummy+1], sizeof(myname) -1);
   dummy = CheckParm("MAP");



	if( dummy )
	{
		if (!VOLUMEONE)
		{
			//boardfilename might be set from commandline only zero if we are replacing
			boardfilename[0] = 0;
			strncpy(boardfilename,_argv[dummy+1], sizeof(boardfilename));
			if( strchr(boardfilename,'.') == 0)
				strncat(boardfilename,".map", sizeof(boardfilename));
			printf("Using level: '%s'.\n",boardfilename);
		}
		else
		{
			Error(EXIT_SUCCESS, "The -map option does not work with the Shareware version of duke3d.grp\n"
								"Change your duke3d.grp file to the 1.3d version or 1.5 Atomic version\n");
		}
	}

   SCRIPT_GetString( scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "Shadows",&ud.shadows);
   SCRIPT_GetString( scripthandle, "Screen Setup","Password",&ud.pwlockout[0]);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "Detail",&ud.detail);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "Tilt",&ud.screen_tilting);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "Messages",&ud.fta_on);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ScreenWidth",&ScreenWidth);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ScreenHeight",&ScreenHeight);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "FullScreen",&FullScreen);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ScreenGamma",&ud.brightness);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ScreenSize",&ud.screen_size);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ExtScreenSize",&ud.extended_screen_size);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "Out",&ud.lockout);
   SCRIPT_GetNumber( scripthandle, "Screen Setup", "ShowFPS",&ud.tickrate);
   ud.tickrate &= 1;
   SCRIPT_GetNumber( scripthandle, "Misc", "Executions",&ud.executions);
   ud.executions++;
   SCRIPT_GetNumber( scripthandle, "Misc", "RunMode",&ud.auto_run);
   SCRIPT_GetNumber( scripthandle, "Misc", "Crosshairs",&ud.crosshair);
   SCRIPT_GetNumber( scripthandle, "Misc", "ShowCinematics",&ud.showcinematics);
   SCRIPT_GetNumber( scripthandle, "Misc", "WeaponAutoSwitch",&ud.weaponautoswitch);
   SCRIPT_GetNumber( scripthandle, "Misc", "HideWeapon",&ud.hideweapon);
   SCRIPT_GetNumber( scripthandle, "Misc", "ShowWeapon",&ud.showweapons);
   SCRIPT_GetNumber( scripthandle, "Misc", "AutoAim",&ud.auto_aim);
	if(ud.auto_aim!=1 && ud.auto_aim != 2)
		ud.auto_aim = 2; // avoid people missing with the cfg to go in a deadlock
   SCRIPT_GetNumber( scripthandle, "Misc", "GitDatMdk",&ud.gitdat_mdk);

   if(ud.mywchoice[0] == 0 && ud.mywchoice[1] == 0)
   {
       ud.mywchoice[0] = 3;
       ud.mywchoice[1] = 4;
       ud.mywchoice[2] = 5;
       ud.mywchoice[3] = 7;
       ud.mywchoice[4] = 8;
       ud.mywchoice[5] = 6;
       ud.mywchoice[6] = 0;
       ud.mywchoice[7] = 2;
       ud.mywchoice[8] = 9;
       ud.mywchoice[9] = 1;

       for(dummy=0;dummy<10;dummy++)
       {
           sprintf(buf,"WeaponChoice%d",dummy);
           SCRIPT_GetNumber( scripthandle, "Misc", buf, &ud.mywchoice[dummy]);
       }
    }
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "FXDevice",&FXDevice);

	// reimplementation of ASS expects a "SoundScape".
	if (FXDevice != SC_Unknown)
	    FXDevice = SC_SoundScape;

   SCRIPT_GetNumber( scripthandle, "Sound Setup", "MusicDevice",&MusicDevice);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "FXVolume",&FXVolume);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "MusicVolume",&MusicVolume);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "SoundToggle",&SoundToggle);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "MusicToggle",&MusicToggle);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "VoiceToggle",&VoiceToggle);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "AmbienceToggle",&AmbienceToggle);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "OpponentSoundToggle",&OpponentSoundToggle);

#if USE_DEFAULT_VALUES
//   NumVoices = 32;
   NumChannels = 2;
   NumBits = 16;
   MixRate = 44100;
#else
//   NumVoices = 8;
   NumChannels = 2;
   NumBits = 16;
   MixRate = 11025;
#endif

   SCRIPT_GetNumber( scripthandle, "Sound Setup", "MidiPort",&MidiPort);
   SCRIPT_GetNumber( scripthandle, "Sound Setup", "ReverseStereo",&ReverseStereo);
   SCRIPT_GetNumber( scripthandle, "Controls","ControllerType",&ControllerType);
   SCRIPT_GetNumber( scripthandle, "Controls","MouseAimingFlipped",&ud.mouseflip);
   SCRIPT_GetNumber( scripthandle, "Controls","MouseAiming",&MouseAiming);
   SCRIPT_GetNumber( scripthandle, "Controls","GameMouseAiming",(int32_t *)&ps[0].aim_mode);
   SCRIPT_GetNumber( scripthandle, "Controls","AimingFlag",(int32_t *)&myaimmode);

   CONTROL_ClearAssignments();

   CONFIG_ReadKeys();

   switch (ControllerType)
      {
        case controltype_keyboardandmouse:
            {
				CONFIG_SetupMouse(scripthandle);
			}
            break;
        case controltype_keyboardandjoystick:
        case controltype_keyboardandflightstick:
        case controltype_keyboardandthrustmaster:
			{
            CONTROL_JoystickEnabled = 1;
            CONFIG_SetupJoystick(scripthandle);
			}
            break;
        case controltype_keyboardandgamepad:
            {
				CONFIG_SetupGamePad(scripthandle);
			}
            break;
        case controltype_joystickandmouse:
            {

                CONTROL_JoystickEnabled = 1;
                CONFIG_SetupJoystick(scripthandle);
                CONFIG_SetupMouse(scripthandle);
            }
            break;
        default:
            {
				CONFIG_SetupMouse(scripthandle);
			}
      }
   setupread = 1;
#endif
}

/*
===================
=
= CONFIG_WriteSetup
=
===================
*/

void CONFIG_WriteSetup( void )
{
    #if USE_SCRIPTLIB
   int32_t dummy, i;
   char  commmacro[] = COMMMACRO;
    char tempbuf[64];
   if (!setupread) return;

   printf("CONFIG_WriteSetup...\n");

   SCRIPT_PutNumber( scripthandle, "Screen Setup", "Shadows",ud.shadows,false,false);
   SCRIPT_PutString( scripthandle, "Screen Setup", "Password",ud.pwlockout);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "Detail",ud.detail,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "Tilt",ud.screen_tilting,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "Messages",ud.fta_on,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "Out",ud.lockout,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ShowFPS",ud.tickrate&1,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ScreenWidth",xdim,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ScreenHeight",ydim,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "FullScreen",FullScreen,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "FXVolume",FXVolume,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "MusicVolume",MusicVolume,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "FXDevice",FXDevice,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "MusicDevice",MusicDevice,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "SoundToggle",SoundToggle,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "VoiceToggle",VoiceToggle,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "AmbienceToggle",AmbienceToggle,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "OpponentSoundToggle",OpponentSoundToggle,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "MusicToggle",MusicToggle,false,false);
   SCRIPT_PutNumber( scripthandle, "Sound Setup", "ReverseStereo",ReverseStereo,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ScreenSize",ud.screen_size,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ExtScreenSize",ud.extended_screen_size,false,false);
   SCRIPT_PutNumber( scripthandle, "Screen Setup", "ScreenGamma",ud.brightness,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "Executions",ud.executions,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "RunMode",ud.auto_run,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "Crosshairs",ud.crosshair,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "ShowCinematics",ud.showcinematics,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "HideWeapon",ud.hideweapon,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "ShowWeapon",ud.showweapons,false,false);
   SCRIPT_PutNumber( scripthandle, "Misc", "WeaponAutoSwitch",ud.weaponautoswitch,false,false);
   if( nHostForceDisableAutoaim == 0) // do not save Host request to have AutoAim Off.
	   SCRIPT_PutNumber( scripthandle, "Misc", "AutoAim",ud.auto_aim,false,false);
   SCRIPT_PutNumber( scripthandle, "Controls", "MouseAimingFlipped",ud.mouseflip,false,false);
   SCRIPT_PutNumber( scripthandle, "Controls","MouseAiming",MouseAiming,false,false);
   SCRIPT_PutNumber( scripthandle, "Controls","GameMouseAiming",(int32_t) ps[myconnectindex].aim_mode,false,false);
   SCRIPT_PutNumber( scripthandle, "Controls","AimingFlag",(int32_t) myaimmode,false,false);

	// FIX_00016: Build in Keyboard/mouse setup. Mouse now faster.
	for(i=0; i<MAXMOUSEBUTTONS; i++)
	{
		snprintf((char *)tempbuf, sizeof(tempbuf), "MouseButton%d", i);
		SCRIPT_PutString(scripthandle, "Controls", (char *)tempbuf,
			(MouseMapping[i]!=-1)?CONFIG_FunctionNumToName(MouseMapping[i]):"");
	}

	for (i=0;i<MAXMOUSEAXES*2;i++)
	{
		sprintf((char *)tempbuf, "MouseDigitalAxes%d_%d", i>>1, i&1);
		SCRIPT_PutString(scripthandle, "Controls", (char *)tempbuf,
			(MouseDigitalAxeMapping[i>>1][i&1]!=-1)?CONFIG_FunctionNumToName(MouseDigitalAxeMapping[i>>1][i&1]):"");
	}

   for(i=0; i<NUMGAMEFUNCTIONS; i++) // write keys
   {
		SCRIPT_PutDoubleString(
			scripthandle,
			"KeyDefinitions",
			gamefunctions[i],
			KB_ScanCodeToString( KeyMapping[i].key1 )?KB_ScanCodeToString( KeyMapping[i].key1 ):"",
			KB_ScanCodeToString( KeyMapping[i].key2 )?KB_ScanCodeToString( KeyMapping[i].key2 ):"");
	}

   for(dummy=0;dummy<10;dummy++)
   {
       sprintf(buf,"WeaponChoice%d",dummy);
       SCRIPT_PutNumber( scripthandle, "Misc",buf,ud.mywchoice[dummy],false,false);
   }

   dummy = CONTROL_GetMouseSensitivity_X();
   SCRIPT_PutNumber( scripthandle, "Controls","MouseSensitivity_X_Rancid",dummy,false,false);

   dummy = CONTROL_GetMouseSensitivity_Y();
   SCRIPT_PutNumber( scripthandle, "Controls","MouseSensitivity_Y_Rancid",dummy,false,false);

   SCRIPT_PutNumber( scripthandle, "Controls","ControllerType",ControllerType,false,false);

   SCRIPT_PutString( scripthandle, "Comm Setup","PlayerName",myname);
   SCRIPT_PutString( scripthandle, "Comm Setup","RTSName",ud.rtsname);

   for(dummy = 0;dummy < 10;dummy++)
   {
       commmacro[13] = dummy+'0';
       SCRIPT_PutString( scripthandle, "Comm Setup",commmacro,ud.ridecule[dummy]);
   }

   SCRIPT_Save (scripthandle, setupfilename);
   SCRIPT_Free (scripthandle);
   #endif
   }

