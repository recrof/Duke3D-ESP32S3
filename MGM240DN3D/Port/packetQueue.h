/***************************************************************************//**
 * @file packetQueue.h
 * @brief Simple queue for packets.
 *
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
#ifndef PACKETQUEUE_H_
#define PACKETQUEUE_H_

#include <stdint.h>
#include <string.h>
// this is for CORE_ENTER/EXIT atomic.
#include "sl_core.h"
#define QUEUE_OK                0
#define QUEUE_FULL              1
#define QUEUE_EMPTY             2
#define QUEUE_BUFFER_TOO_SMALL  3

typedef struct
{
    volatile int writePos;
    volatile int readPos;
    volatile int dataLength;
    uint32_t bufferSize;   // sizeof the buffer passed to data
    volatile uint8_t *data;
    volatile int firstPacketIndex;               // packet counter of the first packet in the queue
    // used to peek
    volatile int peekReadPos;           // current peek read position
    volatile int peekDataLength;        // how much data is left to peek

} packetQueue_t;

void pQinitQueue(packetQueue_t * queue, uint8_t *buffer, int size);
int pQenqueuePacket(packetQueue_t * queue, uint32_t size, uint8_t *buffer);
int pQdequeuePacket(packetQueue_t * queue, int maxsize, uint8_t *buffer, uint32_t *readSize);
int pQdiscardOldestPacket(packetQueue_t *queue);
int pQpeekPacket(packetQueue_t * queue, int maxsize, uint8_t *buffer, uint32_t *readSize);
void pQstartPeek(packetQueue_t * queue);
int pQenqueuePacketWithIdAndLength(packetQueue_t * queue, uint32_t size, uint8_t *buffer, uint8_t id);
int pQenqueuePacketAndAppendCRC(packetQueue_t * queue, uint32_t size, uint8_t *buffer);
int pQdequeuePacketWithIdAndLen(packetQueue_t * queue, int maxsize, uint8_t *buffer, uint32_t *readSize, uint8_t *clientId);
#endif /* PACKETQUEUE_H_ */
