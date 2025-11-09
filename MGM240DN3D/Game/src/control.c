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
#if WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
#include <string.h>

#include "duke3d.h"
#include "config.h"
#include "control.h"
#include "display.h"
#include "engine.h"
#include "function.h"
#include "joystick.h"
#include "mouse.h"
#if MG24
#include "port/keyboard.h"
#endif

//***************************************************************************
//
// GLOBALS
//
//***************************************************************************

// FIX_00018: Removed the "smoothmouse" option. This was just a bad fix. Mouse is now faster,
//            smoother.
// extern int g_CV_EnableSmoothMouse;
// extern int g_CV_SmoothMouseSensX;
// extern int g_CV_SmoothMouseSensY;

uint32_t   CONTROL_RudderEnabled;
boolean  CONTROL_MousePresent;
boolean  CONTROL_JoysPresent[ MaxJoys ];
boolean  CONTROL_MouseEnabled;
boolean  CONTROL_JoystickEnabled;
byte     CONTROL_JoystickPort;
uint32_t   CONTROL_MouseButtonState1;
uint32_t   CONTROL_MouseButtonState2;
// FIX_00019: DigitalAxis Handling now supported. (cool for medkit use)
uint32_t   CONTROL_MouseDigitalAxisState1;
uint32_t   CONTROL_MouseDigitalAxisState2;
//uint32   CONTROL_ButtonHeldState1;
//uint32   CONTROL_ButtonHeldState2;
uint32_t   CONTROL_JoyButtonState1;
uint32_t   CONTROL_JoyButtonState2;

uint32_t   CONTROL_JoyHatState1; //[MAXJOYHATS];
uint32_t   CONTROL_JoyHatState2; //[MAXJOYHATS];


static short mouseButtons = 0;
//static short lastmousebuttons = 0;
#if WIN32
static short joyHats[MAXJOYHATS];
static short lastjoyHats[MAXJOYHATS];
#endif
static int32_t mousePositionX = 0;
static int32_t mousePositionY = 0;
static int32_t mouseRelativeX = 0;
static int32_t mouseRelativeY = 0;
//

KeyMapping_t KeyMapping[MAXGAMEBUTTONS];
#if 0
int32_t MouseDigitalAxeMapping[MAXMOUSEAXES][2];  // [axesX/Y][directionLeft/Right or directionUp/Down]
int32_t MouseMapping[MAXMOUSEBUTTONS];
#endif

//***************************************************************************
//
// FUNCTIONS
//
//***************************************************************************

// Joystick/Gamepad bindings
int8_t JoyAxisMapping[MAXJOYAXES] = {analog_strafing, analog_moving, analog_turning, analog_lookingupanddown};  // lx, ly, rx, ry
#if 0
static int32_t JoyHatMapping[MAXJOYHATS][8];
static int32_t JoyButtonMapping[MAXJOYBUTTONS];
static float JoyAnalogScale[MAXJOYAXES];
static int32_t JoyAnalogDeadzone[MAXJOYAXES];
#endif

const char * const analogFunctions[analog_maxtype] = {"TURN", "STRAFE", "LOOK", "MOVE"};

int ACTION(int i)
{

	//Keyboard input
	if( (get_KB_KeyDown(KeyMapping[i].key1)) ||
		(get_KB_KeyDown(KeyMapping[i].key2))
	  )
	{
		return 1;
	}


	// Check mouse
	if((ControllerType == controltype_keyboardandmouse) ||
		(ControllerType == controltype_joystickandmouse))
	{
		//Mouse buttons
		if ((i)>31)
		{
			if((CONTROL_MouseButtonState2>>( (i) - 32) ) & 1)
			{
				return 1;
			}
		}
		else
		{
			if((CONTROL_MouseButtonState1>> (i) ) & 1)
			{
				return 1;
			}
		}

		// FIX_00019: DigitalAxis Handling now supported. (cool for medkit use)

		//Mouse Digital Axes
		if ((i)>31)
		{
			if((CONTROL_MouseDigitalAxisState2>>( (i) - 32) ) & 1)
			{
				return 1;
			}
		}
		else
		{
			if((CONTROL_MouseDigitalAxisState1>> (i) ) & 1)
			{
				return 1;
			}
		}
	}


	// Check joystick
	if((ControllerType == controltype_keyboardandjoystick) ||
		(ControllerType == controltype_joystickandmouse)
	  )
	{

		if ((i)>31)
		{
			// Check the joystick
			if( (CONTROL_JoyButtonState2 >> (i - 32)) & 1)
			{
				return 1;
			}

			// Check the hats
			if( (CONTROL_JoyHatState2 >> (i - 32)) & 1)
			{
				return 1;
			}

		}
		else
		{
			if( (CONTROL_JoyButtonState1 >> i) & 1)
			{
				return 1;
			}

			// Check the hats
			if( (CONTROL_JoyHatState1 >> i) & 1)
			{
				return 1;
			}
		}

	}

	return 0;
}


