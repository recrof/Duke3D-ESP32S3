/*
 *  CRC32 for settings validation (the MG24 used its GPCRC peripheral).
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include "crc.h"
#include "esp_rom_crc.h"
#define DUKE_NUKEM_3D_CRC_INPUT         0x44334E44 // "DN3D" in little endian.
void crc32Init(void)
{
}
uint32_t crc32Compute(uint8_t *payload, uint32_t length)
{
    const uint32_t seed = DUKE_NUKEM_3D_CRC_INPUT;
    uint32_t crc = esp_rom_crc32_le(0, (const uint8_t*) &seed, sizeof(seed));
    return esp_rom_crc32_le(crc, payload, length);
}
