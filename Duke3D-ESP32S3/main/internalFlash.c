/*
 *  "Internal flash" for the ESP32-S3 port.
 *
 *  On the MG24 the engine caches read-only level data (tile tables, CON code,
 *  flats...) into the MCU's internal flash, a bump allocator that is read
 *  back through ordinary pointers. Here the same allocator lives in a PSRAM
 *  arena, so "programming" is a plain copy. The arena is aligned to
 *  FLASH_PAGE_SIZE because the level loader page-aligns pointers into it.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include <stdlib.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "maincfg.h"
#include "internalFlash.h"
#include "extMemory.h"
#include "board.h"

static uint8_t *arena;
static uint32_t pointer;
static uint32_t commonSize;

static void copyIn(void *dest, const void *src, int size)
{
    if (isOnExternalFlash(src))
    {
        extMemGetDataFromAddress(dest, src, size);
    }
    else
    {
        memcpy(dest, src, size);
    }
}
void internalFlashInit(void)
{
    if (!arena)
    {
        arena = heap_caps_aligned_alloc(FLASH_PAGE_SIZE, INTERNAL_FLASH_ARENA_SIZE, MALLOC_CAP_SPIRAM);
        if (!arena)
        {
            while (1)
            {
                FIXME("Cannot allocate the %d byte level cache in PSRAM\r\n", INTERNAL_FLASH_ARENA_SIZE);
            }
        }
        memset(arena, 0xFF, INTERNAL_FLASH_ARENA_SIZE);
    }
    pointer = 0;
    commonSize = 0;
}
void eraseInternalFlash(int sections)
{
    UNUSED(sections);
    memset(arena, 0xFF, INTERNAL_FLASH_ARENA_SIZE);
}
void flashErasePage(uint32_t *pageAddress)
{
    uint8_t *page = (uint8_t*) ((uint32_t) pageAddress & ~(FLASH_PAGE_SIZE - 1));
    if (page >= arena && page < arena + INTERNAL_FLASH_ARENA_SIZE)
    {
        memset(page, 0xFF, FLASH_PAGE_SIZE);
    }
}
void * storeToInternalFlash(const void *buffer, int size)
{
    if (size & 3)
    {
        FIXME("UNALIGNED STORE TO INTERNAL FLASH (%d)\r\n", size);
    }
    if (pointer + size > INTERNAL_FLASH_ARENA_SIZE)
    {
        while (1)
        {
            FIXME("FLASH OVERFLOW: need %d, %d left\r\n", (int) size, (int) (INTERNAL_FLASH_ARENA_SIZE - pointer));
        }
    }
    uint8_t *ptr = arena + pointer;
    copyIn(ptr, buffer, size);
    pointer += size;
    return ptr;
}
void * storeToInternalFlashAtPointer(const void *buffer, const void *flashPos, int size)
{
    if ((const uint8_t*) flashPos < arena || (const uint8_t*) flashPos + size > arena + INTERNAL_FLASH_ARENA_SIZE)
    {
        while (1)
        {
            FIXME("Store outside the level cache: %p, %d bytes\r\n", flashPos, size);
        }
    }
    copyIn((void*) flashPos, buffer, size);
    return (void*) flashPos;
}
void storeToInternalFlashAndFree(void **buffer, int size)
{
    void *ptr = *buffer;
    *buffer = storeToInternalFlash(*buffer, size);
    free(ptr);
}
void * storeToInternalFlashAlign(void *buffer, int size)
{
    return storeToInternalFlash(buffer, (size + 3) & ~3);
}
void *getCurrentInternalFlashPtr(void)
{
    return arena + pointer;
}
void *reserveInternalFlashSize(uint32_t size)
{
    if (pointer + size > INTERNAL_FLASH_ARENA_SIZE)
    {
        while (1)
        {
            FIXME("FLASH OVERFLOW: need %d, %d left\r\n", (int) size, (int) (INTERNAL_FLASH_ARENA_SIZE - pointer));
        }
    }
    uint8_t *ptr = arena + pointer;
    memset(ptr, 0xFF, size);
    pointer += size;
    return ptr;
}
// called after common stuff has been initialized
void internalFlashSetCommonZone(void)
{
    commonSize = (pointer + FLASH_PAGE_SIZE - 1) & ~(FLASH_PAGE_SIZE - 1);
}
void internalFlashResetToCommonZoneEnd(void)
{
    pointer = commonSize;
}
int internalFlashSpaceLeft(void)
{
    return INTERNAL_FLASH_ARENA_SIZE - pointer;
}
