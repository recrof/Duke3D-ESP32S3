/*
 *  The ESP32-S3 has no 802.15.4 radio, so multiplayer is unavailable. These
 *  stubs make the network layer behave as if no peer ever answers.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include <stdint.h>
#include "radio.h"
#include "esp_rom_crc.h"
static uint16_t allowedAddresses[8];
uint32_t radioComputeCRC32(uint8_t *payload, uint32_t length)
{
    return esp_rom_crc32_le(0, payload, length);
}
void radioInit(void)
{
}
void radioSetConnectionIndex(int index)
{
    (void) index;
}
void radioSetNumberOfPeers(int number)
{
    (void) number;
}
void radioSetClientId(uint16_t id, uint16_t panId)
{
    (void) id;
    (void) panId;
}
int radioSendAsynchPacket(uint8_t *buff, int len)
{
    (void) buff;
    return len;
}
int radioEnqueueSubPacketForTransmission(uint8_t *buff, int len)
{
    (void) buff;
    return len;
}
int radioEnqueuePacket(int id, void *buff, int len)
{
    (void) id;
    (void) buff;
    return len;
}
int radioGetSubPacket(uint8_t *clientId, uint8_t *buff, int maxlen)
{
    (void) clientId;
    (void) buff;
    (void) maxlen;
    return 0;
}
int radioGetAsynchPacket(uint8_t *buff, int maxlen)
{
    (void) buff;
    (void) maxlen;
    return 0;
}
void radioSetChannel(uint8_t newChannel)
{
    (void) newChannel;
}
void disableComm(void)
{
}
void enableComm(void)
{
}
int findConnectionIndexByDestShortAddress(uint16_t shortAddr)
{
    for (int i = 0; i < 8; i++)
    {
        if (allowedAddresses[i] == shortAddr)
        {
            return i;
        }
    }
    return -1;
}
void setAllowedAddress(int idx, uint16_t shortaddr)
{
    if (idx >= 0 && idx < 8)
    {
        allowedAddresses[idx] = shortaddr;
    }
}
uint16_t getAllowedAddress(int idx)
{
    return (idx >= 0 && idx < 8) ? allowedAddresses[idx] : 0;
}
void resetAllowedAddresses(void)
{
    for (int i = 0; i < 8; i++)
    {
        allowedAddresses[i] = 0;
    }
}
int isRadioTxFifoFull(void)
{
    return 0;
}
void setRadioTxBlockCallback(void *f)
{
    (void) f;
}
