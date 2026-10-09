#include "nh_midi.h"
#include "_al_midi.h"
#include <stdint.h>
#include <stddef.h>
#include "extMemory.h"
#include "internalFlash.h"
#include "maincfg.h"
#include "al_midi.h"
#include "opl2.h"
#define EMIDI_END_LOOP_VALUE    127  // not used.
#define EMIDI_INCLUDE_TRACK     110
#define EMIDI_EXCLUDE_TRACK     111
#define EMIDI_PROGRAM_CHANGE    112 // used 2 times in total, in all the songs.
#define EMIDI_VOLUME_CHANGE     113 // not used.
#define EMIDI_CONTEXT_START     114 // not used
#define EMIDI_CONTEXT_END       115 // not used
#define EMIDI_LOOP_START        116  //used
#define EMIDI_LOOP_END          117 //used
#define EMIDI_SONG_LOOP_START   118 // used
#define EMIDI_SONG_LOOP_END     119 // used

void setMusicParameters(int offset, int tempo, int tracksize);
#if WIN32
#include <stdio.h>
#endif // WIN32
#if PRINT_DEBUG
    #define dbgprintf printf
#else
    #define dbgprintf(...)
#endif
#define MAX_MID_CHANNELS 16
#if MG24
#include "filesystem.h"
#endif
//
uint8_t* currentMusicDataPtr = 0;  // address (in external flash!) of the current midi data
//
// Data associated with a channel of a track that is currently playing.
typedef struct
{
    // The instrument index currently used for this track.
    uint8_t instrument;
    // Volume level
    uint8_t note_volume;
    uint8_t volume;
    uint8_t volume_base;
    // Pan - Removed, we put the music as mono.
    //int8_t pan;
    // Pitch bend value:
    int8_t pitchBendHundreds;
    int8_t pitchBendSemiTones;
//    int8_t bend;
    uint16_t rpn;

} mid_channel_t;
int readVariableMusicEvent(void);
mid_channel_t channels[MAX_MID_CHANNELS];
static uint32_t musicLoopStart = 0;
static uint32_t musicLoopCount = 0;
static uint32_t currentSampleIndex = 0;       // amount of samples already outputted for the song
static uint32_t nextEventSampleIndex = 0;       // when the next event shall be generated             //
static uint32_t musicReadIndex = 0;        // the index of the music.
static uint32_t numberOfReadMusicEvents = 0;
static uint8_t loopStartSet = 1;
static int trackDataSize = 0;
#if WIN32
uint8_t musicEventBuffer[1024*1024];
#else
uint8_t musicEventBuffer[64] __attribute__ ((aligned(4)));
#endif
uint8_t musicDataOffset = 0;
float midiTimePerQuarterNote;
float midiInvPulsesPerQuarterNote;
#define RENDER_SAMPLE_RATE 11025
#if WIN32
TIMBRE ADLIB_TimbreBank[256];
#else
const TIMBRE *ADLIB_TimbreBank;
#endif
#if WIN32
void loadTimbres(char *filename)
{
    #if WIN32
        FILE *f = fopen(filename, "rb");
        if (f)
        {
            int n;
            n = fread(ADLIB_TimbreBank, 256, sizeof(TIMBRE) , f);
            printf("Read %d timbres\r\n", n);
            fclose(f);
        }
    #endif // WIN32
}
#else
void MUSIC_RegisterTimbreBank(void * tmb)
{
    ADLIB_TimbreBank = storeToInternalFlash(tmb, sizeof(TIMBRE) * 256 );
}
#endif
void loadtmb(char *name)
{
    #if !WIN32
        TIMBRE tmb[256];
        uint8_t *p = (uint8_t*) kGetExtMemAddress(name, 0);
        if (!p)
            return;
        // get data
        extMemGetDataFromAddress(&tmb, p, sizeof(tmb));
        // register (store to internal flash)
        MUSIC_RegisterTimbreBank(&tmb);
    #else
    // we must mimic storage to internal flash for profiling purposes
        uint8_t buf[13 * 256];
        storeToInternalFlash(buf, sizeof(buf));
    #endif
}

