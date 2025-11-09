/*
 * An SDL replacement for BUILD's VESA code.
 *
 *  Written by Ryan C. Gordon. (icculus@clutteredmind.org)
 *
 * Please do NOT harrass Ken Silverman about any code modifications
 *  (including this file) to BUILD.
 */

/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */
#if WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "graphics.h"

#define BUILD_NOMOUSEGRAB    "BUILD_NOMOUSEGRAB"
#define BUILD_WINDOWED       "BUILD_WINDOWED"
#define BUILD_SDLDEBUG       "BUILD_SDLDEBUG"
#define BUILD_RENDERER       "BUILD_RENDERER"
#define BUILD_GLLIBRARY      "BUILD_GLLIBRARY"
#define BUILD_USERSCREENRES  "BUILD_USERSCREENRES"
#define BUILD_MAXSCREENRES   "BUILD_MAXSCREENRES"
#define BUILD_HALLOFMIRRORS  "BUILD_HALLOFMIRRORS"
#define BUILD_GLDUMP         "BUILD_GLDUMP"
#define BUILD_SDLJOYSTICK    "BUILD_SDLJOYSTICK"

#include "build.h"
#include "display.h"
#include "fixedPoint_math.h"
#include "engine.h"
#if WIN32
#include "sdl_util.h"
#endif
#include "icon.h"
#include "draw.h"
#include "cache.h"


/*
 * !!! remove the surface_end checks, for speed's sake. They are a
 * !!!  needed safety right now. --ryan.
 */


#define DEFAULT_MAXRESWIDTH  MAXXDIM
#define DEFAULT_MAXRESHEIGHT MAXYDIM


#define UNLOCK_SURFACE_AND_RETURN  if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface); return;


	/* !!! move these elsewhere? */
static uint8_t m_rowsToUpdate = 0;
static uint8_t m_sbpaletteNeedsRefresh = 0; // 1 if status bar needs refresh

int32_t xres, yres, bytesperline, imageSize, maxpages;
uint8_t* frameplace;

//The frambuffer address
uint8_t* frameoffset;
uint8_t  *screen, vesachecked;
int32_t buffermode, origbuffermode, linearmode;
uint8_t  permanentupdate = 0, vgacompatible;
#if WIN32

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static SDL_Surface* surface = NULL;
static SDL_Surface* surface_rgba = NULL;
#endif
static int32_t mouse_relative_x = 0;
static int32_t mouse_relative_y = 0;
static short mouse_buttons = 0;
#if WIN32
static unsigned int lastkey = 0;
/* so we can make use of setcolor16()... - DDOI */
static uint8_t  drawpixel_color=0;

#endif
#if WIN32
static uint32_t scancodes[SDL_NUM_SCANCODES];
#endif
static int32_t last_render_ticks = 0;
int32_t total_render_time = 1;
int32_t total_rendered_frames = 0;

//static char *titleName = NULL;

#define print_tf_state(str, val) printf("%s: {%s}\n", str, (val) ? "true" : "false" )

void* get_framebuffer(void)
{
#if WIN32
    if (surface)
    	return surface->pixels;
    else
        return 0;
#else
	return displayData.displayFrameBuffer[0];
#endif
}

/*
 * !!! This is almost an entire copy of the original setgamemode().
 * !!!  Figure out what is needed for just 2D mode, and separate that
 * !!!  out. Then, place the original setgamemode() back into engine.c,
 * !!!  and remove our simple implementation (and this function.)
 * !!!  Just be sure to keep the non-DOS things, like the window's
 * !!!  titlebar caption.   --ryan.
 */
#if WIN32
static uint8_t  screenalloctype = 255;
#endif
static void init_new_res_vars()
{
#ifdef WIN32

	setupmouse();

	xdim = xres = surface->w;
	ydim = yres = surface->h;

	printf("init_new_res_vars %d %d\n", xdim, ydim);

	numpages = 1; // we always expose the same surface to the drawing engine.
	bytesperline = surface->w;
	vesachecked = 1;
	vgacompatible = 1;
	linearmode = 1;
	qsetmode = surface->h;
#if USE_STEREO_VIDEO
	activepage = visualpage = 0;
#endif

	frameoffset = frameplace = (uint8_t*)surface->pixels;

	if (screen != NULL)
	{
		if (screenalloctype == 0) free((void*)screen);
#if USE_CACHE
		if (screenalloctype == 1) suckcache((int32_t*)screen);
#endif
		screen = NULL;
	} /* if */

#if 0
    vidoption = 1;
	switch (vidoption)
	{
	case 1:i = xdim * ydim; break;
	case 2: xdim = 320; ydim = 200; i = xdim * ydim; break;
	default: assert(0);
	}
#endif
#if !USE_STATIC_LOOKUP_TABLES
	j = ydim * 4 * sizeof(int32_t);  /* Leave room for horizlookup&horizlookup2 */

	if (horizlookup)
		free(horizlookup);

	if (horizlookup2)
		free(horizlookup2);

	horizlookup = (int32_t*)malloc(j);
	horizlookup2 = (int32_t*)malloc(j);
#endif

	//Build lookup table (X screespace -> frambuffer offset.
#if YLOOKUP_ARRAY
	int i;
	int j;
	j = 0;

	for (i = 0; i <= ydim; i++)
	{
		ylookup[i] = j;
		j += bytesperline;
	}
#else
    setYlookup(bytesperline);
#endif
	setHorizycent( ((ydim * 4) >> 1) );

	/* Force drawrooms to call dosetaspect & recalculate stuff */
	oxyaspect = oxdimen = oviewingrange = -1;

	//Let the Assembly module how many pixels to skip when drawing a column
	setBytesPerLine(bytesperline);
#else

  xdim = xres = SCREENWIDTH;
  ydim = yres = SCREENHEIGHT;

  printf("init_new_res_vars %d %d\n", xdim, ydim);

  numpages = 1; // we always expose the same surface to the drawing engine.
  bytesperline = SCREENWIDTH;
  linearmode = 1;
  qsetmode = SCREENHEIGHT;


  frameoffset = frameplace = getDisplayFrameBuffer(0);


    vidoption = 1;
    setYlookup(bytesperline);

    setHorizycent((ydim * 4) >> 1);

  /* Force drawrooms to call dosetaspect & recalculate stuff */
  oxyaspect = oxdimen = oviewingrange = -1;

  //Let the Assembly module how many pixels to skip when drawing a column
  setBytesPerLine(bytesperline);

#endif
}

void postDisplaySetup(void)
{
  setview(0L, 0L, xdim - 1, ydim - 1);

  setbrightness(curbrightness, palette);

  if (searchx < 0) {
    searchx = halfxdimen;
    searchy = (ydimen >> 1);
  }

}

