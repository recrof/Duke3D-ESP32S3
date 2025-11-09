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
#include <string.h>
#include <fcntl.h>

#include "types.h"
#include "develop.h"
#include "util_lib.h"
//#include "_animlib.h"
#include "animlib.h"

//****************************************************************************
//
// GLOBALS
//
//****************************************************************************

//****************************************************************************
//
// LOCALS
//
//****************************************************************************
anim_t * anim=NULL;
static boolean Anim_Started = false;

//****************************************************************************
//
//      CheckAnimStarted ()
//
//****************************************************************************

void CheckAnimStarted ( char  * funcname )
   {
   if (!Anim_Started)
      Error(EXIT_FAILURE, "ANIMLIB_%s: Anim has not been initialized\n",funcname);
   }
//****************************************************************************
//
//      findpage ()
//              - given a frame number return the large page number it resides in
//
//****************************************************************************

uint16_t findpage (uint16_t framenumber)
   {
   uint16_t i;

   CheckAnimStarted ( "findpage" );
   for(i=0; i<anim->lpheader.nLps; i++)
      {
      if ( anim->LpArray[i].baseRecord <= framenumber && anim->LpArray[i].baseRecord + anim->LpArray[i].nRecords > framenumber)
         return(i);
      }
   return(i);
   }


//****************************************************************************
//
//      loadpage ()
//      - seek out and load in the large page specified
//
//****************************************************************************

