// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
// This file has been modified from Ken Silverman's original release

/* DDOI - This file is an attempt to reimplement a_nasm.asm in C */
/* FCS: However did that work: This is far from perfect but you have my eternal respect !!! */
#include "maincfg.h"
#include "platform.h"
#include "build.h"
#include "draw.h"
#include <stddef.h>
#pragma GCC optimize ("Ofast")  // we need to compile this code to be as fast as possible.
 drawstruct_t draws;
uint8_t  *transluc = NULL;

static int transrev = 0;
//extern uint8_t *globalpalshade;

#define shrd(a,b,c) (((b)<<(32-(c))) | ((a)>>(c)))
#define shld(a,b,c) (((b)>>(32-(c))) | ((a)<<(c)))

/* ---------------  WALLS RENDERING METHOD (USED TO BE HIGHLY OPTIMIZED ASSEMBLY) ----------------------------*/
//extern int32_t asm1;
//extern intptr_t asm2;
//extern uint8_t *asm3;
//extern int32_t asm4;


void sethlinesizes(int32_t x_size_bits, int32_t y_size_bits, const uint8_t * textureAddress)
{
    draws.r_xsizebits = x_size_bits;
    draws.r_ysizebits = y_size_bits;
    draws.textureSetup = textureAddress;
}

__attribute__((always_inline)) static inline uint32_t packCoordinates (uint32_t fixedx, uint32_t fixedy, uint32_t xprec16, uint32_t yprec16)
{
  uint32_t result;
#if MG24 && !ESP_PLATFORM
  // this packs 32 bits fixed point x and y coordinates to a xxxxxyyyyy right aligned variant for indexing to array.
  // xprec16 and yprec16 are the precision (in number of fractional bits) the fixedpoint would have if they were shifted to a 16 bit integer.
  // i.e. they are 16 - size of integer part

  __ASM volatile (
      "LSR %[result], %[x], %[xprec16]\n\t"  // shift x so that its integer part is on the top halfword
      "PKHTB %[result], %[result], %[y], ASR #16\n\t"  // concatenate integer part of x, directly after yfract >> 16
      "LSR %[result], %[result], %[yprec16]\n\t"
      : [result] "=r" (result)
        : [x] "r" (fixedx), [y] "r" (fixedy), [xprec16] "r" (xprec16), [yprec16] "r" (yprec16) );
  return result ;
#else
  result =  (fixedx >> (xprec16 + 16)) << (16 - yprec16);
  result |= (fixedy >> (yprec16 + 16));
  return result;
#endif
}

//FCS:   Draw ceiling/floors
//Draw a line from destination in the framebuffer to framebuffer-numPixels
// next-hack: why is this called "4" ? it does pixel by pixel!
#if WIN32
int hlineasm4calls = 0;
int hlineasm4pix = 0;
#endif // WIN32
void hlineasmQuad(int32_t numPixels, uint32_t fixed_y, uint32_t fixed_x, uint8_t *dest)
{
#if WIN32 || ESP_PLATFORM
    // if drawing more than 3 pixels, let's check if we can store aligned.
    const uint8_t * texture = draws.textureSetup;
    const int32_t stepx = draws.asm1;
    const int32_t stepy = draws.asm2;
    const uint8_t * pal = draws.globalpalshade;
    int32_t yprec16 = 16 - draws.r_ysizebits;
    int32_t xprec16 = 16 - draws.r_xsizebits;
    if (numPixels > 3)
    {
        int numPixelToDraw = (int32_t) dest & 3;
        switch (numPixelToDraw)
        {
            case 3:  *dest++ = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x += stepx; fixed_y += stepy;
            case 2:  *dest++ = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x += stepx; fixed_y += stepy;
            case 1:  *dest++ = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x += stepx; fixed_y += stepy;
            case 0:
              break;
        }
        numPixels -= numPixelToDraw;
        while (numPixels >= 4)
        {
          uint32_t qpix;
          // accumulate in reverse order
          qpix = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x += stepx; fixed_y += stepy;
          qpix |= pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]] << 8; fixed_x += stepx; fixed_y += stepy;
          qpix |= pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]] << 16; fixed_x += stepx; fixed_y += stepy;
          qpix |= pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]] << 24; fixed_x += stepx; fixed_y += stepy;
          *((uint32_t*)dest) = qpix;
          numPixels -= 4;
          dest += 4;
        }
    }
    switch (numPixels & 3)
    {
        case 3:  *dest++ = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x += stepx; fixed_y += stepy;
        case 2:  *dest++ = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x += stepx; fixed_y += stepy;
        case 1:  *dest++ = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x += stepx; fixed_y += stepy;
    }
#else
    const uint8_t * texture = draws.textureSetup;
    const int32_t stepx = draws.asm1;
    const int32_t stepy = draws.asm2;
    const uint8_t * pal = draws.globalpalshade;
    int32_t yprec16 = 16 - draws.r_ysizebits;
    int32_t xprec16 = 16 - draws.r_xsizebits;
    int tmp = 0, qpix = 0;
#if USE_MORE_REGS
    int numPixels2;
#endif
#define PACK_COORD_ASM(variable) \
        "LSR.W %[" #variable "], %[fixed_x], %[xprec16]\n\t"   \
        "PKHTB.W %[" #variable "],%[" #variable "], %[fixed_y], ASR #16\n\t"  \
        "LSR.W %[" #variable "], %[" #variable "], %[yprec16]\n\t"