static void go_to_new_vid_mode(int w, int h)
{
#if WIN32

	if (window != NULL)
	{
		SDL_FreeSurface(surface_rgba);
		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		window = NULL;
	}

	SDL_CHECK_SUCCESS( SDL_InitSubSystem(SDL_INIT_VIDEO) );

	window = SDL_CreateWindow(
		"BUILD",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		w, h,
		(FullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) | SDL_WINDOW_ALLOW_HIGHDPI);

	SDL_CHECK_NOT_NULL(window, "create window");

	// don't override higher-res app icon on OS X or Windows
#if !PLATFORM_MACOSX && !WIN32
	SDL_Surface* image = SDL_LoadBMP_RW(SDL_RWFromMem(iconBMP, sizeof(iconBMP)), 1);
	Uint32 colorkey = 0; // index in this image to be transparent
	SDL_SetColorKey(image, SDL_TRUE, colorkey);
	SDL_SetWindowIcon(window, image);
	SDL_FreeSurface(image);
#endif
#if USE_MANY_MODES
	getvalidvesamodes();
#endif
	SDL_ClearError();

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_CHECK_NOT_NULL(renderer, "create renderer");

	SDL_RendererInfo rendererInfo;
	SDL_CHECK_SUCCESS(SDL_GetRendererInfo(renderer, &rendererInfo));
	printf("SDL Renderer: '%s'.\n", rendererInfo.name);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

	SDL_CHECK_SUCCESS(SDL_RenderSetLogicalSize(renderer, w, h));

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	SDL_CHECK_NOT_NULL(texture, "create texture");

	surface = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);
	SDL_CHECK_NOT_NULL(surface, "create palettized surface");

	const Uint32 rmask = 0x00ff0000;
	const Uint32 gmask = 0x0000ff00;
	const Uint32 bmask = 0x000000ff;
	const Uint32 amask = 0xff000000;

	surface_rgba = SDL_CreateRGBSurface(0, w, h, 32, rmask, gmask, bmask, amask);
	SDL_CHECK_NOT_NULL(surface_rgba, "create RGBA surface");
#else
	UNUSED(w);
    UNUSED(h);
#endif
    init_new_res_vars();

}

static void present_framebuffer(int rowsToUpdate)
{
#if WIN32
    SDL_Rect    srcRect = {.x = 0, .y = 0, .w = SCREEN_WIDTH, .h = rowsToUpdate};
	SDL_CHECK_SUCCESS(SDL_BlitSurface(surface, &srcRect, surface_rgba, &srcRect));
	SDL_CHECK_SUCCESS(SDL_UpdateTexture(texture, NULL, surface_rgba->pixels, surface_rgba->pitch));
	SDL_CHECK_SUCCESS(SDL_RenderClear(renderer));
	SDL_CHECK_SUCCESS(SDL_RenderCopy(renderer, texture, NULL, NULL));
	SDL_RenderPresent(renderer);
#else
	nextpage(rowsToUpdate);
	#endif

}
#ifdef WIN32

static inline int sdl_mouse_button_filter(SDL_MouseButtonEvent const *event)
{
		/*
		 * What bits BUILD expects:
		 *  0 left button pressed if 1
		 *  1 right button pressed if 1
		 *  2 middle button pressed if 1
		 *
		 *   (That is, this is what Int 33h (AX=0x05) returns...)
		 *
		 *  additionally bits 3&4 are set for the mouse wheel
		 */
	Uint8 button = event->button;
	if (button >= sizeof (mouse_buttons) * 8)
		return(0);

	if (button == SDL_BUTTON_RIGHT)
		button = SDL_BUTTON_MIDDLE;
	else if (button == SDL_BUTTON_MIDDLE)
		button = SDL_BUTTON_RIGHT;

	if (((const SDL_MouseButtonEvent*)event)->state)
		mouse_buttons |= 1<<(button-1);
	else if (button != 4 && button != 5)
		mouse_buttons ^= 1<<(button-1);
#if 0
	Uint8 bmask = SDL_GetMouseState(NULL, NULL);
	mouse_buttons = 0;
	if (bmask & SDL_BUTTON_LMASK) mouse_buttons |= 1;
	if (bmask & SDL_BUTTON_RMASK) mouse_buttons |= 2;
	if (bmask & SDL_BUTTON_MMASK) mouse_buttons |= 4;
#endif

	return(0);
} /* sdl_mouse_up_filter */


static int sdl_mouse_motion_filter(SDL_Event const *event)
{
	if (surface == NULL)
		return(0);

	if (event->type == SDL_JOYBALLMOTION)
	{
		mouse_relative_x = event->jball.xrel/100;
		mouse_relative_y = event->jball.yrel/100;
	}
	else
	{
		if (SDL_GetRelativeMouseMode() == SDL_TRUE)
		{
			mouse_relative_x += event->motion.xrel;
			mouse_relative_y += event->motion.yrel;
			//printf("sdl_mouse_motion_filter: mrx=%d, mry=%d, mx=%d, my=%d\n",
			//	mouse_relative_x, mouse_relative_y, event->motion.xrel, event->motion.yrel);

			// mouse_relative_* is already reset in
			// readmousexy(). It must not be
			// reset here because calling this function does not mean
			// we always handle the mouse.
			// FIX_00001: Mouse speed is uneven and slower in windowed mode vs fullscreen mode.
		}
		else
			mouse_relative_x = mouse_relative_y = 0;
	}

	return(0);
} /* sdl_mouse_motion_filter */


	/*
	 * The windib driver can't alert us to the keypad enter key, which
	 *  Ken's code depends on heavily. It sends it as the same key as the
	 *  regular return key. These users will have to hit SHIFT-ENTER,
	 *  which we check for explicitly, and give the engine a keypad enter
	 *  enter event.
	 */
static inline int handle_keypad_enter_hack(const SDL_Event *event)
{
	static int kp_enter_hack = 0;
	int retval = 0;

	if (event->key.keysym.sym == SDLK_RETURN)
	{
		if (event->key.state == SDL_PRESSED)
		{
			if (event->key.keysym.mod & KMOD_SHIFT)
			{
				kp_enter_hack = 1;
				lastkey = SDL_SCANCODE_KP_ENTER;
				retval = 1;
			} /* if */
		} /* if */

		else  /* key released */
		{
			if (kp_enter_hack)
			{
				kp_enter_hack = 0;
				lastkey = SDL_SCANCODE_KP_ENTER;
				retval = 1;
			} /* if */
		} /* if */
	} /* if */

	return(retval);
} /* handle_keypad_enter_hack */
#endif
void fullscreen_toggle_and_change_driver(void)
{
#ifdef WIN32

//  FIX_00002: New Toggle Windowed/FullScreen system now simpler and will
//  dynamically change for Windib or Directx driver. Windowed/Fullscreen
//  toggle also made available from menu.
//  Replace attempt_fullscreen_toggle(SDL_Surface **surface, Uint32 *flags)

	int32_t x,y;
	x = surface->w;
	y = surface->h;

	FullScreen =!FullScreen;
	_platform_init(0, NULL, "Duke Nukem 3D", "Duke3D");
	_setgamemode(x,y);
	//vscrn();
#endif
	return;
}
#ifdef WIN32

