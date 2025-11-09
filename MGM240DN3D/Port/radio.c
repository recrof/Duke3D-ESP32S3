/***************************************************************************//**
 * @file  radio.c
 * @brief Transport layer with 15.4 RAIL framework
 *
 *  This is a simple custom 15.4 protocol for transmitting data
 *  to the various clients. It supports two modes:
 *  - Asynchronous, where each peer broadcast using CSMA-CA, useful
 *  for the initial part (e.g. letting other clients know about ourselves)
 *  - Synchronous: the client having the lowest address will broadcast every
 *  25 ms. After that, upon receiving client 1 packets, client 2 will
 *  broadcast as well. Clients 3 and 4 follows the same rule.
 *
 *  In a single packet, more subpackets (coming from the game) can be
 *  appended. If a subpacket does not fit in the remaining space, appending
 *  is suspended for next cycle.
 *  All subpackets are transmitted and received in order.
 *
 *  Each packet contains in the header the packets indexes that tell:
 *  - What is the index of the first subpacket being transmitted
 *  - What are the last indexes of packet received from other peers.
 *
 *  In this way, every peer knows:
 *  - how many of its subpacket have been already received from other peers,
 *  so older subpackets in the tx queue can be discarded.
 *  - how many packets from other peers have been already processed and
 *  therefore need to be discarded on receive.
 *
 *  Each peer will retransmit its subpackets starting at the lowest index
 *  received by other peers. In this way no packet loss occur.
 *
 *  The protocol is robust to indefinite temporary loss of connection.
 *  However, if a peer disconnects, then the game won't work anymore and the
 *  match must be restarted.
 *
 *  Packets are signed using CRC32. A fast and crude way to prevent that
 *  accidentally other 15.4 devices will interfere and crash the game.
 *
 *  This protocol is *not* meant to be used for security or any commercial
 *  application. This is just a very simple implementation.
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

#include "maincfg.h"
#include "packetQueue.h"
#include "macros.h"
#include "main.h"
#include "radio.h"
#include "em_timer.h"
#include "em_cmu.h"
#include <stdbool.h>
#include "rail.h"
#include "rail_ieee802154.h"
#include "sli_rail_util_callbacks.h"  // required for callbacks
#include "printf.h"
#include "sl_rail_util_init.h"
#include "delay.h"
#include "crc.h"
// Assert utility
#if defined(SL_COMPONENT_CATALOG_PRESENT)
#include "sl_component_catalog.h"
#endif
#ifdef SL_CATALOG_APP_ASSERT_PRESENT
#include "app_assert.h"
#define APP_ASSERT(expr, ...) app_assert(expr,__VA_ARGS__)
#else
#define APP_ASSERT(expr, ...) \
  do {                        \
    if (!(expr)) {            \
      while (1) ;             \
    }                         \
  } while (0)
#endif

#define INCLUDE_CRC_ON_SUBPACKETS       1  // not necessary at all, but this will reduce to 0 the remote case of a corrupted packet slipping through.
#define MAX_TX_FIFO_SIZE                256 // shall be 128
#define BROADCAST_SHORT_ADDRESS         0xFFFF
#define RADIO_TX_TIMER_NUMBER           1   //
#define RADIO_TX_TIMER                  CAT(TIMER, RADIO_TX_TIMER_NUMBER)
//
#define ADD_DN3D_CRC                    1           // append a 32-bit CRC calcualted by prepending "DN3D" ascii data.
#define PAN_ID                          0x4E44
//
#define RADIO_TX_TIME_MS                25 // transmit every 25 ms
#define RADIO_TX_TIME_PRESCALER         1024
#define RADIO_TX_TIME_FREQUENCY         (CPU_FREQUENCY / RADIO_TX_TIME_PRESCALER)
#define RADIO_TX_TIMER_IRQHandler       CAT3(TIMER, RADIO_TX_TIMER_NUMBER, _IRQHandler)
#define RADIO_TX_TIMER_IRQn             CAT3(TIMER, RADIO_TX_TIMER_NUMBER, _IRQn)
//
#define COMM_MODE_SYNC                  1
#define COMM_MODE_ASYNC                 0
#define MAX_PEERS                       MAXPLAYERS
#define MAXPACKETSIZE    (127 - 1 /*len*/ - 9 /*mhr*/  - 2 /*crc*/ - 16 /*indexes*/ - 1 /*num subpackets*/ - 4 /*CRC32 for packet verification*/)

static struct
{
  uint16_t host;
} allowed_addresses[MAX_PEERS+1];

