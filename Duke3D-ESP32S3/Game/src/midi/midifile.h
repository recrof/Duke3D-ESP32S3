//
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//     MIDI file parsing.
//

#ifndef MIDIFILE_H
#define MIDIFILE_H
#include <stdint.h>
#include <stdbool.h>

typedef struct midi_file_s midi_file_t;
typedef struct midi_track_iter_s midi_track_iter_t;
#define boolean uint8_t
#define byte uint8_t

#define MIDI_CHANNELS_PER_TRACK 16

typedef enum
{
    MIDI_EVENT_NOTE_OFF        = 0x80,  // not used by dn3d?
    MIDI_EVENT_NOTE_ON         = 0x90,   // used
    MIDI_EVENT_AFTERTOUCH      = 0xa0,   // not used
    MIDI_EVENT_CONTROLLER      = 0xb0,   // used
    MIDI_EVENT_PROGRAM_CHANGE  = 0xc0,   // used
    MIDI_EVENT_CHAN_AFTERTOUCH = 0xd0,    // not used
    MIDI_EVENT_PITCH_BEND      = 0xe0,   // used

    MIDI_EVENT_SYSEX           = 0xf0, // used
    MIDI_EVENT_SYSEX_SPLIT     = 0xf7,  // not used
    MIDI_EVENT_META            = 0xff,   // used
} midi_event_type_t;

typedef enum
{

    MIDI_CONTROLLER_BANK_SELECT = 0x00,
    MIDI_CONTROLLER_MODULATION_WHEEL = 0x01,
    MIDI_CONTROLLER_BREATH_CONTROLLER = 0x02,
    MIDI_CONTROLLER_FOOT_CONTROLLER = 0x04,
    MIDI_CONTROLLER_PORTAMENTO_TIME = 0x05,
    MIDI_CONTROLLER_DATA_ENTRY = 0x06,
    MIDI_CONTROLLER_VOLUME = 0x07,
    MIDI_CONTROLLER_BALANCE = 0x08,
    MIDI_CONTROLLER_PAN = 0x0A,
    MIDI_CONTROLLER_EXPRESSION_CONTROLLER = 0x0B,
    MIDI_CONTROLLER_EFFECT_CONTROL_1 = 0x0C,
    MIDI_CONTROLLER_EFFECT_CONTROL_2 = 0x0D,
    MIDI_CONTROLLER_GENERAL_PURPOSE_CONTROLLER_1 = 0x10,
    MIDI_CONTROLLER_GENERAL_PURPOSE_CONTROLLER_2 = 0x11,
    MIDI_CONTROLLER_GENERAL_PURPOSE_CONTROLLER_3 = 0x12,
    MIDI_CONTROLLER_GENERAL_PURPOSE_CONTROLLER_4 = 0x13,
    MIDI_CONTROLLER_DATA_ENTRY_LSB = 0x26,
    MIDI_CONTROLLER_SUSTAIN_PEDAL = 0x40,
    MIDI_CONTROLLER_PORTAMENTO = 0x41,
    MIDI_CONTROLLER_SOSTENUTO = 0x42,
    MIDI_CONTROLLER_SOFT_PEDAL = 0x43,
    MIDI_CONTROLLER_LEGATO_FOOTSWITCH = 0x44,
    MIDI_CONTROLLER_HOLD_2 = 0x45,
    MIDI_CONTROLLER_SOUND_CONTROLLER_1 = 0x46, // Sound Variation
    MIDI_CONTROLLER_SOUND_CONTROLLER_2 = 0x47, // Timbre/Harmonic Intensity
    MIDI_CONTROLLER_SOUND_CONTROLLER_3 = 0x48, // Release Time
    MIDI_CONTROLLER_SOUND_CONTROLLER_4 = 0x49, // Attack Time
    MIDI_CONTROLLER_SOUND_CONTROLLER_5 = 0x4A, // Brightness
    MIDI_CONTROLLER_SOUND_CONTROLLER_6 = 0x4B, // Decay Time
    MIDI_CONTROLLER_SOUND_CONTROLLER_7 = 0x4C, // Vibrato Rate
    MIDI_CONTROLLER_SOUND_CONTROLLER_8 = 0x4D, // Vibrato Depth
    MIDI_CONTROLLER_SOUND_CONTROLLER_9 = 0x4E, // Vibrato Delay
    MIDI_CONTROLLER_SOUND_CONTROLLER_10 = 0x4F, // Undefined
    MIDI_CONTROLLER_GENERAL_PURPOSE_CONTROLLER_5 = 0x50,
    MIDI_CONTROLLER_GENERAL_PURPOSE_CONTROLLER_6 = 0x51,
    MIDI_CONTROLLER_GENERAL_PURPOSE_CONTROLLER_7 = 0x52,
    MIDI_CONTROLLER_GENERAL_PURPOSE_CONTROLLER_8 = 0x53,
    MIDI_CONTROLLER_PORTAMENTO_CONTROL = 0x54,
    MIDI_CONTROLLER_EFFECTS_1_DEPTH = 0x5B, // Reverb Send Level
    MIDI_CONTROLLER_EFFECTS_2_DEPTH = 0x5C, // Tremolo Depth
    MIDI_CONTROLLER_EFFECTS_3_DEPTH = 0x5D, // Chorus Send Level
    MIDI_CONTROLLER_EFFECTS_4_DEPTH = 0x5E, // Celeste Depth
    MIDI_CONTROLLER_EFFECTS_5_DEPTH = 0x5F, // Phaser Depth
    MIDI_CONTROLLER_DATA_INCREMENT = 0x60,
    MIDI_CONTROLLER_DATA_DECREMENT = 0x61,
    MIDI_CONTROLLER_NRPN_LSB = 0x62, // Non-Registered Parameter Number (LSB)
    MIDI_CONTROLLER_NRPN_MSB = 0x63, // Non-Registered Parameter Number (MSB)
    MIDI_CONTROLLER_RPN_LSB = 0x64, // Registered Parameter Number (LSB)
    MIDI_CONTROLLER_RPN_MSB = 0x65, // Registered Parameter Number (MSB)
    MIDI_CONTROLLER_ALL_SOUND_OFF = 0x78,
    MIDI_CONTROLLER_RESET_ALL_CONTROLLERS = 0x79,
    MIDI_CONTROLLER_LOCAL_CONTROL = 0x7A,
    MIDI_CONTROLLER_ALL_NOTES_OFF = 0x7B,
    MIDI_CONTROLLER_OMNI_MODE_OFF = 0x7C,
    MIDI_CONTROLLER_OMNI_MODE_ON = 0x7D,
    MIDI_CONTROLLER_MONO_MODE_ON = 0x7E,
    MIDI_CONTROLLER_POLY_MODE_ON = 0x7F
} midi_controller_t;

