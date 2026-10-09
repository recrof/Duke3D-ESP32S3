#ifndef _PRINTF_H_
#define _PRINTF_H_
// The MG24 port used a tiny printf over a shared USART. ESP-IDF's newlib stdio
// already goes to the USB-Serial-JTAG console.
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include "main.h"
#include "sharedUsart.h"
#endif  // _PRINTF_H_