int RESET_ACTION(int i)
{
	set_KB_KeyDown(KeyMapping[i].key1, 0);
	set_KB_KeyDown(KeyMapping[i].key2, 0);

	return 0;
}
#if 0
static void SETMOUSEBUTTON(int i)
{

	//CONTROL_MouseButtonState1 |= (1<<i);

	int b;

	if (i < 32)
	{
		b = 1 << i;

		CONTROL_MouseButtonState1 |= b;
	}
	else
	{
		i -= 32;

		b = 1 << i;

		CONTROL_MouseButtonState2 |= b;
	}

}

void RESMOUSEBUTTON(int i)
{

	//CONTROL_MouseButtonState1 &= ~(1<<i);

	int b;

	if (i < 32) {
		b = 1 << i;

		CONTROL_MouseButtonState1 &= ~b;
	} else {
		i -= 32;

		b = 1 << i;

		CONTROL_MouseButtonState2 &= ~b;
	}

}

// FIX_00019: DigitalAxis Handling now supported. (cool for medkit use)

void SETMOUSEDIGITALAXIS(int i)
{

	int b;

	if (i<0)
		return;

	if (i < 32)
	{
		b = 1 << i;
		CONTROL_MouseDigitalAxisState1 |= b;
	}
	else
	{
		i -= 32;
		b = 1 << i;
		CONTROL_MouseDigitalAxisState2 |= b;
	}
}

void RESMOUSEDIGITALAXIS(int i)
{

	int b;

	if (i<0)
		return;

	if (i < 32)
	{
		b = 1 << i;
		CONTROL_MouseDigitalAxisState1 &= ~b;
	}
	else
	{
		i -= 32;
		b = 1 << i;
		CONTROL_MouseDigitalAxisState2 &= ~b;
	}
}

static void SETJOYBUTTON(int i)
{
	//CONTROL_JoyButtonState |= (1<<i);
	int b;

	if (i < 32)
	{
		b = 1 << i;

		CONTROL_JoyButtonState1 |= b;
	}
	else
	{
		i -= 32;

		b = 1 << i;

		CONTROL_JoyButtonState2 |= b;
	}
}
static void RESJOYBUTTON(int i)
{
	int b;

	if (i < 32) {
		b = 1 << i;

		CONTROL_JoyButtonState1 &= ~b;
	} else {
		i -= 32;

		b = 1 << i;

		CONTROL_JoyButtonState2 &= ~b;
	}
}

static void SETHATBUTTON(int i)
{
	//CONTROL_JoyHatState1 |= (1<<i);

	int b;

	if (i < 32)
	{
		b = 1 << i;

		CONTROL_JoyHatState1 |= b;
	}
	else
	{
		i -= 32;

		b = 1 << i;

		CONTROL_JoyHatState2 |= b;
	}
}