static int sdl_key_filter(const SDL_Event *event)
{
#ifdef WIN32

	int extended;

	if ( (event->key.keysym.sym == SDLK_m) &&
		 (event->key.state == SDL_PRESSED) &&
		 (event->key.keysym.mod & KMOD_CTRL) )
	{
		// FIX_00005: Mouse pointer can be toggled on/off (see mouse menu or use CTRL-M)
		// This is usefull to move the duke window when playing in window mode.

		if (SDL_GetRelativeMouseMode() == SDL_TRUE)
		{
			SDL_CHECK_SUCCESS( SDL_SetRelativeMouseMode(SDL_FALSE) );
		}
		else
		{
			SDL_CHECK_SUCCESS( SDL_SetRelativeMouseMode(SDL_TRUE) );
		}

		return(0);
	}

	if (((event->key.keysym.sym == SDLK_RETURN) ||
		(event->key.keysym.sym == SDLK_KP_ENTER)) &&
		(event->key.state == SDL_PRESSED) &&
		(event->key.keysym.mod & KMOD_ALT))
	{
		fullscreen_toggle_and_change_driver();

		// hack to discard the ALT key...
		lastkey = scancodes[SDL_SCANCODE_RALT] >> 8; // extended
		keyhandler();
		lastkey = (scancodes[SDL_SCANCODE_RALT] & 0xff) + 0x80; // Simulating Key up
		keyhandler();
		lastkey = (scancodes[SDL_SCANCODE_LALT] & 0xff) + 0x80; // Simulating Key up (not extended)
		keyhandler();
		SDL_SetModState(KMOD_NONE); // SDL doesnt see we are releasing the ALT-ENTER keys

		return(0);
	}

	if (!handle_keypad_enter_hack(event))
		lastkey = scancodes[event->key.keysym.scancode];

//	printf("key.keysym.sym=%d\n", event->key.keysym.sym);

	if (lastkey == 0x0000)   /* No DOS equivalent defined. */
		return(0);

	extended = ((lastkey & 0xFF00) >> 8);
	if (extended != 0)
	{
		lastkey = extended;
		keyhandler();
		lastkey = (scancodes[event->key.keysym.scancode] & 0xFF);
	} /* if */

	if (event->key.state == SDL_RELEASED)
		lastkey += 128;  /* +128 signifies that the key is released in DOS. */

	keyhandler();
	return(0);
} /* sdl_key_filter */


static int root_sdl_event_filter(const SDL_Event *event)
{
	switch (event->type)
	{
		case SDL_KEYUP:
			// FIX_00003: Pause mode is now fully responsive - (Thx to Jonathon Fowler tips)
			if(event->key.keysym.sym == SDLK_PAUSE)
				break;
		case SDL_KEYDOWN:
			return(sdl_key_filter(event));
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			{
				//Do Nothing

				//printf("Joybutton UP/DOWN\n");
				//return(sdl_joystick_button_filter((const SDL_MouseButtonEvent*)event));
				return 0;
			}
		case SDL_JOYBALLMOTION:
		case SDL_MOUSEMOTION:
			return(sdl_mouse_motion_filter(event));
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			return(sdl_mouse_button_filter((const SDL_MouseButtonEvent*)event));
		case SDL_QUIT:
			/* !!! rcg TEMP */
			Error(EXIT_SUCCESS, "Exit through SDL\n");
		default:
			//printf("This event is not handled: %d\n",event->type);
			break;
	} /* switch */
#endif
	return(1);
} /* root_sdl_event_filter */


static void handle_events(void)
{
#ifdef WIN32

	SDL_Event event;

	while(SDL_PollEvent(&event))
		root_sdl_event_filter(&event);
#endif
} /* handle_events */


/* bleh...public version... */
void _handle_events(void)
{
	handle_events();
} /* _handle_events */

#ifdef WIN32
static SDL_Joystick *joystick = NULL;
#endif
void _joystick_init(void)
{
#ifdef WIN32

	const char  *envr = getenv(BUILD_SDLJOYSTICK);
	int favored = 0;
	int numsticks;
	int i;

	if (joystick != NULL)
	{
		printf("Joystick appears to be already initialized.\n");
		printf("...deinitializing for stick redetection...\n");
		_joystick_deinit();
	} /* if */

	if ((envr != NULL) && (strcmp(envr, "none") == 0))
	{
		printf("Skipping joystick detection/initialization at user request\n");
		return;
	} /* if */

	printf("Initializing SDL joystick subsystem...");
	printf(" (export environment variable BUILD_SDLJOYSTICK=none to skip)\n");

	if (SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_NOPARACHUTE) != 0)
	{
		printf("SDL_Init(SDL_INIT_JOYSTICK) failed: [%s].\n", SDL_GetError());
		return;
	} /* if */

	numsticks = SDL_NumJoysticks();
	printf("SDL sees %d joystick%s.\n", numsticks, numsticks == 1 ? "" : "s");
	if (numsticks == 0)
		return;

	for (i = 0; i < numsticks; i++)
	{
		joystick = SDL_JoystickOpen(i);
		if (joystick == NULL)
		{
			printf("Joystick #%d failed to init: %s\n", i, SDL_GetError());
			return;
		}

		const char  *stickname = SDL_JoystickName(joystick);
		if ((envr != NULL) && (strcmp(envr, stickname) == 0))
			favored = i;

		printf("Stick #%d: [%s]\n", i, stickname);

		SDL_JoystickClose(joystick);
	} /* for */

	printf("Using Stick #%d.", favored);
	if ((envr == NULL) && (numsticks > 1))
		printf("Set BUILD_SDLJOYSTICK to one of the above names to change.\n");

	joystick = SDL_JoystickOpen(favored);
	if (joystick == NULL)
	{
		printf("Joystick #%d failed to init: %s\n", favored, SDL_GetError());
		return;
	} /* if */

	printf("Joystick initialized. %d axes, %d buttons, %d hats, %d balls.\n",
			  SDL_JoystickNumAxes(joystick), SDL_JoystickNumButtons(joystick),
			  SDL_JoystickNumHats(joystick), SDL_JoystickNumBalls(joystick));

	SDL_JoystickEventState(SDL_QUERY);
#endif
} /* _joystick_init */


void _joystick_deinit(void)
{
#ifdef WIN32

	if (joystick != NULL)
	{
		printf("Closing joystick device...\n");
		SDL_JoystickClose(joystick);
		printf("Joystick device closed. Deinitializing SDL subsystem...\n");
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		printf("SDL joystick subsystem deinitialized.\n");
		joystick = NULL;
	} /* if */
#endif
} /* _joystick_deinit */