#define WRITE_TEX_PIX_SNGL()   \
        "LDRB.W %[qpix], [%[texture], %[qpix]]\n\t" \
        "ADD.W %[fixed_x], %[fixed_x], %[stepx]\n\t" \
        "LDRB.W %[qpix], [%[pal], %[qpix]]\n\t" \
        "ADD.W %[fixed_y], %[fixed_y], %[stepy]\n\t" \
        "STRB.W %[qpix], [%[dest]], #1\n\t"

    __ASM volatile
    (
        // check if we have at least 4 pixels
        "CMP %[numPixels], #3\n\t"
        "BLE finalPix%=\n\t"
        // 4 pixels or more. Are them aligned?
        "AND.W %[tmp], %[dest], #3\n\t"       // check how many unaligned bytes do we have.
        "TBH.W [pc, %[tmp], LSL #1]\n\t"
        "BR_TABLE%=:\n\t"    // branch table
        ".hword ((SCase0%= - BR_TABLE%=)/2)\n\t"
        ".hword ((SCase1%= - BR_TABLE%=)/2)\n\t"
        ".hword ((SCase2%= - BR_TABLE%=)/2)\n\t"
        ".hword ((SCase3%= - BR_TABLE%=)/2)\n\t"
        "SCase3%=:\n\t"
        PACK_COORD_ASM(qpix)
        WRITE_TEX_PIX_SNGL()
        "SCase2%=:\n\t"
        PACK_COORD_ASM(qpix)
        WRITE_TEX_PIX_SNGL()
        "SCase1%=:\n\t"
        PACK_COORD_ASM(qpix)
        WRITE_TEX_PIX_SNGL()
        "SCase0%=:\n\t"
        // now decrement number of pixels to draw
        "SUB.W %[numPixels], %[numPixels], %[tmp]\n\t"
#if !USE_MORE_REG
        "PUSH.W {%[numPixels]}\n\t"       // save num pixels
        "LSRS %[numPixels], %[numPixels], #2\n\t"   // divide by 4
#else
        "LSRS %[numPixels2], %[numPixels], #2\n\t"   // divide by 4
#endif
        "BEQ preFinalPix%=\n\t"                        // no 4 pixels available ?
        "loop%=:\n\t"
        //
        // FIRST PIXEL
        PACK_COORD_ASM(tmp)
        //
        "LDRB.W %[tmp], [%[texture], %[tmp]]\n\t"
        "ADD.W %[fixed_x], %[fixed_x], %[stepx]\n\t"
        "LDRB.W %[qpix], [%[pal], %[tmp]]\n\t"          // this is different. Load to Qpix directly!
        "ADD.W %[fixed_y], %[fixed_y], %[stepy]\n\t"
        //
        // SECOND PIXEL
        PACK_COORD_ASM(tmp)
        //
        "LDRB.W %[tmp], [%[texture], %[tmp]]\n\t"
        "ADD.W %[fixed_x], %[fixed_x], %[stepx]\n\t"
        "LDRB.W %[tmp], [%[pal], %[tmp]]\n\t"
        "ADD.W %[fixed_y], %[fixed_y], %[stepy]\n\t"
        "ORR.W %[qpix], %[qpix], %[tmp], LSL #8\n\t"
        //
        // THIRD PIXEL
        PACK_COORD_ASM(tmp)
        //
        "LDRB.W %[tmp], [%[texture], %[tmp]]\n\t"
        "ADD.W %[fixed_x], %[fixed_x], %[stepx]\n\t"
        "LDRB.W %[tmp], [%[pal], %[tmp]]\n\t"
        "ADD.W %[fixed_y], %[fixed_y], %[stepy]\n\t"
        "ORR.W %[qpix], %[qpix], %[tmp], LSL #16\n\t"
        //
        // FOURTH PIXEL
        PACK_COORD_ASM(tmp)
        //
        "LDRB.W %[tmp], [%[texture], %[tmp]]\n\t"
        "ADD.W %[fixed_x], %[fixed_x], %[stepx]\n\t"
        "LDRB.W %[tmp], [%[pal], %[tmp]]\n\t"
        "ADD.W %[fixed_y], %[fixed_y], %[stepy]\n\t"
        "ORR.W %[qpix], %[qpix], %[tmp], LSL #24\n\t"
        // store ALL
        "STR.W %[qpix], [%[dest]], #4\n\t"
        // decrement and check
#if USE_MORE_REGS
        "SUBS %[numPixels2], %[numPixels2], #1\n\t"
#else
        "SUBS %[numPixels], %[numPixels], #1\n\t"
#endif
        "BGT loop%=\n\t"
        //
        "preFinalPix%=:\n\t"
#if !USE_MORE_REGS
        "POP.W {%[numPixels]}\n\t"                    // restore number of pixels
#endif
        "AND.W %[numPixels], %[numPixels], #3\n\t"    // get remaining pixels.
        "finalPix%=:"
        "TBH.W [pc, %[numPixels], LSL #1]\n\t"
        "EBR_TABLE%=:\n\t"    // branch table
        ".hword ((ECase0%= - EBR_TABLE%=)/2)\n\t"
        ".hword ((ECase1%= - EBR_TABLE%=)/2)\n\t"
        ".hword ((ECase2%= - EBR_TABLE%=)/2)\n\t"
        ".hword ((ECase3%= - EBR_TABLE%=)/2)\n\t"
        "ECase3%=:\n\t"
        PACK_COORD_ASM(qpix)
        WRITE_TEX_PIX_SNGL()
        "ECase2%=:\n\t"
        PACK_COORD_ASM(qpix)
        WRITE_TEX_PIX_SNGL()
        "ECase1%=:\n\t"
        PACK_COORD_ASM(qpix)
        WRITE_TEX_PIX_SNGL()
        "ECase0%=:\n\t"
        : // out/inout
          [numPixels] "+&r" (numPixels),      // how many pixels to draw
          [tmp] "+&r" (tmp),                  // temporary variable
          [qpix] "+&r" (qpix),                  // temporary quad pix
          [fixed_x] "+&r" (fixed_x),                  // fixed point x coord
          [fixed_y] "+&r" (fixed_y),                  // fixed point y coord
#if USE_MORE_REGS
          [numPixels2] "+&r" (numPixels2),            // teporary store value for numPixels
#endif
          [dest] "+&r" (dest)                // destination pixel

          : // in
          [yprec16] "r" (yprec16),                  // how many fractional bits y has in 16 bit hword
          [xprec16] "r" (xprec16),                  // how many fractional bits x has in 16 bit hword
          [stepx] "r" (stepx),                  // fixed point x coord
          [stepy] "r" (stepy),                  // fixed point y coord
          [pal] "r" (pal),                  // fixed point x coord
          [texture] "r" (texture)                  // fixed point y coord

        : // clobbers
          "cc", "memory"
    );
#endif
}

void hlineasm4(int32_t numPixels, /*int32_t shade,*/ uint32_t fixed_y, uint32_t fixed_x, uint8_t *dest)
{
    #if WIN32
        hlineasm4calls++;
        hlineasm4pix += numPixels;
    #endif // WIN32

    const uint8_t * texture = draws.textureSetup;


   // shade = shade & 0xffffff00;
    numPixels++;
    const int32_t stepx = draws.asm1;
    const int32_t stepy = draws.asm2;
    const uint8_t * pal = draws.globalpalshade;
#define ALIGN_4_BYTES 0
#define UNROLL_H_LINE_LOOP  0
#define USE_PACKADDRESS MG24
#if ALIGN_4_BYTES
    int32_t yprec16 = 16 - draws.r_ysizebits;
    int32_t xprec16 = 16 - draws.r_xsizebits;

    while ((((uint32_t)dest & 3) != 3) && numPixels)
    {
      *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
      numPixels--;
    }
    // start with first pixel of the word
    dest = dest - 3;
    while (numPixels >= 4)
    {
      uint32_t qpix;
      // accumulate in reverse order
      qpix = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]] << 24; fixed_x -= stepx; fixed_y -= stepy;
      qpix |= pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]] << 16; fixed_x -= stepx; fixed_y -= stepy;
      qpix |= pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]] << 8; fixed_x -= stepx; fixed_y -= stepy;
      qpix |= pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
      *((uint32_t*)dest) = qpix;
      numPixels -= 4;
      dest -= 4;
    }
    // start with last pixel
    dest +=3 ;
    while (numPixels)
    {
      *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
      numPixels--;
    }
