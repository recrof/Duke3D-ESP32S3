/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */

#ifndef _INCLUDE_A_H_
#define _INCLUDE_A_H_

#include <inttypes.h>

//C++ linker friendly
#ifdef __cplusplus
    extern "C" {
#endif
#include "maincfg.h"
typedef struct
{
    uint8_t  slopemach_ah1;
    uint8_t  slopemach_ah2;
//    uint8_t xprec16;
//    uint8_t yprec16;
    uint8_t r_xsizebits;
    uint8_t r_ysizebits;
    uint8_t precisionBits;
    const uint8_t * textureSetup;
    const uint8_t *globalpalshade; // basically &globalpalwritten[shade], where shade is the top MSB in a 16-bit word.
    int32_t asm1;
    intptr_t asm2, asm3;
    int32_t asm4;
    int32_t bytesperline;
    int32_t dest_minus_bytes_per_line;
    int32_t slopemach_edx;

    float asm2_f;

} drawstruct_t;
extern drawstruct_t draws;
extern uint8_t  *transluc;


extern const uint8_t  *globalpalwritten;
extern int16_t  globalshiftval;
extern int32_t vplce[2], vince[2];
extern const uint8_t* palookupoffse[2];

void sethlinesizes(int32_t,int32_t,const uint8_t *);

void hlineasm4(int32_t,/*int32_t,*/uint32_t,uint32_t,uint8_t*);
void setuprhlineasm4(int32_t,int32_t,int32_t,const uint8_t*,int32_t);
#if DRAW_CODE_CLEANUP
void rhlineasm4(int32_t numPixels, const uint8_t* texture, uint32_t fractX, uint32_t fracY, uint8_t* dest);
#else
void rhlineasm4(int32_t,const uint8_t*,int32_t,uint32_t,uint32_t,uint8_t*);
#endif
void setuprmhlineasm4(int32_t,int32_t,int32_t, const uint8_t*,int32_t);
void rmhlineasm4(int32_t, const uint8_t*,int32_t,int32_t, uint8_t*);


void setBytesPerLine(int32_t);

int32_t prevlineasm1(int32_t,const uint8_t* palette ,int32_t,int32_t,const uint8_t  *,uint8_t  *);
//FCS: This is used to draw wall border vertical lines
static  inline __attribute__((always_inline)) void vlineasm1(int32_t vince, const uint8_t* palookupoffse, int32_t numPixels, int32_t vplce, const uint8_t* texture, uint8_t* dest)
{
#define USEASM 0
#if DO_NOT_UNROLL_VLINEASM1
    uint32_t temp;
    numPixels++;
    while (numPixels)
    {
      temp = ((uint32_t)vplce) >> draws.precisionBits;

      temp = texture[temp];

      *dest = palookupoffse[temp];

      vplce += vince;
      dest += draw_bytesperline;
      numPixels--;
    }
#elif USEASM
    __asm__ volatile
    (
        "ADD %[npix], %[npix], #1\n\t"
        "loop%=:\n\t"
        "LSR r10, %[y], %[precbits]\r\n"
        "LDRB r10, [%[source], r10]\r\n"
        "ADD %[y], %[y], %[deltay]\n\t"
        "LDRB r10, [%[pal], r10]\n\t"
        "SUBS %[npix], %[npix], #1\n\t"
        "STRB r10, [%[dest]]\n\t"
        "ADD %[dest], %[dest], %[bpl]\n\t"
        "BNE loop%=\n\t"
        : [dest] "+&r" (dest),
          [npix] "+&r" (numPixels),  // out/inout
          [y] "+&r" (vplce)
          : [bpl] "r" (draws.bytesperline),
          [deltay] "r" (vince),
          [precbits] "r" (draws.precisionBits),
          [pal] "r" (palookupoffse),  // in
          [source] "r" (texture)
          :  "cc", "r10"// clobbers
    );
#else
    uint32_t temp;
    uint8_t pix;
   numPixels++;
   int r = numPixels / 16;
   int shiftvalue = draws.precisionBits;  // if we use global variable it will be slower.
   int bpl = draws.bytesperline;
#if MG24 && 0
#define LOAD_NEXT_PIX() \
    "LSR.W %[temp], %[vplce], %[shiftvalue]\n\t"    /* shift to get int y. */ \
    "LDRB.W %[pix], [%[texture], %[temp]]\n\t"       /* load pix*/ \
    "ADD.W %[vplce], %[vplce], %[vince]\n\t"       /*increment fixed y */ \
    "LDRB.W %[pix], [%[pal], %[pix]]\n\t"          /* use palette */ \
    "STRB.W %[pix], [%[dest]]\n\t"          /* use palette */ \
    "ADD.W %[dest], %[dest], %[bpl]\n\t"      /* point to next pixel in bugger*/
   __ASM volatile
   (

       "CMP %[r], #0\n\t"
       "BEQ endloop%=\n\t"
       ".align (2)\n\t"
       "loop%=:\n\t"
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       LOAD_NEXT_PIX()
       "SUBS %[r], %[r], #1\n\t"
       "BNE loop%=\n\t"
       "endloop%=:\n\t"
       : // out/inout
      [r] "+&r" (r),
      [dest] "+&r" (dest),
      [vplce] "+&r" (vplce),
      [temp] "+&r" (temp),
      [pix] "+&r" (pix)
       : // in
      [bpl] "r" (bpl),
      [texture] "r" (texture),
      [pal] "r" (palookupoffse),
      [shiftvalue] "r" (shiftvalue),
      [vince] "r" (vince)
       : // clobber
         "cc"
   );
#else
  while (r)
  {
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    r--;
  }
#endif
  r = numPixels % 16;
  switch (r)
  {
    case 15: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 14: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 13: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 12: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 11: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 10: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 9: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 8: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 7: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 6: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 5: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 4: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 3: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 2: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
    /* no break */
    case 1: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; *dest = palookupoffse[pix];
    /* no break */
    case 0:
         break;
  }

#endif
}
// next-hack: this was already quite well clean up
static inline int32_t mvlineasm1(int32_t vince, const uint8_t* palookupoffse, int32_t numPixMinusOne, int32_t vplce, const uint8_t* texture, uint8_t  *dest)
{

 // return vplce + (numPixMinusOne -1 ) * vince;
    uint32_t temp;
#if 0
    int shiftvalue = draws.precisionBits;
    int bpl = draws.bytesperline;
    for(;numPixMinusOne>=0; numPixMinusOne--)
    {
    temp = ((uint32_t)vplce) >> shiftvalue;
      temp = texture[temp];

      if (temp != 255)
    {
        *dest = palookupoffse[temp];
    }
      vplce += vince;
      dest += bpl;
    }
#else
    uint8_t pix;
   int numPixels =  numPixMinusOne + 1;
   int r = numPixels / 16;
   int shiftvalue = draws.precisionBits;
   int bpl = draws.bytesperline;
   while (r)
   {
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     r--;
   }
   r = numPixels % 16;
   switch (r)
   {
     case 15: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 14: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 13: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 12: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 11: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 10: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 9: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 8: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 7: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 6: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 5: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 4: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 3: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 2: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 )  *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 1: temp = ((uint32_t)vplce) >> shiftvalue; pix = texture[temp]; if (pix != 255 ) *dest = palookupoffse[pix]; vplce += vince; dest += bpl;
     /* no break */
     case 0:
         break;
   }
#endif
    return vplce;
}

