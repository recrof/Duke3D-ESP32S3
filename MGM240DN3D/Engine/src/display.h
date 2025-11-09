/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */

#ifndef _INCLUDE_DISPLAY_H_
#define _INCLUDE_DISPLAY_H_

#include "platform.h"

/* set these in your _platform_init() implementation. */
extern int32_t _argc;
extern char  **_argv;

/* !!! gads, look at all the namespace polution... */
extern int32_t xres, yres, bytesperline, imageSize, maxpages;
extern uint8_t  *screen, vesachecked;
extern int32_t buffermode, origbuffermode, linearmode;
extern uint8_t  permanentupdate, vgacompatible;
extern uint8_t  moustat;
#if !USE_STATIC_LOOKUP_TABLES
extern int32_t *horizlookup, *horizlookup2;
#else
extern const int32_t horizlookup[4 * SCREENHEIGHT], horizlookup2[4 * SCREENHEIGHT];
#endif
//extern int32_t horizycent;
extern int32_t oxdimen, oviewingrange, oxyaspect;
extern int32_t curbrightness;
extern int32_t qsetmode;
extern int32_t  pageoffset, ydim16;
extern uint8_t* frameplace;
extern uint8_t* frameoffset;
#if LOAD_TABLES
extern uint8_t  textfont[1024], smalltextfont[1024];
#else
extern const uint8_t  textfont[1024], smalltextfont[1024];
#endif
extern const uint8_t  pow2char[8];
#if USE_STEREO_VIDEO
extern int32_t stereomode, visualpage, activepage, whiteband, blackband;
#endif
extern int32_t searchx, searchy;
extern int32_t /*wx1, wy1, wx2, wy2, */ydimen;
extern int32_t /* xdimen, xdimenrecip, */halfxdimen/*, xdimenscale, xdimscale*/;

// defined in the game project:
extern uint8_t FullScreen;  // defined in Game\src\config.c

/* these need to be implemented by YOUR driver. */
void _platform_init(int argc, char  **argv, const char  *title, const char  *iconName);
void _idle(void);
void _handle_events(void);
void *_getVideoBase(void);
void initkeys(void);
void uninitkeys(void);
void _nextpage(int rowsToUpdate);
void _uninitengine(void);
void _joystick_init(void);
void _joystick_deinit(void);
int _joystick_update(void);
int _joystick_axis(int axis);
int _joystick_hat(int hat);
int _joystick_button(int button);
#if USE_MANY_MODES
void getvalidvesamodes(void);
#endif
void VBE_getPalette(int32_t start, int32_t num, uint8_t  *dapal);
void VBE_setPalette(uint8_t  *palettebuffer);
void VBE_presentPalette();
void setFullPaletteChanged(void);
int isFullScreenUpdate(void);
int isSbPaletteRefreshNeeded(void);
void postDisplaySetup(void);

void setvmode(int mode);
uint8_t  readpixel(uint8_t  * location);
void drawpixel(uint8_t  * location, uint8_t pixel);


void drawpixel16(int32_t offset);
void fillscreen16 (int32_t input1, int32_t input2, int32_t input3);


void clear2dscreen(void);
void _updateScreenRect(int32_t x, int32_t y, int32_t w, int32_t h);

/* mouse/keystuff stuff. Also implemented in sdl_driver.c ... */
int setupmouse(void);
void readmousexy(short *x, short *y);
void readmousebstatus(short *bstatus);
void keyhandler(void);
uint8_t  _readlastkeyhit(void);

/* timer krap. */
int inittimer(int);
void uninittimer(void);

/* this is implemented in the game, and your driver calls it. */
void timerhandler(void);

/* resolution inits. sdl_driver.c ... */
int32_t _setgamemode(int32_t daxdim, int32_t daydim);
void sampletimer(void);
//
uint32_t getticks();
void* get_framebuffer(void);
void drawline16(int32_t XStart, int32_t YStart, int32_t XEnd, int32_t YEnd, uint8_t  Color);
void setcolor16(uint8_t color);
//
void waitForUpdate(int line);
#endif
