#ifndef DISPLAYH
#define DISPLAYH
#include "boards.h"
#include <stdint.h>
#define MAXROWS (SCREEN_HEIGHT / 8)
#define MAXCOLS (SCREEN_WIDTH / 8)
void displayInit(void);
void displaySetBacklight(uint8_t level);
#endif