static void RESHATBUTTON(int i)
{

	//CONTROL_JoyHatState1 &= ~(1<<i);
	int b;

	if (i < 32) {
		b = 1 << i;

		CONTROL_JoyHatState1 &= ~b;
	} else {
		i -= 32;

		b = 1 << i;

		CONTROL_JoyHatState2 &= ~b;
	}
}
#endif
#if WIN32
void CONTROL_UpdateKeyboardState(int key, int pressed)
{
	/*

		if(pressed)
		{
			CONTROL_KeyStates[key] = 1;
		}
		else
		{
			CONTROL_KeyStates[key] = 0;
		}
		*/
	/*
	int i;

	for (i = 0; i < MAXGAMEBUTTONS; i++)
	{
		if (KeyMapping[i].key_active == false)
		{
			continue;
		}

		if (KeyMapping[i].key1 == key ||
			KeyMapping[i].key2 == key)
		{

			if (pressed)
			{
				SETBUTTON(i);
			}
			else
			{
				RESBUTTON(i);
			}
		}
	}
	*/
}
#endif
void CONTROL_MapKey( int32_t which, kb_scancode key1, kb_scancode key2 )
{
	// FIX_00020: Protect you from assigning a function to the ESC key through duke3d.cfg
	if(key1==sc_Escape || key2==sc_Escape)
	{
		if(key1==sc_Escape)
			key1=0;
		else
			key2=0;

		printf("Discarding ESCAPE key for function : %s\n", gamefunctions[which]);
	}

	if(key1 || key2)
		KeyMapping[which].key_active = true;
	else
		KeyMapping[which].key_active = false;

	KeyMapping[which].key1 = key1;
	KeyMapping[which].key2 = key2;
}
#if 0
void CONTROL_MapButton
        (
        int32_t whichfunction,
        int32_t whichbutton,
        boolean clicked_or_doubleclicked
        )
{
    if(clicked_or_doubleclicked)
	return; // TODO

    if(whichbutton < 0 || whichbutton >= MAXMOUSEBUTTONS)
	    return;

    MouseMapping[whichbutton] = whichfunction;
}

void CONTROL_MapJoyButton(int32_t whichfunction, int32_t whichbutton, boolean doubleclicked)
{
#if WIN32
    if(whichbutton < 0 || whichbutton >= MAXJOYBUTTONS)
    {
        return;
    }

    if(doubleclicked)
	return; // TODO

    JoyButtonMapping[whichbutton] = whichfunction;
#endif
}

void CONTROL_MapJoyHat(int32_t whichfunction, int32_t whichhat, int32_t whichvalue)
{
    if(whichhat < 0 || whichhat >= MAXJOYHATS)
    {
        return;
    }
#if WIN32
    JoyHatMapping[whichhat][whichvalue] = whichfunction;
#endif
}
#endif

void CONTROL_DefineFlag( int32_t which, boolean toggle )
{
    UNUSED(which);
    UNUSED(toggle);
	// STUBBED("CONTROL_DefineFlag");
}
#if WIN32
boolean CONTROL_FlagActive( int32_t which )
{
	STUBBED("CONTROL_FlagActive");
	return false;
}
#endif
#if WIN32

void CONTROL_ClearAssignments( void )
{
	STUBBED("CONTROL_ClearAssignments");
}
void CONTROL_GetUserInput( UserInput *info )
{
	STUBBED("CONTROL_GetUserInput");
}
#endif

