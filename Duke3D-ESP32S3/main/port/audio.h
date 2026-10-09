/*
 *  Audio output (ESP32-S3 port): I2S amplifier or codec.
 *
 *  The game mixer (multivoc.c) writes 8-bit unsigned stereo frames into the
 *  circular audioBuffer ahead of the play cursor. An I2S task drains the ring
 *  and publishes the cursor through audioGetPlayIndex().
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef SRC_PWM_AUDIO_H_
#define SRC_PWM_AUDIO_H_
#include <stdint.h>
#include "main.h"
#include "maincfg.h"
#include "macros.h"
#include "boards.h"
#define AUDIO_SAMPLE_RATE               11025
#define AUDIO_BUFFER_LENGTH             (1024)
#define AUDIO_BUFFER_DELAY              (200)
#define DMX_DATA_SOUND_OFFSET           0x18
void initAudio();
// Index (in stereo frames) of the frame the I2S task will output next.
uint32_t audioGetPlayIndex(void);
extern AUX_SECTION uint8_t audioBuffer[AUDIO_BUFFER_LENGTH * 2];
// Saturate to a signed n-bit value (replacement for the ARM __SSAT intrinsic).
#define __SSAT(val, bits) espSsat((val), (bits))
static inline int32_t espSsat(int32_t val, int bits)
{
    int32_t max = (1 << (bits - 1)) - 1;
    int32_t min = -(1 << (bits - 1));
    return val > max ? max : (val < min ? min : val);
}
#endif /* SRC_PWM_AUDIO_H_ */