#elif !UNROLL_H_LINE_LOOP && USE_PACKADDRESS
#if DISABLED_FOR_NOW
    asm volatile
    (
        // align to 4 bytes boundary
        "TST %[dest], #3\n\t"
        "ITTTE\r\n"
        ""
        :  // outputs, in/outputs
       [dest] "&+r" (dest),
       [numPixels] "&+r" (numPixels),
       [fixed_y] "&+r" (fixed_y),
       [fixed_x] "&+r" (fixed_x),
        :  // input
        [yprec16] "r" (yprec16),
        [xprec16] "r" (xprec16),
        [stepy] "r" (stepy),
        [stepx] "r" (stepx),
        [pal] "r" (pal)
        : "cc"
    );
#else
    int32_t source;
    //int y_precision_bits = 32 - yprec;
    int32_t yprec16 = 16 - draws.r_ysizebits;
    int32_t xprec16 = 16 - draws.r_xsizebits;
    while (numPixels)
    {
	    source = packCoordinates (fixed_x, fixed_y, xprec16, yprec16);


	    source = texture[source];

// write to buffer, applying shade.
      uint8_t pix = pal[source];

      // add the x, y step (including fractional and integer part).

      fixed_y -= stepy;
      fixed_x -= stepx;
      // strange, this is rendering right to left.

      *dest-- = pix;

	    numPixels--;
    }

#endif
#elif !UNROLL_H_LINE_LOOP
    int32_t x_precision_bits = ((256-draws.r_xsizebits) & 0x1f);  // can't it be simply 32-r_xsizebits ?

    const int yprec = draws.r_ysizebits;

    uint32_t source;
    while (numPixels)
    {
//  Compute the texture pointer as (x << logWidth) | y
	    source = fixed_x >> x_precision_bits;     // this gets the integer x part
	    source = shld(source, fixed_y, yprec); // this expands to (i4 >>(32-rh_ysizebits)) | (source << rh_ysizebits)
// load texture pixel
	    source = texture[source];

// write to buffer, applying shade.
        *dest = pal[source];

// strange, this is rendering right to left.
	    dest--;
// add the x, y step (including fractional and integer part).
	    fixed_x -= stepx;
	    fixed_y -= stepy;

	    numPixels--;
    }
#elif UNROLL_H_LINE_LOOP && USE_PACKADDRESS

#if 1
    int32_t yprec16 = 16 - draws.r_ysizebits;
    int32_t xprec16 = 16 - draws.r_xsizebits;

   int r = numPixels / 8;
   while (r--)
   {
     *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
   }
   switch(numPixels & 7)
   {
     case 7: *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     case 6: *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     case 5: *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     case 4: *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     case 3: *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     case 2: *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
     case 1: *dest-- = pal[texture[packCoordinates(fixed_x, fixed_y, xprec16, yprec16)]]; fixed_x -= stepx; fixed_y -= stepy;
   }
#endif
#endif
}

#if DRAW_CODE_CLEANUP
static const uint8_t* r_palette;    // self explanatory
static int32_t r_deltaxFract;       // fractional deltaX part left shifted by 16, so that quick check can be done
static int32_t r_deltayFract;       // fractional deltaY part left shifted by 16, so that quick check can be done
static int32_t r_tileheight;        // self explanatory
static int32_t r_intTileXYStep;     // when computing next pixel, this is the integer deltaX-Y step on the tile array.
#else
static int32_t rmach_eax;
static int32_t rmach_ebx;
static int32_t rmach_ecx;
static const uint8_t* rmach_edx;
static int32_t rmach_esi;
#endif // CODE_CLEANUP
// these apis are called by dorotatesprite.
void setuprhlineasm4(int32_t i1, int32_t i2, int32_t i3, const uint8_t* i4, int32_t i5)
{   // bleaurgh.
    // 16 was not even used. removed.
    // 15 is tileHeight
    // i4 is palette
    // i3 is the offset on the tile buffer.
    // i2 is yv2 << 16
    // i1 is xv2 << 16.
    // note, since the tileoffset is (xv2>>16)*tileHeigth + (yv2 >> 16).
    // this suggests me that xv2 and yv2 are in fixed point. So i1 and i2 are the fractional part multiplied by 65536
#if DRAW_CODE_CLEANUP
    r_deltaxFract = i1;
    r_deltayFract = i2;
    r_intTileXYStep = i3;
    r_palette = i4;
    r_tileheight = i5;
#else
    rmach_eax = i1; // x << 16. Fractional part?
    rmach_ebx = i2;
    rmach_ecx = i3;
    rmach_edx = i4;
    rmach_esi = i5;
#endif // CODE_CLEANUP
}

#if DRAW_CODE_CLEANUP
// this is used by dorotate sprite.
void rhlineasm4(int32_t numPixels, const uint8_t* texture, uint32_t fractX, uint32_t fractY, uint8_t* dest)
{
    // next-hack: this was quite crappy.
    // So: i1 is the number of pixels. Renamed to numpixel.
    // i3 is always 0, so not sure why it was there in the first place.
    // i4 is x start left shifted by 16. Fractional part ?
    // i5 is y start left shifted by 16
    if (numPixels <= 0) return;
	int32_t offset = numPixels + 1;


    do
    {
        // get pixel.
	    uint8_t pixel = *texture; //was i3 = ((i3&0xffffff00)|(*texture));
        // Store it, with the correct palette.
        dest[numPixels - offset] = r_palette[pixel];
        // compute the position of next pixel.
	    // subtract the fractional part of x.
	    fractX -= r_deltaxFract;
	    // did we overflow? Then we subtract the tileheight from the texture buffer.
	    // In asm would be like a sub and a test for carry.
	    texture -= (((fractX + r_deltaxFract) < fractX) ? r_tileheight : 0);

	    // now subtract the fractional y part.
	    //
	    fractY -= r_deltayFract;

	    if ((fractY + r_deltayFract) < fractY)
            texture -= (r_intTileXYStep+1);     // in case of overflow, we must point to the pixel next to the integer increment.
	    else
            texture -= r_intTileXYStep;

        // decrement count
	    numPixels--;
    } while (numPixels);
}

