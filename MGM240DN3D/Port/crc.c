/***************************************************************************//**
 * @file crc.c
 * @brief  Simple CRC helper for DN3D.
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
#include "em_gpcrc.h"
#include "crc.h"
#include "em_cmu.h"
#define DUKE_NUKEM_3D_CRC_INPUT         0x44334E44 // "DN3D" in little endian.
void crc32Init(void)
{
    CORE_irqState_t irqState;
    CORE_ENTER_ATOMIC();
    CMU_ClockEnable(cmuClock_GPCRC, true);
    GPCRC->CTRL = GPCRC_CTRL_AUTOINIT | GPCRC_CTRL_POLYSEL_CRC32 ;
    GPCRC->INIT = 0;
    GPCRC->EN = GPCRC_EN_EN;
    GPCRC->CMD = GPCRC_CMD_INIT;
    CORE_EXIT_ATOMIC();
}
uint32_t crc32Compute(uint8_t *payload, uint32_t length)
{
    CORE_irqState_t irqState;
    CORE_ENTER_ATOMIC();
    GPCRC->CMD = GPCRC_CMD_INIT;
    GPCRC->INPUTDATA = DUKE_NUKEM_3D_CRC_INPUT;
    for (uint32_t i = 0; i < length; i++)
    {
        GPCRC->INPUTDATABYTE = payload[i];
    }
    uint32_t crc32 = GPCRC->DATA;
    CORE_EXIT_ATOMIC();
    return crc32;
}