void loadpage (uint16_t pagenumber, uint16_t *pagepointer)
{
#if MCU_DUKE
   UNUSED(pagepointer);
   int32_t size;
   byte * buffer;
   CheckAnimStarted ( "loadpage" );
   buffer = anim->buffer;
   if (anim->curlpnum != pagenumber)
   {
      anim->curlpnum = pagenumber;
      buffer += 0xb00 + (pagenumber*0x10000);
      size = sizeof(lp_descriptor);

      extMemGetDataFromAddress(&anim->curlp, buffer, size);
      //memcpy(&anim->curlp,buffer,size);
      buffer += size + sizeof(uint16_t);
      //memcpy(pagepointer,buffer,anim->curlp.nBytes+(anim->curlp.nRecords*2));
      anim->thepage = (uint16_t*) buffer;
   }
   if ((void *)anim->currentSubPagepointer != (void *) anim->thepage)
   {
      extMemGetDataFromAddress(anim->subpage, anim->thepage, ANIM_SUBPAGE_SIZE);
       anim->currentSubPagepointer = (uint8_t*) anim->thepage;
   }
#else
   int32_t size;
   byte * buffer;

   CheckAnimStarted ( "loadpage" );
   buffer = anim->buffer;
   if (anim->curlpnum != pagenumber)
  {
      anim->curlpnum = pagenumber;
      buffer += 0xb00 + (pagenumber*0x10000);
      size = sizeof(lp_descriptor);
      memcpy(&anim->curlp,buffer,size);
      buffer += size + sizeof(uint16_t);
      memcpy(pagepointer,buffer,anim->curlp.nBytes+(anim->curlp.nRecords*2));
  }

#endif
}
#if MCU_DUKE
uint8_t getByteFromPage(uint8_t *ptr)
{
    int pos = ptr - anim->currentSubPagepointer;
    if (pos >= ANIM_SUBPAGE_SIZE || pos < 0)
    {
        pos = ptr - (uint8_t*) anim->thepage;
        anim->currentSubPagepointer = ((uint8_t*)anim->thepage) + (pos & ~(ANIM_SUBPAGE_SIZE - 1));
        extMemGetDataFromAddress(anim->subpage, anim->currentSubPagepointer, ANIM_SUBPAGE_SIZE);
        pos = ptr - anim->currentSubPagepointer;
    }
    return  anim->subpage[pos];
}
#endif
//****************************************************************************
//
//      CPlayRunSkipDump ()
//      - This version of the decompressor is here for portability to non PC's
//
//****************************************************************************
static inline byte* drawFramePixels(byte **dstP, byte *frameP, byte *srcP, int32_t cnt)
{
    // copy pixel from source
    byte *dest = *dstP;
    int x, y;
    // calculate coordinates
    int bufferPosition = dest - frameP;
    x = bufferPosition % SCREENWIDTH;
    y = bufferPosition / SCREENWIDTH;
    // update destination
    *dstP = *dstP + cnt;
    // We want to directly draw the enlarged version.
    int newY, lastY;
    newY =  ((int)(y * (SCREENHEIGHT / 200.0f)));
    while (cnt)
    {

        int horizontalPixelInThisLine = SCREENWIDTH - x;
        if (horizontalPixelInThisLine > cnt)
        {
            horizontalPixelInThisLine = cnt;
        }
        cnt -= horizontalPixelInThisLine;
        lastY = newY;
        dest =  frameP + SCREENWIDTH * lastY  + x;
        byte * startdest = dest;
        int numPix = horizontalPixelInThisLine;
        while (numPix--)
        {

            *dest = getByteFromPage(srcP++);
            dest++;
        }
        y++;
        // calculate newY
        newY =  ((int)(y * (SCREENHEIGHT / 200.0f)));
        if (newY - lastY > 1)
        {
            int rows = newY - lastY - 1;
            while (rows--)
            {
                memcpy(startdest + SCREENWIDTH, startdest, horizontalPixelInThisLine);
                startdest += SCREENWIDTH;
            }
        }
        x = 0;  // for sure now x = 0, or there are no more pixels to draw;
    }
    // return new source address
    return srcP;
}
static inline void drawFrameColorRun(byte **dstP, byte *frameP, byte color, int32_t cnt)
{ // draws a run (many consecutive pixels) with the same color
    // copy pixel from source
    byte *dest = *dstP;
    int x, y;
    // calculate coordinates
    int bufferPosition = dest - frameP;
    x = bufferPosition % SCREENWIDTH;
    y = bufferPosition / SCREENWIDTH;
    // update destination
    *dstP = *dstP + cnt;
    // We want to directly draw the enlarged version.
    int newY, lastY;
    newY =  ((int)(y * (SCREENHEIGHT / 200.0f)));
    while (cnt)
    {

        int horizontalPixelInThisLine = SCREENWIDTH - x;
        if (horizontalPixelInThisLine > cnt)
        {
            horizontalPixelInThisLine = cnt;
        }
        cnt -= horizontalPixelInThisLine;
        lastY = newY;
        dest =  frameP + SCREENWIDTH * lastY  + x;
        byte * startdest = dest;
        int numPix = horizontalPixelInThisLine;
        while (numPix--)
        {

            *dest = color;
            dest++;
        }
        y++;
        // calculate newY
        newY =  ((int)(y * (SCREENHEIGHT / 200.0f)));
        if (newY - lastY > 1)
        {
            int rows = newY - lastY - 1;
            while (rows--)
            {
                memcpy(startdest + SCREENWIDTH, startdest, horizontalPixelInThisLine);
                startdest += SCREENWIDTH;
            }
        }
        x = 0;  // for sure now x = 0, or there are no more pixels to draw;
    }
}
void CPlayRunSkipDump (byte *srcP, byte *dstP)
{
#if MCU_DUKE | 1
    byte *frameP = dstP;
   int8_t cnt;
   uint16_t wordCnt;
   byte pixel;
nextOp:
   cnt = (int8_t ) getByteFromPage(srcP++);
   if (cnt > 0)
      goto dump;
   if (cnt == 0)
      goto run;
   cnt -= 0x80;
   if (cnt == 0)
      goto longOp;
/* shortSkip */
   dstP += cnt;                    /* adding 7-bit count to 32-bit pointer */
   goto nextOp;
dump:
#if 1
    srcP = drawFramePixels(&dstP, frameP, srcP, cnt);
#else
   do
      {
      *dstP++ = getByteFromPage(srcP++);
      } while (--cnt);
#endif
   goto nextOp;
run:
   wordCnt = getByteFromPage(srcP++);                /* 8-bit unsigned count */
   pixel = getByteFromPage(srcP++);
    #if 1
    drawFrameColorRun(&dstP, frameP, pixel, wordCnt);
    #else
   do
      {
      *dstP++ = pixel;
      } while (--wordCnt);
#endif

   goto nextOp;
longOp:
   wordCnt = getByteFromPage(srcP++);
   wordCnt |= getByteFromPage(srcP++) << 8;
   if ((int16_t)wordCnt <= 0)
      goto notLongSkip;       /* Do SIGNED test. */

/* longSkip. */
   dstP += wordCnt;
   goto nextOp;

notLongSkip:
   if (wordCnt == 0)
      goto stop;
   wordCnt -= 0x8000;              /* Remove sign bit. */
   if (wordCnt >= 0x4000)
      goto longRun;

/* longDump. */
#if 1
    srcP = drawFramePixels(&dstP, frameP, srcP, wordCnt);
#else
   do
      {
      *dstP++ = getByteFromPage(srcP++);
      } while (--wordCnt);
#endif
   goto nextOp;

longRun:
   wordCnt -= 0x4000;              /* Clear "longRun" bit. */
   pixel = getByteFromPage(srcP++);
#if 1
    drawFrameColorRun(&dstP, frameP, pixel, wordCnt);
#else
   do
      {
      *dstP++ = pixel;
      } while (--wordCnt);
#endif
   goto nextOp;

stop:   /* all done */
   ;


#else
   int8_t cnt;
   uint16_t wordCnt;
   byte pixel;
nextOp:
   cnt = (int8_t ) *srcP++;
   if (cnt > 0)
      goto dump;
   if (cnt == 0)
      goto run;
   cnt -= 0x80;
   if (cnt == 0)
      goto longOp;
/* shortSkip */
   dstP += cnt;                    /* adding 7-bit count to 32-bit pointer */
   goto nextOp;
dump:
   do
      {
      *dstP++ = *srcP++;
      } while (--cnt);
   goto nextOp;
run:
   wordCnt = (byte)*srcP++;                /* 8-bit unsigned count */
   pixel = *srcP++;
   do
      {
      *dstP++ = pixel;
      } while (--wordCnt);

   goto nextOp;
longOp:
   wordCnt = *((uint16_t *)srcP);
   srcP += sizeof(uint16_t);
   if ((int16_t)wordCnt <= 0)
      goto notLongSkip;       /* Do SIGNED test. */

/* longSkip. */
   dstP += wordCnt;
   goto nextOp;

notLongSkip:
   if (wordCnt == 0)
      goto stop;
   wordCnt -= 0x8000;              /* Remove sign bit. */
   if (wordCnt >= 0x4000)
      goto longRun;

/* longDump. */
   do
      {
      *dstP++ = *srcP++;
      } while (--wordCnt);
   goto nextOp;

longRun:
   wordCnt -= 0x4000;              /* Clear "longRun" bit. */
   pixel = *srcP++;
   do
      {
      *dstP++ = pixel;
      } while (--wordCnt);
   goto nextOp;

stop:   /* all done */
   ;

#endif // MCU_DUKE
}
#if MCU_DUKE
void dumpPages(void)
{
    uint8_t *pp = (uint8_t*) anim->thepage;
    uint8_t *sp = anim->subpage;
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 8; j++)
            printf("%02x %02x ", (*pp++) & 0xFF, (*sp++) & 0xFF);
        printf("\r\n");
    }
}
#endif
//****************************************************************************
//
//      renderframe ()
//      - draw the frame specified from the large page in the buffer pointed to
//
//****************************************************************************

