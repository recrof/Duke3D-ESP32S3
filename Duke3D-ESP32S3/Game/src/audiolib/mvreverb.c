#include "multivoc.h"
#include "_multivc.h"
#if WIN32
#ifdef WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
extern MV_FLOAT *MV_FooBuffer;
extern int MV_BufferSize;
extern int MV_SampleSize;
extern int MV_ReverbDelay;
extern int MV_MixRate;
extern int MV_Channels;

static MV_FLOAT * reverbBuffer = 0;
static int delay = 0, CurrAddr;

static	int FB_SRC_A, FB_SRC_B, IIR_DEST_A0, IIR_DEST_A1, ACC_SRC_A0, ACC_SRC_A1, ACC_SRC_B0,
		ACC_SRC_B1, IIR_SRC_A0, IIR_SRC_A1, IIR_DEST_B0, IIR_DEST_B1, ACC_SRC_C0,
		ACC_SRC_C1, ACC_SRC_D0, ACC_SRC_D1, IIR_SRC_B1, IIR_SRC_B0, MIX_DEST_A0,
		MIX_DEST_A1, MIX_DEST_B0, MIX_DEST_B1;
	
static	MV_FLOAT IIR_ALPHA, ACC_COEF_A, ACC_COEF_B, ACC_COEF_C, ACC_COEF_D, IIR_COEF, FB_ALPHA, FB_X,
		IN_COEF_L, IN_COEF_R;

static  MV_FLOAT iRVBLeft, iRVBRight;

static int cnv_offset(int src)
{
	int64_t temp = ((int64_t)src * (int64_t)MV_MixRate) / 22050;
	return (int)temp;
}

// static char err[256];

// extern __stdcall OutputDebugStringA(char *);

static void check_buffer()
{
	int new_delay = cnv_offset(MV_ReverbDelay);

	if (!delay || new_delay != delay)
	{
		printf("new_delay: %d, MV_ReverbDelay: %d\r\n", new_delay, MV_ReverbDelay);
		FB_SRC_A = cnv_offset(0xE3);
		FB_SRC_B = cnv_offset(0xA9);
		IIR_DEST_A0 = cnv_offset(0xDFB);
		IIR_DEST_A1 = cnv_offset(0xB58);
		ACC_SRC_A0 = cnv_offset(0xD09);
		ACC_SRC_A1 = cnv_offset(0xA3C);
		ACC_SRC_B0 = cnv_offset(0xBD9);
		ACC_SRC_B1 = cnv_offset(0x973);
		IIR_SRC_A0 = cnv_offset(0xB59);
		IIR_SRC_A1 = cnv_offset(0x8DA);
		IIR_DEST_B0 = cnv_offset(0x8D9);
		IIR_DEST_B1 = cnv_offset(0x5E9);
		ACC_SRC_C0 = cnv_offset(0x7EC);
		ACC_SRC_C1 = cnv_offset(0x4B0);
		ACC_SRC_D0 = cnv_offset(0x6EF);
		ACC_SRC_D1 = cnv_offset(0x3D2);
		IIR_SRC_B1 = cnv_offset(0x5EA);
		IIR_SRC_B0 = cnv_offset(0x31D);
		MIX_DEST_A0 = cnv_offset(0x31C);
		MIX_DEST_A1 = cnv_offset(0x238);
		MIX_DEST_B0 = cnv_offset(0x154);
		MIX_DEST_B1 = cnv_offset(0xAA);
		IIR_ALPHA = 0.8701171875f;
		ACC_COEF_A = 0.622314453125f;
		ACC_COEF_B = -0.5244140625f;
		ACC_COEF_C = 0.53955078125f;
		ACC_COEF_D = -0.50830078125f;
		IIR_COEF = -0.69921875f;
		FB_ALPHA = 0.67578125f;
		FB_X = 0.646484375f;
		IN_COEF_L = -2.0f;
		IN_COEF_R = -2.0f;
		if (reverbBuffer) reverbBuffer = (MV_FLOAT*) realloc(reverbBuffer, new_delay * sizeof(MV_FLOAT));
		else reverbBuffer = (MV_FLOAT*) malloc(new_delay * sizeof(MV_FLOAT));
		memset(reverbBuffer, 0, new_delay * sizeof(MV_FLOAT));
		delay = new_delay;
		CurrAddr = 0;
	}

}

MV_FLOAT g_buffer(int iOff, MV_FLOAT *ptr)                          // get_buffer content helper: takes care about wraps
{
	int correctDelay = delay;
	if(!correctDelay)
	{
		printf("Error! Reverb race on g_buffer\n");
		correctDelay = cnv_offset(14320);
	}

	iOff=(iOff*4)+CurrAddr;
	while(iOff>correctDelay-1)	
	{
		iOff=iOff-correctDelay;
	}
	while(iOff<0)
	{
		iOff=correctDelay-(0-iOff);
	}
	return (MV_FLOAT)*(ptr+iOff);
}