int _joystick_update(void)
{
#ifdef WIN32

	if (joystick == NULL)
		return(0);

	SDL_JoystickUpdate();
#endif
	return(1);
} /* _joystick_update */


int _joystick_axis(int axis)
{
#ifdef WIN32

	if (joystick == NULL)
	{
		return(0);
	}

	return(SDL_JoystickGetAxis(joystick, axis));
#endif
} /* _joystick_axis */

int _joystick_hat(int hat)
{
#ifdef WIN32

	if (joystick == NULL)
	{
		return(-1);
	}

	return(SDL_JoystickGetHat(joystick, hat));
#endif
} /* _joystick_axis */

int _joystick_button(int button)
{
#ifdef WIN32

	if (joystick == NULL)
		return(0);

	return(SDL_JoystickGetButton(joystick, button) != 0);
#endif
} /* _joystick_button */


uint8_t  _readlastkeyhit(void)
{
	return(lastkey);
} /* _readlastkeyhit */



#if (!defined __DATE__)
#define __DATE__ "a long, int32_t time ago"
#endif

static void output_sdl_versions(void)
{
#ifdef WIN32

	SDL_version linked_ver;
	SDL_GetVersion(&linked_ver);

	SDL_version compiled_ver;
	SDL_VERSION(&compiled_ver);

	printf("SDL Display driver for the BUILD engine initializing.\n");
	printf("SDL Compiled %s against SDL version %d.%d.%d ...\n", __DATE__, compiled_ver.major, compiled_ver.minor, compiled_ver.patch);
	printf("SDL Linked against SDL version %d.%d.%d ...\n", linked_ver.major, linked_ver.minor, linked_ver.patch);
#endif
}
#endif
#if CODE_DISABLED
/* lousy -ansi flag.  :) */
static char  *string_dupe(const char  *str)
{
	char  *retval = malloc(strlen(str) + 1);
	if (retval != NULL)
		strcpy(retval, str);
	return(retval);
}
#endif
#if WIN32
int32_t _argc = 1;
char  **_argv = {""};
#endif
void _platform_init(int argc, char  **argv, const char  *title, const char  *iconName)
{
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(title);
    UNUSED(iconName);
	uint32_t timeElapsed;

	// FIX_00061: "ERROR: Two players have the same random ID" too frequent cuz of internet windows times
#ifdef WIN32
	timeElapsed = SDL_GetTicks();
#else
    timeElapsed = I_GetTimeMicrosecs();
#endif
	srand(timeElapsed&0xFFFFFFFF);
#ifdef WIN32

	SDL_CHECK_SUCCESS( SDL_Init(SDL_INIT_VIDEO) );

	if (title == NULL)
		title = "BUILD";

//	titleName = string_dupe(title);
	set_sdl_scancodes(scancodes, sizeof(scancodes) / sizeof(scancodes[0]));

	output_sdl_versions();

	printf("SDL Video Driver: '%s'.\n", SDL_GetCurrentVideoDriver());
#endif
}
#if WIN32

// Capture BMP of the current frame
void screencapture(char  *filename)
{
//  FIX_00006: better naming system for screenshots + message when pic is taken.
//  Use ./screenshots folder. Screenshot code rerwritten. Faster and
//  makes smaller files. Doesn't freeze or lag the game anymore.

	if (SDL_SaveBMP(surface, filename))
	{
		printf("SDL failed to save screenshot '%s' (%s)\n", filename, SDL_GetError());
	}
}
void setvmode(int mode)
{

	if (mode == 0x3)  /* text mode. */
	{
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		return;
	} else
		printf("setvmode(0x%x) is unsupported in SDL driver.\n", mode);
}
#endif
int32_t _setgamemode(int32_t daxdim, int32_t daydim)
{
#if USE_MANY_MODES
	if (daxdim > MAXXDIM || daydim > MAXYDIM)
	{
		printf("Resolution %dx%d is too high. Changed to %dx%d\n", daxdim, daydim, MAXXDIM,MAXYDIM);
		daxdim = MAXXDIM;
		daydim = MAXYDIM;
	}
	getvalidvesamodes();

	validated = 0;
	for(i=0; i<validmodecnt; i++){
		if(validmodexdim[i] == daxdim && validmodeydim[i] == daydim)
			validated = 1;
	}

	if(!validated){
		printf("Resolution %dx%d unsupported. Changed to 640x480\n", daxdim, daydim);
		daxdim = 640;
		daydim = 480;
	}

	go_to_new_vid_mode(daxdim, daydim);
#else
		daxdim = MAXXDIM;
		daydim = MAXYDIM;
	go_to_new_vid_mode(daxdim, daydim);

#endif
	qsetmode = 200;
	last_render_ticks = getticks();

	return(0);
} /* setgamemode */


static int get_dimensions_from_str(const char  *str, int32_t *_w, int32_t *_h)
{
	char  *xptr = NULL;
	char  *ptr = NULL;
	int32_t w = -1;
	int32_t h = -1;

	if (str == NULL)
		return(0);

	xptr = strchr(str, 'x');
	if (xptr == NULL)
		return(0);

	w = strtol(str, &ptr, 10);
	if (ptr != xptr)
		return(0);

	xptr++;
	h = strtol(xptr, &ptr, 10);
	if ( (*xptr == '\0') || (*ptr != '\0') )
		return(0);

	if ((w <= 1) || (h <= 1))
		return(0);

	if (_w != NULL)
		*_w = w;

	if (_h != NULL)
		*_h = h;

	return(1);
} /* get_dimensions_from_str */


static inline void get_max_screen_res(int32_t *max_w, int32_t *max_h)
{
	int32_t w = DEFAULT_MAXRESWIDTH;
	int32_t h = DEFAULT_MAXRESHEIGHT;
	const char  *envr = getenv(BUILD_MAXSCREENRES);

	if (envr != NULL)
	{
		if (!get_dimensions_from_str(envr, &w, &h))
		{
			printf("User's resolution ceiling [%s] is bogus!\n", envr);
			w = DEFAULT_MAXRESWIDTH;
			h = DEFAULT_MAXRESHEIGHT;
		} /* if */
	} /* if */

	if (max_w != NULL)
		*max_w = w;

	if (max_h != NULL)
		*max_h = h;
}

#if USE_MANY_MODES
static void add_vesa_mode(int w, int h)
{
	validmode[validmodecnt] = validmodecnt;
	validmodexdim[validmodecnt] = w;
	validmodeydim[validmodecnt] = h;
	validmodecnt++;
}

/* Let the user specify a specific mode via environment variable. */
static inline void add_user_defined_resolution(void)
{
	int32_t w;
	int32_t h;
	const char  *envr = getenv(BUILD_USERSCREENRES);

	if (envr == NULL)
		return;

	if (get_dimensions_from_str(envr, &w, &h))
		add_vesa_mode(w, h);
	else
		printf("User defined resolution [%s] is bogus!\n", envr);
} /* add_user_defined_resolution */