void renderframe (uint16_t framenumber, uint16_t *pagepointer)
{
#if MCU_DUKE
   uint16_t offset=0;
   uint16_t i;
   uint16_t destframe;
   byte *ppointer;

   CheckAnimStarted ( "renderframe" );
   destframe = framenumber - anim->curlp.baseRecord;

   for(i = 0; i < destframe; i++)
   {
       uint16_t ofs = getByteFromPage((uint8_t *) &pagepointer[i]);
       ofs += (unsigned int) getByteFromPage(((byte*) (&pagepointer[i]) + 1)) << 8;
       offset += ofs;
   }

   ppointer = (byte *)pagepointer;

   int poffs = anim->curlp.nRecords*2+offset;
   ppointer+=poffs;

   if (getByteFromPage(&ppointer[1]))
   {
       // It seems it is never executed
       uint16_t value16;
       value16 = getByteFromPage(&ppointer[2]);
       value16 |= getByteFromPage(&ppointer[3]) << 8;
       ppointer += (4 + (value16 + (value16 & 1)));
   }
   else
      ppointer+=4;

   CPlayRunSkipDump (ppointer, anim->imagebuffer);

#else
   uint16_t offset=0;
   uint16_t i;
   uint16_t destframe;
   byte *ppointer;

   CheckAnimStarted ( "renderframe" );
   destframe = framenumber - anim->curlp.baseRecord;

   for(i = 0; i < destframe; i++)
      offset += pagepointer[i];

   ppointer = (byte *)pagepointer;

   ppointer+=anim->curlp.nRecords*2+offset;
   if(ppointer[1])
      ppointer += (4 + (((uint16_t *)ppointer)[1] + (((uint16_t *)ppointer)[1] & 1)));
   else
      ppointer+=4;

   CPlayRunSkipDump (ppointer, anim->imagebuffer);
#endif // MCU_DUKE
}