#if MG24
int _joystick_axis(int axis)
{
    int32_t lx, ly, rx, ry;
    getAnalogInput(&lx, &ly, &rx, &ry);
    switch (axis)
    {
        case 0: return lx;
        case 1: return ly;
        case 2: return rx;
        case 3: return ry;
    }
    return 0;
}
#endif
void CONTROL_GetInput( ControlInfo *info )
{
	memset(info, '\0', sizeof (ControlInfo));
#if WIN32

    int32_t sens_X = CONTROL_GetAnalogSensitivity(left_x_axis);
	int32_t sens_Y = CONTROL_GetAnalogSensitivity(left_y_axis);
    int32_t mx = 0, my = 0;


	//info->dx = info->dz = 0;
#if WIN32
    _handle_events(); // get the very last mouse position before reading it.
#endif
    MOUSE_GetDelta(&mx,&my);

	//GetCursorPos(&point);
	//	SDL_GetMouseState(&x, &y);
	//	SDL_WarpMouse(160, 100);
	//mx = (-xx+point.x)<<2;
	//my = (-yy+point.y)<<2;
	//xx=point.x; yy=point.y;

	info->dyaw = (mx * sens_X);

	switch(ControllerType)
	{
		case controltype_keyboardandjoystick:
			{
			}
			break;

		case controltype_joystickandmouse:
			// Not sure what I was thinking here...
			// Commented this out because it totally breaks smooth mouse etc...
			/*
			{
				// Mouse should use pitch instead of forward movement.
				info->dpitch = my * sens*2;
			}
			break;
			*/

		default:
			{
				// If mouse aim is active
				if( myaimmode )
				{
					// FIX_00052: Y axis for the mouse is now twice as sensitive as before
					info->dpitch = (my * sens_Y * 2);
				}
				else
				{
					info->dz = (my * sens_Y * 2);
				}
			}
			break;
	}
#if 0

    // TODO: releasing the mouse button does not honor if a keyboard key with
    // the same function is still pressed. how should it?
    for(i=0; i<MAXMOUSEBUTTONS;++i)
	{
		if( MouseMapping[i] != -1 )
		{
			if(!(lastmousebuttons & (1<<i)) && mouseButtons & (1<<i))
			{
				SETMOUSEBUTTON(MouseMapping[i]); //MouseMapping[i]
				//printf("mouse button: %d\n", i);
			}
			else if(lastmousebuttons & (1<<i) && !(mouseButtons & (1<<i)))
			{
				RESMOUSEBUTTON(MouseMapping[i]);//MouseMapping[i]
			}
		}
    }
    lastmousebuttons = mouseButtons;

	// FIX_00019: DigitalAxis Handling now supported. (cool for medkit use)
	// update digital axis
	RESMOUSEDIGITALAXIS(MouseDigitalAxeMapping[0][0]);
	RESMOUSEDIGITALAXIS(MouseDigitalAxeMapping[0][1]);
	RESMOUSEDIGITALAXIS(MouseDigitalAxeMapping[1][0]);
	RESMOUSEDIGITALAXIS(MouseDigitalAxeMapping[1][1]);

	if (mx < 0)
	{
		SETMOUSEDIGITALAXIS(MouseDigitalAxeMapping[0][0]);
	}
	else if (mx > 0)
	{
		SETMOUSEDIGITALAXIS(MouseDigitalAxeMapping[0][1]);
	}

	if (my < 0)
	{
		SETMOUSEDIGITALAXIS(MouseDigitalAxeMapping[1][0]);
	}
	else if (my > 0)
	{
		SETMOUSEDIGITALAXIS(MouseDigitalAxeMapping[1][1]);
	}

    // update stick state.

    if ((CONTROL_JoystickEnabled) && (_joystick_update()))
    {

		// Check the hats
		JOYSTICK_UpdateHats();

		// TODO: make this NOT use the BUTTON() system for storing the hat input. (requires much game code changing)
		for(i=0; i<MAXJOYHATS; i++)
		{

			for(j=0; j<8; j++)
			{
				if(!(lastjoyHats[i] & (1<<j)) && (joyHats[i] & (1<<j)))
				{
					SETHATBUTTON(JoyHatMapping[i][j]);
				}
				else if((lastjoyHats[i] & (1<<j)) && !(joyHats[i] & (1<<j)))
				{
					RESHATBUTTON(JoyHatMapping[i][j]);
				}
			}

			lastjoyHats[i] = joyHats[i];
		}

        for(int i = 0;  i < MAXJOYAXES; i++)
        {
            printf("JoyAxisMapping[%i] = %i\r\n", i, JoyAxisMapping[i]);
            switch(JoyAxisMapping[i])
            {
                case analog_turning:
                    {
                     info->dyaw +=  (int32_t)((float)CONTROL_FilterDeadzone
                                    (
                                        _joystick_axis(i),
                                        JoyAnalogDeadzone[i]
                                    )
                                        * JoyAnalogScale[i]
                                    );
                    }
            	    break;
                case analog_strafing:
                    {
                        info->dx += (int32_t)((float)CONTROL_FilterDeadzone
                                    (
                                        _joystick_axis(i),
                                        JoyAnalogDeadzone[i]
                                    )
                                        * JoyAnalogScale[i]
                                    );
                                    //printf("Joy %d = %d\n", i, info->dx);
                    }
            	    break;
                case analog_lookingupanddown:
                        info->dpitch += (int32_t)((float)CONTROL_FilterDeadzone
                                    (
                                        _joystick_axis(i),
                                        JoyAnalogDeadzone[i]
                                    )
                                        * JoyAnalogScale[i]
                                    );
            	    break;
#if 0
                case analog_elevation: //STUB
            	    break;
                case analog_rolling: //STUB
            	    break;
#endif
                case analog_moving:
                    {
                        info->dz += (int32_t)((float)CONTROL_FilterDeadzone
                                    (
                                        _joystick_axis(i),
                                        JoyAnalogDeadzone[i]
                                    )
                                        * JoyAnalogScale[i]
                                    );
                    }
            	    break;
                default:
                    break;
            }
        }
        // !!! FIXME: Do this.
        //SETBUTTON based on _joystick_button().
        for(i=0; i<MAXJOYBUTTONS;++i)
        {
            if(_joystick_button(i))
            {
                SETJOYBUTTON(JoyButtonMapping[i]);
            }
            else
            {
                RESJOYBUTTON(JoyButtonMapping[i]);
            }
        }
    }
#endif
#else

    for(int i = 0;  i < MAXJOYAXES; i++)
    {
        switch(JoyAxisMapping[i])
        {
            case analog_turning:
                {
                 info->dyaw +=  _joystick_axis(i) * CONTROL_GetAnalogSensitivity(i) / 2;
                }
                break;
            case analog_strafing:
                {
                    info->dx += _joystick_axis(i) * CONTROL_GetAnalogSensitivity(i);
                                //printf("Joy %d = %d\n", i, info->dx);
                }
                break;
            case analog_lookingupanddown:
                    info->dpitch += _joystick_axis(i) * CONTROL_GetAnalogSensitivity(i);
                break;
            case analog_moving:
                {
                    info->dz += _joystick_axis(i) * CONTROL_GetAnalogSensitivity(i);
                }
                break;
            default:
                break;
        }
    }
#endif
//    printf("Info: yaw, %d, pitch %d, dx %d, dz %d\r\n", info->dyaw,  info->dpitch, info->dx, info->dz);
}