#else
void rhlineasm4(int32_t i1, const uint8_t* texture, uint32_t i4, uint32_t i5, uint8_t* dest)
{
    // next-hack: this is quite crappy.
    // So: i1 is the number of pixels
    // i3 is always 0, so not sure why it was there in the first place.
    // i4 is x start left shifted by 16
    // i5 is y start left shifted by 16

    uint32_t ebp = 0;
    int32_t numPixels;
	int32_t offset = i1 + 1;

    if (i1 <= 0) return;
    uint8_t i3 = 0;
    numPixels = i1;
    do {
	    i3 = ((i3&0xffffff00)|(*texture)); // this can be like uint8_t pixel = *texture
	    i4 -= rmach_eax;                    // subtract the fractional part (in top halfword) with the delta fractional part
	    ebp = (((i4+rmach_eax) < i4) ? -1 : 0); // if overflow, then set epb to -1. Will be used later, masked by tileheight to determine how much we need subtract.
	    // subtract the fractional part of y
	    i5 -= rmach_ebx;

	    if ((i5 + rmach_ebx) < i5)
            texture -= (rmach_ecx+1);
	    else
            texture -= rmach_ecx;

	    ebp &= rmach_esi;
	    i1 = ((i1&0xffffff00)|rmach_edx[i3]);

#if RENDER_LIMIT_PIXELS
		if (pixelsAllowed-- > 0)
#endif
			dest[numPixels - offset] = (i1&0xff);

	    texture -= ebp;
	    numPixels--;
    } while (numPixels);
}

#endif // CLEANUP




#if DRAW_CODE_CLEANUP //
static const uint8_t* r_palette;    // self explanatory
static int32_t r_deltaxFract;       // fractional deltaX part left shifted by 16, so that quick check can be done
static int32_t r_deltayFract;       // fractional deltaY part left shifted by 16, so that quick check can be done
static int32_t r_tileheight;        // self explanatory
static int32_t r_intTileXYStep;     // when computing next pixel, this is the integer deltaX-Y step on the tile array.
// again dorotate sprite. This time is masked.
void setuprmhlineasm4(int32_t i1, int32_t i2, int32_t i3, const uint8_t* i4, int32_t tileHeight)
{
    r_deltaxFract = i1;    // fract x
    r_deltayFract = i2;    // fract y
    r_intTileXYStep = i3;    // integer xy step
    r_palette = i4;    // palette
    r_tileheight = tileHeight;   // height
}
//FCS: ????
// next-hack: this is called by dorotate sprite. Simply draw a masked horizontal line.
void rmhlineasm4(int32_t numPixels, const uint8_t* source, int32_t fractX, int32_t fractY, uint8_t* dest)
{
    if (numPixels <= 0)
    {
        return;
    }
	int32_t offset = numPixels + 1;

    do
    {
	    uint8_t colorIndex = *source;

        //Check if this colorIndex is the transparent color (255).
	    if ((colorIndex) != 255)
        {
				uint8_t pixel = r_palette[colorIndex];
				dest[numPixels - offset] = pixel;
        }

	    fractX -= r_deltaxFract;
	    source -= (((fractX+r_deltaxFract) < fractX) ? r_tileheight : 0);

	    fractY -= r_deltayFract;

	    if ((fractY + r_deltayFract) < fractY)
            source -= (r_intTileXYStep+1);
	    else
            source -= r_intTileXYStep;

	    numPixels--;

    } while (numPixels);
}

#else
static int32_t rmmach_eax;
static int32_t rmmach_ebx;
static int32_t rmmach_ecx;
static const uint8_t* rmmach_edx;
static int32_t setupTileHeight;
void setuprmhlineasm4(int32_t i1, int32_t i2, int32_t i3, const uint8_t* i4, int32_t tileHeight, int32_t i6)
{
    rmmach_eax = i1;    // fract x
    rmmach_ebx = i2;    // fract y
    rmmach_ecx = i3;    // integer xy step
    rmmach_edx = i4;    // palette
    setupTileHeight = tileHeight;   // height
}
//FCS: ????
// next-hack: this is called by dorotate sprite. Simply draw a masked horizontal line.
void rmhlineasm4(int32_t i1, const uint8_t* shade, int32_t colorIndex, int32_t i4, int32_t i5, uint8_t* dest)
{
    uint32_t ebp = 0;
    int32_t numPixels;
	int32_t offset = i1 + 1;

    if (i1 <= 0)
        return;

    numPixels = i1;
    do {
	    colorIndex = ((colorIndex&0xffffff00)|(*((uint8_t *)shade)));
	    i4 -= rmmach_eax;
	    ebp = (((i4+rmmach_eax) < i4) ? -1 : 0);
	    i5 -= rmmach_ebx;

	    if ((i5 + rmmach_ebx) < i5)
            shade -= (rmmach_ecx+1);
	    else
            shade -= rmmach_ecx;

	    ebp &= setupTileHeight;

        //Check if this colorIndex is the transparent color (255).
	    if ((colorIndex&0xff) != 255) {
#if RENDER_LIMIT_PIXELS
			if (pixelsAllowed-- > 0)
#endif
			{
				i1 = ((i1&0xffffff00)|rmmach_edx[colorIndex]);
				dest[numPixels - offset] = (i1 & 0xff);
			}
	    }

	    shade -= ebp;
	    numPixels--;

    } while (numPixels);
}

#endif // DRAW_CODE_CLEANUP





//Variable used to draw column.
//This is how much you have to skip in the framebuffer in order to be one pixel below.
//int32_t draw_bytesperline;
void setBytesPerLine(int32_t bytesperline)
{
    draws.bytesperline = bytesperline;
}



//uint8_t  precisionBits;  // was mach3_al

//FCS:  RENDER TOP AND BOTTOM COLUMN
#if CODE_DISABLED
int32_t prevlineasm1(int32_t i1, const uint8_t* palette, int32_t i3, int32_t i4, const uint8_t  *source, uint8_t  *dest)
{
    // next-hack: what is this shit? Simply if we have to draw a single pixel, we do it here, otherwise we call  vlineasm1.
    // but this is silly, as we would be slower in doing the check.
 #if 0
    if (i3 == 0)
    {
		if (!RENDER_DRAW_TOP_AND_BOTTOM_COLUMN)
            return 0;

	    i1 += i4;
        i4 = ((uint32_t)i4) >> mach3_al;
	    i4 = (i4&0xffffff00) | source[i4];

#if RENDER_LIMIT_PIXELS
		if (pixelsAllowed-- > 0)
#endif
			*dest = palette[i4];

	    return i1;
    } else
#endif
    {
	    return vlineasm1(i1,palette,i3,i4,source,dest);
    }
}
#endif