static void remove_vesa_mode(int index, const char  *reason)
{
	int i;

	assert(index < validmodecnt);
	//printf("Removing resolution #%d, %dx%d [%s].\n",index, validmodexdim[index], validmodeydim[index], reason);

	for (i = index; i < validmodecnt - 1; i++)
	{
		validmode[i] = validmode[i + 1];
		validmodexdim[i] = validmodexdim[i + 1];
		validmodeydim[i] = validmodeydim[i + 1];
	} /* for */

	validmodecnt--;
} /* remove_vesa_mode */


static inline void cull_large_vesa_modes(void)
{
	int32_t max_w;
	int32_t max_h;
	int i;

	get_max_screen_res(&max_w, &max_h);
	printf("Setting resolution ceiling to (%dx%d).\n", max_w, max_h);

	for (i = 0; i < validmodecnt; i++)
	{
		if ((validmodexdim[i] > max_w) || (validmodeydim[i] > max_h))
		{
			remove_vesa_mode(i, "above resolution ceiling");
			i--;  /* list shrinks. */
		} /* if */
	} /* for */
} /* cull_large_vesa_modes */


static inline void cull_duplicate_vesa_modes(void)
{
	int i;
	int j;

	for (i = 0; i < validmodecnt; i++){
		for (j = i + 1; j < validmodecnt; j++){
			if ( (validmodexdim[i] == validmodexdim[j]) &&(validmodeydim[i] == validmodeydim[j]) ){
				remove_vesa_mode(j, "duplicate");
				j--;  /* list shrinks. */
			}
		}
	}
}


#define swap_macro(tmp, x, y) { tmp = x; x = y; y = tmp; }

/* be sure to call cull_duplicate_vesa_modes() before calling this. */
static inline void sort_vesa_modelist(void)
{
	int i;
	int sorted;
	int32_t tmp;

	do
	{
		sorted = 1;
		for (i = 0; i < validmodecnt - 1; i++)
		{
			if ( (validmodexdim[i] >= validmodexdim[i+1]) &&
				 (validmodeydim[i] >= validmodeydim[i+1]) )
			{
				sorted = 0;
				swap_macro(tmp, validmode[i], validmode[i+1]);
				swap_macro(tmp, validmodexdim[i], validmodexdim[i+1]);
				swap_macro(tmp, validmodeydim[i], validmodeydim[i+1]);
			} /* if */
		} /* for */
	} while (!sorted);
} /* sort_vesa_modelist */


static inline void cleanup_vesa_modelist(void)
{
	cull_large_vesa_modes();
	cull_duplicate_vesa_modes();
	sort_vesa_modelist();
} /* cleanup_vesa_modelist */


static inline void output_vesa_modelist(void)
{
	char  buffer[512];
	char  numbuf[20];
	int i;

	buffer[0] = '\0';

	for (i = 0; i < validmodecnt; i++)
	{
		sprintf(numbuf, " (%dx%d)",(int32_t) validmodexdim[i], (int32_t) validmodeydim[i]);

		if ( (strlen(buffer) + strlen(numbuf)) >= (sizeof (buffer) - 1) )
			strcpy(buffer + (sizeof (buffer) - 5), " ...");
		else
			strcat(buffer, numbuf);
	} /* for */

	printf("Final sorted modelist:%s\n", buffer);
}


void getvalidvesamodes(void)
{
	static int already_checked = 0;
	int i;
	int stdres[][2] = {
		{320, 240},
		{640, 480},
		{800, 600},
		{1024, 768},
		{1280, 960},
		{1440, 1080},
		{1600, 1200},
		{1920, 1440},
		{2560, 1920},
		{2880, 2160}
	};

	if (already_checked)
		return;

	already_checked = 1;
	validmodecnt = 0;
	vidoption = 1;  /* !!! tmp */

	// Fill in the standard 4:3 resolutions that the display supports
#ifdef WIN32

	int numModes = SDL_GetNumDisplayModes(0);
	int maxWidth = 0;
	int maxHeight = 0;

	for (i = 0; i < numModes; ++i)
	{
		SDL_DisplayMode mode;
		SDL_CHECK_SUCCESS(SDL_GetDisplayMode(0, i, &mode));

		maxWidth = max(maxWidth, mode.w);
		maxHeight = max(maxHeight, mode.h);
	}

	for (i = 0; i < sizeof(stdres) / sizeof(stdres[0]); i++)
	{
		assert(stdres[i][0] / 4 == stdres[i][1] / 3);

		if (stdres[i][0] <= maxWidth && stdres[i][1] <= maxHeight)
		{
			add_vesa_mode(stdres[i][0], stdres[i][1]);
		}
	}

	/* Now add specific resolutions that the user wants... */
	add_user_defined_resolution();

	/* get rid of dupes and bogus resolutions... */
	cleanup_vesa_modelist();

	/* print it out for debugging purposes... */
	output_vesa_modelist();
#endif
}
#endif
#if CODE_DISABLED
uint8_t lastPalette[768];
void WriteTranslucToFile(void)
{
	uint8_t buffer[65535*4];
	uint8_t tga_header[18];
	uint8_t* transPointer = transluc;
	uint8_t* bufferPointer = buffer;
	int i;
	FILE* file;

	for (i=0; i < 65535; i++) {

		bufferPointer[0] = (lastPalette[(*transPointer)*3+0]) / 63.0 * 255;
		bufferPointer[1] = (lastPalette[(*transPointer)*3+1]) / 63.0 * 255;
		bufferPointer[2] = (lastPalette[(*transPointer)*3+2]) / 63.0 * 255;
		bufferPointer[3] = 255;

		printf("%d,",*transPointer);
		if (i%255 ==0)
			printf("\n");

		transPointer +=1;
		bufferPointer+=4;
	}

	file = fopen("transluc.tga", "w");

	memset(tga_header, 0, 18);
	tga_header[2] = 2;
	tga_header[12] = (256 & 0x00FF);
	tga_header[13] = (256  & 0xFF00) / 256;
	tga_header[14] = (256  & 0x00FF) ;
	tga_header[15] =(256 & 0xFF00) / 256;
	tga_header[16] = 32 ;

	fwrite(&tga_header, 18, sizeof(uint8_t), file);
	fwrite(buffer, 65535, 4, file);
	fclose(file);
}