void CONTROL_ClearAction( int32_t whichbutton )
{
	//RESBUTTON(whichbutton);
	set_KB_KeyDown(KeyMapping[whichbutton].key1, 0);
	set_KB_KeyDown(KeyMapping[whichbutton].key2, 0);
#if 0
	RESJOYBUTTON(whichbutton);
	RESHATBUTTON(whichbutton);

	RESMOUSEDIGITALAXIS(whichbutton);
#endif
}
#if WIN32
void CONTROL_ClearUserInput( UserInput *info )
{
	STUBBED("CONTROL_ClearUserInput");
}
void CONTROL_WaitRelease( void )
{
	STUBBED("CONTROL_WaitRelease");
}

void CONTROL_Ack( void )
{
	STUBBED("CONTROL_Ack");
}

void CONTROL_CenterJoystick
   (
   void ( *CenterCenter )( void ),
   void ( *UpperLeft )( void ),
   void ( *LowerRight )( void ),
   void ( *CenterThrottle )( void ),
   void ( *CenterRudder )( void )
   )
{

	STUBBED("CONTROL_CenterJoystick");
}
#endif

int32_t axisSensitivities[max_axis];
int32_t CONTROL_GetAnalogSensitivity( axis_type_t axis )
{
    return axisSensitivities[axis];
}

void CONTROL_SetAnalogSensitivity( axis_type_t axis, int32_t newsensitivity)
{
	axisSensitivities[axis] = newsensitivity;
}

void CONTROL_Startup
   (
   controltype which,
   int32_t ( *TimeFunction )( void ),
   int32_t ticspersecond
   )
{
    UNUSED(which);
    UNUSED(TimeFunction);
    UNUSED(ticspersecond);
	// Init the joystick
#if WIN32
    int i;

	_joystick_init();
	for(i=0; i < MAXJOYHATS; i++)
    {
		joyHats[i] = 0;
		lastjoyHats[i] = 0;
	}
#endif

   CONTROL_MouseButtonState1 = 0;
   CONTROL_MouseButtonState2 = 0;
   CONTROL_MouseDigitalAxisState1 = 0;
   CONTROL_MouseDigitalAxisState2 = 0;
   CONTROL_JoyButtonState1 = 0;
   CONTROL_JoyButtonState2 = 0;
   CONTROL_JoyHatState1 = 0;
   CONTROL_JoyHatState2 = 0;
}

