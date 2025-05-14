#ifndef _SDCARD_H_
#define _SDCARD_H_

#define SDCARD_SPI_BUS          spi0
#define SDCARD_SPI_SPI_BRG      8000//Khz
#define SDCARD_PIN_SPI0_SCK     2
#define SDCARD_PIN_SPI0_MOSI    3
#define SDCARD_PIN_SPI0_MISO    4
#define SDCARD_PIN_SPI0_CS      5

/* SPI pin assignment */
#ifndef SDCARD_SPI_BUS
    #define SDCARD_SPI_BUS spi0
#endif

#ifndef SDCARD_PIN_SPI0_CS
    #define SDCARD_PIN_SPI0_CS     22
#endif

#ifndef SDCARD_PIN_SPI0_SCK
    #define SDCARD_PIN_SPI0_SCK    18
#endif

#ifndef SDCARD_PIN_SPI0_MOSI
    #define SDCARD_PIN_SPI0_MOSI   19
#endif

#ifndef SDCARD_PIN_SPI0_MISO 
    #define SDCARD_PIN_SPI0_MISO   16
#endif

#ifndef SDCARD_SPI_SPI_BRG
    #define SDCARD_SPI_SPI_BRG     30000//Khz
#endif

#endif // _SDCARD_H_