static uint8_t readMusicEvent(void)
{
 uint8_t v;
  if (musicReadIndex < numberOfReadMusicEvents)
  {
    v = musicEventBuffer[musicReadIndex + sizeof(musicEventBuffer) - numberOfReadMusicEvents]; // note: order matters!
  }
  else
  { // we don't have any more data
#if !WIN32
    // note: near the end of the song, it will happen almost 100% of the cases that we read last piece of music + garbage
    // We rely on the fact that the music has an end at the end...
      numberOfReadMusicEvents = musicReadIndex;
      extMemSetCurrentAddress((uint32_t) currentMusicDataPtr + numberOfReadMusicEvents);
      numberOfReadMusicEvents += sizeof(musicEventBuffer);
      extMemGetDataFromCurrentAddress(musicEventBuffer, sizeof(musicEventBuffer));
#endif
      v = musicEventBuffer[musicReadIndex + sizeof(musicEventBuffer) - numberOfReadMusicEvents];
  }
  musicReadIndex++;
  return v;
}
static void resetMusicCounters(void)
{
    currentSampleIndex = 0;
    musicReadIndex = 0;
#if !WIN32
    numberOfReadMusicEvents = 0;
#endif
    nextEventSampleIndex = readVariableMusicEvent() * RENDER_SAMPLE_RATE *  midiTimePerQuarterNote * midiInvPulsesPerQuarterNote;           // we must read the first delta time!

}
#if WIN32
#include <stdio.h>
void preloadMusic(FILE *f, int size )
{
    fread(musicEventBuffer, 1, size, f);
    printf("preloading %d bytes\r\n", size);
    for (int i = 0; i < 1024; i++)
    {
        if (i % 16 == 0)
        {
            printf("\r\n");
        }
        printf("0x%02x ", musicEventBuffer[i]);
    }
    printf("\r\n");
    numberOfReadMusicEvents = sizeof(musicEventBuffer);
}
#else
static uint8_t musicPlaying = 0;
int isMusicPlaying(void)
{
    return musicPlaying;
}

void MUSIC_StopSong(void)
{
    musicPlaying = 0;
}
// TODO FIXME: handle properly pause.
void  MUSIC_Continue( void )
{
    if (currentMusicDataPtr)
        musicPlaying = 1;
}
void  MUSIC_Pause( void )
{
    musicPlaying = 0;
}
int PlayMusic(char *name)
{
    uint8_t* p = kGetExtMemAddress(name, NULL);//FILE *f = fopen(fileList[choosen],"rb");
    if (!p)
        return 0;
    midi_header_t hdr;
    extMemSetCurrentAddress((uint32_t) p);
    extMemGetDataFromCurrentAddress(&hdr, sizeof(hdr));
    hdr.chunk_header.chunk_size = swap_be_le32(hdr.chunk_header.chunk_size);
    hdr.time_division = swap_be_le16(hdr.time_division);
    chunk_header_t trackChunk;
    // read track chunk
    extMemGetDataFromCurrentAddress(&trackChunk, sizeof(trackChunk));
    trackChunk.chunk_size = swap_be_le32(trackChunk.chunk_size);
    setMusicParameters(8 + 6 + 8, hdr.time_division, trackChunk.chunk_size);
    currentMusicDataPtr = p + sizeof(hdr) + sizeof(trackChunk);
    resetMusicCounters();
    musicPlaying = 1;
    loopStartSet = 0;

    return 1;
}
#endif // WIN32

static void resetToLoopStart(void)
{
    currentSampleIndex = 0;
    musicReadIndex = musicLoopStart;
#if !WIN32
    numberOfReadMusicEvents = 0;//musicLoopStart -1 ;
#endif
    nextEventSampleIndex = readVariableMusicEvent() * RENDER_SAMPLE_RATE *  midiTimePerQuarterNote * midiInvPulsesPerQuarterNote;           // we must read the first delta time!

}

