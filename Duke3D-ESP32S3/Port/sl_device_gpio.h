/***************************************************************************//**
 * @file sl_device_gpio.h
 * @brief This is used to get rid of sl_device_gpio.c bloatware.
 * In particular this removes the hardcoded not-removable IRQ handler
 * (sl_gpio.c is included just by including RAIL and you cannot remove it)
 * and this would waste some hundreds of bytes just for (unused by the game)
 * GPIO IRQ callbacks.
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


#ifndef PORT_SL_DEVICE_GPIO_H_
#define PORT_SL_DEVICE_GPIO_H_
#define GPIO_EVEN_IRQHandler disabled_GPIO_EVEN_IRQHandler
#define GPIO_ODD_IRQHandler disabled_GPIO_ODD_IRQHandler
#if !SDK_IS_2024_6_0
 #include_next "sl_device_gpio.h"
#endif
#endif /* PORT_SL_DEVICE_GPIO_H_ */