typedef struct
{

        union
        {
                uint8_t frameControl[2];
                struct
                {
                       uint8_t frameType : 3;
                       uint8_t securityEnabled : 1;
                       uint8_t framePending : 1;
                       uint8_t ackReq : 1;
                       uint8_t PANIdCompression : 1;
                       uint8_t reserved1 : 1;
                       uint8_t reserved2 : 1;
                       uint8_t reserved3 : 1;
                       uint8_t dstAddrMode : 2;
                       uint8_t frameVersion : 2;
                       uint8_t srcAddrMode : 2;
                };
        };
        uint8_t counter;
        uint8_t dstPAN[2];
        uint8_t dstAddr[2];
        uint8_t srcAddr[2];
} mhr_intraPAN802154_t;
typedef struct
{
        uint8_t length;
        mhr_intraPAN802154_t mhr;
        uint8_t payload[128 - sizeof (mhr_intraPAN802154_t)  - 1 - 2 ];  // CRC16 is appended by hw. CRC32 is appended by us
} intraPANpacket802154_t;
//
typedef struct
{
   uint32_t subPacketIndex[MAX_PEERS];
} p2pIndexes_t;
// todo: packed
typedef struct
{
  union
  {
          uint16_t shortAddress;
          p2pIndexes_t indexes;
  };
  uint8_t numberOfSubPackets;
  uint8_t payload[];
} p2pBroadcastPacket_t;
//
#if FAST_RESTART
static void resetTransmitTimer(void);
#endif
static void initRadioTxTimer(void);
static int readPacket(RAIL_Handle_t railHandle, intraPANpacket802154_t *packet, uint16_t *shortAddress);
static int prepareIntraPanPacket(intraPANpacket802154_t *packet, uint8_t *payload, uint8_t payloadSize, uint16_t dstPan, uint16_t srcAddr, uint16_t dstAddr, uint8_t counter, int copyPayload);
static void splitAndEnqueueReceivedSubPackets(p2pBroadcastPacket_t *packet, int peerIdx);  // adds each subpackets of the packet to the rxpacketqueue
static void prepareAndSendOurPacket(intraPANpacket802154_t *packet);
static void radioInitGPCRC(void);
static uint32_t radioCheckCRC32(uint8_t *payload, uint32_t length);
static void checkAndUpdateSubPacketIndexes(p2pBroadcastPacket_t *packet, int peerIdx);


#if DEBUG_RADIO
    volatile uint64_t g_events = 0;
#endif

// MAC and PHY data
static RAIL_Handle_t railHandle = NULL; // Initialized somewhere else.
static uint8_t channel = 11;
static uint8_t txFifo[MAX_TX_FIFO_SIZE] __attribute__ ((aligned (4)));
static uint16_t m_panId = PAN_ID;
static uint16_t m_myId = 0;
// Transport-related data
volatile uint32_t numOurSubPacketReceivedByPeers[MAX_PEERS];
volatile uint32_t numSubPacketsReceivedFromPeers[MAX_PEERS];
//
static int8_t myRadioConnectionIndex = 0;   // 0 == not initialized. Only asynch mode can be used.
static uint8_t numberOfPeers = 0;
//
static volatile uint8_t radioTxFifoFullState = 0;
//
packetQueue_t rxSubPacketQueue;
packetQueue_t rxAsyncPacketQueue;       // this only holds async packets. Asynch packets are queued directly to the radio
packetQueue_t txSubPacketQueue;
static uint8_t rxQueueBuffer[256+128];
static uint8_t rxAsyncQueueBuffer[128];
static uint8_t txQueueBuffer[256+128];
//
// TIMER for start sending packet.
void RADIO_TX_TIMER_IRQHandler(void)
{
    intraPANpacket802154_t packet;
    RADIO_TX_TIMER->IF_CLR = TIMER_IF_OF;
    // send packet.
    if (myRadioConnectionIndex == 1)
    {
        prepareAndSendOurPacket(&packet);
    }
}