int readVariableMusicEvent(void)
{
    int v = 0;
    int i;
        dbgprintf("Variable Length: bytes ");

    for (i = 0; i < 4; i++)
    {
        uint8_t tmp =  readMusicEvent();
        dbgprintf("0x%x ",tmp);
        v = v * 128 + (tmp & 0x7F);
        if ((tmp & 0x80) == 0)
        {
            break;
        }
    }
    dbgprintf("--->  Value: %d\r\n", v);
    return v;
}
void setMusicParameters(int offset, int tempo, int tracksize)
{
    loopStartSet = 0;
    musicDataOffset = offset;
    midiInvPulsesPerQuarterNote  = 1.0f / tempo;
    trackDataSize = tracksize;
    resetMusicCounters();
}
static void seekBackEventCounter(void)
{
    if (musicReadIndex)
        musicReadIndex--;
}
void _MIDI_SysEx()
{
    printf("midi sysex");
}
void _MIDI_MetaEvent()
{
    printf("Midi meta event");
}
static uint8_t lasteventDescriptor = 0;
void generateMusicOutput(int32_t *destBuffer, uint32_t howManySamples)
{
    for (uint32_t i = 0; i < howManySamples;)
    {
        bool endOfTrack = false;
        // let's process all the concurrent events
        while (nextEventSampleIndex == currentSampleIndex)
        {
            dbgprintf("Sample pos %i\r\n", i);
            // we need to read the event.
            uint8_t eventdescriptor = readMusicEvent();
            //printf("position %i, event 0x%x, musicReadIndex is %d\r\n", i, eventdescriptor, musicReadIndex);
            if (!(eventdescriptor & 0x80))
            {
                dbgprintf("position %i, event 0x%x < 0x80. musicReadIndex is: %d\r\n", i, eventdescriptor, musicReadIndex);
                eventdescriptor = lasteventDescriptor;
                seekBackEventCounter();
            }
            lasteventDescriptor = eventdescriptor;
            uint8_t channel = eventdescriptor & 0x0F;
            uint8_t event = eventdescriptor & 0xF0;
            uint8_t param, param2;
#if DEBUG && PRINT_DEBUG
            static const char *eventString[256] = {"Release", "press", "pitch", "system", "controller", "UNDEF", "END"};
            dbgprintf("At %x, Sample Index: %d (%f s), Read: 0x%02x, Evt 0x%02x ch %d %s\n", musicReadIndex - 1, currentSampleIndex, currentSampleIndex / 1.0f / RENDER_SAMPLE_RATE, eventdescriptor, event, channel, eventString[event >> 4]);
#endif
            switch (event)
            {
                case MIDI_EVENT_SPECIAL:
                    switch(eventdescriptor)
                    {
                        case MIDI_EVENT_SYSEX:
                        case MIDI_EVENT_SYSEX_SPLIT:
                            dbgprintf("special event 0x%x\r\n", eventdescriptor);
                            {
                                int length = readVariableMusicEvent();
                                for (int i = 0; i < length; i++)
                                    readMusicEvent();       // just ignore next bytes.
                            }
                            break;
                        case MIDI_EVENT_META:
                        {
                            param = readMusicEvent();
                            int length = readVariableMusicEvent();
                            #define MAX_MIDI_META_DATA 8
                            uint8_t data[MAX_MIDI_META_DATA];
                            for (int i = 0; i < length; i++)
                            {
                                uint8_t b = readMusicEvent();
                                if (i < MAX_MIDI_META_DATA)
                                {
                                    data[i] = b;
                                }
                            }
                            switch(param)
                            {
                                case    MIDI_META_END_OF_TRACK:
                                    resetMusicCounters();
                                    endOfTrack = true;
                                    dbgprintf("EOT\r\n");
                                    break;
                                case  MIDI_META_SET_TEMPO:
                                    midiTimePerQuarterNote = ((data[0] << 16) | (data[1] << 8) | (data[2])) / 1000000.0f;
                                    dbgprintf(">>>>>>>>>>>>>>>>>Tempo is %f\r\n", midiTimePerQuarterNote);
                                    //FIXME("SET TEMPO\r\n");
                                    break;
                                case MIDI_META_TIME_SIGNATURE:
                                    break;

                                case MIDI_META_KEY_SIGNATURE:
                                    break;

                                    break;
                                case     MIDI_META_MARKER:
                                    // ignore
                                    break;
                                case    MIDI_META_TEXT:
                                case     MIDI_META_COPYRIGHT:
                                case     MIDI_META_TRACK_NAME:
                                case     MIDI_META_INSTR_NAME:
                                case     MIDI_META_LYRICS:
                                case     MIDI_META_CUE_POINT:
                                case        MIDI_META_PORT:
                                    // ignore
                                    break;
                                default: // just ignore
                                    FIXME("Unknown meta %d\r\n", param);
                                    break;
                            }
                            break;
                        }
                                default:
                                    FIXME("ERROR");
                    }
                    break;
                case MIDI_EVENT_NOTE_OFF:
                    param = readMusicEvent();
                    param2 = readMusicEvent();
                    AL_NoteOff(channel, param, param2);
                    //FIXME("NOTE OFF");
                    break;
                case MIDI_EVENT_NOTE_ON:
                    param = readMusicEvent();
                    param2 = readMusicEvent();
                    AL_NoteOn(channel, param, param2);
                    //dbgprintf(">>>>>>>>>>>>>>Note on!\r\n");
                    break;
                case MIDI_EVENT_CHAN_AFTERTOUCH:
                    FIXME("ERROR");
                    break;
                case MIDI_EVENT_AFTERTOUCH:
                    FIXME("ERROR");
                    break;
                case MIDI_EVENT_CONTROLLER:

                    param = readMusicEvent();
                    param2 = readMusicEvent();
                    switch (param)
                    {
                        default:
                            AL_ControlChange(channel, param, param2);
                            break;
                            case EMIDI_END_LOOP_VALUE:
                                dbgprintf("EMIDI_END_LOOP_VALUE\n");
                                break;
                            case EMIDI_INCLUDE_TRACK:
                                dbgprintf("EMIDI_INCLUDE_TRACK\n");
                                break;
                            case EMIDI_EXCLUDE_TRACK:
                                dbgprintf("EMIDI_EXCLUDE_TRACK\n");
                                break;
                            case EMIDI_PROGRAM_CHANGE:
                                AL_ProgramChange( channel, param2 & 0x7f);  // why was created this command, anyway ?
                                dbgprintf("EMIDI_PROGRAM_CHANGE\n");
                                break;
                            case EMIDI_VOLUME_CHANGE:
                                dbgprintf("EMIDI_VOLUME_CHANGE\n");
                                break;
                            case EMIDI_CONTEXT_START:
                                dbgprintf("EMIDI_CONTEXT_START\n");
                                break;
                            case EMIDI_CONTEXT_END:
                                dbgprintf("EMIDI_CONTEXT_END\n");
                                break;
                            case EMIDI_LOOP_START:
                            case EMIDI_SONG_LOOP_START:
                                if (!loopStartSet)
                                {
                                    loopStartSet = 1;
                                    musicLoopStart = musicReadIndex;
                                    musicLoopCount = param2;
                                    if (param2 == 0)
                                        musicLoopCount = 0xFFFFFFFF;
                                }
                                dbgprintf("EMIDI_SONG_LOOP_START\n");
                                break;
                            case EMIDI_SONG_LOOP_END:
                            case EMIDI_LOOP_END:
                                if (musicLoopCount > 0)
                                {
                                    musicLoopCount--;
                                    resetToLoopStart();
                                    endOfTrack = true;
                                }

                                dbgprintf("EMIDI_LOOP_END\n");

                                break;

                    }
                    break;
                case MIDI_EVENT_PITCH_BEND:
                    param = readMusicEvent();
                    param2 = readMusicEvent();
                    AL_SetPitchBend(channel, param, param2);
                    break;
                case MIDI_EVENT_PROGRAM_CHANGE:
                    param = readMusicEvent(); // controller number
                    AL_ProgramChange(channel, param);
                    break;

                default:
                    FIXME("ERROR! FOUND UNKNOWN EVENT %d, at position %d\r\n", event, musicReadIndex);
                    break;
            } // end switch event
            // read time delay  code
            if (!endOfTrack)
            {
                dbgprintf("reading delta time\r\n");
                uint32_t timedelay = 0;
                timedelay = readVariableMusicEvent();
                dbgprintf("delay is %d. midiTimePerQuarterNote is %f and midiInvPulsesPerQuarterNote is %f\r\n", timedelay, midiTimePerQuarterNote, midiInvPulsesPerQuarterNote);
                float delta = timedelay * RENDER_SAMPLE_RATE * midiTimePerQuarterNote * midiInvPulsesPerQuarterNote;
                dbgprintf("Delta: %f %i\r\n", delta, (int) delta);
                nextEventSampleIndex += delta;
            }
            else
            {
                break;
            }
        }//end while
        if (nextEventSampleIndex > currentSampleIndex)      // if is <= then it was end of song
        {
            uint32_t newSamplesToGenerate = nextEventSampleIndex - currentSampleIndex;
            if (newSamplesToGenerate > howManySamples - i)
            {
                newSamplesToGenerate = howManySamples - i;
            }
            //dbgprintf("Generating %d new samples\r\n", newSamplesToGenerate);
            OPL2_GenerateStream(destBuffer + i, newSamplesToGenerate);
            i += newSamplesToGenerate;
            currentSampleIndex += newSamplesToGenerate;
        }
    }
}
#if !WIN32
int MUSIC_Init( int SoundCard, int Address )
{
    UNUSED(SoundCard);
    UNUSED(Address);
    OPL2_Reset(RENDER_SAMPLE_RATE);
    AL_Init(0);
    OPL2_InitRegisters(0);
    return 0;
}
#endif
