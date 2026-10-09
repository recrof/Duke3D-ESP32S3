#ifndef SRC_DELAY_H_
#define SRC_DELAY_H_
#include <stdint.h>
// Busy-wait like the original, so it is safe in any context the engine calls
// it from. A real function: some game files call it without this header.
void delay(uint32_t milliseconds);
#endif /* SRC_DELAY_H_ */