static void initRadioTxTimer(void)
{
    CMU_ClockEnable(CAT(cmuClock_TIMER, RADIO_TX_TIMER_NUMBER), true);
    RADIO_TX_TIMER->CFG = CAT(TIMER_CFG_PRESC_DIV, RADIO_TX_TIME_PRESCALER) ;
    RADIO_TX_TIMER->IEN = TIMER_IEN_OF; // interrupt at overflow
    RADIO_TX_TIMER->EN = TIMER_EN_EN;
    float top =  1.0f * RADIO_TX_TIME_MS * RADIO_TX_TIME_FREQUENCY / 1000;    // float to prevent overflow
    RADIO_TX_TIMER->TOP = top;
    RADIO_TX_TIMER->CMD = TIMER_CMD_START;
    NVIC_SetPriority(RADIO_TX_TIMER_IRQn, 7);
    NVIC_EnableIRQ(RADIO_TX_TIMER_IRQn);
}
#if FAST_RESTART
static void resetTransmitTimer(void)
{
    RADIO_TX_TIMER->CNT = 0;
    RADIO_TX_TIMER->IF_CLR = TIMER_IF_OF;
    NVIC_ClearPendingIRQ(RADIO_TX_TIMER_IRQn);
}
#endif
// CRC: for testing if packets are coming from this application
static void radioInitGPCRC(void)
{
    crc32Init();
}
uint32_t radioComputeCRC32(uint8_t *payload, uint32_t length)
{
    return crc32Compute(payload, length) ;
}
static uint32_t radioCheckCRC32(uint8_t *payload, uint32_t length)
{
    uint32_t crc32 = radioComputeCRC32(payload, length);

#if SHOW_CRC_ERRORS
    if (crc32)
    {
        uint32_t crc32c = radioComputeCRC32(payload, length - 4);
        radioPrintf("len: %d CRC %x. Recv: %x, calc %x \r\n", length, crc32, *((uint32_t*)&payload[length]), crc32c);
    }
#endif
    return 0 == crc32;
}
// static utility functions
// prepare the packet, return length
static int prepareIntraPanPacket(intraPANpacket802154_t *packet, uint8_t *payload, uint8_t payloadSize, uint16_t dstPan, uint16_t srcAddr, uint16_t dstAddr, uint8_t counter, int copyPayload)
{
    // check if too big
    if (payloadSize > sizeof (packet->payload) - (ADD_DN3D_CRC ? 4 : 0))
        return 0;
    // copy packet.
    if (copyPayload)
    {
        memcpy(&packet->payload, payload, payloadSize);
    }
#if ADD_DN3D_CRC
    // we add the CRC32 as signature, to make sure the packet was sent by a duke nukem 3d application, and not by a random 15.4 radio.
    uint32_t crc32 = radioComputeCRC32(packet->payload, payloadSize);
    memcpy((uint8_t*) packet->payload + payloadSize, &crc32, sizeof (crc32));
    payloadSize += sizeof(crc32);
  //  radioPrintf("CRC32: 0x%08x l %d \r\n", crc32, payloadSize);
#endif
    // set address mode
    packet->mhr.frameControl[0] = 0;
    packet->mhr.frameControl[1] = 0;

    packet->mhr.frameType = 1; // data
    packet->mhr.securityEnabled = 0;    // no security
    packet->mhr.framePending = 0;
    packet->mhr.ackReq = 0;             // no ack requested. UDP like
    packet->mhr.PANIdCompression = 1;  //intra APN
    packet->mhr.dstAddrMode = 2;      // short addr
    packet->mhr.srcAddrMode = 2;      // short addr
    packet->mhr.frameVersion = 1;
    packet->mhr.counter = counter;
    packet->mhr.srcAddr[0] = srcAddr;
    packet->mhr.srcAddr[1] = srcAddr >> 8;
    packet->mhr.dstAddr[0] = dstAddr;
    packet->mhr.dstAddr[1] = dstAddr >> 8;
    packet->mhr.dstPAN[0] = dstPan;
    packet->mhr.dstPAN[1] = dstPan >> 8;
    packet->length = payloadSize + sizeof (mhr_intraPAN802154_t) + 2;
    return payloadSize + sizeof (mhr_intraPAN802154_t)  + 1;
}
static int readPacket(RAIL_Handle_t railHandle, intraPANpacket802154_t *packet, uint16_t *shortAddress)
{
    CORE_irqState_t irqState;
    CORE_ENTER_ATOMIC();
    RAIL_RxPacketInfo_t pi;
    // get packet info and handle
    RAIL_RxPacketHandle_t ph = RAIL_GetRxPacketInfo(railHandle, RAIL_RX_PACKET_HANDLE_OLDEST_COMPLETE, &pi);
    if (ph == RAIL_RX_PACKET_HANDLE_INVALID)
    {
        // should never happen
        CORE_EXIT_ATOMIC();
        return 0;
    }
    // read from fifo
    int nTotal = pi.packetBytes;
    int nFirst = pi.firstPortionBytes;
    // TODO: optimize for 0 copy!
    // FIXME: length check!
    uint8_t * d = (uint8_t *) packet;
    for (int i = 0; i < nFirst; i++)
    {
        uint8_t data = pi.firstPortionData[i];
        *d++ = data;
    }
    for (int i = nFirst; i < nTotal; i++)
    {
        uint8_t data = pi.lastPortionData[i - nFirst];
        *d++ = data;
    }
    // set id
    *shortAddress = packet->mhr.srcAddr[0] | (packet->mhr.srcAddr[1] << 8);
    //
    uint32_t len =  packet->length - 11;   // todo: AWAY with magic numbers! This is due to length (1 byte), MHR (8) and CRC16 (2)
    // check CRC32. If bad, discard packet as it was not generated by our application
    // NOTE! Radio packets have already a CRC. We are just using CRC32 for message authentication.
    // In fact, there might be tons of 802.15.4 radios transmitting data, and it might happen that one of them is broadcasting in our PAN-ID
    // to prevent issues, we simply use CRC32 as a very crude signature check.
    if ((len >= sizeof(uint32_t) + sizeof(p2pIndexes_t) + 1) && len < sizeof(packet->payload) && radioCheckCRC32(packet->payload, len))
    {
        len -= sizeof(uint32_t);
        CORE_EXIT_ATOMIC();
        return  len;
    }
    else
    {
        radioPrintf("Error on readpacket\r\n");
    }
    CORE_EXIT_ATOMIC();
    return 0;
}