// this will draw a vertical translucent line
int32_t tvlineasm1(int32_t ystep, const uint8_t *texture, int32_t numPixels, int32_t fixed_y, const uint8_t  *source, uint8_t  *dest)
{
// TODO: to be optimized.
    uint8_t shiftValue = (globalshiftval & 0x1f);

	numPixels++;
	while (numPixels)
	{
		uint8_t pix = source[(uint32_t) fixed_y >> shiftValue];

	    //255 is the index for transparent color index. Skip drawing this pixel.
		if (pix != 255)
		{
			uint16_t colorIndex;

			colorIndex = texture[pix];
			colorIndex |= ((*dest)<<8);

			if (transrev)
				colorIndex = ((colorIndex>>8)|(colorIndex<<8));

            *dest = transluc[colorIndex];
		}

		fixed_y += ystep;

        //We are drawing a column ?!
		dest += draws.bytesperline;
		numPixels--;
	}
	return fixed_y;
} /* tvlineasm1 */

#if CODE_DISABLED

static uint8_t  tran2shr;
static const uint8_t* tran2pal_ebx;
static const uint8_t* tran2pal_ecx;
void setuptvlineasm2(int32_t i1, const uint8_t* i2, const uint8_t* i3)
{
	tran2shr = (i1&0x1f);
	tran2pal_ebx = i2;
	tran2pal_ecx = i3;
} /* */

void tvlineasm2(uint32_t i1, uint32_t i2, uintptr_t i3, uintptr_t i4, uint32_t i5, uintptr_t i6)
{
	uint32_t ebp = i1;
	uint32_t tran2inca = i2;
	uint32_t tran2incb = asm1;
	uintptr_t tran2bufa = i3;
	uintptr_t tran2bufb = i4;
	uintptr_t tran2edi = asm2;
	uintptr_t tran2edi1 = asm2 + 1;

	i6 -= asm2;

	do {

		i1 = i5 >> tran2shr;
		i2 = ebp >> tran2shr;
		i5 += tran2inca;
		ebp += tran2incb;
		i3 = ((uint8_t  *)tran2bufa)[i1];
		i4 = ((uint8_t  *)tran2bufb)[i2];
		if (i3 == 255) { // skipdraw1
			if (i4 != 255) { // skipdraw3
				uint16_t val;
				val = tran2pal_ecx[i4];
				val |= (((uint8_t  *)i6)[tran2edi1]<<8);

				if (transrev)
					val = ((val>>8)|(val<<8));

#if RENDER_LIMIT_PIXELS
				if (pixelsAllowed-- > 0)
#endif
					((uint8_t  *)i6)[tran2edi1] = transluc[val];
			}
		} else if (i4 == 255) { // skipdraw2
			uint16_t val;
			val = tran2pal_ebx[i3];
			val |= (((uint8_t  *)i6)[tran2edi]<<8);

			if (transrev)
                val = ((val>>8)|(val<<8));

#if RENDER_LIMIT_PIXELS
			if (pixelsAllowed-- > 0)
#endif
				((uint8_t  *)i6)[tran2edi] = transluc[val];
		} else {
			uint16_t l = ((uint8_t  *)i6)[tran2edi]<<8;
			uint16_t r = ((uint8_t  *)i6)[tran2edi1]<<8;
			l |= tran2pal_ebx[i3];
			r |= tran2pal_ecx[i4];
			if (transrev) {
				l = ((l>>8)|(l<<8));
				r = ((r>>8)|(r<<8));
			}
#if RENDER_LIMIT_PIXELS
			if (pixelsAllowed-- > 0)
#endif
			{
				((uint8_t  *)i6)[tran2edi] = transluc[l];
				((uint8_t  *)i6)[tran2edi1] =transluc[r];
#if RENDER_LIMIT_PIXELS
				pixelsAllowed--;
#endif
			}
		}
		i6 += draws.bytesperline;
	} while (i6 > i6 - draws.bytesperline);
	asm1 = i5;
	asm2 = ebp;
}
static uint8_t  machmv;
#endif



void setupvlineasm(int32_t i1)
{
    draws.precisionBits = (i1&0x1f);
}
#if CODE_DISABLED
//FCS This is used to fill the inside of a wall (so it draws VERTICAL column, always).
// next-hack: what a confusion, this code is crap.
// So framebuffer is not really the framebuffer, but already starts at the first pixel (x,y)
// Second, column index is not any column. Is the number of vertical pixels to draw.
// tHis function is called vlineasm4 because it does not just print a single column,
// but actually 4.
void vlineasm4(int32_t columnIndex, uint8_t* framebuffer)
{
	if (!RENDER_DRAW_WALL_INSIDE)
		return;

	int i;
	uint32_t temp;
    uint32_t index = 0;
    uint32_t length = ylookup(columnIndex);

	do {
		for (i = 0; i < 4; i++)
		{
			temp = ((uint32_t)vplce[i]) >> mach3_al;
			temp = (((uint8_t*)(bufplce[i]))[temp]);

#if RENDER_LIMIT_PIXELS
			if (pixelsAllowed-- > 0)
#endif
				framebuffer[index + i] = palookupoffse[i][temp];

			vplce[i] += vince[i];
		}
		index += draws.bytesperline;
	} while (index < length);
}
#endif

void setupmvlineasm(int32_t i1)
{
    //Only keep 5 first bits
  draws.precisionBits = (i1&0x1f);
}
#if CODE_DISABLED
void mvlineasm4(int32_t columnIndex, uint8_t* framebuffer)
{
    int i;
    uint32_t temp;
	uint32_t index = 0;
	uint32_t length = ylookup(columnIndex);

    do {

#if RENDER_LIMIT_PIXELS
		if (pixelsAllowed <= 0)
			return;
#endif

        for (i = 0; i < 4; i++)
        {

	      temp = ((uint32_t)vplce[i]) >> machmv;
	      temp = (((uint8_t *)(bufplce[i]))[temp]);
	      if (temp != 255)
		  {
#if RENDER_LIMIT_PIXELS
			  if (pixelsAllowed-- > 0)
#endif
				  framebuffer[index+i] = palookupoffse[i][temp];
		  }
	      vplce[i] += vince[i];
        }
        index += draws.bytesperline;

    } while (index < length);
}
#endif
/* END ---------------  WALLS RENDERING METHOD (USED TO BE HIGHLY OPTIMIZED ASSEMBLY) ----------------------------*/


/* ---------------  SPRITE RENDERING METHOD (USED TO BE HIGHLY OPTIMIZED ASSEMBLY) ----------------------------*/


/*
 FCS: Draw a sprite vertical line of pixels.
 */