void WritePaletteToFile(uint8_t* palette,const char* filename,int width, int height)
{
	uint8_t tga_header[18];
	uint8_t* buffer;
	uint8_t* palettePointer = palette;
	uint8_t* bufferPointer ;
	int i;

	FILE* file = fopen(filename, "w");

	memset(tga_header, 0, 18);
	tga_header[2] = 2;
	tga_header[12] = (width & 0x00FF);
	tga_header[13] = (width  & 0xFF00) / 256;
	tga_header[14] = (height  & 0x00FF) ;
	tga_header[15] =(height & 0xFF00) / 256;
	tga_header[16] = 32 ;

	fwrite(&tga_header, 18, sizeof(uint8_t), file);

	bufferPointer = buffer = malloc(width*height*4);

	for (i = 0 ; i < width*height ; i++)
	{
		bufferPointer[0] = palettePointer[0] / 63.0 * 255;
		bufferPointer[1] = palettePointer[1] / 63.0 * 255;
		bufferPointer[2] = palettePointer[2] / 63.0 * 255;
		bufferPointer[3] = 255;

		bufferPointer += 4;
		palettePointer+= 3;
	}

	fwrite(buffer, width*height, 4, file);
	fclose(file);

	free(buffer);
}


void WriteLastPaletteToFile()
{
	WritePaletteToFile(lastPalette,"lastPalette.tga",16,16);
}
#endif
#if WIN32
void VBE_getPalette(int32_t start, int32_t num, uint8_t* palettebuffer)
{

	SDL_Color* sdlp = surface->format->palette->colors + start;
	uint8_t* p = palettebuffer + (start * 4);
	int i;

	for (i = 0; i < num; i++)
	{
		*p++ = (Uint8)((((float)sdlp->b) / 255.0) * 63.0);
		*p++ = (Uint8)((((float)sdlp->g) / 255.0) * 63.0);
		*p++ = (Uint8)((((float)sdlp->r) / 255.0) * 63.0);
		*p++ = sdlp->a;   /* This byte is unused in both SDL and BUILD. */
		sdlp++;
	}
}
#endif
static uint16_t palette565[256];
void VBE_setPalette(uint8_t  *palettebuffer)
/*
 * (From Ken's docs:)
 *   Set (num) palette palette entries starting at (start)
 *   palette entries are in a 4-byte format in this order:
 *       0: Blue (0-63)
 *       1: Green (0-63)
 *       2: Red (0-63)
 *       3: Reserved
 *
 * Naturally, the bytes are in the reverse order that SDL wants them...
 *  More importantly, SDL wants the color elements in a range from 0-255,
 *  so we do a conversion.
 */
{
#if WIN32

	SDL_Color fmt_swap[256];
	SDL_Color *sdlp = fmt_swap;
	uint8_t  *p = palettebuffer;
	int i;
	//static updated=0;

	//if (updated >=1 )
	//    return ;

	//WritePaletteToFile(palettebuffer,"lastPalette.tga",16,16);
	//updated++;


	//CODE EXPLORATION
	//Used only to write the last palette to file.
    //memcpy(lastPalette, palettebuffer, 768);

	for (i = 0; i < 256; i++){
		sdlp->b = (Uint8) ((((float) *p++) / 63.0) * 255.0);
		sdlp->g = (Uint8) ((((float) *p++) / 63.0) * 255.0);
		sdlp->r = (Uint8) ((((float) *p++) / 63.0) * 255.0);
		sdlp->a = *p++;   /* This byte is unused in BUILD, too. */
		sdlp++;
	}

	SDL_CHECK_SUCCESS( SDL_SetPaletteColors(surface->format->palette, fmt_swap, 0, 256) );
#else

	for (int i = 0; i < 256; i++)
	{
	   int b = palettebuffer[4 * i] >> 1;
     int g = palettebuffer[4 * i + 1];
     int r = palettebuffer[4 * i + 2] >> 1;
     uint16_t rgb = (r << 11) | (g << 5) | b;
     palette565[i] = (rgb >> 8) | (rgb << 8);
	}
  // set palette.
	displayData.pPalette = palette565;
#endif

}
void setFullPaletteChanged(void)
{
    m_sbpaletteNeedsRefresh = 1;
}
void setFramePalette(uint8_t *rgbvalues)
{
  for (int i = 0; i < 256; i++)
  {
     int b = rgbvalues[3 * i] >> 1;
     int g = rgbvalues[3 * i + 1];
     int r = rgbvalues[3 * i + 2] >> 1;
     uint16_t rgb = (r << 11) | (g << 5) | b;
     palette565[i] = (rgb >> 8) | (rgb << 8);
  }
  // set palette.
  displayData.pPalette = palette565;

}

void VBE_presentPalette()
{
	// tanguyf: updating the palette is not immediate with a buffered surface, screen needs updating as well.
	// Call this function if nextpage(SCREEN_HEIGHT) is not called. E.g. static intro logo.
	present_framebuffer(SCREEN_HEIGHT);
}

void _uninitengine(void)
{
#ifdef WIN32

   SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif
} /* _uninitengine */

int setupmouse(void)
{
#ifdef WIN32

	SDL_Event event;

	if (surface == NULL)
		return(0);

	SDL_CHECK_SUCCESS( SDL_SetRelativeMouseMode(SDL_TRUE) );

	mouse_relative_x = mouse_relative_y = 0;

	/*
	 * this global usually gets set by BUILD, but it's a one-shot
	 *  deal, and we may not have an SDL surface at that point. --ryan.
	 */
	moustat = 1;

	// FIX_00063: Duke's angle changing or incorrect when using toggle fullscreen/window mode
	while(SDL_PollEvent(&event)); // Empying the various pending events (especially the mouse one)

	//SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
#endif
	return(1);
} /* setupmouse */


void readmousexy(short *x, short *y)
{
	if (x)
		*x = mouse_relative_x << 2;
	if (y)
		*y = mouse_relative_y << 2;

	mouse_relative_x = mouse_relative_y = 0;
} /* readmousexy */


void readmousebstatus(short *bstatus)
{
	if (bstatus)
		*bstatus = mouse_buttons;

	// special wheel treatment: make it like a button click
	if(mouse_buttons&8)
		mouse_buttons ^= 8;
	if(mouse_buttons&16)
		mouse_buttons ^= 16;

} /* readmousebstatus */

#if WIN32

void _updateScreenRect(int32_t x, int32_t y, int32_t w, int32_t h)
{

	SDL_Rect rect = { x, y, w, h };

	SDL_CHECK_SUCCESS(SDL_BlitSurface(surface, &rect, surface_rgba, &rect));
	SDL_CHECK_SUCCESS(SDL_UpdateTexture(texture, &rect, surface_rgba->pixels, surface_rgba->pitch));
	SDL_CHECK_SUCCESS(SDL_RenderCopy(renderer, texture, &rect, &rect));
	SDL_RenderPresent(renderer);
}
#endif

