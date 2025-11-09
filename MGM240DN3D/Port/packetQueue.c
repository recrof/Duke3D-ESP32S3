/***************************************************************************//**
 * @file packetQueue.c
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
#include "packetQueue.h"
#include "crc.h"
#define SUBPACKET_CRC_SIZE 4
int pQenqueuePacketAndAppendCRC(packetQueue_t * queue, uint32_t size, uint8_t *buffer)
{
    CORE_irqState_t irqState;
    CORE_ENTER_ATOMIC();
    if (queue->dataLength + size + 1 + SUBPACKET_CRC_SIZE > queue->bufferSize) // +1 accounts for the first byte telling the packet size.
    {
        CORE_EXIT_ATOMIC();
        return QUEUE_FULL;
    }
    // calculate CRC size
    uint32_t crc32 = crc32Compute(buffer, size);
    queue->dataLength += size + 1 + SUBPACKET_CRC_SIZE;      // +1 accounts for the packet size byte
    uint32_t nextIdx;
    nextIdx = queue->writePos;
    queue->data[nextIdx++] = size + SUBPACKET_CRC_SIZE; // the first byte accounts for size, excluding the size of this byte
    if (nextIdx >= queue->bufferSize)
    {
        nextIdx = 0;
    }
    while (size--)
    {
        queue->data[nextIdx] = *buffer++;
        nextIdx++;
        if (nextIdx >= queue->bufferSize)
        {
            nextIdx = 0;
        }
    }
    // finally, CRC size
    buffer = (uint8_t*) &crc32;
    //
    size = SUBPACKET_CRC_SIZE;
    //
    while (size--)
    {
        queue->data[nextIdx] = *buffer++;
        nextIdx++;
        if (nextIdx >= queue->bufferSize)
        {
            nextIdx = 0;
        }
    }
    // finally write CRC
    queue->writePos = nextIdx;
    CORE_EXIT_ATOMIC();
    return QUEUE_OK;
}

int pQenqueuePacket(packetQueue_t * queue, uint32_t size, uint8_t *buffer)
{
    CORE_irqState_t irqState;
    CORE_ENTER_ATOMIC();
    if (queue->dataLength + size + 1 > queue->bufferSize) // +1 accounts for the first byte telling the packet size.
    {
        CORE_EXIT_ATOMIC();
        return QUEUE_FULL;
    }
    queue->dataLength += size + 1;      // +1 accounts for the packet size byte
    uint32_t nextIdx;
    nextIdx = queue->writePos;
    queue->data[nextIdx++] = size; // the first byte accounts for size, excluding the size of this byte
    if (nextIdx >= queue->bufferSize)
    {
        nextIdx = 0;
    }
    while (size--)
    {
        queue->data[nextIdx] = *buffer++;
        nextIdx++;
        if (nextIdx >= queue->bufferSize)
        {
            nextIdx = 0;
        }
    }
    queue->writePos = nextIdx;
    CORE_EXIT_ATOMIC();
    return QUEUE_OK;
}
int pQenqueuePacketWithIdAndLength(packetQueue_t * queue, uint32_t size, uint8_t *buffer, uint8_t id)
{
    CORE_irqState_t irqState;
    CORE_ENTER_ATOMIC();
    if (queue->dataLength + size + 3 > queue->bufferSize) // +1 accounts for the first byte telling the packet size.
    {
        CORE_EXIT_ATOMIC();
        return QUEUE_FULL;
    }
    queue->dataLength += size + 1 + 2;      // +1 accounts for the packet size byte. +2 is for packet size and id
    uint32_t nextIdx;
    nextIdx = queue->writePos;
    queue->data[nextIdx++] = size + 2; // the first byte accounts for size, excluding the size of this byte, but including id and length
    if (nextIdx >= queue->bufferSize)
    {
        nextIdx = 0;
    }
    // again size.
    queue->data[nextIdx++] = size; // the first byte accounts for size, excluding the size of this byte
    if (nextIdx >= queue->bufferSize)
    {
        nextIdx = 0;
    }
    // now id
    queue->data[nextIdx++] = id; // the first byte accounts for size, excluding the size of this byte
    if (nextIdx >= queue->bufferSize)
    {
        nextIdx = 0;
    }

    while (size--)
    {
        queue->data[nextIdx] = *buffer++;
        nextIdx++;
        if (nextIdx >= queue->bufferSize)
        {
            nextIdx = 0;
        }
    }
    queue->writePos = nextIdx;
    CORE_EXIT_ATOMIC();
    return QUEUE_OK;
}

void pQstartPeek(packetQueue_t * queue)
{
    queue->peekDataLength = queue->dataLength;
    queue->peekReadPos = queue->readPos;
}
// peek but do not remove packets. Note, before a packet is peeked, startPeek must be called. Also startPeek and peek must be both in the same atomic section.
int pQpeekPacket(packetQueue_t * queue, int maxsize, uint8_t *buffer, uint32_t *readSize)
{
    if (maxsize <= 0)
    {
        return QUEUE_BUFFER_TOO_SMALL;
    }
    if (queue->peekDataLength == 0)
    {
        *readSize = 0;
        return QUEUE_EMPTY;
    }
    uint32_t nextIdx;
    nextIdx = queue->peekReadPos;
    // first read packet size.
    uint8_t size = queue->data[nextIdx];
    if (size > maxsize)
    {
        *readSize = 0;
        return QUEUE_BUFFER_TOO_SMALL;
    }

    *readSize = size;
    nextIdx ++;
    if (nextIdx >= queue->bufferSize)
    {
        nextIdx = 0;
    }
    queue->peekDataLength -= size + 1; // remove size+1 from stored data len.
    // TODO: Assert if peekDataLength < 0
    while (size--)
    {
        *buffer++ = queue->data[nextIdx];
        nextIdx ++;
        if (nextIdx >= queue->bufferSize)
        {
            nextIdx = 0;
        }
    }
    queue->peekReadPos = nextIdx;
    return QUEUE_OK;
}
// quickly discard the oldest packet (useful for already peeked ones)
int pQdiscardOldestPacket(packetQueue_t *queue)
{
       CORE_irqState_t irqState;
       CORE_ENTER_ATOMIC();
       if (queue->dataLength == 0)
       {
           return QUEUE_EMPTY;
       }
       uint32_t nextIdx = queue->readPos;
       // first read packet size.
       uint8_t size = queue->data[nextIdx];

       nextIdx += 1 + size;
       if (nextIdx >= queue->bufferSize)
       {
           queue->readPos = nextIdx - queue->bufferSize;
       }
       else
       {
           queue->readPos = nextIdx;
       }
       queue->dataLength -= size + 1; // remove size+1 from stored data len.
       queue->firstPacketIndex++;
       // TODO: Assert if datalength < 0
       CORE_EXIT_ATOMIC();
       return QUEUE_OK;
}

int pQdequeuePacketWithIdAndLen(packetQueue_t * queue, int maxsize, uint8_t *buffer, uint32_t *readSize, uint8_t *clientId)
{
    if (maxsize <= 0)
    {
        return QUEUE_BUFFER_TOO_SMALL;
    }

    CORE_irqState_t irqState;
    CORE_ENTER_ATOMIC();
    if (queue->dataLength == 0)
    {
        *readSize = 0;
        CORE_EXIT_ATOMIC();
        return QUEUE_EMPTY;
    }
    uint32_t nextIdx;
    nextIdx = queue->readPos;
    // first read packet size.
    uint8_t size = queue->data[nextIdx++];
    // note: remove this duplicate crap
    if (size > maxsize + 2)
    {
        *readSize = 0;
        CORE_EXIT_ATOMIC();
        return QUEUE_BUFFER_TOO_SMALL;
    }
    if (nextIdx >= queue->bufferSize)
    {
        nextIdx = 0;
    }
    // readsize is the actual subpacket length
    *readSize = queue->data[nextIdx++];
    if (nextIdx >= queue->bufferSize)
    {
        nextIdx = 0;
    }
    // read the id the packet came from
    *clientId = queue->data[nextIdx++];
    if (nextIdx >= queue->bufferSize)
    {
        nextIdx = 0;
    }
    queue->dataLength -= size + 1; // remove size+1 from stored data len. Size included + 2.
    size -= 2;
    // TODO: Assert if datalength < 0
    while (size--)
    {
        *buffer++ = queue->data[nextIdx++];
        if (nextIdx >= queue->bufferSize)
        {
            nextIdx = 0;
        }
    }
    queue->readPos = nextIdx;
    queue->firstPacketIndex++;
    CORE_EXIT_ATOMIC();
    return QUEUE_OK;
}
int pQdequeuePacket(packetQueue_t * queue, int maxsize, uint8_t *buffer, uint32_t *readSize)
{
    if (maxsize <= 0)
    {
        return QUEUE_BUFFER_TOO_SMALL;
    }

    CORE_irqState_t irqState;
    CORE_ENTER_ATOMIC();
    if (queue->dataLength == 0)
    {
        *readSize = 0;
        CORE_EXIT_ATOMIC();
        return QUEUE_EMPTY;
    }
    uint32_t nextIdx;
    nextIdx = queue->readPos;
    // first read packet size.
    uint8_t size = queue->data[nextIdx++];
    if (size > maxsize)
    {
        *readSize = 0;
        CORE_EXIT_ATOMIC();
        return QUEUE_BUFFER_TOO_SMALL;
    }
    *readSize = size;
    if (nextIdx >= queue->bufferSize)
    {
        nextIdx = 0;
    }
    queue->dataLength -= size + 1; // remove size+1 from stored data len.
    // TODO: Assert if datalength < 0
    while (size--)
    {
        *buffer++ = queue->data[nextIdx++];
        if (nextIdx >= queue->bufferSize)
        {
            nextIdx = 0;
        }
    }
    queue->readPos = nextIdx;
    queue->firstPacketIndex++;
    CORE_EXIT_ATOMIC();
    return QUEUE_OK;
}

void pQinitQueue(packetQueue_t * queue, uint8_t *buffer, int size)
{
    memset(queue, 0, sizeof (*queue));
    queue->data = buffer;
    queue->bufferSize = size;
}