#if DRAW_CODE_CLEANUP | 1
const uint8_t * tspal;
uint32_t adder;
uint32_t tsmach_eax3; //what is this for ?

void tsetupspritevline(const uint8_t * palette, int32_t intDeltaXptr, int32_t dxf, int32_t tileHeight, int32_t fixedStepY)
{
	r_palette = palette;
	r_deltayFract = fixedStepY << 16;
	adder = (fixedStepY >> 16) + intDeltaXptr;
	tsmach_eax3 = adder + tileHeight;
	r_deltaxFract = dxf;
}

void DrawSpriteTranslucentVerticalLine(int32_t fractY, int32_t numPixels, uint32_t fractX, const uint8_t  * texture, uint8_t  * dest)
{
    // TODO: optimize me
    uint8_t colorIndex;
	while (numPixels) // mh?
	{
	    numPixels --;
	    if (!numPixels)
	        break;
        fractX += (uint32_t) r_deltaxFract;

        if (fractX < (fractX - (uint32_t) r_deltaxFract))
        {
            adder = tsmach_eax3;                // Probably this is when you roll over ?
#if WIN32
            FIXME("I don't think I would be called. Probably I'm a bug");
#endif
        }

        colorIndex = *texture;

        fractY += (uint32_t) r_deltayFract;
        if ((uint32_t) fractY < (uint32_t) (fractY - (uint32_t)r_deltayFract))
            texture++;  // point to next pixel

        texture += adder;

        //255 is the index of the transparent color: Do not draw it.
        if (colorIndex != 255)
        {
            uint16_t val;
            val = r_palette[colorIndex];
            val |= (*dest)<<8;

            if (transrev)
                val = ((val>>8)|(val<<8));

            colorIndex = transluc[val];

            *dest = colorIndex;
        }

        //Move down one pixel on the framebuffer
        dest += draws.bytesperline;
       // numPixels--;
	}
}

#else
const uint8_t * tspal;
uint32_t tsmach_eax1;
uint32_t adder;
uint32_t tsmach_eax3;
uint32_t tsmach_ecx;
void tsetupspritevline(const uint8_t * palette, int32_t i2, int32_t i3, int32_t i4, int32_t i5)
{
	tspal = palette;
	tsmach_eax1 = i5 << 16;
	adder = (i5 >> 16) + i2;
	tsmach_eax3 = adder + i4;
	tsmach_ecx = i3;
}

void DrawSpriteVerticalLine(int32_t i2, int32_t numPixels, uint32_t i4, const uint8_t  * texture, uint8_t  * dest)
{
    uint8_t colorIndex;

	while (numPixels)
	{
		numPixels--;

		if (numPixels != 0)
		{

			i4 += tsmach_ecx;

			if (i4 < (i4 - tsmach_ecx))
                adder = tsmach_eax3;

			colorIndex = *texture;

			i2 += tsmach_eax1;
			if (i2 < (i2 - tsmach_eax1))
                texture++;

			texture += adder;

            //255 is the index of the transparent color: Do not draw it.
			if (colorIndex != 255)
			{
				uint16_t val;
				val = tspal[colorIndex];
				val |= (*dest)<<8;

				if (transrev)
					val = ((val>>8)|(val<<8));

				colorIndex = transluc[val];

#if RENDER_LIMIT_PIXELS
				if (pixelsAllowed-- > 0)
#endif
					*dest = colorIndex;
			}

            //Move down one pixel on the framebuffer
			dest += draws.bytesperline;
		}


	}
}

 #endif // DRAW_CODE_CLEANUP
/* END---------------  SPRITE RENDERING METHOD (USED TO BE HIGHLY OPTIMIZED ASSEMBLY) ----------------------------*/


/* ---------------  FLOOR/CEILING RENDERING METHOD (USED TO BE HIGHLY OPTIMIZED ASSEMBLY) ----------------------------*/

void settrans(int32_t type){
	transrev = type;
}
#if DRAW_CODE_CLEANUP
static const uint8_t  * textureData;
//static uint8_t  * mmach_asm3;
static int32_t stepX;
static int32_t stepY;

void mhline(const uint8_t  * texture, int32_t i2, int32_t numPixels, int32_t i5, uint8_t* dest)
{
// TODO: clean this up
    textureData = texture;
    r_palette = (uint8_t *) draws.asm3;
    stepX = draws.asm1;
    stepY = draws.asm2;
    mhlineskipmodify(i2,numPixels>>16,i5,dest);
}


static uint8_t  fracbits = 26;
static uint8_t  integerbits = 6;
void mhlineskipmodify( uint32_t fixedX, int32_t numPixels, int32_t fixedY, uint8_t* dest)
{
// TODO: to optimize
    uint32_t ebx;
    int32_t colorIndex;

    while (numPixels >= 0)
    {
        // get texture position as x (integer bits) concatenated to integer bits of y:
        // i.e. ((x >> precision) << YBITS) | (y >> PRECISION)
	    ebx = fixedX >> fracbits;
	    ebx = shld (ebx, (uint32_t)fixedY, integerbits);
	    colorIndex = textureData[ebx];

        //Skip transparent color.
		if ((colorIndex&0xff) != 0xff){
#if RENDER_LIMIT_PIXELS
            if (pixelsAllowed-- > 0)
#endif
				*dest = r_palette[colorIndex];
        }
	    fixedX += stepX;
	    fixedY += stepY;
	    dest++;
	    numPixels--;
    }
}


void msethlineshift(int32_t i1, int32_t i2)
{
    i1 = 256-i1;
    fracbits = (i1&0x1f);
    integerbits = (i2&0x1f);
} /* msethlineshift */

#else
static uint8_t  * textureData;
static uint8_t  * mmach_asm3;
static int32_t mmach_asm1;
static int32_t mmach_asm2;

void mhline(uint8_t  * texture, int32_t i2, int32_t numPixels, int32_t i4, int32_t i5, uint8_t* dest)
{

    textureData = texture;
    mmach_asm3 = asm3;
    mmach_asm1 = asm1;
    mmach_asm2 = asm2;
    mhlineskipmodify(i2,numPixels>>16,i5,dest);
}


static uint8_t  mshift_al = 26;
static uint8_t  mshift_bl = 6;
void mhlineskipmodify( uint32_t i2, int32_t numPixels, int32_t i5, uint8_t* dest)
{
    uint32_t ebx;
    int32_t colorIndex;

    while (numPixels >= 0)
    {
	    ebx = i2 >> mshift_al;
	    ebx = shld (ebx, (uint32_t)i5, mshift_bl);
	    colorIndex = textureData[ebx];

        //Skip transparent color.
		if ((colorIndex&0xff) != 0xff){
#if RENDER_LIMIT_PIXELS
            if (pixelsAllowed-- > 0)
#endif
				*dest = mmach_asm3[colorIndex];
        }
	    i2 += mmach_asm1;
	    i5 += mmach_asm2;
	    dest++;
	    numPixels--;


    }
}