void s_buffer(int iOff,MV_FLOAT iVal, MV_FLOAT *ptr)                // set_buffer content helper: takes care about wraps and clipping
{
	int correctDelay = delay;
	if(!correctDelay)
	{
		printf("Error! Reverb race on s_buffer\n");
		correctDelay = cnv_offset(14320);
	}

	iOff=(iOff*4)+CurrAddr;
	while(iOff>correctDelay-1)
	{
		iOff=iOff-correctDelay;
	}
	while(iOff<0)
	{
		iOff=correctDelay-(0-iOff);
	}
	*(ptr+iOff)=iVal;
}

void s_buffer1(int iOff,MV_FLOAT iVal, MV_FLOAT *ptr)                // set_buffer (+1 sample) content helper: takes care about wraps and clipping
{
	int correctDelay = delay;
	if(!correctDelay)
	{
		printf("Error! Reverb race on s_buffer1\n");
		correctDelay = cnv_offset(14320);
	}

	iOff=(iOff*4)+CurrAddr+1;
	while(iOff>correctDelay-1)	
	{
		iOff=iOff-correctDelay;
	}
	while(iOff<0)			
	{
		iOff=correctDelay-(0-iOff);
	}
	*(ptr+iOff)=iVal;
}

MV_FLOAT MixREVERBLeft(MV_FLOAT INPUT_SAMPLE_L, MV_FLOAT INPUT_SAMPLE_R, MV_FLOAT *ptr)
{
	MV_FLOAT ACC0,ACC1,FB_A0,FB_A1,FB_B0,FB_B1;
	
	const MV_FLOAT IIR_INPUT_A0 = (g_buffer(IIR_SRC_A0, ptr) * IIR_COEF) + (INPUT_SAMPLE_L * IN_COEF_L);
	const MV_FLOAT IIR_INPUT_A1 = (g_buffer(IIR_SRC_A1, ptr) * IIR_COEF) + (INPUT_SAMPLE_R * IN_COEF_R);
	const MV_FLOAT IIR_INPUT_B0 = (g_buffer(IIR_SRC_B0, ptr) * IIR_COEF) + (INPUT_SAMPLE_L * IN_COEF_L);
	const MV_FLOAT IIR_INPUT_B1 = (g_buffer(IIR_SRC_B1, ptr) * IIR_COEF) + (INPUT_SAMPLE_R * IN_COEF_R);

	const MV_FLOAT IIR_A0 = (IIR_INPUT_A0 * IIR_ALPHA) + (g_buffer(IIR_DEST_A0, ptr) * (1.f - IIR_ALPHA));
	const MV_FLOAT IIR_A1 = (IIR_INPUT_A1 * IIR_ALPHA) + (g_buffer(IIR_DEST_A1, ptr) * (1.f - IIR_ALPHA));
	const MV_FLOAT IIR_B0 = (IIR_INPUT_B0 * IIR_ALPHA) + (g_buffer(IIR_DEST_B0, ptr) * (1.f - IIR_ALPHA));
	const MV_FLOAT IIR_B1 = (IIR_INPUT_B1 * IIR_ALPHA) + (g_buffer(IIR_DEST_B1, ptr) * (1.f - IIR_ALPHA));
	
	s_buffer1(IIR_DEST_A0, IIR_A0, ptr);
	s_buffer1(IIR_DEST_A1, IIR_A1, ptr);
	s_buffer1(IIR_DEST_B0, IIR_B0, ptr);
	s_buffer1(IIR_DEST_B1, IIR_B1, ptr);
	
	ACC0 = (g_buffer(ACC_SRC_A0, ptr) * ACC_COEF_A) +
		(g_buffer(ACC_SRC_B0, ptr) * ACC_COEF_B) +
		(g_buffer(ACC_SRC_C0, ptr) * ACC_COEF_C) +
		(g_buffer(ACC_SRC_D0, ptr) * ACC_COEF_D);
	ACC1 = (g_buffer(ACC_SRC_A1, ptr) * ACC_COEF_A) +
		(g_buffer(ACC_SRC_B1, ptr) * ACC_COEF_B) +
		(g_buffer(ACC_SRC_C1, ptr) * ACC_COEF_C) +
		(g_buffer(ACC_SRC_D1, ptr) * ACC_COEF_D);
	
	FB_A0 = g_buffer(MIX_DEST_A0 - FB_SRC_A, ptr);
	FB_A1 = g_buffer(MIX_DEST_A1 - FB_SRC_A, ptr);
	FB_B0 = g_buffer(MIX_DEST_B0 - FB_SRC_B, ptr);
	FB_B1 = g_buffer(MIX_DEST_B1 - FB_SRC_B, ptr);
	
	s_buffer(MIX_DEST_A0, ACC0 - (FB_A0 * FB_ALPHA), ptr);
	s_buffer(MIX_DEST_A1, ACC1 - (FB_A1 * FB_ALPHA), ptr);
	
	s_buffer(MIX_DEST_B0, (FB_ALPHA * ACC0) - (FB_A0 * (FB_ALPHA - 1.f)) - (FB_B0 * FB_X), ptr);
	s_buffer(MIX_DEST_B1, (FB_ALPHA * ACC1) - (FB_A1 * (FB_ALPHA - 1.f)) - (FB_B1 * FB_X), ptr);
	
	iRVBLeft  = (g_buffer(MIX_DEST_A0, ptr)+g_buffer(MIX_DEST_B0, ptr))/3.f;
	iRVBRight = (g_buffer(MIX_DEST_A1, ptr)+g_buffer(MIX_DEST_B1, ptr))/3.f;
	
	CurrAddr++;
	if(CurrAddr>delay-1) CurrAddr=0;
	
	return (MV_FLOAT)iRVBLeft;
}

