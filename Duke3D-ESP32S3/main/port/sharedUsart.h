#ifndef SRC_SHAREDUSART_H_
#define SRC_SHAREDUSART_H_
// The SPI/USART sharing of the MG24 port has no equivalent here: ESP-IDF's SPI
// master driver arbitrates the shared display/SD bus.
#define SET_DISPLAY_MODE()
#define SET_DISPLAY_DIRECT_MODE()
#define SET_SD_MODE_FAST()
#define SET_SD_MODE_SLOW()
#define SET_SD_MODE(mode)
#define SET_VCOM_MODE()
#define SET_FLASH_MODE()
#endif /* SRC_SHAREDUSART_H_ */