static void checkAndUpdateSubPacketIndexes(p2pBroadcastPacket_t *packet, int peerIdx)
{
    uint32_t *indexes;
    indexes = packet->indexes.subPacketIndex;
    //numOurPacketReceivedByPeers  must be volatile
    numOurSubPacketReceivedByPeers[peerIdx - 1] = indexes[myRadioConnectionIndex - 1];
    // free up space from tx queue
    uint32_t minReceived = 0xFFFFFFFF;
    for (int i = 0; i < numberOfPeers; i++)
    {
        if (i != (myRadioConnectionIndex - 1) && numOurSubPacketReceivedByPeers[i] < minReceived)
            minReceived = numOurSubPacketReceivedByPeers[i];
    }
    // now we know that all the peers have received at least minReceived of our packets.
    // Let's free the tx queue
    // fixme: txPacketQueue.firstPacketIndex could be instead a module variable. No need to put it in the queue structure.
    int numToFree = minReceived - txSubPacketQueue.firstPacketIndex;
    for (int i = 0; i < numToFree; i++)
    {
        pQdiscardOldestPacket(&txSubPacketQueue);
    }
    if (numToFree)
    {
        radioTxFifoFullState = 0;
    }
    // note: the number of subpackets we have received from this peer is updated in splitAndEnqueueSubpackets().

}
static void prepareAndSendOurPacket(intraPANpacket802154_t *packet)
{
    if (myRadioConnectionIndex == 0)
    {
        while(1)
            printf("ERROR\r\n");
    }
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    p2pBroadcastPacket_t *pBpacket = (p2pBroadcastPacket_t *) packet->payload;
    // get the number of subpacket indexes
    for (int i = 0; i < MAX_PEERS; i++)
    {
        pBpacket->indexes.subPacketIndex[i] = numSubPacketsReceivedFromPeers[i]; // this is updated in splitAndEnqueueSubPackets()
    }
    // fill the index of the first subpacket we are sending.
    pBpacket->indexes.subPacketIndex[myRadioConnectionIndex - 1] = txSubPacketQueue.firstPacketIndex;
    //
    // now we have to peek the txSubPacketQueue and check how many subpackets we can include.
    pQstartPeek(&txSubPacketQueue);
    int sizeAvailable = MAXPACKETSIZE;
    int nSubPacketsIncluded = 0;
    uint32_t readSize = 0;
    uint8_t *ptr = pBpacket->payload;
    // note: packets get the length from buffer[0] TODO: fix this crap, we should have len prepending the packet.
    // check how many subpackets can fit the available payload.
    while( QUEUE_OK == pQpeekPacket(&txSubPacketQueue, sizeAvailable - 2, ptr + 2, &readSize))
    {
        ptr[0] = readSize;
        ptr[1] = myRadioConnectionIndex;
        radioPrintf("Including one more packet\r\n");
        // increment number of subpackets
        nSubPacketsIncluded ++;
        // reduce remaining space
        sizeAvailable -= readSize + 2; // +2 because of readSize and myRadioConnectionIndex.
        // increment pointer
        ptr += readSize + 2;
    }
    pBpacket->numberOfSubPackets = nSubPacketsIncluded;

    int payloadSize = (uint8_t*) ptr - (uint8_t*) pBpacket;

    int packetLen = prepareIntraPanPacket(packet, NULL, payloadSize, m_panId, m_myId, BROADCAST_SHORT_ADDRESS, 0, false);

    int s = RAIL_WriteTxFifo(railHandle, (uint8_t*) packet, packetLen, false);
    radioPrintf("WrTxF :%d\r\n",s);
    (void) s; // suppress warnings

    s = RAIL_StartTx(railHandle, channel, RAIL_TX_OPTIONS_DEFAULT, NULL);
    CORE_EXIT_ATOMIC();

    radioPrintf("StTx :%d\r\n",s);
    (void) s; // suppress warnings
}
static void splitAndEnqueueReceivedSubPackets(p2pBroadcastPacket_t *packet, int peerIdx)  // adds each subpackets of the packet to the rxpacketqueue
{
    if (peerIdx < 1)
        return;
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    // Each host will (re) send the packet, starting from the index corresponding to the lowest number of subpackets received by all.
    // For instance if peer #2 and #3 have received from peer #1 4 and 3 subpackets, respectively, peer #1 will start sending from
    // subPacketIndex 3. (because subPacket indexes 0, 1, 2 have been confirmed to be read by all).
    // This means that there are overlapping subPackets we must discard prior to
    int numberOfSubPacketWeQueued = numSubPacketsReceivedFromPeers[peerIdx - 1];
    int numSubPackets = packet->numberOfSubPackets;
    // let's check what is the first subPacket index we are receiving
    int subPacketIndex = packet->indexes.subPacketIndex[peerIdx - 1];
    //
    uint8_t *ptr = packet->payload;
    // cycle through all the included subpackets
    for (int i = 0; i < numSubPackets; i++)
    {
        // get size of subpacket
        int size = ptr[0];

        // is this a new subpacket?
        if (subPacketIndex >= numberOfSubPacketWeQueued)
        {
 #if INCLUDE_CRC_ON_SUBPACKETS
            // yes, then try to enqueue
            if (!radioCheckCRC32(ptr + 2, size))
            {
                radioPrintf("Crc of subpacket bad, rejected\r\n");
                break;
            }
#endif
            if (QUEUE_OK == pQenqueuePacket(&rxSubPacketQueue, size + 2, ptr))
            {
                // we successful enqueued it, increment number
                numberOfSubPacketWeQueued++;
            }
            else
            {
                // discard all other: we don't want to enqueue a later packet.
                break;
            }
        }
        // packets have already len and id. Len only is referred to payload.
        ptr += size + 2;
        subPacketIndex++;
    }
    numSubPacketsReceivedFromPeers[peerIdx - 1] = numberOfSubPacketWeQueued;
    CORE_EXIT_ATOMIC();
}


// PUBLIC FUNCTIONS
// This sets the connection index. Note, starts from 1.
void radioSetConnectionIndex(int index)
{
    myRadioConnectionIndex = index;
}

void radioSetNumberOfPeers(int number)
{
  numberOfPeers = number;
}

void radioSetClientId(uint16_t id, uint16_t panId)
{
    RAIL_Status_t sc;
    m_panId = panId;
    m_myId = id;
    sc =  RAIL_IEEE802154_SetPanId(railHandle, panId , 0);
    radioPrintf("PanID status = %x\r\n", sc);
    //
    sc = RAIL_IEEE802154_SetShortAddress(railHandle, id, 0);
    radioPrintf("ShortID status = %x\r\n", sc);
    (void) sc; // suppress warnings

}


