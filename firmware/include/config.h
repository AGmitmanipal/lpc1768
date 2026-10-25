#ifndef CONFIG_H
#define CONFIG_H
/* Reference design. MCU port names are NOT development-board header numbers. */
#define REFERENCE_UV 3300000u /* Replace with measured VREFP - VREFN. */
#ifndef USE_EXTERNAL_12MHZ
#define USE_EXTERNAL_12MHZ 0 /* Default: board-independent 4 MHz IRC + PLL. */
#endif
#if USE_EXTERNAL_12MHZ
#define CORE_HZ 100000000u
#else
#define CORE_HZ 96000000u /* Margin below 100 MHz for the IRC tolerance. */
#endif
#define PCLK_HZ (CORE_HZ / 4u)
#ifndef ENABLE_LCD
#define ENABLE_LCD 0 /* Optional 3.3 V compatible HD44780, P1.18..P1.23. */
#endif
#ifndef ENABLE_TM1637
#define ENABLE_TM1637 0 /* Optional 3.3 V TM1637, P1.24 CLK / P1.25 DIO. */
#endif
#define TFT_CS 16u
#define TFT_DC 17u
#define TFT_RST 22u
#define KEY_COUNT 7u /* P2.0..6: RUN, RATE, MODE, EDGE, LEVEL+, LEVEL-, ZOOM. */
#define TFT_SPI_DIV 4u /* SPI 6MHz default, 6.25MHz with external clock. */
#endif
