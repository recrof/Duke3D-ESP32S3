//
//  global.h
//  Duke3D
//
//  Created by fabien sanglard on 12-12-17.
//  Copyright (c) 2012 fabien sanglard. All rights reserved.
//

#ifndef Duke3D_global_h
#define Duke3D_global_h

#include <stdint.h>

void FixFilePath(char  *filename);
int FindDistance3D(int ix, int iy, int iz);
void Shutdown(void);

#ifndef LITTLE_ENDIAN
    #ifdef __APPLE__
    #else
        #define LITTLE_ENDIAN 1234
    #endif
#endif

#ifndef BIG_ENDIAN
     #ifdef __APPLE__
     #else
        #define BIG_ENDIAN 4321
    #endif
#endif

#define BYTE_ORDER LITTLE_ENDIAN


#if (BYTE_ORDER == LITTLE_ENDIAN)
#define KeepShort IntelShort
#define SwapShort MotoShort
#define Keepint32_t IntelLong
#define Swapint32_t MotoLong
#else
#define KeepShort MotoShort
#define SwapShort IntelShort
#define Keepint32_t MotoLong
#define Swapint32_t IntelLong
#endif

int32_t MotoLong (int32_t l);
int32_t IntelLong (int32_t l);

#if WIN32
#define Error(t, f,...) Error2(t, __FILE__, __FUNCTION__, __LINE__,  f, ##__VA_ARGS__)
void   Error2 (int errorType, const char *file, const char *function, int line, const char  *error, ...);
#else
void Error (int errorType, const char *error, ...);
#endif

#endif
