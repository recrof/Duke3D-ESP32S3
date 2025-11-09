#ifndef _MMULTI_UNSTABLE_H_
#define _MMULTI_UNSTABLE_H_

#include <stdint.h>

extern void initmultiplayers();
//void sendpacket(int32_t other, const uint8_t* bufptr, int32_t messleng);
//void setpackettimeout(int32_t datimeoutcount, int32_t daresendagaincount);
void uninitmultiplayers(void);
void sendlogon(void);
void sendlogoff(void);
int32_t  getoutputcirclesize(void);
short getpacket(short* other, uint8_t* bufptr);
void flushpackets(void);
#define DEBUG_BROADCAST_PACKET   0

#if DEBUG_BROADCAST_PACKET
#define broadcastPacket(p, l, n) broadcastPacket2(p, l, n, __FILE__, __LINE__, __FUNCTION__)
void broadcastPacket2(uint8_t * packet, int length, int notme, char *file, int line, char *function);
#else
void broadcastPacket(uint8_t * packet, int length, int notme);
#endif
void genericmultifunction(int32_t other, const uint8_t* bufptr, int32_t messleng, int32_t command);

#endif
