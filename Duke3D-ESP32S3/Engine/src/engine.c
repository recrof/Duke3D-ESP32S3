/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */
/* SUPERBUILD define is in engine.h ... */
#include "graphics.h"
#include "maincfg.h"
#define NO_FPU 0
#if MG24
#include "i_memory.h"
#endif
#include "internalFlash.h"
#include "ext_memory.h"
#define ENGINE
#define MAX_COL_BUF (MAX_COL_SIZE + 8)  // +8 for aligned reads.
#if WIN32
#include "io.h"
#endif
#define CACHE_PALETTE 1
#if CACHE_PALETTE
#define MAX_PALETTE_CACHE_ROWS 1
#if MAX_PALETTE_CACHE_ROWS > 1
uint32_t lastPaletteUsedTime[MAX_PALETTE_CACHE_ROWS];
uint32_t paletteTime = 0;
uint8_t cachedPalette[MAX_PALETTE_CACHE_ROWS][256];
uint8_t *currentCachedPalettePointer[MAX_PALETTE_CACHE_ROWS];
#else
static uint8_t cachedPalette[256];
static const uint8_t *currentCachedPalettePointer = NULL;
#endif
#endif
#include <fcntl.h>
#include <math.h>
#if WIN32
#include <stdio.h>
#else
#include "printf.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "platform.h"
#include "build.h"
#include "engine.h"
#include "tiles.h"
#include "names.h"
#include "duke3d.h"
#if MG24
#include "radio.h"
#endif
//walltype walls[MAXWALLS];
//const_walltype constwall[MAXWALLS];
//
const sectortype *sectors; //[MAXSECTORS];
const const_sectortype *constsector; //[MAXSECTORS];
//
static const uint8_t *bufferedTileImage;
static uint8_t *tempRamTileData;
void setTempRamTileData(uint8_t *buffer)
{
    tempRamTileData = buffer;
}
void clearBufferedTileImagePtr(void)
{
    bufferedTileImage = NULL;
}

int8_t wallshade[MAXWALLS];
walltype ramwalls[MAXRAMWALLS];
sectortype ramsectors[MAXRAMSECTORS];
#if HITAG_CONSTANT
uint8_t wall_hitag9999[(MAXWALLS  + 7 )/ 8 ];        // 1 if hitag is 9999
#endif
uint8_t wall_extraIsMinusOne[(MAXWALLS  + 7 )/ 8 ];
#if USE_STEREO_VIDEO
int32_t stereowidth = 23040, stereopixelwidth = 28, ostereopixelwidth = -1;
int32_t stereomode = 0, visualpage, activepage, whiteband, blackband;
#endif
//
#if USE_STATIC_LOOKUP_TABLES
const int32_t horizlookup[960] =
{
    0xfff772e8, 0xfff76e54, 0xfff769bb, 0xfff7651c, 0xfff76079, 0xfff75bd1, 0xfff75724, 0xfff75271, 0xfff74dba, 0xfff748fd, 0xfff7443b, 0xfff73f74, 0xfff73aa8, 0xfff735d7, 0xfff73100, 0xfff72c24,
    0xfff72742, 0xfff7225b, 0xfff71d6f, 0xfff7187d, 0xfff71386, 0xfff70e89, 0xfff70986, 0xfff7047e, 0xfff6ff70, 0xfff6fa5d, 0xfff6f544, 0xfff6f025, 0xfff6eb00, 0xfff6e5d5, 0xfff6e0a5, 0xfff6db6e,
    0xfff6d632, 0xfff6d0ef, 0xfff6cba7, 0xfff6c658, 0xfff6c103, 0xfff6bba8, 0xfff6b647, 0xfff6b0e0, 0xfff6ab72, 0xfff6a5fe, 0xfff6a084, 0xfff69b03, 0xfff6957b, 0xfff68fee, 0xfff68a59, 0xfff684be,
    0xfff67f1c, 0xfff67974, 0xfff673c5, 0xfff66e0f, 0xfff66852, 0xfff6628e, 0xfff65cc4, 0xfff656f2, 0xfff65119, 0xfff64b3a, 0xfff64553, 0xfff63f64, 0xfff6396f, 0xfff63372, 0xfff62d6e, 0xfff62763,
    0xfff62150, 0xfff61b36, 0xfff61514, 0xfff60eea, 0xfff608b9, 0xfff60280, 0xfff5fc3f, 0xfff5f5f6, 0xfff5efa6, 0xfff5e94d, 0xfff5e2ed, 0xfff5dc84, 0xfff5d614, 0xfff5cf9b, 0xfff5c919, 0xfff5c290,
    0xfff5bbfe, 0xfff5b564, 0xfff5aec1, 0xfff5a815, 0xfff5a161, 0xfff59aa4, 0xfff593df, 0xfff58d10, 0xfff58639, 0xfff57f58, 0xfff5786f, 0xfff5717d, 0xfff56a81, 0xfff5637c, 0xfff55c6e, 0xfff55556,
    0xfff54e35, 0xfff5470a, 0xfff53fd5, 0xfff53897, 0xfff53150, 0xfff529fe, 0xfff522a2, 0xfff51b3c, 0xfff513cd, 0xfff50c53, 0xfff504ce, 0xfff4fd40, 0xfff4f5a7, 0xfff4ee03, 0xfff4e655, 0xfff4de9c,
    0xfff4d6d9, 0xfff4cf0a, 0xfff4c731, 0xfff4bf4c, 0xfff4b75d, 0xfff4af62, 0xfff4a75c, 0xfff49f4a, 0xfff4972d, 0xfff48f05, 0xfff486d0, 0xfff47e90, 0xfff47644, 0xfff46dec, 0xfff46588, 0xfff45d18,
    0xfff4549b, 0xfff44c12, 0xfff4437c, 0xfff43ada, 0xfff4322b, 0xfff4296f, 0xfff420a7, 0xfff417d1, 0xfff40eee, 0xfff405fe, 0xfff3fd00, 0xfff3f3f4, 0xfff3eadb, 0xfff3e1b5, 0xfff3d880, 0xfff3cf3d,
    0xfff3c5ed, 0xfff3bc8e, 0xfff3b320, 0xfff3a9a4, 0xfff3a019, 0xfff39680, 0xfff38cd7, 0xfff38320, 0xfff37959, 0xfff36f83, 0xfff3659d, 0xfff35ba8, 0xfff351a3, 0xfff3478e, 0xfff33d69, 0xfff33334,
    0xfff328ee, 0xfff31e98, 0xfff31431, 0xfff309b9, 0xfff2ff30, 0xfff2f497, 0xfff2e9eb, 0xfff2df2e, 0xfff2d460, 0xfff2c980, 0xfff2be8d, 0xfff2b389, 0xfff2a872, 0xfff29d48, 0xfff2920c, 0xfff286bd,
    0xfff27b5b, 0xfff26fe5, 0xfff2645c, 0xfff258c0, 0xfff24d0f, 0xfff2414a, 0xfff23571, 0xfff22984, 0xfff21d82, 0xfff2116b, 0xfff2053f, 0xfff1f8fd, 0xfff1eca6, 0xfff1e039, 0xfff1d3b6, 0xfff1c71d,
    0xfff1ba6d, 0xfff1ada7, 0xfff1a0ca, 0xfff193d5, 0xfff186c9, 0xfff179a6, 0xfff16c6a, 0xfff15f16, 0xfff151aa, 0xfff14425, 0xfff13687, 0xfff128d0, 0xfff11b00, 0xfff10d15, 0xfff0ff10, 0xfff0f0f1,
    0xfff0e2b8, 0xfff0d463, 0xfff0c5f3, 0xfff0b768, 0xfff0a8c0, 0xfff099fd, 0xfff08b1d, 0xfff07c20, 0xfff06d05, 0xfff05dce, 0xfff04e78, 0xfff03f04, 0xfff02f72, 0xfff01fc1, 0xfff00ff1, 0xfff00000,
    0xffefeff0, 0xffefdfc0, 0xffefcf6f, 0xffefbefc, 0xffefae69, 0xffef9db3, 0xffef8cda, 0xffef7bdf, 0xffef6ac1, 0xffef597f, 0xffef481a, 0xffef368f, 0xffef24e0, 0xffef130b, 0xffef0110, 0xffeeeeef,
    0xffeedca8, 0xffeeca38, 0xffeeb7a1, 0xffeea4e2, 0xffee91fa, 0xffee7ee8, 0xffee6bad, 0xffee5847, 0xffee44b6, 0xffee30fa, 0xffee1d11, 0xffee08fc, 0xffedf4b9, 0xffede049, 0xffedcbaa, 0xffedb6dc,
    0xffeda1de, 0xffed8cb0, 0xffed7750, 0xffed61bf, 0xffed4bfc, 0xffed3605, 0xffed1fdb, 0xffed097c, 0xffecf2e7, 0xffecdc1d, 0xffecc51c, 0xffecade4, 0xffec9673, 0xffec7ec8, 0xffec66e4, 0xffec4ec5,
    0xffec366b, 0xffec1dd4, 0xffec04ff, 0xffebebec, 0xffebd29a, 0xffebb908, 0xffeb9f35, 0xffeb851f, 0xffeb6ac7, 0xffeb502a, 0xffeb3548, 0xffeb1a20, 0xffeafeb0, 0xffeae2f9, 0xffeac6f7, 0xffeaaaab,
    0xffea8e13, 0xffea712e, 0xffea53fb, 0xffea3678, 0xffea18a5, 0xffe9fa7f, 0xffe9dc06, 0xffe9bd38, 0xffe99e14, 0xffe97e98, 0xffe95ec4, 0xffe93e94, 0xffe91e09, 0xffe8fd20, 0xffe8dbd8, 0xffe8ba2f,
    0xffe89824, 0xffe875b4, 0xffe852de, 0xffe82fa1, 0xffe80bfb, 0xffe7e7e8, 0xffe7c369, 0xffe79e7a, 0xffe7791b, 0xffe75347, 0xffe72cff, 0xffe7063f, 0xffe6df05, 0xffe6b750, 0xffe68f1c, 0xffe66667,
    0xffe63d2f, 0xffe61372, 0xffe5e92d, 0xffe5be5c, 0xffe592ff, 0xffe56711, 0xffe53a90, 0xffe50d7a, 0xffe4dfca, 0xffe4b17f, 0xffe48294, 0xffe45307, 0xffe422d5, 0xffe3f1f9, 0xffe3c071, 0xffe38e39,
    0xffe35b4d, 0xffe327aa, 0xffe2f34b, 0xffe2be2c, 0xffe2884a, 0xffe251a0, 0xffe21a2a, 0xffe1e1e2, 0xffe1a8c6, 0xffe16ecf, 0xffe133f9, 0xffe0f83f, 0xffe0bb9b, 0xffe07e08, 0xffe03f81, 0xffe00000,
    0xffdfbf7f, 0xffdf7df8, 0xffdf3b65, 0xffdef7be, 0xffdeb2fe, 0xffde6d1e, 0xffde2616, 0xffddddde, 0xffdd9470, 0xffdd49c4, 0xffdcfdd0, 0xffdcb08e, 0xffdc61f3, 0xffdc11f8, 0xffdbc091, 0xffdb6db7,
    0xffdb195f, 0xffdac37e, 0xffda6c0a, 0xffda12f7, 0xffd9b83a, 0xffd95bc7, 0xffd8fd90, 0xffd89d8a, 0xffd83ba7, 0xffd7d7d8, 0xffd77210, 0xffd70a3e, 0xffd6a053, 0xffd6343f, 0xffd5c5f1, 0xffd55556,
    0xffd4e25c, 0xffd46cf0, 0xffd3f4fe, 0xffd37a70, 0xffd2fd30, 0xffd27d28, 0xffd1fa40, 0xffd1745e, 0xffd0eb67, 0xffd05f42, 0xffcfcfd0, 0xffcf3cf4, 0xffcea68e, 0xffce0c7d, 0xffcd6e9f, 0xffcccccd,
    0xffcc26e3, 0xffcb7cb8, 0xffcace22, 0xffca1af3, 0xffc962fd, 0xffc8a60e, 0xffc7e3f2, 0xffc71c72, 0xffc64f53, 0xffc57c58, 0xffc4a340, 0xffc3c3c4, 0xffc2dd9d, 0xffc1f07d, 0xffc0fc10, 0xffc00000,
    0xffbefbf0, 0xffbdef7c, 0xffbcda3b, 0xffbbbbbc, 0xffba9387, 0xffb9611b, 0xffb823ef, 0xffb6db6e, 0xffb586fc, 0xffb425ee, 0xffb2b78d, 0xffb13b14, 0xffafafb0, 0xffae147b, 0xffac687e, 0xffaaaaab,
    0xffa8d9e0, 0xffa6f4df, 0xffa4fa50, 0xffa2e8bb, 0xffa0be83, 0xff9e79e8, 0xff9c18fa, 0xff99999a, 0xff96f970, 0xff9435e6, 0xff914c1c, 0xff8e38e4, 0xff8af8b0, 0xff878788, 0xff83e0f9, 0xff800000,
    0xff7bdef8, 0xff777778, 0xff72c235, 0xff6db6dc, 0xff684bdb, 0xff627628, 0xff5c28f6, 0xff555556, 0xff4de9be, 0xff45d175, 0xff3cf3d0, 0xff333334, 0xff286bcb, 0xff1c71c8, 0xff0f0f10, 0xff000000,
    0xfeeeeeef, 0xfedb6db7, 0xfec4ec4f, 0xfeaaaaab, 0xfe8ba2e9, 0xfe666667, 0xfe38e38f, 0xfe000000, 0xfdb6db6e, 0xfd555556, 0xfccccccd, 0xfc000000, 0xfaaaaaab, 0xf8000000, 0xf0000000, 0x00000000,
    0x10000000, 0x08000000, 0x05555555, 0x04000000, 0x03333333, 0x02aaaaaa, 0x02492492, 0x02000000, 0x01c71c71, 0x01999999, 0x01745d17, 0x01555555, 0x013b13b1, 0x01249249, 0x01111111, 0x01000000,
    0x00f0f0f0, 0x00e38e38, 0x00d79435, 0x00cccccc, 0x00c30c30, 0x00ba2e8b, 0x00b21642, 0x00aaaaaa, 0x00a3d70a, 0x009d89d8, 0x0097b425, 0x00924924, 0x008d3dcb, 0x00888888, 0x00842108, 0x00800000,
    0x007c1f07, 0x00787878, 0x00750750, 0x0071c71c, 0x006eb3e4, 0x006bca1a, 0x00690690, 0x00666666, 0x0063e706, 0x00618618, 0x005f417d, 0x005d1745, 0x005b05b0, 0x00590b21, 0x00572620, 0x00555555,
    0x00539782, 0x0051eb85, 0x00505050, 0x004ec4ec, 0x004d4873, 0x004bda12, 0x004a7904, 0x00492492, 0x0047dc11, 0x00469ee5, 0x00456c79, 0x00444444, 0x004325c5, 0x00421084, 0x00410410, 0x00400000,
    0x003f03f0, 0x003e0f83, 0x003d2263, 0x003c3c3c, 0x003b5cc0, 0x003a83a8, 0x0039b0ad, 0x0038e38e, 0x00381c0e, 0x003759f2, 0x00369d03, 0x0035e50d, 0x003531de, 0x00348348, 0x0033d91d, 0x00333333,
    0x00329161, 0x0031f383, 0x00315972, 0x0030c30c, 0x00303030, 0x002fa0be, 0x002f1499, 0x002e8ba2, 0x002e05c0, 0x002d82d8, 0x002d02d0, 0x002c8590, 0x002c0b02, 0x002b9310, 0x002b1da4, 0x002aaaaa,
    0x002a3a0f, 0x0029cbc1, 0x00295fad, 0x0028f5c2, 0x00288df0, 0x00282828, 0x0027c459, 0x00276276, 0x00270270, 0x0026a439, 0x002647c6, 0x0025ed09, 0x002593f6, 0x00253c82, 0x0024e6a1, 0x00249249,
    0x00243f6f, 0x0023ee08, 0x00239e0d, 0x00234f72, 0x00230230, 0x0022b63c, 0x00226b90, 0x00222222, 0x0021d9ea, 0x002192e2, 0x00214d02, 0x00210842, 0x0020c49b, 0x00208208, 0x00204081, 0x00200000,
    0x001fc07f, 0x001f81f8, 0x001f4465, 0x001f07c1, 0x001ecc07, 0x001e9131, 0x001e573a, 0x001e1e1e, 0x001de5d6, 0x001dae60, 0x001d77b6, 0x001d41d4, 0x001d0cb5, 0x001cd856, 0x001ca4b3, 0x001c71c7,
    0x001c3f8f, 0x001c0e07, 0x001bdd2b, 0x001bacf9, 0x001b7d6c, 0x001b4e81, 0x001b2036, 0x001af286, 0x001ac570, 0x001a98ef, 0x001a6d01, 0x001a41a4, 0x001a16d3, 0x0019ec8e, 0x0019c2d1, 0x00199999,
    0x001970e4, 0x001948b0, 0x001920fb, 0x0018f9c1, 0x0018d301, 0x0018acb9, 0x001886e5, 0x00186186, 0x00183c97, 0x00181818, 0x0017f405, 0x0017d05f, 0x0017ad22, 0x00178a4c, 0x001767dc, 0x001745d1,
    0x00172428, 0x001702e0, 0x0016e1f7, 0x0016c16c, 0x0016a13c, 0x00168168, 0x001661ec, 0x001642c8, 0x001623fa, 0x00160581, 0x0015e75b, 0x0015c988, 0x0015ac05, 0x00158ed2, 0x001571ed, 0x00155555,
    0x00153909, 0x00151d07, 0x00150150, 0x0014e5e0, 0x0014cab8, 0x0014afd6, 0x00149539, 0x00147ae1, 0x001460cb, 0x001446f8, 0x00142d66, 0x00141414, 0x0013fb01, 0x0013e22c, 0x0013c995, 0x0013b13b,
    0x0013991c, 0x00138138, 0x0013698d, 0x0013521c, 0x00133ae4, 0x001323e3, 0x00130d19, 0x0012f684, 0x0012e025, 0x0012c9fb, 0x0012b404, 0x00129e41, 0x001288b0, 0x00127350, 0x00125e22, 0x00124924,
    0x00123456, 0x00121fb7, 0x00120b47, 0x0011f704, 0x0011e2ef, 0x0011cf06, 0x0011bb4a, 0x0011a7b9, 0x00119453, 0x00118118, 0x00116e06, 0x00115b1e, 0x0011485f, 0x001135c8, 0x00112358, 0x00111111,
    0x0010fef0, 0x0010ecf5, 0x0010db20, 0x0010c971, 0x0010b7e6, 0x0010a681, 0x0010953f, 0x00108421, 0x00107326, 0x0010624d, 0x00105197, 0x00104104, 0x00103091, 0x00102040, 0x00101010, 0x00100000,
    0x000ff00f, 0x000fe03f, 0x000fd08e, 0x000fc0fc, 0x000fb188, 0x000fa232, 0x000f92fb, 0x000f83e0, 0x000f74e3, 0x000f6603, 0x000f5740, 0x000f4898, 0x000f3a0d, 0x000f2b9d, 0x000f1d48, 0x000f0f0f,
    0x000f00f0, 0x000ef2eb, 0x000ee500, 0x000ed730, 0x000ec979, 0x000ebbdb, 0x000eae56, 0x000ea0ea, 0x000e9396, 0x000e865a, 0x000e7937, 0x000e6c2b, 0x000e5f36, 0x000e5259, 0x000e4593, 0x000e38e3,
    0x000e2c4a, 0x000e1fc7, 0x000e135a, 0x000e0703, 0x000dfac1, 0x000dee95, 0x000de27e, 0x000dd67c, 0x000dca8f, 0x000dbeb6, 0x000db2f1, 0x000da740, 0x000d9ba4, 0x000d901b, 0x000d84a5, 0x000d7943,
    0x000d6df4, 0x000d62b8, 0x000d578e, 0x000d4c77, 0x000d4173, 0x000d3680, 0x000d2ba0, 0x000d20d2, 0x000d1615, 0x000d0b69, 0x000d00d0, 0x000cf647, 0x000cebcf, 0x000ce168, 0x000cd712, 0x000ccccc,
    0x000cc297, 0x000cb872, 0x000cae5d, 0x000ca458, 0x000c9a63, 0x000c907d, 0x000c86a7, 0x000c7ce0, 0x000c7329, 0x000c6980, 0x000c5fe7, 0x000c565c, 0x000c4ce0, 0x000c4372, 0x000c3a13, 0x000c30c3,
    0x000c2780, 0x000c1e4b, 0x000c1525, 0x000c0c0c, 0x000c0300, 0x000bfa02, 0x000bf112, 0x000be82f, 0x000bdf59, 0x000bd691, 0x000bcdd5, 0x000bc526, 0x000bbc84, 0x000bb3ee, 0x000bab65, 0x000ba2e8,
    0x000b9a78, 0x000b9214, 0x000b89bc, 0x000b8170, 0x000b7930, 0x000b70fb, 0x000b68d3, 0x000b60b6, 0x000b58a4, 0x000b509e, 0x000b48a3, 0x000b40b4, 0x000b38cf, 0x000b30f6, 0x000b2927, 0x000b2164,
    0x000b19ab, 0x000b11fd, 0x000b0a59, 0x000b02c0, 0x000afb32, 0x000af3ad, 0x000aec33, 0x000ae4c4, 0x000add5e, 0x000ad602, 0x000aceb0, 0x000ac769, 0x000ac02b, 0x000ab8f6, 0x000ab1cb, 0x000aaaaa,
    0x000aa392, 0x000a9c84, 0x000a957f, 0x000a8e83, 0x000a8791, 0x000a80a8, 0x000a79c7, 0x000a72f0, 0x000a6c21, 0x000a655c, 0x000a5e9f, 0x000a57eb, 0x000a513f, 0x000a4a9c, 0x000a4402, 0x000a3d70,
    0x000a36e7, 0x000a3065, 0x000a29ec, 0x000a237c, 0x000a1d13, 0x000a16b3, 0x000a105a, 0x000a0a0a, 0x000a03c1, 0x0009fd80, 0x0009f747, 0x0009f116, 0x0009eaec, 0x0009e4ca, 0x0009deb0, 0x0009d89d,
    0x0009d292, 0x0009cc8e, 0x0009c691, 0x0009c09c, 0x0009baad, 0x0009b4c6, 0x0009aee7, 0x0009a90e, 0x0009a33c, 0x00099d72, 0x000997ae, 0x000991f1, 0x00098c3b, 0x0009868c, 0x000980e4, 0x00097b42,
    0x000975a7, 0x00097012, 0x00096a85, 0x000964fd, 0x00095f7c, 0x00095a02, 0x0009548e, 0x00094f20, 0x000949b9, 0x00094458, 0x00093efd, 0x000939a8, 0x00093459, 0x00092f11, 0x000929ce, 0x00092492,
    0x00091f5b, 0x00091a2b, 0x00091500, 0x00090fdb, 0x00090abc, 0x000905a3, 0x00090090, 0x0008fb82, 0x0008f67a, 0x0008f177, 0x0008ec7a, 0x0008e783, 0x0008e291, 0x0008dda5, 0x0008d8be, 0x0008d3dc,
    0x0008cf00, 0x0008ca29, 0x0008c558, 0x0008c08c, 0x0008bbc5, 0x0008b703, 0x0008b246, 0x0008ad8f, 0x0008a8dc, 0x0008a42f, 0x00089f87, 0x00089ae4, 0x00089645, 0x000891ac, 0x00088d18, 0x00088888,
};
const int32_t horizlookup2[960] =
{
    0x0000020d, 0x0000020e, 0x0000020f, 0x00000210, 0x00000211, 0x00000212, 0x00000214, 0x00000215, 0x00000216, 0x00000217, 0x00000218, 0x00000219, 0x0000021a, 0x0000021c, 0x0000021d, 0x0000021e,
    0x0000021f, 0x00000220, 0x00000221, 0x00000223, 0x00000224, 0x00000225, 0x00000226, 0x00000227, 0x00000229, 0x0000022a, 0x0000022b, 0x0000022c, 0x0000022e, 0x0000022f, 0x00000230, 0x00000231,
    0x00000232, 0x00000234, 0x00000235, 0x00000236, 0x00000238, 0x00000239, 0x0000023a, 0x0000023b, 0x0000023d, 0x0000023e, 0x0000023f, 0x00000241, 0x00000242, 0x00000243, 0x00000245, 0x00000246,
    0x00000247, 0x00000249, 0x0000024a, 0x0000024b, 0x0000024d, 0x0000024e, 0x00000250, 0x00000251, 0x00000252, 0x00000254, 0x00000255, 0x00000257, 0x00000258, 0x0000025a, 0x0000025b, 0x0000025c,
    0x0000025e, 0x0000025f, 0x00000261, 0x00000262, 0x00000264, 0x00000265, 0x00000267, 0x00000268, 0x0000026a, 0x0000026b, 0x0000026d, 0x0000026e, 0x00000270, 0x00000272, 0x00000273, 0x00000275,
    0x00000276, 0x00000278, 0x00000279, 0x0000027b, 0x0000027d, 0x0000027e, 0x00000280, 0x00000281, 0x00000283, 0x00000285, 0x00000286, 0x00000288, 0x0000028a, 0x0000028b, 0x0000028d, 0x0000028f,
    0x00000291, 0x00000292, 0x00000294, 0x00000296, 0x00000298, 0x00000299, 0x0000029b, 0x0000029d, 0x0000029f, 0x000002a0, 0x000002a2, 0x000002a4, 0x000002a6, 0x000002a8, 0x000002aa, 0x000002ab,
    0x000002ad, 0x000002af, 0x000002b1, 0x000002b3, 0x000002b5, 0x000002b7, 0x000002b9, 0x000002bb, 0x000002bd, 0x000002be, 0x000002c0, 0x000002c2, 0x000002c4, 0x000002c6, 0x000002c8, 0x000002ca,
    0x000002cc, 0x000002cf, 0x000002d1, 0x000002d3, 0x000002d5, 0x000002d7, 0x000002d9, 0x000002db, 0x000002dd, 0x000002df, 0x000002e2, 0x000002e4, 0x000002e6, 0x000002e8, 0x000002ea, 0x000002ec,
    0x000002ef, 0x000002f1, 0x000002f3, 0x000002f6, 0x000002f8, 0x000002fa, 0x000002fc, 0x000002ff, 0x00000301, 0x00000303, 0x00000306, 0x00000308, 0x0000030b, 0x0000030d, 0x0000030f, 0x00000312,
    0x00000314, 0x00000317, 0x00000319, 0x0000031c, 0x0000031e, 0x00000321, 0x00000324, 0x00000326, 0x00000329, 0x0000032b, 0x0000032e, 0x00000331, 0x00000333, 0x00000336, 0x00000339, 0x0000033b,
    0x0000033e, 0x00000341, 0x00000344, 0x00000346, 0x00000349, 0x0000034c, 0x0000034f, 0x00000352, 0x00000355, 0x00000357, 0x0000035a, 0x0000035d, 0x00000360, 0x00000363, 0x00000366, 0x00000369,
    0x0000036c, 0x0000036f, 0x00000373, 0x00000376, 0x00000379, 0x0000037c, 0x0000037f, 0x00000382, 0x00000386, 0x00000389, 0x0000038c, 0x0000038f, 0x00000393, 0x00000396, 0x00000399, 0x0000039d,
    0x000003a0, 0x000003a4, 0x000003a7, 0x000003ab, 0x000003ae, 0x000003b2, 0x000003b5, 0x000003b9, 0x000003bc, 0x000003c0, 0x000003c4, 0x000003c7, 0x000003cb, 0x000003cf, 0x000003d3, 0x000003d7,
    0x000003da, 0x000003de, 0x000003e2, 0x000003e6, 0x000003ea, 0x000003ee, 0x000003f2, 0x000003f6, 0x000003fa, 0x000003ff, 0x00000403, 0x00000407, 0x0000040b, 0x0000040f, 0x00000414, 0x00000418,
    0x0000041c, 0x00000421, 0x00000425, 0x0000042a, 0x0000042e, 0x00000433, 0x00000438, 0x0000043c, 0x00000441, 0x00000446, 0x0000044a, 0x0000044f, 0x00000454, 0x00000459, 0x0000045e, 0x00000463,
    0x00000468, 0x0000046d, 0x00000472, 0x00000477, 0x0000047d, 0x00000482, 0x00000487, 0x0000048d, 0x00000492, 0x00000497, 0x0000049d, 0x000004a3, 0x000004a8, 0x000004ae, 0x000004b4, 0x000004b9,
    0x000004bf, 0x000004c5, 0x000004cb, 0x000004d1, 0x000004d7, 0x000004dd, 0x000004e4, 0x000004ea, 0x000004f0, 0x000004f7, 0x000004fd, 0x00000503, 0x0000050a, 0x00000511, 0x00000517, 0x0000051e,
    0x00000525, 0x0000052c, 0x00000533, 0x0000053a, 0x00000541, 0x00000549, 0x00000550, 0x00000557, 0x0000055f, 0x00000566, 0x0000056e, 0x00000576, 0x0000057d, 0x00000585, 0x0000058d, 0x00000595,
    0x0000059e, 0x000005a6, 0x000005ae, 0x000005b7, 0x000005bf, 0x000005c8, 0x000005d1, 0x000005d9, 0x000005e2, 0x000005ec, 0x000005f5, 0x000005fe, 0x00000607, 0x00000611, 0x0000061b, 0x00000624,
    0x0000062e, 0x00000638, 0x00000642, 0x0000064d, 0x00000657, 0x00000662, 0x0000066c, 0x00000677, 0x00000682, 0x0000068d, 0x00000698, 0x000006a4, 0x000006af, 0x000006bb, 0x000006c7, 0x000006d3,
    0x000006df, 0x000006ec, 0x000006f8, 0x00000705, 0x00000712, 0x0000071f, 0x0000072c, 0x0000073a, 0x00000748, 0x00000756, 0x00000764, 0x00000772, 0x00000781, 0x0000078f, 0x0000079e, 0x000007ae,
    0x000007bd, 0x000007cd, 0x000007dd, 0x000007ed, 0x000007fe, 0x0000080e, 0x0000081f, 0x00000831, 0x00000842, 0x00000854, 0x00000866, 0x00000879, 0x0000088c, 0x0000089f, 0x000008b3, 0x000008c6,
    0x000008db, 0x000008ef, 0x00000904, 0x0000091a, 0x0000092f, 0x00000946, 0x0000095c, 0x00000973, 0x0000098b, 0x000009a3, 0x000009bb, 0x000009d4, 0x000009ee, 0x00000a07, 0x00000a22, 0x00000a3d,
    0x00000a59, 0x00000a75, 0x00000a92, 0x00000aaf, 0x00000acd, 0x00000aec, 0x00000b0b, 0x00000b2b, 0x00000b4c, 0x00000b6e, 0x00000b90, 0x00000bb3, 0x00000bd8, 0x00000bfd, 0x00000c22, 0x00000c49,
    0x00000c71, 0x00000c9a, 0x00000cc4, 0x00000cef, 0x00000d1b, 0x00000d48, 0x00000d77, 0x00000da7, 0x00000dd8, 0x00000e0b, 0x00000e3f, 0x00000e74, 0x00000eac, 0x00000ee5, 0x00000f1f, 0x00000f5c,
    0x00000f9a, 0x00000fdb, 0x0000101d, 0x00001062, 0x000010a9, 0x000010f2, 0x0000113f, 0x0000118d, 0x000011df, 0x00001234, 0x0000128c, 0x000012e7, 0x00001346, 0x000013a9, 0x0000140f, 0x0000147a,
    0x000014ea, 0x0000155e, 0x000015d8, 0x00001657, 0x000016dc, 0x00001767, 0x000017fa, 0x00001893, 0x00001934, 0x000019de, 0x00001a91, 0x00001b4e, 0x00001c16, 0x00001ce9, 0x00001dca, 0x00001eb8,
    0x00001fb6, 0x000020c4, 0x000021e5, 0x0000231b, 0x00002468, 0x000025cf, 0x00002752, 0x000028f5, 0x00002abd, 0x00002caf, 0x00002ecf, 0x00003126, 0x000033bd, 0x0000369d, 0x000039d3, 0x00003d70,
    0x00004189, 0x00004637, 0x00004b9e, 0x000051eb, 0x0000595e, 0x0000624d, 0x00006d3a, 0x00007ae1, 0x00008c6f, 0x0000a3d7, 0x0000c49b, 0x0000f5c2, 0x000147ae, 0x0001eb85, 0x0003d70b, 0x0007ae17,
    0x0003d70b, 0x0001eb85, 0x000147ae, 0x0000f5c2, 0x0000c49b, 0x0000a3d7, 0x00008c6f, 0x00007ae1, 0x00006d3a, 0x0000624d, 0x0000595e, 0x000051eb, 0x00004b9e, 0x00004637, 0x00004189, 0x00003d70,
    0x000039d3, 0x0000369d, 0x000033bd, 0x00003126, 0x00002ecf, 0x00002caf, 0x00002abd, 0x000028f5, 0x00002752, 0x000025cf, 0x00002468, 0x0000231b, 0x000021e5, 0x000020c4, 0x00001fb6, 0x00001eb8,
    0x00001dca, 0x00001ce9, 0x00001c16, 0x00001b4e, 0x00001a91, 0x000019de, 0x00001934, 0x00001893, 0x000017fa, 0x00001767, 0x000016dc, 0x00001657, 0x000015d8, 0x0000155e, 0x000014ea, 0x0000147a,
    0x0000140f, 0x000013a9, 0x00001346, 0x000012e7, 0x0000128c, 0x00001234, 0x000011df, 0x0000118d, 0x0000113f, 0x000010f2, 0x000010a9, 0x00001062, 0x0000101d, 0x00000fdb, 0x00000f9a, 0x00000f5c,
    0x00000f1f, 0x00000ee5, 0x00000eac, 0x00000e74, 0x00000e3f, 0x00000e0b, 0x00000dd8, 0x00000da7, 0x00000d77, 0x00000d48, 0x00000d1b, 0x00000cef, 0x00000cc4, 0x00000c9a, 0x00000c71, 0x00000c49,
    0x00000c22, 0x00000bfd, 0x00000bd8, 0x00000bb3, 0x00000b90, 0x00000b6e, 0x00000b4c, 0x00000b2b, 0x00000b0b, 0x00000aec, 0x00000acd, 0x00000aaf, 0x00000a92, 0x00000a75, 0x00000a59, 0x00000a3d,
    0x00000a22, 0x00000a07, 0x000009ee, 0x000009d4, 0x000009bb, 0x000009a3, 0x0000098b, 0x00000973, 0x0000095c, 0x00000946, 0x0000092f, 0x0000091a, 0x00000904, 0x000008ef, 0x000008db, 0x000008c6,
    0x000008b3, 0x0000089f, 0x0000088c, 0x00000879, 0x00000866, 0x00000854, 0x00000842, 0x00000831, 0x0000081f, 0x0000080e, 0x000007fe, 0x000007ed, 0x000007dd, 0x000007cd, 0x000007bd, 0x000007ae,
    0x0000079e, 0x0000078f, 0x00000781, 0x00000772, 0x00000764, 0x00000756, 0x00000748, 0x0000073a, 0x0000072c, 0x0000071f, 0x00000712, 0x00000705, 0x000006f8, 0x000006ec, 0x000006df, 0x000006d3,
    0x000006c7, 0x000006bb, 0x000006af, 0x000006a4, 0x00000698, 0x0000068d, 0x00000682, 0x00000677, 0x0000066c, 0x00000662, 0x00000657, 0x0000064d, 0x00000642, 0x00000638, 0x0000062e, 0x00000624,
    0x0000061b, 0x00000611, 0x00000607, 0x000005fe, 0x000005f5, 0x000005ec, 0x000005e2, 0x000005d9, 0x000005d1, 0x000005c8, 0x000005bf, 0x000005b7, 0x000005ae, 0x000005a6, 0x0000059e, 0x00000595,
    0x0000058d, 0x00000585, 0x0000057d, 0x00000576, 0x0000056e, 0x00000566, 0x0000055f, 0x00000557, 0x00000550, 0x00000549, 0x00000541, 0x0000053a, 0x00000533, 0x0000052c, 0x00000525, 0x0000051e,
    0x00000517, 0x00000511, 0x0000050a, 0x00000503, 0x000004fd, 0x000004f7, 0x000004f0, 0x000004ea, 0x000004e4, 0x000004dd, 0x000004d7, 0x000004d1, 0x000004cb, 0x000004c5, 0x000004bf, 0x000004b9,
    0x000004b4, 0x000004ae, 0x000004a8, 0x000004a3, 0x0000049d, 0x00000497, 0x00000492, 0x0000048d, 0x00000487, 0x00000482, 0x0000047d, 0x00000477, 0x00000472, 0x0000046d, 0x00000468, 0x00000463,
    0x0000045e, 0x00000459, 0x00000454, 0x0000044f, 0x0000044a, 0x00000446, 0x00000441, 0x0000043c, 0x00000438, 0x00000433, 0x0000042e, 0x0000042a, 0x00000425, 0x00000421, 0x0000041c, 0x00000418,
    0x00000414, 0x0000040f, 0x0000040b, 0x00000407, 0x00000403, 0x000003ff, 0x000003fa, 0x000003f6, 0x000003f2, 0x000003ee, 0x000003ea, 0x000003e6, 0x000003e2, 0x000003de, 0x000003da, 0x000003d7,
    0x000003d3, 0x000003cf, 0x000003cb, 0x000003c7, 0x000003c4, 0x000003c0, 0x000003bc, 0x000003b9, 0x000003b5, 0x000003b2, 0x000003ae, 0x000003ab, 0x000003a7, 0x000003a4, 0x000003a0, 0x0000039d,
    0x00000399, 0x00000396, 0x00000393, 0x0000038f, 0x0000038c, 0x00000389, 0x00000386, 0x00000382, 0x0000037f, 0x0000037c, 0x00000379, 0x00000376, 0x00000373, 0x0000036f, 0x0000036c, 0x00000369,
    0x00000366, 0x00000363, 0x00000360, 0x0000035d, 0x0000035a, 0x00000357, 0x00000355, 0x00000352, 0x0000034f, 0x0000034c, 0x00000349, 0x00000346, 0x00000344, 0x00000341, 0x0000033e, 0x0000033b,
    0x00000339, 0x00000336, 0x00000333, 0x00000331, 0x0000032e, 0x0000032b, 0x00000329, 0x00000326, 0x00000324, 0x00000321, 0x0000031e, 0x0000031c, 0x00000319, 0x00000317, 0x00000314, 0x00000312,
    0x0000030f, 0x0000030d, 0x0000030b, 0x00000308, 0x00000306, 0x00000303, 0x00000301, 0x000002ff, 0x000002fc, 0x000002fa, 0x000002f8, 0x000002f6, 0x000002f3, 0x000002f1, 0x000002ef, 0x000002ec,
    0x000002ea, 0x000002e8, 0x000002e6, 0x000002e4, 0x000002e2, 0x000002df, 0x000002dd, 0x000002db, 0x000002d9, 0x000002d7, 0x000002d5, 0x000002d3, 0x000002d1, 0x000002cf, 0x000002cc, 0x000002ca,
    0x000002c8, 0x000002c6, 0x000002c4, 0x000002c2, 0x000002c0, 0x000002be, 0x000002bd, 0x000002bb, 0x000002b9, 0x000002b7, 0x000002b5, 0x000002b3, 0x000002b1, 0x000002af, 0x000002ad, 0x000002ab,
    0x000002aa, 0x000002a8, 0x000002a6, 0x000002a4, 0x000002a2, 0x000002a0, 0x0000029f, 0x0000029d, 0x0000029b, 0x00000299, 0x00000298, 0x00000296, 0x00000294, 0x00000292, 0x00000291, 0x0000028f,
    0x0000028d, 0x0000028b, 0x0000028a, 0x00000288, 0x00000286, 0x00000285, 0x00000283, 0x00000281, 0x00000280, 0x0000027e, 0x0000027d, 0x0000027b, 0x00000279, 0x00000278, 0x00000276, 0x00000275,
    0x00000273, 0x00000272, 0x00000270, 0x0000026e, 0x0000026d, 0x0000026b, 0x0000026a, 0x00000268, 0x00000267, 0x00000265, 0x00000264, 0x00000262, 0x00000261, 0x0000025f, 0x0000025e, 0x0000025c,
    0x0000025b, 0x0000025a, 0x00000258, 0x00000257, 0x00000255, 0x00000254, 0x00000252, 0x00000251, 0x00000250, 0x0000024e, 0x0000024d, 0x0000024b, 0x0000024a, 0x00000249, 0x00000247, 0x00000246,
    0x00000245, 0x00000243, 0x00000242, 0x00000241, 0x0000023f, 0x0000023e, 0x0000023d, 0x0000023b, 0x0000023a, 0x00000239, 0x00000238, 0x00000236, 0x00000235, 0x00000234, 0x00000232, 0x00000231,
    0x00000230, 0x0000022f, 0x0000022e, 0x0000022c, 0x0000022b, 0x0000022a, 0x00000229, 0x00000227, 0x00000226, 0x00000225, 0x00000224, 0x00000223, 0x00000221, 0x00000220, 0x0000021f, 0x0000021e,
    0x0000021d, 0x0000021c, 0x0000021a, 0x00000219, 0x00000218, 0x00000217, 0x00000216, 0x00000215, 0x00000214, 0x00000212, 0x00000211, 0x00000210, 0x0000020f, 0x0000020e, 0x0000020d, 0x0000020c,
};
const int32_t radarang2[321] =
{
    0xffffff00, 0xffffff01, 0xffffff02, 0xffffff03, 0xffffff04, 0xffffff05, 0xffffff06, 0xffffff07, 0xffffff08, 0xffffff09, 0xffffff0b, 0xffffff0c, 0xffffff0d, 0xffffff0e, 0xffffff0f, 0xffffff10,
    0xffffff11, 0xffffff12, 0xffffff13, 0xffffff15, 0xffffff16, 0xffffff17, 0xffffff18, 0xffffff19, 0xffffff1a, 0xffffff1c, 0xffffff1d, 0xffffff1e, 0xffffff1f, 0xffffff21, 0xffffff22, 0xffffff23,
    0xffffff24, 0xffffff25, 0xffffff27, 0xffffff28, 0xffffff29, 0xffffff2b, 0xffffff2c, 0xffffff2d, 0xffffff2e, 0xffffff30, 0xffffff31, 0xffffff32, 0xffffff34, 0xffffff35, 0xffffff36, 0xffffff38,
    0xffffff39, 0xffffff3b, 0xffffff3c, 0xffffff3d, 0xffffff3f, 0xffffff40, 0xffffff42, 0xffffff43, 0xffffff44, 0xffffff46, 0xffffff47, 0xffffff49, 0xffffff4a, 0xffffff4c, 0xffffff4d, 0xffffff4f,
    0xffffff50, 0xffffff52, 0xffffff53, 0xffffff55, 0xffffff56, 0xffffff58, 0xffffff59, 0xffffff5b, 0xffffff5c, 0xffffff5e, 0xffffff60, 0xffffff61, 0xffffff63, 0xffffff64, 0xffffff66, 0xffffff68,
    0xffffff69, 0xffffff6b, 0xffffff6c, 0xffffff6e, 0xffffff70, 0xffffff71, 0xffffff73, 0xffffff75, 0xffffff77, 0xffffff78, 0xffffff7a, 0xffffff7c, 0xffffff7d, 0xffffff7f, 0xffffff81, 0xffffff83,
    0xffffff84, 0xffffff86, 0xffffff88, 0xffffff8a, 0xffffff8b, 0xffffff8d, 0xffffff8f, 0xffffff91, 0xffffff93, 0xffffff95, 0xffffff96, 0xffffff98, 0xffffff9a, 0xffffff9c, 0xffffff9e, 0xffffffa0,
    0xffffffa1, 0xffffffa3, 0xffffffa5, 0xffffffa7, 0xffffffa9, 0xffffffab, 0xffffffad, 0xffffffaf, 0xffffffb1, 0xffffffb3, 0xffffffb4, 0xffffffb6, 0xffffffb8, 0xffffffba, 0xffffffbc, 0xffffffbe,
    0xffffffc0, 0xffffffc2, 0xffffffc4, 0xffffffc6, 0xffffffc8, 0xffffffca, 0xffffffcc, 0xffffffce, 0xffffffd0, 0xffffffd2, 0xffffffd4, 0xffffffd6, 0xffffffd8, 0xffffffda, 0xffffffdc, 0xffffffde,
    0xffffffe0, 0xffffffe2, 0xffffffe4, 0xffffffe6, 0xffffffe8, 0xffffffea, 0xffffffec, 0xffffffee, 0xfffffff0, 0xfffffff2, 0xfffffff4, 0xfffffff6, 0xfffffff8, 0xfffffffa, 0xfffffffc, 0xfffffffe,
    0x00000001, 0x00000003, 0x00000005, 0x00000007, 0x00000009, 0x0000000b, 0x0000000d, 0x0000000f, 0x00000011, 0x00000013, 0x00000015, 0x00000017, 0x00000019, 0x0000001b, 0x0000001d, 0x0000001f,
    0x00000021, 0x00000023, 0x00000025, 0x00000027, 0x00000029, 0x0000002b, 0x0000002d, 0x0000002f, 0x00000031, 0x00000033, 0x00000035, 0x00000037, 0x00000039, 0x0000003b, 0x0000003d, 0x0000003f,
    0x00000041, 0x00000043, 0x00000045, 0x00000047, 0x00000049, 0x0000004b, 0x0000004c, 0x0000004e, 0x00000050, 0x00000052, 0x00000054, 0x00000056, 0x00000058, 0x0000005a, 0x0000005c, 0x0000005e,
    0x0000005f, 0x00000061, 0x00000063, 0x00000065, 0x00000067, 0x00000069, 0x0000006a, 0x0000006c, 0x0000006e, 0x00000070, 0x00000072, 0x00000074, 0x00000075, 0x00000077, 0x00000079, 0x0000007b,
    0x0000007c, 0x0000007e, 0x00000080, 0x00000082, 0x00000083, 0x00000085, 0x00000087, 0x00000088, 0x0000008a, 0x0000008c, 0x0000008e, 0x0000008f, 0x00000091, 0x00000093, 0x00000094, 0x00000096,
    0x00000097, 0x00000099, 0x0000009b, 0x0000009c, 0x0000009e, 0x0000009f, 0x000000a1, 0x000000a3, 0x000000a4, 0x000000a6, 0x000000a7, 0x000000a9, 0x000000aa, 0x000000ac, 0x000000ad, 0x000000af,
    0x000000b0, 0x000000b2, 0x000000b3, 0x000000b5, 0x000000b6, 0x000000b8, 0x000000b9, 0x000000bb, 0x000000bc, 0x000000bd, 0x000000bf, 0x000000c0, 0x000000c2, 0x000000c3, 0x000000c4, 0x000000c6,
    0x000000c7, 0x000000c9, 0x000000ca, 0x000000cb, 0x000000cd, 0x000000ce, 0x000000cf, 0x000000d1, 0x000000d2, 0x000000d3, 0x000000d4, 0x000000d6, 0x000000d7, 0x000000d8, 0x000000da, 0x000000db,
    0x000000dc, 0x000000dd, 0x000000de, 0x000000e0, 0x000000e1, 0x000000e2, 0x000000e3, 0x000000e5, 0x000000e6, 0x000000e7, 0x000000e8, 0x000000e9, 0x000000ea, 0x000000ec, 0x000000ed, 0x000000ee,
    0x000000ef, 0x000000f0, 0x000000f1, 0x000000f2, 0x000000f3, 0x000000f4, 0x000000f6, 0x000000f7, 0x000000f8, 0x000000f9, 0x000000fa, 0x000000fb, 0x000000fc, 0x000000fd, 0x000000fe, 0x000000ff,
    0x00000000,
};
#endif
#include "display.h"

#define MAXCLIPNUM 512   // was 512
#define MAXPERMS 512
#define MAXTILEFILES 256
#define MAXYSAVES ((MAXXDIM*MAXSPRITES)>>8)//was ((MAXXDIM*MAXSPRITES)>>7). With >>9 sometimes glitches.
#define MAXNODESPERLINE 20 //42   /* Warning: This depends on MAXYSAVES & MAXYDIM! */
#define MAXCLIPDIST 1024
#if USE_EXT_MEM
#include "ext_memory.h"
#endif // USE_EXT_MEM
/* used to be static. --ryan. */
uint8_t  moustat = 0;
#if WIN32
    uint8_t wallModified[(MAXWALLS + 7) / 8];
#endif // WIN32
short numModifiedWalls = 0;
short numModifiedSectors = 0;
short numModifiedSprites = 0;
#if MG24
//#pragma GCC optimize ("Ofast")  // we need to compile this code to be as fast as possible.
#endif
static int32_t transarea = 0;
static uint8_t beforedrawrooms = 1;

/* used to be static. --ryan. */
int32_t oxdimen = -1, oviewingrange = -1, oxyaspect = -1;

/* used to be static. --ryan. */
int32_t curbrightness = 0;

/* Textured Map variables */
static uint8_t  globalpolytype;

char  *wallstempbuf; // was static char tempbuf[MAXWALLS_TEMP_BUFF];

#define SLOPEALOOKUP_SIZE (320+320) // 2048 original, 16384 in chocolate DN3D

/*
 * !!! used to be static. If we ever put the original setgamemode() back, this
 * !!! can be made static again.  --ryan.
 */
//static uint8_t  permanentlock = 255;
static int32_t  mapversion;        // are you kidding me ? 32 bits ?
#if !USE_EXT_MEM
uint8_t tilefilenum[MAXTILES];
uint8_t  picsiz[MAXTILES];
#error WE WANT TO USE EXT MEM
#else
const uint8_t  *picsiz; // stored in flash. [MAXTILES];
#endif // USE_EXT_MEM
//
#if !USE_EXT_MEM
int32_t tilefileoffs[MAXTILES];
#endif
int32_t artsize = 0; //, cachesize = 0;
#if LOAD_TABLES
static short radarang[1280];
static uint16_t sqrtable[4096], shlookup[4096+256];
uint8_t  britable[16][64];
uint8_t  textfont[1024], smalltextfont[1024];
int32_t reciptable[2048];
static short radarang2[MAXXDIM+1];
#else
#include "tables.h"
#endif
const uint8_t  pow2char[8] = {1,2,4,8,16,32,64,-128};
const int32_t pow2long[32] =
{
    1L,2L,4L,8L,
    16L,32L,64L,128L,
    256L,512L,1024L,2048L,
    4096L,8192L,16384L,32768L,
    65536L,131072L,262144L,524288L,
    1048576L,2097152L,4194304L,8388608L,
    16777216L,33554432L,67108864L,134217728L,
    268435456L,536870912L,1073741824L,2147483647L,
};
//int32_t fpuasm;

//char  kensmessage[128];

#if STACK_PVWALLS
pvWall_t *pvWalls;
int16_t *pvWalls_worldWallId;
void setStackPvWalls(uint8_t *ptr)
{
    if (NULL == ptr)
    {
        pvWalls =  NULL;
        pvWalls_worldWallId = NULL;
    }
    else
    {
        pvWalls = (pvWall_t*) ptr;
        pvWalls_worldWallId = (int16_t*) (ptr + sizeof(pvWall_t) * MAXWALLSB);
    }
}
#else
// Potentially Visible walls are stored in this stack.
pvWall_t pvWalls[MAXWALLSB];    // so many ? 512 isn't ok?

#if !PVWALL_HAS_WORLDWALLID
    int16_t pvWalls_worldWallId[MAXWALLSB];     //The index of the wall in the map database.
#endif

#endif // STACK_PVWALLS

#if WIN32
#define get_pvWalls_sectorId(x) get_pvWalls_sectorId2(x, __LINE__)
#endif // WIN32
#if WIN32
static inline int get_pvWalls_sectorId2(int index, int line)
#else
static inline int get_pvWalls_sectorId(int index)

#endif // WIN32
{
    if (index >= MAXWALLSB)
    {
        #if WIN32
        printf("called from line %d\r\n", line);
        #endif // WIN32
        FIXME("Too few pvWalls!. Wanted %i, max %i", index, MAXWALLSB);
        EXIT(1);
    }
#if PVWALL_HAS_WORLDWALLID
    return wallSectorId[pvWalls[index].worldWallId];
#else
    return wallSectorId[pvWalls_worldWallId[index]];

#endif
}
static inline int USATCOL(int x)
{
#if REDUCE_PVWALLS
    if (x < 0)
        x = 0;
#endif
    return x;
}


//xb1 and xb2 seems to be storing the column of the wall endpoint
//yb1 and yb2 store the Y distance from the camera.

//static int32_t xb1[MAXWALLSB], yb1[MAXWALLSB], xb2[MAXWALLSB], yb2[MAXWALLSB];

/*
//rx1,rx2,ry1,ry2 stores the cameraspace wall endpoints coordinates.
static int32_t rx1[MAXWALLSB], ry1[MAXWALLSB], rx2[MAXWALLSB], ry2[MAXWALLSB];
static short thesector[MAXWALLSB], thewall[MAXWALLSB];
*/

// bunchWallsList contains the list of walls in a bunch.
#define TEST_STACK 1
#if TEST_STACK
static short *bunchfirst, *bunchlast;
static short *bunchWallsList;

#else
static short bunchfirst[MAXWALLSB], bunchlast[MAXWALLSB];
static short bunchWallsList[MAXWALLSB];

#endif // TEST_STACK


static /*short*/ uint8_t smost[MAXYSAVES]; // next-hack: note: for fillpolygon smost was reused. Not it is not. A dedicated stack buffer is used instead.
static short smostcnt;
static short smoststart[MAXWALLSB];
//static uint8_t  smostwalltype[MAXWALLSB]; // can be reduced to 1/4 because smostwalltype is 0, 1, or 2.
static uint8_t  smostwalltypepack[(MAXWALLSB + 3) / 4]; // can be reduced to 1/4 because smostwalltype is 0, 1, or 2.
static int16_t smostwall[MAXWALLSB], smostwallcnt = -1L;    // next-hack: why were these 32 bits ?
unsigned int get_smostwalltype(int n)
{
    return 0x3 & (smostwalltypepack[n / 4] >> (2 * (n % 4)));
}
void set_smostwalltype(int n, unsigned int v)
{
    // read word
    unsigned int value = smostwalltypepack[n / 4];
    // clear
    value &= ~(3 << (2 * (n % 4)));
    // set
    value |= v << (2 * (n % 4));
    // store back
    smostwalltypepack[n / 4] = value;
}
static short maskwall[MAXWALLSB], maskwallcnt;
#define LONG_SPRITESXYZ 1
#if LONG_SPRITESXYZ
#define USE_SPRITESX 1
#if USE_SPRITESX
#define USE_STACK_SPRITESX 1
#if USE_STACK_SPRITESX
static int32_t *spritesx;
#else
static int32_t spritesx[MAXSPRITESONSCREEN];
#endif
#endif
#define USE_STACK_SPRITESY 1
#if USE_STACK_SPRITESY
static int32_t *spritesy;
#else
static int32_t spritesy[MAXSPRITESONSCREEN+1];
#endif
#define USE_SPRITESZ 1
#if USE_SPRITESZ
#define USE_STACK_SPRITESZ 1
#if USE_STACK_SPRITESZ
static int32_t *spritesz;
#else
static int32_t spritesz[MAXSPRITESONSCREEN];
#endif
#endif
#else
static int16_t spritesx[MAXSPRITESONSCREEN];
static int16_t spritesy[MAXSPRITESONSCREEN+1];
static int16_t spritesz[MAXSPRITESONSCREEN];
#endif
#define USE_TSPRITEPTR 0
#if USE_TSPRITEPTR
static tspritetype *tspriteptr[MAXSPRITESONSCREEN];
#else
static int16_t *tspriteIdx;//[MAXSPRITESONSCREEN];
#endif
//FCS: (up-most pixel on column x that can still be drawn to)

static short umost[MAXXDIM+1];

//FCS: (down-most pixel +1 on column x that can still be drawn to)
static short dmost[MAXXDIM+1];
#if IS_THIS_NECESSARY
int16_t bakumost[MAXXDIM+1], bakdmost[MAXXDIM+1];
#endif
static short uplc[MAXXDIM+1], dplc[MAXXDIM+1];
static int16_t uwall[MAXXDIM+1], dwall[MAXXDIM+1];
typedef int32_t swall_t; // next-hack: must be 32 bit
#define clearbuf_swall              clearbuf
static swall_t swall[MAXXDIM+1];
typedef swall_t swplc_t;        // next-hack: must be 32 bit, same size as swall_t
#define clearbuf_swplc              clearbuf_swall
// next-hack: swplc is now in stack, in parascan()
//static swplc_t swplc[MAXXDIM+1];
typedef int16_t lwall_t; // next-hack: was 32 bit
#define qinterpolatedown16_lwall    qinterpolatedown16short
static lwall_t lwall[MAXXDIM+4];
// next-hack: lplc is now in stack, in parascan()
//static lwall_t lplc[MAXXDIM+1]; //next-hack was 32 bit. Must be same type of lwall
static int32_t xdimen = -1, xdimenrecip, xdimenscale, xdimscale;
int32_t halfxdimen;
static int32_t wx1, wy1, wx2, wy2;
int32_t ydimen;
static int32_t viewoffset;



/* used to be static. --ryan. */
#if !USE_STATIC_LOOKUP_TABLES
int32_t *horizlookup=0, *horizlookup2=0;
#endif
static int32_t horizycent;

static int32_t globalposx, globalposy, globalposz, globalhoriz;
static int16_t globalang, globalcursectnum;
static int32_t globalpal, cosglobalang, singlobalang;
static int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;
const uint8_t  *globalpalwritten;
static int32_t globaluclip, globaldclip, globvis = 0;
static int32_t globalvisibility, globalhisibility, globalpisibility, globalcisibility;
static uint8_t  globparaceilclip, globparaflorclip;

static int32_t xyaspect, viewingrangerecip;

//int32_t asm1, asm4;
//intptr_t asm2, asm3;

// All the following 4 arrays had 4 elements. But at most we use 2.
static int32_t vplce[2], vince[2];
static intptr_t bufplce[2];
static const uint8_t*  palookupoffse[2];


static uint8_t  globalxshift, globalyshift;
static int32_t globalxpanning, globalypanning, globalshade;
int16_t globalpicnum, globalshiftval;
static int32_t globalzd, globalyscale, globalorientation;
static const uint8_t* globalbufplc;
static int32_t globalx1, globaly1, globalx2, globaly2, globalzx;
int32_t globalx3, globaly3;
static int32_t globalx, globaly, globalz;

//FCS:
// Those two variables are using during portal flooding:
// sectorBorder is the stack and sectorbordercnt is the stack counter.
// There is no really point to have this on the heap. That would have been better on the stack.

//static short sectorborder[256], sectorbordercnt;
//FCS: Moved this on the stack

//static uint8_t  tablesloaded = 0;
int32_t pageoffset, ydim16, qsetmode = 0;
//static int32_t startposx, startposy, startposz;
//static int16_t startang, startsectnum;
static int16_t pointhighlight, linehighlight, highlightcnt;

#if LASTX_IN_STACK
int16_t *lastx;  // was 32 bit
#else
static int16_t lastx[MAXYDIM];  // was 32 bit
#endif
static uint8_t  paletteloaded = 0;

#define FASTPALGRIDSIZ 8
#if RAM_COLORS
static int32_t rdist[129], gdist[129], bdist[129];
static uint8_t  colhere[((FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2))>>3];
static uint8_t  colhead[(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)];
static int32_t colnext[256];
static uint8_t  coldist[8] = {0,1,2,3,4,3,2,1};
static int32_t colscan[27];
#else
static const int32_t *rdist, *gdist, *bdist;
static const uint8_t  *colhere;
static const uint8_t  *colhead;
static const int32_t *colnext;
//static const uint8_t  coldist[8] = {0,1,2,3,4,3,2,1};
static const int32_t *colscan;

#endif

static int16_t clipnum, hitwalls[4];
static int32_t hitscangoalx = (1<<29)-1, hitscangoaly = (1<<29)-1;

typedef struct {
    int32_t x1, y1, x2, y2;
} linetype;
static linetype *clipit; //[MAXCLIPNUM];  next-hack: on stack because it is used only by clipmove
//static short clipsectorlist[MAXCLIPNUM];    next-hack: on stack, used only within each function.
static short clipsectnum;
static short *clipobjectval; //[MAXCLIPNUM]; next-hack: in stack but used in clipmove and its child addclipit
#if CODE_NOT_IMPLEMENTED
typedef struct
{
    int32_t sx, sy, z;
    short a, pic num;
    int8_t dashade;
    uint8_t  dapalnum, dastat, pagesleft;
    int32_t cx1, cy1, cx2, cy2;
} permfifotype;
static permfifotype permfifo[MAXPERMS];
static int32_t permhead = 0, permtail = 0;
#endif
//FCS: Num walls to potentially render.
static short numscans ;

static short numbunches;

//FCS: Number of colums to draw. ALWAYS set to the screen dimension width.
static short numhits;

static short editstatus = 0;
static short searchit;
int32_t searchx = -1, searchy;                     /* search input  */
static short searchsector, searchwall, searchstat;     /* search output */

int32_t numtilefiles, artfil = -1, artfilnum, artfilplc;

static uint8_t  inpreparemirror = 0;
static int32_t mirrorsx1, mirrorsy1, mirrorsx2, mirrorsy2;

int32_t totalclocklock;

uint16_t mapCRC;
#include "draw.h"
void setHorizycent(int32_t v)
{
    horizycent = v;
}
#define MIN_PIXELS_FOR_PALETTE_CACHE 8    //
#if WIN32
static int paletteLoadedPerFrame = 0;
#endif // WIN32
#if MAX_PALETTE_CACHE_ROWS > 1
static uint8_t* getPalette(uint8_t *paletteToBeLoaded, int numPixels)
{
#define PROFILE_PALETTE_CACHE WIN32 && 0
    // increment number of calls so we can implement our LRU eviction algorithm
    paletteTime++;
    for (int i = 0; i < MAX_PALETTE_CACHE_ROWS; i++)
    {
        if (paletteToBeLoaded == currentCachedPalettePointer[i])
        {
#if PROFILE_PALETTE_CACHE
        printf("used palette already cached in row %d\r\n", i);
#endif // PROFILE_PALETTE_CACHE
            lastPaletteUsedTime[i] = paletteTime;
            return cachedPalette[i];
        }
    }
    // not yet cached. Worthy ?
    if (numPixels >= MIN_PIXELS_FOR_PALETTE_CACHE)
    {
        // yes. Let's evict one cache line
        uint32_t earliestPaletteTime = 0xFFFFFFFF;
        int earliestPalette = 0;
        for (int i = 0; i < MAX_PALETTE_CACHE_ROWS; i++)
        {
            if (earliestPaletteTime > lastPaletteUsedTime[i])
            {
                earliestPaletteTime = lastPaletteUsedTime[i];
                earliestPalette = i;
            }
        }
#if PROFILE_PALETTE_CACHE
        printf("Evicted row %d\r\n", earliestPalette);
#endif
        uint32_t *s = (uint32_t*) paletteToBeLoaded;
        uint32_t *d = (uint32_t*) (cachedPalette[earliestPalette]);
        for (int i = 0; i < 64 ; i+= 8)
        {
            // copy 32 colors per time
            d[i + 0] = s[i + 0];
            d[i + 1] = s[i + 1];
            d[i + 2] = s[i + 2];
            d[i + 3] = s[i + 3];
            d[i + 4] = s[i + 4];
            d[i + 5] = s[i + 5];
            d[i + 6] = s[i + 6];
            d[i + 7] = s[i + 7];
        }
        #if WIN32
        paletteLoadedPerFrame++;
        #endif // WIN32
        currentCachedPalettePointer[earliestPalette] = paletteToBeLoaded;
        lastPaletteUsedTime[earliestPalette] = paletteTime;
        return cachedPalette[earliestPalette];
    }
    else
    {
#if PROFILE_PALETTE_CACHE
        printf("Not evicited cache.\r\n");
#endif // PROFILE_PALETTE_CACHE
        return paletteToBeLoaded; // keep the one in flash.
    }
}
#else
static const uint8_t* getPalette(const uint8_t *paletteToBeLoaded, int numPixels)
{
  if (paletteToBeLoaded == currentCachedPalettePointer)
    return cachedPalette;

  // not yet cached. Worthy ?
  if (numPixels >= MIN_PIXELS_FOR_PALETTE_CACHE)
  {
    uint32_t *s = (uint32_t*) paletteToBeLoaded;
    uint32_t *d = (uint32_t*) cachedPalette;
    for (int i = 0; i < 64 ; i+= 8)
    {
      // copy 32 colors per time
        d[i + 0] = s[i + 0];
        d[i + 1] = s[i + 1];
        d[i + 2] = s[i + 2];
        d[i + 3] = s[i + 3];
        d[i + 4] = s[i + 4];
        d[i + 5] = s[i + 5];
        d[i + 6] = s[i + 6];
        d[i + 7] = s[i + 7];
    }
#if WIN32
    paletteLoadedPerFrame++;
#endif // WIN32
    currentCachedPalettePointer = paletteToBeLoaded;
    return cachedPalette;
  }
  else
  {
    return paletteToBeLoaded; // keep the one in flash.
  }
}

#endif

void freeRamSprite(int sprite_index)
{
    int index = getRamSpriteIdx(sprite_index);
    if (index)
    {
            index--;
            numModifiedSprites--;
            ramsprites_allocated_bf[index / 8] &= ~(1 << (index % 8));
            /*clear it (not needed but let's do it!)*/
            memset(&ramsprites[index], 0, sizeof(ramsprites[index]));
            ramsprites[index]._sectnum = MAXSECTORS;
            ramspriteIdx[sprite_index] = 0;    /* not needed, would be cleared later.*/
#if WIN32 && 0
            printf("Freed ramsprite index %d, sprite index %d. Modified %d\r\n",  index, sprite_index, numModifiedSprites);
#endif
    }
}
static short sprlastIndex = 0;
void resetRamSpriteAllocIndex(void)
{
    sprlastIndex = 0;
}
spritetype * allocateNewRamSprite(int sprite_index)
{
    int wrapped = 0;
    int startIndex = sprlastIndex;
    do
    {
        sprlastIndex++;
        if (sprlastIndex >= MAXRAMSPRITES) /*roll over */
        {
            sprlastIndex = 0;
        }
        if (!(ramsprites_allocated_bf[sprlastIndex / 8] & (1 << (sprlastIndex % 8))))
        {
#if WIN32 && 0
            printf("will allocate ramsprites[%d] for sprite %d, numModified %d\r\n", sprlastIndex, sprite_index, numModifiedSprites);
#endif
            numModifiedSprites++;

            ramsprites_allocated_bf[sprlastIndex / 8] |= (1 << (sprlastIndex % 8));
            /* copy old sprite*/
            fastmemcpy(&ramsprites[sprlastIndex], &sprites[sprite_index], sizeof(spritetype));
            //ramsprites[sprlastIndex]._sectnum = MAXSECTORS;
            ramspriteIdx[sprite_index] = sprlastIndex + 1;
       /*     printf("Allocated %s # %d for hittype[%d], new index %d\r\n", #type, numHittype_##type, hittype_index, sprlastIndex);*/
            break;
        }
        if (sprlastIndex == startIndex)
        {
            if (wrapped > 1)
            {
            FIXME("Error, cannot find an empty slot for ramsprites\r\n");
            EXIT(0);
        }
            wrapped++;
            freeUnnecessarySprites(sprite_index, NO_HITTYPE_REQUIRED);
        }
    } while (1);
    return &ramsprites[sprlastIndex];
}
//
walltype *allocateNewRamWall(int index)
{
    if (numModifiedWalls > MAXRAMWALLS - 1)
    {
        FIXME("Too many modified walls");
        EXIT(1);
        return NULL;
    }
    else
    {
        memcpy(&ramwalls[numModifiedWalls], &walls[index], sizeof(ramwalls[index]));
        numModifiedWalls++;
        setRamWallIdx(index, numModifiedWalls);       // 0 means not assigned. we might use -1 to set unassigned... TBD
        return &ramwalls[numModifiedWalls - 1];
    }
}
//
sectortype *allocateNewRamSector(int index)
{
    if (numModifiedSectors > MAXRAMSECTORS - 1)
    {
        FIXME("Too many modified sectors");
        EXIT(1);
        return NULL;
    }
    else
    {
        fastmemcpy(&ramsectors[numModifiedSectors], &sectors[index], sizeof(ramsectors[index]));
        numModifiedSectors++;
        ramsectorIdx[index] = numModifiedSectors;       // 0 means not assigned. we might use -1 to set unassigned... TBD
        return &ramsectors[numModifiedSectors - 1];
    }
}


static inline int32_t nsqrtasm(uint32_t  param)
{
#if NO_FPU
    // next-hack: WTF is this ?
    uint16_t *shlookup_a = (uint16_t*)shlookup;
    uint16_t *sqrtable_a = (uint16_t*)sqrtable;
    uint16_t cx;

    if (param & 0xff000000)
        cx = shlookup_a[(param>>24)+4096];
    else
        cx = shlookup_a[param>>12];

    param = param >> (cx&0xff);
    param = ((param&0xffff0000)|sqrtable_a[param]);
    param = param >> ((cx&0xff00)>>8);

    return param;
#else
    return sqrtf(param);
#endif
}

static inline int32_t krecipasm(int32_t i)
{
#if NO_FPU
    // tanguyf: fix strict aliasing rules.
    union
    {
        float f;
        int32_t i;
    } value;

    // Ken did this
 //   float f = (float)i;
 //   i = *(int32_t *)&f;

    value.f = (float)i;
    i = value.i;
    return((reciptable[(i >> 12) & 2047] >> (((i - 0x3f800000) >> 23) & 31)) ^ (i >> 31));
#else
    // if we have fpu, probably it is faster doing this division.
    return (1 << 30)/(float)i;
#endif
}

static inline int32_t getclipmask(int32_t a, int32_t b, int32_t c, int32_t d)
{   // Ken did this
    d = ((a<0)*8) + ((b<0)*4) + ((c<0)*2) + (d<0);
    return(((d<<4)^0xf0)|d);
}

//
// krecip
//
int32_t krecip(int32_t num)
{
    return(krecipasm(num));
}

uint16_t _swap16(uint16_t D)
{
    return((D<<8)|(D>>8));
}

unsigned int _swap32(unsigned int D)
{
    return((D<<24)|((D<<8)&0x00FF0000)|((D>>8)&0x0000FF00)|(D>>24));
}
void copySpriteToTsprite(tspritetype *d , const spritetype *s)
{
 //  memcpy(d, s, sizeof(*s)); return;
    d->x = s->_x;
    d->y = s->_y;
    d->z = s->_z;
    d->cstat = s->_cstat;
    d->picnum = s->_picnum;
    d->shade = s->_shade;
    d->pal = s->_pal;
    d->xrepeat = s->_xrepeat;
    d->yrepeat = s->_yrepeat;
#if TSPRITE_XOFFSET
    d->xoffset = s->_xoffset;
#endif
#if TSPRITE_YOFFSET
    d->yoffset = s->_yoffset;
#endif
    d->sectnum = s->_sectnum;
    d->statnum = s->_statnum;
    d->ang = s->_ang;
    d->owner = s->_owner;
//  d->clipdist = s->clipdist;
//  d->filler = s->filler;
//  d->xvel = s->xvel;
//  d->yvel = s->yvel;
//    d->zvel = s->zvel;
//  d->hitag = s->hitag;
#if TSPRITE_HAS_EXTRA
    d->extra = s->extra;
#else
    d->extraBiggerThanZero = s->_extra > 0;
#endif
#if TSPRITE_HAS_LOTAG
    d->lotag = s->lotag;
#else
    d->lotagIs27 = s->_lotag == 27;
#endif

}
/*
 * next-hack: data on GRP file are not word aligned. So reading from ext mem is quite expensive
 * Because after that a memcpy is required. Instead, we read few more bytes so that everything
 * is aligned
 */
static uint8_t *readColumnAligned(uint8_t *buffer, const uint8_t *address,  int size)
{
  // instead of reading at address 1, 2 or 3, we read from address 0. This means that we need to increase size by the amount we have masked

  int offset =((uint32_t) address & 3);
  //
  size += offset;
  // now we need to adjust size to 4 byte boundary
  size = (size + 3) & ~3;
  // align address
  const uint8_t *alignedAddress = address - offset;
  extMemSetCurrentAddress((uint32_t) alignedAddress);
  extMemGetDataAligned(buffer, size);
  // now we won't return not buffer,
  return &buffer[offset];
}
static uint8_t *readColumnAlignedAsync(uint8_t *buffer, const uint8_t *address,  int size)
{
    #if WIN32
        return extMemGetDataFromAddress(buffer, address, size);
    #else
  // instead of reading at address 1, 2 or 3, we read from address 0. This means that we need to increase size by the amount we have masked

  int offset =((uint32_t) address & 3);
  //
  size += offset;
  // now we need to adjust size to 4 byte boundary
  size = (size + 3) & ~3;
  // align address
  const uint8_t *alignedAddress = address - offset;
  extMemSetCurrentAddress((uint32_t) alignedAddress);
  extMemStartAsynchDataRead((uint32_t) alignedAddress, buffer, size);
  // now we won't return not buffer,
  return &buffer[offset];
  #endif
}

/*
 FCS:
 Scan through sectors using portals (a portal is wall with a nextsector attribute >= 0).
 Flood is prevented if a portal does not face the POV.
 */
static void scansector (short sectnum)
{
    const walltype *wal, *wal2;
//    const spritetype *spr;
    int32_t xs, ys, x1, y1, x2, y2, xp1, yp1, xp2=0, yp2=0, tempint;
    short z, zz, startwall, endwall, numscansbefore, scanfirst, bunchfrst;
    short nextsectnum;

    //The stack storing sectors to visit.
    short sectorsToVisit[256], numSectorsToVisit;


    if (sectnum < 0)
        return;

    if (automapping)
        show2dsector[sectnum>>3] |= pow2char[sectnum&7];

    sectorsToVisit[0] = sectnum;
    numSectorsToVisit = 1;
    do
    {
        sectnum = sectorsToVisit[--numSectorsToVisit];

        //Add every script in the current sector as potentially visible.
        for(z=headspritesect[sectnum]; z>=0; z=spr_add_data[z].nextspritesect)
        {
           const spritetype *sz = getSprite(z);
            if ((((sz->_cstat&SPRITE_INVISIBLE) == 0) || (showinvisibility)) &&
                    (sz->_xrepeat > 0) && (sz->_yrepeat > 0) &&
                    (spritesortcnt < MAXSPRITESONSCREEN))
            {
                xs =sz->_x-globalposx;
                ys = sz->_y-globalposy;
                if ((sz->_cstat&48) || (xs*cosglobalang+ys*singlobalang > 0))
                {
#if !TEST_TSPRITE
#error
                    copybufbyte(getSprite(z),&tsprite[spritesortcnt],sizeof(spritetype));
#else
                    copySpriteToTsprite(&tsprite[spritesortcnt], sz);
#endif
                    tsprite[spritesortcnt++].owner = z;
                }
            }
        }

        //Mark the current sector bit as "visited" in the bitvector
        visitedSectors[sectnum>>3] |= pow2char[sectnum&7];

        bunchfrst = numbunches;
        numscansbefore = numscans;
//  next-hack:  different sectors do not share same walls.
        startwall = constsector[sectnum].wallptr;
        endwall = startwall + constsector[sectnum].wallnum;
        scanfirst = numscans;
        for(z=startwall,wal=getWall(z); z<endwall; z++,wal=getWall(z))
        {

            nextsectnum = constwall[z].nextsector;

            wal2 = getWall(constwall[z]._point2);

            // In camera space the center is the player.
            // Tranform the 2 Wall endpoints (x,y) from worldspace to camera space.
            // After that we have two vectors starting from the camera and going to the endpoints (x1,y1) and (x2,y2).
            x1 = wal->x-globalposx;
            y1 = wal->y-globalposy;

            x2 = wal2->x-globalposx;
            y2 = wal2->y-globalposy;

            // If this is a portal...
            if ((nextsectnum >= 0) && ((wal->_cstat&32) == 0))
                //If this portal has not been visited yet.
                if ((visitedSectors[nextsectnum>>3]&pow2char[nextsectnum&7]) == 0)
                {
                    //Cross product -> Z component
                    tempint = x1*y2-x2*y1;

                    // Using cross product, determine if the portal is facing us or not.
                    // If it is: Add it to the stack and bump the stack counter.
                    // This line is equivalent to tempint < 0x40000
                    if (((uint32_t)tempint+262144) < 524288) // ??? What is this test ?? How acute the angle is ?
                    {
                        //(x2-x1)*(x2-x1)+(y2-y1)*(y2-y1) is the squared length of the wall
                        // ??? What is this test ?? How acute the angle is ?
                        if (mulscale5(tempint,tempint) <= (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1))
                            sectorsToVisit[numSectorsToVisit++] = nextsectnum;
                    }
                }

            // Rotate the wall endpoints vectors according to the player orientation.
            // This is a regular rotation matrix using [29.3] fixed point.
            if ((z == startwall) || (getWall_point2(z-1) != z))
            {
                //If this is the first endpoint of the bunch, rotate: This is a standard cos sin 2D rotation matrix projection
                xp1 = dmulscale6(y1,cosglobalang,-x1,singlobalang);
                yp1 = dmulscale6(x1,cosviewingrangeglobalang,y1,sinviewingrangeglobalang);
            }
            else
            {
                //If this is NOT the first endpoint, Save the coordinate for next loop.
                xp1 = xp2;
                yp1 = yp2;
            }

            // Rotate: This is a standard cos sin 2D rotation matrix projection
            xp2 = dmulscale6(y2,cosglobalang,-x2,singlobalang);
            yp2 = dmulscale6(x2,cosviewingrangeglobalang,y2,sinviewingrangeglobalang);



            // Equivalent of a near plane clipping ?
            if ((yp1 < 256) && (yp2 < 256)) goto skipitaddwall;

            /* If wall's NOT facing you */
            if (dmulscale32(xp1,yp2,-xp2,yp1) >= 0) goto skipitaddwall;

            // The wall is still not eligible for rendition: Let's do some more Frustrum culling !!
            if (xp1 >= -yp1){

                if ((xp1 > yp1) || (yp1 == 0))
                    goto skipitaddwall;

                //Project the point onto screen and see in which column it belongs.
                pvWalls[numscans].screenSpaceCoo[0].vec_col = USATCOL(halfxdimen + scale(xp1,halfxdimen,yp1));
                if (xp1 >= 0)
                    pvWalls[numscans].screenSpaceCoo[0].vec_col++;   /* Fix for SIGNED divide */

                if (pvWalls[numscans].screenSpaceCoo[0].vec_col >= xdimen)
                    pvWalls[numscans].screenSpaceCoo[0].vec_col = xdimen-1;

                setVecDist(&pvWalls[numscans].screenSpaceCoo[0], yp1);
            }
            else{

                if (xp2 < -yp2)
                    goto skipitaddwall;

                pvWalls[numscans].screenSpaceCoo[0].vec_col = 0;
                tempint = yp1-yp2+xp1-xp2;

                if (tempint == 0)
                    goto skipitaddwall;

                setVecDist(&pvWalls[numscans].screenSpaceCoo[0], yp1 + scale(yp2-yp1,xp1+yp1,tempint));
            }

            if (getVecDist(&pvWalls[numscans].screenSpaceCoo[0]) < 256)
                goto skipitaddwall;

            if (xp2 <= yp2){

                if ((xp2 < -yp2) || (yp2 == 0)) goto skipitaddwall;
                pvWalls[numscans].screenSpaceCoo[1].vec_col = USATCOL(halfxdimen + scale(xp2,halfxdimen,yp2) - 1);
                if (xp2 >= 0) pvWalls[numscans].screenSpaceCoo[1].vec_col++;   /* Fix for SIGNED divide */
                if (pvWalls[numscans].screenSpaceCoo[1].vec_col >= xdimen) pvWalls[numscans].screenSpaceCoo[1].vec_col = xdimen-1;
                setVecDist(&pvWalls[numscans].screenSpaceCoo[1], yp2);
            }
            else{

                if (xp1 > yp1) goto skipitaddwall;
                pvWalls[numscans].screenSpaceCoo[1].vec_col = xdimen-1;
                tempint = xp2-xp1+yp1-yp2;
                if (tempint == 0) goto skipitaddwall;
                setVecDist(&pvWalls[numscans].screenSpaceCoo[1], yp1 + scale(yp2-yp1,yp1-xp1,tempint));
            }
            if ((getVecDist(&pvWalls[numscans].screenSpaceCoo[1]) < 256) || (pvWalls[numscans].screenSpaceCoo[0].vec_col > pvWalls[numscans].screenSpaceCoo[1].vec_col)) goto skipitaddwall;

            // Made it all the way!
            // Time to add this wall information to the stack of wall potentially visible.
//    #warning remove me
  //          pvWalls[numscans].sectorId = sectnum;
#if PVWALL_HAS_WORLDWALLID
            pvWalls[numscans].worldWallId = z;
#else
            pvWalls_worldWallId[numscans] = z;

#endif
            //Save the camera space wall endpoints coordinate (camera origin at player location + rotated according to player orientation).
            pvWalls[numscans].cameraSpaceCoo[0][VEC_X] = xp1;
            pvWalls[numscans].cameraSpaceCoo[0][VEC_Y] = yp1;
            pvWalls[numscans].cameraSpaceCoo[1][VEC_X] = xp2;
            pvWalls[numscans].cameraSpaceCoo[1][VEC_Y] = yp2;


            bunchWallsList[numscans] = numscans+1;
            numscans++;

skipitaddwall:

            if ((constwall[z]._point2 < z) && (scanfirst < numscans))
            {
                bunchWallsList[numscans-1] = scanfirst;
                scanfirst = numscans;
            }
        }

        //FCS: TODO rename this p2[] to bunchList[] or something like that. This name is an abomination
        //     DONE, p2 is now called "bunchWallsList".

        //Break down the list of walls for this sector into bunches. Since a bunch is a
        // continuously visible list of wall: A sector can generate many bunches.
        for(z=numscansbefore; z<numscans; z++)
        {

#if PVWALL_HAS_WORLDWALLID
            if ((getWall_point2(pvWalls[z].worldWallId) !=
                 pvWalls[bunchWallsList[z]].worldWallId) || (pvWalls[z].screenSpaceCoo[1].vec_col >= pvWalls[bunchWallsList[z]].screenSpaceCoo[0].vec_col))
 #else
            if ((getWall_point2(pvWalls_worldWallId[z]) !=
                 pvWalls_worldWallId[bunchWallsList[z]]) || (pvWalls[z].screenSpaceCoo[1].vec_col >= pvWalls[bunchWallsList[z]].screenSpaceCoo[0].vec_col))


 #endif
            {
                // Create an entry in the bunch list
                bunchfirst[numbunches++] = bunchWallsList[z];

                //Mark the end of the bunch wall list.
                bunchWallsList[z] = -1;
            }
        }

        //For each bunch, find the last wall and cache it in bunchlast.
        for(z=bunchfrst; z<numbunches; z++)
        {
            for(zz=bunchfirst[z]; bunchWallsList[zz]>=0; zz=bunchWallsList[zz]);
            bunchlast[z] = zz;
        }

    } while (numSectorsToVisit > 0);
    // do this until the stack of sectors to visit if empty.
}
// next-hack: this source tries to render columns 4-pixels thick.
// This would be a good idea with RAM cache (on MCUs we typically don't have, and RAM anyway is quite fast)
// But I see later than the pixels are stored one by one. would be better doing everything on a single register and then
// store the register to memory. Also, to get this 4-pixel rendering, one has to do so many checks, and do some later
// "cleanup" because each column might have different length. So at the end we think that Doom's way (i.e. single pixel column rendering)
// might be faster (and easier to understand) in our case.
#define RENDERING_SIMPLIFICATION 1

/*
 FCS:

 Goal : ???? next-hack: probably trying to get the top and bottom part of the column for each wall, like drawseg in doom?
 param 1: Z is the wallID in the list of potentially visible walls.
 param 2: Only used to lookup the xrepeat attribute of the wall.

*/
static void prepwall(int32_t z, int wallIdx)// walltype *wal)
{
    int32_t i, l=0, ol=0, splc, sinc, x, topinc, top, botinc, bot, walxrepeat;
    vector_t* wallCoo = pvWalls[z].cameraSpaceCoo;

    const walltype *wal = getWall(wallIdx);

    walxrepeat = (constwall[wallIdx]._xrepeat<<3);

    /* lwall calculation */
    i = pvWalls[z].screenSpaceCoo[0].vec_col-halfxdimen;

    //Let's use some of the camera space wall coordinate now.
    topinc = -(wallCoo[0][VEC_Y]>>2);
    botinc = ((wallCoo[1][VEC_Y]-wallCoo[0][VEC_Y])>>8);

    top = mulscale5(wallCoo[0][VEC_X],xdimen)+mulscale2(topinc,i);
    bot = mulscale11(wallCoo[0][VEC_X]-wallCoo[1][VEC_X],xdimen)+mulscale2(botinc,i);

    splc = mulscale19(wallCoo[0][VEC_Y],xdimscale);
    sinc = mulscale16(wallCoo[1][VEC_Y]-wallCoo[0][VEC_Y],xdimscale);

    //X screenspce column of point Z.
    x = pvWalls[z].screenSpaceCoo[0].vec_col;

    if (bot != 0)
    {
        l = divscale12(top,bot);
        swall[x] = mulscale21(l,sinc)+splc;
        l *= walxrepeat;
        lwall[x] = (l>>18);
    }

    //If the wall is less than 4 column wide.
    // next-hack: calculate in group of 4 columns, UNTIL we have 4 columns or more
    while (x+4 <= pvWalls[z].screenSpaceCoo[1].vec_col)
    {
        top += topinc;
        bot += botinc;
        if (bot != 0)
        {
            ol = l;
            l = divscale12(top,bot);
            swall[x+4] = mulscale21(l,sinc)+splc;
            l *= walxrepeat;
            lwall[x+4] = (l>>18);
        }
        i = ((ol+l)>>1);
        lwall[x+2] = (i>>18);
        lwall[x+1] = ((ol+i)>>19);
        lwall[x+3] = ((l+i)>>19);
        swall[x+2] = ((swall[x]+swall[x+4])>>1);
        swall[x+1] = ((swall[x]+swall[x+2])>>1);
        swall[x+3] = ((swall[x+4]+swall[x+2])>>1);
        x += 4;
    }

    //If the wall is less than 2 columns wide.
    // next hack: proceed with next 2 columns.
    if (x+2 <= pvWalls[z].screenSpaceCoo[1].vec_col)
    {
        top += (topinc>>1);
        bot += (botinc>>1);
        if (bot != 0)
        {
            ol = l;
            l = divscale12(top,bot);
            swall[x+2] = mulscale21(l,sinc)+splc;
            l *= walxrepeat;
            lwall[x+2] = (l>>18);
        }
        lwall[x+1] = ((l+ol)>>19);
        swall[x+1] = ((swall[x]+swall[x+2])>>1);
        x += 2;
    }

    //The wall is 1 column wide.
    // next hack:  finally, if the wall had an odd number of columns
    if (x+1 <= pvWalls[z].screenSpaceCoo[1].vec_col)
    {
        bot += (botinc>>2);
        if (bot != 0)
        {
            l = divscale12(top+(topinc>>2),bot);
            swall[x+1] = mulscale21(l,sinc)+splc;
            lwall[x+1] = mulscale18(l,walxrepeat);
        }
    }

    if (lwall[pvWalls[z].screenSpaceCoo[0].vec_col] < 0)           // next-hack: note: screenSpaceCoo[0].vec_col maxes out at 320
        lwall[pvWalls[z].screenSpaceCoo[0].vec_col] = 0;

    if ((lwall[pvWalls[z].screenSpaceCoo[1].vec_col] >= walxrepeat) && (walxrepeat))
        lwall[pvWalls[z].screenSpaceCoo[1].vec_col] = walxrepeat-1;

    if (wal->_cstat&8)  // next-hack: it's fine, no need of wrapper. We are already sending the ptr
    {
        walxrepeat--;
        for(x=pvWalls[z].screenSpaceCoo[0].vec_col; x<=pvWalls[z].screenSpaceCoo[1].vec_col; x++)
            lwall[x] = walxrepeat-lwall[x];
    }
}


static int32_t getpalookup(int32_t davis, int32_t dashade)
{
    return(min(max(dashade+(davis>>8),0),numpalookups-1));
}

static void hline (int32_t xr, int32_t yp)
{
    int32_t xl, r, s;

    xl = lastx[yp];

    if (xl > xr)
        return;

    r = horizlookup2[yp-globalhoriz+horizycent];
    draws.asm1 = globalx1*r;
    draws.asm2 = globaly2*r;
    s = (getpalookup(mulscale16(r,globvis),globalshade)<<8);


    draws.globalpalshade = &globalpalwritten[s];
    draws.globalpalshade = getPalette(draws.globalpalshade, xr-xl);
        // here put globalx2*r+globalypanning - globalx1*r * (xr - xl), and not +xr, but - xr.
    //hlineasm4(xr-xl,/*s,*/globalx2*r+globalypanning, globaly1*r+globalxpanning, ylookup(yp)+xr+frameoffset);
    waitRenderLine(yp);

    hlineasmQuad(xr - xl + 1, globalx2*r+globalypanning - draws.asm2 * (xr - xl), globaly1 * r + globalxpanning - draws.asm1 * (xr - xl), ylookup(yp) + xl + frameoffset);
}


static void slowhline (int32_t xr, int32_t yp)
{
    // is this called? I mean, it is called in the source, but during gameplay this was never called
    int32_t xl, r;
#if WIN32
                FIXME("Slow Line");
#endif
    xl = lastx[yp];
    if (xl > xr) return;
    r = horizlookup2[yp-globalhoriz+horizycent];
    draws.asm1 = globalx1*r;
    draws.asm2 = globaly2*r;

    draws.asm3 = (int32_t)globalpalwritten + (getpalookup(mulscale16(r,globvis),globalshade)<<8);
    waitRenderLine(yp);
    if (!(globalorientation&256))
    {
        mhline(globalbufplc,globaly1*r+globalxpanning- draws.asm1*(xr-xl),(xr-xl)<<16,
               globalx2*r+globalypanning- draws.asm2*(xr-xl),ylookup(yp)+xl+frameoffset);
        return;
    }
    thline(globalbufplc,globaly1*r+globalxpanning- draws.asm1*(xr-xl),(xr-xl)<<16,globalx2*r+globalypanning-draws.asm2*(xr-xl),ylookup(yp)+xl+frameoffset);
    transarea += (xr-xl);
}





/* renders non-parallaxed ceilings. --ryan. */
static void ceilscan (int32_t x1, int32_t x2, int32_t sectnum)
{
    int32_t i, j, ox, oy, x, y1, y2, twall, bwall;
    const sectortype *sec;
    uint8_t flat[MAX_RAM_FLAT_SIZE];    // next-hack: yes, I have borrowed Doom's terminology.

    sec = getSector(sectnum);

    if (palookup[sec->ceilingpal] != globalpalwritten)
        globalpalwritten = palookup[sec->ceilingpal];


    globalzd = sec->ceilingz-globalposz;


    if (globalzd > 0)
        return;


    globalpicnum = sec->ceilingpicnum;

    if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES)
        globalpicnum = 0;

    setgotpic(globalpicnum);

    //Check the tile dimension are valid.
    if ((tiles[globalpicnum].dim.width <= 0) ||
        (tiles[globalpicnum].dim.height <= 0))
        return;

    if (tiles[globalpicnum].animFlags&192)
        globalpicnum += animateoffs(globalpicnum);

    TILE_MakeAvailable(globalpicnum);

    globalbufplc = tileData[globalpicnum];
    int tileSize;
    tileSize = tiles[globalpicnum].dim.height *tiles[globalpicnum].dim.width;
    if (isOnExternalFlash(globalbufplc))
    {
    #if WIN32
        if (tileSize> MAX_RAM_FLAT_SIZE)
        {
            FIXME("This is going wrong. Tile size is %d\r\n", tileSize);
        }
    #endif
        globalbufplc = extMemGetDataFromAddress(flat, globalbufplc, tileSize);
    }
#if SEPARATE_SECTOR_CEILINGSHADE
    globalshade = getSector_ceilingshade(sectnum);
#else
    globalshade = (int32_t)sec->ceilingshade;
#endif
    globvis = globalcisibility;
    if (constsector[sectnum].visibility != 0)
        globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[sectnum].visibility+16)));

    globalorientation = (int32_t)sec->ceilingstat;


    if ((globalorientation&64) == 0){
        globalx1 = singlobalang;
        globalx2 = singlobalang;
        globaly1 = cosglobalang;
        globaly2 = cosglobalang;
        globalxpanning = (globalposx<<20);
        globalypanning = -(globalposy<<20);
    }
    else{
        j = constsector[sectnum].wallptr;
        const walltype *wallj = getWall(j);
        const walltype *walljp2 = getWall(constwall[j]._point2);
        ox = walljp2->x - wallj->x;
        oy = walljp2->y - wallj->y;
        i = nsqrtasm(ox*ox+oy*oy);

        if (i == 0)
            i = 1024;
        else
            i = 1048576/i;

        globalx1 = mulscale10(dmulscale10(ox,singlobalang,-oy,cosglobalang),i);
        globaly1 = mulscale10(dmulscale10(ox,cosglobalang,oy,singlobalang),i);
        globalx2 = -globalx1;
        globaly2 = -globaly1;

        ox = ((wallj->x - globalposx)<<6);
        oy = ((wallj->y - globalposy)<<6);
        i = dmulscale14(oy,cosglobalang,-ox,singlobalang);
        j = dmulscale14(ox,cosglobalang,oy,singlobalang);
        ox = i;
        oy = j;
        globalxpanning = globalx1*ox - globaly1*oy;
        globalypanning = globaly2*ox + globalx2*oy;
    }

    globalx2 = mulscale16(globalx2,viewingrangerecip);
    globaly1 = mulscale16(globaly1,viewingrangerecip);
    globalxshift = (8-(picsiz[globalpicnum]&15));
    globalyshift = (8-(picsiz[globalpicnum]>>4));
    if (globalorientation&8) {
        globalxshift++;
        globalyshift++;
    }

    if ((globalorientation&0x4) > 0){
        i = globalxpanning;
        globalxpanning = globalypanning;
        globalypanning = i;
        i = globalx2;
        globalx2 = -globaly1;
        globaly1 = -i;
        i = globalx1;
        globalx1 = globaly2;
        globaly2 = i;
    }
    if ((globalorientation&0x10) > 0){
        globalx1 = -globalx1;
        globaly1 = -globaly1;
        globalxpanning = -globalxpanning;
    }
    if ((globalorientation&0x20) > 0){
        globalx2 = -globalx2;
        globaly2 = -globaly2;
        globalypanning = -globalypanning;
    }

    globalx1 <<= globalxshift;
    globaly1 <<= globalxshift;
    globalx2 <<= globalyshift;
    globaly2 <<= globalyshift;
    globalxpanning <<= globalxshift;
    globalypanning <<= globalyshift;
    globalxpanning += (((int32_t)sec->ceilingxpanning)<<24);
    globalypanning += (((int32_t)sec->ceilingypanning)<<24);
    globaly1 = (-globalx1-globaly1)*halfxdimen;
    globalx2 = (globalx2-globaly2)*halfxdimen;

    sethlinesizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,globalbufplc);

    globalx2 += globaly2*(x1-1);
    globaly1 += globalx1*(x1-1);
    globalx1 = mulscale16(globalx1,globalzd);
    globalx2 = mulscale16(globalx2,globalzd);
    globaly1 = mulscale16(globaly1,globalzd);
    globaly2 = mulscale16(globaly2,globalzd);
    globvis = klabs(mulscale10(globvis,globalzd));

    if (!(globalorientation&0x180))
    {
        y1 = umost[x1];
        y2 = y1;
        for(x=x1; x<=x2; x++)
        {
            twall = umost[x]-1;
            bwall = min(uplc[x],dmost[x]);
            if (twall < bwall-1)
            {
                if (twall >= y2)
                {
                    while (y1 < y2-1) hline(x-1,++y1);
                    y1 = twall;
                }
                else
                {
                    while (y1 < twall)
                        hline(x-1,++y1);
                    while (y1 > twall)
                        lastx[y1--] = x;
                }
                while (y2 > bwall)
                    hline(x-1,--y2);
                while (y2 < bwall)
                    lastx[y2++] = x;
            }
            else
            {
                while (y1 < y2-1)
                    hline(x-1,++y1);
                if (x == x2) {
                    globalx2 += globaly2;
                    globaly1 += globalx1;
                    break;
                }
                y1 = umost[x+1];
                y2 = y1;
            }
            globalx2 += globaly2;
            globaly1 += globalx1;
        }
        while (y1 < y2-1) hline(x2,++y1);
#if FREQUENT_FAKETIMERHANDLER
        faketimerhandler();
#endif
        return;
    }

    switch(globalorientation&0x180)
    {
    case 128:
        msethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 256:
        settrans(TRANS_NORMAL);
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 384:
        settrans(TRANS_REVERSE);
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    }

    y1 = umost[x1];
    y2 = y1;
    for(x=x1; x<=x2; x++)
    {
        twall = umost[x]-1;
        bwall = min(uplc[x],dmost[x]);
        if (twall < bwall-1)
        {
            if (twall >= y2)
            {
                while (y1 < y2-1) slowhline(x-1,++y1);
                y1 = twall;
            }
            else
            {
                while (y1 < twall) slowhline(x-1,++y1);
                while (y1 > twall) lastx[y1--] = x;
            }
            while (y2 > bwall) slowhline(x-1,--y2);
            while (y2 < bwall) lastx[y2++] = x;
        }
        else
        {
            while (y1 < y2-1) slowhline(x-1,++y1);
            if (x == x2) {
                globalx2 += globaly2;
                globaly1 += globalx1;
                break;
            }
            y1 = umost[x+1];
            y2 = y1;
        }
        globalx2 += globaly2;
        globaly1 += globalx1;
    }
    while (y1 < y2-1) slowhline(x2,++y1);
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
}


/* renders non-parallaxed floors. --ryan. */
static void florscan (int32_t x1, int32_t x2, int32_t sectnum)
{
    int32_t i, j, ox, oy, x, y1, y2, twall, bwall;
    const sectortype *sec;
    uint8_t flat[MAX_RAM_FLAT_SIZE];    // next-hack: yes, I have borrowed Doom's terminology.

    //Retrieve the sector object
    sec = getSector(sectnum);

    //Retrieve the floor palette.
    if (palookup[sec->floorpal] != globalpalwritten)
        globalpalwritten = palookup[sec->floorpal];

    globalzd = globalposz-sec->floorz;

    //We are UNDER the floor: Do NOT render anything.
    if (globalzd > 0)
        return;

    //Retrive the floor texture.
    globalpicnum = sec->floorpicnum;
    if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES)
        globalpicnum = 0;

    //Lock the floor texture
    setgotpic(globalpicnum);


    //This tile has unvalid dimensions ( negative)
    if ((tiles[globalpicnum].dim.width <= 0) ||
        (tiles[globalpicnum].dim.height <= 0))
        return;

    //If this is an animated texture: Animate it.
    if (tiles[globalpicnum].animFlags&192)
        globalpicnum += animateoffs(globalpicnum);

    //If the texture is not in RAM: Load it !!
    TILE_MakeAvailable(globalpicnum);

    //Check where is the texture in RAM
    globalbufplc = tileData[globalpicnum];
    int tileSize;
    tileSize = tiles[globalpicnum].dim.height *tiles[globalpicnum].dim.width;
    if (isOnExternalFlash(globalbufplc))
    {
    #if WIN32
        if (tileSize> MAX_RAM_FLAT_SIZE)
        {
            FIXME("This is going wrong. Tile size is %d\r\n", tileSize);
        }
    #endif
        globalbufplc = extMemGetDataFromAddress(flat, globalbufplc, tileSize);
    }

    //Retrieve the shade of the sector (illumination level).
#if SEPARATE_SECTOR_FLOORSHADE
    globalshade = getSector_floorshade(sectnum);
#else
    globalshade = (int32_t)sec->floorshade;
#endif // SEPARATE_SECTOR_FLOORSHADE

    globvis = globalcisibility;
    if (constsector[sectnum].visibility != 0)
        globvis = mulscale4(globvis,(int32_t)((uint8_t)(constsector[sectnum].visibility+16)));


    globalorientation = (int32_t)sec->floorstat;


    if ((globalorientation&64) == 0)
    {
        globalx1 = singlobalang;
        globalx2 = singlobalang;
        globaly1 = cosglobalang;
        globaly2 = cosglobalang;
        globalxpanning = (globalposx<<20);
        globalypanning = -(globalposy<<20);
    }
    else
    {
        j = constsector[sectnum].wallptr;
        const walltype *wallj, *walljp2;
        wallj = getWall(j);
        walljp2 = getWall(constwall[j]._point2);
        ox = walljp2->x - wallj->x;
        oy = walljp2->y - wallj->y;
        i = nsqrtasm(ox*ox+oy*oy);
        if (i == 0)
            i = 1024;
        else
            i = 1048576/i;
        globalx1 = mulscale10(dmulscale10(ox,singlobalang,-oy,cosglobalang),i);
        globaly1 = mulscale10(dmulscale10(ox,cosglobalang,oy,singlobalang),i);
        globalx2 = -globalx1;
        globaly2 = -globaly1;

        ox = ((wallj->x-globalposx)<<6);
        oy = ((wallj->y-globalposy)<<6);
        i = dmulscale14(oy,cosglobalang,-ox,singlobalang);
        j = dmulscale14(ox,cosglobalang,oy,singlobalang);
        ox = i;
        oy = j;
        globalxpanning = globalx1*ox - globaly1*oy;
        globalypanning = globaly2*ox + globalx2*oy;
    }


    globalx2 = mulscale16(globalx2,viewingrangerecip);
    globaly1 = mulscale16(globaly1,viewingrangerecip);
    globalxshift = (8-(picsiz[globalpicnum]&15));
    globalyshift = (8-(picsiz[globalpicnum]>>4));
    if (globalorientation&8) {
        globalxshift++;
        globalyshift++;
    }

    if ((globalorientation&0x4) > 0)
    {
        i = globalxpanning;
        globalxpanning = globalypanning;
        globalypanning = i;
        i = globalx2;
        globalx2 = -globaly1;
        globaly1 = -i;
        i = globalx1;
        globalx1 = globaly2;
        globaly2 = i;
    }


    if ((globalorientation&0x10) > 0){
        globalx1 = -globalx1;
        globaly1 = -globaly1;
        globalxpanning = -globalxpanning;
    }

    if ((globalorientation&0x20) > 0){
        globalx2 = -globalx2;
        globaly2 = -globaly2;
        globalypanning = -globalypanning;
    }


    globalx1 <<= globalxshift;
    globaly1 <<= globalxshift;
    globalx2 <<= globalyshift;
    globaly2 <<= globalyshift;
    globalxpanning <<= globalxshift;
    globalypanning <<= globalyshift;
#if SEPARATE_SECTOR_FLOORXPANNING
    globalxpanning += (((int32_t)getSector_floorxpanning(sectnum))<<24);
#else
    globalxpanning += (((int32_t)sec->floorxpanning)<<24);
#endif // SEPARATE_SECTOR_FLOORXPANNING
    globalypanning += (((int32_t)sec->floorypanning)<<24);
    globaly1 = (-globalx1-globaly1)*halfxdimen;
    globalx2 = (globalx2-globaly2)*halfxdimen;

    //Setup the drawing routine paramters
    sethlinesizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,globalbufplc);

    globalx2 += globaly2*(x1-1);
    globaly1 += globalx1*(x1-1);
    globalx1 = mulscale16(globalx1,globalzd);
    globalx2 = mulscale16(globalx2,globalzd);
    globaly1 = mulscale16(globaly1,globalzd);
    globaly2 = mulscale16(globaly2,globalzd);
    globvis = klabs(mulscale10(globvis,globalzd));

    if (!(globalorientation&0x180))
    {
        y1 = max(dplc[x1],umost[x1]);
        y2 = y1;
        for(x=x1; x<=x2; x++)
        {
            twall = max(dplc[x],umost[x])-1;
            bwall = dmost[x];
            if (twall < bwall-1)
            {
                if (twall >= y2)
                {
                    while (y1 < y2-1)
                        hline(x-1,++y1);
                    y1 = twall;
                }
                else
                {
                    while (y1 < twall)
                        hline(x-1,++y1);
                    while (y1 > twall)
                        lastx[y1--] = x;
                }
                while (y2 > bwall)
                    hline(x-1,--y2);
                while (y2 < bwall)
                    lastx[y2++] = x;
            }
            else
            {
                while (y1 < y2-1) hline(x-1,++y1);
                if (x == x2) {
                    globalx2 += globaly2;
                    globaly1 += globalx1;
                    break;
                }
                y1 = max(dplc[x+1],umost[x+1]);
                y2 = y1;
            }
            globalx2 += globaly2;
            globaly1 += globalx1;
        }
        while (y1 < y2-1)
            hline(x2,++y1);
#if FREQUENT_FAKETIMERHANDLER
        faketimerhandler();
#endif
        return;
    }

    switch(globalorientation&0x180)
    {
    case 128:
        msethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 256:
        settrans(TRANS_NORMAL);
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 384:
        settrans(TRANS_REVERSE);
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    }

    y1 = max(dplc[x1],umost[x1]);
    y2 = y1;
    for(x=x1; x<=x2; x++)
    {
        twall = max(dplc[x],umost[x])-1;
        bwall = dmost[x];
        if (twall < bwall-1)
        {
            if (twall >= y2)
            {
                while (y1 < y2-1) slowhline(x-1,++y1);
                y1 = twall;
            }
            else
            {
                while (y1 < twall)
                    slowhline(x-1,++y1);
                while (y1 > twall)
                    lastx[y1--] = x;
            }
            while (y2 > bwall)
                slowhline(x-1,--y2);
            while (y2 < bwall)
                lastx[y2++] = x;
        }
        else
        {
            while (y1 < y2-1)
                slowhline(x-1,++y1);
            if (x == x2) {
                globalx2 += globaly2;
                globaly1 += globalx1;
                break;
            }
            y1 = max(dplc[x+1],umost[x+1]);
            y2 = y1;
        }
        globalx2 += globaly2;
        globaly1 += globalx1;
    }
    while (y1 < y2-1)
        slowhline(x2,++y1);
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
}
/*
 * renders walls and parallaxed skies/floors. Look at parascan() for the
 *  higher level of parallaxing.
 *
 *    x1 == offset of leftmost pixel of wall. 0 is left of surface.
 *    x2 == offset of rightmost pixel of wall. 0 is left of surface.
 *
 *  apparently, walls are always vertical; there are sloping functions
 *   (!!!) er...elsewhere. Only the sides need be vertical, as the top and
 *   bottom of the polygon will need to be angled as the camera perspective
 *   shifts (user spins in a circle, etc.)
 *
 *  uwal is an array of the upper most pixels, and dwal are the lower most.
 *   This must be a list, as the top and bottom of the polygon are not
 *   necessarily horizontal lines.
 *
 *   So, the screen coordinate of the top left of a wall is specified by
 *   uwal[x1], the bottom left by dwal[x1], the top right by uwal[x2], and
 *   the bottom right by dwal[x2]. Every physical point on the edge of the
 *   wall in between is specified by traversing those arrays, one pixel per
 *   element.
 *
 *  --ryan.
 */
static void wallscan(int32_t x1, int32_t x2,
                     int16_t *uwal, int16_t *dwal,
                     swall_t *swal, lwall_t *lwal)
{
    int32_t x, xnice;
    const uint8_t* fpalookup;
    int32_t y1ve[2], y2ve[2];
    uint32_t tileWidth, tileHeight;
    uint8_t coldata[2][MAX_COL_BUF];
    tileWidth = tiles[globalpicnum].dim.width;
    tileHeight = tiles[globalpicnum].dim.height;

    setgotpic(globalpicnum);
    // don't draw 0-sized walls
    if ((tileWidth <= 0) || (tileHeight <= 0))
        return;
    // don't draw walls below the bottom of the screen
    if ((uwal[x1] > ydimen) && (uwal[x2] > ydimen))
        return;
    // don't draw walls above top of the screen
    if ((dwal[x1] < 0) && (dwal[x2] < 0))
        return;

    TILE_MakeAvailable(globalpicnum);

    xnice = ( (uint32_t) pow2long[picsiz[globalpicnum]&15] ==  tileWidth);
    if (xnice)
        tileWidth--;

   /* ynice = (pow2long[picsiz[globalpicnum]>>4] == tsizy);
    if (ynice)
        tsizy = (picsiz[globalpicnum]>>4);*/

    fpalookup = palookup[globalpal];

    setupvlineasm(globalshiftval);

    //Starting on the left column of the wall, check the occlusion arrays.
    x = x1;
    // this will skip all the columns where the view to the wall is occluded.
    while ((umost[x] > dmost[x]) && (x <= x2))
        x++;
    if (isOnExternalFlash(tileData[globalpicnum]))
    {
      // next-hack. To reduce unnecessary loads from external flash, which take
      // a lot of time, let's spend some time here to understand how much we shall load.
      unsigned int minY = 0xFFFFFFFF, maxY = 0;

      int32_t colHeight = 0;
      int xx;
      for(xx = x; (xx<=x2); xx++)
      {
       //   printf(" 1 tileHeight: %d\r\n", tileHeight);
          y1ve[0] = max(uwal[xx],umost[xx]);
          y2ve[0] = min(dwal[xx],dmost[xx]);
          if (y2ve[0] <= y1ve[0])
              continue;
          vince[0] = swal[xx]*globalyscale;
          vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);
          //
          // calculate where will the start and stop pixels be, using 64-bit integers.
          // so that we can easily understand when we are out of bounds.
          int numIncrements = (y2ve[0]-y1ve[0] - 1);  // y2ve - y1ve is at least 1 or larger
          int64_t startPos64 = (uint32_t) vplce[0];
          int64_t stopPos64 = startPos64 + (uint64_t) numIncrements * vince[0];

          if (!((tileHeight - 1) & tileHeight))  // power of two ?
          {
              // next-hack: textures having power-of-2 height can have yrepeat.
              // If you try on non-power-of-2, then you will get crap on the
              // original duke.
              // Todo check: one might have a particular offset + repeat combination
              // for which the texture is partially displayed and it is fine...
              //
              // get the unsigned value
              uint64_t uStopPos64 = stopPos64;
              // if stop overflowed, then we need to load the entire column. Stop now.
              if (uStopPos64 >> 32)
          {
              minY = 0;
              colHeight = tileHeight;
                  break;
              }
              else
              {   // we can use directly start and stop values computed using 32 bit arithmetics.
                  uint32_t startY = (uint32_t) vplce[0] >> globalshiftval;
                  uint32_t stopY = (uint32_t) (vplce[0] + numIncrements * vince[0]) >> globalshiftval;
                  //
                  if (minY > startY)
                  {
                      minY = startY;
                  }
                  if (minY > stopY)
                  {
                      minY = stopY;
                  }
                  if (maxY < startY)
                  {
                      maxY = startY;
                  }
                  if (maxY < stopY)
                  {
                      maxY = stopY;
                  }
                  colHeight = maxY - minY + 1;
  #if 0
                  if (colHeight >= tileHeight)
                  {
                     colHeight = tileHeight;
                     minY = 0;
                     break;
                  }
  #endif
          }
      }
      else
      {

              // If the column is not a power of two, it's more complex to fix.
              // swall both affects start and stop pixels.
              // not power of two is different.
              uint32_t startY = (uint32_t) vplce[0] >> globalshiftval;
              if (startY >= tileHeight)
              {
                  minY = 0;
                  colHeight = 1 <<( 1 + (picsiz[globalpicnum]>>4)); // this will load some crap after. but who cares.
                  break;

              }
              uint64_t uStopPos64 = stopPos64;    // note: if stopPos64<0, uStopPos64 will be an exceptionally huge number
              // if anyone overflowed, then we need to load the entire column. Stop now.
              if ((uStopPos64 >> globalshiftval) >= tileHeight)
              {
                  minY = 0;
                  colHeight = 1 << ( 1+ ( picsiz[globalpicnum]>>4));  // this will load some crap after. but who cares.
                  break;
              }
              else
              {   // we can use directly start and stop values computed using 32 bit arithmetics.
                 // uint32_t startY = (uint32_t) vplce[0] >> globalshiftval;
                  uint32_t stopY = (uint32_t) (vplce[0] + numIncrements * vince[0]) >> globalshiftval;
                  //
                  if (minY > startY)
                  {
                      minY = startY;
                  }
                  if (minY > stopY)
                  {
                      minY = stopY;
                  }
                  if (maxY < startY)
                  {
                      maxY = startY;
                  }
                  if (maxY < stopY)
                  {
                      maxY = stopY;
      }
                  colHeight = maxY  - minY + 1;
              }
          }
      }
      if (minY == 0x7FFFFFFF)
      {
          return; // no draw
      }
     // printf("Choosen: min %d, max %d, colHeight %d, tileHeight was: %d. xx is %d, x2: %d, x %d\r\n", minY, maxY, colHeight, tileHeight, xx, x2, x);
  // next-hack: asynchronous column load
      int col = 0;
      int bufcol = 0;
      unsigned int oldColumn = 0xFFFFFFFF;
      uint8_t *tilebufpos;
      uint8_t *nexttilebufpos;
      // skip first unseen columns
      int oldx = x2 + 1;
      for(; (x<=x2); x++)
      {
          y1ve[col] = max(uwal[x],umost[x]);
          y2ve[col] = min(dwal[x],dmost[x]);
          if (y2ve[col] <= y1ve[col])
              continue;
          bufplce[col] = lwal[x] + globalxpanning;
          if (bufplce[col] >= (int) tileWidth)
          {
              if (xnice == col)
                  bufplce[col] %= tileWidth;
              else
                  bufplce[col] &= tileWidth;
          }
          palookupoffse[col] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);
          // LOAD COLUMN
          tilebufpos = bufplce[col]*tileHeight + (uint8_t*) tileData[globalpicnum];
       //   if (isOnExternalFlash(tilebufpos))
          {
  #if WIN32
              if (colHeight > MAX_COL_SIZE)
              {
                  FIXME("This is going wrong. Tile height is %d\r\n", colHeight);
              }
  #endif
              tilebufpos = (uint8_t*) readColumnAlignedAsync(coldata[bufcol], (uint8_t*) tilebufpos + minY, colHeight) - minY;
              bufcol = 1 - bufcol;
          }
          vince[col] = swal[x]*globalyscale;
          vplce[col] = globalzd + vince[col]*(y1ve[col]-globalhoriz+1);

          oldColumn = bufplce[col];
          oldx = x;
          x++;
          break;
      }
      col = 1;
      for(; (x<=x2); x++)
      {
          y1ve[col] = max(uwal[x],umost[x]);
          y2ve[col] = min(dwal[x],dmost[x]);
          if (y2ve[col] <= y1ve[col])
              continue;
          bufplce[col] = lwal[x] + globalxpanning;

          palookupoffse[col] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

          if (bufplce[col] >= (int) tileWidth)
          {
              if (xnice == 0)
                  bufplce[col] %= tileWidth;
              else
                  bufplce[col] &= tileWidth;
          }
          if (oldColumn != (uint32_t) bufplce[col])
          {
              nexttilebufpos = bufplce[col]*tileHeight + (uint8_t*) tileData[globalpicnum];
           //   if (isOnExternalFlash(nexttilebufpos))
              {
  #if WIN32
                  if (colHeight > MAX_COL_SIZE)
                  {
                      FIXME("This is going wrong. Tile height is %d\r\n", colHeight);
                  }
                  if (colHeight == 0)
                      FIXME("COL HEIGHT is 0");
                  memset(coldata[bufcol], 251, sizeof(coldata[bufcol]));
  #endif // WIN32
                  extMemWaitAsynchDataRead();
                  nexttilebufpos = (uint8_t*) readColumnAlignedAsync(coldata[bufcol], (uint8_t*) nexttilebufpos + minY, colHeight) - minY;
                  bufcol = 1 - bufcol;
              }
              oldColumn = bufplce[col];
          }
          else
          {
              //if (isOnExternalFlash(tilebufpos))  // still must wait before drawing
                  extMemWaitAsynchDataRead();
              nexttilebufpos = tilebufpos;
          }

          vince[col] = swal[x]*globalyscale;
          vplce[col] = globalzd + vince[col]*(y1ve[col]-globalhoriz+1);

        const uint8_t *palette = palookupoffse[1- col];
          palette = getPalette(palette, y2ve[1 - col]-y1ve[1 - col]);
          waitRenderLine(y2ve[1 - col]);
          vlineasm1(vince[1 - col],palette, y2ve[1 - col]-y1ve[1 - col]-1,vplce[1 - col], tilebufpos ,oldx+frameoffset+ylookup(y1ve[1 - col]));
          tilebufpos = nexttilebufpos;
          col = 1 - col;
          oldx = x;
      }
      if (oldx <= x2)
      {
          // this means last was not consumed
          //if (isOnExternalFlash(tilebufpos))  // still must wait before drawing
              extMemWaitAsynchDataRead();
        const uint8_t *palette = palookupoffse[1- col];
          palette = getPalette(palette, y2ve[1 - col]-y1ve[1 - col]);
          vlineasm1(vince[1 - col],palette, y2ve[1 - col]-y1ve[1 - col]-1,vplce[1 - col], tilebufpos ,oldx+frameoffset+ylookup(y1ve[1 - col]));
      }
    }
    else
    {
      int y1, y2;
        for(; (x<=x2); x++)
          {
              y1 = max(uwal[x],umost[x]);
              y2 = min(dwal[x],dmost[x]);
              if (y2 <= y1)
                  continue;
              bufplce[0] = lwal[x] + globalxpanning;

              palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

              if (bufplce[0] >= (int) tileWidth)
              {
                  if (xnice == 0)
                      bufplce[0] %= tileWidth;
                  else
                      bufplce[0] &= tileWidth;
              }
              uint8_t *tilebufpos = bufplce[0]*tileHeight + (uint8_t*) tileData[globalpicnum];

              vince[0] = swal[x]*globalyscale;
              vplce[0] = globalzd + vince[0]*(y1-globalhoriz+1);

              const uint8_t *palette = palookupoffse[0];
              palette = getPalette(palette, y2 - y1);
              waitRenderLine(y2);
              vlineasm1(vince[0],palette, y2-y1-1,vplce[0], tilebufpos ,x+frameoffset+ylookup(y1));
          }
    }
#if !RENDERING_SIMPLIFICATION
    for(; x<=x2-3; x+=4)
    {
        bad = 0;
        for(z=3; z>=0; z--)
        {
            y1ve[z] = max(uwal[x+z],umost[x+z]);
            y2ve[z] = min(dwal[x+z],dmost[x+z])-1;

            if (y2ve[z] < y1ve[z])
            {
                bad += pow2char[z];
                continue;
            }

            i = lwal[x+z] + globalxpanning;
            if (i >= tileWidth) {
                if (xnice == 0) i %= tileWidth;
                else i &= tileWidth;
            }
            if (ynice == 0)
                i *= tsizy;
            else
                i <<= tsizy;
            bufplce[z] = (intptr_t) tileData[globalpicnum]+i;

            vince[z] = swal[x+z]*globalyscale;
            vplce[z] = globalzd + vince[z]*(y1ve[z]-globalhoriz+1);
        }

        if (bad == 15)
            continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);
        palookupoffse[3] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+3],globvis),globalshade)<<8);

        if ((palookupoffse[0] == palookupoffse[3]) && ((bad&0x9) == 0))
        {
            palookupoffse[1] = palookupoffse[0];
            palookupoffse[2] = palookupoffse[0];
        }
        else
        {
            palookupoffse[1] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+1],globvis),globalshade)<<8);
            palookupoffse[2] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+2],globvis),globalshade)<<8);
        }

        u4 = max(max(y1ve[0],y1ve[1]),max(y1ve[2],y1ve[3]));
        d4 = min(min(y2ve[0],y2ve[1]),min(y2ve[2],y2ve[3]));

        if ((bad != 0) || (u4 >= d4))
        {
            if (!(bad&1))
                prevlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0],vplce[0],(uint8_t*) bufplce[0],ylookup(y1ve[0])+x+frameoffset+0);
            if (!(bad&2))
                prevlineasm1(vince[1],palookupoffse[1],y2ve[1]-y1ve[1],vplce[1],(uint8_t*) bufplce[1],ylookup(y1ve[1])+x+frameoffset+1);
            if (!(bad&4))
                prevlineasm1(vince[2],palookupoffse[2],y2ve[2]-y1ve[2],vplce[2],(uint8_t*) bufplce[2],ylookup(y1ve[2])+x+frameoffset+2);
            if (!(bad&8))
                prevlineasm1(vince[3],palookupoffse[3],y2ve[3]-y1ve[3],vplce[3],(uint8_t*) bufplce[3],ylookup(y1ve[3])+x+frameoffset+3);
            continue;
        }

        if (u4 > y1ve[0])
            vplce[0] =prevlineasm1(vince[0],palookupoffse[0],u4-y1ve[0]-1,vplce[0],(uint8_t*) bufplce[0],ylookup(y1ve[0])+x+frameoffset+0);
        if (u4 > y1ve[1])
            vplce[1] = prevlineasm1(vince[1],palookupoffse[1],u4-y1ve[1]-1,vplce[1],(uint8_t*) bufplce[1],ylookup(y1ve[1])+x+frameoffset+1);
        if (u4 > y1ve[2])
            vplce[2] = prevlineasm1(vince[2],palookupoffse[2],u4-y1ve[2]-1,vplce[2],(uint8_t*) bufplce[2],ylookup(y1ve[2])+x+frameoffset+2);
        if (u4 > y1ve[3])
            vplce[3] = prevlineasm1(vince[3],palookupoffse[3],u4-y1ve[3]-1,vplce[3],(uint8_t*) bufplce[3],ylookup(y1ve[3])+x+frameoffset+3);

        if (d4 >= u4)
            vlineasm4(d4-u4+1,ylookup(u4)+x+frameoffset);

        i = x+frameoffset+ylookup(d4+1);

        if (y2ve[0] > d4)
            prevlineasm1(vince[0],palookupoffse[0],y2ve[0]-d4-1,vplce[0],(uint8_t*) bufplce[0],(uint8_t*) i+0);
        if (y2ve[1] > d4)
            prevlineasm1(vince[1],palookupoffse[1],y2ve[1]-d4-1,vplce[1],(uint8_t*) bufplce[1],(uint8_t*) i+1);
        if (y2ve[2] > d4)
            prevlineasm1(vince[2],palookupoffse[2],y2ve[2]-d4-1,vplce[2],(uint8_t*) bufplce[2],(uint8_t*) i+2);
        if (y2ve[3] > d4)
            prevlineasm1(vince[3],palookupoffse[3],y2ve[3]-d4-1,vplce[3],(uint8_t*) bufplce[3],(uint8_t*) i+3);
    }
    for(; x<=x2; x++)
    {
        y1ve[0] = max(uwal[x],umost[x]);
        y2ve[0] = min(dwal[x],dmost[x]);
        if (y2ve[0] <= y1ve[0])
            continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

        bufplce[0] = lwal[x] + globalxpanning;
        if (bufplce[0] >= tileWidth) {
            if (xnice == 0)
                bufplce[0] %= tileWidth;
            else
                bufplce[0] &= tileWidth;
        }

        if (ynice == 0) bufplce[0]
            *= tsizy;
        else
            bufplce[0] <<= tsizy;

        vince[0] = swal[x]*globalyscale;
        vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);

        vlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0]+tileData[globalpicnum],x+frameoffset+ylookup(y1ve[0]));
    }
#endif
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
}


/* this renders masking sprites. See wallscan(). --ryan. */
static void maskwallscan(int32_t x1, int32_t x2,
                         short *uwal, short *dwal,
                         swall_t *swal, lwall_t *lwal)
{
    int32_t x, startx, xnice;
    const uint8_t* fpalookup;
    int32_t y1ve[4], y2ve[4];
    uint32_t tileWidth, tileHeight;
    uint8_t coldata[2][MAX_COL_BUF];
    tileWidth = tiles[globalpicnum].dim.width;
    tileHeight = tiles[globalpicnum].dim.height;
    setgotpic(globalpicnum);

    if ((tileWidth <= 0) || (tileHeight <= 0))
        return;
    if ((uwal[x1] > ydimen) && (uwal[x2] > ydimen))
        return;
    if ((dwal[x1] < 0) && (dwal[x2] < 0))
        return;

    TILE_MakeAvailable(globalpicnum);

    startx = x1;

    xnice = ((uint32_t) pow2long[picsiz[globalpicnum]&15] == tileWidth);
    if (xnice)
        tileWidth = (tileWidth-1);

/*    ynice = (pow2long[picsiz[globalpicnum]>>4] == tileHeight);
    if (ynice)
        tileHeight = (picsiz[globalpicnum]>>4);*/

    fpalookup = palookup[globalpal];

    setupmvlineasm(globalshiftval);

    x = startx;
    while ((startumost(x+windowx1) > startdmost(x+windowx1)) && (x <= x2)) x++;
#if 0
    int minY = 0;
    int colHeight = tileHeight;
    int col = 0;
#else //
   // next-hack. To reduce unnecessary loads from external flash, which take
    // a lot of time, let's spend some time here to understand how much we shall load.
        unsigned int minY = 0xFFFFFFFF, maxY = 0;
        int32_t colHeight = 0;
    int xx;
    for(xx = x; (xx<=x2); xx++)
        {
            y1ve[0] = max(uwal[xx],startumost(x+windowx1)-windowy1);
            y2ve[0] = min(dwal[xx],startdmost(x+windowx1)-windowy1);
            if (y2ve[0] <= y1ve[0])
                continue;
            vince[0] = swal[xx]*globalyscale;
            vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);
        //
        // calculate where will the start and stop pixels be, using 64-bit integers.
        // so that we can easily understand when we are out of bounds.
        int numIncrements = (y2ve[0]-y1ve[0] - 1);  // y2ve - y1ve is at least 1 or larger
        int64_t startPos64 = (uint32_t) vplce[0];
        int64_t stopPos64 = startPos64 + (uint64_t) numIncrements * vince[0];

        if (!((tileHeight - 1) & tileHeight))  // power of two ?
        {
            // next-hack: textures having power-of-2 height can have yrepeat.
            // If you try on non-power-of-2, then you will get crap on the
            // original duke.
            // Todo check: one might have a particular offset + repeat combination
            // for which the texture is partially displayed and it is fine...
            //
            // get the unsigned value
            uint64_t uStopPos64 = stopPos64;
            // if stop overflowed, then we need to load the entire column. Stop now.
            if (uStopPos64 >> 32)
            {
                minY = 0;
                colHeight = tileHeight;
                break;
            }
            else
            {   // we can use directly start and stop values computed using 32 bit arithmetics.
                uint32_t startY = (uint32_t) vplce[0] >> globalshiftval;
                uint32_t stopY = (uint32_t) (vplce[0] + numIncrements * vince[0]) >> globalshiftval;
                //
                if (minY > startY)
                {
                    minY = startY;
                }
                if (minY > stopY)
                {
                    minY = stopY;
                }
                if (maxY < startY)
                {
                    maxY = startY;
                }
                if (maxY < stopY)
                {
                    maxY = stopY;
                }
                colHeight = maxY - minY + 1;
            }
        }
        else
        {

            // If the column is not a power of two, it's more complex to fix.
            // swall both affects start and stop pixels.
            // not power of two is different.
            uint32_t startY = (uint32_t) vplce[0] >> globalshiftval;
            if (startY >= tileHeight)
            {
            minY = 0;
                colHeight = 1 <<( 1 + (picsiz[globalpicnum]>>4)); // this will load some crap after. but who cares.
#if WIN32 & 0
                printf("startY %d, tileH %d colH %d\r\n", startY, tileHeight, colHeight);
#endif
                break;

            }
            uint64_t uStopPos64 = stopPos64;    // note: if stopPos64<0, uStopPos64 will be an exceptionally huge number
            // if anyone overflowed, then we need to load the entire column. Stop now.
            if ((uStopPos64 >> globalshiftval) >= tileHeight)
            {
                minY = 0;
                colHeight = 1 << ( 1+ ( picsiz[globalpicnum]>>4));  // this will load some crap after. but who cares.
#if WIN32 & 0
                printf("ustopPos %d, tileH %d colH %d\r\n", (uint32_t) (uStopPos64 >> globalshiftval), tileHeight, colHeight);
#endif
                break;
        }
            else
            {   // we can use directly start and stop values computed using 32 bit arithmetics.
               // uint32_t startY = (uint32_t) vplce[0] >> globalshiftval;
                uint32_t stopY = (uint32_t) (vplce[0] + numIncrements * vince[0]) >> globalshiftval;
                //
                if (minY > startY)
                {
                    minY = startY;
                }
                if (minY > stopY)
                {
                    minY = stopY;
                }
                if (maxY < startY)
                {
                    maxY = startY;
                }
                if (maxY < stopY)
                {
                    maxY = stopY;
                }
                colHeight = maxY  - minY + 1;
            }
        }
    }
    if (minY == 0x7FFFFFFF)
    {
        return; // no draw
    }
    int col = 0;

    #endif
    const uint8_t *tileBuffer = tileData[globalpicnum];
    if (useRenderedCamData() && (globalpicnum == VIEWSCREEN || globalpicnum == VIEWSCREEN2))
    {
        tileBuffer = (uint8_t *) get_framebuffer() + SCREENWIDTH * (SCREENHEIGHT - STATUS_BAR_HEIGHT);
    }
    if (isOnExternalFlash(tileBuffer))
    {
   // printf("Choosen: min %d, max %d, colHeight %d, tileHeight was: %d. xx is %d, x2: %d, x %d\r\n", minY, maxY, colHeight, tileHeight, xx, x2, x);
    // next-hack: asynchronous column load
        unsigned int oldColumn = 0xFFFFFFFF;
        int bufcol = 0;
        uint8_t *tilebufpos = NULL;
        uint8_t *nexttilebufpos;
        // skip first unseen columns
        int oldx = x2 + 1;
        for(; (x<=x2); x++)
        {
            y1ve[col] = max(uwal[x],startumost(x+windowx1)-windowy1);
            y2ve[col] = min(dwal[x],startdmost(x+windowx1)-windowy1);
            if (y2ve[col] <= y1ve[col])
                continue;

            bufplce[col] = lwal[x] + globalxpanning;
            if (bufplce[col] >= (int) tileWidth)
            {
                if (xnice == col)
                    bufplce[col] %= tileWidth;
                else
                    bufplce[col] &= tileWidth;
            }
            palookupoffse[col] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);
            // LOAD COLUMN
        tilebufpos = bufplce[col]*tileHeight + (uint8_t*) tileBuffer;
            if (isOnExternalFlash(tilebufpos))
            {
    #if WIN32
                if (colHeight > MAX_COL_SIZE)
                {
                    FIXME("This is going wrong. Tile height is %d\r\n", colHeight);
                }
    #endif
                tilebufpos = (uint8_t*) readColumnAlignedAsync(coldata[bufcol], (uint8_t*) tilebufpos + minY, colHeight) - minY;
                bufcol = 1 - bufcol;
            }
            vince[col] = swal[x]*globalyscale;
            vplce[col] = globalzd + vince[col]*(y1ve[col]-globalhoriz+1);

            oldColumn = bufplce[col];
            oldx = x;
            x++;
            break;
        }
        col = 1;
        for(; (x<=x2); x++)
        {
            y1ve[col] = max(uwal[x],startumost(x+windowx1)-windowy1);
            y2ve[col] = min(dwal[x],startdmost(x+windowx1)-windowy1);
            if (y2ve[col] <= y1ve[col])
                continue;
            bufplce[col] = lwal[x] + globalxpanning;

            palookupoffse[col] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

            if (bufplce[col] >= (int) tileWidth)
            {
                if (xnice == 0)
                    bufplce[col] %= tileWidth;
                else
                    bufplce[col] &= tileWidth;
            }
            if (oldColumn != (uint32_t) bufplce[col])
            {
            nexttilebufpos = bufplce[col]*tileHeight + (uint8_t*) tileBuffer;
  #if WIN32
              if (colHeight > MAX_COL_SIZE)
              {
                  FIXME("This is going wrong. Tile height is %d\r\n", colHeight);
              }
  #endif
              extMemWaitAsynchDataRead(); // wait before load next column
              nexttilebufpos = (uint8_t*) readColumnAlignedAsync(coldata[bufcol], (uint8_t*) nexttilebufpos + minY, colHeight) - minY;
              bufcol = 1 - bufcol;
              oldColumn = bufplce[col];
            }
            else
            {
              extMemWaitAsynchDataRead();
              nexttilebufpos = tilebufpos;
            }

                const uint8_t *palette = palookupoffse[1- col];
            palette = getPalette(palette, y2ve[1 - col]-y1ve[1 - col]);
            waitRenderLine(y2ve[1 - col]);
            vince[col] = swal[x]*globalyscale;
            vplce[col] = globalzd + vince[col]*(y1ve[col]-globalhoriz+1);

            mvlineasm1(vince[1 - col],palette ,y2ve[1 - col]-y1ve[1 - col]-1,vplce[1 - col], tilebufpos ,oldx+frameoffset+ylookup(y1ve[1 - col]));
            tilebufpos = nexttilebufpos;
            col = 1 - col;
            oldx = x;
        }
        if (oldx <= x2)
        {
          //if (isOnExternalFlash(tilebufpos))
            extMemWaitAsynchDataRead();
            // this means last was not consumed
              const uint8_t *palette = palookupoffse[1- col];
          palette = getPalette(palette, y2ve[1 - col]-y1ve[1 - col]);

          mvlineasm1(vince[1 - col],palette ,y2ve[1 - col]-y1ve[1 - col]-1,vplce[1 - col], tilebufpos ,oldx+frameoffset+ylookup(y1ve[1 - col]));
        }
    }
    else
    {
        int y1, y2;
        for(; (x<=x2); x++)
        {
            y1 = max(uwal[x],startumost(x+windowx1)-windowy1);
            y2 = min(dwal[x],startdmost(x+windowx1)-windowy1);
            if (y2 <= y1)
                continue;
            bufplce[0] = lwal[x] + globalxpanning;

            palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

            if (bufplce[0] >= (int) tileWidth)
            {
                if (xnice == 0)
                    bufplce[0] %= tileWidth;
                else
                    bufplce[0] &= tileWidth;
            }
                const uint8_t *palette = palookupoffse[0];
                palette = getPalette(palette, y2-y1);
                waitRenderLine(y2);
                vince[0] = swal[x]*globalyscale;
                vplce[0] = globalzd + vince[0]*(y1-globalhoriz+1);
                uint8_t * tilebufpos = bufplce[0]*tileHeight + (uint8_t*) tileBuffer;
                mvlineasm1(vince[0],palette ,y2-y1-1,vplce[0], tilebufpos ,x + frameoffset+ylookup(y1));
        }

    }
 #if !RENDERING_SIMPLIFICATION
    for(; x<=x2-3; x+=4,p+=4)
    {
        bad = 0;
        for(z=3,dax=x+3; z>=0; z--,dax--)
        {
            y1ve[z] = max(uwal[dax],startumost(dax+windowx1)-windowy1);
            y2ve[z] = min(dwal[dax],startdmost(dax+windowx1)-windowy1)-1;
            if (y2ve[z] < y1ve[z]) {
                bad += pow2char[z];
                continue;
            }

            i = lwal[dax] + globalxpanning;
            if (i >= tileWidth) {
                if (xnice == 0) i %= tileWidth;
                else i &= tileWidth;
            }

            if (ynice == 0)
                i *= tileHeight;
            else
                i <<= tileHeight;

            bufplce[z] = tileData[globalpicnum]+i;

            vince[z] = swal[dax]*globalyscale;
            vplce[z] = globalzd + vince[z]*(y1ve[z]-globalhoriz+1);
        }
        if (bad == 15) continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);
        palookupoffse[3] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+3],globvis),globalshade)<<8);

        if ((palookupoffse[0] == palookupoffse[3]) && ((bad&0x9) == 0))
        {
            palookupoffse[1] = palookupoffse[0];
            palookupoffse[2] = palookupoffse[0];
        }
        else
        {
            palookupoffse[1] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+1],globvis),globalshade)<<8);
            palookupoffse[2] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+2],globvis),globalshade)<<8);
        }

        u4 = max(max(y1ve[0],y1ve[1]),max(y1ve[2],y1ve[3]));
        d4 = min(min(y2ve[0],y2ve[1]),min(y2ve[2],y2ve[3]));

        if ((bad > 0) || (u4 >= d4))
        {
            if (!(bad&1)) mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0],vplce[0],(const uint8_t*)bufplce[0],ylookup(y1ve[0])+p+0);
            if (!(bad&2)) mvlineasm1(vince[1],palookupoffse[1],y2ve[1]-y1ve[1],vplce[1],(const uint8_t*)bufplce[1],ylookup(y1ve[1])+p+1);
            if (!(bad&4)) mvlineasm1(vince[2],palookupoffse[2],y2ve[2]-y1ve[2],vplce[2],(const uint8_t*)bufplce[2],ylookup(y1ve[2])+p+2);
            if (!(bad&8)) mvlineasm1(vince[3],palookupoffse[3],y2ve[3]-y1ve[3],vplce[3],(const uint8_t*)bufplce[3],ylookup(y1ve[3])+p+3);
            continue;
        }

        if (u4 > y1ve[0]) vplce[0] = mvlineasm1(vince[0],palookupoffse[0],u4-y1ve[0]-1,vplce[0],(const uint8_t*)bufplce[0],ylookup(y1ve[0])+p+0);
        if (u4 > y1ve[1]) vplce[1] = mvlineasm1(vince[1],palookupoffse[1],u4-y1ve[1]-1,vplce[1],(const uint8_t*)bufplce[1],ylookup(y1ve[1])+p+1);
        if (u4 > y1ve[2]) vplce[2] = mvlineasm1(vince[2],palookupoffse[2],u4-y1ve[2]-1,vplce[2],(const uint8_t*)bufplce[2],ylookup(y1ve[2])+p+2);
        if (u4 > y1ve[3]) vplce[3] = mvlineasm1(vince[3],palookupoffse[3],u4-y1ve[3]-1,vplce[3],(const uint8_t*)bufplce[3],ylookup(y1ve[3])+p+3);

        if (d4 >= u4) mvlineasm4(d4-u4+1,ylookup(u4)+p);

        i = p+ylookup(d4+1);
        if (y2ve[0] > d4) mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-d4-1,vplce[0],(const uint8_t*)bufplce[0],(uint8_t*)i+0);
        if (y2ve[1] > d4) mvlineasm1(vince[1],palookupoffse[1],y2ve[1]-d4-1,vplce[1],(const uint8_t*)bufplce[1],(uint8_t*)i+1);
        if (y2ve[2] > d4) mvlineasm1(vince[2],palookupoffse[2],y2ve[2]-d4-1,vplce[2],(const uint8_t*)bufplce[2],(uint8_t*)i+2);
        if (y2ve[3] > d4) mvlineasm1(vince[3],palookupoffse[3],y2ve[3]-d4-1,vplce[3],(const uint8_t*)bufplce[3],(uint8_t*)i+3);
    }
    for(; x<=x2; x++,p++)
    {
        y1ve[0] = max(uwal[x],startumost(x+windowx1)-windowy1);
        y2ve[0] = min(dwal[x],startdmost(x+windowx1)-windowy1);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

        bufplce[0] = lwal[x] + globalxpanning;
        if (bufplce[0] >= tileWidth) {
            if (xnice == 0) bufplce[0] %= tileWidth;
            else bufplce[0] &= tileWidth;
        }
        if (ynice == 0)
            bufplce[0] *= tileHeight;
        else
            bufplce[0] <<= tileHeight;

        vince[0] = swal[x]*globalyscale;
        vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);

        mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0]+tileData[globalpicnum],p+ylookup(y1ve[0]));
    }
#endif
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
}

/* renders parallaxed skies/floors  --ryan. */
static void parascan(int32_t dax1, int32_t dax2, int32_t sectnum,uint8_t  dastat, int32_t bunch)
{
    UNUSED(dax1);
    UNUSED(dax2);
    // next-hack: lplc and swplc were originally global. These can be in stack instead.
    lwall_t lplc[MAXXDIM+1];
    swplc_t swplc[MAXXDIM+1];
    const sectortype *sec;
    int32_t j, k, l, m, n, x, z, wallnum, nextsectnum, globalhorizbak;
    short *topptr, *botptr;
    sectnum = get_pvWalls_sectorId(bunchfirst[bunch]);
    sec = getSector(sectnum);

    globalhorizbak = globalhoriz;
    if (parallaxyscale != 65536)
        globalhoriz = mulscale16(globalhoriz-(ydimen>>1),parallaxyscale) + (ydimen>>1);
    globvis = globalpisibility;
    /* globalorientation = 0L; */
    if (constsector[sectnum].visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[sectnum].visibility+16)));

    if (dastat == 0)
    {
        globalpal = sec->ceilingpal;
        globalpicnum = sec->ceilingpicnum;
#if SEPARATE_SECTOR_CEILINGSHADE
        globalshade = getSector_ceilingshade(sectnum);
#else
        globalshade = (int32_t)sec->ceilingshade;
#endif // SEPARATE_SECTOR_CEILINGSHADE
        globalxpanning = (int32_t)sec->ceilingxpanning;
        globalypanning = (int32_t)sec->ceilingypanning;
        topptr = umost;
        botptr = uplc;
    }
    else
    {
        globalpal = sec->floorpal;
        globalpicnum = sec->floorpicnum;
#if SEPARATE_SECTOR_FLOORSHADE
        globalshade = getSector_floorshade(sectnum);
#else
        globalshade = (int32_t)sec->floorshade;
#endif // SEPARATE_SECTOR_FLOORSHADE
#if SEPARATE_SECTOR_FLOORXPANNING
        globalxpanning = (int32_t)getSector_floorxpanning(sectnum);
#else
        globalxpanning = (int32_t)sec->floorxpanning;
#endif
        globalypanning = (int32_t)sec->floorypanning;
        topptr = dplc;
        botptr = dmost;
    }

    if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES) globalpicnum = 0;

    if (tiles[globalpicnum].animFlags&192)
        globalpicnum += animateoffs(globalpicnum);

    globalshiftval = (picsiz[globalpicnum]>>4);

    if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height)
        globalshiftval++;
    globalshiftval = 32-globalshiftval;
    globalzd = (((tiles[globalpicnum].dim.height>>1)+parallaxyoffs)<<globalshiftval)+(globalypanning<<24);
    globalyscale = (8<<(globalshiftval-19));
    /*if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;*/

    k = 11 - (picsiz[globalpicnum]&15) - pskybits;
    x = -1;

    for(z=bunchfirst[bunch]; z>=0; z=bunchWallsList[z])
    {
#if PVWALL_HAS_WORLDWALLID
        wallnum = pvWalls[z].worldWallId;
#else
        wallnum = pvWalls_worldWallId[z];
#endif
        nextsectnum = constwall[wallnum].nextsector;

        if (dastat == 0) j = getSector(nextsectnum)->ceilingstat;
        else j = getSector(nextsectnum)->floorstat;

        if ((nextsectnum < 0) || (getWall_cstat(wallnum)&32) || ((j&1) == 0))
        {
            if (x == -1) x = pvWalls[z].screenSpaceCoo[0].vec_col;

            if (parallaxtype == 0)
            {
                n = mulscale16(xdimenrecip,viewingrange);
                for(j=pvWalls[z].screenSpaceCoo[0].vec_col; j<=pvWalls[z].screenSpaceCoo[1].vec_col; j++)
                    lplc[j] = (((mulscale23(j-halfxdimen,n)+globalang)&2047)>>k);
            }
            else
            {
                for(j=pvWalls[z].screenSpaceCoo[0].vec_col; j<=pvWalls[z].screenSpaceCoo[1].vec_col; j++)
                    lplc[j] = ((((int32_t)radarang2[j]+globalang)&2047)>>k);
            }
            if (parallaxtype == 2)
            {
                n = mulscale16(xdimscale,viewingrange);
                for(j=pvWalls[z].screenSpaceCoo[0].vec_col; j<=pvWalls[z].screenSpaceCoo[1].vec_col; j++)
                    swplc[j] = mulscale14(sintable[((int32_t)radarang2[j]+512)&2047],n);
            }
            else
                clearbuf_swplc(&swplc[pvWalls[z].screenSpaceCoo[0].vec_col],pvWalls[z].screenSpaceCoo[1].vec_col-pvWalls[z].screenSpaceCoo[0].vec_col+1,mulscale16(xdimscale,viewingrange));
        }
        else if (x >= 0)
        {
            l = globalpicnum;
            m = (picsiz[globalpicnum]&15);
            globalpicnum = l+pskyoff[lplc[x]>>m];

            if (((lplc[x]^lplc[pvWalls[z].screenSpaceCoo[0].vec_col-1])>>m) == 0)
                wallscan(x,pvWalls[z].screenSpaceCoo[0].vec_col-1,topptr,botptr,swplc,lplc);
            else
            {
                j = x;
                while (x < pvWalls[z].screenSpaceCoo[0].vec_col)
                {
                    n = l+pskyoff[lplc[x]>>m];
                    if (n != globalpicnum)
                    {
                        wallscan(j,x-1,topptr,botptr,swplc,lplc);
                        j = x;
                        globalpicnum = n;
                    }
                    x++;
                }
                if (j < x)
                    wallscan(j,x-1,topptr,botptr,swplc,lplc);
            }

            globalpicnum = l;
            x = -1;
        }
    }

    if (x >= 0)
    {
        l = globalpicnum;
        m = (picsiz[globalpicnum]&15);
        globalpicnum = l+pskyoff[lplc[x]>>m];

        if (((lplc[x]^lplc[pvWalls[bunchlast[bunch]].screenSpaceCoo[1].vec_col])>>m) == 0)
            wallscan(x,pvWalls[bunchlast[bunch]].screenSpaceCoo[1].vec_col,topptr,botptr,swplc,lplc);
        else
        {
            j = x;
            while (x <= pvWalls[bunchlast[bunch]].screenSpaceCoo[1].vec_col)
            {
                n = l+pskyoff[lplc[x]>>m];
                if (n != globalpicnum)
                {
                    wallscan(j,x-1,topptr,botptr,swplc,lplc);
                    j = x;
                    globalpicnum = n;
                }
                x++;
            }
            if (j <= x)
                wallscan(j,x,topptr,botptr,swplc,lplc);
        }
        globalpicnum = l;
    }
    globalhoriz = globalhorizbak;
}


#define BITSOFPRECISION 3  /* Don't forget to change this in A.ASM also! */
static void grouscan (int32_t dax1, int32_t dax2, int32_t sectnum, uint8_t  dastat)
{
// TODO: to be optimized.
    uint8_t flat[MAX_RAM_FLAT_SIZE];
    int32_t i, l, x, y, dx, dy, wx, wy, y1, y2, daz;
    int32_t daslope, dasqr;
    intptr_t j, shoffs, shinc, m1, m2, *mptr1, *mptr2, *nptr1, *nptr2;
    walltype *wal;
    const sectortype *sec;
    // next-hack: put here. Eating stack but whatever.
    intptr_t slopalookup[SLOPEALOOKUP_SIZE];
    //
    sec = getSector(sectnum);

    if (dastat == 0)
    {
        if (globalposz <= getceilzofslope((short) sectnum,globalposx,globalposy))
            return;  /* Back-face culling */
        globalorientation = sec->ceilingstat;
        globalpicnum = sec->ceilingpicnum;
#if SEPARATE_SECTOR_CEILINGSHADE
        globalshade = getSector_ceilingshade(sectnum);
#else
        globalshade = sec->ceilingshade;
#endif // SEPARATE_SECTOR_CEILINGSHADE
        globalpal = sec->ceilingpal;
        daslope = constsector[sectnum].ceilingheinum;
        daz = sec->ceilingz;
    }
    else
    {
        if (globalposz >= getflorzofslope((short) sectnum,globalposx,globalposy))
            return;  /* Back-face culling */
        globalorientation = sec->floorstat;
        globalpicnum = sec->floorpicnum;
#if SEPARATE_SECTOR_FLOORSHADE
        globalshade = getSector_floorshade(sectnum);
#else
        globalshade = (int32_t)sec->floorshade;
#endif // SEPARATE_SECTOR_FLOORSHADE

        globalpal = sec->floorpal;
        daslope = sec->floorheinum;
        daz = sec->floorz;
    }

    if ((tiles[globalpicnum].animFlags&192) != 0)
        globalpicnum += animateoffs(globalpicnum);

    setgotpic(globalpicnum);

    if ((tiles[globalpicnum].dim.width <= 0) ||
        (tiles[globalpicnum].dim.height <= 0))
        return;

    TILE_MakeAvailable(globalpicnum);

    wal = getWall(constsector[sectnum].wallptr);
    const walltype *wallp2 = getWall(constwall[constsector[sectnum].wallptr]._point2);
    wx = wallp2->x - wal->x;
    wy = wallp2->y - wal->y;
    dasqr = krecipasm(nsqrtasm(wx*wx+wy*wy));
    i = mulscale21(daslope,dasqr);
    wx *= i;
    wy *= i;

    globalx = -mulscale19(singlobalang,xdimenrecip);
    globaly = mulscale19(cosglobalang,xdimenrecip);
    globalx1 = (globalposx<<8);
    globaly1 = -(globalposy<<8);
    i = (dax1-halfxdimen)*xdimenrecip;
    globalx2 = mulscale16(cosglobalang<<4,viewingrangerecip) - mulscale27(singlobalang,i);
    globaly2 = mulscale16(singlobalang<<4,viewingrangerecip) + mulscale27(cosglobalang,i);
    globalzd = (xdimscale<<9);
    globalzx = -dmulscale17(wx,globaly2,-wy,globalx2) + mulscale10(1-globalhoriz,globalzd);
    globalz = -dmulscale25(wx,globaly,-wy,globalx);

    if (globalorientation&64)  /* Relative alignment */
    {
        dx = mulscale14(wallp2->x-wal->x,dasqr);
        dy = mulscale14(wallp2->y-wal->y,dasqr);

        i = nsqrtasm(daslope*daslope+16777216);

        x = globalx;
        y = globaly;
        globalx = dmulscale16(x,dx,y,dy);
        globaly = mulscale12(dmulscale16(-y,dx,x,dy),i);

        x = ((wal->x-globalposx)<<8);
        y = ((wal->y-globalposy)<<8);
        globalx1 = dmulscale16(-x,dx,-y,dy);
        globaly1 = mulscale12(dmulscale16(-y,dx,x,dy),i);

        x = globalx2;
        y = globaly2;
        globalx2 = dmulscale16(x,dx,y,dy);
        globaly2 = mulscale12(dmulscale16(-y,dx,x,dy),i);
    }
    if (globalorientation&0x4)
    {
        i = globalx;
        globalx = -globaly;
        globaly = -i;
        i = globalx1;
        globalx1 = globaly1;
        globaly1 = i;
        i = globalx2;
        globalx2 = -globaly2;
        globaly2 = -i;
    }
    if (globalorientation&0x10) {
        globalx1 = -globalx1, globalx2 = -globalx2, globalx = -globalx;
    }
    if (globalorientation&0x20) {
        globaly1 = -globaly1, globaly2 = -globaly2, globaly = -globaly;
    }

    daz = dmulscale9(wx,globalposy-wal->y,-wy,globalposx-wal->x) + ((daz-globalposz)<<8);
    globalx2 = mulscale20(globalx2,daz);
    globalx = mulscale28(globalx,daz);
    globaly2 = mulscale20(globaly2,-daz);
    globaly = mulscale28(globaly,-daz);

    i = 8-(picsiz[globalpicnum]&15);
    j = 8-(picsiz[globalpicnum]>>4);
    if (globalorientation&8) {
        i++;
        j++;
    }
    globalx1 <<= (i+12);
    globalx2 <<= i;
    globalx <<= i;
    globaly1 <<= (j+12);
    globaly2 <<= j;
    globaly <<= j;

    if (dastat == 0)
    {
        globalx1 += (((int32_t)sec->ceilingxpanning)<<24);
        globaly1 += (((int32_t)sec->ceilingypanning)<<24);
    }
    else
    {
#if SEPARATE_SECTOR_FLOORXPANNING
        globalx1 += (((int32_t)getSector_floorxpanning(sectnum))<<24);

#else
        globalx1 += (((int32_t)sec->floorxpanning)<<24);
#endif // SEPARATE_SECTOR_FLOORXPANNING
        globaly1 += (((int32_t)sec->floorypanning)<<24);
    }

    draws.asm1 = -(globalzd>>(16-BITSOFPRECISION));

    globvis = globalvisibility;
    if (constsector[sectnum].visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[sectnum].visibility+16)));
    globvis = mulscale13(globvis,daz);
    globvis = mulscale16(globvis,xdimscale);
    j = (intptr_t) palookup[globalpal];
    const uint8_t *tilebuffer = tileData[globalpicnum];
    if (isOnExternalFlash(tilebuffer))
    {
            int size = tiles[globalpicnum].dim.width * tiles[globalpicnum].dim.height;
        #if WIN32
            if (size > MAX_RAM_FLAT_SIZE)
            {
                printf("Global picnum: %d, sec->floorpicnum %d sec->ceilingpicnum %d\r\n", globalpicnum, sec->floorpicnum, sec->ceilingpicnum);
                FIXME("this is bad. sloped floor has %d size\r\n", size);
            }
          #endif // WIN32
            tilebuffer = extMemGetDataFromAddress(flat, tilebuffer, size);
    }
    setupslopevlin(((int32_t)(picsiz[globalpicnum]&15))+(((int32_t)(picsiz[globalpicnum]>>4))<<8),tilebuffer,-ylookup(1));

    l = (globalzd>>16);

    shinc = mulscale16(globalz,xdimenscale);
    if (shinc > 0)
        shoffs = (4<<15);
    else
        shoffs = ((SLOPEALOOKUP_SIZE - 4 -ydimen)<<15); // JBF: was 2044     16380
    if (dastat == 0) y1 = umost[dax1];
    else y1 = max(umost[dax1],dplc[dax1]);
    m1 = mulscale16(y1,globalzd) + (globalzx>>6);
    /* Avoid visibility overflow by crossing horizon */
    if (globalzd > 0) m1 += (globalzd>>16);
    else m1 -= (globalzd>>16);
    m2 = m1+l;
    mptr1 = (intptr_t *)&slopalookup[y1+(shoffs>>15)];
    mptr2 = mptr1+1;

    for(x=dax1; x<=dax2; x++)
    {
        if (dastat == 0) {
            y1 = umost[x];
            y2 = min(dmost[x],uplc[x])-1;
        }
        else {
            y1 = max(umost[x],dplc[x]);
            y2 = dmost[x]-1;
        }
        if (y1 <= y2)
        {
            nptr1 = (intptr_t *)&slopalookup[y1+(shoffs>>15)];
            nptr2 = (intptr_t *)&slopalookup[y2+(shoffs>>15)];
            while (nptr1 <= mptr1)
            {
                *mptr1-- = j + (getpalookup((int32_t)mulscale24(krecipasm(m1),globvis),globalshade)<<8);
                m1 -= l;
            }
            while (nptr2 >= mptr2)
            {
                *mptr2++ = j + (getpalookup((int32_t)mulscale24(krecipasm(m2),globvis),globalshade)<<8);
                m2 += l;
            }

            globalx3 = (globalx2>>10);
            globaly3 = (globaly2>>10);
            draws.asm3 = mulscale16(y2,globalzd) + (globalzx>>6);
            waitRenderLine(y2);
            slopevlin((uint32_t) (ylookup(y2)+x+frameoffset),krecipasm(draws.asm3>>3),slopalookup,y2+(shoffs>>15),y2-y1+1,globalx1,globaly1);
        }
        globalx2 += globalx;
        globaly2 += globaly;
        globalzx += globalz;
        shoffs += shinc;
    }
}


static int owallmost(short *mostbuf, int32_t w, int32_t z)
{
    int32_t bad, inty, xcross, y, yinc;
    int32_t s1, s2, s3, s4, ix1, ix2, iy1, iy2, t;

    z <<= 7;
    s1 = mulscale20(globaluclip,getVecDist(&pvWalls[w].screenSpaceCoo[0]));
    s2 = mulscale20(globaluclip,getVecDist(&pvWalls[w].screenSpaceCoo[1]));
    s3 = mulscale20(globaldclip,getVecDist(&pvWalls[w].screenSpaceCoo[0]));
    s4 = mulscale20(globaldclip,getVecDist(&pvWalls[w].screenSpaceCoo[1]));
    bad = (z<s1)+((z<s2)<<1)+((z>s3)<<2)+((z>s4)<<3);

    ix1 = pvWalls[w].screenSpaceCoo[0].vec_col;
    iy1 = getVecDist(&pvWalls[w].screenSpaceCoo[0]);
    ix2 = pvWalls[w].screenSpaceCoo[1].vec_col;
    iy2 = getVecDist(&pvWalls[w].screenSpaceCoo[1]);

    if ((bad&3) == 3)
    {
        clearbufbyte(&mostbuf[ix1],(ix2-ix1+1)*sizeof(mostbuf[0]),0L);
        return(bad);
    }

    if ((bad&12) == 12)
    {
        clearbufbyte(&mostbuf[ix1],(ix2-ix1+1)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
        return(bad);
    }

    if (bad&3)
    {
        t = divscale30(z-s1,s2-s1);
        inty = getVecDist(&pvWalls[w].screenSpaceCoo[0]) + mulscale30(getVecDist(&pvWalls[w].screenSpaceCoo[1])-getVecDist(&pvWalls[w].screenSpaceCoo[0]),t);
        xcross = pvWalls[w].screenSpaceCoo[0].vec_col + scale(mulscale30(getVecDist(&pvWalls[w].screenSpaceCoo[1]),t),pvWalls[w].screenSpaceCoo[1].vec_col-pvWalls[w].screenSpaceCoo[0].vec_col,inty);

        if ((bad&3) == 2)
        {
            if (pvWalls[w].screenSpaceCoo[0].vec_col <= xcross) {
                iy2 = inty;
                ix2 = xcross;
            }
            clearbufbyte(&mostbuf[xcross+1],(pvWalls[w].screenSpaceCoo[1].vec_col-xcross)*sizeof(mostbuf[0]),0L);
        }
        else
        {
            if (xcross <= pvWalls[w].screenSpaceCoo[1].vec_col) {
                iy1 = inty;
                ix1 = xcross;
            }
            clearbufbyte(&mostbuf[pvWalls[w].screenSpaceCoo[0].vec_col],(xcross-pvWalls[w].screenSpaceCoo[0].vec_col+1)*sizeof(mostbuf[0]),0L);
        }
    }

    if (bad&12)
    {
        t = divscale30(z-s3,s4-s3);
        inty = getVecDist(&pvWalls[w].screenSpaceCoo[0]) + mulscale30(getVecDist(&pvWalls[w].screenSpaceCoo[1])-getVecDist(&pvWalls[w].screenSpaceCoo[0]),t);
        xcross = pvWalls[w].screenSpaceCoo[0].vec_col + scale(mulscale30(getVecDist(&pvWalls[w].screenSpaceCoo[1]),t),pvWalls[w].screenSpaceCoo[1].vec_col-pvWalls[w].screenSpaceCoo[0].vec_col,inty);

        if ((bad&12) == 8)
        {
            if (pvWalls[w].screenSpaceCoo[0].vec_col <= xcross) {
                iy2 = inty;
                ix2 = xcross;
            }
            clearbufbyte(&mostbuf[xcross+1],(pvWalls[w].screenSpaceCoo[1].vec_col-xcross)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
        }
        else
        {
            if (xcross <= pvWalls[w].screenSpaceCoo[1].vec_col) {
                iy1 = inty;
                ix1 = xcross;
            }
            clearbufbyte(&mostbuf[pvWalls[w].screenSpaceCoo[0].vec_col],(xcross-pvWalls[w].screenSpaceCoo[0].vec_col+1)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
        }
    }

    y = (scale(z,xdimenscale,iy1)<<4);
    yinc = ((scale(z,xdimenscale,iy2)<<4)-y) / (ix2-ix1+1);
    qinterpolatedown16short((short *)&mostbuf[ix1],ix2-ix1+1,y+(globalhoriz<<16),yinc);

    if (mostbuf[ix1] < 0) mostbuf[ix1] = 0;
    if (mostbuf[ix1] > ydimen) mostbuf[ix1] = ydimen;
    if (mostbuf[ix2] < 0) mostbuf[ix2] = 0;
    if (mostbuf[ix2] > ydimen) mostbuf[ix2] = ydimen;

    return(bad);
}


static int wallmost(short *mostbuf, int32_t w, const int32_t sectnum, uint8_t  dastat)
{
    int32_t bad, i, j, t, y, z, inty, intz, xcross, yinc, fw;
    int32_t x1, y1, z1, x2, y2, z2, xv, yv, dx, dy, dasqr, oz1, oz2;
    int32_t s1, s2, s3, s4, ix1, ix2, iy1, iy2;
    const sectortype *sec = getSector(sectnum);
    if (dastat == 0){
        z = sec->ceilingz-globalposz;
        if ((sec->ceilingstat&2) == 0)
            return(owallmost(mostbuf,w,z));
    }
    else{
        z = getSector(sectnum)->floorz-globalposz;
        if ((getSector(sectnum)->floorstat&2) == 0)
            return(owallmost(mostbuf,w,z));
    }
#if PVWALL_HAS_WORLDWALLID
    i = pvWalls[w].worldWallId;
#else
    i = pvWalls_worldWallId[w];
#endif
    if (i == constsector[sectnum].wallptr)
        return(owallmost(mostbuf,w,z));
    const walltype *wall, *wallp2;
    wall = getWall(i);
    wallp2 = getWall(constwall[i]._point2);
    x1 = wall->x;
    x2 = wallp2->x-x1;
    y1 = wall->y;
    y2 = wallp2->y-y1;

    fw = constsector[sectnum].wallptr;
    wall = getWall(fw);

    i = constwall[fw]._point2;
    wallp2 = getWall(i);

    dx = wallp2->x-wall->x;
    dy = wallp2->y-wall->y;
    dasqr = krecipasm(nsqrtasm(dx*dx+dy*dy));

    if (pvWalls[w].screenSpaceCoo[0].vec_col == 0){
        xv = cosglobalang+sinviewingrangeglobalang;
        yv = singlobalang-cosviewingrangeglobalang;
    }
    else{
        xv = x1-globalposx;
        yv = y1-globalposy;
    }
    i = xv*(y1-globalposy)-yv*(x1-globalposx);
    j = yv*x2-xv*y2;

    if (klabs(j) > klabs(i>>3))
        i = divscale28(i,j);

    if (dastat == 0){
        t = mulscale15(constsector[sectnum].ceilingheinum,dasqr);
        z1 = sec->ceilingz;
    }
    else{
        t = mulscale15(sec->floorheinum,dasqr);
        z1 = sec->floorz;
    }

    z1 = dmulscale24(dx*t,mulscale20(y2,i)+((y1-wall->y)<<8),-dy*t,mulscale20(x2,i)+((x1-wall->x)<<8))+((z1-globalposz)<<7);


    if (pvWalls[w].screenSpaceCoo[1].vec_col == xdimen-1){
        xv = cosglobalang-sinviewingrangeglobalang;
        yv = singlobalang+cosviewingrangeglobalang;
    }
    else{
        xv = (x2+x1)-globalposx;
        yv = (y2+y1)-globalposy;
    }

    i = xv*(y1-globalposy)-yv*(x1-globalposx);
    j = yv*x2-xv*y2;

    if (klabs(j) > klabs(i>>3))
        i = divscale28(i,j);

    if (dastat == 0){
        t = mulscale15(constsector[sectnum].ceilingheinum,dasqr);
        z2 = sec->ceilingz;
    }
    else{
        t = mulscale15(sec->floorheinum,dasqr);
        z2 = sec->floorz;
    }

    z2 = dmulscale24(dx*t,mulscale20(y2,i)+((y1-wall->y)<<8),-dy*t,mulscale20(x2,i)+((x1-wall->x)<<8))+((z2-globalposz)<<7);


    s1 = mulscale20(globaluclip,getVecDist(&pvWalls[w].screenSpaceCoo[0]));
    s2 = mulscale20(globaluclip,getVecDist(&pvWalls[w].screenSpaceCoo[1]));
    s3 = mulscale20(globaldclip,getVecDist(&pvWalls[w].screenSpaceCoo[0]));
    s4 = mulscale20(globaldclip,getVecDist(&pvWalls[w].screenSpaceCoo[1]));
    bad = (z1<s1)+((z2<s2)<<1)+((z1>s3)<<2)+((z2>s4)<<3);

    ix1 = pvWalls[w].screenSpaceCoo[0].vec_col;
    ix2 = pvWalls[w].screenSpaceCoo[1].vec_col;
    iy1 = getVecDist(&pvWalls[w].screenSpaceCoo[0]);
    iy2 = getVecDist(&pvWalls[w].screenSpaceCoo[1]);
    oz1 = z1;
    oz2 = z2;

    if ((bad&3) == 3){
        clearbufbyte(&mostbuf[ix1],(ix2-ix1+1)*sizeof(mostbuf[0]),0L);
        return(bad);
    }

    if ((bad&12) == 12){
        clearbufbyte(&mostbuf[ix1],(ix2-ix1+1)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
        return(bad);
    }

    if (bad&3){
        /* inty = intz / (globaluclip>>16) */
        t = divscale30(oz1-s1,s2-s1+oz1-oz2);
        inty = getVecDist(&pvWalls[w].screenSpaceCoo[0]) + mulscale30(getVecDist(&pvWalls[w].screenSpaceCoo[1])-getVecDist(&pvWalls[w].screenSpaceCoo[0]),t);
        intz = oz1 + mulscale30(oz2-oz1,t);
        xcross = pvWalls[w].screenSpaceCoo[0].vec_col + scale(mulscale30(getVecDist(&pvWalls[w].screenSpaceCoo[1]),t),pvWalls[w].screenSpaceCoo[1].vec_col-pvWalls[w].screenSpaceCoo[0].vec_col,inty);

        if ((bad&3) == 2){
            if (pvWalls[w].screenSpaceCoo[0].vec_col <= xcross){
                z2 = intz;
                iy2 = inty;
                ix2 = xcross;
            }
            clearbufbyte(&mostbuf[xcross+1],(pvWalls[w].screenSpaceCoo[1].vec_col-xcross)*sizeof(mostbuf[0]),0L);
        }
        else{
            if (xcross <= pvWalls[w].screenSpaceCoo[1].vec_col) {
                z1 = intz;
                iy1 = inty;
                ix1 = xcross;
            }
            clearbufbyte(&mostbuf[pvWalls[w].screenSpaceCoo[0].vec_col],(xcross-pvWalls[w].screenSpaceCoo[0].vec_col+1)*sizeof(mostbuf[0]),0L);
        }
    }

    if (bad&12){
        /* inty = intz / (globaldclip>>16) */
        t = divscale30(oz1-s3,s4-s3+oz1-oz2);
        inty = getVecDist(&pvWalls[w].screenSpaceCoo[0]) + mulscale30(getVecDist(&pvWalls[w].screenSpaceCoo[1])-getVecDist(&pvWalls[w].screenSpaceCoo[0]),t);
        intz = oz1 + mulscale30(oz2-oz1,t);
        xcross = pvWalls[w].screenSpaceCoo[0].vec_col + scale(mulscale30(getVecDist(&pvWalls[w].screenSpaceCoo[1]),t),pvWalls[w].screenSpaceCoo[1].vec_col-pvWalls[w].screenSpaceCoo[0].vec_col,inty);

        if ((bad&12) == 8){
            if (pvWalls[w].screenSpaceCoo[0].vec_col <= xcross) {
                z2 = intz;
                iy2 = inty;
                ix2 = xcross;
            }
            clearbufbyte(&mostbuf[xcross+1],(pvWalls[w].screenSpaceCoo[1].vec_col-xcross)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
        }
        else{
            if (xcross <= pvWalls[w].screenSpaceCoo[1].vec_col) {
                z1 = intz;
                iy1 = inty;
                ix1 = xcross;
            }
            clearbufbyte(&mostbuf[pvWalls[w].screenSpaceCoo[0].vec_col],(xcross-pvWalls[w].screenSpaceCoo[0].vec_col+1)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
        }
    }

    y = (scale(z1,xdimenscale,iy1)<<4);
    yinc = ((scale(z2,xdimenscale,iy2)<<4)-y) / (ix2-ix1+1);
    qinterpolatedown16short((short *)&mostbuf[ix1],ix2-ix1+1,y+(globalhoriz<<16),yinc);

    if (mostbuf[ix1] < 0)
        mostbuf[ix1] = 0;
    if (mostbuf[ix1] > ydimen)
        mostbuf[ix1] = ydimen;
    if (mostbuf[ix2] < 0)
        mostbuf[ix2] = 0;
    if (mostbuf[ix2] > ydimen)
        mostbuf[ix2] = ydimen;

    return(bad);
}


static void drawalls(int32_t bunch)
{
    const sectortype *sec, *nextsec;
    const walltype *wal;
    int32_t i, x, x1, x2, cz[5], fz[5];
    int32_t z, wallnum, sectnum, nextsectnum;
    int32_t startsmostwallcnt, startsmostcnt, gotswall;
    uint8_t  andwstat1, andwstat2;

    z = bunchfirst[bunch];
    sectnum = get_pvWalls_sectorId(z);
    sec = getSector(sectnum);

    andwstat1 = 0xff;
    andwstat2 = 0xff;
    for(; z>=0; z=bunchWallsList[z]){ /* uplc/dplc calculation */

        andwstat1 &= wallmost(uplc,z,sectnum,(uint8_t )0);
        andwstat2 &= wallmost(dplc,z,sectnum,(uint8_t )1);
    }
    /* draw ceilings */
    if ((andwstat1&3) != 3){
        if ((sec->ceilingstat&3) == 2)
            grouscan(pvWalls[bunchfirst[bunch]].screenSpaceCoo[0].vec_col,pvWalls[bunchlast[bunch]].screenSpaceCoo[1].vec_col,sectnum,0);
        else if ((sec->ceilingstat&1) == 0)
            ceilscan(pvWalls[bunchfirst[bunch]].screenSpaceCoo[0].vec_col,pvWalls[bunchlast[bunch]].screenSpaceCoo[1].vec_col,sectnum);
        else
            parascan(pvWalls[bunchfirst[bunch]].screenSpaceCoo[0].vec_col,pvWalls[bunchlast[bunch]].screenSpaceCoo[1].vec_col,sectnum,0,bunch);
    }

    /* draw floors */
    if ((andwstat2&12) != 12){
        if ((sec->floorstat&3) == 2)
            grouscan(pvWalls[bunchfirst[bunch]].screenSpaceCoo[0].vec_col,pvWalls[bunchlast[bunch]].screenSpaceCoo[1].vec_col,sectnum,1);
        else if ((sec->floorstat&1) == 0)
            florscan(pvWalls[bunchfirst[bunch]].screenSpaceCoo[0].vec_col,pvWalls[bunchlast[bunch]].screenSpaceCoo[1].vec_col,sectnum);
        else
            parascan(pvWalls[bunchfirst[bunch]].screenSpaceCoo[0].vec_col,pvWalls[bunchlast[bunch]].screenSpaceCoo[1].vec_col,sectnum,1,bunch);
    }

    /* DRAW WALLS SECTION! */
    for(z=bunchfirst[bunch]; z>=0; z=bunchWallsList[z])
    {

        x1 = pvWalls[z].screenSpaceCoo[0].vec_col;
        x2 = pvWalls[z].screenSpaceCoo[1].vec_col;
        if (umost[x2] >= dmost[x2])
        {

            for(x=x1; x<x2; x++)
                if (umost[x] < dmost[x])
                    break;

            if (x >= x2)
            {
                smostwall[smostwallcnt] = z;
                set_smostwalltype(smostwallcnt, 0);
                smostwallcnt++;
                continue;
            }
        }
#if PVWALL_HAS_WORLDWALLID
        wallnum = pvWalls[z].worldWallId;
#else
        wallnum = pvWalls_worldWallId[z];
#endif
        wal = getWall(wallnum);
        nextsectnum = constwall[wallnum].nextsector;
        nextsec = getSector(nextsectnum);

        gotswall = 0;

        startsmostwallcnt = smostwallcnt;
        startsmostcnt = smostcnt;

        if ((searchit == 2) && (searchx >= x1) && (searchx <= x2))
        {
            if (searchy <= uplc[searchx]){ /* ceiling */
                searchsector = sectnum;
                searchwall = wallnum;
                searchstat = 1;
                searchit = 1;
            }
            else if (searchy >= dplc[searchx]){ /* floor */
                searchsector = sectnum;
                searchwall = wallnum;
                searchstat = 2;
                searchit = 1;
            }
        }

        if (nextsectnum >= 0){
            int wal_point2 = constwall[wallnum]._point2; //getWall_point2(wallnum);
            const walltype *wallp2 = getWall(wal_point2);
            getzsofslope((short)sectnum,wal->x,wal->y,&cz[0],&fz[0]);
            getzsofslope((short)sectnum,wallp2->x,wallp2->y,&cz[1],&fz[1]);
            getzsofslope((short)nextsectnum,wal->x,wal->y,&cz[2],&fz[2]);
            getzsofslope((short)nextsectnum,wallp2->x,wallp2->y,&cz[3],&fz[3]);
            getzsofslope((short)nextsectnum,globalposx,globalposy,&cz[4],&fz[4]);

            if ((wal->_cstat/*getWall_cstat(wallnum)*/&48) == 16)
                maskwall[maskwallcnt++] = z;

            if (((sec->ceilingstat&1) == 0) || ((nextsec->ceilingstat&1) == 0)){
                if ((cz[2] <= cz[0]) && (cz[3] <= cz[1])){
                    if (globparaceilclip)
                        for(x=x1; x<=x2; x++)
                            if (uplc[x] > umost[x])
                                if (umost[x] <= dmost[x]){
                                    umost[x] = uplc[x];
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                }
                else{
                    wallmost(dwall,z,nextsectnum,(uint8_t )0);
                    if ((cz[2] > fz[0]) || (cz[3] > fz[1]))
                        for(i=x1; i<=x2; i++) if (dwall[i] > dplc[i]) dwall[i] = dplc[i];

                    if ((searchit == 2) && (searchx >= x1) && (searchx <= x2))
                        if (searchy <= dwall[searchx]) /* wall */{
                            searchsector = sectnum;
                            searchwall = wallnum;
                            searchstat = 0;
                            searchit = 1;
                        }

                    globalorientation = wal->_cstat; //getWall_cstat(wallnum);
                    globalpicnum = wal->_picnum; //getWall_picnum(wallnum);
                    if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES) globalpicnum = 0;
                    globalxpanning = (int32_t) wal->_xpanning; //getWall_xpanning(wallnum);
                    globalypanning = (int32_t) wal->_ypanning; //getWall_ypanning(wallnum);
                    globalshiftval = (picsiz[globalpicnum]>>4);
                    if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height) globalshiftval++;
                    globalshiftval = 32-globalshiftval;

                    //Animated
                    if (tiles[globalpicnum].animFlags&192)
                        globalpicnum += animateoffs(globalpicnum);

                    globalshade = getWall_shade(wallnum);
                    globvis = globalvisibility;
                    if (constsector[sectnum].visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[sectnum].visibility+16)));
                    globalpal = (int32_t)wal->_pal; //getWall_pal(wallnum);
                    globalyscale = (constwall[wallnum]._yrepeat<<(globalshiftval-19));
                    if ((globalorientation&4) == 0)
                        globalzd = (((globalposz-nextsec->ceilingz)*globalyscale)<<8);
                    else
                        globalzd = (((globalposz-sec->ceilingz)*globalyscale)<<8);
                    globalzd += (globalypanning<<24);
                    if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;

                    if (gotswall == 0) {
                        gotswall = 1;
                        prepwall(z, wallnum);
                    }
                    wallscan(x1,x2,uplc,dwall,swall,lwall);

                    if ((cz[2] >= cz[0]) && (cz[3] >= cz[1])){
                        for(x=x1; x<=x2; x++)
                            if (dwall[x] > umost[x])
                                if (umost[x] <= dmost[x]){
                                    umost[x] = dwall[x];
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                    }
                    else
                    {
                        for(x=x1; x<=x2; x++)
                            if (umost[x] <= dmost[x]){
                                i = max(uplc[x],dwall[x]);
                                if (i > umost[x]){
                                    umost[x] = i;
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                            }
                    }
                }
                if ((cz[2] < cz[0]) || (cz[3] < cz[1]) || (globalposz < cz[4])){
                    i = x2-x1+1;
                    if (smostcnt+i < MAXYSAVES){
                        smoststart[smostwallcnt] = smostcnt;
                        smostwall[smostwallcnt] = z;
                        set_smostwalltype(smostwallcnt, 1);   /* 1 for umost */
                        smostwallcnt++;
                        for (int j = 0; j < i; j++)
                        {
                            smost[smostcnt + j] = umost[x1 + j];
                            #if WIN32
                            if (umost[x1 +j] < 0)
                            {
                                FIXME("UMOST NEGATIVE");
                            }
                            #endif
                        }
                        //copybufbyte((int32_t *)&umost[x1],(int32_t *)&smost[smostcnt],i*sizeof(smost[0]));
                        smostcnt += i;
                    }
                }
            }
            if (((sec->floorstat&1) == 0) || ((nextsec->floorstat&1) == 0)){
                if ((fz[2] >= fz[0]) && (fz[3] >= fz[1])){
                    if (globparaflorclip)
                        for(x=x1; x<=x2; x++)
                            if (dplc[x] < dmost[x])
                                if (umost[x] <= dmost[x]){
                                    dmost[x] = dplc[x];
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                }
                else{
                    wallmost(uwall,z,nextsectnum,(uint8_t )1);
                    if ((fz[2] < cz[0]) || (fz[3] < cz[1]))
                        for(i=x1; i<=x2; i++) if (uwall[i] < uplc[i]) uwall[i] = uplc[i];

                    if ((searchit == 2) && (searchx >= x1) && (searchx <= x2))
                        if (searchy >= uwall[searchx]) /* wall */{
                            searchsector = sectnum;
                            searchwall = wallnum;
                            if ((wal->_cstat /*getWall_cstat(wallnum)*/&2) > 0) searchwall = constwall[wallnum].nextwall;
                            searchstat = 0;
                            searchit = 1;
                        }

                    if ((wal->_cstat /*getWall_cstat(wallnum)*/&2) > 0){
                        wallnum = constwall[wallnum].nextwall;
                        wal = getWall(wallnum);
                        globalorientation = wal->_cstat; // getWall_cstat(wallnum);
                        globalpicnum = wal->_picnum; //getWall_picnum(wallnum);
                        if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES) globalpicnum = 0;
                        globalxpanning = (int32_t)wal->_xpanning; //getWall_xpanning(wallnum);
                        globalypanning = (int32_t)wal->_ypanning; // getWall_ypanning(wallnum);

                        if (tiles[globalpicnum].animFlags&192)
                            globalpicnum += animateoffs(globalpicnum);

                        globalshade = getWall_shade(wallnum);
                        globalpal = wal->_pal;//getWall_pal(wallnum);
#if PVWALL_HAS_WORLDWALLID
                        wallnum = pvWalls[z].worldWallId;
#else
                        wallnum = pvWalls_worldWallId[z];
#endif
                        wal = getWall(wallnum);
                    }
                    else{
                        globalorientation = wal->_cstat; //getWall_cstat(wallnum);
                        globalpicnum = wal->_picnum; //getWall_picnum(wallnum);

                        if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES)
                            globalpicnum = 0;

                        globalxpanning = (int32_t)wal->_xpanning; //getWall_xpanning(wallnum);
                        globalypanning = (int32_t)wal->_ypanning; //getWall_ypanning(wallnum);

                        if (tiles[globalpicnum].animFlags&192)
                            globalpicnum += animateoffs(globalpicnum);
                        globalshade = getWall_shade(wallnum);
                        globalpal = wal->_pal; //getWall_pal(wallnum);
                    }
                    globvis = globalvisibility;
                    if (constsector[sectnum].visibility != 0)
                        globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[sectnum].visibility+16)));
                    globalshiftval = (picsiz[globalpicnum]>>4);

                    if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height)
                        globalshiftval++;

                    globalshiftval = 32-globalshiftval;
                    globalyscale = (constwall[wallnum]._yrepeat<<(globalshiftval-19));

                    if ((globalorientation&4) == 0)
                        globalzd = (((globalposz-nextsec->floorz)*globalyscale)<<8);
                    else
                        globalzd = (((globalposz-sec->ceilingz)*globalyscale)<<8);

                    globalzd += (globalypanning<<24);
                    if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;

                    if (gotswall == 0) {
                        gotswall = 1;
                        prepwall(z,wallnum);
                    }
                    wallscan(x1,x2,uwall,dplc,swall,lwall);

                    if ((fz[2] <= fz[0]) && (fz[3] <= fz[1]))
                    {
                        for(x=x1; x<=x2; x++)
                            if (uwall[x] < dmost[x])
                                if (umost[x] <= dmost[x]){
                                    dmost[x] = uwall[x];
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                    }
                    else
                    {
                        for(x=x1; x<=x2; x++)
                            if (umost[x] <= dmost[x]){
                                i = min(dplc[x],uwall[x]);
                                if (i < dmost[x])
                                {
                                    dmost[x] = i;
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                            }
                    }
                }
                if ((fz[2] > fz[0]) || (fz[3] > fz[1]) || (globalposz > fz[4])){
                    i = x2-x1+1;
                    if (smostcnt+i < MAXYSAVES){
                        smoststart[smostwallcnt] = smostcnt;
                        smostwall[smostwallcnt] = z;
                        set_smostwalltype(smostwallcnt, 2);   /* 2 for dmost */
                        smostwallcnt++;
//                        copybufbyte((int32_t *)&dmost[x1],(int32_t *)&smost[smostcnt],i*sizeof(smost[0]));
                        for (int j = 0; j < i; j++)
                        {
                            smost[smostcnt + j] = dmost[x1 + j];
#if WIN32
                            if (dmost[x1 + j] < 0)
                                FIXME("DMOST < 0");
#endif
                        }
                        smostcnt += i;
                    }
                }
            }
            if (numhits < 0) return;
            if ((!(wal->_cstat /*getWall_cstat(wallnum)*/&32)) && ((visitedSectors[nextsectnum>>3]&pow2char[nextsectnum&7]) == 0)){
                if (umost[x2] < dmost[x2])
                    scansector((short) nextsectnum);
                else
                {
                    for(x=x1; x<x2; x++)
                        if (umost[x] < dmost[x]){
                            scansector((short) nextsectnum);
                            break;
                        }

                    /*
                     * If can't see sector beyond, then cancel smost array and just
                     *  store wall!
                     */
                    if (x == x2){
                        smostwallcnt = startsmostwallcnt;
                        smostcnt = startsmostcnt;
                        smostwall[smostwallcnt] = z;
                        set_smostwalltype(smostwallcnt, 0);
                        smostwallcnt++;
                    }
                }
            }
        }
        if ((nextsectnum < 0) || (wal->_cstat /*getWall_cstat(wallnum)*/&32))   /* White/1-way wall */
        {
            globalorientation = wal->_cstat; //getWall_cstat(wallnum);
            if (nextsectnum < 0)
                globalpicnum = wal->_picnum; //getWall_picnum(wallnum);
            else
                globalpicnum = wal->_overpicnum;//getWall_overpicnum(wallnum);

            if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES)
                globalpicnum = 0;

            globalxpanning = wal->_xpanning; //getWall_xpanning(wallnum);
            globalypanning = wal->_ypanning; // getWall_ypanning(wallnum);

            if (tiles[globalpicnum].animFlags&192)
                globalpicnum += animateoffs(globalpicnum);

            globalshade = getWall_shade(wallnum);
            globvis = globalvisibility;
            if (constsector[sectnum].visibility != 0)
                globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[sectnum].visibility+16)));

            globalpal = wal->_pal; //getWall_pal(wallnum);
            globalshiftval = (picsiz[globalpicnum]>>4);
            if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height)
                globalshiftval++;

            globalshiftval = 32-globalshiftval;
            globalyscale = (constwall[wallnum]._yrepeat<<(globalshiftval-19));
            if (nextsectnum >= 0)
            {
                if ((globalorientation&4) == 0)
                    globalzd = globalposz-nextsec->ceilingz;
                else
                    globalzd = globalposz-sec->ceilingz;
            }
            else
            {
                if ((globalorientation&4) == 0)
                    globalzd = globalposz-sec->ceilingz;
                else
                    globalzd = globalposz-sec->floorz;
            }
            globalzd = ((globalzd*globalyscale)<<8) + (globalypanning<<24);

            if (globalorientation&256){
                globalyscale = -globalyscale;
                globalzd = -globalzd;
            }

            if (gotswall == 0) {
                gotswall = 1;
                prepwall(z,wallnum);
            }

            wallscan(x1,x2,uplc,dplc,swall,lwall);

            for(x=x1; x<=x2; x++)
                if (umost[x] <= dmost[x])
                {
                    umost[x] = 1;
                    dmost[x] = 0;
                    numhits--;
                }
            smostwall[smostwallcnt] = z;
            set_smostwalltype(smostwallcnt, 0);
            smostwallcnt++;

            if ((searchit == 2) && (searchx >= x1) && (searchx <= x2)){
                searchit = 1;
                searchsector = sectnum;
                searchwall = wallnum;
                if (nextsectnum < 0) searchstat = 0;
                else searchstat = 4;
            }
        }
    }
}


static void dosetaspect(void)
{
    #if !USE_STATIC_LOOKUP_TABLES
    int32_t i, j, k, x, xinc;

    if (xyaspect != oxyaspect){
        oxyaspect = xyaspect;
        j = xyaspect*320;
        horizlookup2[horizycent-1] = divscale26(131072,j);
        for(i=ydim*4-1; i>=0; i--)
            if (i != (horizycent-1)){
                horizlookup[i] = divscale28(1,i-(horizycent-1));
                horizlookup2[i] = divscale14(klabs(horizlookup[i]),j);
            }
    }


    if ((xdimen != oxdimen) || (viewingrange != oviewingrange)){
        oxdimen = xdimen;
        oviewingrange = viewingrange;
        xinc = mulscale32(viewingrange*320,xdimenrecip);
        x = (640<<16)-mulscale1(xinc,xdimen);
        for(i=0; i<xdimen; i++){
            j = (x&65535);
            k = (x>>16);
            x += xinc;
            if (j != 0) j = mulscale16((int32_t)radarang[k+1]-(int32_t)radarang[k],j);
            radarang2[i] = (short)(((int32_t)radarang[k]+j)>>6);
        }
    }
    PRINT_ARRAY_SZ(horizlookup, int32_t, ydim*4);
    PRINT_ARRAY_SZ(horizlookup2, int32_t, ydim*4);
    PRINT_ARRAY(radarang2, int32_t);
    FIXME("DONE CALCULATING TABLES");
#endif
}


/*
  FCS: Geez one more horrible algorithm to decipher :| :/ :( cry smiley.....
  Algorithm:

  1.
  Take wall 1 vector [point1,point2] and using two cross products determine if the two endpoints of wall 2 are on the same side of Wall 1 plan.
  If they are then we can determine according to globalposx and globalposy if  wall2 is before or after wall1's plan.

  2. Do the same thing again but this time with wall2's plan. Try to find if wall1 is in front of behind wall2's plan.

  Key concept: If a cross-product is equal to 0 this mean they are parallel.

  Return: pvWallID1 in the potentially visible wall list is in front of pvWallID2 (in the same potentially visible list)
*/
int wallfront(int32_t pvWallID1, int32_t pvWallID2)
{
    walltype *wal;
    int32_t x11, y11, x21, y21, x12, y12, x22, y22, dx, dy, t1, t2;

    //It seems we are going to work in Worldspace coordinates.
#if PVWALL_HAS_WORLDWALLID
    wal = getWall(pvWalls[pvWallID1].worldWallId);
#else
    wal = getWall(pvWalls_worldWallId[pvWallID1]);
#endif
    x11 = wal->x;
    y11 = wal->y;
#if PVWALL_HAS_WORLDWALLID
    wal = getWall(getWall_point2(pvWalls[pvWallID1].worldWallId));
#else
    wal = getWall(constwall[pvWalls_worldWallId[pvWallID1]]._point2);
#endif
    x21 = wal->x;
    y21 = wal->y;
#if PVWALL_HAS_WORLDWALLID
    wal = getWall(pvWalls[pvWallID2].worldWallId);
#else
    wal = getWall(pvWalls_worldWallId[pvWallID2]);
#endif
    x12 = wal->x;
    y12 = wal->y;
#if PVWALL_HAS_WORLDWALLID
    wal = getWall(getWall_point2(pvWalls[pvWallID2].worldWallId));
#else
    wal = getWall(constwall[pvWalls_worldWallId[pvWallID2]]._point2);
#endif
    x22 = wal->x;
    y22 = wal->y;


    //This is part 1

    //Wall 1's vector
    dx = x21-x11;
    dy = y21-y11;

    //This is a cross-product between Wall 1 vector and the [Wall 1 Point 1-> Wall 2 Point 1] vector
    t1 = dmulscale2(x12-x11,dy,-dx,y12-y11); /* p1(l2) vs. l1 */
    //This is a cross-product between Wall 1 vector and the [Wall 1 Point 1-> Wall 2 Point 2] vector
    t2 = dmulscale2(x22-x11,dy,-dx,y22-y11); /* p2(l2) vs. l1 */

    //If the vectors a parallel, then the cross-product is zero.
    if (t1 == 0) {
        //wall2's point1 is on wall1's plan.
        t1 = t2;
        if (t1 == 0) // Those two walls are on the same plan.
        {
            //Wall 2's point 2 is on wall1's plan.
            return(-1);
        }
    }
    if (t2 == 0)
        t2 = t1;


    //This XOR just determine if the cross-product have the same sign and hence if both points are on the same side of wall 1 plan.
    //Test if both points of wall2 are on the same side of wall 1 (in front or behind).
    if ((t1^t2) >= 0)
    {
        //cross-product have the same sign: Both points of wall2 are on the same side of wall1 : An answer is possible !!

        //Now is time to take into account the camera position and determine which of wall1 or wall2 is seen first.
        t2 = dmulscale2(globalposx-x11,dy,-dx,globalposy-y11); /* pos vs. l1 */

        //Test the cross product sign difference.
        //If (t2^t1) >= 0 then  both cross product had different sign so wall1 is in front of wall2
        //otherwise wall2 is in front of wall1
        return((t2^t1) >= 0);
    }


    //This is part 2
    //Do it again but this time will wall2's plan.

    //Wall 2's vector
    dx = x22-x12;
    dy = y22-y12;

    t1 = dmulscale2(x11-x12,dy,-dx,y11-y12); /* p1(l1) vs. l2 */
    t2 = dmulscale2(x21-x12,dy,-dx,y21-y12); /* p2(l1) vs. l2 */
    if (t1 == 0) {
        t1 = t2;
        if (t1 == 0)
            return(-1);
    }
    if (t2 == 0)
        t2 = t1;
    if ((t1^t2) >= 0)
    {
        t2 = dmulscale2(globalposx-x12,dy,-dx,globalposy-y12); /* pos vs. l2 */
        return((t2^t1) < 0);
    }

    //FCS: No wall is in front of the other's plan: This means they are crossing.
    return(-2);
}


//Return 1 if bunch firstBunchID is in from of bunch secondBunchID.
static int bunchfront(int32_t firstBunchID, int32_t secondBunchID)
{
    int32_t x1b1, x2b1, x1b2, x2b2;


    x1b1 = pvWalls[bunchfirst[firstBunchID]].screenSpaceCoo[0].vec_col;
    x2b2 = pvWalls[bunchlast[secondBunchID]].screenSpaceCoo[1].vec_col+1;
    if (x1b1 >= x2b2)
    {
        //Bunch 1 left side is completely on the right of bunch2's right in screenspace: They do not overlap.
        return(-1);
    }


    x1b2 = pvWalls[bunchfirst[secondBunchID]].screenSpaceCoo[0].vec_col;
    x2b1 = pvWalls[bunchlast[firstBunchID]].screenSpaceCoo[1].vec_col+1;
    if (x1b2 >= x2b1)
    {
        //Bunch 2 left side is completely on the right of bunch 1 right side: They do not overlap.
        return(-1);
    }


    if (x1b1 >= x1b2)
    {
        //Get the last wall in the bunch2.
        int lastWallID;
        for(lastWallID=bunchfirst[secondBunchID];
            pvWalls[lastWallID].screenSpaceCoo[1].vec_col<x1b1;
            lastWallID=bunchWallsList[lastWallID]);

        return(wallfront(bunchfirst[firstBunchID],lastWallID));
    }
    else
    {
        //Get the last wall in the bunch.
        int lastWallID;
        for(lastWallID=bunchfirst[firstBunchID];
            pvWalls[lastWallID].screenSpaceCoo[1].vec_col<x1b2;
            lastWallID=bunchWallsList[lastWallID]);

        return(wallfront(lastWallID,bunchfirst[secondBunchID]));
    }
}

//#include "keyboard.h"
//void WriteLastPaletteToFile(void);
//void WriteTranslucToFile(void);
/*
      FCS: Draw every walls in Front to Back Order.
*/
void drawrooms(int32_t daposx, int32_t daposy, int32_t daposz,short daang, int32_t dahoriz, short dacursectnum)
{
#if TEST_STACK
    short stack_bunchfirst[MAXWALLSB];
    short stack_bunchlast[MAXWALLSB];
    short stack_bunchWallsList[MAXWALLSB];
    bunchfirst = stack_bunchfirst;
    bunchlast = stack_bunchlast;
    bunchWallsList = stack_bunchWallsList;
#endif // TEST_STACK
    int32_t i, j, z, closest;
    //Ceiling and Floor height at the player position.
    int32_t cz, fz;

    // When visualizing the rendering process, part of the screen
    // are not updated: In order to avoid the "ghost effect", we
    // clear the framebuffer to black.
#if WIN32
    if (CLEAR_FRAMEBUFFER)
        clear2dscreen();
#endif

    //CODE EXPLORATION
    /*
    if( KB_KeyDown[0x39]){ // 0x39 = SPACE
        //CODE EXPLORATION
        WriteLastPaletteToFile();
        WriteTranslucToFile();
    }
    */

    //pixelsAllowed = pixelRenderable;
#if RENDER_LIMIT_PIXELS
    pixelsAllowed = 100000000;
#endif
    //printf("%d\n",pixelsAllowed);

    beforedrawrooms = 0;

    // FCS: What was the point of having those values as parameters of this function....if it is to overwrite the
    // values with the gloval variables ?!?!?
    globalposx = daposx;
    globalposy = daposy;
    globalposz = daposz;
    globalang = (daang&2047); //FCS: Mask and keep only 11 bits of angle value.

    globalhoriz = mulscale16(dahoriz-100,xdimenscale)+(ydimen>>1);
    globaluclip = (0-globalhoriz)*xdimscale;
    globaldclip = (ydimen-globalhoriz)*xdimscale;

    i = mulscale16(xdimenscale,viewingrangerecip);
    globalpisibility = mulscale16(parallaxvisibility,i);
    globalvisibility = mulscale16(visibility,i);
    globalhisibility = mulscale16(globalvisibility,xyaspect);
    globalcisibility = mulscale8(globalhisibility,320);

    globalcursectnum = dacursectnum;
    totalclocklock = totalclock;

    cosglobalang = sintable[(globalang+512)&2047];
    singlobalang = sintable[globalang&2047];
    cosviewingrangeglobalang = mulscale16(cosglobalang,viewingrange);
    sinviewingrangeglobalang = mulscale16(singlobalang,viewingrange);
#if USE_STEREO_VIDEO
    if (stereomode != 0)
    {
        if (stereopixelwidth != ostereopixelwidth)
        {
            ostereopixelwidth = stereopixelwidth;
            xdimen = (windowx2-windowx1+1)+(stereopixelwidth<<1);
            halfxdimen = (xdimen>>1);
            xdimenrecip = divscale32(1L,xdimen);
            setaspect((int32_t)divscale16(xdimen,windowx2-windowx1+1),yxaspect);
        }

        if ((!(activepage&1)) ^ inpreparemirror)
        {
#if SET_STARTxMOST
            for(i=windowx1; i<windowx1+(stereopixelwidth<<1); i++) {
                _startumost[i] = 1, _startdmost[i] = 0;
            }
            for(; i<windowx2+1+(stereopixelwidth<<1); i++) {
                _startumost[i] = windowy1, _startdmost[i] = windowy2+1;
            }
#else
        _startumost = windowy1, _startdmost = windowy2+1;
#endif
            viewoffset = windowy1*bytesperline+windowx1-(stereopixelwidth<<1);
            i = stereowidth;
        }
        else
        {
#if SET_STARTxMOST
            for(i=windowx1; i<windowx2+1; i++) {
                _startumost[i] = windowy1, _startdmost[i] = windowy2+1;
            }
            for(; i<windowx2+1+(stereopixelwidth<<1); i++) {
                _startumost[i] = 1, _startdmost[i] = 0;
            }
#else
  _startumost = windowy1, _startdmost = windowy2+1;
#endif
            viewoffset = windowy1*bytesperline+windowx1;
            i = -stereowidth;
        }
        globalposx += mulscale24(singlobalang,i);
        globalposy -= mulscale24(cosglobalang,i);

    }
#endif
    if ((xyaspect != oxyaspect) || (xdimen != oxdimen) || (viewingrange != oviewingrange))
    {
        dosetaspect();
    }

    frameoffset = frameplace+viewoffset;

    //Clear the bit vector that keep track of what sector has been flooded in.
    clearbufbyte(visitedSectors,(int32_t)((numsectors+7)>>3),0L);

    //Clear the occlusion array.
#if SET_STARTxMOST
    shortptr1 = (short *)&_startumost[windowx1];
    shortptr2 = (short *)&_startdmost[windowx1];
#endif
    i = xdimen-1;
    do
    {
        umost[i] = startumost(i + windowx1)-windowy1;
        dmost[i] = startdmost(i + windowx1)-windowy1;
        i--;
    } while (i != 0);
    umost[0] = startumost(windowx1)-windowy1;
    dmost[0] = startdmost(windowx1)-windowy1;

    //NumHits is the number of column to draw.
    numhits = xdimen;
    //Num walls to potentially render.
    numscans = 0;

    numbunches = 0;
    maskwallcnt = 0;
    smostwallcnt = 0;
    smostcnt = 0;
    spritesortcnt = 0;

    if (globalcursectnum >= MAXSECTORS)
        globalcursectnum -= MAXSECTORS;
    else
    {
        // Even if the player leaves the map, the engine will keep on rendering from the last visited sector.
        // Save it.
        i = globalcursectnum;
        updatesector(globalposx,globalposy,&globalcursectnum);
        //Seem the player has left the map since updatesector cannot locate him -> Restore to the last known sector.
        if (globalcursectnum < 0)
            globalcursectnum = i;
    }

    globparaceilclip = 1;
    globparaflorclip = 1;

    //Update the ceiling and floor Z coordinate for the player's 2D position.
    getzsofslope(globalcursectnum,globalposx,globalposy,&cz,&fz);

    if (globalposz < cz) globparaceilclip = 0;
    if (globalposz > fz) globparaflorclip = 0;

    //Build the list of potentially visible wall in to "bunches".
    scansector(globalcursectnum);
    if (inpreparemirror)
    {
        inpreparemirror = 0;
        mirrorsx1 = xdimen-1;
        mirrorsx2 = 0;
        for(i=numscans-1; i>=0; i--)
        {
#if PVWALL_HAS_WORLDWALLID
            if (constwall[pvWalls[i].worldWallId].nextsector < 0) continue;
#else
            if (constwall[pvWalls_worldWallId[i]].nextsector < 0) continue;
#endif
            if (pvWalls[i].screenSpaceCoo[0].vec_col < mirrorsx1) mirrorsx1 = pvWalls[i].screenSpaceCoo[0].vec_col;
            if (pvWalls[i].screenSpaceCoo[1].vec_col > mirrorsx2) mirrorsx2 = pvWalls[i].screenSpaceCoo[1].vec_col;
        }
#if USE_STEREO_VIDEO
        if (stereomode)
        {
            mirrorsx1 += (stereopixelwidth<<1);
            mirrorsx2 += (stereopixelwidth<<1);
        }
#endif
        for(i=0; i<mirrorsx1; i++)
            if (umost[i] <= dmost[i])
            {
                umost[i] = 1;
                dmost[i] = 0;
                numhits--;
            }
        for(i=mirrorsx2+1; i<xdimen; i++)
            if (umost[i] <= dmost[i])
            {
                umost[i] = 1;
                dmost[i] = 0;
                numhits--;
            }

        drawalls(0L);
        numbunches--;
        bunchfirst[0] = bunchfirst[numbunches];
        bunchlast[0] = bunchlast[numbunches];

        mirrorsy1 = min(umost[mirrorsx1],umost[mirrorsx2]);
        mirrorsy2 = max(dmost[mirrorsx1],dmost[mirrorsx2]);
    }

    // scansector has generated the bunches, it is now time to see which ones to render.
    // numhits is the number of column of pixels to draw: (if the screen is 320x200 then numhits starts at 200 - next-hack: you mean 320?).
    // Due to rounding error, not all columns may be drawn so an additional stop condition is here:
    // When every bunches have been tested for rendition.
    while ((numbunches > 0) && (numhits > 0))
    {
        // wallstempbuf is used to mark which bunches have been elected as "closest".
        // if tempbug[x] == 1 then it should be skipped.
        clearbuf(&wallstempbuf[0],(int32_t)((numbunches+3)>>2),0L);

        /* Almost works, but not quite :( */
        closest = 0;
        wallstempbuf[closest] = 1;
        for(i=1; i<numbunches; i++)
        {
            if ((j = bunchfront(i,closest)) < 0)
                continue;
            wallstempbuf[i] = 1;
            if (j == 0){
                wallstempbuf[closest] = 1;
                closest = i;
            }
        }

        /* Double-check */
        for(i=0; i<numbunches; i++)
        {
            if (wallstempbuf[i])
                continue;
            if ((j = bunchfront(i,closest)) < 0)
                continue;
            wallstempbuf[i] = 1;
            if (j == 0){
                wallstempbuf[closest] = 1;
                closest = i, i = 0;
            }
        }

        //Draw every solid walls with ceiling/floor in the bunch "closest"
        drawalls(closest);

        if (automapping)
        {
            for(z=bunchfirst[closest]; z>=0; z=bunchWallsList[z])
#if PVWALL_HAS_WORLDWALLID
                show2dwall[pvWalls[z].worldWallId>>3] |= pow2char  [pvWalls[z].worldWallId&7];
#else
                show2dwall[pvWalls_worldWallId[z]>>3] |= pow2char  [pvWalls_worldWallId[z]&7];
#endif
        }

        //Since we just rendered a bunch, lower the current stack element so we can treat the next item
        numbunches--;
        //...and move the bunch at the top of the stack so we won't iterate on it again...
        // (next-hack: actually, copy in the "closest" position, the bunch from the top of the stack,
        // which won't be evaluated twice as we decreased the number of bunches.
        bunchfirst[closest] = bunchfirst[numbunches];
        bunchlast[closest] = bunchlast[numbunches];
    }
  //  printf("numscans: %d\r\n", numscans);

#if TEST_STACK
    bunchfirst = NULL;
    bunchlast = NULL;
    bunchWallsList = NULL;
#endif // TEST_STACK

}


static int spritewallfront (tspritetype *s, int32_t w)
{
    walltype *wal;
    int32_t x1, y1;

    wal = getWall(w);
    x1 = wal->x;
    y1 = wal->y;
    wal = getWall(constwall[w]._point2);
    return (dmulscale32(wal->x-x1,s->y-y1,-(s->x-x1),wal->y-y1) >= 0);
}

#if CODE_DISABLED
static void transmaskvline(int32_t x)
{
    int32_t vplc, vinc, i;
    uint8_t* palookupoffs;
    uint8_t* bufplc;
    uint8_t* p;
    short y1v, y2v;

    if ((x < 0) || (x >= xdimen)) return;

    y1v = max(uwall[x],startumost(x+windowx1)-windowy1);
    y2v = min(dwall[x],startdmost(x+windowx1)-windowy1);
    y2v--;
    if (y2v < y1v) return;

    palookupoffs = palookup[globalpal] + (getpalookup((int32_t)mulscale16(swall[x],globvis),globalshade)<<8);

    vinc = swall[x]*globalyscale;
    vplc = globalzd + vinc*(y1v-globalhoriz+1);

    i = lwall[x]+globalxpanning;

    if (i >= tiles[globalpicnum].dim.width)
        i %= tiles[globalpicnum].dim.width;

    bufplc = tileData[globalpicnum]+i*tiles[globalpicnum].dim.height;

    p = ylookup(y1v)+x+frameoffset;

    tvlineasm1(vinc,palookupoffs,y2v-y1v,vplc,bufplc,p);

    transarea += y2v-y1v;
}

static void transmaskvline2 (int32_t x)
{
    int32_t y1, y2, x2;
    intptr_t i;
    short y1ve[2], y2ve[2];

    if ((x < 0) || (x >= xdimen)) return;
    if (x == xdimen-1) {
        transmaskvline(x);
        return;
    }

    x2 = x+1;

    y1ve[0] = max(uwall[x],startumost(x+windowx1)-windowy1);
    y2ve[0] = min(dwall[x],startdmost(x+windowx1)-windowy1)-1;
    if (y2ve[0] < y1ve[0]) {
        transmaskvline(x2);
        return;
    }
    y1ve[1] = max(uwall[x2],startumost(x2+windowx1)-windowy1);
    y2ve[1] = min(dwall[x2],startdmost(x2+windowx1)-windowy1)-1;
    if (y2ve[1] < y1ve[1]) {
        transmaskvline(x);
        return;
    }

    palookupoffse[0] = palookup[globalpal] + (getpalookup((int32_t)mulscale16(swall[x],globvis),globalshade)<<8);
    palookupoffse[1] = palookup[globalpal] + (getpalookup((int32_t)mulscale16(swall[x2],globvis),globalshade)<<8);

    setuptvlineasm2(globalshiftval,palookupoffse[0],palookupoffse[1]);

    vince[0] = swall[x]*globalyscale;
    vince[1] = swall[x2]*globalyscale;
    vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);
    vplce[1] = globalzd + vince[1]*(y1ve[1]-globalhoriz+1);

    i = lwall[x] + globalxpanning;
    if (i >= tiles[globalpicnum].dim.width)
        i %= tiles[globalpicnum].dim.width;
    bufplce[0] = tileData[globalpicnum]+i*tiles[globalpicnum].dim.height;

    i = lwall[x2] + globalxpanning;
    if (i >= tiles[globalpicnum].dim.width)
        i %= tiles[globalpicnum].dim.width;
    bufplce[1] = tileData[globalpicnum]+i*tiles[globalpicnum].dim.height;


    y1 = max(y1ve[0],y1ve[1]);
    y2 = min(y2ve[0],y2ve[1]);

    i = x+frameoffset;

    if (y1ve[0] != y1ve[1])
    {
        if (y1ve[0] < y1)
            vplce[0] = tvlineasm1(vince[0],palookupoffse[0],y1-y1ve[0]-1,vplce[0],(const uint8_t*)bufplce[0],(uint8_t*)ylookup(y1ve[0])+i);
        else
            vplce[1] = tvlineasm1(vince[1],palookupoffse[1],y1-y1ve[1]-1,vplce[1],(const uint8_t*)bufplce[1],(uint8_t*)ylookup(y1ve[1])+i+1);
    }

    if (y2 > y1)
    {
        asm1 = vince[1];
        asm2 = ylookup(y2)+i+1;
        tvlineasm2(vplce[1],vince[0],bufplce[0],bufplce[1],vplce[0],ylookup(y1)+i);
        transarea += ((y2-y1)<<1);
    }
    else
    {
        asm1 = vplce[0];
        asm2 = vplce[1];
    }

    if (y2ve[0] > y2ve[1])
        tvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y2-1,asm1,(const uint8_t*)bufplce[0],(uint8_t*)ylookup(y2+1)+i);
    else if (y2ve[0] < y2ve[1])
        tvlineasm1(vince[1],palookupoffse[1],y2ve[1]-y2-1,asm2,(const uint8_t*)bufplce[1],(uint8_t*)ylookup(y2+1)+i+1);
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
}
#endif
static void transmaskwallscan(int32_t x1, int32_t x2)
{
    int32_t x;

    setgotpic(globalpicnum);
    uint8_t coldata[MAX_COL_BUF];
    //Tile dimensions are invalid
    if ((tiles[globalpicnum].dim.width <= 0) ||
        (tiles[globalpicnum].dim.height <= 0))
        return;

    TILE_MakeAvailable(globalpicnum);

    x = x1;
    while ((startumost(x+windowx1) > startdmost(x+windowx1)) && (x <= x2)) x++;
#if CODE_DISABLED
    if ((x <= x2) && (x&1)) transmaskvline(x), x++;
    while (x < x2) transmaskvline2(x), x += 2;
#endif
    int oldi = -1;
    while (x <= x2)
    {
        int32_t vplc, vinc, i;
        const uint8_t* palookupoffs;
        const uint8_t* bufplc;
        uint8_t* p;
        short y1v, y2v;

        if ((x < 0) || (x >= xdimen)) return;

        y1v = max(uwall[x],startumost(x+windowx1)-windowy1);
        y2v = min(dwall[x],startdmost(x+windowx1)-windowy1);
        y2v--;
        if (y2v < y1v) return;

        palookupoffs = palookup[globalpal] + (getpalookup((int32_t)mulscale16(swall[x],globvis),globalshade)<<8);

        vinc = swall[x]*globalyscale;
        vplc = globalzd + vinc*(y1v-globalhoriz+1);

        i = lwall[x]+globalxpanning;

        if (i >= tiles[globalpicnum].dim.width)
            i %= tiles[globalpicnum].dim.width;

        if (i != oldi)
        {
            int tileheight = tiles[globalpicnum].dim.height;
            bufplc = tileData[globalpicnum]+i*tileheight;
            if (isOnExternalFlash(bufplc))
            {
#if WIN32
                if (tileheight > MAX_COL_SIZE )
                    FIXME("This is bad. translucent column size is %d\r\n", tileheight);
#endif // WIN32
                bufplc = readColumnAligned(coldata, bufplc, tileheight);
            }
            oldi = i;
        }
        p = ylookup(y1v)+x+frameoffset;
        waitRenderLine(y2v);
        tvlineasm1(vinc,palookupoffs,y2v-y1v,vplc,bufplc,p);

        transarea += y2v-y1v;
        x++;
    }
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
}

#if WIN32
extern int maxSprites;
#endif

#ifdef WIN32
int loadedNumSprites;
#endif
// next-hack: blockmap to speed-up updateSector() functions.'
#if USE_BLOCKMAP
static bool doBoundingBoxesIntersect(const boundingBox_t *bb1, const boundingBox_t *bb2)
{
    // Check if one bounding box is to the left of the other
    if (bb1->xmax < bb2->xmin || bb2->xmax < bb1->xmin)
    {
        return false;
    }
    // Check if one bounding box is above the other
    if (bb1->ymax < bb2->ymin || bb2->ymax < bb1->ymin)
    {
        return false;
    }
    // If neither of the above conditions is true, the bounding boxes intersect or touch
    return true;
}
//
blockmap_data_t *pblockmapData;
static boundingBox_t getWallRangeExtent(int nstart, int nstop)
{
    boundingBox_t bb;
    bb.ymax = bb.xmax = 0x80000000;
    bb.ymin = bb.xmin = 0x7FFFFFFF;
    for (int i = nstart; i < nstop; i++)
    {
        const walltype *wal = getWall(i);
        int x = wal->x;
        int y = wal->y;
        // get level extent;
        if (x > bb.xmax)
        {
            bb.xmax = x;
        }
        if (y > bb.ymax)
        {
            bb.ymax = y;
        }
        if (x < bb.xmin)
        {
            bb.xmin = x;
        }
        if (y < bb.ymin)
        {
            bb.ymin = y;
        }
    }
    return bb;
}
static void createBlockmap(void)
{
    boundingBox_t bb = getWallRangeExtent(0, numwalls);
    blockmap_data_t stackBlockmap;
    // clear stackBlockmap
    memset(&stackBlockmap, 0, sizeof(stackBlockmap));
    //
    stackBlockmap.bb = bb;
    stackBlockmap.deltax = (bb.xmax - bb.xmin) / BLOCKMAP_SIZE;
    stackBlockmap.deltay = (bb.ymax - bb.ymin) / BLOCKMAP_SIZE;
    // increase if the rest of the division was not 0. In this way we can cover the entire map.
    if (stackBlockmap.deltax * BLOCKMAP_SIZE < (bb.xmax - bb.xmin))
    {
        stackBlockmap.deltax++;
    }
    if (stackBlockmap.deltay * BLOCKMAP_SIZE < (bb.ymax - bb.ymin))
    {
        stackBlockmap.deltay++;
    }
#if WIN32
    printf("Map extents: (%d, %d) to (%d, %d). Deltas: (%d, %d)\r\n", bb.xmin, bb.ymin, bb.xmax, bb.ymax, stackBlockmap.deltax, stackBlockmap.deltay);
#endif
    // now we have the extent. Let's fill with data;
    #define MAX_SECTORS_PER_BLOCKMAP 256
    int16_t sectorList[MAX_SECTORS_PER_BLOCKMAP];
    // a sector is considered to be in one blockmap if at least one vertex is inside.
    // find all sector extents. Put them in displayFrameBuffer, we have a lot of space there.
    boundingBox_t *sectorsBBs = (boundingBox_t *) getDisplayFrameBuffer(0);
    for (int si = 0; si < numsectors; si++)
    {
        // for each sector find the extents
        int walli = constsector[si].wallptr;
        unsigned int walln = constsector[si].wallnum;
        sectorsBBs[si] = getWallRangeExtent(walli, walli + walln);
#if WIN32
        printf("Sector %d has extents (%d, %d) to (%d, %d)\r\n", si, sectorsBBs[si].xmin, sectorsBBs[si].ymin, sectorsBBs[si].xmax, sectorsBBs[si].ymax);
#endif
    }
    //
    uint32_t totalSectors = 0;
    for (int yb = 0; yb < BLOCKMAP_SIZE; yb++)
    {
        for (int xb = 0; xb < BLOCKMAP_SIZE; xb++)
        {
            int sectorsInBlock = 0;
            // create block bounding box
            bb.xmin = stackBlockmap.bb.xmin + xb * stackBlockmap.deltax;
            bb.ymin = stackBlockmap.bb.ymin + yb * stackBlockmap.deltay;
            bb.xmax = stackBlockmap.bb.xmin + (xb + 1) * stackBlockmap.deltax;
            bb.ymax = stackBlockmap.bb.ymin + (yb + 1) * stackBlockmap.deltay;
            memset(sectorList, 0xFF, sizeof(sectorList)); // clear sector list so that everything is -1
            for (int si = numsectors - 1; si >= 0; si--)
            {
                if(doBoundingBoxesIntersect(&bb, &sectorsBBs[si]))
                {
                    if (sectorsInBlock < MAX_SECTORS_PER_BLOCKMAP - 1)
                    {
                        sectorList[sectorsInBlock] = si;
                        sectorsInBlock++;
                    }
                    else
                    {
                        FIXME("Too many sectors in blockmap (%d, %d)!\r\n", xb, yb);
                    }
                }
            }
#if WIN32
            printf("Blockmap (%d, %d) has %d sectors\r\n", xb, yb, sectorsInBlock);
#endif
            if (sectorsInBlock)
            {
                sectorsInBlock++;
                // let's use 4-byte alignment
                if (sectorsInBlock & 1)
                {
                    sectorsInBlock++;
                }
                totalSectors += sectorsInBlock;
                stackBlockmap.blockmap[yb][xb] = storeToInternalFlash(sectorList, sectorsInBlock * sizeof(sectorList[0]));
            }
            // if there were no sector, null is enough.
        }
    }
#if WIN32
    printf("Total sectors: %d\r\n", totalSectors);
    printf("Total size for map data: %d\r\n", totalSectors * 2 + sizeof(stackBlockmap));
#endif
    pblockmapData = storeToInternalFlash(&stackBlockmap, sizeof(stackBlockmap));

}
//
#endif

#define COPY_FIELD(pdest, source, field) pdest->field = source.field
int loadboard(const char  *filename, int32_t *daposx, int32_t *daposy,
              int32_t *daposz, short *daang, short *dacursectnum)
{
    waitForUpdate(-1);    // here we are using the framebuffer for temporary data :)
    isLoadingLevel = 1;
    // reset allocator indexes
    resetRamSpriteAllocIndex();
#if USE_SEPARATE_HITTYPE_TEMPDATA
    resetAllocHittype_lastIndex_tempdata0();
    resetAllocHittype_lastIndex_tempdata1();
    resetAllocHittype_lastIndex_tempdata2();
    resetAllocHittype_lastIndex_tempdata3();
    resetAllocHittype_lastIndex_tempdata45();
#else
    resetAllocHittype_lastIndex_tempdata();
#endif
    resetAllocHittype_lastIndex_otherdata();
#if !WIN32 && DISABLE_COMM_DURING_LOAD
    disableComm();
#endif
#if USE_RAM_HITTYPE
    resetAllocRamHittypeLastIndex();
#endif
    int x;
    short  i, numsprites;
#if WIN32
    maxSprites = 0;
#endif
    x = 0;
    // We need now to overwrite the level-specific data, so put back the internal flash pointer to the end of common-zone
    internalFlashResetToCommonZoneEnd();
    //
    #if USE_BLOCKMAP
        memset(sectorModifiedBf, 0, sizeof (sectorModifiedBf));
    #endif
    //
    resetAllHittypeData();

    // clear modified wall pointers
    //memset(ramwallIdx, 0, sizeof(ramwallIdx));
    for (int i = 0; i < MAXWALLS; i++)
        setRamWallIdx(i, 0);
    // clear modified walls
    memset(ramwalls, 0, sizeof(ramwalls));
    // clear number of modified walls
    numModifiedWalls = 0;
    // reset shade value (not really necessary. IT MUST BE DONE HERE, not after this function.
    memset(wallshade, 0, sizeof(wallshade));
    // clear modified sector pointers
    memset(ramsectorIdx, 0, sizeof(ramsectorIdx));
    // clear modified Sectors
    memset(ramsectors, 0, sizeof(ramsectors));
    // clear number of modified walls
    numModifiedSectors = 0;
    numModifiedSprites = 0;
    //
    memset(wall_extraIsMinusOne, 0, sizeof (wall_extraIsMinusOne));
    //
#if HITAG_CONSTANT
    // reset state of hitag
    memset(wall_hitag9999, 0, sizeof(wall_hitag9999));
#endif // HITAG_CONSTANT



    uint32_t size;
    uint8_t *ptr = kGetExtMemAddress(filename, &size);
    if (ptr == NULL)
    {
        mapversion = 7L;
        return(-1);
    }
    extMemSetCurrentAddress((uint32_t) ptr);
    extMemGetDataFromCurrentAddress(&mapversion, 4);
    if (mapversion != 7L) return(-1);


    clearbuf(&show2dsector[0],(int32_t)((MAXSECTORS+3)>>5),0L);
    clearbuf(&show2dsprite[0],(int32_t)((MAXSPRITES+3)>>5),0L);
    clearbuf(&show2dwall[0],(int32_t)((MAXWALLS+3)>>5),0L);

    extMemGetDataFromCurrentAddress(daposx, 4);
    extMemGetDataFromCurrentAddress(daposy, 4);
    extMemGetDataFromCurrentAddress(daposz, 4);
    extMemGetDataFromCurrentAddress(daang, 2);
    extMemGetDataFromCurrentAddress(dacursectnum, 2);
    extMemGetDataFromCurrentAddress(&numsectors, 2);


    //const_sectortype stack_constsector[MAXSECTORS] = {0};
    uint8_t stack_sectors[MAXSECTORS * sizeof (sectortype)] = {0};
    printf("Numsectors %d", numsectors);
    constsector = getCurrentInternalFlashPtr();
    const_sectortype scs[2] = {0};
    for (x = 0; x < numsectors; x++)
    {
        disk_sectortype sect;
        extMemGetDataFromCurrentAddress(&sect.wallptr, 2);
        extMemGetDataFromCurrentAddress(&sect.wallnum, 2);
        extMemGetDataFromCurrentAddress(&sect.ceilingz, 4);
        extMemGetDataFromCurrentAddress(&sect.floorz, 4);
        extMemGetDataFromCurrentAddress(&sect.ceilingstat, 2);
        extMemGetDataFromCurrentAddress(&sect.floorstat, 2);
        extMemGetDataFromCurrentAddress(&sect.ceilingpicnum, 2);
        extMemGetDataFromCurrentAddress(&sect.ceilingheinum, 2);
        extMemGetDataFromCurrentAddress((uint8_t *)&sect.ceilingshade, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&sect.ceilingpal, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&sect.ceilingxpanning, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&sect.ceilingypanning, 1);
        extMemGetDataFromCurrentAddress(&sect.floorpicnum, 2);
        extMemGetDataFromCurrentAddress(&sect.floorheinum, 2);
        extMemGetDataFromCurrentAddress((uint8_t *)&sect.floorshade, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&sect.floorpal, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&sect.floorxpanning, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&sect.floorypanning, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&sect.visibility, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&sect.filler, 1);
        extMemGetDataFromCurrentAddress(&sect.lotag, 2);
        extMemGetDataFromCurrentAddress(&sect.hitag, 2);
        extMemGetDataFromCurrentAddress(&sect.extra, 2);
        sectortype *s = ((sectortype*)stack_sectors) + x;
        const_sectortype *cs = &scs[x & 1]; // &stack_constsector[x];
#define TESTING_FIELDS 0
#if !TESTING_FIELDS
        COPY_FIELD(cs, sect, wallptr);
        COPY_FIELD(cs, sect, wallnum);
        COPY_FIELD(s, sect, ceilingz);
        COPY_FIELD(s, sect, floorz);
        COPY_FIELD(s, sect, ceilingstat);
        COPY_FIELD(s, sect, floorstat);
        COPY_FIELD(s, sect, ceilingpicnum);
        COPY_FIELD(cs, sect, ceilingheinum);
#if SEPARATE_SECTOR_CEILINGSHADE
        setSector_ceilingshade(x, sect.ceilingshade);
#else
        COPY_FIELD(s, sect, ceilingshade);
#endif
        COPY_FIELD(s, sect, ceilingpal);
        COPY_FIELD(s, sect, ceilingxpanning);
        COPY_FIELD(s, sect, ceilingypanning);
        COPY_FIELD(s, sect, floorpicnum);
        COPY_FIELD(s, sect, floorheinum);
#if SEPARATE_SECTOR_FLOORSHADE
        setSector_floorshade(x, sect.floorshade);
#else
        COPY_FIELD(s, sect, floorshade);
#endif
        COPY_FIELD(s, sect, floorpal);
#if SEPARATE_SECTOR_FLOORXPANNING
        setSector_floorxpanning(x, sect.floorxpanning);
#else
        COPY_FIELD(s, sect, floorxpanning);
#endif
        COPY_FIELD(s, sect, floorypanning);
        COPY_FIELD(cs, sect, visibility);
//        COPY_FIELD(cs, sect, filler);
        COPY_FIELD(s, sect, lotag);
        COPY_FIELD(s, sect, hitag);
#if SEPARATE_SECTOR_EXTRA
        setSector_extra(x, 256);
#else
        COPY_FIELD(s, sect, extra);
        s->extra = 256;
#endif
        // next-hack: was done in prelevel. But better to do now.
        switch(s->lotag)
        {
            case 20:
            case 22:
                if( s->floorz > s->ceilingz)
                    s->lotag |= 32768;
        }
#endif
        if (x & 1)
        {
        storeToInternalFlash(&scs, sizeof(scs));
    }
    }
    if (numsectors & 1)
    {
        storeToInternalFlash(&scs[0], (sizeof(scs[0]) + 3) & ~3 );
    }
    printf("Trying storing \r\n");
    //constsector = storeToInternalFlash(stack_constsector, sizeof(stack_constsector[0]) * numsectors);
    sectors = storeToInternalFlash(stack_sectors, (3 + sizeof(sectortype) * numsectors) & ~3);

    extMemGetDataFromCurrentAddress(&numwalls, 2);
    #if WIN32 && 0
        static int maxnumwalls;
        if (numwalls > maxnumwalls)
            maxnumwalls = numwalls;
        printf("Numwalls is %d, max is %d\r\n", numwalls, maxnumwalls);
    #endif
    walls = reserveInternalFlashSize((sizeof (walltype) * MAXWALLS  + 3) & ~3 );
    constwall = reserveInternalFlashSize((sizeof (const_walltype) * MAXWALLS + 3) & ~3);

   // next-hack: I know it's horrible, but we need to reduce stack usage.
#define MAXSTACK_WALLS 512
    walltype *stackwall = (walltype *)stack_sectors;
    const_walltype *stackconstwall = (const_walltype*) &stackwall[MAXSTACK_WALLS];
    // check
    char dummy[(sizeof(walltype) + sizeof(const_walltype)) * MAXSTACK_WALLS > sizeof(stack_sectors) ? -1 : 0];
    (void) dummy;
    //
    int numstackw = 0;
    int storedIdx = 0;
    for (x = 0; x < numwalls; x++)
    {
        // TODO: refactor below
        walltype *w = &stackwall[numstackw];      // this is because I don't want to refactor
        const_walltype *cw = &stackconstwall[numstackw];
        disk_walltype dw;
        extMemGetDataFromCurrentAddress(&dw.x, 4);
        extMemGetDataFromCurrentAddress(&dw.y, 4);
        extMemGetDataFromCurrentAddress(&dw._point2, 2);
        extMemGetDataFromCurrentAddress(&dw.nextwall, 2);
        extMemGetDataFromCurrentAddress(&dw.nextsector, 2);
        extMemGetDataFromCurrentAddress(&dw._cstat, 2);
        extMemGetDataFromCurrentAddress(&dw._picnum, 2);
        extMemGetDataFromCurrentAddress(&dw._overpicnum, 2);
        extMemGetDataFromCurrentAddress((uint8_t *)&dw._shade, 1);
        extMemGetDataFromCurrentAddress(&dw._pal, 1);
        extMemGetDataFromCurrentAddress(&dw._xrepeat, 1);
        extMemGetDataFromCurrentAddress(&dw._yrepeat, 1);
        extMemGetDataFromCurrentAddress(&dw._xpanning, 1);
        extMemGetDataFromCurrentAddress(&dw._ypanning, 1);
        extMemGetDataFromCurrentAddress(&dw._lotag, 2);
        extMemGetDataFromCurrentAddress(&dw._hitag, 2);
        extMemGetDataFromCurrentAddress(&dw._extra, 2);
#if WIN32
        static uint32_t maxHitag;
        if (dw._hitag < 0)
        {
            maxHitag = dw._hitag;
            FIXME("Max hitag increased to: %d\r\n", maxHitag);
        }
#endif
        //
        COPY_FIELD(w, dw, x);
        COPY_FIELD(w, dw, y);
//        COPY_FIELD(w, dw, _point2);
        cw->_point2 = dw._point2;
        cw->nextwall = dw.nextwall;
        cw->nextsector = dw.nextsector;
        COPY_FIELD(w, dw, _cstat);
        COPY_FIELD(w, dw, _picnum);
        COPY_FIELD(w, dw, _overpicnum);
        wallshade[x] = dw._shade;
        COPY_FIELD(w, dw, _pal);
        cw->_xrepeat = dw._xrepeat;
        cw->_yrepeat = dw._yrepeat;
        COPY_FIELD(w, dw, _xpanning);
        COPY_FIELD(w, dw, _ypanning);
        COPY_FIELD(w, dw, _lotag);
        cw->_hitag = dw._hitag;
        COPY_FIELD(w, dw, _extra);
        /* next-hack: brought here from prelevel() so that we will have less changing walls ?*/
        switch (w->_overpicnum)
        {
            case W_FORCEFIELD:
            case W_FORCEFIELD+1:
            case W_FORCEFIELD+2:
                if(wallshade[x] > 31)
                {
                    w->_cstat = 0;
                }
                else// wal->cstat |= 85+256;
                    w->_cstat |= 85 + 256; //setWall_cstat(i, getWall_cstat(i) | (85+256));
        }
        numstackw++;
        if (numstackw >= MAXSTACK_WALLS)
        {

          storeToInternalFlashAtPointer(stackwall, &walls[storedIdx], sizeof (walltype) * numstackw);
          storeToInternalFlashAtPointer(stackconstwall, &constwall[storedIdx], sizeof (const_walltype) * numstackw);
          storedIdx += numstackw;
          numstackw = 0;
        }
    }
    // store remaining
    if (numstackw)
    {

      storeToInternalFlashAtPointer(stackwall, &walls[storedIdx], sizeof (walltype) * numstackw);
      storeToInternalFlashAtPointer(stackconstwall, &constwall[storedIdx], sizeof (const_walltype) * numstackw);
      storedIdx += numstackw;
      numstackw = 0;
    }
    //
    #if USE_BLOCKMAP
        createBlockmap();
    #endif
    //
    sprites = getDisplayFrameBuffer(0);     // we are using the framebuffer to temporarily store sprites. It will be large enough.
    memset((void*) sprites, 0, getFrameBufferSize());
    memset(ramspriteIdx, 0, sizeof(ramspriteIdx));
    memset(ramsprites_allocated_bf, 0, sizeof(ramsprites_allocated_bf));

    initspritelists();

    extMemGetDataFromCurrentAddress(&numsprites, 2);
#if WIN32 & 0
    printf("Num sprites in this map %d\r\n", numsprites);
#endif
#ifdef WIN32
    loadedNumSprites = numsprites;
    if (numsprites > MAXSPRITES)
    {
        FIXME("This is going bad. There are %d sprites, where only %d are defined\r\n", numsprites, MAXSPRITES);
    }
#endif
    spritetype *ss;
    for (int i = 0; i< MAXSPRITES; i++)
        ((spritetype *)sprites)[i]._statnum = MAXSTATUS;
    int numPlayersFound = 0;
    for (x = 0,  ss = (spritetype *) &sprites[0]; x < numsprites; x++, ss++)
    {
        disk_spritetype sp;
        disk_spritetype *s = &sp;
        extMemGetDataFromCurrentAddress(&s->x, 4);
        extMemGetDataFromCurrentAddress(&s->y, 4);
        extMemGetDataFromCurrentAddress(&s->z, 4);
        extMemGetDataFromCurrentAddress(&s->cstat, 2);
        extMemGetDataFromCurrentAddress(&s->picnum, 2);
        extMemGetDataFromCurrentAddress((uint8_t *)&s->shade, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&s->pal, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&s->clipdist, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&s->filler, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&s->xrepeat, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&s->yrepeat, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&s->xoffset, 1);
        extMemGetDataFromCurrentAddress((uint8_t *)&s->yoffset, 1);
        extMemGetDataFromCurrentAddress(&s->sectnum, 2);
        extMemGetDataFromCurrentAddress(&s->statnum, 2);
        extMemGetDataFromCurrentAddress(&s->ang, 2);
        extMemGetDataFromCurrentAddress(&s->owner, 2);
        extMemGetDataFromCurrentAddress(&s->xvel, 2);
        extMemGetDataFromCurrentAddress(&s->yvel, 2);
        extMemGetDataFromCurrentAddress(&s->zvel, 2);
        extMemGetDataFromCurrentAddress(&s->lotag, 2);
        extMemGetDataFromCurrentAddress(&s->hitag, 2);
        extMemGetDataFromCurrentAddress(&s->extra, 2);
        // next-hack: disable player respawn sites exceeding MAXPLAYERS.
        if (s->picnum == APLAYER)
        {
            numPlayersFound++;
            if (numPlayersFound > MAXPLAYERORIGINS)
            {
                // dirty hack
                //s->statnum = MAXSTATUS;
                memset(s, 0, sizeof(*s));
                s->picnum = -1;
                s->cstat = SPRITE_INVISIBLE;
            }
        }
        //
        ss->_x = s->x;
        ss->_y = s->y;
        ss->_z = s->z;
        ss->_cstat = s->cstat;
        ss->_picnum = s->picnum;
        ss->_shade =s->shade;
        ss->_pal = s->pal;
#if SPRITETYPE_HAS_CLIPDIST
        ss->_clipdist =s->clipdist;
#else
    spr_add_data[x].clipdist = s->clipdist;
#endif
        //ss->filler = s->filler;
        ss->_xrepeat = s->xrepeat;
        ss->_yrepeat = s->yrepeat;
#if SPRITETYPE_HAS_OFFSETS
        ss->_xoffset = s->xoffset;
        ss->_yoffset = s->yoffset;
#else
        sprite_offsets[x].xoffset = s->xoffset;
        sprite_offsets[x].yoffset = s->yoffset;
#endif
        ss->_sectnum = s->sectnum;
        ss->_statnum = s->statnum;
        ss->_ang = s->ang;
        ss->_owner = s->owner;
        ss->_xvel = s->xvel;
        ss->_yvel = s->yvel;
        ss->_zvel = s->zvel;
        ss->_lotag = s->lotag;
        ss->_hitag = s->hitag;
        ss->_extra = s->extra;
    }


// it is safe to use sprites as array here.
    for(i=0; i<numsprites; i++)
        insertsprite(sprites[i]._sectnum,sprites[i]._statnum);

    /* Must be after loading sectors, etc! */
    updatesector(*daposx,*daposy,dacursectnum);

    // FIX_00009: Show map CRC and GRP file version of each player in case of Out Of Synch
#if USE_CRC
    mapCRC = crc16((uint8_t *)sector, numsectors*sizeof(sectortype));
    mapCRC += crc16((uint8_t *)wall, numwalls*sizeof(walltype));
    mapCRC += crc16((uint8_t *)sprite, numsprites*sizeof(spritetype));
#endif
    printf("Map %s stat: Sprites %d, walls %d, sectors %d. Size: %d\r\n", filename, numsprites, numwalls, numsectors, numsprites * (sizeof (spritetype)
            + sizeof(spritetypeoffs)) + numsectors * sizeof(ramsectors[0]));
    return(0);
}
#if WIN32


extern const char level_file_names[44][10];
static int checkSize(int minimum, int maximum)
{
    int numbits = 1;
    while (((1 << numbits) - 1) < maximum)
    {
        numbits ++;
    }
    if (minimum < 0)
    {
        // if minimum < 0, then we need to add a bit for sign
        numbits++;
        minimum = ~minimum;
        while (((1 << (numbits - 1) )) < minimum)
        {
            numbits ++;
        }
    }

    return numbits;
}
#define FINDMIN(minimum, field, test) do{ if (minimum.field > test.field) minimum.field = test.field; }while(0)
#define FINDMAX(maximum, field, test) do{ if (maximum.field < test.field) maximum.field = test.field; }while(0)
#define FINDMINMAX(minimum, maximum, field, test) do {FINDMIN(minimum, field, test); FINDMAX(maximum, field, test); }while(0)
#define PRINTSTAT(minimum, maximum, field) do{ printf("%s.%s is 0x%08x, %s.%s is 0x%08x. Minimum field size %d bits\r\n", \
                                                      #minimum, \
                                                      #field,  \
                                                      (unsigned int) (minimum.field & ( ((uint64_t)1 << (8 * sizeof(minimum.field) )) - 1)), \
                                                      #maximum, \
                                                      #field, \
                                                      (unsigned int) (maximum.field & ( ((uint64_t) 1 << (8 * sizeof(maximum.field) )) - 1)), \
                                                      checkSize(minimum.field, maximum.field)); } while(0)
#define MAXINT 0x7FFFFFFF
#define MININT 0x80000000
#define MAXCHAR 0x7F
#define MINCHAR 0x80
#define MAXSHORT 0x7FFF
#define MINSHORT 0x8000
void analyzeLevels(void)
{
    int32_t daposx;
    int32_t daposy;
    int32_t daposz;
    short daang;
    short dacursectnum;
    int maxWalls = 0;
    int maxSprites = 0;
    int maxSectors = 0;
    //
    disk_walltype minWall =
    {
        .x = MAXINT,
        .y = MAXINT,
        ._point2 = MAXSHORT,
        .nextwall = MAXSHORT,
        .nextsector= MAXSHORT,
        ._cstat = MAXSHORT,
        ._picnum = MAXSHORT,
        ._overpicnum = MAXSHORT,
        ._shade = MAXCHAR,
        ._pal = 0xFF,
        ._xrepeat = 0xFF,
        ._yrepeat = 0xFF,
        ._xpanning = 0xFF,
        ._ypanning = 0xFF,
        ._lotag = MAXSHORT,
        ._hitag = MAXSHORT,
        ._extra = MAXSHORT,
    };
    disk_walltype maxWall =
    {
        .x = MININT,
        .y = MININT,
        ._point2 = MINSHORT,
        .nextwall = MINSHORT,
        .nextsector= MINSHORT,
        ._cstat = MINSHORT,
        ._picnum = MINSHORT,
        ._overpicnum = MINSHORT,
        ._shade = MINCHAR,
        ._pal = 0,
        ._xrepeat = 0,
        ._yrepeat = 0,
        ._xpanning = 0,
        ._ypanning = 0,
        ._lotag = MINSHORT,
        ._hitag = MINSHORT,
        ._extra = MINSHORT,
    };
    disk_sectortype minSector =
    {
        .wallptr = MAXSHORT,
        .wallnum = MAXSHORT,
        .ceilingz = MAXINT,
        .floorz = MAXINT,
        .ceilingstat = MAXSHORT,
        .floorstat = MAXSHORT,
        .ceilingpicnum = MAXSHORT,
        .ceilingheinum = MAXSHORT,
        .ceilingshade = MAXCHAR,
        .ceilingpal = 0xFF,
        .ceilingxpanning = 0xFF,
        .ceilingypanning = 0xFF,
        .floorpicnum = MAXSHORT,
        .floorheinum = MAXSHORT,
        .floorshade = MAXCHAR,
        .floorpal = 0xFF,
        .floorxpanning = 0xFF,
        .floorypanning = 0xFF,
        .visibility = 0xFF,
        .filler = 0xFF,
        .lotag = MAXSHORT,
        .hitag = MAXSHORT,
        .extra = MAXSHORT
    };
    disk_sectortype maxSector =
    {
        .wallptr = MINSHORT,
        .wallnum = MINSHORT,
        .ceilingz = MININT,
        .floorz = MININT,
        .ceilingstat = MINSHORT,
        .floorstat = MINSHORT,
        .ceilingpicnum = MINSHORT,
        .ceilingheinum = MINSHORT,
        .ceilingshade = MINCHAR,
        .ceilingpal = 0,
        .ceilingxpanning = 0,
        .ceilingypanning = 0,
        .floorpicnum = MINSHORT,
        .floorheinum = MINSHORT,
        .floorshade = MINCHAR,
        .floorpal = 0,
        .floorxpanning = 0,
        .floorypanning = 0,
        .visibility = 0,
        .filler = 0,
        .lotag = MINSHORT,
        .hitag = MINSHORT,
        .extra = MINSHORT
    };
    #if ATOMIC_EDITION
    int maxlevels = 43;
    #else
    int maxlevels = 33;
    #endif
    for (int l = 0; l < maxlevels; l++)
    {
        loadboard(level_file_names[l], &daposx, &daposy, &daposz, &daang, &dacursectnum);
        // walls
        printf("Level %d, name %s, sprites: %d, walls %d, sectors %d\r\n", l, level_file_names[l], loadedNumSprites, numwalls, numsectors);
        if (numwalls > maxWalls)
            maxWalls = numwalls;
        if (numsectors > maxSectors)
            maxSectors = numsectors;
        if (loadedNumSprites > maxSprites)
            maxSprites = loadedNumSprites;
        //
        for (int i = 0; i < numwalls; i++)
        {
            FINDMINMAX(minWall, maxWall, x, (*getWall(i)));
            FINDMINMAX(minWall, maxWall, y, (*getWall(i)));
         //   FINDMINMAX(minWall, maxWall, _point2, (*getWall(i)));
            FINDMINMAX(minWall, maxWall, nextsector, constwall[i]);
            FINDMINMAX(minWall, maxWall, nextwall, constwall[i]);
            FINDMINMAX(minWall, maxWall, _cstat, (*getWall(i)));
            FINDMINMAX(minWall, maxWall, _picnum, (*getWall(i)));
            FINDMINMAX(minWall, maxWall, _overpicnum, (*getWall(i)));
//            FINDMINMAX(minWall, maxWall, _shade, (*getWall(i)));
            FINDMINMAX(minWall, maxWall, _pal, (*getWall(i)));
            FINDMINMAX(minWall, maxWall, _xrepeat, constwall[i]);
            FINDMINMAX(minWall, maxWall, _yrepeat, constwall[i]);
            FINDMINMAX(minWall, maxWall, _xpanning, (*getWall(i)));
            FINDMINMAX(minWall, maxWall, _ypanning, (*getWall(i)));
            FINDMINMAX(minWall, maxWall, _lotag, (*getWall(i)));
//            FINDMINMAX(minWall, maxWall, _hitag, (*getWall(i)));
            FINDMINMAX(minWall, maxWall, _extra, (*getWall(i)));
        }
        for (int i = 0; i < numsectors; i++)
        {
            sectortype s =  *getSector(i);
//            FINDMINMAX(minSector, maxSector, wallptr, s);
 //           FINDMINMAX(minSector, maxSector, wallnum, s);
            FINDMINMAX(minSector, maxSector, ceilingz, s);
            FINDMINMAX(minSector, maxSector, floorz, s);
            FINDMINMAX(minSector, maxSector, ceilingstat, s);
            FINDMINMAX(minSector, maxSector, floorstat, s);
            FINDMINMAX(minSector, maxSector, ceilingpicnum, s);
//            FINDMINMAX(minSector, maxSector, ceilingheinum, s);
#if !SEPARATE_SECTOR_CEILINGSHADE
            FINDMINMAX(minSector, maxSector, ceilingshade, s);
#else
// not supported
#endif
            FINDMINMAX(minSector, maxSector, ceilingpal, s);
            FINDMINMAX(minSector, maxSector, ceilingxpanning, s);
            FINDMINMAX(minSector, maxSector, ceilingypanning, s);
            FINDMINMAX(minSector, maxSector, floorpicnum, s);
            FINDMINMAX(minSector, maxSector, floorheinum, s);
#if !SEPARATE_SECTOR_FLOORSHADE
            FINDMINMAX(minSector, maxSector, floorshade, s);
#endif
            FINDMINMAX(minSector, maxSector, floorpal, s);
#if !SEPARATE_SECTOR_FLOORXPANNING
            FINDMINMAX(minSector, maxSector, floorxpanning, s);
#endif
            FINDMINMAX(minSector, maxSector, floorypanning, s);
//            FINDMINMAX(minSector, maxSector, visibility, s);
//            FINDMINMAX(minSector, maxSector, filler, s);
            FINDMINMAX(minSector, maxSector, lotag, s);
            FINDMINMAX(minSector, maxSector, hitag, s);
#if !SEPARATE_SECTOR_EXTRA
            FINDMINMAX(minSector, maxSector, extra, s);
#endif
        }
    }
    printf("---- LIMITS ----\r\n");
    printf("Max Sprites: %d\r\n", maxSprites);
    printf("Max Walls: %d\r\n", maxWalls);
    printf("Max Sectors: %d\r\n", maxSectors);
    //
    //
    printf("> Walls\r\n");
            PRINTSTAT(minWall, maxWall, x);
            PRINTSTAT(minWall, maxWall, y);
            PRINTSTAT(minWall, maxWall, _point2);
            PRINTSTAT(minWall, maxWall, nextwall);
            PRINTSTAT(minWall, maxWall, nextsector);
            PRINTSTAT(minWall, maxWall, _cstat);
            PRINTSTAT(minWall, maxWall, _picnum);
            PRINTSTAT(minWall, maxWall, _overpicnum);
//            PRINTSTAT(minWall, maxWall, _shade);
            PRINTSTAT(minWall, maxWall, _pal);
            PRINTSTAT(minWall, maxWall, _xrepeat);
            PRINTSTAT(minWall, maxWall, _yrepeat);
            PRINTSTAT(minWall, maxWall, _xpanning);
            PRINTSTAT(minWall, maxWall, _ypanning);
            PRINTSTAT(minWall, maxWall, _lotag);
            PRINTSTAT(minWall, maxWall, _hitag);
            PRINTSTAT(minWall, maxWall, _extra);
    printf("> Sectors\r\n");
            PRINTSTAT(minSector, maxSector, wallptr);
            PRINTSTAT(minSector, maxSector, wallnum);
            PRINTSTAT(minSector, maxSector, ceilingz);
            PRINTSTAT(minSector, maxSector, floorz);
            PRINTSTAT(minSector, maxSector, ceilingstat);
            PRINTSTAT(minSector, maxSector, floorstat);
            PRINTSTAT(minSector, maxSector, ceilingpicnum);
            PRINTSTAT(minSector, maxSector, ceilingheinum);
            PRINTSTAT(minSector, maxSector, ceilingshade);
            PRINTSTAT(minSector, maxSector, ceilingpal);
            PRINTSTAT(minSector, maxSector, ceilingxpanning);
            PRINTSTAT(minSector, maxSector, ceilingypanning);
            PRINTSTAT(minSector, maxSector, floorpicnum);
            PRINTSTAT(minSector, maxSector, floorheinum);
            PRINTSTAT(minSector, maxSector, floorshade);
            PRINTSTAT(minSector, maxSector, floorpal);
            PRINTSTAT(minSector, maxSector, floorxpanning);
            PRINTSTAT(minSector, maxSector, floorypanning);
            PRINTSTAT(minSector, maxSector, visibility);
            PRINTSTAT(minSector, maxSector, filler);
            PRINTSTAT(minSector, maxSector, lotag);
            PRINTSTAT(minSector, maxSector, hitag);
            PRINTSTAT(minSector, maxSector, extra);
}

#endif
#if LOAD_TABLES
static void initksqrt(void)
{
    int32_t i, j, k;

    j = 1;
    k = 0;
    for(i=0; i<4096; i++)
    {
        if (i >= j) {
            j <<= 2;
            k++;
        }
        sqrtable[i] = (uint16_t)(msqrtasm((i<<18)+131072)<<1);
        shlookup[i] = (k<<1)+((10-k)<<8);
        if (i < 256) shlookup[i+4096] = ((k+6)<<1)+((10-(k+6))<<8);
    }
}


static void loadtables(void)
{
    int32_t i, fil;

    if (tablesloaded == 0)
    {
        initksqrt();

        for(i=0; i<2048; i++) reciptable[i] = divscale30(2048L,i+2048);  // (2048 << 30) / (i + 2048)

        if ((fil = TCkopen4load("tables.dat",LOAD_ONLY_GRP)) != -1)
        {
            for (i = 0; i < 2048; i++)
                kread16(fil,&sintable[i]);

            for (i = 0; i < 640; i++)
                kread16(fil,&radarang[i]);

            for(i=0; i<640; i++) radarang[1279-i] = -radarang[i];
            kread(fil,textfont,1024);
            kread(fil,smalltextfont,1024);
            kread(fil,britable,1024);
            kclose(fil);
        }
        tablesloaded = 1;
    }
    //
#if PRINT_TABLES
    printf ("// Tables, do not edit!\r\n");
    PRINT_ARRAY(sintable, short);
    PRINT_ARRAY(sqrtable, uint16_t);
    PRINT_ARRAY(shlookup, uint16_t);
    PRINT_ARRAY(reciptable, int32_t);
    PRINT_ARRAY(radarang, short);
    PRINT_ARRAY(textfont, uint8_t);
    PRINT_ARRAY(smalltextfont, uint8_t);
    PRINT_DOUBLE_ARRAY(britable, uint8_t, 16, 64);
    //
#endif
}
#endif

static void initfastcolorlookup(int32_t rscale, int32_t gscale, int32_t bscale)
{
    int32_t i, j, x, y, z;
    const uint8_t  *pal1;

    j = 0;
    int32_t stack_rdist[129], stack_gdist[129], stack_bdist[129];
    uint8_t  stack_colhere[((FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2))>>3];
    uint8_t  stack_colhead[(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)];
    int32_t stack_colnext[256];
    int32_t stack_colscan[27];
    //
    for(i=64; i>=0; i--)
    {
        /*j = (i-64)*(i-64);*/
        stack_rdist[i] = stack_rdist[128-i] = j*rscale;
        stack_gdist[i] = stack_gdist[128-i] = j*gscale;
        stack_bdist[i] = stack_bdist[128-i] = j*bscale;
        j += 129-(i<<1);
    }

    clearbufbyte(stack_colhere, sizeof(stack_colhere), 0L);
    clearbufbyte(stack_colhead, sizeof(stack_colhead), 0L);

    pal1 = &palette[768-3];
    for(i=255; i>=0; i--,pal1-=3)
    {
        j = (pal1[0]>>3)*FASTPALGRIDSIZ*FASTPALGRIDSIZ+(pal1[1]>>3)*FASTPALGRIDSIZ+(pal1[2]>>3)+FASTPALGRIDSIZ*FASTPALGRIDSIZ+FASTPALGRIDSIZ+1;
        if (stack_colhere[j>>3]&pow2char[j&7]) stack_colnext[i] = stack_colhead[j];
        else stack_colnext[i] = -1;
        stack_colhead[j] = i;
        stack_colhere[j>>3] |= pow2char[j&7];
    }

    i = 0;
    for(x=-FASTPALGRIDSIZ*FASTPALGRIDSIZ; x<=FASTPALGRIDSIZ*FASTPALGRIDSIZ; x+=FASTPALGRIDSIZ*FASTPALGRIDSIZ)
        for(y=-FASTPALGRIDSIZ; y<=FASTPALGRIDSIZ; y+=FASTPALGRIDSIZ)
            for(z=-1; z<=1; z++)
                stack_colscan[i++] = x+y+z;
    i = stack_colscan[13];
    stack_colscan[13] = stack_colscan[26];
    stack_colscan[26] = i;
    //
    rdist = storeToInternalFlashAlign(stack_rdist, sizeof(stack_rdist));
    gdist = storeToInternalFlashAlign(stack_gdist, sizeof(stack_gdist));
    bdist = storeToInternalFlashAlign(stack_bdist, sizeof(stack_bdist));
    colhere = storeToInternalFlashAlign(stack_colhere, sizeof(stack_colhere));
    colhead = storeToInternalFlashAlign(stack_colhead, sizeof(stack_colhead));
    colnext = storeToInternalFlashAlign(stack_colnext, sizeof(stack_colnext));
    colscan = storeToInternalFlashAlign(stack_colscan, sizeof(stack_colscan));
}

//extern uint8_t lastPalette[768];
static void loadpalette(void)
{


    if (paletteloaded != 0)
        return;
#if USE_EXT_MEM
    uint8_t * p = kGetExtMemAddress( "palette.dat", 0);
    if (!p)
        return;
    uint8_t stackPalette[768];
    extMemGetDataFromAddress(stackPalette, p, 768);
    stackPalette[767] = stackPalette[766] = stackPalette[765] = 0;
    palette = storeToInternalFlash(stackPalette, 768);
    p += 768;

    extMemGetDataFromAddress(&numpalookups, p, sizeof(numpalookups));
    //
    p += 2;
    //CODE EXPLORATION
    printf("Num palettes lookup: %d.\n",numpalookups);

    //Transluctent palette is 65KB.


    globalpal = 0;

    //    kread(fil,palookup[globalpal],numpalookups<<8);

    palookup = reserveInternalFlashSize(MAXPALOOKUPS * sizeof(*palookup));

    void *palette = storeToInternalFlash(p, numpalookups<<8);

    storeToInternalFlashAtPointer(&palette, &palookup[0], sizeof(*palookup));
    p += numpalookups<< 8;
    globalpalwritten = palookup[0];

    /*kread(fil,transluc,65536);*/
//    for (k = 0; k < (65536 / 4); k++)
//        kread32(fil, ((int32_t *) transluc) + k);

    transluc = storeToInternalFlash(p, 65536);


    initfastcolorlookup(30L,59L,11L);




#else
    int32_t k, fil;
    if ((fil = TCkopen4load("palette.dat",LOAD_ONLY_GRP)) == -1)
        return;

    kread(fil,palette,768);

    //CODE EXPLORATION
    //WritePaletteToFile(palette,"palette.tga",16, 16);
    memcpy(lastPalette, palette, 768);


    kread16(fil,&numpalookups);

    //CODE EXPLORATION
    //printf("Num palettes lookup: %d.\n",numpalookups);

    if ((palookup[0] = (uint8_t  *)malloc(numpalookups<<8)) == NULL)
/*code disabled*/        allocache(&palookup[0],numpalookups<<8,&permanentlock);

    //Transluctent palette is 65KB.
    if ((transluc = (uint8_t  *)malloc(65536)) == NULL)
/*code disabled*/        allocache(&transluc,65536,&permanentlock);

    globalpalwritten = palookup[0];
    globalpal = 0;




    kread(fil,palookup[globalpal],numpalookups<<8);


    /*kread(fil,transluc,65536);*/
    for (k = 0; k < (65536 / 4); k++)
        kread32(fil, ((int32_t *) transluc) + k);



    kclose(fil);

    initfastcolorlookup(30L,59L,11L);

#endif
    paletteloaded = 1;

}



int setgamemode(int32_t daxdim, int32_t daydim)
{
//    strcpy(kensmessage,"!!!! BUILD engine&tools programmed by Ken Silverman of E.G. RI.  (c) Copyright 1995 Ken Silverman.  Summary:  BUILD = Ken. !!!!");

    return(_setgamemode(daxdim, daydim));
}

void uninitengine(void)
{
#if CODE_DISABLED
    if (transluc != NULL) {
        free(transluc);
        transluc = NULL;
    }
    if (pic != NULL) {
        free(pic);
        pic = NULL;
    }
    if (artfil != -1) kclose(artfil);
    _uninitengine(); /* video driver specific. */
#endif
}

/* Assume npoints=4 with polygon on &rx1,&ry1 */
//FCS This is horrible to read: I hate you.
static int clippoly4(int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, pvWall_t *pvWalls)
{
    int32_t n, nn, z, zz, x, x1, x2, y, y1, y2, t;
    nn = 0;
    z = 0;
    do
    {
        zz = ((z+1)&3);


        x1 = pvWalls[z] .cameraSpaceCoo[0][VEC_X];
        x2 = pvWalls[zz].cameraSpaceCoo[0][VEC_X]-x1;

        if ((cx1 <= x1) && (x1 <= cx2)){
            pvWalls[nn] .cameraSpaceCoo[1][VEC_X] = x1;
            pvWalls[nn] .cameraSpaceCoo[1][VEC_Y] = pvWalls[z] .cameraSpaceCoo[0][VEC_Y];
            nn++;
        }

        if (x2 <= 0)
            x = cx2;
        else
            x = cx1;

        t = x-x1;

        if (((t-x2)^t) < 0){
            pvWalls[nn] .cameraSpaceCoo[1][VEC_X] = x;
            pvWalls[nn] .cameraSpaceCoo[1][VEC_Y] = pvWalls[z].cameraSpaceCoo[0][VEC_Y] +
            scale(t,pvWalls[zz].cameraSpaceCoo[0][VEC_Y]-pvWalls[z].cameraSpaceCoo[0][VEC_Y],x2);
            nn++;
        }

        if (x2 <= 0)
            x = cx1;
        else
            x = cx2;

        t = x-x1;

        if (((t-x2)^t) < 0){
            pvWalls[nn] .cameraSpaceCoo[1][VEC_X] = x;
            pvWalls[nn] .cameraSpaceCoo[1][VEC_Y] = pvWalls[z] .cameraSpaceCoo[0][VEC_Y]+
            scale(t,pvWalls[zz].cameraSpaceCoo[0][VEC_Y]-pvWalls[z].cameraSpaceCoo[0][VEC_Y],x2);
            nn++;
        }
        z = zz;
    } while (z != 0);
    if (nn < 3) return(0);

    n = 0;
    z = 0;
    do
    {
        zz = z+1;
        if (zz == nn)
            zz = 0;

        y1 = pvWalls[z] .cameraSpaceCoo[1][VEC_Y];
        y2 = pvWalls[zz].cameraSpaceCoo[1][VEC_Y]-y1;

        if ((cy1 <= y1) && (y1 <= cy2)){
            pvWalls[n] .cameraSpaceCoo[0][VEC_Y] = y1;
            pvWalls[n] .cameraSpaceCoo[0][VEC_X] = pvWalls[z] .cameraSpaceCoo[1][VEC_X];
            n++;
        }
        if (y2 <= 0) y = cy2;
        else y = cy1;
        t = y-y1;
        if (((t-y2)^t) < 0){
            pvWalls[n] .cameraSpaceCoo[0][VEC_Y] = y;
            pvWalls[n] .cameraSpaceCoo[0][VEC_X] =
            pvWalls[z] .cameraSpaceCoo[1][VEC_X]+scale(t,
                                                       pvWalls[zz].cameraSpaceCoo[1][VEC_X]-
                                                       pvWalls[z] .cameraSpaceCoo[1][VEC_X],y2);
            n++;
        }

        if (y2 <= 0) y = cy1;
        else y = cy2;
        t = y-y1;
        if (((t-y2)^t) < 0){
            pvWalls[n] .cameraSpaceCoo[0][VEC_Y] = y;
            pvWalls[n] .cameraSpaceCoo[0][VEC_X] =
            pvWalls[z] .cameraSpaceCoo[1][VEC_X]+scale(t,
                                                       pvWalls[zz].cameraSpaceCoo[1][VEC_X]-
                                                       pvWalls[z ].cameraSpaceCoo[1][VEC_X],y2);
            n++;
        }
        z = zz;
    } while (z != 0);
    return(n);
}


#if WIN32
#define dorotatesprite(sx, sy, z, a, picnum, dashade, dapalnum,  dastat, cx1, cy1, cx2, cy2) dorotatesprite2(sx, sy, z, a, picnum, dashade, dapalnum,  dastat, cx1, cy1, cx2, cy2, __FILE__, __LINE__)
static void dorotatesprite2 (int32_t sx, int32_t sy, int32_t z, short a, short picnum,
                            int8_t dashade, uint8_t  dapalnum, uint8_t  dastat, int32_t cx1,
                            int32_t cy1, int32_t cx2, int32_t cy2, char *file, int line)

#else
static void dorotatesprite (int32_t sx, int32_t sy, int32_t z, short a, short picnum,
                            int8_t dashade, uint8_t  dapalnum, uint8_t  dastat, int32_t cx1,
                            int32_t cy1, int32_t cx2, int32_t cy2)

#endif // WIN32
{
    // next-hack:
    // dastat: bit 6 (64) == 0 => masked,                           1 => solid
    // dastat: bit 4 (16) == 0 => no offset                         1 => offset defined in animflag bits 8-15 and 16-23.
    // dastat: bit 2 (4) == 0 => no change in offset                1 => offset is with respect tile height.
    // dastat: bit 1 (2) == 0 => no scaling to window size          1 => scale to window size.
    // dastat: bit 3 (8)
    // BIG WARNING ---------------------------------------------------------------------------
    // BIG WARNING
    // BIG WARNING   !!!!! IF THE ANGLE IS NOT 0, THEN THE SPRITE IS LOADED TO RAM. YOU
    // BIG WARNING         MUST SET THE setTempRamTileData AND CALL clearBufferedTileImagePtr!
    // BIG WARNING   (oh, you need lastx in stack as well!)
    // BIG WARNING ---------------------------------------------------------------------------
    #define SHOW_ROTATESPRITE_DEBUG 0
    #if WIN32 && SHOW_ROTATESPRITE_DEBUG
    printf("dorotatesprite: File %s, line %d data: %p\r\n", file, line, tileData[picnum]);
    #endif // WIN32
    int32_t cosang, sinang, v, nextv, dax1, dax2, oy, bx, by, ny1, ny2;
    int32_t i, x, y, x1, y1, x2, y2, gx1, gy1;
    const uint8_t* bufplc;
    const uint8_t* palookupoffs;
    uint8_t* p;
    int32_t xoff, yoff, npoints, yplc, yinc, lx, rx;
    int32_t xv, yv, xv2, yv2, obuffermode=0, qlinemode=0;
    short tileWidht, tileHeight;

    uint8_t coldata[2][MAX_COL_BUF];
    #if STACK_PVWALLS

    pvWall_t pvWalls[16];   // TBD: enough?
    #endif // STACK_PVWALLS
    tileWidht = tiles[picnum].dim.width;
    tileHeight = tiles[picnum].dim.height;

    if (dastat&16) {
        xoff = 0;
        yoff = 0;
    }
    else{
        xoff = (int32_t)((int8_t )((tiles[picnum].animFlags>>8)&255))+(tileWidht>>1);
        yoff = (int32_t)((int8_t )((tiles[picnum].animFlags>>16)&255))+(tileHeight>>1);
    }

    if (dastat&4)
        yoff = tileHeight-yoff;

    cosang = sintable[(a+512)&2047];
    sinang = sintable[a&2047];

    if ((dastat&2) != 0)  /* Auto window size scaling */
    {
        if ((dastat&8) == 0)
        {
            x = xdimenscale;   /* = scale(xdimen,yxaspect,320); */
#if USE_STEREO_VIDEO
            if (stereomode) x = scale(windowx2-windowx1+1,yxaspect,320);
#endif
            sx = ((cx1+cx2+2)<<15)+scale(sx-(320<<15),xdimen,320);
            sy = ((cy1+cy2+2)<<15)+mulscale16(sy-(200<<15),x);
        }
        else
        {
            /*
             * If not clipping to startmosts, & auto-scaling on, as a
             *  hard-coded bonus, scale to full screen instead
             */
            x = scale(xdim,yxaspect,320);
            sx = (xdim<<15)+32768+scale(sx-(320<<15),xdim,320);
            sy = (ydim<<15)+32768+mulscale16(sy-(200<<15),x);
        }
        z = mulscale16(z,x);
    }

    xv = mulscale14(cosang,z);
    yv = mulscale14(sinang,z);
    if (((dastat&2) != 0) || ((dastat&8) == 0)) /* Don't aspect unscaled perms */
    {
        xv2 = mulscale16(xv,xyaspect);
        yv2 = mulscale16(yv,xyaspect);
    }
    else
    {
        xv2 = xv;
        yv2 = yv;
    }

    //Taking care of the Y coordinates.
    pvWalls[0].cameraSpaceCoo[0][VEC_Y] = sy - (yv*xoff + xv*yoff);
    pvWalls[1].cameraSpaceCoo[0][VEC_Y] = pvWalls[0].cameraSpaceCoo[0][VEC_Y] + yv * tileWidht;
    pvWalls[3].cameraSpaceCoo[0][VEC_Y] = pvWalls[0].cameraSpaceCoo[0][VEC_Y] + xv * tileHeight;

    pvWalls[2].cameraSpaceCoo[0][VEC_Y] = pvWalls[1].cameraSpaceCoo[0][VEC_Y] +
                                          pvWalls[3].cameraSpaceCoo[0][VEC_Y] -
                                          pvWalls[0].cameraSpaceCoo[0][VEC_Y] ;

    i = (cy1<<16);

    if ((pvWalls[0].cameraSpaceCoo[0][VEC_Y]<i) &&
        (pvWalls[1].cameraSpaceCoo[0][VEC_Y]<i) &&
        (pvWalls[2].cameraSpaceCoo[0][VEC_Y]<i) &&
        (pvWalls[3].cameraSpaceCoo[0][VEC_Y]<i))
        return;

    i = (cy2<<16);

    if ((pvWalls[0].cameraSpaceCoo[0][VEC_Y]>i) &&
        (pvWalls[1].cameraSpaceCoo[0][VEC_Y]>i) &&
        (pvWalls[2].cameraSpaceCoo[0][VEC_Y]>i) &&
        (pvWalls[3].cameraSpaceCoo[0][VEC_Y]>i))
        return;



    //Taking care of the X coordinates.
    pvWalls[0].cameraSpaceCoo[0][VEC_X] = sx - (xv2*xoff - yv2*yoff);
    pvWalls[1].cameraSpaceCoo[0][VEC_X] = pvWalls[0].cameraSpaceCoo[0][VEC_X] + xv2 * tileWidht;
    pvWalls[3].cameraSpaceCoo[0][VEC_X] = pvWalls[0].cameraSpaceCoo[0][VEC_X] - yv2 * tileHeight;
    pvWalls[2].cameraSpaceCoo[0][VEC_X] = pvWalls[1].cameraSpaceCoo[0][VEC_X] +
                                          pvWalls[3].cameraSpaceCoo[0][VEC_X] -
                                          pvWalls[0].cameraSpaceCoo[0][VEC_X] ;

    i = (cx1<<16);
    if ((pvWalls[0].cameraSpaceCoo[0][VEC_X]<i) &&
        (pvWalls[1].cameraSpaceCoo[0][VEC_X]<i) &&
        (pvWalls[2].cameraSpaceCoo[0][VEC_X]<i) &&
        (pvWalls[3].cameraSpaceCoo[0][VEC_X]<i))
        return;

    i = (cx2<<16);
    if ((pvWalls[0].cameraSpaceCoo[0][VEC_X]>i) &&
        (pvWalls[1].cameraSpaceCoo[0][VEC_X]>i) &&
        (pvWalls[2].cameraSpaceCoo[0][VEC_X]>i) &&
        (pvWalls[3].cameraSpaceCoo[0][VEC_X]>i))
        return;

    gx1 = pvWalls[0].cameraSpaceCoo[0][VEC_X];
    gy1 = pvWalls[0].cameraSpaceCoo[0][VEC_Y];   /* back up these before clipping */

    if ((npoints = clippoly4(cx1<<16,cy1<<16,(cx2+1)<<16,(cy2+1)<<16, pvWalls)) < 3) return;

    lx = pvWalls[0].cameraSpaceCoo[0][VEC_X];
    rx = pvWalls[0].cameraSpaceCoo[0][VEC_X];

    nextv = 0;
    for(v=npoints-1; v>=0; v--)
    {
        x1 = pvWalls[    v].cameraSpaceCoo[0][VEC_X];
        x2 = pvWalls[nextv].cameraSpaceCoo[0][VEC_X];
        dax1 = (x1>>16);
        if (x1 < lx) lx = x1;
        dax2 = (x2>>16);
        if (x1 > rx) rx = x1;
        if (dax1 != dax2)
        {
            y1 = pvWalls[    v].cameraSpaceCoo[0][VEC_Y];
            y2 = pvWalls[nextv].cameraSpaceCoo[0][VEC_Y];
            yinc = divscale16(y2-y1,x2-x1);
            if (dax2 > dax1)
            {
                yplc = y1 + mulscale16((dax1<<16)+65535-x1,yinc);
                qinterpolatedown16short((short *)(&uplc[dax1]),dax2-dax1,yplc,yinc);
            }
            else
            {
                yplc = y2 + mulscale16((dax2<<16)+65535-x2,yinc);
                qinterpolatedown16short((short *)(&dplc[dax2]),dax1-dax2,yplc,yinc);
            }
        }
        nextv = v;
    }

    TILE_MakeAvailable(picnum);

    setgotpic(picnum);
    bufplc = tileData[picnum];

    palookupoffs = palookup[dapalnum] + (getpalookup(0L,(int32_t)dashade)<<8);

    i = divscale32(1L,z);
    xv = mulscale14(sinang,i);
    yv = mulscale14(cosang,i);
    if (((dastat&2) != 0) || ((dastat&8) == 0)) /* Don't aspect unscaled perms */
    {
        yv2 = mulscale16(-xv,yxaspect);
        xv2 = mulscale16(yv,yxaspect);
    }
    else
    {
        yv2 = -xv;
        xv2 = yv;
    }

    x1 = (lx>>16);
    x2 = (rx>>16);

    oy = 0;
    x = (x1<<16)-1-gx1;
    y = (oy<<16)+65535-gy1;
    bx = dmulscale16(x,xv2,y,xv);
    by = dmulscale16(x,yv2,y,yv);
    if (dastat&4) {
        yv = -yv;
        yv2 = -yv2;
        by = (tileHeight<<16)-1-by;
    }

    if ((vidoption == 1) && (origbuffermode == 0))
    {
        if (dastat&128)
        {
            obuffermode = buffermode;
            buffermode = 0;

        }
    }
    else if (dastat&8)
        permanentupdate = 1;

    if ((dastat&1) == 0)
    {

        if (((a&1023) == 0) && (tileHeight <= 256))  /* vlineasm4 has 256 high limit! */
        {
 //           if (dastat&64)
                setupvlineasm(24L);
 //           else
 //               setupmvlineasm(24L);

            by <<= 8;
            yv <<= 8;
            yv2 <<= 8;
#define SIMPLIFY_DOROTATE 1
#if SIMPLIFY_DOROTATE
//            palookupoffse[0] = palookupoffse[1] = palookupoffse[2] = palookupoffse[3] = palookupoffs;
//            vince[0] = vince[1] = vince[2] = vince[3] = yv;
   #if 0
            int o_ibx = -1; //old column
            uint8_t * tilebufpos = 0;

            for(x=x1; x<x2; x++)
                {
                    bx += xv2;

                y1 = uplc[x];
                y2 = dplc[x];
                    if ((dastat&8) == 0)
                    {
                    if (startumost(x) > y1) y1 = startumost(x);
                    if (startdmost(x) < y2) y2 = startdmost(x);
                    }
                    if (y2 <= y1) continue;

                    by += yv*(y1-oy);
                    oy = y1;
                // get column
                if (o_ibx != (bx >> 16))
                {
                    tilebufpos = (bx>>16)*tileHeight+bufplc;
                    if (isOnExternalFlash(tilebufpos))
                    {
#if WIN32
                        if (tileHeight > MAX_COL_SIZE)
                        {
                            FIXME("This is going wrong. Tile height is %d\r\n", tileHeight);
                        }
#endif
                        tilebufpos = readColumnAligned(coldata, tilebufpos, tileHeight);
                    }
                    o_ibx = bx >> 16;
                }

                p = x+frameplace;
                waitRenderLine(y2);

                if (dastat&64)
                {
                    vlineasm1(yv, palookupoffs, y2 - 1 - y1, by,(const uint8_t*)tilebufpos,ylookup(y1)+p);
                }
                else
                {
                    mvlineasm1(yv,palookupoffs,y2 - 1 - y1, by,(const uint8_t*)tilebufpos,ylookup(y1)+p);
                }
            }
#else

            int oldColumn = -1; //old column
            const uint8_t * tilebufpos = NULL;
            const uint8_t *nexttilebufpos = NULL;
            int col = 0;
            int bufcol = 0;
            int oldx = x2 + 1;
            int oy2 = 0, oldby = 0;
            for(x = x1; x < x2; x++)    // find the first column to preload asynchronously
            {
                bx += xv2;

                y1 = uplc[x];
                y2 = dplc[x];
                if ((dastat&8) == 0)
                {
                    if (startumost(x) > y1) y1 = startumost(x);
                    if (startdmost(x) < y2) y2 = startdmost(x);
                }
                if (y2 <= y1) continue;
                // LOAD COLUMN
                tilebufpos = (bx >> 16) * tileHeight + bufplc;
                if (isOnExternalFlash(tilebufpos))
                {
#if WIN32
                     if (tileHeight > MAX_COL_SIZE)
                     {
                        FIXME("This is going wrong. Tile height is %d\r\n", tileHeight);
                    }
#endif
                    tilebufpos = readColumnAlignedAsync(coldata[bufcol], tilebufpos, tileHeight);
                    bufcol = 1 - bufcol;
                }
                oldColumn = (bx >> 16); // remember old texture column
                oldx = x;
                by += yv*(y1-oy);
                oy = y1;
                oy2 = y2;
                oldby = by;
                x++;
                break;
            }
            // now we can load again
            col = 1;
            for( ;x<x2; x++)
            {
                bx += xv2;

                y1 = uplc[x];
                y2 = dplc[x];
                if ((dastat&8) == 0)
                {
                    if (startumost(x) > y1) y1 = startumost(x);
                    if (startdmost(x) < y2) y2 = startdmost(x);
                }
                if (y2 <= y1) continue;

                by += yv*(y1-oy);

                // get column
                if (oldColumn != (bx >> 16))
                {
                    nexttilebufpos = (bx>>16)*tileHeight+bufplc;
                    if (isOnExternalFlash(nexttilebufpos))
                    {
#if WIN32
                        if (tileHeight > MAX_COL_SIZE)
                        {
                            FIXME("This is going wrong. Tile height is %d\r\n", tileHeight);
                        }
#endif
                        // wait until the data has been read, so that we can load a new one
                        extMemWaitAsynchDataRead();
                        nexttilebufpos = readColumnAlignedAsync(coldata[bufcol], nexttilebufpos, tileHeight);
                        bufcol = 1 - bufcol;
                    }
                    oldColumn = bx >> 16;
                }
                else
                {
                    //if (isOnExternalFlash(tilebufpos))  // still must wait before drawing
                    extMemWaitAsynchDataRead();
                    nexttilebufpos = tilebufpos;
                }
                p = oldx+frameplace;
                if (dastat&64)
                {
                    const uint8_t *palette = palookupoffs;
                    palette = getPalette(palette, oy2 - 1 - oy);
                    vlineasm1(yv, palette, oy2 - 1 - oy, oldby,(const uint8_t*)tilebufpos,ylookup(oy)+p);
                }
                else
                {
                  const uint8_t *palette = palookupoffs;
                  palette = getPalette(palette, oy2 - 1 - oy);
                  mvlineasm1(yv, palette, oy2 - 1 - oy, oldby,(const uint8_t*)tilebufpos,ylookup(oy)+p);
                }
                tilebufpos = nexttilebufpos;
                col = 1 - col;
                oldx = x;
                oldby = by;
                oy = y1;
                oy2 = y2;
            }
            if (oldx <= x2)
            {
                //if (isOnExternalFlash(tilebufpos))  // still must wait before drawing
                    extMemWaitAsynchDataRead();
                // this means last was not consumed
                p = oldx+frameplace;
                waitRenderLine(y2);
                if (dastat&64)
                {
                    const uint8_t *palette = palookupoffs;
                    palette = getPalette(palette, oy2 - 1 - oy);
                    vlineasm1(yv, palette, oy2 - 1 - oy, oldby,(const uint8_t*)tilebufpos,ylookup(oy)+p);
                }
                else
                {
                    const uint8_t *palette = palookupoffs;
                    palette = getPalette(palette, oy2 - 1 - oy);
                    mvlineasm1(yv, palette, oy2 - 1 - oy, oldby,(const uint8_t*)tilebufpos,ylookup(oy)+p);
                }
            }
#endif
#else
#error
#endif // SIMPLIFY_DOROTATE
        }
        else
        {
            if (bufferedTileImage == bufplc)
            {
                bufplc = tempRamTileData;
            }
            if (isOnExternalFlash(bufplc))
            {
 
                int tileSize = tiles[picnum].dim.width * tiles[picnum].dim.height;
#if WIN32
                if (tileSize > MAX_RAM_FLAT_SIZE)
                {
                    FIXME("Dorotatesprite: This is going to be bad. Tilesize = %d\r\n", tileSize);
                }
#endif // WIN32
                bufferedTileImage = bufplc;
                bufplc = extMemGetDataFromAddress(tempRamTileData, bufplc, tileSize);

            }

            // this is the case when we rotate the camera due to death or by 90 degrees. No need to setup, as the data is in RAM
            // actually we are going to remove this stuff.
            if (dastat&64)
            {
                if ((xv2&0x0000ffff) == 0)
                {
                    qlinemode = 1;
                    setuprhlineasm4(0L,yv2<<16,(xv2>>16)*tileHeight+(yv2>>16),palookupoffs,0L);
                }
                else
                {
                    qlinemode = 0;
                    setuprhlineasm4(xv2<<16,yv2<<16,(xv2>>16)*tileHeight+(yv2>>16),palookupoffs,tileHeight);
                }
            }
            else
                setuprmhlineasm4(xv2<<16,yv2<<16,(xv2>>16)*tileHeight+(yv2>>16),palookupoffs,tileHeight);

            y1 = uplc[x1];
            if (((dastat&8) == 0) && (startumost(x1) > y1)) y1 = startumost(x1);
            y2 = y1;
            for(x=x1; x<x2; x++)
            {

                ny1 = uplc[x]-1;
                ny2 = dplc[x];
                if ((dastat&8) == 0)
                {
                    if (startumost(x)-1 > ny1) ny1 = startumost(x)-1;
                    if (startdmost(x) < ny2) ny2 = startdmost(x);
                }
                if (ny1 < ny2-1)
                {
                    if (ny1 >= y2)
                    {
                        while (y1 < y2-1)
                        {
                            y1++;
#if FREQUENT_FAKETIMERHANDLER
                            if ((y1&31) == 0) faketimerhandler();
#endif
                            /* x,y1 */
                            bx += xv*(y1-oy);
                            by += yv*(y1-oy);
                            oy = y1;
                            waitRenderLine(y1);

                            if (dastat&64) {

                                if (qlinemode)
                                    rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,0L    ,by<<16,ylookup(y1)+x+frameplace);
                                else
                                    rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                            } else
                                rmhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                        }
                        y1 = ny1;
                    }
                    else
                    {

                        while (y1 < ny1)
                        {
                            y1++;
#if FREQUENT_FAKETIMERHANDLER
                            if ((y1&31) == 0) faketimerhandler();
#endif
                            /* x,y1 */
                            bx += xv*(y1-oy);
                            by += yv*(y1-oy);
                            oy = y1;
                            waitRenderLine(y1);
                            if (dastat&64)
                            {

                                if (qlinemode)
                                    rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,0L,by<<16,ylookup(y1)+x+frameplace);
                                else
                                    rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                            }
                            else
                            {
                                rmhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                            }
                        }
                        while (y1 > ny1) lastx[y1--] = x;
                    }
                    while (y2 > ny2)
                    {
                        y2--;
#if FREQUENT_FAKETIMERHANDLER
                        if ((y2&31) == 0) faketimerhandler();
#endif
                        /* x,y2 */
                        bx += xv*(y2-oy);
                        by += yv*(y2-oy);
                        oy = y2;
                        waitRenderLine(y2);
                        if (dastat&64)
                        {

                            if (qlinemode)
                                rhlineasm4(x-lastx[y2],(bx>>16)*tileHeight+(by>>16)+bufplc,0L    ,by<<16,ylookup(y2)+x+frameplace);
                            else
                                rhlineasm4(x-lastx[y2],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y2)+x+frameplace);
                        }
                        else
                            rmhlineasm4(x-lastx[y2],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y2)+x+frameplace);
                    }
                    while (y2 < ny2)
                    {
                        lastx[y2++] = x;
                    }
                }
                else
                {
                    while (y1 < y2-1)
                    {
                        y1++;
#if FREQUENT_FAKETIMERHANDLER
                        if ((y1&31) == 0) faketimerhandler();
#endif
                        /* x,y1 */
                        bx += xv*(y1-oy);
                        by += yv*(y1-oy);
                        oy = y1;
                        waitRenderLine(y1);
                        if (dastat&64)
                        {
                            if (qlinemode)
                                rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,0L    ,by<<16,ylookup(y1)+x+frameplace);
                            else
                                rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                        }
                        else
                            rmhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                    }
                    if (x == x2-1)
                    {
                        bx += xv2;
                        by += yv2;
                        break;
                    }

                    y1 = uplc[x+1];

                    if (((dastat&8) == 0) && (startumost(x+1) > y1))
                        y1 = startumost(x+1);

                    y2 = y1;
                }
                bx += xv2;
                by += yv2;
            }
            while (y1 < y2-1)
            {
                y1++;
#if FREQUENT_FAKETIMERHANDLER
                if ((y1&31) == 0) faketimerhandler();
#endif
                /* x2,y1 */
                bx += xv*(y1-oy);
                by += yv*(y1-oy);
                oy = y1;
                waitRenderLine(y1);
                if (dastat&64)
                {
                    if (qlinemode)
                        rhlineasm4(x2-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,0L    ,by<<16,ylookup(y1)+x2+frameplace);
                    else
                        rhlineasm4(x2-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x2+frameplace);
                } else
                    rmhlineasm4(x2-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x2+frameplace);
            }
        }
    }
    else
    {
        tsetupspritevline(palookupoffs,(xv>>16)*tileHeight,xv<<16,tileHeight,yv);

        if (dastat&32)
            settrans(TRANS_REVERSE);
        else
            settrans(TRANS_NORMAL);
        int o_ibx = -1; // old column
        const uint8_t *tilebufpos;

        for(x=x1; x<x2; x++)
        {
            bx += xv2;
            by += yv2;

            y1 = uplc[x];
            y2 = dplc[x];
            if ((dastat&8) == 0)
            {
                if (startumost(x) > y1) y1 = startumost(x);
                if (startdmost(x) < y2) y2 = startdmost(x);
            }
            if (y2 <= y1) continue;

            switch(y1-oy)
            {
            case -1:
                bx -= xv;
                by -= yv;
                oy = y1;
                break;
            case 0:
                break;
            case 1:
                bx += xv;
                by += yv;
                oy = y1;
                break;
            default:
                bx += xv*(y1-oy);
                by += yv*(y1-oy);
                oy = y1;
                break;
            }

            p = ylookup(y1)+x+frameplace;
            if (o_ibx != (bx >> 16))
            {
                tilebufpos = (bx>>16)*tileHeight+bufplc;
                if (isOnExternalFlash(tilebufpos))
                {
#if WIN32
                    if (tileHeight > MAX_COL_SIZE)
                    {
                        FIXME("This is going wrong. Tile height is %d\r\n", tileHeight);
                    }
#endif
                    tilebufpos = readColumnAligned((uint8_t*) coldata, tilebufpos, tileHeight);
                }
                o_ibx = bx >> 16;
            }
            DrawSpriteTranslucentVerticalLine(by<<16,y2-y1+1,bx<<16,tilebufpos+(by>>16),p);
            transarea += (y2-y1);
#if FREQUENT_FAKETIMERHANDLER
            faketimerhandler();
#endif
        }
    }

    if ((vidoption == 1) && (dastat&128) && (origbuffermode == 0))
    {
        buffermode = obuffermode;

    }
}


static void dorotateRAMsprite (int32_t sx, int32_t sy, int32_t z, short a, uint8_t *buffer,
                            int8_t dashade, uint8_t  dapalnum, uint8_t  dastat, int32_t cx1,
                            int32_t cy1, int32_t cx2, int32_t cy2)

{
    // next-hack:
    // dastat: bit 6 (64) == 0 => masked,                           1 => solid
    // dastat: bit 4 (16) == 0 => no offset                         1 => offset defined in animflag bits 8-15 and 16-23.
    // dastat: bit 2 (4) == 0 => no change in offset                1 => offset is with respect tile height.
    // dastat: bit 1 (2) == 0 => no scaling to window size          1 => scale to window size.
    // dastat: bit 3 (8)
    int32_t cosang, sinang, v, nextv, dax1, dax2, oy, bx, by, ny1, ny2;
    int32_t i, x, y, x1, y1, x2, y2, gx1, gy1;
    const uint8_t* bufplc;
    const uint8_t* palookupoffs;
    uint8_t* p;
    int32_t xoff, yoff, npoints, yplc, yinc, lx, rx;
    int32_t xv, yv, xv2, yv2, qlinemode=0;


    short tileWidht, tileHeight;

    #if STACK_PVWALLS

    pvWall_t pvWalls[16];  // TBD: enough?
    #endif // STACK_PVWALLS
    tileWidht = SCREEN_TILT_TILE_SIZE;
    tileHeight = SCREEN_TILT_TILE_SIZE;

    xoff = /*(int32_t)((int8_t )((tiles[picnum].animFlags>>8)&255))*/+(tileWidht>>1);
    yoff = /*(int32_t)((int8_t )((tiles[picnum].animFlags>>16)&255))*/+(tileHeight>>1);

    if (dastat&4)
        yoff = tileHeight-yoff;

    cosang = sintable[(a+512)&2047];
    sinang = sintable[a&2047];

    if ((dastat&2) != 0)  /* Auto window size scaling */
    {
        if ((dastat&8) == 0)
        {
            x = xdimenscale;   /* = scale(xdimen,yxaspect,320); */
            sx = ((cx1+cx2+2)<<15)+scale(sx-(320<<15),xdimen,320);
            sy = ((cy1+cy2+2)<<15)+mulscale16(sy-(200<<15),x);
        }
        else
        {
            /*
             * If not clipping to startmosts, & auto-scaling on, as a
             *  hard-coded bonus, scale to full screen instead
             */
            x = scale(xdim,yxaspect,320);
            sx = (xdim<<15)+32768+scale(sx-(320<<15),xdim,320);
            sy = (ydim<<15)+32768+mulscale16(sy-(200<<15),x);
        }
        z = mulscale16(z,x);
    }

    xv = mulscale14(cosang,z);
    yv = mulscale14(sinang,z);
    if (((dastat&2) != 0) || ((dastat&8) == 0)) /* Don't aspect unscaled perms */
    {
        xv2 = mulscale16(xv,xyaspect);
        yv2 = mulscale16(yv,xyaspect);
    }
    else
    {
        xv2 = xv;
        yv2 = yv;
    }

    //Taking care of the Y coordinates.
    pvWalls[0].cameraSpaceCoo[0][VEC_Y] = sy - (yv*xoff + xv*yoff);
    pvWalls[1].cameraSpaceCoo[0][VEC_Y] = pvWalls[0].cameraSpaceCoo[0][VEC_Y] + yv * tileWidht;
    pvWalls[3].cameraSpaceCoo[0][VEC_Y] = pvWalls[0].cameraSpaceCoo[0][VEC_Y] + xv * tileHeight;

    pvWalls[2].cameraSpaceCoo[0][VEC_Y] = pvWalls[1].cameraSpaceCoo[0][VEC_Y] +
                                          pvWalls[3].cameraSpaceCoo[0][VEC_Y] -
                                          pvWalls[0].cameraSpaceCoo[0][VEC_Y] ;

    i = (cy1<<16);

    if ((pvWalls[0].cameraSpaceCoo[0][VEC_Y]<i) &&
        (pvWalls[1].cameraSpaceCoo[0][VEC_Y]<i) &&
        (pvWalls[2].cameraSpaceCoo[0][VEC_Y]<i) &&
        (pvWalls[3].cameraSpaceCoo[0][VEC_Y]<i))
        return;

    i = (cy2<<16);

    if ((pvWalls[0].cameraSpaceCoo[0][VEC_Y]>i) &&
        (pvWalls[1].cameraSpaceCoo[0][VEC_Y]>i) &&
        (pvWalls[2].cameraSpaceCoo[0][VEC_Y]>i) &&
        (pvWalls[3].cameraSpaceCoo[0][VEC_Y]>i))
        return;



    //Taking care of the X coordinates.
    pvWalls[0].cameraSpaceCoo[0][VEC_X] = sx - (xv2*xoff - yv2*yoff);
    pvWalls[1].cameraSpaceCoo[0][VEC_X] = pvWalls[0].cameraSpaceCoo[0][VEC_X] + xv2 * tileWidht;
    pvWalls[3].cameraSpaceCoo[0][VEC_X] = pvWalls[0].cameraSpaceCoo[0][VEC_X] - yv2 * tileHeight;
    pvWalls[2].cameraSpaceCoo[0][VEC_X] = pvWalls[1].cameraSpaceCoo[0][VEC_X] +
                                          pvWalls[3].cameraSpaceCoo[0][VEC_X] -
                                          pvWalls[0].cameraSpaceCoo[0][VEC_X] ;

    i = (cx1<<16);
    if ((pvWalls[0].cameraSpaceCoo[0][VEC_X]<i) &&
        (pvWalls[1].cameraSpaceCoo[0][VEC_X]<i) &&
        (pvWalls[2].cameraSpaceCoo[0][VEC_X]<i) &&
        (pvWalls[3].cameraSpaceCoo[0][VEC_X]<i))
        return;

    i = (cx2<<16);
    if ((pvWalls[0].cameraSpaceCoo[0][VEC_X]>i) &&
        (pvWalls[1].cameraSpaceCoo[0][VEC_X]>i) &&
        (pvWalls[2].cameraSpaceCoo[0][VEC_X]>i) &&
        (pvWalls[3].cameraSpaceCoo[0][VEC_X]>i))
        return;

    gx1 = pvWalls[0].cameraSpaceCoo[0][VEC_X];
    gy1 = pvWalls[0].cameraSpaceCoo[0][VEC_Y];   /* back up these before clipping */

    if ((npoints = clippoly4(cx1<<16,cy1<<16,(cx2+1)<<16,(cy2+1)<<16, pvWalls)) < 3) return;

    lx = pvWalls[0].cameraSpaceCoo[0][VEC_X];
    rx = pvWalls[0].cameraSpaceCoo[0][VEC_X];

    nextv = 0;
    for(v=npoints-1; v>=0; v--)
    {
        x1 = pvWalls[    v].cameraSpaceCoo[0][VEC_X];
        x2 = pvWalls[nextv].cameraSpaceCoo[0][VEC_X];
        dax1 = (x1>>16);
        if (x1 < lx) lx = x1;
        dax2 = (x2>>16);
        if (x1 > rx) rx = x1;
        if (dax1 != dax2)
        {
            y1 = pvWalls[    v].cameraSpaceCoo[0][VEC_Y];
            y2 = pvWalls[nextv].cameraSpaceCoo[0][VEC_Y];
            yinc = divscale16(y2-y1,x2-x1);
            if (dax2 > dax1)
            {
                yplc = y1 + mulscale16((dax1<<16)+65535-x1,yinc);
                qinterpolatedown16short((short *)(&uplc[dax1]),dax2-dax1,yplc,yinc);
            }
            else
            {
                yplc = y2 + mulscale16((dax2<<16)+65535-x2,yinc);
                qinterpolatedown16short((short *)(&dplc[dax2]),dax1-dax2,yplc,yinc);
            }
        }
        nextv = v;
    }

    bufplc = buffer;

    palookupoffs = palookup[dapalnum] + (getpalookup(0L,(int32_t)dashade)<<8);

    i = divscale32(1L,z);
    xv = mulscale14(sinang,i);
    yv = mulscale14(cosang,i);
    if (((dastat&2) != 0) || ((dastat&8) == 0)) /* Don't aspect unscaled perms */
    {
        yv2 = mulscale16(-xv,yxaspect);
        xv2 = mulscale16(yv,yxaspect);
    }
    else
    {
        yv2 = -xv;
        xv2 = yv;
    }

    x1 = (lx>>16);
    x2 = (rx>>16);

    oy = 0;
    x = (x1<<16)-1-gx1;
    y = (oy<<16)+65535-gy1;
    bx = dmulscale16(x,xv2,y,xv);
    by = dmulscale16(x,yv2,y,yv);
    if (dastat&4) {
        yv = -yv;
        yv2 = -yv2;
        by = (tileHeight<<16)-1-by;
    }

    if ((vidoption == 1) && (origbuffermode == 0))
    {
        if (dastat&128)
        {
//            obuffermode = buffermode;
            buffermode = 0;

        }
    }
    else if (dastat&8)
        permanentupdate = 1;

    if ((dastat&1) == 0)
    {

        if (((a&1023) == 0) && (tileHeight <= 256))  /* vlineasm4 has 256 high limit! */
        {
 //           if (dastat&64)
                setupvlineasm(24L);
 //           else
 //               setupmvlineasm(24L);

            by <<= 8;
            yv <<= 8;
            yv2 <<= 8;
#define SIMPLIFY_DOROTATE 1
#if SIMPLIFY_DOROTATE
   #if 0
#else

            int oldColumn = -1; //old column
            const uint8_t * tilebufpos = NULL;
            const uint8_t *nexttilebufpos = NULL;
            int col = 0;
            int oldx = x2 + 1;
            int oy2 = 0, oldby = 0;
            for(x = x1; x < x2; x++)    // find the first column to preload asynchronously
            {
                bx += xv2;

                y1 = uplc[x];
                y2 = dplc[x];
                if ((dastat&8) == 0)
                {
                    if (startumost(x) > y1) y1 = startumost(x);
                    if (startdmost(x) < y2) y2 = startdmost(x);
                }
                if (y2 <= y1) continue;
                // LOAD COLUMN
                tilebufpos = (bx >> 16) * tileHeight + bufplc;

                oldColumn = (bx >> 16); // remember old texture column
                oldx = x;
                by += yv*(y1-oy);
                oy = y1;
                oy2 = y2;
                oldby = by;
                x++;
                break;
            }
            // now we can load again
            col = 1;
            for( ;x<x2; x++)
            {
                bx += xv2;

                y1 = uplc[x];
                y2 = dplc[x];
                if ((dastat&8) == 0)
                {
                    if (startumost(x) > y1) y1 = startumost(x);
                    if (startdmost(x) < y2) y2 = startdmost(x);
                }
                if (y2 <= y1) continue;

                by += yv*(y1-oy);

                // get column
                if (oldColumn != (bx >> 16))
                {
                    nexttilebufpos = (bx>>16)*tileHeight+bufplc;
                    oldColumn = bx >> 16;
                }
                else
                {
                    nexttilebufpos = tilebufpos;
                }
                p = oldx+frameplace;
                if (dastat&64)
                {
                    const uint8_t *palette = palookupoffs;
                    palette = getPalette(palette, oy2 - 1 - oy);
                    vlineasm1(yv, palette, oy2 - 1 - oy, oldby,(const uint8_t*)tilebufpos,ylookup(oy)+p);
                }
                else
                {
                  const uint8_t *palette = palookupoffs;
                  palette = getPalette(palette, oy2 - 1 - oy);
                  mvlineasm1(yv, palette, oy2 - 1 - oy, oldby,(const uint8_t*)tilebufpos,ylookup(oy)+p);
                }
                tilebufpos = nexttilebufpos;
                col = 1 - col;
                oldx = x;
                oldby = by;
                oy = y1;
                oy2 = y2;
            }
            if (oldx <= x2)
            {
                // this means last was not consumed
                p = oldx+frameplace;
                waitRenderLine(y2);
                if (dastat&64)
                {
                    const uint8_t *palette = palookupoffs;
                    palette = getPalette(palette, oy2 - 1 - oy);
                    vlineasm1(yv, palette, oy2 - 1 - oy, oldby,(const uint8_t*)tilebufpos,ylookup(oy)+p);
                }
                else
                {
                    const uint8_t *palette = palookupoffs;
                    palette = getPalette(palette, oy2 - 1 - oy);
                    mvlineasm1(yv, palette, oy2 - 1 - oy, oldby,(const uint8_t*)tilebufpos,ylookup(oy)+p);
                }
            }
#endif
#else
#error
#endif // SIMPLIFY_DOROTATE
        }
        else
        {
            bufplc = buffer;

            // this is the case when we rotate the camera due to death or by 90 degrees. No need to setup, as the data is in RAM
            // actually we are going to remove this stuff.
            if (dastat&64)
            {
                if ((xv2&0x0000ffff) == 0)
                {
                    qlinemode = 1;
                    setuprhlineasm4(0L,yv2<<16,(xv2>>16)*tileHeight+(yv2>>16),palookupoffs,0L);
                }
                else
                {
                    qlinemode = 0;
                    setuprhlineasm4(xv2<<16,yv2<<16,(xv2>>16)*tileHeight+(yv2>>16),palookupoffs,tileHeight);
                }
            }
            else
                setuprmhlineasm4(xv2<<16,yv2<<16,(xv2>>16)*tileHeight+(yv2>>16),palookupoffs,tileHeight);

            y1 = uplc[x1];
            if (((dastat&8) == 0) && (startumost(x1) > y1)) y1 = startumost(x1);
            y2 = y1;
            for(x=x1; x<x2; x++)
            {

                ny1 = uplc[x]-1;
                ny2 = dplc[x];
                if ((dastat&8) == 0)
                {
                    if (startumost(x)-1 > ny1) ny1 = startumost(x)-1;
                    if (startdmost(x) < ny2) ny2 = startdmost(x);
                }
                if (ny1 < ny2-1)
                {
                    if (ny1 >= y2)
                    {
                        while (y1 < y2-1)
                        {
                            y1++;
                            /* x,y1 */
                            bx += xv*(y1-oy);
                            by += yv*(y1-oy);
                            oy = y1;
                            waitRenderLine(y1);

                            if (dastat&64) {

                                if (qlinemode)
                                    rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,0L    ,by<<16,ylookup(y1)+x+frameplace);
                                else
                                    rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                            } else
                                rmhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                        }
                        y1 = ny1;
                    }
                    else
                    {

                        while (y1 < ny1)
                        {
                            y1++;
                            /* x,y1 */
                            bx += xv*(y1-oy);
                            by += yv*(y1-oy);
                            oy = y1;
                            waitRenderLine(y1);
                            if (dastat&64)
                            {

                                if (qlinemode)
                                    rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,0L,by<<16,ylookup(y1)+x+frameplace);
                                else
                                    rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                            }
                            else
                            {
                                rmhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                            }
                        }
                        while (y1 > ny1) lastx[y1--] = x;
                    }
                    while (y2 > ny2)
                    {
                        y2--;

                        /* x,y2 */
                        bx += xv*(y2-oy);
                        by += yv*(y2-oy);
                        oy = y2;
                        waitRenderLine(y2);
                        if (dastat&64)
                        {

                            if (qlinemode)
                                rhlineasm4(x-lastx[y2],(bx>>16)*tileHeight+(by>>16)+bufplc,0L    ,by<<16,ylookup(y2)+x+frameplace);
                            else
                                rhlineasm4(x-lastx[y2],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y2)+x+frameplace);
                        }
                        else
                            rmhlineasm4(x-lastx[y2],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y2)+x+frameplace);
                    }
                    while (y2 < ny2)
                    {
                        lastx[y2++] = x;
                    }
                }
                else
                {
                    while (y1 < y2-1)
                    {
                        y1++;
                        /* x,y1 */
                        bx += xv*(y1-oy);
                        by += yv*(y1-oy);
                        oy = y1;
                        waitRenderLine(y1);
                        if (dastat&64)
                        {
                            if (qlinemode)
                                rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,0L    ,by<<16,ylookup(y1)+x+frameplace);
                            else
                                rhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                        }
                        else
                            rmhlineasm4(x-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x+frameplace);
                    }
                    if (x == x2-1)
                    {
                        bx += xv2;
                        by += yv2;
                        break;
                    }

                    y1 = uplc[x+1];

                    if (((dastat&8) == 0) && (startumost(x+1) > y1))
                        y1 = startumost(x+1);

                    y2 = y1;
                }
                bx += xv2;
                by += yv2;
            }
            while (y1 < y2-1)
            {
                y1++;

                /* x2,y1 */
                bx += xv*(y1-oy);
                by += yv*(y1-oy);
                oy = y1;
                waitRenderLine(y1);
                if (dastat&64)
                {
                    if (qlinemode)
                        rhlineasm4(x2-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,0L    ,by<<16,ylookup(y1)+x2+frameplace);
                    else
                        rhlineasm4(x2-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x2+frameplace);
                } else
                    rmhlineasm4(x2-lastx[y1],(bx>>16)*tileHeight+(by>>16)+bufplc,bx<<16,by<<16,ylookup(y1)+x2+frameplace);
            }
        }
    }
}


void nextpage(int rowsToUpdate)
{

    _nextpage(rowsToUpdate);  /* video driver specific. */

#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif

    beforedrawrooms = 1;
    numframes++;

}



int clipinsidebox(int32_t x, int32_t y, short wallnum, int32_t walldist)
{
    walltype *wal;
    int32_t x1, y1, x2, y2, r;

    r = (walldist<<1);
    wal = getWall(wallnum);
    x1 = wal->x+walldist-x;
    y1 = wal->y+walldist-y;
    wal = getWall(constwall[wallnum]._point2);
    x2 = wal->x+walldist-x;
    y2 = wal->y+walldist-y;

    if ((x1 < 0) && (x2 < 0)) return(0);
    if ((y1 < 0) && (y2 < 0)) return(0);
    if ((x1 >= r) && (x2 >= r)) return(0);
    if ((y1 >= r) && (y2 >= r)) return(0);

    x2 -= x1;
    y2 -= y1;
    if (x2*(walldist-y1) >= y2*(walldist-x1))  /* Front */
    {
        if (x2 > 0) x2 *= (0-y1);
        else x2 *= (r-y1);
        if (y2 > 0) y2 *= (r-x1);
        else y2 *= (0-x1);
        return(x2 < y2);
    }
    if (x2 > 0) x2 *= (r-y1);
    else x2 *= (0-y1);
    if (y2 > 0) y2 *= (0-x1);
    else y2 *= (r-x1);
    return((x2 >= y2)<<1);
}

static int clipinsideboxline(int32_t x, int32_t y, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t walldist)
{
    int32_t r;

    r = (walldist<<1);

    x1 += walldist-x;
    x2 += walldist-x;
    if ((x1 < 0) && (x2 < 0)) return(0);
    if ((x1 >= r) && (x2 >= r)) return(0);

    y1 += walldist-y;
    y2 += walldist-y;
    if ((y1 < 0) && (y2 < 0)) return(0);
    if ((y1 >= r) && (y2 >= r)) return(0);

    x2 -= x1;
    y2 -= y1;
    if (x2*(walldist-y1) >= y2*(walldist-x1))  /* Front */
    {
        if (x2 > 0) x2 *= (0-y1);
        else x2 *= (r-y1);
        if (y2 > 0) y2 *= (r-x1);
        else y2 *= (0-x1);
        return(x2 < y2);
    }
    if (x2 > 0) x2 *= (r-y1);
    else x2 *= (0-y1);
    if (y2 > 0) y2 *= (0-x1);
    else y2 *= (r-x1);
    return((x2 >= y2)<<1);
}

void drawline256 (int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t  col)
{
    int32_t dx, dy, i, j, inc, plc, daend;
    uint8_t* p;
    col = palookup[0][col];

    dx = x2-x1;
    dy = y2-y1;
    if (dx >= 0)
    {
        if ((x1 >= wx2) || (x2 < wx1)) return;
        if (x1 < wx1) y1 += scale(wx1-x1,dy,dx), x1 = wx1;
        if (x2 > wx2) y2 += scale(wx2-x2,dy,dx), x2 = wx2;
    }
    else
    {
        if ((x2 >= wx2) || (x1 < wx1)) return;
        if (x2 < wx1) y2 += scale(wx1-x2,dy,dx), x2 = wx1;
        if (x1 > wx2) y1 += scale(wx2-x1,dy,dx), x1 = wx2;
    }
    if (dy >= 0)
    {
        if ((y1 >= wy2) || (y2 < wy1)) return;
        if (y1 < wy1) x1 += scale(wy1-y1,dx,dy), y1 = wy1;
        if (y2 > wy2) x2 += scale(wy2-y2,dx,dy), y2 = wy2;
    }
    else
    {
        if ((y2 >= wy2) || (y1 < wy1)) return;
        if (y2 < wy1) x2 += scale(wy1-y2,dx,dy), y2 = wy1;
        if (y1 > wy2) x1 += scale(wy2-y1,dx,dy), y1 = wy2;
    }

    if (klabs(dx) >= klabs(dy))
    {
        if (dx == 0) return;
        if (dx < 0)
        {
            i = x1;
            x1 = x2;
            x2 = i;
            i = y1;
            y1 = y2;
            y2 = i;
        }

        inc = divscale12(dy,dx);
        plc = y1+mulscale12((2047-x1)&4095,inc);
        i = ((x1+2048)>>12);
        daend = ((x2+2048)>>12);
        for(; i<daend; i++)
        {
            j = (plc>>12);
            if ((j >= startumost(i) && (j < startdmost(i))))
                drawpixel(ylookup(j)+i+frameplace,col);
            plc += inc;
        }
    }
    else
    {
        if (dy < 0)
        {
            i = x1;
            x1 = x2;
            x2 = i;
            i = y1;
            y1 = y2;
            y2 = i;
        }

        inc = divscale12(dx,dy);
        plc = x1+mulscale12((2047-y1)&4095,inc);
        i = ((y1+2048)>>12);
        daend = ((y2+2048)>>12);
        p = ylookup(i)+frameplace;
        for(; i<daend; i++)
        {
            j = (plc>>12);
            if ((i >= startumost(j)) && (i < startdmost(j)))
                drawpixel(j+p,col);
            plc += inc;
            p += ylookup(1);
        }
    }
}
/*
 FCS: Return true if the point (x,Y) is inside the sector sectnum.
 Note that a sector is closed (but can be concave) so the answer is always 0 or 1.

 Algorithm: This is an optimized raycasting inside polygon test:
 http://en.wikipedia.org/wiki/Point_in_polygon#Ray_casting_algorithm
 The goal is to follow an ***horizontal*** ray passing by (x,y) and count how many
 wall are being crossed.
 If it is an odd number of time: (x,y) is inside the sector.
 If it is an even number of time:(x,y) is outside the sector.
 */

int inside(int32_t x, int32_t y, short sectnum)
{
    const walltype *wal;
    const walltype *wallp2;

    int32_t i, x1, y1, x2, y2;
    uint32_t  wallCrossed;

    //Quick check if the sector ID is valid.
    if ((sectnum < 0) || (sectnum >= numsectors)) return(-1);

    wallCrossed = 0;
    wal = getWall(constsector[sectnum].wallptr);
    int wallIdx = constsector[sectnum].wallptr;
    i = constsector[sectnum].wallnum;
    do
    {
        y1 = wal->y-y;
        wallp2 = getWall(constwall[wallIdx]._point2);
        y2 = wallp2->y-y;

        // Compare the sign of y1 and y2.
        // If (y1^y2) < 0 : y1 and y2 have different sign bit:  y is between wal->y and wall[wal->point2].y.
        // The goal is to not take into consideration any wall that is totally above or totally under the point [x,y].
        if ((y1^y2) < 0)
        {
            x1 = wal->x-x;
            x2 = wallp2->x-x;

            //If (x1^x2) >= 0 x1 and x2 have identic sign bit: x is on the left or the right of both wal->x and wall[wal->point2].x.
            if ((x1^x2) >= 0)
            {
                // If (x,y) is totally on the left or on the right, just count x1 (which indicate if we are on
                // on the left or on the right.
                wallCrossed ^= x1;
            }
            else
            {
                // This is the most complicated case: X is between x1 and x2, we need a fine grained test.
                // We need to know exactly if it is on the left or on the right in order to know if the ray
                // is crossing the wall or not,
                // The sign of the Cross-Product can answer this case :) !
                wallCrossed ^= (x1*y2-x2*y1)^y2;
            }
        }

        wallIdx++;
        wal = getWall(wallIdx);
        i--;

    } while (i);

    //Just return the sign. If the position vector cut the sector walls an odd number of time
    //it is inside. Otherwise (even) it is outside.
    return(wallCrossed>>31);
}


int getangle(int32_t xvect, int32_t yvect)
{
    if ((xvect|yvect) == 0)
        return(0);
    if (xvect == 0)
        return(512+((yvect<0)<<10));
    if (yvect == 0)
        return(((xvect<0)<<10));
    if (xvect == yvect)
        return(256+((xvect<0)<<10));
    if (xvect == -yvect)
        return(768+((xvect>0)<<10));
    if (klabs(xvect) > klabs(yvect))
        return(((radarang[640+scale(160,yvect,xvect)]>>6)+((xvect<0)<<10))&2047);

    return(((radarang[640-scale(160,xvect,yvect)]>>6)+512+((yvect<0)<<10))&2047);
}


int ksqrt(int32_t num)
{
    return(nsqrtasm(num));
}





static void drawmaskwall(short damaskwallcnt)
{
    int32_t i, j, k, x, z, sectnum, z1, z2, lx, rx;
    const sectortype *sec, *nsec;
    walltype *wal;

    //Retrive pvWall ID.
    z = maskwall[damaskwallcnt];

    //Retrive world wall ID.
#if PVWALL_HAS_WORLDWALLID
    wal = getWall(pvWalls[z].worldWallId);
#else
    wal = getWall(pvWalls_worldWallId[z]);
#endif
    //Retrive sector ID
    sectnum = get_pvWalls_sectorId(z);

    //Retrive sector.
    sec = getSector(sectnum);

    //Retrive next sector.
#if PVWALL_HAS_WORLDWALLID
    nsec = getSector(constwall[pvWalls[z].worldWallId].nextsector);
#else
    nsec = getSector(constwall[pvWalls_worldWallId[z]].nextsector);
#endif
    z1 = max(nsec->ceilingz,sec->ceilingz);
    z2 = min(nsec->floorz,sec->floorz);

    wallmost(uwall,z,sectnum,(uint8_t )0);
#if PVWALL_HAS_WORLDWALLID
    wallmost(uplc,z,(int32_t)constwall[pvWalls[z].worldWallId].nextsector,(uint8_t )0);
#else
    wallmost(uplc,z,(int32_t)constwall[pvWalls_worldWallId[z]].nextsector,(uint8_t )0);
#endif
    for(x=pvWalls[z].screenSpaceCoo[0].vec_col; x<=pvWalls[z].screenSpaceCoo[1].vec_col; x++)
        if (uplc[x] > uwall[x])
            uwall[x] = uplc[x];

    wallmost(dwall,z,sectnum,(uint8_t )1);
#if PVWALL_HAS_WORLDWALLID
    wallmost(dplc,z, constwall[pvWalls[z].worldWallId].nextsector,(uint8_t )1);
#else
    wallmost(dplc,z, constwall[pvWalls_worldWallId[z]].nextsector,(uint8_t )1);
#endif
    for(x=pvWalls[z].screenSpaceCoo[0].vec_col; x<=pvWalls[z].screenSpaceCoo[1].vec_col; x++)
        if (dplc[x] < dwall[x])
            dwall[x] = dplc[x];

#if PVWALL_HAS_WORLDWALLID
    prepwall(z,pvWalls[z].worldWallId);
    globalorientation = (int32_t)getWall_cstat(pvWalls[z].worldWallId);
    globalpicnum = getWall_overpicnum(pvWalls[z].worldWallId);
    if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES)
        globalpicnum = 0;
    globalxpanning = (int32_t)getWall_xpanning(pvWalls[z].worldWallId);
    globalypanning = (int32_t)getWall_ypanning(pvWalls[z].worldWallId);

    if (tiles[globalpicnum].animFlags&192)
        globalpicnum += animateoffs(globalpicnum);

    globalshade = getWall_shade(pvWalls[z].worldWallId);

#else
    prepwall(z,pvWalls_worldWallId[z]);
    globalorientation = (int32_t)wal->_cstat;//getWall_cstat(pvWalls_worldWallId[z]);
    globalpicnum = wal->_overpicnum;//getWall_overpicnum(pvWalls_worldWallId[z]);
    if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES)
        globalpicnum = 0;
    globalxpanning = (int32_t)wal->_xpanning; //getWall_xpanning(pvWalls_worldWallId[z]);
    globalypanning = (int32_t)wal->_ypanning; //getWall_ypanning(pvWalls_worldWallId[z]);

    if (tiles[globalpicnum].animFlags&192)
        globalpicnum += animateoffs(globalpicnum);

    globalshade = getWall_shade(pvWalls_worldWallId[z]);

#endif
    globvis = globalvisibility;
    if (constsector[sectnum].visibility != 0)
        globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[sectnum].visibility+16)));
#if PVWALL_HAS_WORLDWALLID
    globalpal = (int32_t)getWall_pal(pvWalls[z].worldWallId);
#else
    globalpal = (int32_t)wal->_pal;//getWall_pal(pvWalls_worldWallId[z]);
#endif
    globalshiftval = (picsiz[globalpicnum]>>4);
    if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height)
        globalshiftval++;

    globalshiftval = 32-globalshiftval;
#if PVWALL_HAS_WORLDWALLID
    globalyscale = (constwall[pvWalls[z].worldWallId]._yrepeat<<(globalshiftval-19));
#else
    globalyscale = (constwall[pvWalls_worldWallId[z]]._yrepeat<<(globalshiftval-19));
#endif
    if ((globalorientation&4) == 0)
        globalzd = (((globalposz-z1)*globalyscale)<<8);
    else
        globalzd = (((globalposz-z2)*globalyscale)<<8);
    globalzd += (globalypanning<<24);
    if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;

    for(i=smostwallcnt-1; i>=0; i--)
    {
        j = smostwall[i];
        if ((pvWalls[j].screenSpaceCoo[0].vec_col > pvWalls[z].screenSpaceCoo[1].vec_col) || (pvWalls[j].screenSpaceCoo[1].vec_col < pvWalls[z].screenSpaceCoo[0].vec_col)) continue;
        if (wallfront(j,z)) continue;

        lx = max(pvWalls[j].screenSpaceCoo[0].vec_col,pvWalls[z].screenSpaceCoo[0].vec_col);
        rx = min(pvWalls[j].screenSpaceCoo[1].vec_col,pvWalls[z].screenSpaceCoo[1].vec_col);

        switch(get_smostwalltype(i))
        {
        case 0:
            if (lx <= rx)
            {
                if ((lx == pvWalls[z].screenSpaceCoo[0].vec_col) && (rx == pvWalls[z].screenSpaceCoo[1].vec_col)) return;
                clearbufbyte(&dwall[lx],(rx-lx+1)*sizeof(dwall[0]),0L);
            }
            break;
        case 1:
            k = smoststart[i] - pvWalls[j].screenSpaceCoo[0].vec_col;
            for(x=lx; x<=rx; x++)
                if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
            break;
        case 2:
            k = smoststart[i] - pvWalls[j].screenSpaceCoo[0].vec_col;
            for(x=lx; x<=rx; x++)
                if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
            break;
        }
    }

    /* maskwall */
    if ((searchit >= 1) && (searchx >= pvWalls[z].screenSpaceCoo[0].vec_col) && (searchx <= pvWalls[z].screenSpaceCoo[1].vec_col))
        if ((searchy >= uwall[searchx]) && (searchy <= dwall[searchx]))
        {
            searchsector = sectnum;
#if PVWALL_HAS_WORLDWALLID
            searchwall = pvWalls[z].worldWallId;
#else
            searchwall = pvWalls_worldWallId[z];
#endif
            searchstat = 4;
            searchit = 1;
        }

    if ((globalorientation&128) == 0)
        maskwallscan(pvWalls[z].screenSpaceCoo[0].vec_col,pvWalls[z].screenSpaceCoo[1].vec_col,uwall,dwall,swall,lwall);
    else
    {
        if (globalorientation&128)
        {
            if (globalorientation&512)
                settrans(TRANS_REVERSE);
            else
                settrans(TRANS_NORMAL);
        }
        transmaskwallscan(pvWalls[z].screenSpaceCoo[0].vec_col,pvWalls[z].screenSpaceCoo[1].vec_col);
    }
}





static void ceilspritehline (int32_t x2, int32_t y)
{
    int32_t x1, v, bx, by;

    /*
     * x = x1 + (x2-x1)t + (y1-y2)u  �  x = 160v
     * y = y1 + (y2-y1)t + (x2-x1)u  �  y = (scrx-160)v
     * z = z1 = z2                   �  z = posz + (scry-horiz)v
     */

    x1 = lastx[y];
    if (x2 < x1) return;

    v = mulscale20(globalzd,horizlookup[y-globalhoriz+horizycent]);
    bx = mulscale14(globalx2*x1+globalx1,v) + globalxpanning;
    by = mulscale14(globaly2*x1+globaly1,v) + globalypanning;
    draws.asm1 = mulscale14(globalx2,v);
    draws.asm2 = mulscale14(globaly2,v);
    draws.asm3 = (intptr_t) palookup[globalpal] + (getpalookup((int32_t)mulscale28(klabs(v),globvis),globalshade)<<8);

    if ((globalorientation&2) == 0)
    {
        mhline(globalbufplc,bx,(x2-x1)<<16,by,ylookup(y)+x1+frameoffset);
    }
    else
    {
        thline(globalbufplc,bx,(x2-x1)<<16,by,ylookup(y)+x1+frameoffset);
        transarea += (x2-x1);
    }
}


static void ceilspritescan (int32_t x1, int32_t x2)
{
    int32_t x, y1, y2, twall, bwall;

    y1 = uwall[x1];
    y2 = y1;
    for(x=x1; x<=x2; x++)
    {
        twall = uwall[x]-1;
        bwall = dwall[x];
        if (twall < bwall-1)
        {

            if (twall >= y2)
            {
                while (y1 < y2-1) ceilspritehline(x-1,++y1);
                y1 = twall;
            }
            else
            {
                while (y1 < twall) ceilspritehline(x-1,++y1);
                while (y1 > twall) lastx[y1--] = x;
            }
            while (y2 > bwall) ceilspritehline(x-1,--y2);
            while (y2 < bwall) lastx[y2++] = x;
        }
        else
        {
            while (y1 < y2-1) ceilspritehline(x-1,++y1);
            if (x == x2) break;
            y1 = uwall[x+1];
            y2 = y1;
        }
    }
    while (y1 < y2-1) ceilspritehline(x2,++y1);
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
}

static void drawsprite (int32_t snum)
{
    uint8_t flat[MAX_RAM_FLAT_SIZE]; // borrowing from Doom's terminology
    tspritetype *tspr;
    const sectortype *sec;
    int32_t startum, startdm, sectnum, xb, yp, cstat;
    int32_t siz, xsiz, ysiz, xoff, yoff;
    dimensions_t spriteDim;
    int32_t x1, y1, x2, y2, lx, rx, dalx2, darx2, i, j, k, x, linum, linuminc;
    int32_t yinc, z, z1, z2, xp1, yp1, xp2, yp2;
    int32_t xv, yv, top, topinc, bot, botinc, hplc, hinc;
    int32_t cosang, sinang, dax, day, lpoint, lmax, rpoint, rmax, dax1, dax2, y;
    int32_t npoints, npoints2, zz, t, zsgn, zzsgn;
    short tilenum, spritenum;
    int32_t rxi[8], ryi[8], rzi[8], rxi2[8], ryi2[8], rzi2[8];

    uint8_t  swapped, daclip;
#if USE_TSPRITEPTR
    tspr = tspriteptr[snum];
#else
    tspr = &tsprite[tspriteIdx[snum]];
#endif // USE_TSPRITEPTR

#if USE_SPRITESX
    xb = spritesx[snum];
#else
    xb = tspr->x;
#endif
    yp = spritesy[snum];
    tilenum = tspr->picnum;
    spritenum = tspr->owner;
    cstat = tspr->cstat;

    if ((cstat&48) != 48)
    {
        if (tiles[tilenum].animFlags&192)
        {
            tilenum += animateoffs(tilenum);
        }

        if ((tiles[tilenum].dim.width <= 0) || (tiles[tilenum].dim.height <= 0) || (spritenum < 0))
            return;
    }
    if ((tspr->xrepeat <= 0) || (tspr->yrepeat <= 0)) return;

    sectnum = tspr->sectnum;
    sec = getSector(sectnum);
    globalpal = tspr->pal;
    // FIX_00088: crash on maps using a bad palette index (like the end of roch3.map)
    if (!palookup[globalpal])
        globalpal = 0; // seem to crash when globalpal > 25
    globalshade = tspr->shade;
    if (cstat&2)
    {

        if (cstat&512)
            settrans(TRANS_REVERSE);
        else
            settrans(TRANS_NORMAL);
    }
#if TSPRITE_XOFFSET
    xoff = (int32_t)((int8_t )((tiles[tilenum].animFlags>>8)&255))+((int32_t)tspr->xoffset);
#else
    if (tspr->owner >= 0)
        xoff = (int32_t)getSprite_xoffset(tspr->owner);
    else
        xoff = 0;
    xoff = (int32_t)((int8_t )((tiles[tilenum].animFlags>>8)&255))+(xoff);
#endif
#if TSPRITE_YOFFSET
    yoff = (int32_t)((int8_t )((tiles[tilenum].animFlags>>16)&255))+((int32_t)tspr->yoffset);
#else
    if (tspr->owner >= 0)
        yoff = (int32_t)getSprite_yoffset(tspr->owner);
    else
        yoff = 0;
    yoff = (int32_t)((int8_t )((tiles[tilenum].animFlags>>16)&255))+(yoff);
#endif
    if ((cstat&48) == 0)
    {
        if (yp <= (4<<8))
            return;

        siz = divscale19(xdimenscale,yp);

        xv = mulscale16(((int32_t)tspr->xrepeat)<<16,xyaspect);

        spriteDim.width = tiles[tilenum].dim.width;
        spriteDim.height = tiles[tilenum].dim.height;

        xsiz = mulscale30(siz,xv * spriteDim.width);
        ysiz = mulscale14(siz,tspr->yrepeat * spriteDim.height);

        if (((tiles[tilenum].dim.width>>11) >= xsiz) || (spriteDim.height >= (ysiz>>1)))
            return;  /* Watch out for divscale overflow */

        x1 = xb-(xsiz>>1);
        if (spriteDim.width & 1)
            x1 += mulscale31(siz,xv);  /* Odd xspans */
        i = mulscale30(siz,xv*xoff);
        if ((cstat&4) == 0)
            x1 -= i;
        else
            x1 += i;

        y1 = mulscale16(tspr->z-globalposz,siz);
        y1 -= mulscale14(siz,tspr->yrepeat*yoff);
        y1 += (globalhoriz<<8)-ysiz;
        if (cstat&128)
        {
            y1 += (ysiz>>1);
            if (spriteDim.height&1) y1 += mulscale15(siz,tspr->yrepeat);  /* Odd yspans */
        }

        x2 = x1+xsiz-1;
        y2 = y1+ysiz-1;
        if ((y1|255) >= (y2|255)) return;

        lx = (x1>>8)+1;
        if (lx < 0) lx = 0;
        rx = (x2>>8);
        if (rx >= xdimen) rx = xdimen-1;
        if (lx > rx) return;

        yinc = divscale32(spriteDim.height,ysiz);

        if ((sec->ceilingstat&3) == 0)
            startum = globalhoriz+mulscale24(siz,sec->ceilingz-globalposz)-1;
        else
            startum = 0;

        if ((sec->floorstat&3) == 0)
            startdm = globalhoriz+mulscale24(siz,sec->floorz-globalposz)+1;
        else
            startdm = 0x7fffffff;

        if ((y1>>8) > startum) startum = (y1>>8);
        if ((y2>>8) < startdm) startdm = (y2>>8);

        if (startum < -32768) startum = -32768;
        if (startdm > 32767) startdm = 32767;
        if (startum >= startdm) return;

        if ((cstat&4) == 0)
        {
            linuminc = divscale24(spriteDim.width,xsiz);
            linum = mulscale8((lx<<8)-x1,linuminc);
        }
        else
        {
            linuminc = -divscale24(spriteDim.width,xsiz);
            linum = mulscale8((lx<<8)-x2,linuminc);
        }
        if ((cstat&8) > 0)
        {
            yinc = -yinc;
            i = y1;
            y1 = y2;
            y2 = i;
        }

        for(x=lx; x<=rx; x++)
        {
            uwall[x] = max(startumost(x+windowx1)-windowy1,(short)startum);
            dwall[x] = min(startdmost(x+windowx1)-windowy1,(short)startdm);
        }
        daclip = 0;
        for(i=smostwallcnt-1; i>=0; i--)
        {
            if (get_smostwalltype(i)&daclip)
                continue;

            j = smostwall[i];
            if ((pvWalls[j].screenSpaceCoo[0].vec_col > rx) || (pvWalls[j].screenSpaceCoo[1].vec_col < lx))
                continue;

            if ((yp <= getVecDist(&pvWalls[j].screenSpaceCoo[0])) && (yp <= getVecDist(&pvWalls[j].screenSpaceCoo[1])))
                continue;
#if PVWALL_HAS_WORLDWALLID
            if (spritewallfront(tspr,pvWalls[j].worldWallId) && ((yp <= pvWalls[j].screenSpaceCoo[0].vec_dist) || (yp <= pvWalls[j].screenSpaceCoo[1].vec_dist)))
                continue;
#else
            if (spritewallfront(tspr,pvWalls_worldWallId[j]) && ((yp <= getVecDist(&pvWalls[j].screenSpaceCoo[0])) || (yp <= getVecDist(&pvWalls[j].screenSpaceCoo[1]))))
                continue;
#endif
            dalx2 = max(pvWalls[j].screenSpaceCoo[0].vec_col,lx);
            darx2 = min(pvWalls[j].screenSpaceCoo[1].vec_col,rx);

            switch(get_smostwalltype(i))
            {
            case 0:
                if (dalx2 <= darx2)
                {
                    if ((dalx2 == lx) && (darx2 == rx))
                        return;
                    clearbufbyte(&dwall[dalx2],(darx2-dalx2+1)*sizeof(dwall[0]),0L);
                }
                break;
            case 1:
                k = smoststart[i] - pvWalls[j].screenSpaceCoo[0].vec_col;
                for(x=dalx2; x<=darx2; x++)
                    if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
                if ((dalx2 == lx) && (darx2 == rx)) daclip |= 1;
                break;
            case 2:
                k = smoststart[i] - pvWalls[j].screenSpaceCoo[0].vec_col;
                for(x=dalx2; x<=darx2; x++)
                    if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
                if ((dalx2 == lx) && (darx2 == rx)) daclip |= 2;
                break;
            }
        }

        if (uwall[rx] >= dwall[rx])
        {
            for(x=lx; x<rx; x++)
                if (uwall[x] < dwall[x]) break;
            if (x == rx) return;
        }

        /* sprite */
        if ((searchit >= 1) && (searchx >= lx) && (searchx <= rx))
            if ((searchy >= uwall[searchx]) && (searchy < dwall[searchx]))
            {
                searchsector = sectnum;
                searchwall = spritenum;
                searchstat = 3;
                searchit = 1;
            }

        z2 = tspr->z - ((yoff*tspr->yrepeat)<<2);
        if (cstat&128)
        {
            z2 += ((spriteDim.height*tspr->yrepeat)<<1);
            if (spriteDim.height&1) z2 += (tspr->yrepeat<<1);        /* Odd yspans */
        }
        z1 = z2 - ((spriteDim.height*tspr->yrepeat)<<2);

        globalorientation = 0;
        globalpicnum = tilenum;
        if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES) globalpicnum = 0;
        globalxpanning = 0L;
        globalypanning = 0L;
        globvis = globalvisibility;
        if (constsector[sectnum].visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[sectnum].visibility+16)));
        globalshiftval = (picsiz[globalpicnum]>>4);
        if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height)
            globalshiftval++;

        globalshiftval = 32-globalshiftval;
        globalyscale = divscale(512,tspr->yrepeat,globalshiftval-19);
        globalzd = (((globalposz-z1)*globalyscale)<<8);
        if ((cstat&8) > 0)
        {
            globalyscale = -globalyscale;
            globalzd = (((globalposz-z2)*globalyscale)<<8);
        }
        qinterpolatedown16_lwall(&lwall[lx],rx-lx+1,linum,linuminc);
        clearbuf_swall(&swall[lx],rx-lx+1,mulscale19(yp,xdimscale));
        if ((cstat&2) == 0)
            maskwallscan(lx,rx,uwall,dwall,swall,lwall);
        else
            transmaskwallscan(lx,rx);
    }
    else if ((cstat&48) == 16)
    {
        if ((cstat&4) > 0) xoff = -xoff;
        if ((cstat&8) > 0) yoff = -yoff;

        spriteDim.width = tiles[tilenum].dim.width;
        spriteDim.height = tiles[tilenum].dim.height;

        xv = tspr->xrepeat*sintable[(tspr->ang+2560+1536)&2047];
        yv = tspr->xrepeat*sintable[(tspr->ang+2048+1536)&2047];
        i = (spriteDim.width >>1)+xoff;
        x1 = tspr->x-globalposx-mulscale16(xv,i);
        x2 = x1+mulscale16(xv,spriteDim.width );
        y1 = tspr->y-globalposy-mulscale16(yv,i);
        y2 = y1+mulscale16(yv,spriteDim.width );

        yp1 = dmulscale6(x1,cosviewingrangeglobalang,y1,sinviewingrangeglobalang);
        yp2 = dmulscale6(x2,cosviewingrangeglobalang,y2,sinviewingrangeglobalang);
        if ((yp1 <= 0) && (yp2 <= 0)) return;
        xp1 = dmulscale6(y1,cosglobalang,-x1,singlobalang);
        xp2 = dmulscale6(y2,cosglobalang,-x2,singlobalang);

        x1 += globalposx;
        y1 += globalposy;
        x2 += globalposx;
        y2 += globalposy;

        swapped = 0;
        if (dmulscale32(xp1,yp2,-xp2,yp1) >= 0)  /* If wall's NOT facing you */
        {
            if ((cstat&64) != 0) return;
            i = xp1, xp1 = xp2, xp2 = i;
            i = yp1, yp1 = yp2, yp2 = i;
            i = x1, x1 = x2, x2 = i;
            i = y1, y1 = y2, y2 = i;
            swapped = 1;
        }

        if (xp1 >= -yp1)
        {
            if (xp1 > yp1) return;

            if (yp1 == 0) return;
            pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col = USATCOL(halfxdimen + scale(xp1,halfxdimen,yp1));
            if (xp1 >= 0) pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col++;   /* Fix for SIGNED divide */
            if (pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col >= xdimen) pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col = xdimen-1;
            setVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[0], yp1);
        }
        else
        {
            if (xp2 < -yp2) return;
            pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col = 0;
            i = yp1-yp2+xp1-xp2;
            if (i == 0) return;
            setVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[0], yp1 + scale(yp2-yp1,xp1+yp1,i));
        }
        if (xp2 <= yp2)
        {
            if (xp2 < -yp2) return;

            if (yp2 == 0) return;
            pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col = USATCOL(halfxdimen + scale(xp2,halfxdimen,yp2) - 1);
            if (xp2 >= 0) pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col++;   /* Fix for SIGNED divide */
            if (pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col >= xdimen) pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col = xdimen-1;
            setVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[1], yp2);
        }
        else
        {
            if (xp1 > yp1) return;

            pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col = xdimen-1;
            i = xp2-xp1+yp1-yp2;
            if (i == 0) return;
            setVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[1], yp1 + scale(yp2-yp1,yp1-xp1,i));
        }

        if ((getVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[0]) < 256) || (getVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[1]) < 256) || (pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col > pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col))
            return;

        topinc = -mulscale10(yp1,spriteDim.width);
        top = (((mulscale10(xp1,xdimen) - mulscale9(pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col-halfxdimen,yp1))*spriteDim.width)>>3);
        botinc = ((yp2-yp1)>>8);
        bot = mulscale11(xp1-xp2,xdimen) + mulscale2(pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col-halfxdimen,botinc);

        j = pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col+3;
        z = mulscale20(top,krecipasm(bot));
        lwall[pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col] = (z>>8);
        for(x=pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col+4; x<=j; x+=4)
        {
            top += topinc;
            bot += botinc;
            zz = z;
            z = mulscale20(top,krecipasm(bot));
            lwall[x] = (z>>8);
            i = ((z+zz)>>1);
            lwall[x-2] = (i>>8);
            lwall[x-3] = ((i+zz)>>9);
            lwall[x-1] = ((i+z)>>9);
        }

        if (lwall[pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col] < 0) lwall[pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col] = 0;
        if (lwall[pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col] >= spriteDim.width) lwall[pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col] = spriteDim.width-1;

        if ((swapped^((cstat&4)>0)) > 0)
        {
            j = spriteDim.width-1;
            for(x=pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col; x<=pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col; x++)
                lwall[x] = j-lwall[x];
        }

        pvWalls[MAXWALLSB-1].cameraSpaceCoo[0][VEC_X] = xp1 ;
        pvWalls[MAXWALLSB-1].cameraSpaceCoo[0][VEC_Y] = yp1 ;
        pvWalls[MAXWALLSB-1].cameraSpaceCoo[1][VEC_X] = xp2 ;
        pvWalls[MAXWALLSB-1].cameraSpaceCoo[1][VEC_Y] = yp2 ;


        hplc = divscale19(xdimenscale,getVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[0]));
        hinc = divscale19(xdimenscale,getVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[1]));
        hinc = (hinc-hplc)/(pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col-pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col+1);

        z2 = tspr->z - ((yoff*tspr->yrepeat)<<2);
        if (cstat&128)
        {
            z2 += ((spriteDim.height*tspr->yrepeat)<<1);
            if (spriteDim.height&1) z2 += (tspr->yrepeat<<1);        /* Odd yspans */
        }
        z1 = z2 - ((spriteDim.height*tspr->yrepeat)<<2);

        globalorientation = 0;
        globalpicnum = tilenum;
        if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES) globalpicnum = 0;
        globalxpanning = 0L;
        globalypanning = 0L;
        globvis = globalvisibility;
        if (constsector[sectnum].visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[sectnum].visibility+16)));
        globalshiftval = (picsiz[globalpicnum]>>4);
        if (pow2long[globalshiftval] != tiles[globalpicnum].dim.height) globalshiftval++;
        globalshiftval = 32-globalshiftval;
        globalyscale = divscale(512,tspr->yrepeat,globalshiftval-19);
        globalzd = (((globalposz-z1)*globalyscale)<<8);
        if ((cstat&8) > 0)
        {
            globalyscale = -globalyscale;
            globalzd = (((globalposz-z2)*globalyscale)<<8);
        }

        if (((sec->ceilingstat&1) == 0) && (z1 < sec->ceilingz))
            z1 = sec->ceilingz;
        if (((sec->floorstat&1) == 0) && (z2 > sec->floorz))
            z2 = sec->floorz;

        owallmost(uwall,(int32_t)(MAXWALLSB-1),z1-globalposz);
        owallmost(dwall,(int32_t)(MAXWALLSB-1),z2-globalposz);
        for(i=pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col; i<=pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col; i++)
        {
            swall[i] = (krecipasm(hplc)<<2); // check this value. Is
            hplc += hinc;
        }

        for(i=smostwallcnt-1; i>=0; i--)
        {
            j = smostwall[i];

            if ((pvWalls[j].screenSpaceCoo[0].vec_col > pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col) || (pvWalls[j].screenSpaceCoo[1].vec_col < pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col)) continue;

            dalx2 = pvWalls[j].screenSpaceCoo[0].vec_col;
            darx2 = pvWalls[j].screenSpaceCoo[1].vec_col;
            if (max(getVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[0]),getVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[1]))
                > min(getVecDist(&pvWalls[j].screenSpaceCoo[0]),getVecDist(&pvWalls[j].screenSpaceCoo[1])))
            {
                if (min(getVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[0]),getVecDist(&pvWalls[MAXWALLSB-1].screenSpaceCoo[1])) >
                    max(getVecDist(&pvWalls[j].screenSpaceCoo[0]),getVecDist(&pvWalls[j].screenSpaceCoo[1])))
                {
// FIXME: next-hack: this value might have to be changed, we have changed bitfield size.s
                    x = 0x80000000;
                }
                else
                {
#if PVWALL_HAS_WORLDWALLID
                    x = pvWalls[j].worldWallId;
#else
                    x = pvWalls_worldWallId[j];
#endif
                    const walltype *wallx = getWall(x);
                    xp1 = wallx->x;
                    yp1 = wallx->y;
                    x = constwall[x]._point2;
                    wallx = getWall(x);
                    xp2 = wallx->x;
                    yp2 = wallx->y;

                    z1 = (xp2-xp1)*(y1-yp1) - (yp2-yp1)*(x1-xp1);
                    z2 = (xp2-xp1)*(y2-yp1) - (yp2-yp1)*(x2-xp1);
                    if ((z1^z2) >= 0)
                        x = (z1+z2);
                    else
                    {
                        z1 = (x2-x1)*(yp1-y1) - (y2-y1)*(xp1-x1);
                        z2 = (x2-x1)*(yp2-y1) - (y2-y1)*(xp2-x1);

                        if ((z1^z2) >= 0)
                            x = -(z1+z2);
                        else
                        {
                            if ((xp2-xp1)*(tspr->y-yp1) == (tspr->x-xp1)*(yp2-yp1))
                            {
#if PVWALL_HAS_WORLDWALLID
                                if (constwall[pvWalls[j].worldWallId].nextsector == tspr->sectnum)
                                    x = 0x80000000;
                                else
                                    x = 0x7fffffff;
#else
// FIXME: next-hack: these values might need to be changed, as we have modified bitfield sizes.
                                if (constwall[pvWalls_worldWallId[j]].nextsector == tspr->sectnum)
                                    x = 0x80000000;
                                else
                                    x = 0x7fffffff;

#endif
                            }
                            else
                            {   /* INTERSECTION! */
                                x = (xp1-globalposx) + scale(xp2-xp1,z1,z1-z2);
                                y = (yp1-globalposy) + scale(yp2-yp1,z1,z1-z2);

                                yp1 = dmulscale14(x,cosglobalang,y,singlobalang);
                                if (yp1 > 0)
                                {
                                    xp1 = dmulscale14(y,cosglobalang,-x,singlobalang);

                                    x = halfxdimen + scale(xp1,halfxdimen,yp1);
                                    if (xp1 >= 0) x++;   /* Fix for SIGNED divide */

                                    if (z1 < 0)
                                    {
                                        if (dalx2 < x) dalx2 = x;
                                    }
                                    else
                                    {
                                        if (darx2 > x) darx2 = x;
                                    }
// FIXME: next-hack: these values might need to be changed, as we have modified bitfield sizes.
                                    x = 0x80000001;
                                }
                                else
                                    x = 0x7fffffff;
                            }
                        }
                    }
                }
                if (x < 0)
                {
                    if (dalx2 < pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col) dalx2 = pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col;
                    if (darx2 > pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col) darx2 = pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col;
                    switch(get_smostwalltype(i))
                    {
                    case 0:
                        if (dalx2 <= darx2)
                        {
                            if ((dalx2 == pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col) && (darx2 == pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col)) return;
                            clearbufbyte(&dwall[dalx2],(darx2-dalx2+1)*sizeof(dwall[0]),0L);
                        }
                        break;
                    case 1:
                        k = smoststart[i] - pvWalls[j].screenSpaceCoo[0].vec_col;
                        for(x=dalx2; x<=darx2; x++)
                            if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
                        break;
                    case 2:
                        k = smoststart[i] - pvWalls[j].screenSpaceCoo[0].vec_col;
                        for(x=dalx2; x<=darx2; x++)
                            if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
                        break;
                    }
                }
            }
        }

        /* sprite */
        if ((searchit >= 1) && (searchx >= pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col) && (searchx <= pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col))
            if ((searchy >= uwall[searchx]) && (searchy <= dwall[searchx]))
            {
                searchsector = sectnum;
                searchwall = spritenum;
                searchstat = 3;
                searchit = 1;
            }

        if ((cstat&2) == 0)
            maskwallscan(pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col,pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col,uwall,dwall,swall,lwall);
        else
            transmaskwallscan(pvWalls[MAXWALLSB-1].screenSpaceCoo[0].vec_col,pvWalls[MAXWALLSB-1].screenSpaceCoo[1].vec_col);
    }
    else if ((cstat&48) == 32)
    {
        if ((cstat&64) != 0)
            if ((globalposz > tspr->z) == ((cstat&8)==0))
                return;

        if ((cstat&4) > 0) xoff = -xoff;
        if ((cstat&8) > 0) yoff = -yoff;
        spriteDim.width = tiles[tilenum].dim.width;
        spriteDim.height = tiles[tilenum].dim.height;

        /* Rotate center point */
        dax = tspr->x-globalposx;
        day = tspr->y-globalposy;
        rzi[0] = dmulscale10(cosglobalang,dax,singlobalang,day);
        rxi[0] = dmulscale10(cosglobalang,day,-singlobalang,dax);

        /* Get top-left corner */
        i = ((tspr->ang+2048-globalang)&2047);
        cosang = sintable[(i+512)&2047];
        sinang = sintable[i];
        dax = ((spriteDim.width>>1)+xoff)*tspr->xrepeat;
        day = ((spriteDim.height>>1)+yoff)*tspr->yrepeat;
        rzi[0] += dmulscale12(sinang,dax,cosang,day);
        rxi[0] += dmulscale12(sinang,day,-cosang,dax);

        /* Get other 3 corners */
        dax = spriteDim.width*tspr->xrepeat;
        day = spriteDim.height*tspr->yrepeat;
        rzi[1] = rzi[0]-mulscale12(sinang,dax);
        rxi[1] = rxi[0]+mulscale12(cosang,dax);
        dax = -mulscale12(cosang,day);
        day = -mulscale12(sinang,day);
        rzi[2] = rzi[1]+dax;
        rxi[2] = rxi[1]+day;
        rzi[3] = rzi[0]+dax;
        rxi[3] = rxi[0]+day;

        /* Put all points on same z */
        ryi[0] = scale((tspr->z-globalposz),yxaspect,320<<8);
        if (ryi[0] == 0) return;
        ryi[1] = ryi[2] = ryi[3] = ryi[0];

        if ((cstat&4) == 0)
        {
            z = 0;
            z1 = 1;
            z2 = 3;
        }
        else
        {
            z = 1;
            z1 = 0;
            z2 = 2;
        }

        dax = rzi[z1]-rzi[z];
        day = rxi[z1]-rxi[z];
        bot = dmulscale8(dax,dax,day,day);
        if (((klabs(dax)>>13) >= bot) || ((klabs(day)>>13) >= bot)) return;
        globalx1 = divscale18(dax,bot);
        globalx2 = divscale18(day,bot);

        dax = rzi[z2]-rzi[z];
        day = rxi[z2]-rxi[z];
        bot = dmulscale8(dax,dax,day,day);
        if (((klabs(dax)>>13) >= bot) || ((klabs(day)>>13) >= bot)) return;
        globaly1 = divscale18(dax,bot);
        globaly2 = divscale18(day,bot);

        /* Calculate globals for hline texture mapping function */
        globalxpanning = (rxi[z]<<12);
        globalypanning = (rzi[z]<<12);
        globalzd = (ryi[z]<<12);

        rzi[0] = mulscale16(rzi[0],viewingrange);
        rzi[1] = mulscale16(rzi[1],viewingrange);
        rzi[2] = mulscale16(rzi[2],viewingrange);
        rzi[3] = mulscale16(rzi[3],viewingrange);

        if (ryi[0] < 0)   /* If ceilsprite is above you, reverse order of points */
        {
            i = rxi[1];
            rxi[1] = rxi[3];
            rxi[3] = i;
            i = rzi[1];
            rzi[1] = rzi[3];
            rzi[3] = i;
        }


        /* Clip polygon in 3-space */
        npoints = 4;

        /* Clip edge 1 */
        npoints2 = 0;
        zzsgn = rxi[0]+rzi[0];
        for(z=0; z<npoints; z++)
        {
            zz = z+1;
            if (zz == npoints) zz = 0;
            zsgn = zzsgn;
            zzsgn = rxi[zz]+rzi[zz];
            if (zsgn >= 0)
            {
                rxi2[npoints2] = rxi[z];
                ryi2[npoints2] = ryi[z];
                rzi2[npoints2] = rzi[z];
                npoints2++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                t = divscale30(zsgn,zsgn-zzsgn);
                rxi2[npoints2] = rxi[z] + mulscale30(t,rxi[zz]-rxi[z]);
                ryi2[npoints2] = ryi[z] + mulscale30(t,ryi[zz]-ryi[z]);
                rzi2[npoints2] = rzi[z] + mulscale30(t,rzi[zz]-rzi[z]);
                npoints2++;
            }
        }
        if (npoints2 <= 2) return;

        /* Clip edge 2 */
        npoints = 0;
        zzsgn = rxi2[0]-rzi2[0];
        for(z=0; z<npoints2; z++)
        {
            zz = z+1;
            if (zz == npoints2) zz = 0;
            zsgn = zzsgn;
            zzsgn = rxi2[zz]-rzi2[zz];
            if (zsgn <= 0)
            {
                rxi[npoints] = rxi2[z];
                ryi[npoints] = ryi2[z];
                rzi[npoints] = rzi2[z];
                npoints++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                t = divscale30(zsgn,zsgn-zzsgn);
                rxi[npoints] = rxi2[z] + mulscale30(t,rxi2[zz]-rxi2[z]);
                ryi[npoints] = ryi2[z] + mulscale30(t,ryi2[zz]-ryi2[z]);
                rzi[npoints] = rzi2[z] + mulscale30(t,rzi2[zz]-rzi2[z]);
                npoints++;
            }
        }
        if (npoints <= 2) return;

        /* Clip edge 3 */
        npoints2 = 0;
        zzsgn = ryi[0]*halfxdimen + (rzi[0]*(globalhoriz-0));
        for(z=0; z<npoints; z++)
        {
            zz = z+1;
            if (zz == npoints) zz = 0;
            zsgn = zzsgn;
            zzsgn = ryi[zz]*halfxdimen + (rzi[zz]*(globalhoriz-0));
            if (zsgn >= 0)
            {
                rxi2[npoints2] = rxi[z];
                ryi2[npoints2] = ryi[z];
                rzi2[npoints2] = rzi[z];
                npoints2++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                t = divscale30(zsgn,zsgn-zzsgn);
                rxi2[npoints2] = rxi[z] + mulscale30(t,rxi[zz]-rxi[z]);
                ryi2[npoints2] = ryi[z] + mulscale30(t,ryi[zz]-ryi[z]);
                rzi2[npoints2] = rzi[z] + mulscale30(t,rzi[zz]-rzi[z]);
                npoints2++;
            }
        }
        if (npoints2 <= 2) return;

        /* Clip edge 4 */
        npoints = 0;
        zzsgn = ryi2[0]*halfxdimen + (rzi2[0]*(globalhoriz-ydimen));
        for(z=0; z<npoints2; z++)
        {
            zz = z+1;
            if (zz == npoints2) zz = 0;
            zsgn = zzsgn;
            zzsgn = ryi2[zz]*halfxdimen + (rzi2[zz]*(globalhoriz-ydimen));
            if (zsgn <= 0)
            {
                rxi[npoints] = rxi2[z];
                ryi[npoints] = ryi2[z];
                rzi[npoints] = rzi2[z];
                npoints++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                t = divscale30(zsgn,zsgn-zzsgn);
                rxi[npoints] = rxi2[z] + mulscale30(t,rxi2[zz]-rxi2[z]);
                ryi[npoints] = ryi2[z] + mulscale30(t,ryi2[zz]-ryi2[z]);
                rzi[npoints] = rzi2[z] + mulscale30(t,rzi2[zz]-rzi2[z]);
                npoints++;
            }
        }
        if (npoints <= 2) return;

        /* Project onto screen */

        lpoint = -1;
        lmax = 0x7fffffff;
        rpoint = -1;
        rmax = 0x80000000;
        int32_t xsi[8], ysi[8];
        for(z=0; z<npoints; z++)
        {
            xsi[z] = scale(rxi[z],xdimen<<15,rzi[z]) + (xdimen<<15);
            ysi[z] = scale(ryi[z],xdimen<<15,rzi[z]) + (globalhoriz<<16);
            if (xsi[z] < 0) xsi[z] = 0;
            if (xsi[z] > (xdimen<<16)) xsi[z] = (xdimen<<16);
            if (ysi[z] < ((int32_t)0<<16)) ysi[z] = ((int32_t)0<<16);
            if (ysi[z] > ((int32_t)ydimen<<16)) ysi[z] = ((int32_t)ydimen<<16);
            if (xsi[z] < lmax) lmax = xsi[z], lpoint = z;
            if (xsi[z] > rmax) rmax = xsi[z], rpoint = z;
        }

        /* Get uwall arrays */
        for(z=lpoint; z!=rpoint; z=zz)
        {
            zz = z+1;
            if (zz == npoints) zz = 0;

            dax1 = ((xsi[z]+65535)>>16);
            dax2 = ((xsi[zz]+65535)>>16);
            if (dax2 > dax1)
            {
                yinc = divscale16(ysi[zz]-ysi[z],xsi[zz]-xsi[z]);
                y = ysi[z] + mulscale16((dax1<<16)-xsi[z],yinc);
                qinterpolatedown16short((short *)(&uwall[dax1]),dax2-dax1,y,yinc);
            }
        }

        /* Get dwall arrays */
        for(; z!=lpoint; z=zz)
        {
            zz = z+1;
            if (zz == npoints) zz = 0;

            dax1 = ((xsi[zz]+65535)>>16);
            dax2 = ((xsi[z]+65535)>>16);
            if (dax2 > dax1)
            {
                yinc = divscale16(ysi[zz]-ysi[z],xsi[zz]-xsi[z]);
                y = ysi[zz] + mulscale16((dax1<<16)-xsi[zz],yinc);
                qinterpolatedown16short((short *)(&dwall[dax1]),dax2-dax1,y,yinc);
            }
        }


        lx = ((lmax+65535)>>16);
        rx = ((rmax+65535)>>16);
        for(x=lx; x<=rx; x++)
        {
            uwall[x] = max(uwall[x],startumost(x+windowx1)-windowy1);
            dwall[x] = min(dwall[x],startdmost(x+windowx1)-windowy1);
        }

        /* Additional uwall/dwall clipping goes here */
        for(i=smostwallcnt-1; i>=0; i--)
        {
            j = smostwall[i];
            if ((pvWalls[j].screenSpaceCoo[0].vec_col > rx) || (pvWalls[j].screenSpaceCoo[1].vec_col < lx)) continue;
            if ((yp <= getVecDist(&pvWalls[j].screenSpaceCoo[0])) && (yp <= getVecDist(&pvWalls[j].screenSpaceCoo[1]))) continue;

            /* if (spritewallfront(tspr,thewall[j]) == 0) */
#if PVWALL_HAS_WORLDWALLID
            x = pvWalls[j].worldWallId;
#else
            x = pvWalls_worldWallId[j];
#endif
            const walltype *wallx = getWall(x);

            xp1 = wallx->x;
            yp1 = wallx->y;
            x = constwall[x]._point2; //getWall_point2(x);
            wallx = getWall(x);
            xp2 = wallx->x;
            yp2 = wallx->y;
            x = (xp2-xp1)*(tspr->y-yp1)-(tspr->x-xp1)*(yp2-yp1);
            if ((yp > getVecDist(&pvWalls[j].screenSpaceCoo[0])) && (yp > getVecDist(&pvWalls[j].screenSpaceCoo[1]))) x = -1;
#if PVWALL_HAS_WORLDWALLID
            if ((x >= 0) && ((x != 0) || (constwall[pvWalls[j].worldWallId].nextsector != tspr->sectnum))) continue;
#else
            if ((x >= 0) && ((x != 0) || (constwall[pvWalls_worldWallId[j]].nextsector != tspr->sectnum))) continue;
#endif
            dalx2 = max(pvWalls[j].screenSpaceCoo[0].vec_col,lx);
            darx2 = min(pvWalls[j].screenSpaceCoo[1].vec_col,rx);

            switch(get_smostwalltype(i))
            {
            case 0:
                if (dalx2 <= darx2)
                {
                    if ((dalx2 == lx) && (darx2 == rx)) return;
                    clearbufbyte(&dwall[dalx2],(darx2-dalx2+1)*sizeof(dwall[0]),0L);
                }
                break;
            case 1:
                k = smoststart[i] - pvWalls[j].screenSpaceCoo[0].vec_col;
                for(x=dalx2; x<=darx2; x++)
                    if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
                break;
            case 2:
                k = smoststart[i] - pvWalls[j].screenSpaceCoo[0].vec_col;
                for(x=dalx2; x<=darx2; x++)
                    if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
                break;
            }
        }

        /* sprite */
        if ((searchit >= 1) && (searchx >= lx) && (searchx <= rx))
            if ((searchy >= uwall[searchx]) && (searchy <= dwall[searchx]))
            {
                searchsector = sectnum;
                searchwall = spritenum;
                searchstat = 3;
                searchit = 1;
            }

        globalorientation = cstat;
        globalpicnum = tilenum;
        if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES)
            globalpicnum = 0;

        TILE_MakeAvailable(globalpicnum);

        setgotpic(globalpicnum);
        globalbufplc = tileData[globalpicnum];
        if (isOnExternalFlash(globalbufplc))
        {
            int size = tiles[globalpicnum].dim.height * tiles[globalpicnum].dim.width;
            if (size > MAX_RAM_FLAT_SIZE)
            {
                FIXME("This is bad. Size of ceiling sprite is %d\r\n", size);
            }
            globalbufplc = extMemGetDataFromAddress(flat, globalbufplc, size);
        }
//
//
        globvis = mulscale16(globalhisibility,viewingrange);
        if (constsector[sectnum].visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[sectnum].visibility+16)));

        x = picsiz[globalpicnum];
        y = ((x>>4)&15);
        x &= 15;
        if (pow2long[x] != spriteDim.width)
        {
            x++;
            globalx1 = mulscale(globalx1,spriteDim.width,x);
            globalx2 = mulscale(globalx2,spriteDim.width,x);
        }

        dax = globalxpanning;
        day = globalypanning;
        globalxpanning = -dmulscale6(globalx1,day,globalx2,dax);
        globalypanning = -dmulscale6(globaly1,day,globaly2,dax);

        globalx2 = mulscale16(globalx2,viewingrange);
        globaly2 = mulscale16(globaly2,viewingrange);
        globalzd = mulscale16(globalzd,viewingrangerecip);

        globalx1 = (globalx1-globalx2)*halfxdimen;
        globaly1 = (globaly1-globaly2)*halfxdimen;

        if ((cstat&2) == 0)
            msethlineshift(x,y);
        else
            tsethlineshift(x,y);

        /* Draw it! */
        ceilspritescan(lx,rx-1);
    }

    if (automapping == 1) show2dsprite[spritenum>>3] |= pow2char[spritenum&7];
}

/*
     FCS: Draw every transparent sprites in Back To Front Order. Also draw decals on the walls...
 */
void drawmasks(void)
{
    #if USE_STACK_SPRITESX
        int32_t stackspritesx[MAXSPRITESONSCREEN];
        spritesx = stackspritesx;
    #endif // USE_STACK_SPRITESX
    #if USE_STACK_SPRITESY
        int32_t stackspritesy[MAXSPRITESONSCREEN + 1]; // why +1 >
        spritesy = stackspritesy;
    #endif // USE_STACK_SPRITESY
    #if USE_STACK_SPRITESZ
        int32_t stackspritesz[MAXSPRITESONSCREEN];
        spritesz = stackspritesz;
    #endif // USE_STACK_SPRITESZ
    int32_t i, j, k, l, gap, xs, ys, xp, yp, yoff, yspan;
    /* int32_t zs, zp; */
    int16_t stack_tspriteIdx[MAXSPRITESONSCREEN];
    tspriteIdx = stack_tspriteIdx;
    //Copy sprite address in a sprite proxy structure (pointers are easier to re-arrange than structs).
    for(i=spritesortcnt-1; i>=0; i--)
    {
#if USE_TSPRITEPTR
        tspriteptr[i] = &tsprite[i];
#else
        tspriteIdx[i] = i;
#endif
    }

    //Generate screenspace coordinate (X column and Y distance).
    for(i=spritesortcnt-1; i>=0; i--)
    {
        //Translate and rotate the sprite in Camera space coordinate.
#if USE_TSPRITEPTR
        xs = tspriteptr[i]->x-globalposx;
        ys = tspriteptr[i]->y-globalposy;
#else
        xs = tsprite[tspriteIdx[i]].x-globalposx;
        ys = tsprite[tspriteIdx[i]].y-globalposy;
#endif
        yp = dmulscale6(xs,cosviewingrangeglobalang,ys,sinviewingrangeglobalang);

        if (yp > (4<<8))
        {
            xp = dmulscale6(ys,cosglobalang,-xs,singlobalang);
#if USE_SPRITESX
            spritesx[i] = scale(xp+yp,xdimen<<7,yp);
#else
            tsprite[tspriteIdx[i]].x = scale(xp+yp,xdimen<<7,yp);
#endif
        }
#if USE_TSPRITEPTR
        else if ((tspriteptr[i]->cstat&48) == 0)
#else
        else if ((tsprite[tspriteIdx[i]].cstat&48) == 0)
#endif
        {
            spritesortcnt--;  /* Delete face sprite if on wrong side! */
            //Move the sprite at the end of the array and decrease array length.
            if (i != spritesortcnt)
            {
#if USE_TSPRITEPTR
                tspriteptr[i] = tspriteptr[spritesortcnt];
#else
                tspriteIdx[i] = tspriteIdx[spritesortcnt];
#endif
#if USE_SPRITESX
                spritesx[i] = spritesx[spritesortcnt];
#elif WE_DO_NEED_TO_SWAP
                tsprite[tspriteIdx[i]].x = tsprite[tspriteIdx[spritesortcnt]].x;
#endif
                spritesy[i] = spritesy[spritesortcnt];
            }
            continue;
        }
        spritesy[i] = yp;
    }

    //FCS: Bubble sort ?! REally ?!?!?
    gap = 1;
    while (gap < spritesortcnt) gap = (gap<<1)+1;
    for(gap>>=1; gap>0; gap>>=1)    /* Sort sprite list */
        for(i=0; i<spritesortcnt-gap; i++)
            for(l=i; l>=0; l-=gap)
            {
                if (spritesy[l] <= spritesy[l+gap])
                    break;
#if USE_TSPRITEPTR
                swaplong((int32_t *)&tspriteptr[l],(int32_t *)&tspriteptr[l+gap]);
#else
                swapshort((short *)&tspriteIdx[l],(short *)&tspriteIdx[l+gap]);
#endif
#if LONG_SPRITESXYZ
#if USE_SPRITESX
                swaplong(&spritesx[l],&spritesx[l+gap]);
#elif WE_DO_NEED_TO_SWAP
                int temp;
                temp = tsprite[tspriteIdx[l]].x;
                tsprite[tspriteIdx[l]].x = tsprite[tspriteIdx[l + gap]].x;
                tsprite[tspriteIdx[l + gap]].x = temp;

#endif
                swaplong(&spritesy[l],&spritesy[l+gap]);
#else
                swapshort(&spritesx[l],&spritesx[l+gap]);
                swapshort(&spritesy[l],&spritesy[l+gap]);
#endif
            }

    if (spritesortcnt > 0)
        spritesy[spritesortcnt] = (spritesy[spritesortcnt-1]^1);

    ys = spritesy[0];
    i = 0;
    for(j=1; j<=spritesortcnt; j++)
    {
        if (spritesy[j] == ys)
            continue;

        ys = spritesy[j];
        if (j > i+1)
        {
            for(k=i; k<j; k++)
            {
#if USE_TSPRITEPTR
                spritesz[k] = tspriteptr[k]->z;
#else
#if USE_SPRITESZ
                spritesz[k] = tsprite[tspriteIdx[k]].z;
#else
                // nothing.
#endif
#endif
#if USE_TSPRITEPTR
                if ((tspriteptr[k]->cstat&48) != 32)
#else
                if ((tsprite[tspriteIdx[k]].cstat&48) != 32)
#endif
                {
#if TSPRITE_YOFFSET
                    yoff = (int32_t)((int8_t )((tiles[tspriteptr[k]->picnum].animFlags>>16)&255))+((int32_t)tspriteptr[k]->yoffset);
#else
#if USE_TSPRITEPTR
                if (tspriteptr[k]->owner >= 0)
                    yoff = (int32_t)sprite[tspriteptr[k]->owner].yoffset;
                else
                    yoff = 0;
                 yoff = (int32_t)((int8_t )((tiles[tspriteptr[k]->picnum].animFlags>>16)&255))+(yoff);

#else
                if (tsprite[tspriteIdx[k]].owner >= 0)
                    yoff = (int32_t)getSprite_yoffset(tsprite[tspriteIdx[k]].owner);
                else
                    yoff = 0;
                 yoff = (int32_t)((int8_t )((tiles[tsprite[tspriteIdx[k]].picnum].animFlags>>16)&255))+(yoff);

#endif


#endif

#if USE_TSPRITEPTR
                    spritesz[k] -= ((yoff*tspriteptr[k]->yrepeat)<<2);
                    yspan = (tiles[tspriteptr[k]->picnum].dim.height*tspriteptr[k]->yrepeat<<2);
                    if (!(tspriteptr[k]->cstat&128))
                        spritesz[k] -= (yspan>>1);

#else
#if USE_SPRITESZ
                    spritesz[k] -= ((yoff*tsprite[tspriteIdx[k]].yrepeat)<<2);
#else
                    tsprite[tspriteIdx[k]].z -= ((yoff*tsprite[tspriteIdx[k]].yrepeat)<<2);

#endif
                    yspan = (tiles[tsprite[tspriteIdx[k]].picnum].dim.height*tsprite[tspriteIdx[k]].yrepeat<<2);
                    if (!(tsprite[tspriteIdx[k]].cstat&128))
#if USE_SPRITESZ
                        spritesz[k] -= (yspan>>1);
#else
                    tsprite[tspriteIdx[k]].z  -= (yspan>>1);
#endif

#endif
#if USE_SPRITESZ
                    if (klabs(spritesz[k]-globalposz) < (yspan>>1))
                        spritesz[k] = globalposz;
#else
                    if (klabs(tsprite[tspriteIdx[k]].z-globalposz) < (yspan>>1))
                         tsprite[tspriteIdx[k]].z = globalposz;
#endif
                }
            }
            for(k=i+1; k<j; k++)
                for(l=i; l<k; l++)
#if USE_SPRITESZ
                    if (klabs(spritesz[k]-globalposz) < klabs(spritesz[l]-globalposz))
#else
                    if (klabs(tsprite[tspriteIdx[k]].z-globalposz) < klabs(tsprite[tspriteIdx[l]].z-globalposz))
#endif
                    {
#if USE_TSPRITEPTR
                        swaplong((int32_t *)&tspriteptr[k],(int32_t *)&tspriteptr[l]);
#else
                        swapshort((short *)&tspriteIdx[k],(short *)&tspriteIdx[l]);
#endif
#if LONG_SPRITESXYZ
#if USE_SPRITESX
                        swaplong(&spritesx[k],&spritesx[l]);
#elif WE_DO_NEED_TO_SWAP
                    int tempx = tsprite[tspriteIdx[k]].x;
                    tsprite[tspriteIdx[k]].x = tsprite[tspriteIdx[l]].x;
                    tsprite[tspriteIdx[l]].x = tempx;
#endif
                        swaplong(&spritesy[k],&spritesy[l]);
#if USE_TSPRITEPTR
                        swaplong(&spritesz[k],&spritesz[l]);
#elif WE_DO_NEED_TO_SWAP
                        int32_t tempz = tsprite[tspriteIdx[k]].z;
                        tsprite[tspriteIdx[k]].z = tsprite[tspriteIdx[l]].z;
                        tsprite[tspriteIdx[l]].z = tempz;
#endif
#else
                        swapshort(&spritesx[k],&spritesx[l]);
                        swapshort(&spritesy[k],&spritesy[l]);
                        swapshort(&spritesz[k],&spritesz[l]);

#endif
                    }
            for(k=i+1; k<j; k++)
                for(l=i; l<k; l++)
#if USE_TSPRITEPTR
                    if (tspriteptr[k]->statnum < tspriteptr[l]->statnum)
                    {
                        swaplong((int32_t *)&tspriteptr[k],(int32_t *)&tspriteptr[l]);
#else
                    if (tsprite[tspriteIdx[k]].statnum < tsprite[tspriteIdx[l]].statnum)
                    {
                        swapshort((short *)&tspriteIdx[k],(short *)&tspriteIdx[l]);

#endif
#if LONG_SPRITESXYZ
#if USE_SPRITESX
                        swaplong(&spritesx[k],&spritesx[l]);
#elif WE_DO_NEED_TO_SWAP
                        int32_t tempx = tsprite[tspriteIdx[k]].x;
                        tsprite[tspriteIdx[k]].x = tsprite[tspriteIdx[l]].x;
                        tsprite[tspriteIdx[l]].x = tempx;
#endif
                        swaplong(&spritesy[k],&spritesy[l]);
#else
                        swapshort(&spritesx[k],&spritesx[l]);
                        swapshort(&spritesy[k],&spritesy[l]);
#endif
                    }
        }
        i = j;
    }

    while ((spritesortcnt > 0) && (maskwallcnt > 0))  /* While BOTH > 0 */
    {
        j = maskwall[maskwallcnt-1];
#if USE_TSPRITEPTR
        if (spritewallfront(tspriteptr[spritesortcnt-1],pvWalls[j].worldWallId) == 0)
#else
#if PVWALL_HAS_WORLDWALLID
        if (spritewallfront(&tsprite[tspriteIdx[spritesortcnt-1]],pvWalls[j].worldWallId) == 0)
#else
        if (spritewallfront(&tsprite[tspriteIdx[spritesortcnt-1]],pvWalls_worldWallId[j]) == 0)
#endif
#endif
            drawsprite(--spritesortcnt);
        else
        {
            /* Check to see if any sprites behind the masked wall... */
            k = -1;
            gap = 0;
            for(i=spritesortcnt-2; i>=0; i--)
#if USE_SPRITESX
                if ((pvWalls[j].screenSpaceCoo[0].vec_col <= (spritesx[i]>>8)) && ((spritesx[i]>>8) <= pvWalls[j].screenSpaceCoo[1].vec_col))
#else
                if ((pvWalls[j].screenSpaceCoo[0].vec_col <= (tsprite[tspriteIdx[i]].x>>8)) && ((tsprite[tspriteIdx[i]].x>>8) <= pvWalls[j].screenSpaceCoo[1].vec_col))
#endif
#if USE_TSPRITEPTR
                    if (spritewallfront(tspriteptr[i],pvWalls[j].worldWallId) == 0)
                    {
                        drawsprite(i);
                        tspriteptr[i]->owner = -1;
                        k = i;
                        gap++;
                    }
            if (k >= 0)       /* remove holes in sprite list */
            {
                for(i=k; i<spritesortcnt; i++)
                    if (tspriteptr[i]->owner >= 0)
                    {
                        if (i > k)
                        {
                            tspriteptr[k] = tspriteptr[i];
                            spritesx[k] = spritesx[i];
                            spritesy[k] = spritesy[i];
                        }
                        k++;
                    }
                spritesortcnt -= gap;
            }
#else
#if PVWALL_HAS_WORLDWALLID
                    if (spritewallfront(&tsprite[tspriteIdx[i]],pvWalls[j].worldWallId) == 0)
#else
                    if (spritewallfront(&tsprite[tspriteIdx[i]],pvWalls_worldWallId[j]) == 0)
#endif
                    {
                        drawsprite(i);
                        tsprite[tspriteIdx[i]].owner = -1;
                        k = i;
                        gap++;
                    }
            if (k >= 0)       /* remove holes in sprite list */
            {
                for(i=k; i<spritesortcnt; i++)
                    if (tsprite[tspriteIdx[i]].owner >= 0)
                    {
                        if (i > k)
                        {
                            tspriteIdx[k] = tspriteIdx[i];
#if USE_SPRITESX
                            spritesx[k] = spritesx[i];
#elif WE_DO_NEED_TO_SWAP
                            tsprite[tspriteIdx[k]].x = tsprite[tspriteIdx[i]].x;
#endif
                            spritesy[k] = spritesy[i];
                        }
                        k++;
                    }
                spritesortcnt -= gap;
            }
#endif
            /* finally safe to draw the masked wall */
            drawmaskwall(--maskwallcnt);
        }
    }
    while (spritesortcnt > 0) drawsprite(--spritesortcnt);
    while (maskwallcnt > 0) drawmaskwall(--maskwallcnt);
    tspriteIdx = NULL;
#if USE_STACK_SPRITESX
    spritesx = NULL;
#endif // USE_STACK_SPRITESX
#if USE_STACK_SPRITESY
    spritesy = NULL;
#endif // USE_STACK_SPRITESY
#if USE_STACK_SPRITESZ
    spritesz = NULL;
#endif // USE_STACK_SPRITESZ
}


int setsprite(short spritenum, int32_t newx, int32_t newy, int32_t newz)
{
    short tempsectnum;

    setSprite_x(spritenum, newx);
    setSprite_y(spritenum, newy);
    setSprite_z(spritenum, newz);

    tempsectnum = getSprite_sectnum(spritenum);
    updatesector(newx,newy, &tempsectnum);
     if (tempsectnum < 0)
     {
        return(-1);
     }
    if (tempsectnum != getSprite_sectnum(spritenum))
        changespritesect(spritenum,tempsectnum);

    return(0);
}


void initspritelists(void)
{
    int32_t i;

    for (i=0; i<MAXSECTORS; i++)   /* Init doubly-linked sprite sector lists */
        headspritesect[i] = -1;

    headspritesect[MAXSECTORS] = 0;

    for(i=0; i<MAXSPRITES; i++){
#if SPRITES_USE_DOUBLY_LINKED_LIST
        prevspritesect[i] = i-1;
#endif
        spr_add_data[i].nextspritesect = i+1;
        ((spritetype*) &sprites[i])->_sectnum = MAXSECTORS;  // this is safe: this is done in loadboard, where sprites is pointing to a temporary buffer.
    }
#if SPRITES_USE_DOUBLY_LINKED_LIST
    prevspritesect[0] = -1; // why this ? wasn't the previous loop doing the same ?
#endif
    spr_add_data[MAXSPRITES-1].nextspritesect = -1;


    for(i=0; i<MAXSTATUS; i++)    /* Init doubly-linked sprite status lists */
        headspritestat[i] = -1;
    headspritestat[MAXSTATUS] = 0;
    for(i=0; i<MAXSPRITES; i++)
    {
#if SPRITES_USE_DOUBLY_LINKED_LIST
        prevspritestat[i] = i-1;
#endif
        spr_add_data[i].nextspritestat = i+1;
         ((spritetype*) &sprites[i])->_statnum = MAXSTATUS; // this is safe: this is done in loadboard, where sprites is pointing to a temporary buffer.
    }
#if SPRITES_USE_DOUBLY_LINKED_LIST
    prevspritestat[0] = -1;
#endif
    spr_add_data[MAXSPRITES-1].nextspritestat = -1;
}
#if WIN32
int maxSprites = 0;
#endif
int insertsprite(short sectnum, short statnum)
{
    #if WIN32
        maxSprites ++;
//        printf("num sprites %d\r\n", maxSprites);
    #endif
    insertspritestat(statnum);
    return(insertspritesect(sectnum));
}


int insertspritesect(short sectnum)
{
    short blanktouse;

    if ((sectnum >= MAXSECTORS) || (headspritesect[MAXSECTORS] == -1))
        return(-1);  /* list full */

    blanktouse = headspritesect[MAXSECTORS];

    headspritesect[MAXSECTORS] = spr_add_data[blanktouse].nextspritesect;
#if SPRITES_USE_DOUBLY_LINKED_LIST
    if (headspritesect[MAXSECTORS] >= 0)
        prevspritesect[headspritesect[MAXSECTORS]] = -1;

    prevspritesect[blanktouse] = -1;
#endif
    spr_add_data[blanktouse].nextspritesect = headspritesect[sectnum];
#if SPRITES_USE_DOUBLY_LINKED_LIST
    if (headspritesect[sectnum] >= 0)
        prevspritesect[headspritesect[sectnum]] = blanktouse;
#endif
    headspritesect[sectnum] = blanktouse;

    setSprite_sectnum(blanktouse, sectnum);

    return(blanktouse);
}


int insertspritestat(short statnum)
{
    short blanktouse;

    if ((statnum >= MAXSTATUS) || (headspritestat[MAXSTATUS] == -1))
        return(-1);  /* list full */

    blanktouse = headspritestat[MAXSTATUS];

    headspritestat[MAXSTATUS] = spr_add_data[blanktouse].nextspritestat;
#if SPRITES_USE_DOUBLY_LINKED_LIST
    if (headspritestat[MAXSTATUS] >= 0)
        prevspritestat[headspritestat[MAXSTATUS]] = -1;
    prevspritestat[blanktouse] = -1;
#endif

    spr_add_data[blanktouse].nextspritestat = headspritestat[statnum];
#if SPRITES_USE_DOUBLY_LINKED_LIST
    if (headspritestat[statnum] >= 0)
        prevspritestat[headspritestat[statnum]] = blanktouse;
#endif
    headspritestat[statnum] = blanktouse;

    setSprite_statnum(blanktouse, statnum);

    return(blanktouse);
}

static inline int prevspritestat(int index)
{
    for (int i = 0; i < MAXSPRITES; i++)
    {
        if (spr_add_data[i].nextspritestat == index)
            return i;
    }
    return -1;
}
static inline int prevspritesect(int index)
{
    for (int i = 0; i < MAXSPRITES; i++)
    {
        if (spr_add_data[i].nextspritesect == index)
            return i;
    }
    return -1;
}

int deletesprite(short spritenum)
{

    deletespritestat(spritenum);
    deletehittype(spritenum);
    #if WIN32
        maxSprites --;
    #endif
    int r = ( deletespritesect(spritenum));
    freeRamSprite(spritenum);
    return r;
}


int deletespritesect(short deleteme)
{
    if (getSprite(deleteme)->_sectnum == MAXSECTORS)
        return(-1);

    if (headspritesect[getSprite(deleteme)->_sectnum] == deleteme)
        headspritesect[getSprite(deleteme)->_sectnum] = spr_add_data[deleteme].nextspritesect;
#if SPRITES_USE_DOUBLY_LINKED_LIST
    if (prevspritesect[deleteme] >= 0) nextspritesect[prevspritesect[deleteme]] = nextspritesect[deleteme];
    if (nextspritesect[deleteme] >= 0) prevspritesect[nextspritesect[deleteme]] = prevspritesect[deleteme];
    if (headspritesect[MAXSECTORS] >= 0) prevspritesect[headspritesect[MAXSECTORS]] = deleteme;
    prevspritesect[deleteme] = -1;
#else
    int prev = prevspritesect(deleteme);
    if ( prev >= 0) spr_add_data[prev].nextspritesect = spr_add_data[deleteme].nextspritesect;
#endif

    spr_add_data[deleteme].nextspritesect = headspritesect[MAXSECTORS];
    headspritesect[MAXSECTORS] = deleteme;

    // do we need this ?
    setSprite_sectnum(deleteme,  MAXSECTORS);
    return(0);
}


int deletespritestat(short deleteme)
{
    if (getSprite(deleteme)->_statnum == MAXSTATUS)
        return(-1);

    if (headspritestat[getSprite(deleteme)->_statnum] == deleteme)
        headspritestat[getSprite(deleteme)->_statnum] = spr_add_data[deleteme].nextspritestat;
#if SPRITES_USE_DOUBLY_LINKED_LIST
    if (prevspritestat[deleteme] >= 0) nextspritestat[prevspritestat[deleteme]] = nextspritestat[deleteme];
    if (nextspritestat[deleteme] >= 0) prevspritestat[nextspritestat[deleteme]] = prevspritestat[deleteme];

    if (headspritestat[MAXSTATUS] >= 0) prevspritestat[headspritestat[MAXSTATUS]] = deleteme;
    prevspritestat[deleteme] = -1;
#else
    int prev = prevspritestat(deleteme);
    if (prev >= 0) spr_add_data[prev].nextspritestat = spr_add_data[deleteme].nextspritestat;
#endif
    spr_add_data[deleteme].nextspritestat = headspritestat[MAXSTATUS];
    headspritestat[MAXSTATUS] = deleteme;

    setSprite_statnum(deleteme, MAXSTATUS);
    return(0);
}


int changespritesect(short spritenum, short newsectnum)
{
    if ((newsectnum < 0) || (newsectnum > MAXSECTORS)) return(-1);
    if (getSprite_sectnum(spritenum) == newsectnum) return(0);
    if (getSprite_sectnum(spritenum) == MAXSECTORS) return(-1);
    if (deletespritesect(spritenum) < 0) return(-1);
    insertspritesect(newsectnum);
    return(0);
}


int changespritestat(short spritenum, short newstatnum)
{
    if ((newstatnum < 0) || (newstatnum > MAXSTATUS)) return(-1);
    if (getSprite_statnum(spritenum) == newstatnum) return(0);
    if (getSprite_statnum(spritenum) == MAXSTATUS) return(-1);
    if (deletespritestat(spritenum) < 0) return(-1);
    insertspritestat(newstatnum);
    return(0);
}


int nextsectorneighborz(short sectnum, int32_t thez,
                        short topbottom, short direction)
{
    int32_t i, testz, nextz;
    short sectortouse;

    if (direction == 1) nextz = 0x7fffffff;
    else nextz = 0x80000000;

    sectortouse = -1;
    int wallIdx = constsector[sectnum].wallptr;
    i = constsector[sectnum].wallnum;
    do
    {
        if (constwall[wallIdx].nextsector >= 0)
        {
            if (topbottom == 1)
            {
                testz = getSector(constwall[wallIdx].nextsector)->floorz;
                if (direction == 1)
                {
                    if ((testz > thez) && (testz < nextz))
                    {
                        nextz = testz;
                        sectortouse = constwall[wallIdx].nextsector;
                    }
                }
                else
                {
                    if ((testz < thez) && (testz > nextz))
                    {
                        nextz = testz;
                        sectortouse = constwall[wallIdx].nextsector;
                    }
                }
            }
            else
            {
                testz = getSector(constwall[wallIdx].nextsector)->ceilingz;
                if (direction == 1)
                {
                    if ((testz > thez) && (testz < nextz))
                    {
                        nextz = testz;
                        sectortouse = constwall[wallIdx].nextsector;
                    }
                }
                else
                {
                    if ((testz < thez) && (testz > nextz))
                    {
                        nextz = testz;
                        sectortouse = constwall[wallIdx].nextsector;
                    }
                }
            }
        }
        wallIdx++;
        i--;
    } while (i != 0);

    return(sectortouse);
}


int cansee(int32_t x1, int32_t y1, int32_t z1, short sect1,
           int32_t x2, int32_t y2, int32_t z2, short sect2)
{
    walltype *wal, *wal2;
    int32_t i, cnt, nexts, x, y, z, cz, fz, dasectnum, dacnt, danum;
    int32_t x21, y21, z21, x31, y31, x34, y34, bot, t;
    short clipsectorlist[MAXCLIPNUM];
    if ((x1 == x2) && (y1 == y2))
        return(sect1 == sect2);

    x21 = x2-x1;
    y21 = y2-y1;
    z21 = z2-z1;

    clipsectorlist[0] = sect1;
    danum = 1;
    for(dacnt=0; dacnt<danum; dacnt++)
    {
        dasectnum = clipsectorlist[dacnt];
        int wallIdx = constsector[dasectnum].wallptr;
        for(cnt=constsector[dasectnum].wallnum,wal=getWall(constsector[dasectnum].wallptr); cnt>0; cnt--, wallIdx++, wal = getWall(wallIdx))
        {
            wal2 = getWall(getWall_point2(wallIdx));
            x31 = wal->x-x1;
            x34 = wal->x-wal2->x;
            y31 = wal->y-y1;
            y34 = wal->y-wal2->y;

            bot = y21*x34-x21*y34;
            if (bot <= 0)
                continue;

            t = y21*x31-x21*y31;
            if ((uint32_t)t >= (uint32_t)bot)
                continue;
            t = y31*x34-x31*y34;
            if ((uint32_t)t >= (uint32_t)bot)
                continue;

            nexts = constwall[wallIdx].nextsector;
            if ((nexts < 0) || (getWall_cstat(wallIdx)&32)) return(0);

            t = divscale24(t,bot);
            x = x1 + mulscale24(x21,t);
            y = y1 + mulscale24(y21,t);
            z = z1 + mulscale24(z21,t);

            getzsofslope((short)dasectnum,x,y,&cz,&fz);
            if ((z <= cz) || (z >= fz))
                return(0);
            getzsofslope((short)nexts,x,y,&cz,&fz);
            if ((z <= cz) || (z >= fz))
                return(0);

            for(i=danum-1; i>=0; i--)
                if (clipsectorlist[i] == nexts)
                    break;
            if (i < 0)
                clipsectorlist[danum++] = nexts;
        }
    }
    for(i=danum-1; i>=0;i--)
        if (clipsectorlist[i] == sect2)
            return(1);
    return(0);
}


int lintersect(int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2,
               int32_t x3, int32_t y3, int32_t x4, int32_t y4, int32_t *intx,
               int32_t *inty, int32_t *intz)
{   /* p1 to p2 is a line segment */
    int32_t x21, y21, x34, y34, x31, y31, bot, topt, topu, t;

    x21 = x2-x1;
    x34 = x3-x4;
    y21 = y2-y1;
    y34 = y3-y4;
    bot = x21*y34 - y21*x34;
    if (bot >= 0)
    {
        if (bot == 0) return(0);
        x31 = x3-x1;
        y31 = y3-y1;
        topt = x31*y34 - y31*x34;
        if ((topt < 0) || (topt >= bot)) return(0);
        topu = x21*y31 - y21*x31;
        if ((topu < 0) || (topu >= bot)) return(0);
    }
    else
    {
        x31 = x3-x1;
        y31 = y3-y1;
        topt = x31*y34 - y31*x34;
        if ((topt > 0) || (topt <= bot)) return(0);
        topu = x21*y31 - y21*x31;
        if ((topu > 0) || (topu <= bot)) return(0);
    }
    t = divscale24(topt,bot);
    *intx = x1 + mulscale24(x21,t);
    *inty = y1 + mulscale24(y21,t);
    *intz = z1 + mulscale24(z2-z1,t);
    return(1);
}


int rintersect(int32_t x1, int32_t y1, int32_t z1, int32_t vx, int32_t vy, int32_t vz,
               int32_t x3, int32_t y3, int32_t x4, int32_t y4, int32_t *intx,
               int32_t *inty, int32_t *intz)
{   /* p1 towards p2 is a ray */
    int32_t x34, y34, x31, y31, bot, topt, topu, t;

    x34 = x3-x4;
    y34 = y3-y4;
    bot = vx*y34 - vy*x34;
    if (bot >= 0)
    {
        if (bot == 0) return(0);
        x31 = x3-x1;
        y31 = y3-y1;
        topt = x31*y34 - y31*x34;
        if (topt < 0) return(0);
        topu = vx*y31 - vy*x31;
        if ((topu < 0) || (topu >= bot)) return(0);
    }
    else
    {
        x31 = x3-x1;
        y31 = y3-y1;
        topt = x31*y34 - y31*x34;
        if (topt > 0) return(0);
        topu = vx*y31 - vy*x31;
        if ((topu > 0) || (topu <= bot)) return(0);
    }
    t = divscale16(topt,bot);
    *intx = x1 + mulscale16(vx,t);
    *inty = y1 + mulscale16(vy,t);
    *intz = z1 + mulscale16(vz,t);
    return(1);
}


int hitscan(int32_t xs, int32_t ys, int32_t zs, short sectnum,
            int32_t vx, int32_t vy, int32_t vz,
            short *hitsect, short *hitwall, short *hitsprite,
            int32_t *hitx, int32_t *hity, int32_t *hitz, uint32_t  cliptype)
{
    const sectortype *sec;
    walltype *wal, *wal2;
//    const spritetype *spr;
    int32_t z, zz, x1, y1=0, z1=0, x2, y2, x3, y3, x4, y4, intx, inty, intz;
    int32_t topt, topu, bot, dist, offx, offy, cstat;
    int32_t i, j, k, l, tilenum, xoff, yoff, dax, day, daz, daz2;
    int32_t ang, cosang, sinang, xspan, yspan, xrepeat, yrepeat;
    int32_t dawalclipmask, dasprclipmask;
    short tempshortcnt, tempshortnum, dasector, startwall, endwall;
    short nextsector;
    uint8_t  clipyou;

    *hitsect = -1;
    *hitwall = -1;
    *hitsprite = -1;
    if (sectnum < 0) return(-1);

    *hitx = hitscangoalx;
    *hity = hitscangoaly;

    dawalclipmask = (cliptype&65535);
    dasprclipmask = (cliptype>>16);
    short clipsectorlist[MAXCLIPNUM];
    clipsectorlist[0] = sectnum;
    tempshortcnt = 0;
    tempshortnum = 1;
    do
    {
        dasector = clipsectorlist[tempshortcnt];
        sec = getSector(dasector);

        x1 = 0x7fffffff;
        if (sec->ceilingstat&2)
        {
            wal = getWall(constsector[dasector].wallptr);
            wal2 = getWall(getWall_point2(constsector[dasector].wallptr));
            dax = wal2->x-wal->x;
            day = wal2->y-wal->y;
            i = nsqrtasm(dax*dax+day*day);
            if (i == 0) continue;
            i = divscale15(constsector[dasector].ceilingheinum,i);
            dax *= i;
            day *= i;

            j = (vz<<8)-dmulscale15(dax,vy,-day,vx);
            if (j != 0)
            {
                i = ((sec->ceilingz-zs)<<8)+dmulscale15(dax,ys-wal->y,-day,xs-wal->x);
                if (((i^j) >= 0) && ((klabs(i)>>1) < klabs(j)))
                {
                    i = divscale30(i,j);
                    x1 = xs + mulscale30(vx,i);
                    y1 = ys + mulscale30(vy,i);
                    z1 = zs + mulscale30(vz,i);
                }
            }
        }
        else if ((vz < 0) && (zs >= sec->ceilingz))
        {
            z1 = sec->ceilingz;
            i = z1-zs;
            if ((klabs(i)>>1) < -vz)
            {
                i = divscale30(i,vz);
                x1 = xs + mulscale30(vx,i);
                y1 = ys + mulscale30(vy,i);
            }
        }
        if ((x1 != 0x7fffffff) && (klabs(x1-xs)+klabs(y1-ys) < klabs((*hitx)-xs)+klabs((*hity)-ys)))
            if (inside(x1,y1,dasector) != 0)
            {
                *hitsect = dasector;
                *hitwall = -1;
                *hitsprite = -1;
                *hitx = x1;
                *hity = y1;
                *hitz = z1;
            }

        x1 = 0x7fffffff;
        if (sec->floorstat&2)
        {
            wal = getWall(constsector[dasector].wallptr);
            wal2 = getWall(getWall_point2(constsector[dasector].wallptr));
            dax = wal2->x-wal->x;
            day = wal2->y-wal->y;
            i = nsqrtasm(dax*dax+day*day);
            if (i == 0) continue;
            i = divscale15(sec->floorheinum,i);
            dax *= i;
            day *= i;

            j = (vz<<8)-dmulscale15(dax,vy,-day,vx);
            if (j != 0)
            {
                i = ((sec->floorz-zs)<<8)+dmulscale15(dax,ys-wal->y,-day,xs-wal->x);
                if (((i^j) >= 0) && ((klabs(i)>>1) < klabs(j)))
                {
                    i = divscale30(i,j);
                    x1 = xs + mulscale30(vx,i);
                    y1 = ys + mulscale30(vy,i);
                    z1 = zs + mulscale30(vz,i);
                }
            }
        }
        else if ((vz > 0) && (zs <= sec->floorz))
        {
            z1 = sec->floorz;
            i = z1-zs;
            if ((klabs(i)>>1) < vz)
            {
                i = divscale30(i,vz);
                x1 = xs + mulscale30(vx,i);
                y1 = ys + mulscale30(vy,i);
            }
        }
        if ((x1 != 0x7fffffff) && (klabs(x1-xs)+klabs(y1-ys) < klabs((*hitx)-xs)+klabs((*hity)-ys)))
            if (inside(x1,y1,dasector) != 0)
            {
                *hitsect = dasector;
                *hitwall = -1;
                *hitsprite = -1;
                *hitx = x1;
                *hity = y1;
                *hitz = z1;
            }

        startwall = constsector[dasector].wallptr;
        endwall = startwall + constsector[dasector].wallnum;
        for(z=startwall,wal=getWall(startwall); z<endwall; z++,wal = getWall(z))
        {
            wal2 = getWall(getWall_point2(z));
            x1 = wal->x;
            y1 = wal->y;
            x2 = wal2->x;
            y2 = wal2->y;

            if ((x1-xs)*(y2-ys) < (x2-xs)*(y1-ys)) continue;
            if (rintersect(xs,ys,zs,vx,vy,vz,x1,y1,x2,y2,&intx,&inty,&intz) == 0) continue;

            if (klabs(intx-xs)+klabs(inty-ys) >= klabs((*hitx)-xs)+klabs((*hity)-ys)) continue;

            nextsector = constwall[z].nextsector;
            if ((nextsector < 0) || (getWall_cstat(z)&dawalclipmask))
            {
                *hitsect = dasector;
                *hitwall = z;
                *hitsprite = -1;
                *hitx = intx;
                *hity = inty;
                *hitz = intz;
                continue;
            }
            getzsofslope(nextsector,intx,inty,&daz,&daz2);
            if ((intz <= daz) || (intz >= daz2))
            {
                *hitsect = dasector;
                *hitwall = z;
                *hitsprite = -1;
                *hitx = intx;
                *hity = inty;
                *hitz = intz;
                continue;
            }

            for(zz=tempshortnum-1; zz>=0; zz--)
                if (clipsectorlist[zz] == nextsector) break;
            if (zz < 0) clipsectorlist[tempshortnum++] = nextsector;
        }

        for(z=headspritesect[dasector]; z>=0; z=spr_add_data[z].nextspritesect)
        {
            cstat = getSprite(z)->_cstat;
            if ((cstat&dasprclipmask) == 0) continue;

            x1 = getSprite(z)->_x;
            y1 = getSprite(z)->_y;
            z1 = getSprite(z)->_z;
            switch(cstat&48)
            {
            case 0:
                topt = vx*(x1-xs) + vy*(y1-ys);
                if (topt <= 0) continue;
                bot = vx*vx + vy*vy;
                if (bot == 0) continue;

                intz = zs+scale(vz,topt,bot);

                i = (tiles[getSprite(z)->_picnum].dim.height*getSprite(z)->_yrepeat<<2);

                if (cstat&128)
                    z1 += (i>>1);

                if (tiles[getSprite(z)->_picnum].animFlags&0x00ff0000)
                    z1 -= ((int32_t)((int8_t )((tiles[getSprite(z)->_picnum].animFlags>>16)&255))*getSprite(z)->_yrepeat<<2);

                if ((intz > z1) || (intz < z1-i)) continue;
                topu = vx*(y1-ys) - vy*(x1-xs);

                offx = scale(vx,topu,bot);
                offy = scale(vy,topu,bot);
                dist = offx*offx + offy*offy;
                i = tiles[getSprite(z)->_picnum].dim.width*getSprite(z)->_xrepeat;
                i *= i;
                if (dist > (i>>7)) continue;
                intx = xs + scale(vx,topt,bot);
                inty = ys + scale(vy,topt,bot);

                if (klabs(intx-xs)+klabs(inty-ys) > klabs((*hitx)-xs)+klabs((*hity)-ys)) continue;

                *hitsect = dasector;
                *hitwall = -1;
                *hitsprite = z;
                *hitx = intx;
                *hity = inty;
                *hitz = intz;
                break;
            case 16:
                /*
                 * These lines get the 2 points of the rotated sprite
                 * Given: (x1, y1) starts out as the center point
                 */
                tilenum = getSprite(z)->_picnum;
                xoff = (int32_t)((int8_t )((tiles[tilenum].animFlags>>8)&255))+((int32_t)getSprite(z)->_xoffset);
                if ((cstat&4) > 0) xoff = -xoff;
                k = getSprite(z)->_ang;
                l = getSprite(z)->_xrepeat;
                dax = sintable[k&2047]*l;
                day = sintable[(k+1536)&2047]*l;
                l = tiles[tilenum].dim.width;
                k = (l>>1)+xoff;
                x1 -= mulscale16(dax,k);
                x2 = x1+mulscale16(dax,l);
                y1 -= mulscale16(day,k);
                y2 = y1+mulscale16(day,l);

                if ((cstat&64) != 0)   /* back side of 1-way sprite */
                    if ((x1-xs)*(y2-ys) < (x2-xs)*(y1-ys)) continue;

                if (rintersect(xs,ys,zs,vx,vy,vz,x1,y1,x2,y2,&intx,&inty,&intz) == 0) continue;

                if (klabs(intx-xs)+klabs(inty-ys) > klabs((*hitx)-xs)+klabs((*hity)-ys)) continue;

                k = ((tiles[getSprite(z)->_picnum].dim.height*getSprite(z)->_yrepeat)<<2);
                if (cstat&128)
                    daz = getSprite(z)->_z+(k>>1);
                else
                    daz = getSprite(z)->_z;

                if (tiles[getSprite(z)->_picnum].animFlags&0x00ff0000)
                    daz -= ((int32_t)((int8_t  )((tiles[getSprite(z)->_picnum].animFlags>>16)&255))*getSprite(z)->_yrepeat<<2);

                if ((intz < daz) && (intz > daz-k))
                {
                    *hitsect = dasector;
                    *hitwall = -1;
                    *hitsprite = z;
                    *hitx = intx;
                    *hity = inty;
                    *hitz = intz;
                }
                break;
            case 32:
                if (vz == 0) continue;
                intz = z1;
                if (((intz-zs)^vz) < 0) continue;
                if ((cstat&64) != 0)
                    if ((zs > intz) == ((cstat&8)==0)) continue;

                intx = xs+scale(intz-zs,vx,vz);
                inty = ys+scale(intz-zs,vy,vz);

                if (klabs(intx-xs)+klabs(inty-ys) > klabs((*hitx)-xs)+klabs((*hity)-ys)) continue;

                tilenum = getSprite(z)->_picnum;
                xoff = (int32_t)((int8_t )((tiles[tilenum].animFlags>>8)&255))+((int32_t)getSprite(z)->_xoffset);
                yoff = (int32_t)((int8_t )((tiles[tilenum].animFlags>>16)&255))+((int32_t)getSprite(z)->_yoffset);
                if ((cstat&4) > 0) xoff = -xoff;
                if ((cstat&8) > 0) yoff = -yoff;

                ang = getSprite(z)->_ang;
                cosang = sintable[(ang+512)&2047];
                sinang = sintable[ang];
                xspan = tiles[tilenum].dim.width;
                xrepeat = getSprite(z)->_xrepeat;
                yspan = tiles[tilenum].dim.height;
                yrepeat = getSprite(z)->_yrepeat;

                dax = ((xspan>>1)+xoff)*xrepeat;
                day = ((yspan>>1)+yoff)*yrepeat;
                x1 += dmulscale16(sinang,dax,cosang,day)-intx;
                y1 += dmulscale16(sinang,day,-cosang,dax)-inty;
                l = xspan*xrepeat;
                x2 = x1 - mulscale16(sinang,l);
                y2 = y1 + mulscale16(cosang,l);
                l = yspan*yrepeat;
                k = -mulscale16(cosang,l);
                x3 = x2+k;
                x4 = x1+k;
                k = -mulscale16(sinang,l);
                y3 = y2+k;
                y4 = y1+k;

                clipyou = 0;
                if ((y1^y2) < 0)
                {
                    if ((x1^x2) < 0) clipyou ^= (x1*y2<x2*y1)^(y1<y2);
                    else if (x1 >= 0) clipyou ^= 1;
                }
                if ((y2^y3) < 0)
                {
                    if ((x2^x3) < 0) clipyou ^= (x2*y3<x3*y2)^(y2<y3);
                    else if (x2 >= 0) clipyou ^= 1;
                }
                if ((y3^y4) < 0)
                {
                    if ((x3^x4) < 0) clipyou ^= (x3*y4<x4*y3)^(y3<y4);
                    else if (x3 >= 0) clipyou ^= 1;
                }
                if ((y4^y1) < 0)
                {
                    if ((x4^x1) < 0) clipyou ^= (x4*y1<x1*y4)^(y4<y1);
                    else if (x4 >= 0) clipyou ^= 1;
                }

                if (clipyou != 0)
                {
                    *hitsect = dasector;
                    *hitwall = -1;
                    *hitsprite = z;
                    *hitx = intx;
                    *hity = inty;
                    *hitz = intz;
                }
                break;
            }
        }
        tempshortcnt++;
    } while (tempshortcnt < tempshortnum);
    return(0);
}


int neartag(int32_t xs, int32_t ys, int32_t zs, short sectnum, short ange,
            short *neartagsector, short *neartagwall, short *neartagsprite,
            int32_t *neartaghitdist, int32_t neartagrange, uint8_t  tagsearch)
{
    walltype *wal, *wal2;
//    const spritetype *spr;
    int32_t i, z, zz, xe, ye, ze, x1, y1, z1, x2, y2, intx, inty, intz;
    int32_t topt, topu, bot, dist, offx, offy, vx, vy, vz;
    short tempshortcnt, tempshortnum, dasector, startwall, endwall;
    short nextsector, good;
    short clipsectorlist[MAXCLIPNUM];
    *neartagsector = -1;
    *neartagwall = -1;
    *neartagsprite = -1;
    *neartaghitdist = 0;

    if (sectnum < 0) return(0);
    if ((tagsearch < 1) || (tagsearch > 3)) return(0);

    vx = mulscale14(sintable[(ange+2560)&2047],neartagrange);
    xe = xs+vx;
    vy = mulscale14(sintable[(ange+2048)&2047],neartagrange);
    ye = ys+vy;
    vz = 0;
    ze = 0;

    clipsectorlist[0] = sectnum;
    tempshortcnt = 0;
    tempshortnum = 1;

    do
    {
        dasector = clipsectorlist[tempshortcnt];

        startwall = constsector[dasector].wallptr;
        endwall = startwall + constsector[dasector].wallnum - 1;
        for(z=startwall,wal=getWall(startwall); z<=endwall; z++,wal = getWall(z))
        {
            wal2 = getWall(getWall_point2(z));
            x1 = wal->x;
            y1 = wal->y;
            x2 = wal2->x;
            y2 = wal2->y;

            nextsector = constwall[z].nextsector;

            good = 0;
            if (nextsector >= 0)
            {
                if ((tagsearch&1) && getSector(nextsector)->lotag) good |= 1;
                if ((tagsearch&2) && getSector(nextsector)->hitag) good |= 1;
            }
#if NO_MINIMIZE_WALLS
            if ((tagsearch&1) && wal->lotag) good |= 2;
            if ((tagsearch&2) && wal->hitag) good |= 2;
#else
            if ((tagsearch&1) && getWall_lotag(z)) good |= 2;
            if ((tagsearch&2) && getWall_hitag(z)) good |= 2;
#endif
            if ((good == 0) && (nextsector < 0)) continue;
            if ((x1-xs)*(y2-ys) < (x2-xs)*(y1-ys)) continue;

            if (lintersect(xs,ys,zs,xe,ye,ze,x1,y1,x2,y2,&intx,&inty,&intz) == 1)
            {
                if (good != 0)
                {
                    if (good&1) *neartagsector = nextsector;
                    if (good&2) *neartagwall = z;
                    *neartaghitdist = dmulscale14(intx-xs,sintable[(ange+2560)&2047],inty-ys,sintable[(ange+2048)&2047]);
                    xe = intx;
                    ye = inty;
                    ze = intz;
                }
                if (nextsector >= 0)
                {
                    for(zz=tempshortnum-1; zz>=0; zz--)
                        if (clipsectorlist[zz] == nextsector) break;
                    if (zz < 0) clipsectorlist[tempshortnum++] = nextsector;
                }
            }
        }

        for(z=headspritesect[dasector]; z>=0; z=spr_add_data[z].nextspritesect)
        {

            good = 0;
            if ((tagsearch&1) && getSprite(z)->_lotag) good |= 1;
            if ((tagsearch&2) && getSprite(z)->_hitag) good |= 1;
            if (good != 0)
            {
                x1 = getSprite(z)->_x;
                y1 = getSprite(z)->_y;
                z1 = getSprite(z)->_z;

                topt = vx*(x1-xs) + vy*(y1-ys);
                if (topt > 0)
                {
                    bot = vx*vx + vy*vy;
                    if (bot != 0)
                    {
                        intz = zs+scale(vz,topt,bot);
                        i = tiles[getSprite(z)->_picnum].dim.height*getSprite(z)->_yrepeat;
                        if (getSprite(z)->_cstat&128)
                            z1 += (i<<1);
                        if (tiles[getSprite(z)->_picnum].animFlags&0x00ff0000)
                            z1 -= ((int32_t)((int8_t  )((tiles[getSprite(z)->_picnum].animFlags>>16)&255))*getSprite(z)->_yrepeat<<2);
                        if ((intz <= z1) && (intz >= z1-(i<<2)))
                        {
                            topu = vx*(y1-ys) - vy*(x1-xs);

                            offx = scale(vx,topu,bot);
                            offy = scale(vy,topu,bot);
                            dist = offx*offx + offy*offy;
                            i = (tiles[getSprite(z)->_picnum].dim.width*getSprite(z)->_xrepeat);
                            i *= i;
                            if (dist <= (i>>7))
                            {
                                intx = xs + scale(vx,topt,bot);
                                inty = ys + scale(vy,topt,bot);
                                if (klabs(intx-xs)+klabs(inty-ys) < klabs(xe-xs)+klabs(ye-ys))
                                {
                                    *neartagsprite = z;
                                    *neartaghitdist = dmulscale14(intx-xs,sintable[(ange+2560)&2047],inty-ys,sintable[(ange+2048)&2047]);
                                    xe = intx;
                                    ye = inty;
                                    ze = intz;
                                }
                            }
                        }
                    }
                }
            }
        }

        tempshortcnt++;
    } while (tempshortcnt < tempshortnum);
    return(0);
}

#if USE_BLOCKMAP
static inline void setModifiedSector(int sector)
{
   if (sector >= 0)
       sectorModifiedBf[sector/8] |= (1 << (sector % 8));
}
#endif
void dragpoint(short pointhighlight, int32_t dax, int32_t day)
{
    short cnt, tempshort;
    setWall_x(pointhighlight, dax);
    setWall_y(pointhighlight, day);
#if USE_BLOCKMAP
    setModifiedSector(wallSectorId[pointhighlight]);
#endif // USE_BLOCKMAP
    cnt = MAXWALLS;
    tempshort = pointhighlight;    /* search points CCW */
    do
    {
        if (constwall[tempshort].nextwall >= 0)
        {
            tempshort = getWall_point2(constwall[tempshort].nextwall);
            setWall_x(tempshort, dax);
            setWall_y(tempshort, day);
            #if USE_BLOCKMAP
                setModifiedSector(constwall[tempshort].nextsector);
            #endif // USE_BLOCKMAP

        }
        else
        {
            tempshort = pointhighlight;    /* search points CW if not searched all the way around */
            do
            {
                if (constwall[lastwall(tempshort)].nextwall >= 0)
                {
                    tempshort = constwall[lastwall(tempshort)].nextwall;
                    setWall_x(tempshort, dax);
                    setWall_y(tempshort, day);
                     #if USE_BLOCKMAP
                        setModifiedSector(constwall[tempshort].nextsector);
                    #endif // USE_BLOCKMAP

                }
                else
                {
                    break;
                }
                cnt--;
            }
            while ((tempshort != pointhighlight) && (cnt > 0));
            break;
        }
        cnt--;
    }
    while ((tempshort != pointhighlight) && (cnt > 0));
}


int lastwall(short point)
{
    int32_t i, j, cnt;

    if ((point > 0) && (getWall_point2(point-1) == point)) return(point-1);
    i = point;
    cnt = MAXWALLS;
    do
    {
        j = getWall_point2(i);
        if (j == point) return(i);
        i = j;
        cnt--;
    } while (cnt > 0);
    return(point);
}

static inline void addclipline(int dax1, int day1, int dax2, int day2, int daoval)
{
    #if WIN32
        if (clipnum >= MAXCLIPNUM)
        {
            FIXME("Too many cliplines");
            return;
        }
    #endif // WIN32
    clipit[clipnum].x1 = dax1; clipit[clipnum].y1 = day1;
    clipit[clipnum].x2 = dax2; clipit[clipnum].y2 = day2;
    clipobjectval[clipnum] = daoval;
    clipnum++;
}


static void keepaway (int32_t *x, int32_t *y, int32_t w)
{   // called only by clipmove

    int32_t dx, dy, ox, oy, x1, y1;
    uint8_t  first;

    x1 = clipit[w].x1;
    dx = clipit[w].x2-x1;
    y1 = clipit[w].y1;
    dy = clipit[w].y2-y1;
    ox = ksgn(-dy);
    oy = ksgn(dx);
    first = (klabs(dx) <= klabs(dy));
    while (1)
    {
        if (dx*(*y-y1) > (*x-x1)*dy) return;
        if (first == 0) *x += ox;
        else *y += oy;
        first ^= 1;
    }
}


static int raytrace(int32_t x3, int32_t y3, int32_t *x4, int32_t *y4)   // called only by clipmode
{
    int32_t x1, y1, x2, y2, bot, topu, nintx, ninty, cnt, z, hitwall;
    int32_t x21, y21, x43, y43;

    hitwall = -1;
    for(z=clipnum-1; z>=0; z--)
    {
        x1 = clipit[z].x1;
        x2 = clipit[z].x2;
        x21 = x2-x1;
        y1 = clipit[z].y1;
        y2 = clipit[z].y2;
        y21 = y2-y1;

        topu = x21*(y3-y1) - (x3-x1)*y21;
        if (topu <= 0) continue;
        if (x21*(*y4-y1) > (*x4-x1)*y21) continue;
        x43 = *x4-x3;
        y43 = *y4-y3;
        if (x43*(y1-y3) > (x1-x3)*y43) continue;
        if (x43*(y2-y3) <= (x2-x3)*y43) continue;
        bot = x43*y21 - x21*y43;
        if (bot == 0) continue;

        cnt = 256;
        do
        {
            cnt--;
            if (cnt < 0) {
                *x4 = x3;
                *y4 = y3;
                return(z);
            }
            nintx = x3 + scale(x43,topu,bot);
            ninty = y3 + scale(y43,topu,bot);
            topu--;
        } while (x21*(ninty-y1) <= (nintx-x1)*y21);

        if (klabs(x3-nintx)+klabs(y3-ninty) < klabs(x3-*x4)+klabs(y3-*y4))
        {
            *x4 = nintx;
            *y4 = ninty;
            hitwall = z;
        }
    }
    return(hitwall);
}


/* !!! ugh...move this var into clipmove as a parameter, and update build2.txt! */
int32_t clipmoveboxtracenum = 3;
int clipmove (int32_t *x, int32_t *y, int32_t *z, short *sectnum,
              int32_t xvect, int32_t yvect, int32_t walldist, int32_t ceildist,
              int32_t flordist, uint32_t  cliptype)
{
    short stack_clipobjectval[MAXCLIPNUM];
    clipobjectval = stack_clipobjectval;
    linetype stack_clipit[MAXCLIPNUM];
    clipit = stack_clipit;
    walltype *wal, *wal2;
//    const spritetype *spr;
    const sectortype *sec2;
    short clipsectorlist[MAXCLIPNUM];
    int32_t i, j, templong1, templong2;
    int32_t oxvect, oyvect, goalx, goaly, intx, inty, lx, ly, retval;
    int32_t k, l, clipsectcnt, startwall, endwall, cstat, dasect;
    int32_t x1, y1, x2, y2, cx, cy, rad, xmin, ymin, xmax, ymax, daz, daz2;
    int32_t bsz, dax, day, xoff, yoff, xspan, yspan, cosang, sinang, tilenum;
    int32_t xrepeat, yrepeat, gx, gy, dx, dy, dasprclipmask, dawalclipmask;
    int32_t hitwall, cnt, clipyou;

    if (((xvect|yvect) == 0) || (*sectnum < 0))
    {
        clipit = NULL;
        clipobjectval = NULL;
        return(0);
    }
    retval = 0;

    oxvect = xvect;
    oyvect = yvect;

    goalx = (*x) + (xvect>>14);
    goaly = (*y) + (yvect>>14);


    clipnum = 0;

    cx = (((*x)+goalx)>>1);
    cy = (((*y)+goaly)>>1);
    /* Extra walldist for sprites on sector lines */
    gx = goalx-(*x);
    gy = goaly-(*y);
    rad = nsqrtasm(gx*gx + gy*gy) + MAXCLIPDIST+walldist + 8;
    xmin = cx-rad;
    ymin = cy-rad;
    xmax = cx+rad;
    ymax = cy+rad;

    dawalclipmask = (cliptype&65535);        /* CLIPMASK0 = 0x00010001 */
    dasprclipmask = (cliptype>>16);          /* CLIPMASK1 = 0x01000040 */

    clipsectorlist[0] = (*sectnum);
    clipsectcnt = 0;
    clipsectnum = 1;
    do
    {
        dasect = clipsectorlist[clipsectcnt++];
        startwall = constsector[dasect].wallptr;
        endwall = startwall + constsector[dasect].wallnum;
        for(j=startwall,wal=getWall(startwall); j<endwall; j++,wal = getWall(j))
        {
            wal2 = getWall(constwall[j]._point2);
            if ((wal->x < xmin) && (wal2->x < xmin)) continue;
            if ((wal->x > xmax) && (wal2->x > xmax)) continue;
            if ((wal->y < ymin) && (wal2->y < ymin)) continue;
            if ((wal->y > ymax) && (wal2->y > ymax)) continue;

            x1 = wal->x;
            y1 = wal->y;
            x2 = wal2->x;
            y2 = wal2->y;

            dx = x2-x1;
            dy = y2-y1;
            if (dx*((*y)-y1) < ((*x)-x1)*dy) continue;  /* If wall's not facing you */

            if (dx > 0) dax = dx*(ymin-y1);
            else dax = dx*(ymax-y1);
            if (dy > 0) day = dy*(xmax-x1);
            else day = dy*(xmin-x1);
            if (dax >= day) continue;

            clipyou = 0;
            if ((constwall[j].nextsector < 0) || (wal->_cstat &dawalclipmask)) clipyou = 1;
            else if (editstatus == 0)
            {
                if (rintersect(*x,*y,0,gx,gy,0,x1,y1,x2,y2,&dax,&day,&daz) == 0)
                    dax = *x, day = *y;
                daz = getflorzofslope((short)dasect,dax,day);
                daz2 = getflorzofslope(constwall[j].nextsector,dax,day);

                sec2 = getSector(constwall[j].nextsector);
                if (daz2 < daz-(1<<8))
                    if ((sec2->floorstat&1) == 0)
                        if ((*z) >= daz2-(flordist-1)) clipyou = 1;
                if (clipyou == 0)
                {
                    daz = getceilzofslope((short)dasect,dax,day);
                    daz2 = getceilzofslope(constwall[j].nextsector,dax,day);
                    if (daz2 > daz+(1<<8))
                        if ((sec2->ceilingstat&1) == 0)
                            if ((*z) <= daz2+(ceildist-1)) clipyou = 1;
                }
            }

            if (clipyou)
            {
                /* Add 2 boxes at endpoints */
                bsz = walldist;
                if (gx < 0) bsz = -bsz;
                addclipline(x1-bsz,y1-bsz,x1-bsz,y1+bsz,(short)j+32768);
                addclipline(x2-bsz,y2-bsz,x2-bsz,y2+bsz,(short)j+32768);
                bsz = walldist;
                if (gy < 0) bsz = -bsz;
                addclipline(x1+bsz,y1-bsz,x1-bsz,y1-bsz,(short)j+32768);
                addclipline(x2+bsz,y2-bsz,x2-bsz,y2-bsz,(short)j+32768);

                dax = walldist;
                if (dy > 0) dax = -dax;
                day = walldist;
                if (dx < 0) day = -day;
                addclipline(x1+dax,y1+day,x2+dax,y2+day,(short)j+32768);
            }
            else
            {
                for(i=clipsectnum-1; i>=0; i--)
                    if (constwall[j].nextsector == clipsectorlist[i]) break;
                if (i < 0) clipsectorlist[clipsectnum++] = constwall[j].nextsector;
            }
        }

        for(j=headspritesect[dasect]; j>=0; j=spr_add_data[j].nextspritesect)
        {
            const spritetype *sprj = getSprite(j);

            sprite_additional_data *spr_d = &spr_add_data[j];
            cstat = sprj->_cstat;
            if ((cstat&dasprclipmask) == 0) continue;
            x1 = sprj->_x;
            y1 = sprj->_y;
            switch(cstat&48)
            {
            case 0:
                if ((x1 >= xmin) && (x1 <= xmax) && (y1 >= ymin) && (y1 <= ymax))
                {
                    k = ((tiles[sprj->_picnum].dim.height*sprj->_yrepeat)<<2);
                    if (cstat&128)
                        daz = sprj->_z+(k>>1);
                    else
                        daz = sprj->_z;

                    if (tiles[sprj->_picnum].animFlags&0x00ff0000)
                        daz -= ((int32_t)((int8_t )((tiles[sprj->_picnum].animFlags>>16)&255))*sprj->_yrepeat<<2);

                    if (((*z) < daz+ceildist) && ((*z) > daz-k-flordist)){
                        bsz = (spr_d->clipdist<<2)+walldist;
                        if (gx < 0) bsz = -bsz;
                        addclipline(x1-bsz,y1-bsz,x1-bsz,y1+bsz,(short)j+49152);
                        bsz = (spr_d->clipdist<<2)+walldist;
                        if (gy < 0) bsz = -bsz;
                        addclipline(x1+bsz,y1-bsz,x1-bsz,y1-bsz,(short)j+49152);
                    }
                }
                break;
            case 16:
                k = ((tiles[sprj->_picnum].dim.height*sprj->_yrepeat)<<2);

                if (cstat&128)
                    daz = sprj->_z+(k>>1);
                else
                    daz = sprj->_z;

                if (tiles[sprj->_picnum].animFlags&0x00ff0000)
                    daz -= ((int32_t)((int8_t  )((tiles[sprj->_picnum].animFlags>>16)&255))*sprj->_yrepeat<<2);
                daz2 = daz-k;
                daz += ceildist;
                daz2 -= flordist;
                if (((*z) < daz) && ((*z) > daz2))
                {
                    /*
                     * These lines get the 2 points of the rotated sprite
                     * Given: (x1, y1) starts out as the center point
                     */
                    tilenum = sprj->_picnum;
                    xoff = (int32_t)((int8_t  )((tiles[tilenum].animFlags>>8)&255))+((int32_t)sprj->_xoffset);
                    if ((cstat&4) > 0) xoff = -xoff;
                    k = sprj->_ang;
                    l = sprj->_xrepeat;
                    dax = sintable[k&2047]*l;
                    day = sintable[(k+1536)&2047]*l;
                    l = tiles[tilenum].dim.width;
                    k = (l>>1)+xoff;
                    x1 -= mulscale16(dax,k);
                    x2 = x1+mulscale16(dax,l);
                    y1 -= mulscale16(day,k);
                    y2 = y1+mulscale16(day,l);
                    if (clipinsideboxline(cx,cy,x1,y1,x2,y2,rad) != 0)
                    {
                        dax = mulscale14(sintable[(sprj->_ang+256+512)&2047],walldist);
                        day = mulscale14(sintable[(sprj->_ang+256)&2047],walldist);

                        if ((x1-(*x))*(y2-(*y)) >= (x2-(*x))*(y1-(*y)))   /* Front */
                        {
                            addclipline(x1+dax,y1+day,x2+day,y2-dax,(short)j+49152);
                        }
                        else
                        {
                            if ((cstat&64) != 0) continue;
                            addclipline(x2-dax,y2-day,x1-day,y1+dax,(short)j+49152);
                        }

                        /* Side blocker */
                        if ((x2-x1)*((*x)-x1) + (y2-y1)*((*y)-y1) < 0)
                        {
                            addclipline(x1-day,y1+dax,x1+dax,y1+day,(short)j+49152);
                        }
                        else if ((x1-x2)*((*x)-x2) + (y1-y2)*((*y)-y2) < 0)
                        {
                            addclipline(x2+day,y2-dax,x2-dax,y2-day,(short)j+49152);
                        }
                    }
                }
                break;
            case 32:
                daz = sprj->_z+ceildist;
                daz2 = sprj->_z-flordist;
                if (((*z) < daz) && ((*z) > daz2))
                {
                  int32_t rxi[4], ryi[4]; // next-hack: moved here.
                  if ((cstat&64) != 0)
                        if (((*z) > sprj->_z) == ((cstat&8)==0)) continue;

                    tilenum = sprj->_picnum;
                    xoff = (int32_t)((int8_t  )((tiles[tilenum].animFlags>>8)&255))+((int32_t)sprj->_xoffset);
                    yoff = (int32_t)((int8_t  )((tiles[tilenum].animFlags>>16)&255))+((int32_t)sprj->_yoffset);
                    if ((cstat&4) > 0) xoff = -xoff;
                    if ((cstat&8) > 0) yoff = -yoff;

                    k = sprj->_ang;
                    cosang = sintable[(k+512)&2047];
                    sinang = sintable[k];
                    xspan = tiles[tilenum].dim.width;
                    xrepeat = sprj->_xrepeat;
                    yspan = tiles[tilenum].dim.height;
                    yrepeat = sprj->_yrepeat;

                    dax = ((xspan>>1)+xoff)*xrepeat;
                    day = ((yspan>>1)+yoff)*yrepeat;
                    rxi[0] = x1 + dmulscale16(sinang,dax,cosang,day);
                    ryi[0] = y1 + dmulscale16(sinang,day,-cosang,dax);
                    l = xspan*xrepeat;
                    rxi[1] = rxi[0] - mulscale16(sinang,l);
                    ryi[1] = ryi[0] + mulscale16(cosang,l);
                    l = yspan*yrepeat;
                    k = -mulscale16(cosang,l);
                    rxi[2] = rxi[1]+k;
                    rxi[3] = rxi[0]+k;
                    k = -mulscale16(sinang,l);
                    ryi[2] = ryi[1]+k;
                    ryi[3] = ryi[0]+k;

                    dax = mulscale14(sintable[(sprj->_ang-256+512)&2047],walldist);
                    day = mulscale14(sintable[(sprj->_ang-256)&2047],walldist);

                    if ((rxi[0]-(*x))*(ryi[1]-(*y)) < (rxi[1]-(*x))*(ryi[0]-(*y)))
                    {
                        if (clipinsideboxline(cx,cy,rxi[1],ryi[1],rxi[0],ryi[0],rad) != 0)
                            addclipline(rxi[1]-day,ryi[1]+dax,rxi[0]+dax,ryi[0]+day,(short)j+49152);
                    }
                    else if ((rxi[2]-(*x))*(ryi[3]-(*y)) < (rxi[3]-(*x))*(ryi[2]-(*y)))
                    {
                        if (clipinsideboxline(cx,cy,rxi[3],ryi[3],rxi[2],ryi[2],rad) != 0)
                            addclipline(rxi[3]+day,ryi[3]-dax,rxi[2]-dax,ryi[2]-day,(short)j+49152);
                    }

                    if ((rxi[1]-(*x))*(ryi[2]-(*y)) < (rxi[2]-(*x))*(ryi[1]-(*y)))
                    {
                        if (clipinsideboxline(cx,cy,rxi[2],ryi[2],rxi[1],ryi[1],rad) != 0)
                            addclipline(rxi[2]-dax,ryi[2]-day,rxi[1]-day,ryi[1]+dax,(short)j+49152);
                    }
                    else if ((rxi[3]-(*x))*(ryi[0]-(*y)) < (rxi[0]-(*x))*(ryi[3]-(*y)))
                    {
                        if (clipinsideboxline(cx,cy,rxi[0],ryi[0],rxi[3],ryi[3],rad) != 0)
                            addclipline(rxi[0]+dax,ryi[0]+day,rxi[3]+day,ryi[3]-dax,(short)j+49152);
                    }
                }
                break;
            }
        }
    } while (clipsectcnt < clipsectnum);


    hitwall = 0;
    cnt = clipmoveboxtracenum;
    do
    {
        intx = goalx;
        inty = goaly;
        if ((hitwall = raytrace(*x, *y, &intx, &inty)) >= 0)
        {
            lx = clipit[hitwall].x2-clipit[hitwall].x1;
            ly = clipit[hitwall].y2-clipit[hitwall].y1;
            templong2 = lx*lx + ly*ly;
            if (templong2 > 0)
            {
                templong1 = (goalx-intx)*lx + (goaly-inty)*ly;

                if ((klabs(templong1)>>11) < templong2)
                    i = divscale20(templong1,templong2);
                else
                    i = 0;
                goalx = mulscale20(lx,i)+intx;
                goaly = mulscale20(ly,i)+inty;
            }

            templong1 = dmulscale6(lx,oxvect,ly,oyvect);
            for(i=cnt+1; i<=clipmoveboxtracenum; i++)
            {
                j = hitwalls[i];
                templong2 = dmulscale6(clipit[j].x2-clipit[j].x1,oxvect,clipit[j].y2-clipit[j].y1,oyvect);
                if ((templong1^templong2) < 0)
                {
                    updatesector(*x,*y,sectnum);
                    clipit = NULL;
                    return(retval);
                }
            }

            keepaway(&goalx, &goaly, hitwall);
            xvect = ((goalx-intx)<<14);
            yvect = ((goaly-inty)<<14);

            if (cnt == clipmoveboxtracenum) retval = clipobjectval[hitwall];
            hitwalls[cnt] = hitwall;
        }
        cnt--;

        *x = intx;
        *y = inty;
    } while (((xvect|yvect) != 0) && (hitwall >= 0) && (cnt > 0));

    for(j=0; j<clipsectnum; j++)
        if (inside(*x,*y,clipsectorlist[j]) == 1)
        {
            *sectnum = clipsectorlist[j];
            clipit = NULL;
            clipobjectval = NULL;
            return(retval);
        }

    *sectnum = -1;
    templong1 = 0x7fffffff;
    for(j=numsectors-1; j>=0; j--)
    {
        const sectortype *secj = getSector(j);
        if (inside(*x,*y,j) == 1)
        {
            if (secj->ceilingstat&2)
                templong2 = (getceilzofslope((short)j,*x,*y)-(*z));
            else
                templong2 = (secj->ceilingz-(*z));

            if (templong2 > 0)
            {
                if (templong2 < templong1)
                {
                    *sectnum = j;
                    templong1 = templong2;
                }
            }
            else
            {
                if (secj->floorstat&2)
                    templong2 = ((*z)-getflorzofslope((short)j,*x,*y));
                else
                    templong2 = ((*z)-secj->floorz);

                if (templong2 <= 0)
                {
                    *sectnum = j;
                    clipit = NULL;
                    return(retval);
                }
                if (templong2 < templong1)
                {
                    *sectnum = j;
                    templong1 = templong2;
                }
            }
        }
    }
    clipobjectval = NULL;
    clipit = NULL;
    return(retval);
}


int pushmove(int32_t *x, int32_t *y, int32_t *z, short *sectnum,
             int32_t walldist, int32_t ceildist, int32_t flordist,
             uint32_t  cliptype)
{
    short clipsectorlist[MAXCLIPNUM];
    const sectortype *sec2;
    walltype *wal;
    int32_t i, j, k, t, dx, dy, dax, day, daz, daz2, bad, dir;
//    int32_t dasprclipmask;
    int32_t dawalclipmask;
    short startwall, endwall, clipsectcnt;
    uint8_t  bad2;

    if ((*sectnum) < 0) return(-1);

    dawalclipmask = (cliptype&65535);
//    dasprclipmask = (cliptype>>16);

    k = 32;
    dir = 1;
    do
    {
        bad = 0;

        clipsectorlist[0] = *sectnum;
        clipsectcnt = 0;
        clipsectnum = 1;
        do
        {
            if (dir > 0)
                startwall = constsector[clipsectorlist[clipsectcnt]].wallptr, endwall = startwall + constsector[clipsectorlist[clipsectcnt]].wallnum;
            else
                endwall = constsector[clipsectorlist[clipsectcnt]].wallptr, startwall = endwall + constsector[clipsectorlist[clipsectcnt]].wallnum;

            for(i=startwall,wal=getWall(startwall); i!=endwall; i+=dir,wal = getWall(i))
                if (clipinsidebox(*x,*y,i,walldist-4) == 1)
                {
                    j = 0;
                    if (constwall[i].nextsector < 0) j = 1;
                    if (getWall_cstat(i)&dawalclipmask) j = 1;
                    if (j == 0)
                    {
                        sec2 = getSector(constwall[i].nextsector);


                        /* Find closest point on wall (dax, day) to (*x, *y) */
                        dax = getWall(getWall_point2(i))->x-wal->x;
                        day = getWall(getWall_point2(i))->y-wal->y;
                        daz = dax*((*x)-wal->x) + day*((*y)-wal->y);
                        if (daz <= 0)
                            t = 0;
                        else
                        {
                            daz2 = dax*dax+day*day;
                            if (daz >= daz2) t = (1<<30);
                            else t = divscale30(daz,daz2);
                        }
                        dax = wal->x + mulscale30(dax,t);
                        day = wal->y + mulscale30(day,t);


                        daz = getflorzofslope(clipsectorlist[clipsectcnt],dax,day);
                        daz2 = getflorzofslope(constwall[i].nextsector,dax,day);
                        if ((daz2 < daz-(1<<8)) && ((sec2->floorstat&1) == 0))
                            if (*z >= daz2-(flordist-1)) j = 1;

                        daz = getceilzofslope(clipsectorlist[clipsectcnt],dax,day);
                        daz2 = getceilzofslope(constwall[i].nextsector,dax,day);
                        if ((daz2 > daz+(1<<8)) && ((sec2->ceilingstat&1) == 0))
                            if (*z <= daz2+(ceildist-1)) j = 1;
                    }
                    if (j != 0)
                    {
                        j = getangle(getWall(getWall_point2(i))->x-wal->x,getWall(getWall_point2(i))->y-wal->y);
                        dx = (sintable[(j+1024)&2047]>>11);
                        dy = (sintable[(j+512)&2047]>>11);
                        bad2 = 16;
                        do
                        {
                            *x = (*x) + dx;
                            *y = (*y) + dy;
                            bad2--;
                            if (bad2 == 0) break;
                        } while (clipinsidebox(*x,*y,i,walldist-4) != 0);
                        bad = -1;
                        k--;
                        if (k <= 0) return(bad);
                        updatesector(*x,*y,sectnum);
                    }
                    else
                    {
                        for(j=clipsectnum-1; j>=0; j--)
                            if (constwall[i].nextsector == clipsectorlist[j]) break;
                        if (j < 0) clipsectorlist[clipsectnum++] = constwall[i].nextsector;
                    }
                }

            clipsectcnt++;
        } while (clipsectcnt < clipsectnum);
        dir = -dir;
    } while (bad != 0);

    return(bad);
}

/*
 FCS:  x and y are the new position of the entity that has just moved:
 lastKnownSector is an hint (the last known sectorID of the entity).

 Thanks to the "hint", the algorithm check:
 1. Is (x,y) inside sectors[sectnum].
 2. Flood in sectnum portal and check again if (x,y) is inside.
 3. Do a linear search on sectors[sectnum] from 0 to numSectors.

 Note: Inside uses cross_product and return as soon as the point switch
 from one side to the other.

 next-hack: linear search is extremely slow, especially in the case when
 the sprite is in the first sectors or it is even outside the map. The
 algorithm is O(numwalls). Also we did our good part in rendering
 this job even slower, because of wall getters.
 To improve this, we have brought the blockmap. If the sprite is not found
 in the current or neighboring sectors, we use blockmap to know which sectors we should look at.
 */
void updatesector(int32_t x, int32_t y, short *lastKnownSector)
{
    //First check the last sector where (old_x,old_y) was before being updated to (x,y)
    if (inside(x,y,*lastKnownSector) == 1)
    {
        //We found it and (x,y) is still in the same sector: nothing to update !
        return;
    }
    int32_t i, j;
    // Seems (x,y) moved into an other sector....hopefully one connected via a portal. Let's flood in each portal.
    if ((*lastKnownSector >= 0) && (*lastKnownSector < numsectors))
    {
        int wallIdx = constsector[*lastKnownSector].wallptr;
        j = constsector[*lastKnownSector].wallnum;
        do
        {
            i = constwall[wallIdx].nextsector;
            if (i >= 0)
            {
                if (inside(x,y,(short)i) == 1)
                {
                    *lastKnownSector = i;
                    return;
                }
            }
            wallIdx++;
            j--;
        } while (j != 0);
    }
#if USE_BLOCKMAP
    // prematurely exit if outside
    if (x < pblockmapData->bb.xmin || y < pblockmapData->bb.ymin || x >= pblockmapData->bb.xmax || y >= pblockmapData->bb.ymax )
    {
        *lastKnownSector = -1;
        return;
    }
    // get the position
    int deltax = x - pblockmapData->bb.xmin;
    int deltay = y - pblockmapData->bb.ymin;
    // get the block
    int bx = deltax / pblockmapData->deltax;
    int by = deltay / pblockmapData->deltay;
    int16_t *pbm = pblockmapData->blockmap[by][bx];
    // note: unfortunately, the updatesector is order-sensitive.
    // this might come out because there might be sectors inside other sectors.
    //
    // assume we did not find the sprite:
    *lastKnownSector = -1;
    if (pbm)
    {
        // if there is block map data, there was at least 1 sector in that block
        do
        {
            if (inside(x, y, *pbm))
            {
                *lastKnownSector = *pbm;
                break;
            }
            pbm++;
        } while(*pbm != -1);  // scan until there is a sector.
    }

    // Now, we either have found a sector where the sprite might be. Or we have not found at all (-1)
    // So we now search through all the modified sectors, up to the last one we found previously.
    int minSectorToCheck = *lastKnownSector;
    for (int si = numsectors - 1; si > minSectorToCheck; si--)
    {
        int byte_index = si / 8;        // do not worry. The compiler translates
        int bit_index = si % 8;         // these two instructions as si >> 3 and si & 7
        // skip 8 if all 0
        if (sectorModifiedBf[byte_index] == 0)
        {
            // Skip the entire byte if it is 0
            si -= bit_index; // Move to the next byte
            continue;
        }

        if (sectorModifiedBf[byte_index] & (1 << bit_index))
        {
            if (inside(x, y, si))
            {
                *lastKnownSector = si;
                break;
            }
        }
    }
    // note: if the sprite is in no sector, it is already set to -1
    return;
#else

    //Damn that is a BIG move, still cannot find which sector (x,y) belongs to. Let's search via linear search.
    for(i=numsectors-1; i>=0; i--)
    {
        if (inside(x,y,(short)i) == 1)
        {
            *lastKnownSector = i;
            return;
        }
    }
#if WIN32
    printf("(%d, %d) is outside the map\r\n", x, y);
#endif
    // (x,y) is contained in NO sector. (x,y) is likely out of the map. next-hack: or inside walls.
    *lastKnownSector = -1;
#endif // USE_BLOCKMAP
}


void rotatepoint(int32_t xpivot, int32_t ypivot, int32_t x, int32_t y, short daang, int32_t *x2, int32_t *y2)
{
    int32_t dacos, dasin;

    dacos = sintable[(daang+2560)&2047];
    dasin = sintable[(daang+2048)&2047];
    x -= xpivot;
    y -= ypivot;
    *x2 = dmulscale14(x,dacos,-y,dasin) + xpivot;
    *y2 = dmulscale14(y,dacos,x,dasin) + ypivot;
}


int initmouse(void)
{
    return(moustat = setupmouse());
}


void getmousevalues(short *mousx, short *mousy, short *bstatus)
{
    if (moustat == 0) {
        *mousx = 0;
        *mousy = 0;
        *bstatus = 0;
        return;
    }
    readmousexy(mousx,mousy);
    readmousebstatus(bstatus);
}
#if 0
void draw2dgrid(int32_t posxe, int32_t posye, short ange, int32_t zoome, short gride)
{
    int32_t i, xp1, yp1, xp2=0, yp2, tempy; // tempint;
    uint8_t  mask;

    if (gride > 0)
    {
        yp1 = 200-mulscale14(posye+131072,zoome);
        if (yp1 < 0) yp1 = 0;
        yp2 = 200-mulscale14(posye-131072,zoome);
        if (yp2 >= ydim16) yp2 = ydim16-1;

        if ((yp1 < ydim16) && (yp2 >= 0) && (yp2 >= yp1))
        {
            setcolor16(8);

//            tempint = ((yp1*640+pageoffset)>>3)+(int32_t)_getVideoBase();
            tempy = yp2-yp1+1;
            mask = 0;
            xp1 = 320-mulscale14(posxe+131072,zoome);

            for(i=-131072; i<=131072; i+=(2048>>gride))
            {
                xp2 = xp1;
                xp1 = 320-mulscale14(posxe-i,zoome);

                if (xp1 >= 640) break;
                if (xp1 >= 0)
                {


                    drawline16(xp1, 0, xp1, tempy, 8);
                }
            }
            if ((i >= 131072) && (xp1 < 640))
                xp2 = xp1;
            if ((mask != 0) && ((xp2>>3) >= 0) && ((xp2>>3) < 80))
            {
                /* !!! Does this code ever get hit? Do something with this! */
          //      fprintf (stderr, "STUB: %s:%d\n",__FILE__,__LINE__);
            }
        }

        xp1 = mulscale14(posxe+131072,zoome);
        xp2 = mulscale14(posxe-131072,zoome);
        tempy = 0x80000000;
        for(i=-131072; i<=131072; i+=(2048>>gride))
        {
            yp1 = (((posye-i)*zoome)>>14);
            if (yp1 != tempy)
            {
                if ((yp1 > 200-ydim16) && (yp1 <= 200))
                {
                    drawline16(320-xp1,200-yp1,320-xp2,200-yp1,8);
                    tempy = yp1;
                }
            }
        }
    }
}

void draw2dscreen(int32_t posxe, int32_t posye, short ange, int32_t zoome, short gride)
{
    walltype *wal;
    int32_t i, j, xp1, yp1, xp2, yp2, tempint;
    uint8_t  col;

    if (qsetmode == 200) return;

    if (editstatus == 0)
    {
#if FREQUENT_FAKETIMERHANDLER
        faketimerhandler();
#endif
        clear2dscreen();
#if FREQUENT_FAKETIMERHANDLER
        faketimerhandler();
#endif
        draw2dgrid(posxe,posye,ange,zoome,gride);
    }
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
    for(i=numwalls-1,wal=getWall(i); i>=0; i--,wal = getWall(i))
    {
        if (editstatus == 0)
        {
            if ((show2dwall[i>>3]&pow2char[i&7]) == 0) continue;
            j = constwall[i].nextwall;
            if ((j >= 0) && (i > j))
                if ((show2dwall[j>>3]&pow2char[j&7]) > 0) continue;
        }
        else
        {
            j = constwall[i].nextwall;
            if ((j >= 0) && (i > j)) continue;
        }

        if (j < 0)
        {
            col = 7;
            if (i == linehighlight) col += ((numframes&2)<<2);
        }
        else
        {
            col = 4;
            if ((getWall_cstat(i)&1) != 0) col = 5;
            if ((i == linehighlight) || ((linehighlight >= 0) && (i == constwall[linehighlight].nextwall)))
                col += ((numframes&2)<<2);
        }

        xp1 = mulscale14(wal->x-posxe,zoome);
        yp1 = mulscale14(wal->y-posye,zoome);
        xp2 = mulscale14(getWall(getWall_point2(i))->x-posxe,zoome);
        yp2 = mulscale14(getWall(getWall_point2(i))->y-posye,zoome);

        if ((getWall_cstat(i)&64) > 0)
        {
            if (klabs(xp2-xp1) >= klabs(yp2-yp1))
            {
                drawline16(320+xp1,200+yp1+1,320+xp2,200+yp2+1,col);
                drawline16(320+xp1,200+yp1-1,320+xp2,200+yp2-1,col);
            }
            else
            {
                drawline16(320+xp1+1,200+yp1,320+xp2+1,200+yp2,col);
                drawline16(320+xp1-1,200+yp1,320+xp2-1,200+yp2,col);
            }
            col += 8;
        }
        drawline16(320+xp1,200+yp1,320+xp2,200+yp2,col);

        if ((zoome >= 256) && (editstatus == 1))
            if (((320+xp1) >= 2) && ((320+xp1) <= 637))
                if (((200+yp1) >= 2) && ((200+yp1) <= ydim16-3))
                {
                    col = 2;
                    if (i == pointhighlight) col += ((numframes&2)<<2);
                    else if ((highlightcnt > 0) && (editstatus == 1))
                    {
                        if (show2dwall[i>>3]&pow2char[i&7])
                            col += ((numframes&2)<<2);
                    }


                    tempint = (mul5(200+yp1)<<7)+(320+xp1);

                    setcolor16(col);

                    drawpixel16(tempint-2-1280);
                    drawpixel16(tempint-1-1280);
                    drawpixel16(tempint+0-1280);
                    drawpixel16(tempint+1-1280);
                    drawpixel16(tempint+2-1280);

                    drawpixel16(tempint-2+1280);
                    drawpixel16(tempint-1+1280);
                    drawpixel16(tempint+0+1280);
                    drawpixel16(tempint+1+1280);
                    drawpixel16(tempint+2+1280);

                    drawpixel16(tempint-2-640);
                    drawpixel16(tempint-2+0);
                    drawpixel16(tempint-2+640);

                    drawpixel16(tempint+2-640);
                    drawpixel16(tempint+2+0);
                    drawpixel16(tempint+2+640);
                }
    }
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
    if ((zoome >= 256) || (editstatus == 0))
        for(i=0; i<numsectors; i++)
            for(j=headspritesect[i]; j>=0; j=spr_add_data[j].nextspritesect)
                if ((editstatus == 1) || (show2dsprite[j>>3]&pow2char[j&7]))
                {
                    col = 3;
                    if ((getSprite(j)->_cstat&1) > 0) col = 5;
                    if (editstatus == 1)
                    {
                        if (j+16384 == pointhighlight)
                            col += ((numframes&2)<<2);
                        else if ((highlightcnt > 0) && (editstatus == 1))
                        {
                            if (show2dsprite[j>>3]&pow2char[j&7])
                                col += ((numframes&2)<<2);
                        }
                    }

                    xp1 = mulscale14(getSprite(j)->_x-posxe,zoome);
                    yp1 = mulscale14(getSprite(j)->_y-posye,zoome);
                    if (((320+xp1) >= 2) && ((320+xp1) <= 637))
                        if (((200+yp1) >= 2) && ((200+yp1) <= ydim16-3))
                        {

                            tempint = (mul5(200+yp1)<<7)+(320+xp1);


                            setcolor16(col);
                            drawpixel16(tempint-1-1280);
                            drawpixel16(tempint+0-1280);
                            drawpixel16(tempint+1-1280);

                            drawpixel16(tempint-1+1280);
                            drawpixel16(tempint+0+1280);
                            drawpixel16(tempint+1+1280);

                            drawpixel16(tempint-2-640);
                            drawpixel16(tempint-2+0);
                            drawpixel16(tempint-2+640);

                            drawpixel16(tempint+2-640);
                            drawpixel16(tempint+2+0);
                            drawpixel16(tempint+2+640);

                            drawpixel16(tempint+1+640);
                            drawpixel16(tempint-1+640);
                            drawpixel16(tempint+1-640);
                            drawpixel16(tempint-1-640);

                            xp2 = mulscale11(sintable[(getSprite(j)->_ang+2560)&2047],zoome) / 768;
                            yp2 = mulscale11(sintable[(getSprite(j)->_ang+2048)&2047],zoome) / 768;

                            if ((getSprite(j)->_cstat&256) > 0)
                            {
                                if (((getSprite(j)->_ang+256)&512) == 0)
                                {
                                    drawline16(320+xp1,200+yp1-1,320+xp1+xp2,200+yp1+yp2-1,col);
                                    drawline16(320+xp1,200+yp1+1,320+xp1+xp2,200+yp1+yp2+1,col);
                                }
                                else
                                {
                                    drawline16(320+xp1-1,200+yp1,320+xp1+xp2-1,200+yp1+yp2,col);
                                    drawline16(320+xp1+1,200+yp1,320+xp1+xp2+1,200+yp1+yp2,col);
                                }
                                col += 8;
                            }
                            drawline16(320+xp1,200+yp1,320+xp1+xp2,200+yp1+yp2,col);
                        }
                }
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
    xp1 = mulscale11(sintable[(ange+2560)&2047],zoome) / 768; /* Draw white arrow */
    yp1 = mulscale11(sintable[(ange+2048)&2047],zoome) / 768;
    drawline16(320+xp1,200+yp1,320-xp1,200-yp1,15);
    drawline16(320+xp1,200+yp1,320+yp1,200-xp1,15);
    drawline16(320+xp1,200+yp1,320-yp1,200+xp1,15);
}
#endif
#if PRINT_COORDS_SUPPORT

/*
 * This is ryan's change. SDL requires me to call SDL_UpdateRect() to force
 *  vid updates without a SDL_Flip() call, but there's no such thing in the
 *  DOS version of this program, so text won't show up sometimes without
 *  my update call in Linux.  However, to get a nice shadow effect on some
 *  text, Ken draws a string at an offset and darker, and then on top of it
 *  draws the actual string. Two SDL_UpdateRect() calls in over top of each
 *  other cause flicker, so I have this function here so the shadow can
 *  be drawn with _noupdate, and the actual string is draw with an update.
 */
void printext256(int32_t xpos, int32_t ypos, short col, short backcol, char*  name, uint8_t  fontsize)
{
    int32_t stx, i, x, y, charxsiz;
    const uint8_t  *fontptr;
    uint8_t *letptr, *ptr;



    stx = xpos;

    if (fontsize) {
        fontptr = smalltextfont;
        charxsiz = 4;
    }
    else {
        fontptr = textfont;
        charxsiz = 8;
    }

    //For each character in the string.
    for(i=0; name[i]; i++)
    {
        letptr = &fontptr[name[i]<<3];
        ptr = ylookup(ypos+7)+(stx-fontsize)+frameplace;
        for(y=7; y>=0; y--)
        {
            for(x=charxsiz-1; x>=0; x--)
            {
                if (letptr[y]&pow2char[7-fontsize-x])
                    ptr[x] = (uint8_t )col;
                else if (backcol >= 0)
                    ptr[x] = (uint8_t )backcol;
            }
            ptr -= ylookup(1);
        }
        stx += charxsiz;
    }

    _updateScreenRect(xpos, ypos, charxsiz * i, 8);
}
#endif
int krand()
{
    randomseed = (randomseed*27584621)+1;
    return(((uint32_t )randomseed)>>16);
}


void getzrange(int32_t x, int32_t y, int32_t z, short sectnum,
               int32_t *ceilz, int32_t *ceilhit, int32_t *florz, int32_t *florhit,
               int32_t walldist, uint32_t  cliptype)
{
    short clipsectorlist[MAXCLIPNUM];
    const sectortype *sec;
    walltype *wal, *wal2;
//    const spritetype *spr;
    int32_t clipsectcnt, startwall, endwall, tilenum, xoff, yoff, dax, day;
    int32_t xmin, ymin, xmax, ymax, i, j, k, l, daz, daz2, dx, dy;
    int32_t x1, y1, x2, y2, x3, y3, x4, y4, ang, cosang, sinang;
    int32_t xspan, yspan, xrepeat, yrepeat, dasprclipmask, dawalclipmask;
    short cstat;
    uint8_t  clipyou;

    if (sectnum < 0)
    {
        *ceilz = 0x80000000;
        *ceilhit = -1;
        *florz = 0x7fffffff;
        *florhit = -1;
        return;
    }

    /* Extra walldist for sprites on sector lines */
    i = walldist+MAXCLIPDIST+1;
    xmin = x-i;
    ymin = y-i;
    xmax = x+i;
    ymax = y+i;

    getzsofslope(sectnum,x,y,ceilz,florz);
    *ceilhit = sectnum+16384;
    *florhit = sectnum+16384;

    dawalclipmask = (cliptype&65535);
    dasprclipmask = (cliptype>>16);

    clipsectorlist[0] = sectnum;
    clipsectcnt = 0;
    clipsectnum = 1;

    do  /* Collect sectors inside your square first */
    {
        sec = getSector(clipsectorlist[clipsectcnt]);
        startwall = constsector[clipsectorlist[clipsectcnt]].wallptr;
        endwall = startwall + constsector[clipsectorlist[clipsectcnt]].wallnum;
        for(j=startwall,wal=getWall(startwall); j<endwall; j++,wal = getWall(j))
        {
            k = constwall[j].nextsector;
            if (k >= 0)
            {
                wal2 = getWall(getWall_point2(j));
                x1 = wal->x;
                x2 = wal2->x;
                if ((x1 < xmin) && (x2 < xmin)) continue;
                if ((x1 > xmax) && (x2 > xmax)) continue;
                y1 = wal->y;
                y2 = wal2->y;
                if ((y1 < ymin) && (y2 < ymin)) continue;
                if ((y1 > ymax) && (y2 > ymax)) continue;

                dx = x2-x1;
                dy = y2-y1;
                if (dx*(y-y1) < (x-x1)*dy) continue; /* back */
                if (dx > 0) dax = dx*(ymin-y1);
                else dax = dx*(ymax-y1);
                if (dy > 0) day = dy*(xmax-x1);
                else day = dy*(xmin-x1);
                if (dax >= day) continue;

                if (getWall_cstat(j)&dawalclipmask) continue;
                sec = getSector(k);
                if (editstatus == 0)
                {
                    if (((sec->ceilingstat&1) == 0) && (z <= sec->ceilingz+(3<<8))) continue;
                    if (((sec->floorstat&1) == 0) && (z >= sec->floorz-(3<<8))) continue;
                }

                for(i=clipsectnum-1; i>=0; i--) if (clipsectorlist[i] == k) break;
                if (i < 0) clipsectorlist[clipsectnum++] = k;

                if ((x1 < xmin+MAXCLIPDIST) && (x2 < xmin+MAXCLIPDIST)) continue;
                if ((x1 > xmax-MAXCLIPDIST) && (x2 > xmax-MAXCLIPDIST)) continue;
                if ((y1 < ymin+MAXCLIPDIST) && (y2 < ymin+MAXCLIPDIST)) continue;
                if ((y1 > ymax-MAXCLIPDIST) && (y2 > ymax-MAXCLIPDIST)) continue;
                if (dx > 0) dax += dx*MAXCLIPDIST;
                else dax -= dx*MAXCLIPDIST;
                if (dy > 0) day -= dy*MAXCLIPDIST;
                else day += dy*MAXCLIPDIST;
                if (dax >= day) continue;

                /* It actually got here, through all the continue's! */
                getzsofslope((short)k,x,y,&daz,&daz2);
                if (daz > *ceilz) {
                    *ceilz = daz;
                    *ceilhit = k+16384;
                }
                if (daz2 < *florz) {
                    *florz = daz2;
                    *florhit = k+16384;
                }
            }
        }
        clipsectcnt++;
    } while (clipsectcnt < clipsectnum);

    for(i=0; i<clipsectnum; i++)
    {
        for(j=headspritesect[clipsectorlist[i]]; j>=0; j=spr_add_data[j].nextspritesect)
        {
            sprite_additional_data *spr_d = &spr_add_data[j];
            cstat = getSprite(j)->_cstat;
            if (cstat&dasprclipmask)
            {
                x1 = getSprite(j)->_x;
                y1 = getSprite(j)->_y;

                clipyou = 0;
                switch(cstat&48)
                {
                case 0:
                    k = walldist+(spr_d->clipdist<<2)+1;
                    if ((klabs(x1-x) <= k) && (klabs(y1-y) <= k))
                    {
                        daz = getSprite(j)->_z;
                        k = ((tiles[getSprite(j)->_picnum].dim.height*getSprite(j)->_yrepeat)<<1);
                        if (cstat&128)
                            daz += k;
                        if (tiles[getSprite(j)->_picnum].animFlags&0x00ff0000) daz -= ((int32_t)((int8_t  )((tiles[getSprite(j)->_picnum].animFlags>>16)&255))*getSprite(j)->_yrepeat<<2);
                        daz2 = daz - (k<<1);
                        clipyou = 1;
                    }
                    break;
                case 16:
                    tilenum = getSprite(j)->_picnum;
                    xoff = (int32_t)((int8_t  )((tiles[tilenum].animFlags>>8)&255))+((int32_t)getSprite(j)->_xoffset);
                    if ((cstat&4) > 0) xoff = -xoff;
                    k = getSprite(j)->_ang;
                    l = getSprite(j)->_xrepeat;
                    dax = sintable[k&2047]*l;
                    day = sintable[(k+1536)&2047]*l;
                    l = tiles[tilenum].dim.width;
                    k = (l>>1)+xoff;
                    x1 -= mulscale16(dax,k);
                    x2 = x1+mulscale16(dax,l);
                    y1 -= mulscale16(day,k);
                    y2 = y1+mulscale16(day,l);
                    if (clipinsideboxline(x,y,x1,y1,x2,y2,walldist+1) != 0)
                    {
                        daz = getSprite(j)->_z;
                        k = ((tiles[getSprite(j)->_picnum].dim.height*getSprite(j)->_yrepeat)<<1);
                        if (cstat&128)
                            daz += k;

                        if (tiles[getSprite(j)->_picnum].animFlags&0x00ff0000)
                            daz -= ((int32_t)((int8_t  )((tiles[getSprite(j)->_picnum].animFlags>>16)&255))*getSprite(j)->_yrepeat<<2);

                        daz2 = daz-(k<<1);
                        clipyou = 1;
                    }
                    break;
                case 32:
                    daz = getSprite(j)->_z;
                    daz2 = daz;

                    if ((cstat&64) != 0)
                        if ((z > daz) == ((cstat&8)==0)) continue;

                    tilenum = getSprite(j)->_picnum;
                    xoff = (int32_t)((int8_t  )((tiles[tilenum].animFlags>>8)&255))+((int32_t)getSprite(j)->_xoffset);
                    yoff = (int32_t)((int8_t  )((tiles[tilenum].animFlags>>16)&255))+((int32_t)getSprite(j)->_yoffset);
                    if ((cstat&4) > 0) xoff = -xoff;
                    if ((cstat&8) > 0) yoff = -yoff;

                    ang = getSprite(j)->_ang;
                    cosang = sintable[(ang+512)&2047];
                    sinang = sintable[ang];
                    xspan = tiles[tilenum].dim.width;
                    xrepeat = getSprite(j)->_xrepeat;
                    yspan = tiles[tilenum].dim.height;
                    yrepeat = getSprite(j)->_yrepeat;

                    dax = ((xspan>>1)+xoff)*xrepeat;
                    day = ((yspan>>1)+yoff)*yrepeat;
                    x1 += dmulscale16(sinang,dax,cosang,day)-x;
                    y1 += dmulscale16(sinang,day,-cosang,dax)-y;
                    l = xspan*xrepeat;
                    x2 = x1 - mulscale16(sinang,l);
                    y2 = y1 + mulscale16(cosang,l);
                    l = yspan*yrepeat;
                    k = -mulscale16(cosang,l);
                    x3 = x2+k;
                    x4 = x1+k;
                    k = -mulscale16(sinang,l);
                    y3 = y2+k;
                    y4 = y1+k;

                    dax = mulscale14(sintable[(getSprite(j)->_ang-256+512)&2047],walldist+4);
                    day = mulscale14(sintable[(getSprite(j)->_ang-256)&2047],walldist+4);
                    x1 += dax;
                    x2 -= day;
                    x3 -= dax;
                    x4 += day;
                    y1 += day;
                    y2 += dax;
                    y3 -= day;
                    y4 -= dax;

                    if ((y1^y2) < 0)
                    {
                        if ((x1^x2) < 0) clipyou ^= (x1*y2<x2*y1)^(y1<y2);
                        else if (x1 >= 0) clipyou ^= 1;
                    }
                    if ((y2^y3) < 0)
                    {
                        if ((x2^x3) < 0) clipyou ^= (x2*y3<x3*y2)^(y2<y3);
                        else if (x2 >= 0) clipyou ^= 1;
                    }
                    if ((y3^y4) < 0)
                    {
                        if ((x3^x4) < 0) clipyou ^= (x3*y4<x4*y3)^(y3<y4);
                        else if (x3 >= 0) clipyou ^= 1;
                    }
                    if ((y4^y1) < 0)
                    {
                        if ((x4^x1) < 0) clipyou ^= (x4*y1<x1*y4)^(y4<y1);
                        else if (x4 >= 0) clipyou ^= 1;
                    }
                    break;
                }

                if (clipyou != 0)
                {
                    if ((z > daz) && (daz > *ceilz)) {
                        *ceilz = daz;
                        *ceilhit = j+49152;
                    }
                    if ((z < daz2) && (daz2 < *florz)) {
                        *florz = daz2;
                        *florhit = j+49152;
                    }
                }
            }
        }
    }
}


void setview(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
//    printf("set view: x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", x1, y1, x2, y2);
    windowx1 = x1;
    wx1 = (x1<<12);
    windowy1 = y1;
    wy1 = (y1<<12);
    windowx2 = x2;
    wx2 = ((x2+1)<<12);
    windowy2 = y2;
    wy2 = ((y2+1)<<12);

    xdimen = (x2-x1)+1;
    halfxdimen = (xdimen>>1);
    xdimenrecip = divscale32(1L,xdimen);
    ydimen = (y2-y1)+1;

    setaspect(65536L,(int32_t)divscale16(ydim*320L,xdim*200L));

#if SET_STARTxMOST
    int32_t i;
    for(i=0; i<windowx1; i++) {
        _startumost[i] = 1, _startdmost[i] = 0;
    }
    for(i=windowx1; i<=windowx2; i++)
    {
        _startumost[i] = windowy1, _startdmost[i] = windowy2+1;
    }
    for(i=windowx2+1; i<xdim; i++) {
        _startumost[i] = 1, _startdmost[i] = 0;
    }
#else
 _startumost = windowy1, _startdmost = windowy2+1;
#endif

    viewoffset = windowy1*bytesperline + windowx1;
#if USE_STEREO_VIDEO
    if (stereomode)
    {
        ostereopixelwidth = stereopixelwidth;
        xdimen = (windowx2-windowx1+1)+(stereopixelwidth<<1);
        halfxdimen = (xdimen>>1);
        xdimenrecip = divscale32(1L,xdimen);
        setaspect((int32_t)divscale16(xdimen,windowx2-windowx1+1),yxaspect);
    }
#endif
}


void setaspect(int32_t daxrange, int32_t daaspect)
{
    viewingrange = daxrange;
    viewingrangerecip = divscale32(1L,daxrange);

    yxaspect = daaspect;
    xyaspect = divscale32(1,yxaspect);
    xdimenscale = scale(xdimen,yxaspect,320);
    xdimscale = scale(320,xyaspect,xdimen);
}

void flushperms(void)
{
#if CODE_NOT_IMPLEMENTED

    permhead = permtail = 0;
#endif

}
// Render a sprite on screen. This is used by the Engine but also the Game module
// when drawing the HUD or the Weapon held by the player !!!
#if WIN32
void rotatesprite2(int32_t sx, int32_t sy, int32_t z, short a, short picnum,
                  int8_t dashade, uint8_t  dapalnum, uint8_t  dastat,
                  int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, char *file, int line)

#else
void rotatesprite(int32_t sx, int32_t sy, int32_t z, short a, short picnum,
                  int8_t dashade, uint8_t  dapalnum, uint8_t  dastat,
                  int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2)


#endif
{
#if WIN32 && SHOW_ROTATESPRITE_DEBUG
    printf("rotatesprite: %s %d\r\n", file, line);
#endif
#if CODE_NOT_IMPLEMENTED
    permfifotype *per, *per2;
#endif
    //If 2D target coordinate do not make sense (left > right)..
    if ((cx1 > cx2) || (cy1 > cy2))
        return;

    if (z <= 16)
        return;

    if (tiles[picnum].animFlags&192)
        picnum += animateoffs(picnum);

    //Does the tile has negative dimensions ?
    if ((tiles[picnum].dim.width <= 0) || (tiles[picnum].dim.height <= 0))
        return;
    if (((dastat&128) == 0) || (numpages < 2) || (beforedrawrooms != 0))    // this is always implemented, we have only 1 page.
        dorotatesprite(sx,sy,z,a,picnum,dashade,dapalnum,dastat,cx1,cy1,cx2,cy2);
#if CODE_NOT_IMPLEMENTED
    if ((dastat&64) && (cx1 <= 0) && (cy1 <= 0) && (cx2 >= xdim-1) && (cy2 >= ydim-1) &&
            (sx == (160<<16)) && (sy == (100<<16)) && (z == 65536L) && (a == 0) && ((dastat&1) == 0))
        permhead = permtail = 0;

    if ((dastat&128) == 0) return;
    if (numpages >= 2)
    {
        per = &permfifo[permhead];
        per->sx = sx;
        per->sy = sy;
        per->z = z;
        per->a = a;
        per->picnum = picnum;
        per->dashade = dashade;
        per->dapalnum = dapalnum;
        per->dastat = dastat;
        per->pagesleft = numpages+((beforedrawrooms&1)<<7);
        per->cx1 = cx1;
        per->cy1 = cy1;
        per->cx2 = cx2;
        per->cy2 = cy2;
        /* Would be better to optimize out true bounding boxes */
        if (dastat&64)  /* If non-masking write, checking for overlapping cases */
        {
            for(i=permtail; i!=permhead; i=((i+1)&(MAXPERMS-1)))
            {
                per2 = &permfifo[i];
                if ((per2->pagesleft&127) == 0) continue;
                if (per2->sx != per->sx) continue;
                if (per2->sy != per->sy) continue;
                if (per2->z != per->z) continue;
                if (per2->a != per->a) continue;
                if (tiles[per2->picnum].dim.width > tiles[per->picnum].dim.width)
                    continue;

                if (tiles[per2->picnum].dim.height > tiles[per->picnum].dim.height)
                    continue;
                if (per2->cx1 < per->cx1) continue;
                if (per2->cy1 < per->cy1) continue;
                if (per2->cx2 > per->cx2) continue;
                if (per2->cy2 > per->cy2) continue;
                per2->pagesleft = 0;
            }
            if ((per->z == 65536) && (per->a == 0))
                for(i=permtail; i!=permhead; i=((i+1)&(MAXPERMS-1)))
                {
                    per2 = &permfifo[i];
                    if ((per2->pagesleft&127) == 0) continue;
                    if (per2->z != 65536) continue;
                    if (per2->a != 0) continue;
                    if (per2->cx1 < per->cx1) continue;
                    if (per2->cy1 < per->cy1) continue;
                    if (per2->cx2 > per->cx2) continue;
                    if (per2->cy2 > per->cy2) continue;
                    if ((per2->sx>>16) < (per->sx>>16)) continue;
                    if ((per2->sy>>16) < (per->sy>>16)) continue;
                    if ((per2->sx>>16)+tiles[per2->picnum].dim.width > (per->sx>>16)+tiles[per->picnum].dim.width)
                        continue;
                    if ((per2->sy>>16)+tiles[per2->picnum].dim.height > (per->sy>>16)+tiles[per->picnum].dim.height)
                        continue;
                    per2->pagesleft = 0;
                }
        }

        permhead = ((permhead+1)&(MAXPERMS-1));
    }
        #endif

}

void rotateRAMsprite(int32_t sx, int32_t sy, int32_t z, short a, uint8_t *buffer,
                  int8_t dashade, uint8_t  dapalnum, uint8_t  dastat,
                  int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2)


{
    if (z <= 16)
        return;
    if (((dastat&128) == 0) || (numpages < 2) || (beforedrawrooms != 0))    // this is always implemented, we have only 1 page.
        dorotateRAMsprite(sx,sy,z,a,buffer,dashade,dapalnum,dastat,cx1,cy1,cx2,cy2);
}
#if 0
static int getclosestcol(int32_t r, int32_t g, int32_t b)
{
    int32_t i, j, k, dist, mindist, retcol;
    uint8_t  *pal1;

    j = (r>>3)*FASTPALGRIDSIZ*FASTPALGRIDSIZ+(g>>3)*FASTPALGRIDSIZ+(b>>3)+FASTPALGRIDSIZ*FASTPALGRIDSIZ+FASTPALGRIDSIZ+1;
    mindist = min(rdist[coldist[r&7]+64+8],gdist[coldist[g&7]+64+8]);
    mindist = min(mindist,bdist[coldist[b&7]+64+8]);
    mindist++;

    r = 64-r;
    g = 64-g;
    b = 64-b;

    retcol = -1;
    for(k=26; k>=0; k--)
    {
        i = colscan[k]+j;
        if ((colhere[i>>3]&pow2char[i&7]) == 0) continue;
        i = colhead[i];
        do
        {
            pal1 = (uint8_t  *)&palette[i*3];
            dist = gdist[pal1[1]+g];
            if (dist < mindist)
            {
                dist += rdist[pal1[0]+r];
                if (dist < mindist)
                {
                    dist += bdist[pal1[2]+b];
                    if (dist < mindist) {
                        mindist = dist;
                        retcol = i;
                    }
                }
            }
            i = colnext[i];
        } while (i >= 0);
    }
    if (retcol >= 0) return(retcol);

    mindist = 0x7fffffff;
    pal1 = (uint8_t  *)&palette[768-3];
    for(i=255; i>=0; i--,pal1-=3)
    {
        dist = gdist[pal1[1]+g];
        if (dist >= mindist) continue;
        dist += rdist[pal1[0]+r];
        if (dist >= mindist) continue;
        dist += bdist[pal1[2]+b];
        if (dist >= mindist) continue;
        mindist = dist;
        retcol = i;
    }
    return(retcol);
}
#endif

void makepalookup(int32_t palnum, uint8_t  *remapbuf, int8_t r,
                  int8_t g, int8_t b, uint8_t  dastat)
{
    #if USE_EXT_MEM
        int32_t i, j;
        uint8_t  *ptr, *ptr2;

        if (paletteloaded == 0)
            return;

        if (dastat == 0) return;
        if ((r|g|b|63) != 63) return;

        if ((r|g|b) == 0)
        {
            printf("Making palookup %i. r = %d, g = %d, b = %d, dastat = %d\r\n", palnum, r, g, b, dastat);
            uint8_t temppalookup[256 * 32];

            for(i=0; i<256; i++)
            {
                ptr = (uint8_t*) palookup[0] + remapbuf[i];
                ptr2 = (uint8_t*) temppalookup + i;
                for(j=0; j<numpalookups; j++)
                {
                    *ptr2 = *ptr;
                    ptr += 256;
                    ptr2 += 256;
                }
            }
            void *p = storeToInternalFlash(temppalookup, sizeof(temppalookup));
            storeToInternalFlashAtPointer(&p, &palookup[palnum], sizeof(p));
        }
        else
        {
            FIXME("ERROR PALETTE");
            EXIT(1);
        }
#if 0 // NOT USED
        else
        {
            ptr2 = palookup[palnum];
            for(i=0; i<numpalookups; i++)
            {
                palscale = divscale16(i,numpalookups);
                for(j=0; j<256; j++)
                {
                    ptr = (uint8_t  *)&palette[remapbuf[j]*3];
                    *ptr2++ = getclosestcol((int32_t)ptr[0]+mulscale16(r-ptr[0],palscale),
                                            (int32_t)ptr[1]+mulscale16(g-ptr[1],palscale),
                                            (int32_t)ptr[2]+mulscale16(b-ptr[2],palscale));
                }
            }
        }
#endif
    #else
    int32_t i, j, palscale;
    uint8_t  *ptr, *ptr2;

    if (paletteloaded == 0)
        return;

    if (palookup[palnum] == NULL)
    {
        /* Allocate palookup buffer */
        if ((palookup[palnum] = (uint8_t  *)malloc(numpalookups<<8)) == NULL)
/*code disabled*/            allocache(&palookup[palnum],numpalookups<<8,&permanentlock);
    }

    if (dastat == 0) return;
    if ((r|g|b|63) != 63) return;

    if ((r|g|b) == 0)
    {
        for(i=0; i<256; i++)
        {
            ptr = palookup[0] + remapbuf[i];
            ptr2 = palookup[palnum] + i;
            for(j=0; j<numpalookups; j++)
            {
                *ptr2 = *ptr;
                ptr += 256;
                ptr2 += 256;
            }
        }
    }
    else
    {
        ptr2 = palookup[palnum];
        for(i=0; i<numpalookups; i++)
        {
            palscale = divscale16(i,numpalookups);
            for(j=0; j<256; j++)
            {
                ptr = (uint8_t  *)&palette[remapbuf[j]*3];
                *ptr2++ = getclosestcol((int32_t)ptr[0]+mulscale16(r-ptr[0],palscale),
                                        (int32_t)ptr[1]+mulscale16(g-ptr[1],palscale),
                                        (int32_t)ptr[2]+mulscale16(b-ptr[2],palscale));
            }
        }
    }

    #endif // USE_EXT_MEM
}


void setbrightness(uint8_t  dabrightness, const uint8_t  *dapal)
{
    int32_t i, k;
    uint8_t newPalette[256*4];

    //Clamp bightness to [0-15]
    curbrightness = min(max(dabrightness,0),15);

    k = 0;

    for(i=0; i<256; i++){
        newPalette[k++] = britable[curbrightness][dapal[i*3+2]];
        newPalette[k++] = britable[curbrightness][dapal[i*3+1]];
        newPalette[k++] = britable[curbrightness][dapal[i*3+0]];
        newPalette[k++] = 0;
    }

    VBE_setPalette(newPalette);
}

//This is only used by drawmapview.
static void fillpolygon(int32_t npoints, const uint8_t *tilebufpos)
{
    short *dotp1[MAXYDIM + 1], *dotp2[MAXYDIM + 1];
    int32_t z, zz, x1, y1, x2, y2, miny, maxy, y, xinc, cnt;
    int32_t ox, oy, bx, by, day1, day2;
    uint8_t *p;
    short *ptr, *ptr2;

    miny = 0x7fffffff;
    maxy = 0x80000000;
    for(z=npoints-1; z>=0; z--)
    {
        y = pvWalls[z].cameraSpaceCoo[0][VEC_Y];
        miny = min(miny,y);
        maxy = max(maxy,y);
    }
    miny = (miny>>12);
    maxy = (maxy>>12);
    if (miny < 0) miny = 0;
    if (maxy >= ydim) maxy = ydim-1;
    // next-hack: global smost is reused, but it should be large enough not to overflow.
    // let's put in stack, there should be not huge usage here.
    short smost[(MAXYDIM + 1) * MAXNODESPERLINE * 2];
    ptr = smost;    /* They're pointers! - watch how you optimize this thing */
    for(y=miny; y<=maxy; y++)
    {
        dotp1[y] = ptr;
        dotp2[y] = ptr+(MAXNODESPERLINE>>1);
        ptr += MAXNODESPERLINE;
    }
    //printf("dotp1[0]: %x\r\n", dotp1[0]);
    for(z=npoints-1; z>=0; z--)
    {
        zz = pvWalls[z].screenSpaceCoo[0].vec_colmap;
        y1 = pvWalls[z] .cameraSpaceCoo[0][VEC_Y];
        if (y1 < 0)
            y1 = 0;
        day1 = (y1>>12);
        y2 = pvWalls[zz].cameraSpaceCoo[0][VEC_Y];
        //
        if (y2 < 0)
            y2 = 0;
        day2 = (y2>>12);

        if (day1 != day2)
        {
            x1 = pvWalls[z ].cameraSpaceCoo[0][VEC_X];
            x2 = pvWalls[zz].cameraSpaceCoo[0][VEC_X];
            xinc = divscale12(x2-x1,y2-y1);
            if (day2 > day1)
            {
                x1 += mulscale12((day1<<12)+4095-y1,xinc);
                for(y=day1; y<day2; y++) {
                  // printf("2[%d]: %x\r\n", y, &dotp2[y]);
                    *dotp2[y]++ = (x1>>12);
                    x1 += xinc;
                }
            }
            else
            {
                x2 += mulscale12((day2<<12)+4095-y2,xinc);
                for(y=day2; y<day1; y++) {
                   // printf("1[%d]: %x\r\n", y, &dotp1[y]);
                    *dotp1[y]++ = (x2>>12);
                    x2 += xinc;
                }
            }
        }
    }

    globalx1 = mulscale16(globalx1,xyaspect);
    globaly2 = mulscale16(globaly2,xyaspect);

    oy = miny+1-(ydim>>1);
    globalposx += oy*globalx1;
    globalposy += oy*globaly2;



    ptr = smost;
    for(y=miny; y<=maxy; y++)
    {
        cnt = dotp1[y]-ptr;
        ptr2 = ptr+(MAXNODESPERLINE>>1);
        for(z=cnt-1; z>=0; z--)
        {
            day1 = 0;
            day2 = 0;
            for(zz=z; zz>0; zz--)
            {
                if (ptr[zz] < ptr[day1]) day1 = zz;
                if (ptr2[zz] < ptr2[day2]) day2 = zz;
            }
            x1 = ptr[day1];
            ptr[day1] = ptr[z];
            x2 = ptr2[day2]-1;
            ptr2[day2] = ptr2[z];
            if (x1 > x2) continue;
            if (x1 < 0)
                x1 = 0;
            if (globalpolytype < 1)
            {
                /* maphline */
                ox = x2+1-(xdim>>1);
                bx = ox * draws.asm1 + globalposx;
                by = ox * draws.asm2 - globalposy;

                p = ylookup(y)+x2+frameplace;
                draws.globalpalshade = &globalpalwritten[globalshade << 8];
                draws.globalpalshade = getPalette(draws.globalpalshade, x2-x1);
                hlineasm4(x2-x1,/*globalshade<<8,*/by,bx,p);
            }
            else
            {
 // called when drawing sprites on ceil/floors.
                /* maphline */
                ox = x1+1-(xdim>>1);
                bx = ox*draws.asm1 + globalposx;
                by = ox * draws.asm2 - globalposy;

                p = ylookup(y)+x1+frameplace;
                if (globalpolytype == 1)
                {
                    mhline(tilebufpos,bx,(x2-x1)<<16,by,p);
                }
                else
                {
                    thline(tilebufpos,bx,(x2-x1)<<16,by,p);
                    transarea += (x2-x1);by = ox*draws.asm2 - globalposy;
                }
            }
        }
        globalposx += globalx1;
        globalposy += globaly2;
        ptr += MAXNODESPERLINE;
    }
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
}

//This is only used by drawmapview.
static int clippoly (int32_t npoints, int32_t clipstat)
{
    int32_t z, zz, s1, s2, t, npoints2, start2, z1, z2, z3, z4, splitcnt;
    int32_t cx1, cy1, cx2, cy2;
    short bunchWallsList[1024];     // next-hack: let's use some stack.
//    pvWall_t pvWalls[256];

    cx1 = windowx1;
    cy1 = windowy1;
    cx2 = windowx2+1;
    cy2 = windowy2+1;
    cx1 <<= 12;
    cy1 <<= 12;
    cx2 <<= 12;
    cy2 <<= 12;

    if (clipstat&0xa)   /* Need to clip top or left */
    {
        npoints2 = 0;
        start2 = 0;
        z = 0;
        splitcnt = 0;
        do
        {
            s2 = cx1-pvWalls[z].cameraSpaceCoo[0][VEC_X];
            do
            {
                zz = pvWalls[z].screenSpaceCoo[0].vec_colmap;
                pvWalls[z].screenSpaceCoo[0].vec_colmap = -1;
                s1 = s2;
                s2 = cx1-pvWalls[zz].cameraSpaceCoo[0][VEC_X];

                if (s1 < 0)
                {
                    pvWalls[npoints2].cameraSpaceCoo[1][VEC_X] = pvWalls[z].cameraSpaceCoo[0][VEC_X];
                    pvWalls[npoints2].cameraSpaceCoo[1][VEC_Y] = pvWalls[z].cameraSpaceCoo[0][VEC_Y];
                    pvWalls[npoints2].screenSpaceCoo[1].vec_colmap = npoints2+1;
                    npoints2++;
                }

                if ((s1 ^ s2) < 0)
                {
                    pvWalls[npoints2].cameraSpaceCoo[1][VEC_X] =
                        pvWalls[z].cameraSpaceCoo[0][VEC_X] + scale(pvWalls[zz].cameraSpaceCoo[0][VEC_X] - pvWalls[z].cameraSpaceCoo[0][VEC_X], s1, s1 - s2);
                    pvWalls[npoints2].cameraSpaceCoo[1][VEC_Y] =
                        pvWalls[z].cameraSpaceCoo[0][VEC_Y] + scale(pvWalls[zz].cameraSpaceCoo[0][VEC_Y] - pvWalls[z].cameraSpaceCoo[0][VEC_Y], s1, s1 - s2);

                    if (s1 < 0)
                        bunchWallsList[splitcnt++] = npoints2;

                    pvWalls[npoints2].screenSpaceCoo[1].vec_colmap = npoints2 + 1;
                    npoints2++;
                }
                z = zz;
            } while (pvWalls[z].screenSpaceCoo[0].vec_colmap >= 0);

            if (npoints2 >= start2+3)
                pvWalls[npoints2-1].screenSpaceCoo[1].vec_colmap = start2, start2 = npoints2;
            else
                npoints2 = start2;

            z = 1;
            while ((z < npoints) && (pvWalls[z].screenSpaceCoo[0].vec_colmap < 0)) z++;
        } while (z < npoints);
        if (npoints2 <= 2) return(0);

        for(z=1; z<splitcnt; z++)
            for(zz=0; zz<z; zz++)
            {
                z1 = bunchWallsList[z];
                z2 = pvWalls[z1].screenSpaceCoo[1].vec_colmap;
                z3 = bunchWallsList[zz];
                z4 = pvWalls[z3].screenSpaceCoo[1].vec_colmap;
                s1  = klabs(pvWalls[z1].cameraSpaceCoo[1][VEC_X]-pvWalls[z2].cameraSpaceCoo[1][VEC_X])+klabs(pvWalls[z1].cameraSpaceCoo[1][VEC_Y]-pvWalls[z2].cameraSpaceCoo[1][VEC_Y]);
                s1 += klabs(pvWalls[z3].cameraSpaceCoo[1][VEC_X]-pvWalls[z4].cameraSpaceCoo[1][VEC_X])+klabs(pvWalls[z3].cameraSpaceCoo[1][VEC_Y]-pvWalls[z4].cameraSpaceCoo[1][VEC_Y]);
                s2  = klabs(pvWalls[z1].cameraSpaceCoo[1][VEC_X]-pvWalls[z4].cameraSpaceCoo[1][VEC_X])+klabs(pvWalls[z1].cameraSpaceCoo[1][VEC_Y]-pvWalls[z4].cameraSpaceCoo[1][VEC_Y]);
                s2 += klabs(pvWalls[z3].cameraSpaceCoo[1][VEC_X]-pvWalls[z2].cameraSpaceCoo[1][VEC_X])+klabs(pvWalls[z3].cameraSpaceCoo[1][VEC_Y]-pvWalls[z2].cameraSpaceCoo[1][VEC_Y]);
                if (s2 < s1)
                {
                    t = pvWalls[bunchWallsList[z]].screenSpaceCoo[1].vec_colmap;
                    pvWalls[bunchWallsList[z]].screenSpaceCoo[1].vec_colmap = pvWalls[bunchWallsList[zz]].screenSpaceCoo[1].vec_colmap;
                    pvWalls[bunchWallsList[zz]].screenSpaceCoo[1].vec_colmap = t;
                }
            }


        npoints = 0;
        start2 = 0;
        z = 0;
        splitcnt = 0;
        do
        {
            s2 = cy1-pvWalls[z].cameraSpaceCoo[1][VEC_Y];
            do
            {
                zz = pvWalls[z].screenSpaceCoo[1].vec_colmap;
                pvWalls[z].screenSpaceCoo[1].vec_colmap = -1;
                s1 = s2;
                s2 = cy1-pvWalls[zz].cameraSpaceCoo[1][VEC_Y];
                if (s1 < 0)
                {
                    pvWalls[npoints].cameraSpaceCoo[0][VEC_X] = pvWalls[z].cameraSpaceCoo[1][VEC_X];
                    pvWalls[npoints].cameraSpaceCoo[0][VEC_Y] = pvWalls[z].cameraSpaceCoo[1][VEC_Y];
                    pvWalls[npoints].screenSpaceCoo[0].vec_colmap = npoints+1;
                    npoints++;
                }
                if ((s1^s2) < 0)
                {
                    pvWalls[npoints].cameraSpaceCoo[0][VEC_X] = pvWalls[z].cameraSpaceCoo[1][VEC_X]+scale(pvWalls[zz].cameraSpaceCoo[1][VEC_X]-pvWalls[z].cameraSpaceCoo[1][VEC_X],s1,s1-s2);
                    pvWalls[npoints].cameraSpaceCoo[0][VEC_Y] = pvWalls[z].cameraSpaceCoo[1][VEC_Y]+scale(pvWalls[zz].cameraSpaceCoo[1][VEC_Y]-pvWalls[z].cameraSpaceCoo[1][VEC_Y],s1,s1-s2);
                    if (s1 < 0) bunchWallsList[splitcnt++] = npoints;
                    pvWalls[npoints].screenSpaceCoo[0].vec_colmap = npoints+1;
                    npoints++;
                }
                z = zz;
            } while (pvWalls[z].screenSpaceCoo[1].vec_colmap >= 0);

            if (npoints >= start2+3)
                pvWalls[npoints-1].screenSpaceCoo[0].vec_colmap = start2, start2 = npoints;
            else
                npoints = start2;

            z = 1;
            while ((z < npoints2) && (pvWalls[z].screenSpaceCoo[1].vec_colmap < 0)) z++;
        } while (z < npoints2);
        if (npoints <= 2) return(0);

        for(z=1; z<splitcnt; z++)
            for(zz=0; zz<z; zz++)
            {
                z1 = bunchWallsList[z];
                z2 = pvWalls[z1].screenSpaceCoo[0].vec_colmap;
                z3 = bunchWallsList[zz];
                z4 = pvWalls[z3].screenSpaceCoo[0].vec_colmap;
                s1  = klabs(pvWalls[z1].cameraSpaceCoo[0][VEC_X]-pvWalls[z2].cameraSpaceCoo[0][VEC_X])+klabs(pvWalls[z1].cameraSpaceCoo[0][VEC_Y]-pvWalls[z2].cameraSpaceCoo[0][VEC_Y]);
                s1 += klabs(pvWalls[z3].cameraSpaceCoo[0][VEC_X]-pvWalls[z4].cameraSpaceCoo[0][VEC_X])+klabs(pvWalls[z3].cameraSpaceCoo[0][VEC_Y]-pvWalls[z4].cameraSpaceCoo[0][VEC_Y]);
                s2  = klabs(pvWalls[z1].cameraSpaceCoo[0][VEC_X]-pvWalls[z4].cameraSpaceCoo[0][VEC_X])+klabs(pvWalls[z1].cameraSpaceCoo[0][VEC_Y]-pvWalls[z4].cameraSpaceCoo[0][VEC_Y]);
                s2 += klabs(pvWalls[z3].cameraSpaceCoo[0][VEC_X]-pvWalls[z2].cameraSpaceCoo[0][VEC_X])+klabs(pvWalls[z3].cameraSpaceCoo[0][VEC_Y]-pvWalls[z2].cameraSpaceCoo[0][VEC_Y]);
                if (s2 < s1)
                {
                    t = pvWalls[bunchWallsList[z]].screenSpaceCoo[0].vec_colmap;
                    pvWalls[bunchWallsList[z]].screenSpaceCoo[0].vec_colmap = pvWalls[bunchWallsList[zz]].screenSpaceCoo[0].vec_colmap;
                    pvWalls[bunchWallsList[zz]].screenSpaceCoo[0].vec_colmap = t;
                }
            }
    }
    if (clipstat&0x5)   /* Need to clip bottom or right */
    {
        npoints2 = 0;
        start2 = 0;
        z = 0;
        splitcnt = 0;
        do
        {
            s2 = pvWalls[z].cameraSpaceCoo[0][VEC_X]-cx2;
            do
            {
                zz = pvWalls[z].screenSpaceCoo[0].vec_colmap;
                pvWalls[z].screenSpaceCoo[0].vec_colmap = -1;
                s1 = s2;
                s2 = pvWalls[zz].cameraSpaceCoo[0][VEC_X]-cx2;
                if (s1 < 0)
                {
                    pvWalls[npoints2].cameraSpaceCoo[1][VEC_X] = pvWalls[z].cameraSpaceCoo[0][VEC_X];
                    pvWalls[npoints2].cameraSpaceCoo[1][VEC_Y] = pvWalls[z].cameraSpaceCoo[0][VEC_Y];
                    pvWalls[npoints2].screenSpaceCoo[1].vec_colmap = npoints2+1;
                    npoints2++;
                }
                if ((s1^s2) < 0)
                {
                    pvWalls[npoints2].cameraSpaceCoo[1][VEC_X] = pvWalls[z].cameraSpaceCoo[0][VEC_X]+scale(pvWalls[zz].cameraSpaceCoo[0][VEC_X]-pvWalls[z].cameraSpaceCoo[0][VEC_X],s1,s1-s2);
                    pvWalls[npoints2].cameraSpaceCoo[1][VEC_Y] = pvWalls[z].cameraSpaceCoo[0][VEC_Y]+scale(pvWalls[zz].cameraSpaceCoo[0][VEC_Y]-pvWalls[z].cameraSpaceCoo[0][VEC_Y],s1,s1-s2);
                    if (s1 < 0) bunchWallsList[splitcnt++] = npoints2;
                    pvWalls[npoints2].screenSpaceCoo[1].vec_colmap = npoints2+1;
                    npoints2++;
                }
                z = zz;
            } while (pvWalls[z].screenSpaceCoo[0].vec_colmap >= 0);

            if (npoints2 >= start2+3)
                pvWalls[npoints2-1].screenSpaceCoo[1].vec_colmap = start2, start2 = npoints2;
            else
                npoints2 = start2;

            z = 1;
            while ((z < npoints) && (pvWalls[z].screenSpaceCoo[0].vec_colmap < 0)) z++;
        } while (z < npoints);
        if (npoints2 <= 2) return(0);

        for(z=1; z<splitcnt; z++)
            for(zz=0; zz<z; zz++)
            {
                z1 = bunchWallsList[z];
                z2 = pvWalls[z1].screenSpaceCoo[1].vec_colmap;
                z3 = bunchWallsList[zz];
                z4 = pvWalls[z3].screenSpaceCoo[1].vec_colmap;
                s1  = klabs(pvWalls[z1].cameraSpaceCoo[1][VEC_X]-pvWalls[z2].cameraSpaceCoo[1][VEC_X])+klabs(pvWalls[z1].cameraSpaceCoo[1][VEC_Y]-pvWalls[z2].cameraSpaceCoo[1][VEC_Y]);
                s1 += klabs(pvWalls[z3].cameraSpaceCoo[1][VEC_X]-pvWalls[z4].cameraSpaceCoo[1][VEC_X])+klabs(pvWalls[z3].cameraSpaceCoo[1][VEC_Y]-pvWalls[z4].cameraSpaceCoo[1][VEC_Y]);
                s2  = klabs(pvWalls[z1].cameraSpaceCoo[1][VEC_X]-pvWalls[z4].cameraSpaceCoo[1][VEC_X])+klabs(pvWalls[z1].cameraSpaceCoo[1][VEC_Y]-pvWalls[z4].cameraSpaceCoo[1][VEC_Y]);
                s2 += klabs(pvWalls[z3].cameraSpaceCoo[1][VEC_X]-pvWalls[z2].cameraSpaceCoo[1][VEC_X])+klabs(pvWalls[z3].cameraSpaceCoo[1][VEC_Y]-pvWalls[z2].cameraSpaceCoo[1][VEC_Y]);
                if (s2 < s1)
                {
                    t = pvWalls[bunchWallsList[z]].screenSpaceCoo[1].vec_colmap;
                    pvWalls[bunchWallsList[z]].screenSpaceCoo[1].vec_colmap = pvWalls[bunchWallsList[zz]].screenSpaceCoo[1].vec_colmap;
                    pvWalls[bunchWallsList[zz]].screenSpaceCoo[1].vec_colmap = t;
                }
            }


        npoints = 0;
        start2 = 0;
        z = 0;
        splitcnt = 0;
        do
        {
            s2 = pvWalls[z].cameraSpaceCoo[1][VEC_Y]-cy2;
            do
            {
                zz = pvWalls[z].screenSpaceCoo[1].vec_colmap;
                pvWalls[z].screenSpaceCoo[1].vec_colmap = -1;
                s1 = s2;
                s2 = pvWalls[zz].cameraSpaceCoo[1][VEC_Y]-cy2;
                if (s1 < 0)
                {
                    pvWalls[npoints].cameraSpaceCoo[0][VEC_X] = pvWalls[z].cameraSpaceCoo[1][VEC_X];
                    pvWalls[npoints].cameraSpaceCoo[0][VEC_Y] = pvWalls[z].cameraSpaceCoo[1][VEC_Y];
                    pvWalls[npoints].screenSpaceCoo[0].vec_colmap = npoints+1;
                    npoints++;
                }
                if ((s1^s2) < 0)
                {
                    pvWalls[npoints].cameraSpaceCoo[0][VEC_X] = pvWalls[z].cameraSpaceCoo[1][VEC_X]+scale(pvWalls[zz].cameraSpaceCoo[1][VEC_X]-pvWalls[z].cameraSpaceCoo[1][VEC_X],s1,s1-s2);
                    pvWalls[npoints].cameraSpaceCoo[0][VEC_Y] = pvWalls[z].cameraSpaceCoo[1][VEC_Y]+scale(pvWalls[zz].cameraSpaceCoo[1][VEC_Y]-pvWalls[z].cameraSpaceCoo[1][VEC_Y],s1,s1-s2);
                    if (s1 < 0) bunchWallsList[splitcnt++] = npoints;
                    pvWalls[npoints].screenSpaceCoo[0].vec_colmap = npoints+1;
                    npoints++;
                }
                z = zz;
            } while (pvWalls[z].screenSpaceCoo[1].vec_colmap >= 0);

            if (npoints >= start2+3)
                pvWalls[npoints-1].screenSpaceCoo[0].vec_colmap = start2, start2 = npoints;
            else
                npoints = start2;

            z = 1;
            while ((z < npoints2) && (pvWalls[z].screenSpaceCoo[1].vec_colmap < 0)) z++;
        } while (z < npoints2);
        if (npoints <= 2) return(0);

        for(z=1; z<splitcnt; z++)
            for(zz=0; zz<z; zz++)
            {
                z1 = bunchWallsList[z];
                z2 = pvWalls[z1].screenSpaceCoo[0].vec_colmap;
                z3 = bunchWallsList[zz];
                z4 = pvWalls[z3].screenSpaceCoo[0].vec_colmap;
                s1  = klabs(pvWalls[z1].cameraSpaceCoo[0][VEC_X]-pvWalls[z2].cameraSpaceCoo[0][VEC_X])+klabs(pvWalls[z1].cameraSpaceCoo[0][VEC_Y]-pvWalls[z2].cameraSpaceCoo[0][VEC_Y]);
                s1 += klabs(pvWalls[z3].cameraSpaceCoo[0][VEC_X]-pvWalls[z4].cameraSpaceCoo[0][VEC_X])+klabs(pvWalls[z3].cameraSpaceCoo[0][VEC_Y]-pvWalls[z4].cameraSpaceCoo[0][VEC_Y]);
                s2  = klabs(pvWalls[z1].cameraSpaceCoo[0][VEC_X]-pvWalls[z4].cameraSpaceCoo[0][VEC_X])+klabs(pvWalls[z1].cameraSpaceCoo[0][VEC_Y]-pvWalls[z4].cameraSpaceCoo[0][VEC_Y]);
                s2 += klabs(pvWalls[z3].cameraSpaceCoo[0][VEC_X]-pvWalls[z2].cameraSpaceCoo[0][VEC_X])+klabs(pvWalls[z3].cameraSpaceCoo[0][VEC_Y]-pvWalls[z2].cameraSpaceCoo[0][VEC_Y]);
                if (s2 < s1)
                {
                    t = pvWalls[bunchWallsList[z]].screenSpaceCoo[0].vec_colmap;
                    pvWalls[bunchWallsList[z]].screenSpaceCoo[0].vec_colmap = pvWalls[bunchWallsList[zz]].screenSpaceCoo[0].vec_colmap;
                    pvWalls[bunchWallsList[zz]].screenSpaceCoo[0].vec_colmap = t;
                }
            }
    }
    return(npoints);
}


void drawmapview(int32_t dax, int32_t day, int32_t zoome, int16_t ang)
{

    // next-hack: here we need only tsprites[...].owner.
    // And this is filled here. So, we declare a struct containing only the owner.
    // yes, it is horrible, but it is the cleast way without having to refactor all

    struct
    {
            short owner;
    } tsprite[MAXSPRITESONSCREEN] = {0};
#if STACK_PVWALLS
#define MAX_NWALLS_PER_SECTOR      226
    pvWall_t spvWalls[MAX_NWALLS_PER_SECTOR] = {0};
    pvWalls = spvWalls;
#endif
    uint8_t flat[MAX_RAM_FLAT_SIZE];    // next-hack: yes, I have borrowed Doom's terminology.
    walltype *wal;
    const sectortype *sec;
//    const spritetype *spr;
    int32_t tilenum, xoff, yoff, i, j, k, l, cosang, sinang, xspan, yspan;
    int32_t xrepeat, yrepeat, x, y, x1, y1, x2, y2, x3, y3, x4, y4, bakx1, baky1;
    int32_t s, w, ox, oy, startwall, cx1, cy1, cx2, cy2;
    int32_t bakgxvect, bakgyvect, sortnum, gap, npoints;
    int32_t xvect, yvect, xvect2, yvect2, daslope;

    beforedrawrooms = 0;


    //This seems to be dead code.
    //clearbuf(visitedSectors,(int32_t)((numsectors+31)>>5),0L);

    cx1 = (windowx1<<12);
    cy1 = (windowy1<<12);
    cx2 = ((windowx2+1)<<12)-1;
    cy2 = ((windowy2+1)<<12)-1;
    zoome <<= 8;
    bakgxvect = divscale28(sintable[(1536-ang)&2047],zoome);
    bakgyvect = divscale28(sintable[(2048-ang)&2047],zoome);
    xvect = mulscale8(sintable[(2048-ang)&2047],zoome);
    yvect = mulscale8(sintable[(1536-ang)&2047],zoome);
    xvect2 = mulscale16(xvect,yxaspect);
    yvect2 = mulscale16(yvect,yxaspect);

    sortnum = 0;
    const const_sectortype *constsec;
    const uint8_t *tilebufpos;
    const void* oldglobalbufplc = NULL;
    for(s=0,sec=getSector(s), constsec = &constsector[0]; s<numsectors; s++,sec = getSector(s), constsec++)
    {
        if (show2dsector[s>>3]&pow2char[s&7])
        {
            npoints = 0;
            i = 0;
            startwall = constsector[s].wallptr;
            int wallIdx = startwall;
            for(w=constsector[s].wallnum,wal=getWall(startwall); w>0; w--,wallIdx++, wal = getWall(wallIdx))
            {
                ox = wal->x - dax;
                oy = wal->y - day;
                x = dmulscale16(ox,xvect,-oy,yvect) + (xdim<<11);
                y = dmulscale16(oy,xvect2,ox,yvect2) + (ydim<<11);
                i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
                pvWalls[npoints].cameraSpaceCoo[0][VEC_X] = x;
                pvWalls[npoints].cameraSpaceCoo[0][VEC_Y] = y;
                pvWalls[npoints].screenSpaceCoo[0].vec_colmap = getWall_point2(wallIdx) - startwall;
                npoints++;
            }

            if ((i&0xf0) != 0xf0)
                continue;

            bakx1 = pvWalls[0].cameraSpaceCoo[0][VEC_X];
            baky1 = mulscale16(pvWalls[0].cameraSpaceCoo[0][VEC_Y]-(ydim<<11),xyaspect)+(ydim<<11);
            if (i&0x0f)
            {
                npoints = clippoly(npoints,i);
                if (npoints < 3) continue;
            }

            /* Collect floor sprites to draw */
            for(i=headspritesect[s]; i>=0; i=spr_add_data[i].nextspritesect)
                if ((getSprite(i)->_cstat&48) == 32)
                {
                    if ((getSprite(i)->_cstat&(64+8)) == (64+8)) continue;
                    tsprite[sortnum++].owner = i;
                }

            //This seems to be dead code.
            //visitedSectors[s>>3] |= pow2char[s&7];

            globalorientation = (int32_t)sec->floorstat;
            if ((globalorientation&1) != 0) continue;

            if (palookup[sec->floorpal] != globalpalwritten)
            {
                globalpalwritten = palookup[sec->floorpal];

            }
            globalpicnum = sec->floorpicnum;
            if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES) globalpicnum = 0;
            setgotpic(globalpicnum);

            if ((tiles[globalpicnum].dim.width <= 0) ||
                (tiles[globalpicnum].dim.height <= 0)) continue;

            if ((tiles[globalpicnum].animFlags&192) != 0)
                globalpicnum += animateoffs(globalpicnum);

            TILE_MakeAvailable(globalpicnum);
            int tileSize;
            tileSize = tiles[globalpicnum].dim.height *tiles[globalpicnum].dim.width;

            globalbufplc = tileData[globalpicnum];
            if (oldglobalbufplc != globalbufplc)
            {
                tilebufpos = globalbufplc;
                oldglobalbufplc = globalbufplc;
                if (isOnExternalFlash(globalbufplc))
                {
                #if WIN32
                    if (tileSize> MAX_RAM_FLAT_SIZE)
                    {
                        FIXME("This is going wrong. Tile size is %d\r\n", tileSize);
                    }
                #endif
                    tilebufpos = extMemGetDataFromAddress(flat, tilebufpos, tileSize);
                }
            }
            globalshade = max(min(getSector_floorshade(s),numpalookups-1),0);
            globvis = globalhisibility;
            if (constsector[s].visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[s].visibility+16)));
            globalpolytype = 0;
            if ((globalorientation&64) == 0)
            {
                globalposx = dax;
                globalx1 = bakgxvect;
                globaly1 = bakgyvect;
                globalposy = day;
                globalx2 = bakgxvect;
                globaly2 = bakgyvect;
            }
            else
            {
                ox = getWall(getWall_point2(startwall))->x - getWall(startwall)->x;
                oy = getWall(getWall_point2(startwall))->y - getWall(startwall)->y;
                i = nsqrtasm(ox*ox+oy*oy);

                if (i == 0)
                    continue;

                i = 1048576/i;
                globalx1 = mulscale10(dmulscale10(ox,bakgxvect,oy,bakgyvect),i);
                globaly1 = mulscale10(dmulscale10(ox,bakgyvect,-oy,bakgxvect),i);
                ox = (bakx1>>4)-(xdim<<7);
                oy = (baky1>>4)-(ydim<<7);
                globalposx = dmulscale28(-oy,globalx1,-ox,globaly1);
                globalposy = dmulscale28(-ox,globalx1,oy,globaly1);
                globalx2 = -globalx1;
                globaly2 = -globaly1;

                daslope = getSector(s)->floorheinum;
                i = nsqrtasm(daslope*daslope+16777216);
                globalposy = mulscale12(globalposy,i);
                globalx2 = mulscale12(globalx2,i);
                globaly2 = mulscale12(globaly2,i);
            }
            globalxshift = (8-(picsiz[globalpicnum]&15));
            globalyshift = (8-(picsiz[globalpicnum]>>4));
            if (globalorientation&8) {
                globalxshift++;
                globalyshift++;
            }

            sethlinesizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,tilebufpos);

            if ((globalorientation&0x4) > 0)
            {
                i = globalposx;
                globalposx = -globalposy;
                globalposy = -i;
                i = globalx2;
                globalx2 = globaly1;
                globaly1 = i;
                i = globalx1;
                globalx1 = -globaly2;
                globaly2 = -i;
            }
            if ((globalorientation&0x10) > 0) globalx1 = -globalx1, globaly1 = -globaly1, globalposx = -globalposx;
            if ((globalorientation&0x20) > 0) globalx2 = -globalx2, globaly2 = -globaly2, globalposy = -globalposy;
            draws.asm1 = (globaly1<<globalxshift);
            draws.asm2 = (globalx2<<globalyshift);
            globalx1 <<= globalxshift;
            globaly2 <<= globalyshift;
            globalposx = (globalposx<<(20+globalxshift))+(((int32_t)getSector_floorxpanning(s))<<24);
            globalposy = (globalposy<<(20+globalyshift))-(((int32_t)sec->floorypanning)<<24);

            fillpolygon(npoints, tilebufpos);
        }
    }
    oldglobalbufplc = NULL;
    /* Sort sprite list */
    gap = 1;
    while (gap < sortnum) gap = (gap<<1)+1;
    for(gap>>=1; gap>0; gap>>=1)
        for(i=0; i<sortnum-gap; i++)
            for(j=i; j>=0; j-=gap)
            {
                if (getSprite(tsprite[j].owner)->_z <= getSprite(tsprite[j+gap].owner)->_z) break;
                // next-hack: done to let us use bitfields
                short owj = tsprite[j].owner;
                short owjgap = tsprite[j+gap].owner;
                swapshort(&owj,&owjgap);
                tsprite[j].owner = owj;
                tsprite[j+gap].owner=  owjgap;
            }
    for(s=sortnum-1; s>=0; s--)
    {
//        spritetypeoffs *spr_o = &sprite_offsets[tsprite[s].owner];
        if ((getSprite(tsprite[s].owner)->_cstat&48) == 32)
        {
            npoints = 0;

            tilenum = getSprite(tsprite[s].owner)->_picnum;
            xoff = (int32_t)((int8_t  )((tiles[tilenum].animFlags>>8)&255))+((int32_t)getSprite(tsprite[s].owner)->_xoffset);
            yoff = (int32_t)((int8_t  )((tiles[tilenum].animFlags>>16)&255))+((int32_t)getSprite(tsprite[s].owner)->_yoffset);

            if ((getSprite(tsprite[s].owner)->_cstat&4) > 0)
                xoff = -xoff;
            if ((getSprite(tsprite[s].owner)->_cstat&8) > 0)
                yoff = -yoff;

            k = getSprite(tsprite[s].owner)->_ang;
            cosang = sintable[(k+512)&2047];
            sinang = sintable[k];
            xspan = tiles[tilenum].dim.width;
            xrepeat = getSprite(tsprite[s].owner)->_xrepeat;
            yspan = tiles[tilenum].dim.height;
            yrepeat = getSprite(tsprite[s].owner)->_yrepeat;

            ox = ((xspan>>1)+xoff)*xrepeat;
            oy = ((yspan>>1)+yoff)*yrepeat;
            x1 = getSprite(tsprite[s].owner)->_x + mulscale(sinang,ox,16) + mulscale(cosang,oy,16);
            y1 = getSprite(tsprite[s].owner)->_y + mulscale(sinang,oy,16) - mulscale(cosang,ox,16);
            l = xspan*xrepeat;
            x2 = x1 - mulscale(sinang,l,16);
            y2 = y1 + mulscale(cosang,l,16);
            l = yspan*yrepeat;
            k = -mulscale(cosang,l,16);
            x3 = x2+k;
            x4 = x1+k;
            k = -mulscale(sinang,l,16);
            y3 = y2+k;
            y4 = y1+k;

            pvWalls[0].screenSpaceCoo[0].vec_colmap = 1;
            pvWalls[1].screenSpaceCoo[0].vec_colmap = 2;
            pvWalls[2].screenSpaceCoo[0].vec_colmap = 3;
            pvWalls[3].screenSpaceCoo[0].vec_colmap = 0;
            npoints = 4;

            i = 0;

            ox = x1 - dax;
            oy = y1 - day;
            x = dmulscale16(ox,xvect,-oy,yvect) + (xdim<<11);
            y = dmulscale16(oy,xvect2,ox,yvect2) + (ydim<<11);
            i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
            pvWalls[0].cameraSpaceCoo[0][VEC_X] = x;
            pvWalls[0].cameraSpaceCoo[0][VEC_Y] = y;

            ox = x2 - dax;
            oy = y2 - day;
            x = dmulscale16(ox,xvect,-oy,yvect) + (xdim<<11);
            y = dmulscale16(oy,xvect2,ox,yvect2) + (ydim<<11);
            i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
            pvWalls[1].cameraSpaceCoo[0][VEC_X] = x;
            pvWalls[1].cameraSpaceCoo[0][VEC_Y] = y;

            ox = x3 - dax;
            oy = y3 - day;
            x = dmulscale16(ox,xvect,-oy,yvect) + (xdim<<11);
            y = dmulscale16(oy,xvect2,ox,yvect2) + (ydim<<11);
            i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
            pvWalls[2].cameraSpaceCoo[0][VEC_X] = x;
            pvWalls[2].cameraSpaceCoo[0][VEC_Y] = y;

            x = pvWalls[0].cameraSpaceCoo[0][VEC_X]+pvWalls[2].cameraSpaceCoo[0][VEC_X]-pvWalls[1].cameraSpaceCoo[0][VEC_X];
            y = pvWalls[0].cameraSpaceCoo[0][VEC_Y]+pvWalls[2].cameraSpaceCoo[0][VEC_Y]-pvWalls[1].cameraSpaceCoo[0][VEC_Y];
            i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
            pvWalls[3].cameraSpaceCoo[0][VEC_X] = x;
            pvWalls[3].cameraSpaceCoo[0][VEC_Y] = y;

            if ((i&0xf0) != 0xf0) continue;
            bakx1 = pvWalls[0].cameraSpaceCoo[0][VEC_X];
            baky1 = mulscale16(pvWalls[0].cameraSpaceCoo[0][VEC_Y]-(ydim<<11),xyaspect)+(ydim<<11);
            if (i&0x0f)
            {
                npoints = clippoly(npoints,i);
                if (npoints < 3) continue;
            }

            globalpicnum = getSprite(tsprite[s].owner)->_picnum;
            if ((uint32_t)globalpicnum >= (uint32_t)MAXTILES)
                globalpicnum = 0;
            setgotpic(globalpicnum);

            if ((tiles[globalpicnum].dim.width <= 0) ||
                (tiles[globalpicnum].dim.height <= 0))
                continue;

            if ((tiles[globalpicnum].animFlags&192) != 0)
                globalpicnum += animateoffs(globalpicnum);

            TILE_MakeAvailable(globalpicnum);

            globalbufplc = tileData[globalpicnum];
            int tileSize;
            tileSize = tiles[globalpicnum].dim.height *tiles[globalpicnum].dim.width;
            if (oldglobalbufplc != globalbufplc)
            {
                tilebufpos = globalbufplc;
                oldglobalbufplc = globalbufplc;
                if (isOnExternalFlash(globalbufplc))
                {
                #if WIN32
                    if (tileSize> MAX_RAM_FLAT_SIZE)
                    {
                        FIXME("This is going wrong. Tile size is %d\r\n", tileSize);
                    }
                #endif
                    tilebufpos = extMemGetDataFromAddress(flat, tilebufpos, tileSize);
                }
            }
            if ((getSector(getSprite(tsprite[s].owner)->_sectnum)->ceilingstat&1) > 0)
                globalshade = (int32_t)getSector_ceilingshade(getSprite(tsprite[s].owner)->_sectnum);
            else
                globalshade = ((int32_t)getSector_floorshade(getSprite(tsprite[s].owner)->_sectnum));
            globalshade = max(min(globalshade+getSprite(tsprite[s].owner)->_shade+6,numpalookups-1),0);
            draws.asm3 = (intptr_t) palookup[getSprite(tsprite[s].owner)->_pal]+(globalshade<<8);
            globvis = globalhisibility;
// FIXME: next-hack: here the original source referenced to sec... (See commented below) which sec? the last of previous iteration? Shall it be tsprite[s]->sectnum or getSprite(tsprite[s].owner)->_sectnum ?
            // if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
#define TRY_FIXBUG 1
#if TRY_FIXBUG
            if (constsector[getSprite(tsprite[s].owner)->_sectnum].visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsector[getSprite(tsprite[s].owner)->_sectnum].visibility+16)));
#else
            if (constsec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t )(constsec->visibility+16)));
#endif
            globalpolytype = ((getSprite(tsprite[s].owner)->_cstat&2)>>1)+1;

            /* relative alignment stuff */
            ox = x2-x1;
            oy = y2-y1;
            i = ox*ox+oy*oy;
            if (i == 0) continue;
            i = (65536*16384)/i;
            globalx1 = mulscale10(dmulscale10(ox,bakgxvect,oy,bakgyvect),i);
            globaly1 = mulscale10(dmulscale10(ox,bakgyvect,-oy,bakgxvect),i);
            ox = y1-y4;
            oy = x4-x1;
            i = ox*ox+oy*oy;
            if (i == 0) continue;
            i = (65536*16384)/i;
            globalx2 = mulscale10(dmulscale10(ox,bakgxvect,oy,bakgyvect),i);
            globaly2 = mulscale10(dmulscale10(ox,bakgyvect,-oy,bakgxvect),i);

            ox = picsiz[globalpicnum];
            oy = ((ox>>4)&15);
            ox &= 15;
            if (pow2long[ox] != xspan)
            {
                ox++;
                globalx1 = mulscale(globalx1,xspan,ox);
                globaly1 = mulscale(globaly1,xspan,ox);
            }

            bakx1 = (bakx1>>4)-(xdim<<7);
            baky1 = (baky1>>4)-(ydim<<7);
            globalposx = dmulscale28(-baky1,globalx1,-bakx1,globaly1);
            globalposy = dmulscale28(bakx1,globalx2,-baky1,globaly2);

            if ((getSprite(tsprite[s].owner)->_cstat&2) == 0)
                msethlineshift(ox,oy);
            else
                tsethlineshift(ox,oy);

            if ((getSprite(tsprite[s].owner)->_cstat&0x4) > 0) globalx1 = -globalx1, globaly1 = -globaly1, globalposx = -globalposx;
            draws.asm1 = (globaly1<<2);
            globalx1 <<= 2;
            globalposx <<= (20+2);
            draws.asm2 = (globalx2<<2);
            globaly2 <<= 2;
            globalposy <<= (20+2);
            fillpolygon(npoints, tilebufpos);
        }
    }
}
void clearview(int32_t dacol)
{
    int32_t y, dx;
    uint8_t* p;

    if (qsetmode != 200) return;

    dx = windowx2-windowx1+1;
    dacol += (dacol<<8);
    dacol += (dacol<<16);

    p = frameplace+ylookup(windowy1)+windowx1;
    for(y=windowy1; y<=windowy2; y++)
    {
        clearbufbyte(p, dx, dacol);
        p += ylookup(1);
    }
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
}


void clearallviews(int32_t dacol)
{
    int32_t i;

    if (qsetmode != 200) return;
    dacol += (dacol<<8);
    dacol += (dacol<<16);

    switch(vidoption)
    {
    case 1:
        for(i=0; i<numpages; i++)
        {

            clearbufbyte((void *)frameplace,imageSize,0L);
        }

    case 2:
        clearbuf((void *)frameplace,(xdim*ydim)>>2,0L);
        break;
    }
#if FREQUENT_FAKETIMERHANDLER
    faketimerhandler();
#endif
}


void plotpixel(int32_t x, int32_t y, uint8_t  col)
{
    drawpixel(ylookup(y)+x+frameplace,(int32_t)col);
}


uint8_t  getpixel(int32_t x, int32_t y)
{
    return(readpixel(ylookup(y)+x+frameplace));
}

/* MUST USE RESTOREFORDRAWROOMS AFTER DRAWING */
int32_t setviewcnt = 0;
int32_t bakvidoption[4];
uint8_t* bakframeplace[4];
int32_t bakxsiz[4], bakysiz[4];
int32_t bakwindowx1[4], bakwindowy1[4];
int32_t bakwindowx2[4], bakwindowy2[4];

void setviewback(void)
{

    if (setviewcnt <= 0) return;
    setviewcnt--;

    setview(bakwindowx1[setviewcnt],bakwindowy1[setviewcnt],
            bakwindowx2[setviewcnt],bakwindowy2[setviewcnt]);
#if OLD
    copybufbyte(&bakumost[windowx1],&startumost[windowx1],(windowx2-windowx1+1)*sizeof(startumost[0]));
    copybufbyte(&bakdmost[windowx1],&startdmost[windowx1],(windowx2-windowx1+1)*sizeof(startdmost[0]));
#elif IS_THIS_NECESSARY
    int32_t i, j, k;
    for (int i = 0; i < (windowx2-windowx1+1); i++)
    {
        bakumost[windowx1 + i ] = startumost(windowx1 + i);
        bakdmost[windowx1 + i ] = startdmost(windowx1 + i);

    }
#endif
    vidoption = bakvidoption[setviewcnt];
    frameplace = bakframeplace[setviewcnt];
#if YLOOKUP_ARRAY
    if (setviewcnt == 0)
        k = bakxsiz[0];
    else
        k = max(bakxsiz[setviewcnt-1],bakxsiz[setviewcnt]);
    j = 0;
    for(i=0; i<=k; i++) ylookup(i) = j, j += bytesperline;
#else
    setYlookup(bytesperline);
#endif
    setBytesPerLine(bytesperline);
}





void preparemirror(int32_t dax, int32_t day, int32_t daz,
                   short daang, int32_t dahoriz, short dawall,
                   short dasector, int32_t *tposx, int32_t *tposy,
                   short *tang)
{
    UNUSED(daz);
    UNUSED(dahoriz);
    UNUSED(dasector);
    int32_t i, j, x, y, dx, dy;

    x = getWall(dawall)->x;
    dx = getWall(getWall_point2(dawall))->x-x;
    y = getWall(dawall)->y;
    dy = getWall(getWall_point2(dawall))->y-y;
    j = dx*dx + dy*dy;
    if (j == 0) return;
    i = (((dax-x)*dx + (day-y)*dy)<<1);
    *tposx = (x<<1) + scale(dx,i,j) - dax;
    *tposy = (y<<1) + scale(dy,i,j) - day;
    *tang = (((getangle(dx,dy)<<1)-daang)&2047);

    inpreparemirror = 1;
}


void completemirror(void)
{
    int32_t i, dy;
    intptr_t p;

    /* Can't reverse with uninitialized data */
    if (inpreparemirror) {
        inpreparemirror = 0;
        return;
    }
    if (mirrorsx1 > 0) mirrorsx1--;
    if (mirrorsx2 < windowx2-windowx1-1) mirrorsx2++;
    if (mirrorsx2 < mirrorsx1) return;

    transarea += (mirrorsx2-mirrorsx1)*(windowy2-windowy1);

    p = (intptr_t) frameplace+ylookup(windowy1+mirrorsy1)+windowx1+mirrorsx1;
    i = windowx2-windowx1-mirrorsx2-mirrorsx1;
    mirrorsx2 -= mirrorsx1;
    // FIX_00085: Optimized Video driver. FPS increases by +20%.
    for(dy=mirrorsy2-mirrorsy1-1; dy>=0; dy--)
    {
        copybufbyte((void *)(p),wallstempbuf,mirrorsx2+1);
        wallstempbuf[mirrorsx2] = wallstempbuf[mirrorsx2-1];
        copybufreverse(&wallstempbuf[mirrorsx2],(void *)(p+i),mirrorsx2+1);
        p += ylookup(1);
#if FREQUENT_FAKETIMERHANDLER
        faketimerhandler();
#endif
    }
}


int sectorofwall(short theline)
{
    int32_t i, gap;

    if ((theline < 0) || (theline >= numwalls)) return(-1);
    i = constwall[theline].nextwall;
    if (i >= 0) return(constwall[i].nextsector);

    gap = (numsectors>>1);
    i = gap;
    while (gap > 1)
    {
        gap >>= 1;
        if (constsector[i].wallptr < theline) i += gap;
        else i -= gap;
    }
    while (constsector[i].wallptr > theline) i--;
    while (constsector[i].wallptr+constsector[i].wallnum <= theline) i++;

    return(i);
}


int getceilzofslope(short sectnum, int32_t dax, int32_t day)
{
    int32_t dx, dy, i, j;
    walltype *wal;

    if (!(getSector(sectnum)->ceilingstat&2)) return(getSector(sectnum)->ceilingz);
    wal = getWall(constsector[sectnum].wallptr);
    dx = getWall(getWall_point2(constsector[sectnum].wallptr))->x-wal->x;
    dy = getWall(getWall_point2(constsector[sectnum].wallptr))->y-wal->y;
    i = (nsqrtasm(dx*dx+dy*dy)<<5);
    if (i == 0) return(getSector(sectnum)->ceilingz);
    j = dmulscale3(dx,day-wal->y,-dy,dax-wal->x);
    return(getSector(sectnum)->ceilingz+scale(constsector[sectnum].ceilingheinum,j,i));
}


int getflorzofslope(short sectnum, int32_t dax, int32_t day)
{
    int32_t dx, dy, i, j;
    walltype *wal;

    if (!(getSector(sectnum)->floorstat&2))
        return(getSector(sectnum)->floorz);

    wal = getWall(constsector[sectnum].wallptr);
    dx = getWall(getWall_point2(constsector[sectnum].wallptr))->x-wal->x;
    dy = getWall(getWall_point2(constsector[sectnum].wallptr))->y-wal->y;
    i = (nsqrtasm(dx*dx+dy*dy)<<5);

    if (i == 0)
        return(getSector(sectnum)->floorz);

    j = dmulscale3(dx,day-wal->y,-dy,dax-wal->x);

    return(getSector(sectnum)->floorz+scale(getSector(sectnum)->floorheinum,j,i));
}

/*
 FCS:

 Output the ceiling and floor Z coordinate in the two last parameters for given:
 sectorNumber and worldspace (coordinate X,Y).

 If the sector is flat, this is just a lookup. But if either the floor/ceiling have
 a slope it requires more calculation

 */
void getzsofslope(short sectnum, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    int32_t dx, dy, i, j;
    walltype *wal, *wal2;
    const sectortype *sec;

    sec = getSector(sectnum);
    *ceilz = sec->ceilingz;
    *florz = sec->floorz;

    //If the sector has a slopped ceiling or a slopped floor then it needs more calculation.
    if ((sec->ceilingstat|sec->floorstat)&2)
    {
        wal = getWall(constsector[sectnum].wallptr);
        wal2 = getWall(constwall[constsector[sectnum].wallptr]._point2);//getWall_point2(constsector[sectnum].wallptr));
        dx = wal2->x-wal->x;
        dy = wal2->y-wal->y;
        i = (nsqrtasm(dx*dx+dy*dy)<<5);
        if (i == 0) return;
        j = dmulscale3(dx,day-wal->y,-dy,dax-wal->x);

        if (sec->ceilingstat&2)
            *ceilz = (*ceilz)+scale(constsector[sectnum].ceilingheinum,j,i);
        if (sec->floorstat&2)
            *florz = (*florz)+scale(sec->floorheinum,j,i);
    }
}

#if CODE_NOT_USED
void alignceilslope(short dasect, int32_t x, int32_t y, int32_t z)
{
    int32_t i, dax, day;
    walltype *wal;

    wal = getWall(sector[dasect].wallptr);
    dax = getWall(getWall_point2(sector[dasect].wallptr))->x-wal->x;
    day = getWall(getWall_point2(sector[dasect].wallptr))->y-wal->y;

    i = (y-wal->y)*dax - (x-wal->x)*day;
    if (i == 0) return;
    sector[dasect].ceilingheinum = scale((z-sector[dasect].ceilingz)<<8,
                                         nsqrtasm(dax*dax+day*day),i);

    if (sector[dasect].ceilingheinum == 0) sector[dasect].ceilingstat &= ~2;
    else sector[dasect].ceilingstat |= 2;
}
#endif

void alignflorslope(short dasect, int32_t x, int32_t y, int32_t z)
{
    int32_t i, dax, day;
    walltype *wal;

    wal = getWall(constsector[dasect].wallptr);
    dax = getWall(getWall_point2(constsector[dasect].wallptr))->x-wal->x;
    day = getWall(getWall_point2(constsector[dasect].wallptr))->y-wal->y;

    i = (y-wal->y)*dax - (x-wal->x)*day;
    if (i == 0) return;
    setSector_floorheinum(dasect, scale((z-getSector(dasect)->floorz)<<8,
                                       nsqrtasm(dax*dax+day*day),i));

    if (getSector(dasect)->floorheinum == 0)
        setSector_floorstat(dasect, getSector(dasect)->floorstat & ~2);
    else
        setSector_floorstat(dasect, getSector(dasect)->floorstat | 2);
}

/*
 FCS:
 Search for ???
*/
int loopnumofsector(short sectnum, short wallnum)
{
    int32_t i, numloops, startwall, endwall;

    numloops = 0;

    startwall = constsector[sectnum].wallptr;
    endwall = startwall + constsector[sectnum].wallnum;

    for(i=startwall; i<endwall; i++)
    {
        if (i == wallnum)
            return(numloops);

        if (getWall_point2(i) < i)
            numloops++;
    }
    return(-1);
}
#if UNUSED_CODE

void setfirstwall(short sectnum, short newfirstwall)
{
    int32_t i, j, k, numwallsofloop;
    int32_t startwall, endwall, danumwalls, dagoalloop;

    startwall = getSector(sectnum)->wallptr;
    danumwalls = getSector(sectnum)->wallnum;
    endwall = startwall+danumwalls;
    if ((newfirstwall < startwall) || (newfirstwall >= startwall+danumwalls)) return;
    for(i=0; i<danumwalls; i++)
        memcpy(&wall[i+numwalls],&wall[i+startwall],sizeof(walltype));

    numwallsofloop = 0;
    i = newfirstwall;
    do
    {
        numwallsofloop++;
        i = getWall_point2(i);
    } while (i != newfirstwall);

    /* Put correct loop at beginning */
    dagoalloop = loopnumofsector(sectnum,newfirstwall);
    if (dagoalloop > 0)
    {
        j = 0;
        while (loopnumofsector(sectnum,j+startwall) != dagoalloop) j++;
        for(i=0; i<danumwalls; i++)
        {
            k = i+j;
            if (k >= danumwalls) k -= danumwalls;
            memcpy(&wall[startwall+i],&wall[numwalls+k],sizeof(walltype));

            setWall_point2(startwall+i, getWall_point2(startwall+i) + danumwalls-startwall-j); //wall[startwall+i].point2 += danumwalls-startwall-j;
            if (getWall_point2(startwall+i) >= danumwalls)
                setWall_point2(startwall+i, getWall_point2(startwall+i) - danumwalls);  //                wall[startwall+i].point2 -= danumwalls;
              setWall_point2(startwall+i, getWall_point2(startwall+i) + startwall);  //wall[startwall+i].point2 += startwall;
        }
        newfirstwall += danumwalls-j;
        if (newfirstwall >= startwall+danumwalls) newfirstwall -= danumwalls;
    }

    for(i=0; i<numwallsofloop; i++)
        memcpy(&wall[i+numwalls],&wall[i+startwall],sizeof(walltype));
    for(i=0; i<numwallsofloop; i++)
    {
        k = i+newfirstwall-startwall;
        if (k >= numwallsofloop) k -= numwallsofloop;
        memcpy(&wall[startwall+i],&wall[numwalls+k],sizeof(walltype));

        setWall_point2(startwall+i, numwallsofloop-newfirstwall);
        if (getWall_point2(startwall+i) >= numwallsofloop)
            setWall_point2(startwall+i, getWall_point2(startwall+i) -numwallsofloop);
        setWall_point2(startwall+i, getWall_point2(startwall+i) + startwall);
    }

    for(i=startwall; i<endwall; i++)
        if (wall[i].nextwall >= 0) wall[wall[i].nextwall].nextwall = i;
}
#endif

//#pragma GCC optimize ("Ofast")  // we need to compile this code to be as fast as possible.

void initengine(void)
{
#if LOAD_TABLES
    loadtables();
#endif
    xyaspect = -1;

    pskyoff[0] = 0;
    pskybits = 0;

    parallaxtype = 2;
    parallaxyoffs = 0L;
    parallaxyscale = 65536;
    showinvisibility = 0;

    paletteloaded = 0;

    searchit = 0;
    searchstat = -1;

#if !USE_EXT_MEM
    int32_t i;
    for(i=0; i<MAXPALOOKUPS; i++)
        palookup[i] = NULL;
    for(i=0 ; i < MAXTILES ; i++)
        tiles[i].data = NULL;

#endif

    clearbuf(&show2dsector[0],(int32_t)((MAXSECTORS+3)>>5),0L);
    clearbuf(&show2dsprite[0],(int32_t)((MAXSPRITES+3)>>5),0L);
    clearbuf(&show2dwall[0],(int32_t)((MAXWALLS+3)>>5),0L);
    automapping = 0;
#if USE_MANY_MODES
    validmodecnt = 0;
#endif
    pointhighlight = -1;
    linehighlight = -1;
    highlightcnt = 0;

    totalclock = 0;
    visibility = 512;
    parallaxvisibility = 512;

    loadpalette();
}
#if TRYING_SPEEDUP_RAMWALLS
void setStackWallIndexes(uint16_t *stackWall)
{
    stackWallIndexes = stackWall;
}
#endif
/* end of engine.c ... */