int32_t tvlineasm1(int32_t,const uint8_t  *,int32_t,int32_t,const uint8_t  *,uint8_t  * dest);
void setuptvlineasm2(int32_t, const uint8_t*, const uint8_t*);
void tvlineasm2(uint32_t,uint32_t,uintptr_t,uintptr_t,uint32_t,uintptr_t);
int32_t mvlineasm1(int32_t,const uint8_t*,int32_t,int32_t,const uint8_t* texture,uint8_t* dest);
void setupvlineasm(int32_t);
void vlineasm4(int32_t,uint8_t*);
void setupmvlineasm(int32_t);
void mvlineasm4(int32_t,uint8_t*);
void tsetupspritevline(const uint8_t *,int32_t,int32_t,int32_t,int32_t);
void DrawSpriteTranslucentVerticalLine(int32_t,int32_t,uint32_t,const uint8_t* ,uint8_t*);
void mhline(const uint8_t  *,int32_t,int32_t,int32_t,uint8_t*);
void mhlineskipmodify(uint32_t,int32_t,int32_t,uint8_t*);
void msethlineshift(int32_t,int32_t);
void thline(const uint8_t*,int32_t,int32_t,int32_t,uint8_t *);
void tsethlineshift(int32_t,int32_t);
void setupslopevlin(int32_t, const uint8_t *,int32_t);
void slopevlin(intptr_t,uint32_t,intptr_t*,uint32_t,int32_t,int32_t,int32_t);
void hlineasmQuad(int32_t numPixels, uint32_t fixed_y, uint32_t fixed_x, uint8_t *dest);

#define TRANS_NORMAL  0
#define TRANS_REVERSE 1
void settrans(int32_t type);





//FCS: In order to see how the engine renders different part of the screen you can set the following macros
//VISUALIZE RENDERER

#define RENDER_LIMIT_PIXELS 0 // tanguyf: pixelsAllowed causes too many subtle rendering bugs to be left on by default
#if RENDER_LIMIT_PIXELS
extern int64_t pixelsAllowed;
#endif

#define RENDER_DRAW_WALL_BORDERS 1
#define RENDER_DRAW_WALL_INSIDE 1
#define RENDER_DRAW_CEILING_AND_FLOOR 1
#define RENDER_DRAW_TOP_AND_BOTTOM_COLUMN 1
#define RENDER_SLOPPED_CEILING_AND_FLOOR 1

#if RENDER_DRAW_WALL_BORDERS && RENDER_DRAW_WALL_INSIDE && RENDER_DRAW_CEILING_AND_FLOOR  && RENDER_DRAW_TOP_AND_BOTTOM_COLUMN && RENDER_SLOPPED_CEILING_AND_FLOOR
   #define CLEAR_FRAMEBUFFER 0
#else
   #define CLEAR_FRAMEBUFFER 1
#endif
//END VISUALIZE RENDERER


//C++
#ifdef __cplusplus
    }
#endif

#endif
