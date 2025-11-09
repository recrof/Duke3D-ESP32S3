#ifndef MAINCFG_H
#define MAINCFG_H
#if !WIN32
#include "printf.h"
#else
#include <stdio.h>
#include <stdlib.h>
#endif
#include "macros.h"
#include "ext_memory.h"
// The code is full of fallthrough cases...
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
//
#define ATOMIC_EDITION                  1
#define LOAD_FROM_CON_FILE              0
#define MAX_STATIC_ZONE                 1048576
#define LOAD_TABLES                     0
#define PRINT_TABLES                    1
#define USE_EXT_MEM                     1
#define LOAD_ONLY_GRP                   1       // this replaces hardcoded 0, when actually grp is used.
#define NEW_MAXSPRITES                  1       // this is because we are using a non power of 2 MAXSPRITES.
#define MAX_SPRITESMASK                 (2047)  //
#define MCU_DUKE                        1
#define ANIM_SUBPAGE_SIZE               1024
#define INTERPOLATION_USES_POSPTR       0
#define MCU_TEST                        1
#define CUSTOM_FTA                      255
#define MAX_FTA_CUSTOM_TEXT_SIZE        64             //  more than one line.
#define RAM_FTA_QUOTES                  0
#define CVARTYPE                        uint8_t       // why do we need int?
#define STACK_PVWALLS                   1
#define LASTX_IN_STACK                  1               // if 1, camera view does not work
#define ERROR_ON_MCU_TODO               1
#define REUSE_ARRAYS                    0           // allow reusing big arrays (e.g. ramsprites) outside game rendering (e.g. anim)
#define INTERPOLATION_HAS_BAKIPOS       0
#define CLOUD_SUPPORT                   ATOMIC_EDITION // I can't find any map with clouds in original. Only on ATOMIC_EDITION
#define USE_SCRIPTLIB                   0
#define USE_MANY_MODES                  0
#define ALLOW_LOW_DETAIL                0
#define SPRITES_USE_DOUBLY_LINKED_LIST  0
#define LETS_WASTE_RAM_FOR_USERDEFS     0
#define OPTIMIZED_HITTYPE_TEMPDATA      1
#define FREQUENT_FAKETIMERHANDLER       0
#define SPRITETYPE_HAS_OFFSETS          1
#define USE_STATIC_LOOKUP_TABLES        1
#define DRAW_CODE_CLEANUP               1
#define STATUS_BAR_HEIGHT               40    // can't be arbitrary, we are always sending 256 pixels.
#define MAX_COL_SIZE                    512
#if WIN32
#define SCREEN_WIDTH                    320
#define SCREEN_HEIGHT                   240
#define DRAW_BUFFER_HEIGHT              240
#endif
#define MAX_VOICES                      8
#define VOICE_LINKED_LIST               0
#define MAX_RAM_FLAT_SIZE               8192
#define AUX_SECTION                     // nothing. We are not using FRCRAM in DN3D
#define MAXPLAYERORIGINS                16
#define MAXPLAYERS                      4
//
#if !WIN32
#define MG24                            1
#define DISABLE_COMM_DURING_LOAD        1
#endif
//
#define MAX_WORDS_PER_LINE              16
#define MAX_SHORTS_PER_LINE             32
#define MAX_BYTES_PER_LINE              64
#define SCREENHEIGHT                    240
#define SCREENWIDTH                     320
#define BIG_TILT_TILE                   0
#define SCREEN_TILT_TILE_SIZE           160     // for rotation
#define USE_SEPARATE_HITTYPE_TEMPDATA   1
#define USE_RAM_HITTYPE                 1
#define FIX_EARTHQUAKE                  0       // kill "randomscrap" sprites as soon as they are inside the wall.
#define USE_BLOCKMAP                    1       // will speed up updateSector
#if WIN32
#define FIXME(...) do {printf("FIXME AT LINE: %d, file %s, function %s\r\n", __LINE__, __FILE__, __FUNCTION__); printf (__VA_ARGS__); system("pause");} while(0)
#define WARNING(...) do {printf("WARNING AT LINE: %d, file %s, function %s\r\n", __LINE__, __FILE__, __FUNCTION__); printf (__VA_ARGS__); } while(0)
#define EXIT(n) do {printf("EXIT %d AT LINE: %d, file %s, function %s\r\n", n, __LINE__, __FILE__, __FUNCTION__); system("pause"); exit(n);} while(0)
#else
#define FIXME(...) do {printf("FIXME AT LINE: %d, file %s, function %s\r\n", __LINE__, __FILE__, __FUNCTION__); printf (__VA_ARGS__); ;} while(0)
#define WARNING(...) do {printf("WARNING AT LINE: %d, file %s, function %s\r\n", __LINE__, __FILE__, __FUNCTION__); printf (__VA_ARGS__); } while(0)
#define EXIT(n) do {printf("EXIT %d AT LINE: %d, file %s, function %s\r\n", n, __LINE__, __FILE__, __FUNCTION__); while(1);} while(0)

#endif
#define CHECK_SPRITE(s) do { if (((s) & ~0xC000) >= MAXSPRITES) FIXME("TOO LARGE\r\n"); }while(0)
//
#define PRINT_ARRAY_SZ(name, type, size) \
 do  \
 { \
    printf("const %s %s[%d] =\r\n{", #type, #name, size);\
    for (int i = 0; i < size; i++) \
    { \
        switch (sizeof(type)) \
        { \
            case 1:\
                if (i % MAX_BYTES_PER_LINE == 0) printf("\r\n    ");\
                printf("0x%02x, ", name[i]);\
                break;\
            case 2:\
                if (i % MAX_SHORTS_PER_LINE == 0) printf("\r\n    ");\
                printf("0x%04x, ", name[i] & 0xFFFF);\
                break;\
            case 4:\
                if (i % MAX_WORDS_PER_LINE == 0) printf("\r\n    ");\
                printf("0x%08x, ", name[i]);\
                break;\
        }\
    }\
    printf("\r\n};\r\n");\
 } while(0)
//
#define PRINT_ARRAY(name, type) \
 do  \
 { \
    PRINT_ARRAY_SZ(name, type, sizeof(name) / sizeof(name[0]));\
 } while(0)

//
#define PRINT_DOUBLE_ARRAY(name, type, size1, size2) \
 do  \
 { \
    printf("const %s %s[%d][%d] =\r\n{\r\n", #type, #name, size1, size2);\
    for (int j = 0; j < size1; j++)\
    {\
        printf("    {\r\n");\
        for (int i = 0; i < size2; i++) \
        { \
            switch (sizeof(type)) \
            { \
                case 1:\
                    if (i % MAX_BYTES_PER_LINE == 0) printf("\r\n        ");\
                    printf("0x%02x, ", name[j][i]);\
                    break;\
                case 2:\
                    if (i % MAX_SHORTS_PER_LINE == 0) printf("\r\n        ");\
                    printf("0x%04x, ", name[j][i] & 0xFFFF);\
                    break;\
                case 4:\
                    if (i % MAX_WORDS_PER_LINE == 0) printf("\r\n        ");\
                    printf("0x%08x, ", name[j][i]);\
                    break;\
            }\
        }\
        printf("\r\n    },\r\n");\
    }\
    printf("\r\n};\r\n");\
 } while(0)
#endif // MAINCFG_H
