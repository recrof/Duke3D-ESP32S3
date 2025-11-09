/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
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

*/
/**********************************************************************
   file:   MULTIVOC.H

   author: James R. Dose
   date:   December 20, 1993

   Public header for MULTIVOC.C

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef __MULTIVOC_H
#define __MULTIVOC_H
#include <stdint.h>
//#include <windows.h>
#ifdef WIN32
#include <SDL2/SDL.h>
#endif
#define MV_FLOAT float
// forward declare
//struct SDL_mutex;


#define MV_MinVoiceHandle  1

extern int MV_ErrorCode;

enum MV_Errors
   {
   MV_Warning = -2,
   MV_Error   = -1,
   MV_Ok      = 0,
   MV_UnsupportedCard,
   MV_NotInstalled,
   MV_NoVoices,
   MV_NoMem,
   MV_VoiceNotFound,
   MV_BlasterError,
   MV_PasError,
   MV_SoundScapeError,
   MV_SoundSourceError,
   MV_DPMI_Error,
   MV_InvalidVOCFile,
   MV_InvalidWAVFile,
   MV_InvalidMixMode,
   MV_SoundSourceFailure,
   MV_IrqFailure,
   MV_DMAFailure,
   MV_DMA16Failure,
   MV_NullRecordFunction
   };

char *MV_ErrorString( int ErrorNumber );
int   MV_VoicePlaying( int handle );
int   MV_KillAllVoices( void );
int   MV_Kill( int handle );
int   MV_VoicesPlaying( void );
int   MV_VoiceAvailable( int priority );
int   MV_SetPan( int handle, int vol, int left, int right );
int   MV_Pan3D( int handle, int angle, int distance );
void  MV_SetReverb( int reverb );
int   MV_GetMaxReverbDelay( void );
void  MV_SetReverbDelay( int delay );
int   MV_SetMixMode( int numchannels, int samplebits );
int   MV_StartPlayback( void );
void  MV_StopPlayback( void );
int   MV_PlayWAV( uint8_t *ptr, int pitchoffset, int vol, int left,
         int right, int priority, unsigned long soundNum );
int   MV_PlayWAV3D( uint8_t *ptr, int pitchoffset, int angle, int distance,
         int priority, unsigned long soundNum );
int   MV_PlayLoopedWAV( uint8_t *ptr, long loopstart, long loopend,
         int pitchoffset, int vol, int left, int right, int priority,
         unsigned long soundNum );
#if USE_VOC
int   MV_PlayVOC3D( uint8_t *ptr, int pitchoffset, int angle, int distance,
         int priority, unsigned long soundNum );
int   MV_PlayVOC( uint8_t * ptr, int pitchoffset, int vol, int left, int right,
         int priority, unsigned long soundNum );
int   MV_PlayLoopedVOC( uint8_t *ptr, long loopstart, long loopend,
         int pitchoffset, int vol, int left, int right, int priority,
         uint32_t soundNum );
#endif
void  MV_CreateVolumeTable( int index, int volume, int MaxVolume );
void  MV_SetVolume( int volume );
int   MV_GetVolume( void );
void  MV_SetCallBack( void ( *function )( int32_t ) );
void  MV_SetReverseStereo( int setting );
int   MV_GetReverseStereo( void );
int   MV_Init( int soundcard, int MixRate, int Voices, int numchannels,
         int samplebits );
int   MV_Shutdown( void );

void waitForVoiceEnd(int voiceNum, int maxDelay, void (*f)(void)); // next-hack: wait for voice to end and in between calls f() if not null.
void setSoundNum(int sound, unsigned int num);
int getSoundNum(int sound);
int getNumSoundForOwner(int i);
int getSoundOwnerFromSoundNum(int soundNum);
int getVoiceSoundOwner(int voice);
int getVoiceSoundNum(int voice);
void  setVoiceSoundOwner(int voice, int owner);
//CRITICAL_SECTION reverbCS;
#ifdef WIN32
extern SDL_mutex* reverbMutex;
#endif
#endif
