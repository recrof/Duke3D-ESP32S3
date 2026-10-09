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

//***************************************************************************
//
// Public header for CONTROL.C.
//
//***************************************************************************

#ifndef _control_public
#define _control_public

#include "keyboard.h"

#ifdef __cplusplus
extern "C" {
#endif


//***************************************************************************
//
// DEFINES
//
//***************************************************************************

#define MaxJoys             2
#define MAXGAMEBUTTONS      64

	/*
#define BUTTON(x) \
    ( \
    ((x)>31) ? \
    ((CONTROL_ButtonState2>>( (x) - 32) ) & 1) :\
    ((CONTROL_ButtonState1>> (x) ) & 1)          \
    )
	*/
	/*
#define BUTTONHELD(x) \
    ( \
    ((x)>31) ? \
    ((CONTROL_ButtonHeldState2>>((x)-32)) & 1) :\
    ((CONTROL_ButtonHeldState1>>(x)) & 1)\
    )
#define BUTTONJUSTPRESSED(x) \
    ( BUTTON( x ) && !BUTTONHELD( x ) )
#define BUTTONRELEASED(x) \
    ( !BUTTON( x ) && BUTTONHELD( x ) )
#define BUTTONSTATECHANGED(x) \
    ( BUTTON( x ) != BUTTONHELD( x ) )
	*/

//***************************************************************************
//
// TYPEDEFS
//
//***************************************************************************
typedef enum
   {
   axis_up,
   axis_down,
   axis_left,
   axis_right
   } axisdirection;
// next-hack reordered
typedef enum
   {
   analog_turning=0,
   analog_strafing=1,
   analog_lookingupanddown=2,
   analog_moving = 3,
#if 0
   analog_elevation,
   analog_rolling,
#endif
   analog_maxtype
   } analogcontrol;

typedef enum
   {
   dir_North,
   dir_NorthEast,
   dir_East,
   dir_SouthEast,
   dir_South,
   dir_SouthWest,
   dir_West,
   dir_NorthWest,
   dir_None
   } direction;
typedef enum
{
    left_x_axis = 0,
    left_y_axis,
    right_x_axis,
    right_y_axis,
    max_axis
} axis_type_t;
typedef struct
   {
   boolean   button0;
   boolean   button1;
   direction dir;
   } UserInput;


typedef struct
   {
   fixed     dx;
   fixed     dy;
   fixed     dz;
   fixed     dyaw;
   fixed     dpitch;
   fixed     droll;
   } ControlInfo;

/*
typedef struct
   {
   float     dx;
   float     dy;
   float     dz;
   float     dyaw;
   float     dpitch;
   float     droll;
   } ControlInfo;
*/

typedef enum
   {
   controltype_keyboard,
   controltype_keyboardandmouse,
   controltype_keyboardandjoystick,
   controltype_keyboardandexternal,
   controltype_keyboardandgamepad,
   controltype_keyboardandflightstick,
   controltype_keyboardandthrustmaster,
   controltype_joystickandmouse
   } controltype;


//***************************************************************************
//
// GLOBALS
//
//***************************************************************************

extern uint32_t   CONTROL_RudderEnabled;
extern boolean  CONTROL_MousePresent;
extern boolean  CONTROL_JoysPresent[ MaxJoys ];
extern boolean  CONTROL_MouseEnabled;
extern boolean  CONTROL_JoystickEnabled;
extern byte     CONTROL_JoystickPort;
extern uint32_t   CONTROL_MouseButtonState1;
extern uint32_t   CONTROL_MouseButtonState2;
//extern uint32   CONTROL_ButtonHeldState1;
//extern uint32   CONTROL_ButtonHeldState2;
extern uint32_t   CONTROL_JoyButtonState;
extern int32_t axisSensitivities[max_axis];

//***************************************************************************
//
// PROTOTYPES
//
//***************************************************************************
typedef struct _KeyMapping
{
	boolean key_active;
	kb_scancode key1;
	kb_scancode key2;

	/* other mappings go here */
} KeyMapping_t;
extern KeyMapping_t KeyMapping[MAXGAMEBUTTONS];
extern const char * const analogFunctions[analog_maxtype];

//extern int32_t MouseMapping[MAXMOUSEBUTTONS];
//extern int32_t MouseDigitalAxeMapping[MAXMOUSEAXES][2];  // [axesX/Y][directionLeft/Right or directionUp/Down]
extern int8_t JoyAxisMapping[MAXJOYAXES];

int  ACTION(int i);
int  RESET_ACTION(int i);
void RESBUTTON(int i);
void CONTROL_MapKey( int32_t which, kb_scancode key1, kb_scancode key2 );
void CONTROL_MapButton
        (
        int32_t whichfunction,
        int32_t whichbutton,
        boolean clicked_or_doubleclicked
        );
void CONTROL_MapJoyButton(int32_t whichfunction, int32_t whichbutton, boolean doubleclicked);
void CONTROL_MapJoyHat(int32_t whichfunction, int32_t whichhat, int32_t whichvalue);
void CONTROL_DefineFlag( int32_t which, boolean toggle );
boolean CONTROL_FlagActive( int32_t which );
void CONTROL_ClearAssignments( void );
void CONTROL_GetUserInput( UserInput *info );
void CONTROL_GetInput( ControlInfo *info );
void CONTROL_ClearAction( int32_t whichbutton );
void CONTROL_ClearUserInput( UserInput *info );
void CONTROL_WaitRelease( void );
void CONTROL_Ack( void );
void CONTROL_CenterJoystick
   (
   void ( *CenterCenter )( void ),
   void ( *UpperLeft )( void ),
   void ( *LowerRight )( void ),
   void ( *CenterThrottle )( void ),
   void ( *CenterRudder )( void )
   );
int32_t CONTROL_GetAnalogSensitivity( axis_type_t axis );
void CONTROL_SetAnalogSensitivity( axis_type_t axis, int32_t newsensitivity);
void CONTROL_Startup
   (
   controltype which,
   int32_t ( *TimeFunction )( void ),
   int32_t ticspersecond
   );
void CONTROL_Shutdown( void );

void CONTROL_MapAnalogAxis
   (
   int32_t whichaxis,
   int32_t whichanalog
   );

void CONTROL_MapDigitalAxis
   (
   int32_t whichaxis,
   int32_t whichfunction,
   int32_t direction
   );
void CONTROL_SetAnalogAxisScale
   (
   int32_t whichaxis,
   float axisscale
   );
void CONTROL_SetAnalogAxisDeadzone
   (
   int32_t whichaxis,
   int32_t axisdeadzone
   );
int32_t CONTROL_FilterDeadzone
   (
   int32_t axisvalue,
   int32_t axisdeadzone
   );
int32_t CONTROL_GetFilteredAxisValue(int32_t axis);
void CONTROL_PrintAxes( void );

void CONTROL_UpdateKeyboardState(int key, int pressed);

#ifdef __cplusplus
};
#endif
#endif