int radioSendAsynchPacket(uint8_t *buff, int len)
{
// TODO: checklength
    intraPANpacket802154_t packet;
    p2pBroadcastPacket_t *pBpacket = (p2pBroadcastPacket_t *) &packet.payload;
    memset(&pBpacket->indexes, 0xFF, sizeof(pBpacket->indexes));
    pBpacket->numberOfSubPackets = 1;
    memcpy(pBpacket->payload, buff, len);
    uint8_t *ptr = &pBpacket->payload[len];
    int packetLen = prepareIntraPanPacket(&packet, NULL, ptr - (uint8_t*) pBpacket, m_panId, m_myId, BROADCAST_SHORT_ADDRESS, 0, false);
    int count = 0;
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    if (RAIL_GetTxFifoSpaceAvailable(railHandle) >= packetLen)
    {
        count = RAIL_WriteTxFifo(railHandle, (uint8_t*) &packet, packetLen, false);
    }

    CORE_EXIT_ATOMIC();
    RAIL_Status_t sc ;
    const RAIL_CsmaConfig_t csmaConfig = RAIL_CSMA_CONFIG_802_15_4_2003_2p4_GHz_OQPSK_CSMA;
    RAIL_TxOptions_t txOptions = RAIL_TX_OPTIONS_DEFAULT;
    sc = RAIL_StartCcaCsmaTx(railHandle, channel, txOptions, &csmaConfig, NULL);
    radioPrintf("TxS %d count %d\r\n", sc, count);
    UNUSED(count); // suppress warnings
    UNUSED(sc); // suppress warnings

    return 1;
}

void disableComm(void)
{
    NVIC_DisableIRQ(RADIO_TX_TIMER_IRQn);

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    RAIL_Idle(railHandle, RAIL_IDLE, true);
    CORE_EXIT_ATOMIC();
    delay(100);

}
void enableComm(void)
{

    RAIL_StartRx(railHandle, channel, NULL);
    NVIC_EnableIRQ(RADIO_TX_TIMER_IRQn);
}
void (*radioTxBlockCallback)(void);

void setRadioTxBlockCallback(void *f)
{
    radioTxBlockCallback = f;
}
// Enqueue a subpacket into the subpacket queue. The subpacket is sent
// when it is our turn, assuming enough space on the packet buffer.
int radioEnqueueSubPacketForTransmission(uint8_t *buff, int len)
{
    int status;
    do
    {
        CORE_DECLARE_IRQ_STATE;
        CORE_ENTER_ATOMIC();
    #if INCLUDE_CRC_ON_SUBPACKETS
        status = pQenqueuePacketAndAppendCRC(&txSubPacketQueue, len, buff);
    #else
        status = pQenqueuePacket(&txSubPacketQueue, len, buff);
    #endif
        if (status == QUEUE_OK)
        {
            radioTxFifoFullState = 0;
        }
        else
        {
            radioTxFifoFullState = 1;
        }
        CORE_EXIT_ATOMIC();
        if (status != QUEUE_OK)
        {
            if (radioTxBlockCallback)
            {
                radioTxBlockCallback();
            }
        }
    } while (status != QUEUE_OK);
    return status == QUEUE_OK;
}

int radioGetSubPacket(uint8_t *clientId, uint8_t *buff, int maxlen)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    int32_t readSize;
    // this assumes the packet had in but 0 the length, and in buf 1 the clientId. Note that readsize is the packet length without
    // clientId and size
    int status = pQdequeuePacketWithIdAndLen(&rxSubPacketQueue, maxlen, buff, (uint32_t *) &readSize, clientId);

    if (status != QUEUE_OK)
    {
        readSize = 0;
    }
    else
    {
#if INCLUDE_CRC_ON_SUBPACKETS
       readSize -= 4;       // remove CRC
       if (readSize < 0)
       {
           readSize = 0;
       }

#endif
    }
    CORE_EXIT_ATOMIC();
    return readSize;
}

int radioGetAsynchPacket(uint8_t *buff, int maxlen)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    uint32_t readSize;
    int status = pQdequeuePacket(&rxAsyncPacketQueue, maxlen, buff, &readSize);
    if (status != QUEUE_OK)
    {
        readSize = 0;
    }
    // fix this crap.
    CORE_EXIT_ATOMIC();
    return readSize;
}

