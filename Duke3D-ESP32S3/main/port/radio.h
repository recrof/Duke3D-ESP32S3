#ifndef RADIO_H_
#define RADIO_H_
// config
#define DEBUG_RADIO                       0
#define INITIAL_PAN_ID                  0x4E44 // Ascii for "DN", little endian

#if DEBUG_RADIO
    extern volatile uint64_t g_events;
    void printRadioEventType(uint64_t eventFlag);
    #define radioPrintf printf
#else
    #define radioPrintf(...)
    #define printRadioEventType(...)
#endif

// exported function.
uint32_t radioComputeCRC32(uint8_t *payload, uint32_t length);
void radioInit(void);
void radioSetConnectionIndex(int index);
void radioSetNumberOfPeers(int number);
void radioSetClientId(uint16_t id, uint16_t panId);
int radioSendAsynchPacket(uint8_t *buff, int len);
int radioEnqueueSubPacketForTransmission(uint8_t *buff, int len);
int radioGetSubPacket(uint8_t *clientId, uint8_t *buff, int maxlen);
int radioGetAsynchPacket(uint8_t *buff, int maxlen);
void radioSetChannel(uint8_t newChannel);
void disableComm(void);
void enableComm(void);
//
int findConnectionIndexByDestShortAddress( uint16_t shortAddr);
void setAllowedAddress(int idx, uint16_t shortaddr);
uint16_t getAllowedAddress(int idx);
void resetAllowedAddresses(void);
int isRadioTxFifoFull(void);
void setRadioTxBlockCallback(void *f);
#endif /* RADIO_H_ */