MV_FLOAT MixREVERBRight(void)
{
	return (MV_FLOAT)iRVBRight;
}

void MV_FPReverb(int volume)
{
	int i, count = MV_BufferSize / MV_SampleSize * MV_Channels;

//	sprintf(err, "count: %d, old_delay: %d", count, delay);
	//EnterCriticalSection(&reverbCS);
#ifdef WIN32
	SDL_mutexP(reverbMutex);
#endif
	check_buffer();

	// DAVE
	if(delay == 0)
	{
		//get out now!!!
		printf("Error! MV_FPReverb() delay==0\n");
		return;
	}

//	OutputDebugStringA(err);
#if USE_VOC
	if (MV_Channels == 1)
	{
		for (i = 0; i < count; i++)
		{
			MV_FLOAT temp = MV_FooBuffer[i];
			MV_FooBuffer[i] += ((MixREVERBLeft(temp, temp, reverbBuffer) + MixREVERBRight()) * .5f) * (MV_FLOAT)volume / (MV_FLOAT)MV_MaxVolume;
		}
	}
	else
#endif
	{
		count >>= 1;
		for (i = 0; i < count; i++)
		{
			MV_FLOAT left = MV_FooBuffer[i*2];
			MV_FLOAT right = MV_FooBuffer[i*2+1];
			MV_FLOAT scale = (MV_FLOAT)volume * (1.0f / (MV_FLOAT)MV_MaxVolume);
			left += MixREVERBLeft(left, right, reverbBuffer) * scale;
			right += MixREVERBRight() * scale;
			MV_FooBuffer[i*2] = left;
			MV_FooBuffer[i*2+1] = right;
		}
	}

	//LeaveCriticalSection(&reverbCS);
#ifdef WIN32
	SDL_mutexV(reverbMutex);
#endif
}

void MV_FPReverbFree(void)
{
#ifdef WIN32
	SDL_mutexP(reverbMutex);
#endif
	//EnterCriticalSection(&reverbCS);
	delay = 0;
	if (reverbBuffer)
	{
		free(reverbBuffer);
		reverbBuffer = 0;
	}
	//LeaveCriticalSection(&reverbCS);
#ifdef WIN32
	SDL_mutexV(reverbMutex);
#endif
}
#if USE_VOC
void MV_16BitDownmix(uint8_t *dest, int count)
{
	int i;

	short *pdest = (short *)dest;

	for (i = 0; i < count; i++)
	{
		int out = (int)((MV_FooBuffer[i] * (MV_FLOAT)0x8000));
		if (out < -32768) pdest[i] = -32768;
		else if (out > 32767) pdest[i] = 32767;
		else pdest[i] = out;
	}
}
void MV_16BitReverbFast( const uint8_t*src, uint8_t*dest, int count, int shift )
{
	int i;

	short *pdest = (short *)dest;
	const short *psrc = (const short *)src;

	for (i = 0; i < count; i++) {
		pdest[i] = psrc[i] >> shift;
	}
}

#endif
void MV_8BitDownmix(uint8_t*dest, int count)
{
	int i;

	for (i = 0; i < count; i++)
	{
		int out = ((int)((MV_FooBuffer[i] * (MV_FLOAT)0x80)));
		if (out < -128) dest[i] = 0;
		else if (out > 127) dest[i] = 255;
		else dest[i] = out + 0x80;
	}
}



void MV_8BitReverbFast( const int8_t *src, int8_t *dest, int count, int shift )
{
	int i;

	unsigned char sh = 0x80 - (0x80 >> shift);
	
	for (i = 0; i < count; i++) {
		unsigned char a = ((unsigned char) src[i]) >> shift;
		unsigned char c = (((unsigned char) src[i]) ^ 0x80) >> 7;
		
		dest[i] = (signed char) (a + sh + c);
	}
}
#endif