// main event handler
void sl_rail_util_on_event(RAIL_Handle_t railHandle, RAIL_Events_t events)
{
  //  GPIO_PinModeSet(gpioPortA, 8, gpioModePushPull, 1);
#if DEBUG_RADIO
    g_events |= events;
#endif
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    // we only deal with received events
    if (events & RAIL_EVENT_RX_PACKET_ABORTED)
    {
        RAIL_RxPacketInfo_t packetInfo;
        RAIL_GetRxPacketInfo(railHandle, RAIL_RX_PACKET_HANDLE_NEWEST, &packetInfo);
        radioPrintf("Flags: %x\r\n", packetInfo.packetStatus);
    }
    if (events & RAIL_EVENT_RX_FIFO_OVERFLOW)
    {
        RAIL_ReleaseRxPacket(railHandle, RAIL_RX_PACKET_HANDLE_NEWEST);
        CORE_EXIT_ATOMIC();
 //       GPIO_PinModeSet(gpioPortA, 8, gpioModePushPull, 0);
        return;
    }
    if (events & RAIL_EVENT_RX_PACKET_RECEIVED)
    {
        // temporary packet for read/tx
        intraPANpacket802154_t packet;
        // short address of peer from which we received the packet
        uint16_t shortAddress;
        // length of received packet, excluding the CRC32 verification.
        int len;
        // the connection index of the peer sending packet. It is from 1 to MAX_PEERS (inclusive)
        int connectionIndex;
        len = readPacket(railHandle, &packet, &shortAddress);   // read packet, also validating the crc32 signature and filling id
#define TEST_ERRORS 0
#if TEST_ERRORS
    int i = id_rand() & 0xFF;
    if (i > 128)
    {
        GPIO_PinModeSet(gpioPortC, 1, gpioModePushPull, 0);
        len = 0;
    }
    else
    {
        GPIO_PinModeSet(gpioPortC, 1, gpioModePushPull, 1);
    }
#endif
        if (!len)                                       // if len is 0 then there was an error (CRC for instance)
        {
            CORE_EXIT_ATOMIC();
            return;
        }
        // infer communication mode from indexes.
        int communicationMode = COMM_MODE_ASYNC;
        p2pBroadcastPacket_t *pBpacket = (p2pBroadcastPacket_t*) packet.payload;
        if (pBpacket->numberOfSubPackets == 1)          // only 1 subpacket allowed
        {
            for (int i = 0; i < MAX_PEERS; i++)
            {
                if (pBpacket->indexes.subPacketIndex[i] != 0xFFFFFFFF)
                {
                    communicationMode = COMM_MODE_SYNC;
                    break;
                }
            }
        }
        else
        {
            communicationMode = COMM_MODE_SYNC;
        }
        if ( COMM_MODE_SYNC == communicationMode && myRadioConnectionIndex != 0)    // SYNC mode is only when we do have a non-zero myRadioConnectionIndex
        {
            connectionIndex = findConnectionIndexByDestShortAddress(shortAddress);
            if (connectionIndex <= 0)
            {
                CORE_EXIT_ATOMIC();
                return;    // drop as it is not from a recognized peer.
            }
            checkAndUpdateSubPacketIndexes(pBpacket, connectionIndex);  // this will also remove old packets from the tx queue
            // note: each subpacket has len and id in pos 0.
            splitAndEnqueueReceivedSubPackets(pBpacket, connectionIndex);
            //
            int sendOurPacket = 0;
            // if this was the packet coming from the last peer, and we are the first peer, then we can immediately transmit.
            // this will increase throughput.
            if (connectionIndex == numberOfPeers && myRadioConnectionIndex == 1)
            {
                // Note: typically transmit timer shall trigger well after all the other peers have sent their packets, even counting for other delays
#if FAST_RESTART
                resetTransmitTimer();
                sendOurPacket = 1;
#endif
            }
            else if (connectionIndex == myRadioConnectionIndex - 1)
            {
                // We are not peer 1, but this is still our turn. Transmit immediately.
                sendOurPacket = 1;
            }
            // we can reuse the packet, as we have already split and enqueued it
            if (sendOurPacket)
            {
                radioPrintf("Sending our packet\r\n");
                prepareAndSendOurPacket(&packet);
            }
        }
        else
        {
            // it was asynch packet, send to asynch queue. But store the source address
             pBpacket->shortAddress = packet.mhr.srcAddr[0] | (packet.mhr.srcAddr[0] << 8);
             pQenqueuePacket(&rxAsyncPacketQueue,   len - sizeof(pBpacket->indexes) - sizeof(pBpacket->numberOfSubPackets), pBpacket->payload);
        }
    }
    CORE_EXIT_ATOMIC();
//    GPIO_PinModeSet(gpioPortA, 8, gpioModePushPull, 0);

}
void radioInit(void)
{

    memset((uint8_t*) numOurSubPacketReceivedByPeers, 0, sizeof (numOurSubPacketReceivedByPeers));
    //
    pQinitQueue(&rxSubPacketQueue, rxQueueBuffer, sizeof(rxQueueBuffer));
    pQinitQueue(&rxAsyncPacketQueue, rxAsyncQueueBuffer, sizeof(rxAsyncQueueBuffer));
    pQinitQueue(&txSubPacketQueue, txQueueBuffer, sizeof(txQueueBuffer));
    // init GPCRC. Used for crude authentication
    radioInitGPCRC();
    //
    railHandle = sl_rail_util_get_handle(0);
    APP_ASSERT((NULL != railHandle),
               "RAIL_Init failed, return value: NULL");
    //
    RAIL_Status_t sc;

     railHandle = sl_rail_util_get_handle(0);
     APP_ASSERT((NULL != railHandle), "RAIL_Init failed, return value: NULL");
     //
     int count;
     count = RAIL_SetTxFifo(railHandle, txFifo, 0, sizeof(txFifo));
     radioPrintf("real fifo size = %d\r\n", count);

     sc = RAIL_Idle  (railHandle, RAIL_IDLE, true);
     radioPrintf("status = %d\r\n", sc);
     sc = RAIL_SetTxPowerDbm(railHandle, 100);
     radioPrintf("set power status = %d\r\n", sc);
     //
     sc =  RAIL_Calibrate(railHandle, NULL, RAIL_CAL_ALL_PENDING);
     radioPrintf("cal status = %d\r\n", sc);
     //
     initRadioTxTimer();
     // NOTE!!!! THIS IS MANDATORY!
     RAIL_StartRx(railHandle, channel, NULL);
     //
     UNUSED(sc);
     UNUSED(count);
    //
}

#if DEBUG_RADIO

