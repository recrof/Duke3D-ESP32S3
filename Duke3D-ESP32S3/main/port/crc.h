#ifndef PORT_CRC_H_
#define PORT_CRC_H_
#include <stdint.h>
void crc32Init(void);
uint32_t crc32Compute(uint8_t *payload, uint32_t length);
#endif /* PORT_CRC_H_ */