void CONTROL_Shutdown( void )
{
#if WIN32
    _joystick_deinit();
#endif
}


void CONTROL_MapAnalogAxis
   (
   int32_t whichaxis,
   int32_t whichanalog
   )
{
	//STUBBED("CONTROL_MapAnalogAxis");
    if(whichaxis < MAXJOYAXES)
    {
        JoyAxisMapping[whichaxis] = whichanalog;
    }
}

// FIX_00019: DigitalAxis Handling now supported. (cool for medkit use)
#if 0
void CONTROL_MapDigitalAxis
   (
   int32_t whichaxis,
   int32_t whichfunction,
   int32_t direction
   )
{
    if(whichaxis < 0 || whichaxis >= MAXMOUSEAXES || direction < 0 || direction >= 2)
	    return;

	MouseDigitalAxeMapping[whichaxis][direction] = whichfunction;

}
#endif
#if 0
void CONTROL_SetAnalogAxisScale
   (
   int32_t whichaxis,
   float axisscale
   )
{
#if WIN32
    if(whichaxis < MAXJOYAXES)
    {
        // Set it... make sure we don't let them set it to 0.. div by 0 is bad.
        JoyAnalogScale[whichaxis] = (axisscale == 0) ? 1.0f : axisscale;
    }
#endif
}

void CONTROL_SetAnalogAxisDeadzone
   (
   int32_t whichaxis,
   int32_t axisdeadzone
   )
{
#if WIN32
    if(whichaxis < MAXJOYAXES)
    {
        // Set it...
        JoyAnalogDeadzone[whichaxis] = axisdeadzone;
    }
#endif
}
#endif
int32_t CONTROL_FilterDeadzone
   (
   int32_t axisvalue,
   int32_t axisdeadzone
   )
{
    if((axisvalue < axisdeadzone) && (axisvalue > -axisdeadzone))
    {
        return 0;
    }

    return axisvalue;
}

#if WIN32
int32_t CONTROL_GetFilteredAxisValue(int32_t axis)
{
#if 0

return (int32_t)((float)CONTROL_FilterDeadzone
                                    (
                                        _joystick_axis(axis),
                                        JoyAnalogDeadzone[axis]
                                    )
                                        * JoyAnalogScale[axis]
                                    );
#else
return 0;
#endif
}

void CONTROL_PrintAxes( void )
{
	STUBBED("CONTROL_PrintAxes");
}
#endif

#if 0
boolean MOUSE_Init( void )
{
	memset(MouseMapping,-1,sizeof(MouseMapping));
	memset(MouseDigitalAxeMapping, -1, sizeof(MouseDigitalAxeMapping));
	return true;
}
#endif
#if WIN32
void    MOUSE_Shutdown( void )
{
	STUBBED("MOUSE_Shutdown");
}

void    MOUSE_ShowCursor( void )
{
	STUBBED("MOUSE_ShowCursor");
}

void    MOUSE_HideCursor( void )
{
	STUBBED("MOUSE_HideCursor");
}
#endif
static void updateMouse(void)
{
    // this is in buildengine.
    short x, y;
    getmousevalues(&x, &y, &mouseButtons);

    mouseRelativeX += x;
    mouseRelativeY += y;
    mousePositionX += x;
    mousePositionY += y;
}

int32_t   MOUSE_GetButtons( void )
{
    //updateMouse();
    return ((int32_t) mouseButtons);
}

void    MOUSE_GetPosition( int32_t*x, int32_t*y  )
{
    if (x) *x = mousePositionX;
    if (y) *y = mousePositionY;
}

void    MOUSE_GetDelta( int32_t*x, int32_t*y )
{
    updateMouse();

    if (x) *x = mouseRelativeX;
    if (y) *y = mouseRelativeY;

	mouseRelativeX = 0;
	mouseRelativeY = 0;
}
#if WIN32
void JOYSTICK_UpdateHats()
{
	int i;

	for(i=0; i<MAXJOYHATS; i++)
	{
		//for(j=0; j<8; j++)
		//{
			joyHats[i] = _joystick_hat(i);
		//}
	}
}
#endif