void printRadioEventType(uint64_t eventFlag)
{
    for (int i = 0; i < 64; i++)
    {
        if (!((eventFlag >> i) & 1))
            continue;
        switch (eventFlag & (1ULL << i))
        {
            case (1ULL << RAIL_EVENT_RSSI_AVERAGE_DONE_SHIFT):
                radioPrintf("RAIL_EVENT_RSSI_AVERAGE_DONE\n");
                break;
            case (1ULL << RAIL_EVENT_RX_ACK_TIMEOUT_SHIFT):
                radioPrintf("RAIL_EVENT_RX_ACK_TIMEOUT\n");
                break;
            case (1ULL << RAIL_EVENT_RX_FIFO_ALMOST_FULL_SHIFT):
                radioPrintf("RAIL_EVENT_RX_FIFO_ALMOST_FULL\n");
                break;
            case (1ULL << RAIL_EVENT_RX_PACKET_RECEIVED_SHIFT):
                radioPrintf("RAIL_EVENT_RX_PACKET_RECEIVED\n");
                break;
            case (1ULL << RAIL_EVENT_RX_PREAMBLE_LOST_SHIFT):
                radioPrintf("RAIL_EVENT_RX_PREAMBLE_LOST\n");
                break;
            case (1ULL << RAIL_EVENT_RX_PREAMBLE_DETECT_SHIFT):
                radioPrintf("RAIL_EVENT_RX_PREAMBLE_DETECT\n");
                break;
            case (1ULL << RAIL_EVENT_RX_SYNC1_DETECT_SHIFT):
                radioPrintf("RAIL_EVENT_RX_SYNC1_DETECT\n");
                break;
            case (1ULL << RAIL_EVENT_RX_SYNC2_DETECT_SHIFT):
                radioPrintf("RAIL_EVENT_RX_SYNC2_DETECT\n");
                break;
            case (1ULL << RAIL_EVENT_RX_FRAME_ERROR_SHIFT):
                radioPrintf("RAIL_EVENT_RX_FRAME_ERROR\n");
                break;
            case (1ULL << RAIL_EVENT_RX_FIFO_FULL_SHIFT):
                radioPrintf("RAIL_EVENT_RX_FIFO_FULL\n");
                break;
            case (1ULL << RAIL_EVENT_RX_FIFO_OVERFLOW_SHIFT):
                radioPrintf("RAIL_EVENT_RX_FIFO_OVERFLOW\n");
                break;
            case (1ULL << RAIL_EVENT_RX_ADDRESS_FILTERED_SHIFT):
                radioPrintf("RAIL_EVENT_RX_ADDRESS_FILTERED\n");
                break;
            case (1ULL << RAIL_EVENT_RX_TIMEOUT_SHIFT):
                radioPrintf("RAIL_EVENT_RX_TIMEOUT\n");
                break;
            case (1ULL << RAIL_EVENT_SCHEDULED_RX_STARTED_SHIFT):
                radioPrintf("RAIL_EVENT_SCHEDULED_RX_STARTED\n");
                break;
            case (1ULL << RAIL_EVENT_RX_SCHEDULED_RX_END_SHIFT):
                radioPrintf("RAIL_EVENT_RX_SCHEDULED_RX_END\n");
                break;
            case (1ULL << RAIL_EVENT_RX_SCHEDULED_RX_MISSED_SHIFT):
                radioPrintf("RAIL_EVENT_RX_SCHEDULED_RX_MISSED\n");
                break;
            case (1ULL << RAIL_EVENT_RX_PACKET_ABORTED_SHIFT):
                radioPrintf("RAIL_EVENT_RX_PACKET_ABORTED\n");
                break;
            case (1ULL << RAIL_EVENT_RX_FILTER_PASSED_SHIFT):
                radioPrintf("RAIL_EVENT_RX_FILTER_PASSED\n");
                break;
            case (1ULL << RAIL_EVENT_RX_TIMING_LOST_SHIFT):
                radioPrintf("RAIL_EVENT_RX_TIMING_LOST\n");
                break;
            case (1ULL << RAIL_EVENT_RX_TIMING_DETECT_SHIFT):
                radioPrintf("RAIL_EVENT_RX_TIMING_DETECT\n");
                break;
            case (1ULL << RAIL_EVENT_RX_CHANNEL_HOPPING_COMPLETE_SHIFT):
                radioPrintf("RAIL_EVENT_RX_CHANNEL_HOPPING_COMPLETE\n");
                break;
            case (1ULL << RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND_SHIFT):
                radioPrintf("RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND\n");
                break;
            case (1ULL << RAIL_EVENT_ZWAVE_BEAM_SHIFT):
                radioPrintf("RAIL_EVENT_ZWAVE_BEAM\n");
                break;
            case (1ULL << RAIL_EVENT_TX_FIFO_ALMOST_EMPTY_SHIFT):
                radioPrintf("RAIL_EVENT_TX_FIFO_ALMOST_EMPTY\n");
                break;
            case (1ULL << RAIL_EVENT_TX_PACKET_SENT_SHIFT):
                radioPrintf("RAIL_EVENT_TX_PACKET_SENT\n");
                break;
            case (1ULL << RAIL_EVENT_TXACK_PACKET_SENT_SHIFT):
                radioPrintf("RAIL_EVENT_TXACK_PACKET_SENT\n");
                break;
            case (1ULL << RAIL_EVENT_TX_ABORTED_SHIFT):
                radioPrintf("RAIL_EVENT_TX_ABORTED\n");
                break;
            case (1ULL << RAIL_EVENT_TXACK_ABORTED_SHIFT):
                radioPrintf("RAIL_EVENT_TXACK_ABORTED\n");
                break;
            case (1ULL << RAIL_EVENT_TX_BLOCKED_SHIFT):
                radioPrintf("RAIL_EVENT_TX_BLOCKED\n");
                break;
            case (1ULL << RAIL_EVENT_TXACK_BLOCKED_SHIFT):
                radioPrintf("RAIL_EVENT_TXACK_BLOCKED\n");
                break;
            case (1ULL << RAIL_EVENT_TX_UNDERFLOW_SHIFT):
                radioPrintf("RAIL_EVENT_TX_UNDERFLOW\n");
                break;
            case (1ULL << RAIL_EVENT_TXACK_UNDERFLOW_SHIFT):
                radioPrintf("RAIL_EVENT_TXACK_UNDERFLOW\n");
                break;
            case (1ULL << RAIL_EVENT_TX_CHANNEL_CLEAR_SHIFT):
                radioPrintf("RAIL_EVENT_TX_CHANNEL_CLEAR\n");
                break;
            case (1ULL << RAIL_EVENT_TX_CHANNEL_BUSY_SHIFT):
                radioPrintf("RAIL_EVENT_TX_CHANNEL_BUSY\n");
                break;
            case (1ULL << RAIL_EVENT_TX_CCA_RETRY_SHIFT):
                radioPrintf("RAIL_EVENT_TX_CCA_RETRY\n");
                break;
            case (1ULL << RAIL_EVENT_TX_START_CCA_SHIFT):
                radioPrintf("RAIL_EVENT_TX_START_CCA\n");
                break;
            case (1ULL << RAIL_EVENT_TX_STARTED_SHIFT):
                radioPrintf("RAIL_EVENT_TX_STARTED\n");
                break;
            case (1ULL << RAIL_EVENT_TX_SCHEDULED_TX_MISSED_SHIFT):
                radioPrintf("RAIL_EVENT_TX_SCHEDULED_TX_MISSED\n");
                break;
            case (1ULL << RAIL_EVENT_CONFIG_UNSCHEDULED_SHIFT):
                radioPrintf("RAIL_EVENT_CONFIG_UNSCHEDULED\n");
                break;
            case (1ULL << RAIL_EVENT_CONFIG_SCHEDULED_SHIFT):
                radioPrintf("RAIL_EVENT_CONFIG_SCHEDULED\n");
                break;
            case (1ULL << RAIL_EVENT_SCHEDULER_STATUS_SHIFT):
                radioPrintf("RAIL_EVENT_SCHEDULER_STATUS\n");
                break;
            case (1ULL << RAIL_EVENT_CAL_NEEDED_SHIFT):
                radioPrintf("RAIL_EVENT_CAL_NEEDED\n");
                break;
            case (1ULL << RAIL_EVENT_RF_SENSED_SHIFT):
                radioPrintf("RAIL_EVENT_RF_SENSED\n");
                break;
            case (1ULL << RAIL_EVENT_PA_PROTECTION_SHIFT):
                radioPrintf("RAIL_EVENT_PA_PROTECTION\n");
                break;
            case (1ULL << RAIL_EVENT_SIGNAL_DETECTED_SHIFT):
                radioPrintf("RAIL_EVENT_SIGNAL_DETECTED\n");
                break;
            case (1ULL << RAIL_EVENT_IEEE802154_MODESWITCH_START_SHIFT):
                radioPrintf("RAIL_EVENT_IEEE802154_MODESWITCH_START\n");
                break;
            case (1ULL << RAIL_EVENT_IEEE802154_MODESWITCH_END_SHIFT):
                radioPrintf("RAIL_EVENT_IEEE802154_MODESWITCH_END\n");
                break;
            case (1ULL << RAIL_EVENT_DETECT_RSSI_THRESHOLD_SHIFT):
                radioPrintf("RAIL_EVENT_DETECT_RSSI_THRESHOLD\n");
                break;
            case (1ULL << RAIL_EVENT_THERMISTOR_DONE_SHIFT):
                radioPrintf("RAIL_EVENT_THERMISTOR_DONE\n");
                break;
            case (1ULL << RAIL_EVENT_TX_BLOCKED_TOO_HOT_SHIFT):
                radioPrintf("RAIL_EVENT_TX_BLOCKED_TOO_HOT\n");
                break;
            case (1ULL << RAIL_EVENT_TEMPERATURE_TOO_HOT_SHIFT):
                radioPrintf("RAIL_EVENT_TEMPERATURE_TOO_HOT\n");
                break;
            case (1ULL << RAIL_EVENT_TEMPERATURE_COOL_DOWN_SHIFT):
                radioPrintf("RAIL_EVENT_TEMPERATURE_COOL_DOWN\n");
                break;
            case (1ULL << RAIL_EVENT_USER_MBOX_SHIFT):
                radioPrintf("RAIL_EVENT_USER_MBOX\n");
                break;
            default:
                radioPrintf("Unknown event: %d\n");
                break;
        }
        radioPrintf("\r\n");
    }
}
#endif
// find which index corresponds to the short address. Yes we do a linear search, even though these are sorted.
// But, who cares, we have only 3 peers
int findConnectionIndexByDestShortAddress( uint16_t shortAddr)
{
    for (int i = 1; i <= numberOfPeers; i++)
    {
        if (i != myRadioConnectionIndex && shortAddr == allowed_addresses[i].host)
        {
            return i;
        }
    }
    return -1;
}
void setAllowedAddress(int idx, uint16_t shortaddr)
{
    allowed_addresses[idx].host = shortaddr;
}
uint16_t getAllowedAddress(int idx)
{
    if (idx <= numberOfPeers)
        return allowed_addresses[idx].host;
    //
    return 0;  // 0 and 0xFFFF are not valid.
}
int isRadioTxFifoFull(void)
{
    return radioTxFifoFullState;
}

void resetAllowedAddresses(void)
{
    memset(allowed_addresses, 0, sizeof(allowed_addresses));
}
void radioSetChannel(uint8_t newChannel)
{
    channel = newChannel;
}
void sl_rail_util_on_assert_failed(RAIL_Handle_t rail_handle, RAIL_AssertErrorCodes_t error_code)
{
    (void) rail_handle;
    (void) error_code;
    // do exactly nothing.
}