void msethlineshift(int32_t i1, int32_t i2)
{
    i1 = 256-i1;
    mshift_al = (i1&0x1f);
    mshift_bl = (i2&0x1f);
} /* msethlineshift */

#endif // DRAW_CODE_CLEANUP

#if DRAW_CODE_CLEANUP
static int32_t tmach_asm1;
static int32_t tmach_asm2;
// next-hack: translucent horizontal line.

//static uint8_t  tshift_al = 26;
//static uint8_t  tshift_bl = 6;
static void thlineskipmodify(uint32_t fixed_x, int32_t counter, int32_t fixed_y, uint8_t * dest)
{
// TODO: to optimize
    uint32_t ebx;
    while (counter >= 0)
    {
        // compute pixel location in texture
	    ebx = fixed_x >> fracbits;
	    ebx = shld (ebx, (uint32_t)fixed_y, integerbits);
	    uint8_t pixel =  draws.textureSetup[ebx];
	    if ((pixel&0xff) != 0xff)
	    {
		    uint16_t val = r_palette[pixel];
		    val |= (*dest)<<8;

		    if (transrev)
				val = ((val>>8)|(val<<8));

#if RENDER_LIMIT_PIXELS
			if (pixelsAllowed-- > 0)
#endif
			 *dest = transluc[val];
	    }

	    fixed_x += tmach_asm1;
	    fixed_y += tmach_asm2;
	    dest++;
	    counter--;
    }
}

void thline(const uint8_t  * source, int32_t offsetx1, int32_t deltaXlsl16, int32_t offsety, uint8_t * dest)
{
    draws.textureSetup = source;
    r_palette = (uint8_t*) draws.asm3;
    tmach_asm1 = draws.asm1;
    tmach_asm2 = draws.asm2;
    thlineskipmodify(offsetx1,deltaXlsl16 >> 16,offsety,dest);
}

void tsethlineshift(int32_t sizex, int32_t sizey)
{
    sizex = 256-sizex;
    fracbits = (sizex&0x1f);
    integerbits = (sizey&0x1f);
}
#else
static uint8_t * tmach_eax;
static uint8_t * tmach_asm3;
static int32_t tmach_asm1;
static int32_t tmach_asm2;
// next-hack: translucent horizontal line.
void thline(uint8_t  * i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, uint8_t * i6)
{
    tmach_eax = i1;
    tmach_asm3 = asm3;
    tmach_asm1 = asm1;
    tmach_asm2 = asm2;
    thlineskipmodify(asm2,i2,i3,i4,i5,i6);
}

static uint8_t  tshift_al = 26;
static uint8_t  tshift_bl = 6;
void thlineskipmodify(int32_t i1, uint32_t i2, uint32_t i3, int32_t i4, int32_t i5, uint8_t * i6)
{
    uint32_t ebx;
    int counter = (i3>>16);
    while (counter >= 0)
    {
	    ebx = i2 >> tshift_al;
	    ebx = shld (ebx, (uint32_t)i5, tshift_bl);
	    i1 = tmach_eax[ebx];
	    if ((i1&0xff) != 0xff)
	    {
		    uint16_t val = tmach_asm3[i1];
		    val |= (*i6)<<8;

		    if (transrev)
				val = ((val>>8)|(val<<8));

#if RENDER_LIMIT_PIXELS
			if (pixelsAllowed-- > 0)
#endif
			 *i6 = transluc[val];
	    }

	    i2 += tmach_asm1;
	    i5 += tmach_asm2;
	    i6++;
	    counter--;


    }
}


void tsethlineshift(int32_t i1, int32_t i2)
{
    i1 = 256-i1;
    tshift_al = (i1&0x1f);
    tshift_bl = (i2&0x1f);
}

#endif


#if DRAW_CODE_CLEANUP | 1
//static intptr_t slopemach_ebx;

//static float asm2_f;
typedef union { unsigned int i; float f; } bitwisef2i;
void setupslopevlin(int32_t numBitsTextureSize, const uint8_t* textureptr, int32_t dmbpl)
{
    // numBitsTextureSize: byte1: num bits height, byte 0: num bits width
    draws.textureSetup = textureptr;
    draws.dest_minus_bytes_per_line = dmbpl;
    draws.slopemach_edx = (1<<(numBitsTextureSize&0x1f)) - 1;  // number of row bytes -1
    draws.slopemach_edx <<= ((numBitsTextureSize&0x1f00)>>8);  // this is the mask for x coord in the texture
    draws.slopemach_ah1 = 32-((numBitsTextureSize&0x1f00)>>8);    //
    draws.slopemach_ah2 = (draws.slopemach_ah1 - (numBitsTextureSize&0x1f)) & 0x1f;
    draws.asm2_f = (float)draws.asm1;
}
#if NO_FPU
extern int32_t reciptable[2048];
#endif
extern int32_t globalx3, globaly3;
#define low32(a) (((a)&0xffffffff))
#define high32(a) ((int)(((int64_t)(a)&(int64_t)0xffffffff00000000)>>32))

