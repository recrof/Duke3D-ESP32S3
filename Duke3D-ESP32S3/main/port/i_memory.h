/*
 *  Memory map definitions for the ESP32-S3 port.
 *
 *  "External memory" is a virtual address space, never dereferenced directly:
 *  the engine reads it through the extMem* functions. Addresses carry the
 *  EXT_FLASH_BASE tag bit so they can be told apart from real pointers (on the
 *  ESP32-S3 data lives at 0x3Cxxxxxx/0x3Fxxxxxx, which never has bit 30 set).
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#ifndef I_MEMORY_H_
#define I_MEMORY_H_
#include <stdint.h>
#define EXT_FLASH_BASE                        0x40000000UL
#define GRP_ADDRESS                           ((uint8_t*)(EXT_FLASH_BASE + 4))
extern uint8_t                                *ext_memory;
#ifndef isOnExternalFlash
#define isOnExternalFlash(a)                  ((((uint32_t) (a)) & EXT_FLASH_BASE) == EXT_FLASH_BASE)
#endif
#endif /* I_MEMORY_H_ */
