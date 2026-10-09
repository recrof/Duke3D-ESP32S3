/*
 *  External memory for the ESP32-S3 port.
 *
 *  Virtual layout (offsets, without the EXT_FLASH_BASE tag):
 *    [4, 4 + GRP size)                          DUKE3D.GRP, read from the SD card
 *                                               through a PSRAM page cache
 *    [EXT_MEMORY_SIZE - NVM size, EXT_MEMORY_SIZE) settings, savegames and the
 *                                               tile-pointer cache, stored in the
 *                                               "dukenvm" flash partition with NOR
 *                                               semantics (erase to 0xFF, program
 *                                               clears bits), as on the MG24 board.
 *  The layout is fixed so pointers cached in NVM stay valid across boots.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef SRC_EXTMEMORY_H_
#define SRC_EXTMEMORY_H_
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "main.h"
#include "boards.h"
#include "i_memory.h"
//
#define EXT_MEMORY_SIZE                         0x08000000UL      // 128 MB virtual
#define EXT_MEMORY_ADDRESS_MASK                 0x3FFFFFFFUL
#define EXT_MEMORY_HEADER_SIZE                  0
#define EXT_MEMORY_READ_ALIGN_SIZE              4
//
// Boot-time setup. Returns false (with a message on the boot console) if the
// SD card or the GRP file cannot be used.
bool extMemInit(void);
uint32_t extMemGetGrpSize(void);
//
void* extMemGetDataFromAddress(void *dest, const void *src, unsigned int length);
void* extMemGetDataFromCurrentAddress(void *dest, unsigned int length);
void* extMemGetDataAligned(void *dest, unsigned int length);
void extMemSetCurrentAddress(uint32_t address);
uint8_t extMemGetByteFromAddress(const void *addr);
short extMemFlashGetShortFromAddress(const void *addr);
//
// Reads are synchronous; the asynchronous API is kept for the engine.
void* extMemStartAsynchDataRead(uint32_t address, void *dest, uint32_t cnt);
static inline void extMemWaitAsynchDataRead(void)
{
}
static inline int extMemHasAsynchDataReadFinished(void)
{
    return 1;
}
static inline void extMemStopDMA(void)
{
}
static inline void extMemRestoreInterface(void)
{
}
static inline int extMemGetRemainingBytes(void)
{
    return 0;
}
// Called while the game is blocked on slow storage (e.g. while saving) to keep
// the audio ring fed.
void extMemSetMemBusyBackgroundTask(void *task);
//
static inline int extMemGetSize(void)
{
    return EXT_MEMORY_SIZE;
}
void extMemErase(uint32_t address, uint32_t size);
void extMemProgram(uint32_t address, uint8_t *buffer, uint32_t size);
void extMemWrite(uint32_t address, void *buffer, uint32_t size);
void extMemEraseNvm(void);
#endif /* SRC_EXTMEMORY_H_ */