//FCS: Render RENDER_SLOPPED_CEILING_AND_FLOOR.
// next-hack: this will be a pain in the ass to reverse engineer and clean up.
void slopevlin(intptr_t dest, uint32_t i2, intptr_t* slopalookup, uint32_t index, int32_t numrows, int32_t fixed_x1, int32_t fixed_y1)
{
    // fixed_x1 and y1 seems 8.24
  uintptr_t ecx, ebx;
  intptr_t eax;
  uint32_t edx, esi;
  uint32_t edi;
  int lx3 = globalx3;
  int ly3 = globaly3;
//This is so bad to cast asm3 to int then float :( !!!
  float delta_a = draws.asm2_f;
  float a = (float)(int32_t) draws.asm3 + delta_a;
    esi = fixed_x1 + low32((int64_t)lx3 * (int64_t)(i2<<3));
    edi = fixed_y1 + low32((int64_t)ly3 * (int64_t)(i2<<3));

    uint32_t dest_minus_bytes_per_line = draws.dest_minus_bytes_per_line;
    const uint8_t *texture = draws.textureSetup;
    uint32_t slopemach_ah1 = draws.slopemach_ah1;
    uint32_t slopemach_ah2 = (uint32_t) draws.slopemach_ah2;
    uint32_t slopemach_edx = draws.slopemach_edx;

    if (draws.dest_minus_bytes_per_line == -SCREEN_WIDTH)
    {
      do
      {
          eax =  ((int64_t)1 << 33)/a;
        // -------------
        edx = i2;
        i2 = eax;
        eax -= edx;
        ecx = low32((int64_t)lx3 * (int64_t)eax);
        eax = low32((int64_t)ly3 * (int64_t)eax);
        a += delta_a;


        int cliprows = numrows;
        if (cliprows > 8)
            cliprows = 8;
        while(cliprows--)
        {
          edx = edi >> slopemach_ah1;  // ytext  = yplc >> prec
          edi += eax; // eax = yinc. edy = yplc (fixed point) => yplc += yinc
          ebx = (esi >> slopemach_ah2) & slopemach_edx;   //x coordinate in the texture already shifted by the number of bytes
          ebx |= edx; // sum the x and y coordinates
          esi += ecx;     // xplc (fixed point) += xinc
          uint8_t pix = texture[ebx];
          ebx = slopalookup[index];
          index--;
          *((uint8_t  *)dest) = (*((uint8_t*)(ebx + pix)));
          dest -= SCREEN_WIDTH;
        }
        numrows -= 8; // BITSOFPRECISIONPOW
      } while ((int32_t)numrows > 0);
    }
    else
    {
      do
      {
          eax =  ((int64_t)1 << 33)/a;
        // -------------
        edx = i2;
        i2 = eax;
        eax -= edx;
        ecx = low32((int64_t)lx3 * (int64_t)eax);
        eax = low32((int64_t)ly3 * (int64_t)eax);
        a += delta_a;
        int cliprows = numrows;
        if (cliprows > 8)
            cliprows = 8;
        while(cliprows--)
        {
          edx = edi >> draws.slopemach_ah1;  // ytext  = yplc >> prec
          edi += eax; // eax = yinc. edy = yplc (fixed point) => yplc += yinc
            ebx = (esi >> draws.slopemach_ah2) & draws.slopemach_edx;   //x coordinate in the texture already shifted by the number of bytes
          ebx |= edx; // sum the x and y coordinates
          esi += ecx;     // xplc (fixed point) += xinc
          uint8_t pix = texture[ebx];
          ebx = slopalookup[index];
          index--;
          *((uint8_t  *)dest) = (*((uint8_t*)(ebx + pix)));
          dest += dest_minus_bytes_per_line;

        }
        numrows -= 8; // BITSOFPRECISIONPOW
      } while ((int32_t)numrows > 0);
    }
}


#else
static intptr_t slopemach_ebx;
static int32_t slopemach_ecx;
static int32_t slopemach_edx;
static uint8_t  slopemach_ah1;
static uint8_t  slopemach_ah2;
static float asm2_f;
typedef union { unsigned int i; float f; } bitwisef2i;
void setupslopevlin(int32_t i1, intptr_t i2, int32_t i3)
{
    bitwisef2i c;
    slopemach_ebx = i2;
    slopemach_ecx = i3;
    slopemach_edx = (1<<(i1&0x1f)) - 1;
    slopemach_edx <<= ((i1&0x1f00)>>8);
    slopemach_ah1 = 32-((i1&0x1f00)>>8);
    slopemach_ah2 = (slopemach_ah1 - (i1&0x1f)) & 0x1f;
    c.f = asm2_f = (float)asm1;
    asm2 = c.i;
}

extern int32_t reciptable[2048];
extern int32_t globalx3, globaly3;
extern int32_t fpuasm;
#define low32(a) (((a)&0xffffffff))
#define high32(a) ((int)(((int64_t)(a)&(int64_t)0xffffffff00000000)>>32))

//FCS: Render RENDER_SLOPPED_CEILING_AND_FLOOR.
// next-hack: this will be a pain in the ass to reverse engineer and clean up.
void slopevlin(intptr_t i1, uint32_t i2, intptr_t* i3, uint32_t index, int32_t i4, int32_t i5, int32_t i6)
{
    bitwisef2i c;
	uintptr_t ecx, eax, ebx, edx, esi;
	uint32_t edi;
//This is so bad to cast asm3 to int then float :( !!!
    float a = (float)(int32_t) asm3 + asm2_f;
    i1 -= slopemach_ecx;
    esi = i5 + low32((int64_t)globalx3 * (int64_t)(i2<<3));
    edi = i6 + low32((int64_t)globaly3 * (int64_t)(i2<<3));
    ebx = i4;

	if (!RENDER_SLOPPED_CEILING_AND_FLOOR)
		return;

    do {
	    // -------------
	    // All this is calculating a fixed point approx. of 1/a
	    c.f = a;
	    fpuasm = eax = c.i;
	    edx = (((int32_t)eax) < 0) ? 0xffffffff : 0;
	    eax = eax << 1;
	    ecx = (eax>>24);	//  exponent
	    eax = ((eax&0xffe000)>>11);
	    ecx = ((ecx&0xffffff00)|((ecx-2)&0xff));
	    eax = reciptable[eax/4];
	    eax >>= (ecx&0x1f);
	    eax ^= edx;
	    // -------------
	    edx = i2;
	    i2 = eax;
	    eax -= edx;
	    ecx = low32((int64_t)globalx3 * (int64_t)eax);
	    eax = low32((int64_t)globaly3 * (int64_t)eax);
	    a += asm2_f;

	    asm4 = ebx;
	    ecx = ((ecx&0xffffff00)|(ebx&0xff));
	    if (ebx >= 8) ecx = ((ecx&0xffffff00)|8);

	    ebx = esi;
	    edx = edi;
	    while ((ecx&0xff))
	    {
		    ebx >>= slopemach_ah2;
		    esi += ecx;
		    edx >>= slopemach_ah1;
		    ebx &= slopemach_edx;
		    edi += eax;
		    i1 += slopemach_ecx;
		    edx = ((edx&0xffffff00)|((((uint8_t *)(ebx+edx))[slopemach_ebx])));
			ebx = i3[index];
			index--;
			eax = ((eax & 0xffffff00) | (*((uint8_t*)(ebx + edx))));
			ebx = esi;

#if RENDER_LIMIT_PIXELS
			if (pixelsAllowed-- > 0)
#endif
				*((uint8_t  *)i1) = (eax&0xff);

		    edx = edi;
		    ecx = ((ecx&0xffffff00)|((ecx-1)&0xff));


	    }
	    ebx = asm4;
	    ebx -= 8;	// BITSOFPRECISIONPOW



    } while ((int32_t)ebx > 0);
}

#endif // CODE_CLEANUP


/* END ---------------  FLOOR/CEILING RENDERING METHOD (USED TO BE HIGHLY OPTIMIZED ASSEMBLY) ----------------------------*/
