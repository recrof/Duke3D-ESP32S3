#ifndef NH_MIDI_H
#define NH_MIDI_H
#include <stdint.h>
#include <stdbool.h>
#define MIDI_CHANNELS_PER_TRACK 16
#define MIDI_EVENT_SPECIAL 0xf0
#include "midifile.h"
int isMusicPlaying(void);
void generateMusicOutput(int32_t *destBuffer, uint32_t howManySamples);
static inline uint16_t swap_be_le16(uint16_t v)
{
    return (v << 8) | (v >> 8);
}
static inline uint32_t swap_be_le32(uint32_t value)
{
 return ((value >> 24) & 0xff) |
           ((value << 8) & 0xff0000) |
           ((value >> 8) & 0xff00) |
           ((value << 24) & 0xff000000);
}
#endif // NH_MIDI_H