//****************************************************************************
//
//      drawframe ()
//      - high level frame draw routine
//
//****************************************************************************

void drawframe (uint16_t framenumber)
{
   CheckAnimStarted ( "drawframe" );

   loadpage(findpage(framenumber), anim->thepage);

   renderframe(framenumber, anim->thepage);
}


//****************************************************************************
//
//      ANIM_LoadAnim ()
//
//****************************************************************************

void ANIM_LoadAnim (byte * buffer)
{
#if MCU_DUKE
    uint16_t i;
    int32_t size;

    if (!Anim_Started) Anim_Started = true;
    anim->buffer = buffer;
    anim->curlpnum = 0xffff;
    anim->currentframe = -1;
    size = sizeof(lpfileheader);
    extMemGetDataFromAddress(&anim->lpheader, buffer, size);
    buffer += size+128;
    // load the color palette
  for (i = 0; i < 768; i += 3)
  {
      uint8_t bfr[4];
      extMemGetDataFromAddress(bfr, buffer, 4);
      buffer += 4;
      anim->pal[i+2] = bfr[0];
      anim->pal[i+1] = bfr[1];
      anim->pal[i] =   bfr[2];;

        /*anim->pal[i+2] = *buffer++;
        anim->pal[i+1] = *buffer++;
        anim->pal[i] = *buffer++;
        buffer++;*/
   }
    // read in large page descriptors
    size = sizeof(anim->LpArray);
    extMemGetDataFromAddress(&anim->LpArray, buffer,size);

#else
    uint16_t i;
    int32_t size;

    if (!Anim_Started) Anim_Started = true;

    anim->buffer = buffer;
    anim->curlpnum = 0xffff;
    anim->currentframe = -1;
    size = sizeof(lpfileheader);
    memcpy(&anim->lpheader, buffer, size );
    buffer += size+128;
    // load the color palette
    for (i = 0; i < 768; i += 3)
    {
        anim->pal[i+2] = *buffer++;
        anim->pal[i+1] = *buffer++;
        anim->pal[i] = *buffer++;
        buffer++;
    }
    // read in large page descriptors
    size = sizeof(anim->LpArray);
    memcpy(&anim->LpArray,buffer,size);
#endif // MCU_DUKE

}

//****************************************************************************
//
//      ANIM_FreeAnim ()
//
//****************************************************************************

void ANIM_FreeAnim ( void )
   {
   if (Anim_Started)
      {
//      SafeFree(anim);
      Anim_Started = false;
      }
   }

//****************************************************************************
//
//      ANIM_NumFrames ()
//
//****************************************************************************

int32_t ANIM_NumFrames ( void )
   {
   CheckAnimStarted ( "NumFrames" );
   return anim->lpheader.nRecords;
   }

//****************************************************************************
//
//      ANIM_DrawFrame ()
//
//****************************************************************************

byte * ANIM_DrawFrame (int32_t framenumber)
   {
   int32_t cnt;

   CheckAnimStarted ( "DrawFrame" );
   if ((anim->currentframe != -1) && (anim->currentframe<=framenumber))
      {
      for (cnt = anim->currentframe; cnt < framenumber; cnt++)
          drawframe (cnt);
      }
   else
      {
      for (cnt = 0; cnt < framenumber; cnt++)
         drawframe (cnt);
      }
   anim->currentframe = framenumber;
   return anim->imagebuffer;
   }

//****************************************************************************
//
//      ANIM_GetPalette ()
//
//****************************************************************************

byte * ANIM_GetPalette ( void )
   {
   CheckAnimStarted ( "GetPalette" );
   return anim->pal;
   }
