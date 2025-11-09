/***************************************************************************//**
 * @file ext_memory.h
 * @brief Wrapper for various external memory organization support. This
 *        is used to support both WIN32 development.
 *
 * This is used to support external memory, even if it is not memory mapped.
 *
 *  Copyright (C) 2022-2025 by Nicola Wrachien
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 ******************************************************************************/
#ifndef EXT_MEMORY_H
#define EXT_MEMORY_H
#if WIN32
#include <stdint.h>
#define USE_EXT_MEMORY 1
#ifdef WIN32
#define EXT_MEMORY_SIZE (32 * 1024 * 1024)
#define extMemGetSize() EXT_MEMORY_SIZE
#define EXT_MEMORY_PAGE_SIZE    4096
#define extMemGetDataAligned extMemGetDataFromCurrentAddress
extern uint8_t ext_memory[EXT_MEMORY_SIZE];
void extMemProgram(uint32_t address, uint8_t *buffer, uint32_t size);
void* extMemGetDataFromAddress(void *dest, void *source, unsigned int length);
uint8_t extMemGetByteFromCurrentAddress(void);
uint16_t extMemGetHalfFromCurrentAddress(void);
uint32_t extMemGetWordFromCurrentAddress(void);

uint8_t extMemGetByteFromAddress(void *address);
void* extMemGetDataFromCurrentAddress(void *dest, unsigned int length);
void extMemSetCurrentAddress(void *address);

static inline int isOnExternalFlash(void *address)
{
    return ( (uint32_t) address >=  (uint32_t) ext_memory && (uint32_t) address < (uint32_t) ext_memory + EXT_MEMORY_SIZE);
}
static inline int extMemGetRemainingBytes(void)
{
    return 0;
}
static inline void extMemErase(uint32_t address, uint32_t size)
{
    address &= EXT_MEMORY_SIZE - 1;
    size = (size + EXT_MEMORY_PAGE_SIZE) & ~( EXT_MEMORY_PAGE_SIZE - 1);
    address &=  ~( EXT_MEMORY_PAGE_SIZE - 1);
    memset (&ext_memory[address], 0xFF, size);
}
#define EXT_MEM_BYTE_TIME   1
#define EXT_MEM_ACCESS_TIME 65
static inline void extMemStopDMA(void)
{

}
uint8_t extMemGetDMAByte(void);
#else
extern uint8_t *ext_memory;
#endif
#else
#include "extMemory.h"
#endif
#endif // EXT_MEMORY_H