typedef enum
{
    MIDI_META_SEQUENCE_NUMBER       = 0x0,

    MIDI_META_TEXT                  = 0x1,
    MIDI_META_COPYRIGHT             = 0x2,
    MIDI_META_TRACK_NAME            = 0x3,
    MIDI_META_INSTR_NAME            = 0x4,
    MIDI_META_LYRICS                = 0x5,
    MIDI_META_MARKER                = 0x6,
    MIDI_META_CUE_POINT             = 0x7,

    MIDI_META_CHANNEL_PREFIX        = 0x20,
    MIDI_META_PORT                  = 0x21,
    MIDI_META_END_OF_TRACK          = 0x2f,

    MIDI_META_SET_TEMPO             = 0x51,
    MIDI_META_SMPTE_OFFSET          = 0x54,
    MIDI_META_TIME_SIGNATURE        = 0x58,
    MIDI_META_KEY_SIGNATURE         = 0x59,
    MIDI_META_SEQUENCER_SPECIFIC    = 0x7f,
} midi_meta_event_type_t;

typedef struct
{
    // Meta event type:

    uint32_t type;

    // Length:

    uint32_t length;

    // Meta event data:

    uint8_t *data;
} midi_meta_event_data_t;

typedef struct
{
    // Length:

    unsigned int length;

    // Event data:

    uint8_t *data;
} midi_sysex_event_data_t;

typedef struct
{
    // The channel number to which this applies:

    unsigned int channel;

    // Extra parameters:

    unsigned int param1;
    unsigned int param2;
} midi_channel_event_data_t;

typedef struct
{
    // Time between the previous event and this event.
    uint32_t delta_time;

    // Type of event:
    midi_event_type_t event_type;

    union
    {
        midi_channel_event_data_t channel;
        midi_meta_event_data_t meta;
        midi_sysex_event_data_t sysex;
    } data;
} midi_event_t;


// haleyjd 09/09/10: packing required
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
#define PACKED_STRUCT(x)  struct x __attribute__((packed))
typedef PACKED_STRUCT (
{
    byte chunk_id[4];
    unsigned int chunk_size;
}) chunk_header_t;

typedef PACKED_STRUCT (
{
    chunk_header_t chunk_header;
    unsigned short format_type;
    unsigned short num_tracks;
    unsigned short time_division;
}) midi_header_t;

// haleyjd 09/09/10: packing off.
#ifdef _MSC_VER
#pragma pack(pop)
#endif

typedef struct
{
    // Length in bytes:

    unsigned int data_len;

    // Events in this track:

    midi_event_t *events;
    int num_events;
} midi_track_t;

struct midi_track_iter_s
{
    midi_track_t *track;
    unsigned int position;
};

struct midi_file_s
{
    midi_header_t header;

    // All tracks in this file:
    midi_track_t *tracks;
    unsigned int num_tracks;

    // Data buffer used to store data read for SysEx or meta events:
    byte *buffer;
    unsigned int buffer_size;
};

// Load a MIDI file.

midi_file_t *MIDI_LoadFile(char *filename);

// Free a MIDI file.

void MIDI_FreeFile(midi_file_t *file);

// Get the time division value from the MIDI header.

unsigned int MIDI_GetFileTimeDivision(midi_file_t *file);

// Get the number of tracks in a MIDI file.

unsigned int MIDI_NumTracks(midi_file_t *file);

// Start iterating over the events in a track.

midi_track_iter_t *MIDI_IterateTrack(midi_file_t *file, unsigned int track_num);

// Free an iterator.

void MIDI_FreeIterator(midi_track_iter_t *iter);

// Get the time until the next MIDI event in a track.

unsigned int MIDI_GetDeltaTime(midi_track_iter_t *iter);

// Get a pointer to the next MIDI event.

int MIDI_GetNextEvent(midi_track_iter_t *iter, midi_event_t **event);

// Reset an iterator to the beginning of a track.

void MIDI_RestartIterator(midi_track_iter_t *iter);

#endif /* #ifndef MIDIFILE_H */