int isSbPaletteRefreshNeeded(void)
{
    return m_sbpaletteNeedsRefresh;
}
int isFullScreenUpdate(void)
{
    return m_rowsToUpdate >= SCREENHEIGHT - STATUS_BAR_HEIGHT;
}
//int counter= 0 ;
//char bmpName[256];
void _nextpage(int rowsToUpdate)
{
    m_rowsToUpdate = rowsToUpdate;
    if (isFullScreenUpdate())
        m_sbpaletteNeedsRefresh = 0;
#if WIN32
    uint32_t ticks;
    _handle_events();

	present_framebuffer(rowsToUpdate);

	//sprintf(bmpName,"%d.bmp",counter++);
	//SDL_SaveBMP(surface,bmpName);

	//if (CLEAR_FRAMEBUFFER)
	//    SDL_FillRect(surface,NULL,0);

	ticks = getticks();
	total_render_time = (ticks - last_render_ticks);
	if (total_render_time > 1000){
		total_rendered_frames = 0;
		total_render_time = 1;
		last_render_ticks = ticks;
	}
	total_rendered_frames++;
#else
	// switch based on next page
	keyhandler();
  startDisplayRefresh(0, rowsToUpdate);
 //

#endif

}
void waitForUpdate(int line)
{
#if MG24
  if (line <= 0 || line >= SCREENHEIGHT)
    waitForDisplayDMA(SCREEN_HEIGHT - 1);
  else
    waitForDisplayDMA(line);
#endif
}
void drawpixel(uint8_t* location, uint8_t pixel)
{
    *location = pixel;
}

#if WIN32
uint8_t  readpixel(uint8_t* offset)
{
	return *offset;
}


/* Fix this up The Right Way (TM) - DDOI */
void setcolor16(uint8_t col)
{
	drawpixel_color = col;
}
void drawpixel16(int32_t offset)
{

	drawpixel((uint8_t*)surface->pixels + offset, drawpixel_color);
} /* drawpixel16 */


void fillscreen16(int32_t offset, int32_t color, int32_t blocksize)
{

	Uint8 *surface_end;
	Uint8 *wanted_end;
	Uint8 *pixels;

	if (SDL_MUSTLOCK(surface))
		SDL_LockSurface(surface);

	pixels = get_framebuffer();

	/* Make this function pageoffset aware - DDOI */
	if (!pageoffset) {
		offset = offset << 3;
		offset += 640*336;
	}

	surface_end = (pixels + (surface->w * surface->h)) - 1;
	wanted_end = (pixels + offset) + blocksize;

	if (offset < 0)
		offset = 0;

	if (wanted_end > surface_end)
		blocksize = surface_end - (pixels + offset);

	memset(pixels + offset, (int) color, blocksize);

	if (SDL_MUSTLOCK(surface))
		SDL_UnlockSurface(surface);

	_nextpage(SCREEN_HEIGHT);

} /* fillscreen16 */


/* Most of this line code is taken from Abrash's "Graphics Programming Blackbook".
Remember, sharing code is A Good Thing. AH */
static inline void DrawHorizontalRun (uint8_t  **ScreenPtr, int XAdvance, int RunLength, uint8_t  Color)
{

	int i;
	uint8_t  *WorkingScreenPtr = *ScreenPtr;

	for (i=0; i<RunLength; i++)
	{
		*WorkingScreenPtr = Color;
		WorkingScreenPtr += XAdvance;
	}
	WorkingScreenPtr += surface->w;
	*ScreenPtr = WorkingScreenPtr;
}

static inline void DrawVerticalRun (uint8_t  **ScreenPtr, int XAdvance, int RunLength, uint8_t  Color)
{

	int i;
	uint8_t  *WorkingScreenPtr = *ScreenPtr;

	for (i=0; i<RunLength; i++)
	{
		*WorkingScreenPtr = Color;
		WorkingScreenPtr += surface->w;
	}
	WorkingScreenPtr += XAdvance;
	*ScreenPtr = WorkingScreenPtr;
}

void drawline16(int32_t XStart, int32_t YStart, int32_t XEnd, int32_t YEnd, uint8_t  Color)
{

  int Temp, AdjUp, AdjDown, ErrorTerm, XAdvance, XDelta, YDelta;
	int WholeStep, InitialPixelCount, FinalPixelCount, i, RunLength;
	uint8_t  *ScreenPtr;
	int32_t dx, dy;

	if (SDL_MUSTLOCK(surface))
		SDL_LockSurface(surface);

	dx = XEnd-XStart;
	dy = YEnd-YStart;

	//Analyse the slope
	if (dx >= 0)
	{
		if ((XStart > 639) || (XEnd < 0)) return;
		if (XStart < 0) { if (dy) YStart += scale(0-XStart,dy,dx); XStart = 0; }
		if (XEnd > 639) { if (dy) YEnd += scale(639-XEnd,dy,dx); XEnd = 639; }
	}
	else
	{
		if ((XEnd > 639) || (XStart < 0)) return;
		if (XEnd < 0) { if (dy) YEnd += scale(0-XEnd,dy,dx); XEnd = 0; }
		if (XStart > 639) { if (dy) YStart += scale(639-XStart,dy,dx); XStart = 639; }
	}
	if (dy >= 0)
	{
		if ((YStart >= ydim16) || (YEnd < 0)) return;
		if (YStart < 0) { if (dx) XStart += scale(0-YStart,dx,dy); YStart = 0; }
		if (YEnd >= ydim16) { if (dx) XEnd += scale(ydim16-1-YEnd,dx,dy); YEnd = ydim16-1; }
	}
	else
	{
		if ((YEnd >= ydim16) || (YStart < 0)) return;
		if (YEnd < 0) { if (dx) XEnd += scale(0-YEnd,dx,dy); YEnd = 0; }
		if (YStart >= ydim16) { if (dx) XStart += scale(ydim16-1-YStart,dx,dy); YStart = ydim16-1; }
	}

	/* Make sure the status bar border draws correctly - DDOI */
	if (!pageoffset) { YStart += 336; YEnd += 336; }

	/* We'll always draw top to bottom */
	if (YStart > YEnd) {
		Temp = YStart;
		YStart = YEnd;
		YEnd = Temp;
		Temp = XStart;
		XStart = XEnd;
		XEnd = Temp;
	}

	/* Point to the bitmap address first pixel to draw */
	ScreenPtr = (uint8_t  *) (get_framebuffer()) + XStart + (surface->w * YStart);

	/* Figure out whether we're going left or right, and how far we're going horizontally */
	if ((XDelta = XEnd - XStart) < 0)
	{
		XAdvance = (-1);
		XDelta = -XDelta;
	} else {
		XAdvance = 1;
	}

	/* Figure out how far we're going vertically */
	YDelta = YEnd - YStart;

	/* Special cases: Horizontal, vertical, and diagonal lines */
	if (XDelta == 0)
	{
		for (i=0; i <= YDelta; i++)
		{
			*ScreenPtr = Color;
			ScreenPtr += surface->w;
		}

		UNLOCK_SURFACE_AND_RETURN;
	}
	if (YDelta == 0)
	{
		for (i=0; i <= XDelta; i++)
		{
			*ScreenPtr = Color;
			ScreenPtr += XAdvance;
		}
		UNLOCK_SURFACE_AND_RETURN;
	}
	if (XDelta == YDelta)
	{
		for (i=0; i <= XDelta; i++)
		{
			*ScreenPtr = Color;
			ScreenPtr += XAdvance + surface->w;
		}
		UNLOCK_SURFACE_AND_RETURN;
	}

	/* Determine whether the line is X or Y major, and handle accordingly */
	if (XDelta >= YDelta) /* X major line */
	{
		WholeStep = XDelta / YDelta;
		AdjUp = (XDelta % YDelta) * 2;
		AdjDown = YDelta * 2;
		ErrorTerm = (XDelta % YDelta) - (YDelta * 2);

		InitialPixelCount = (WholeStep / 2) + 1;
		FinalPixelCount = InitialPixelCount;

		if ((AdjUp == 0) && ((WholeStep & 0x01) == 0)) InitialPixelCount--;
		if ((WholeStep & 0x01) != 0) ErrorTerm += YDelta;

		DrawHorizontalRun(&ScreenPtr, XAdvance, InitialPixelCount, Color);

		for (i=0; i<(YDelta-1); i++)
		{
			RunLength = WholeStep;
			if ((ErrorTerm += AdjUp) > 0)
			{
			RunLength ++;
			ErrorTerm -= AdjDown;
			}

			DrawHorizontalRun(&ScreenPtr, XAdvance, RunLength, Color);
		 }

		 DrawHorizontalRun(&ScreenPtr, XAdvance, FinalPixelCount, Color);

		 UNLOCK_SURFACE_AND_RETURN;
	} else {	/* Y major line */
		WholeStep = YDelta / XDelta;
		AdjUp = (YDelta % XDelta) * 2;
		AdjDown = XDelta * 2;
		ErrorTerm = (YDelta % XDelta) - (XDelta * 2);
		InitialPixelCount = (WholeStep / 2) + 1;
		FinalPixelCount = InitialPixelCount;

		if ((AdjUp == 0) && ((WholeStep & 0x01) == 0)) InitialPixelCount --;
		if ((WholeStep & 0x01) != 0) ErrorTerm += XDelta;

		DrawVerticalRun(&ScreenPtr, XAdvance, InitialPixelCount, Color);

		for (i=0; i<(XDelta-1); i++)
		{
			RunLength = WholeStep;
			if ((ErrorTerm += AdjUp) > 0)
			{
				RunLength ++;
				ErrorTerm -= AdjDown;
			}

			DrawVerticalRun(&ScreenPtr, XAdvance, RunLength, Color);
		}

		DrawVerticalRun(&ScreenPtr, XAdvance, FinalPixelCount, Color);
		UNLOCK_SURFACE_AND_RETURN;
	 }
} /* drawline16 */
#endif

