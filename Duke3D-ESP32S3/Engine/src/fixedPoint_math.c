// converted from asm to c by Jonof

#ifdef WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
#include <string.h>
#include "platform.h"
#include "fixedPoint_math.h"
void clearbufshort(void *d, int32_t c, int16_t a)
{
	int16_t *p = (int16_t*)d;
	while ((c--) > 0) *(p++) = a;
}
void clearbuf(void *d, int32_t c, int32_t a)
{

#if 1
    // tanguyf: this completely violates strict-aliasing.
  // next-hack: restored back, it's faster and since we are not going to support 1800 big-endian machines, who cares.
	int32_t *p = (int32_t*)d;
	while ((c--) > 0) *(p++) = a;
#else
	union
    {
	    struct { uint8_t a,b,c,d; };
	    int32_t value;
    } src;

	src.value = a;

	uint8_t* dst = (uint8_t*) d;

	for (int32_t i = 0; i < c; ++i)
    {
	    dst[i*4 + 0] = src.a;
        dst[i*4 + 1] = src.b;
        dst[i*4 + 2] = src.c;
        dst[i*4 + 3] = src.d;
    }
#endif
}
// not called ?
void clearbufShort(short *d, int32_t c, int32_t a)
{
    for (int i = 0; i < c; i++)
        *d++ = a;
}

void clearbufbyte(void *D, int32_t c, int32_t a)
{ // Cringe City
#if 1
  uint32_t *p = D;
  (void) a;
  while (c > 4)
  {
    *p++ = a;
    c-= 4;
  }
  // then store a, byte by byte, every time shifting by one byte.
  uint8_t *pb = (uint8_t*) p;
  while (c > 0)
  {
    *pb++ = a;  // this automatically takes LSB
    a = a >> 8;  // right shift to get next byte
    c--;
  }
#else
	uint8_t  *p = (uint8_t *)D;
	int32_t m[4] = { 0xffl,0xff00l,0xff0000l,0xff000000l };
	int32_t n[4] = { 0,8,16,24 };
	int32_t z=0;
	while ((c--) > 0) {
		*(p++) = (uint8_t )((a & m[z])>>n[z]);
		z=(z+1)&3;
	}
#endif

}

void copybuf(const void *s, void *d, int32_t c)
{
    // tanguyf: this completely violates strict-aliasing.
	// next-hack: but memcpy is implemented in the silly slow way.
  int32_t *p = (int32_t*)s, *q = (int32_t*)d;
	while ((c--) > 0) *(q++) = *(p++);
//	memcpy(d, s, c * sizeof(int32_t));
}

void copybufbyte(const void *S, void *D, int32_t c)
{
#if 0
	uint8_t  *p = (uint8_t *)S, *q = (uint8_t *)D;
	while((c--) > 0) *(q++) = *(p++);
#else
  uint32_t *p = D;
  const uint32_t *ps = S;
  while (c > 4)
  {
    *p++ = *ps++;
    c-= 4;
  }
  uint8_t *pb = (uint8_t*) p;
  uint8_t *psb = (uint8_t *) ps;
  while (c > 0)
  {
    *pb++ = *psb++;
    c--;
  }
#endif
}
/*
void fastmemcpy(void *d, void *s, int32_t c)
{
  uint32_t *p = d;
  uint32_t *ps = s;
  while (c > 4)
  {
    *p++ = *ps++;
    c-= 4;
  }
  uint8_t *pb = (uint8_t*) p;
  uint8_t *psb = (uint8_t *) ps;
  while (c > 0)
  {
    *pb++ = *psb++;
    c--;
  }
}*/

void copybufreverse(const void *S, void *D, int32_t c)
{
	uint8_t  *p = (uint8_t *)S, *q = (uint8_t *)D;
	while((c--) > 0) *(q++) = *(p--);
}

void qinterpolatedown16(int32_t* bufptr, int32_t num, int32_t val, int32_t add)
{ // gee, I wonder who could have provided this...
    int32_t i, *lptr = bufptr;
    for(i=0;i<num;i++) { lptr[i] = (val>>16); val += add; }
}

void qinterpolatedown16short(short* bufptr, int32_t num, int32_t val, int32_t add)
{ // ...maybe the same person who provided this too?
    int32_t i; short *sptr = (short *)bufptr;
    for(i=0;i<num;i++) { sptr[i] = (short)(val>>16); val += add; }
}
void qinterpolatedown16byte(uint8_t * bufptr, int32_t num, int32_t val, int32_t add)
{ // ...maybe the same person who provided this too?
    int32_t i; uint8_t *sptr = (uint8_t *)bufptr;
    for(i=0;i<num;i++) { sptr[i] = (uint8_t)(val>>16); val += add; }
}