#if WIN32
void clear2dscreen(void)
{

	SDL_Rect rect;

	rect.x = rect.y = 0;
	rect.w = surface->w;

	if (qsetmode == 350)
		rect.h = 350;
	else if (qsetmode == 480)
	{
		if (ydim16 <= 336)
			rect.h = 336;
		else
			rect.h = 480;
	} /* else if */

	SDL_FillRect(surface, &rect, 0);
} /* clear2dscreen */
#endif


void _idle(void)
{
#ifdef WIN32

	if (surface != NULL)
		_handle_events();
	SDL_Delay(1);
#endif
} /* _idle */
#if WIN32

void *_getVideoBase(void)
{

	return((void *) surface->pixels);
	return NULL;
}
#endif


//-------------------------------------------------------------------------------------------------
//  TIMER
//=================================================================================================




// FIX_00007: game speed corrected. The game speed is now as the real
// DOS duke3d. Unloading a full 200 bullet pistol must take 45.1 sec.
// SDL timer was not fast/accurate enough and was slowing down the gameplay,
// so bad


static int64_t timerfreq=0;
static uint32_t timerlastsample=0;
static int timerticspersec=0;
static void (*usertimercallback)(void) = NULL;

//  This timer stuff is all Ken's idea.

//
// installusertimercallback() -- set up a callback function to be called when the timer is fired
//
#if 0
void (*installusertimercallback(void (*callback)(void)))(void)
{
	void (*oldtimercallback)(void);

	oldtimercallback = usertimercallback;
	usertimercallback = callback;

	return oldtimercallback;
}
#endif
/*
 inittimer() -- initialise timer
 FCS: The tickspersecond parameter is a ratio value that helps replicating
	  oldschool DOS tick per seconds.

	  The way the timer work is:
	  float newSystemTickPerSecond = [0,1]
	  tickPerSecond on a DOS system = tickspersecond * newSystemTickPerSecond ;
*/
// FIXME: to do this better.
#define TICKS_PER_SECOND 120
int inittimer(int tickspersecond)
{

	if (timerfreq) return 0;	// already installed

	//printf("Initialising timer, with tickPerSecond=%d\n",tickspersecond);
    timerticspersec = tickspersecond;
	timerfreq = 1000;
#ifdef WIN32
    int64_t t;

	t = SDL_GetTicks();

	timerlastsample = (int32_t)(t*timerticspersec / timerfreq);
#else

	timerlastsample =  TICK_TIMER->CNT;
	#endif
	usertimercallback = NULL;

	return 0;
}

//
// uninittimer() -- shut down timer
//
void uninittimer(void)
{
	if (!timerfreq) return;

	timerfreq=0;
	timerticspersec = 0;
}

//
// sampletimer() -- update totalclock
//


void sampletimer(void)
{
#if WIN32
    int64_t i;
	uint32_t n;

	if (!timerfreq) return;

	i = SDL_GetTicks();
  n = (int32_t)(i*timerticspersec / timerfreq) - timerlastsample;
  if (n>0)
  {
      totalclock += n;
      timerlastsample += n;
  }

  #else
  // get number of timer ticks now
  uint32_t timerNow =  TICK_TIMER->CNT;
  // compute difference between last time
  uint32_t deltatimer = timerNow - timerlastsample;
  // get the number of dn3d ticks elapsed
  float timeElapsed = (float) deltatimer * TICKS_PER_SECOND / TICK_TIMER_FREQUENCY_HZ;
  // we want the integer one
  uint32_t iTimeElaped = timeElapsed;
  // now calculate back timer last sample
  float timerTicks = iTimeElaped * (TICK_TIMER_FREQUENCY_HZ / TICKS_PER_SECOND);
  timerlastsample += (uint32_t) timerTicks;
  totalclock += iTimeElaped;
  #endif
}

/*
   getticks() -- returns the windows ticks count
   FCS: This seems to be only used in the multiplayer code
*/
uint32_t getticks(void)
{
#if WIN32

	int64_t i;
	i = SDL_GetTicks();
	return (uint32_t)(i*(int32_t)(1000)/timerfreq);
#else
	sampletimer();
	return totalclock * (1000.0f / TICKS_PER_SECOND);
#endif
}

//
// gettimerfreq() -- returns the number of ticks per second the timer is configured to generate
//
int gettimerfreq(void)
{
	return timerticspersec;
}
