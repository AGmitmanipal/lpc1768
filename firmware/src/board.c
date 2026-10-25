#include "hw.h"
#include "config.h"
static volatile uint32_t ticks;
static void feed(void) { SC(0x08C) = 0xAA; SC(0x08C) = 0x55; }
void clock_init(void) {
    irq_off();
    if (SC(0x088) & (1u<<25)) { SC(0x080) = 1; feed(); }
    SC(0x080) = 0; feed();
    SC(0x104) = 0;
    SC(0x000) = (SC(0x000) & ~(15u<<12)) | (5u<<12);
#if USE_EXTERNAL_12MHZ
    SC(0x1A0) = (SC(0x1A0) & ~(1u<<4)) | (1u<<5);
    while (!(SC(0x1A0) & (1u<<6))) {} /* Board MUST have a 12 MHz crystal. */
    SC(0x10C) = 1;
    SC(0x084) = 99u | (5u<<16); /* 2*12MHz*100/6 = 400MHz. */
#else
    SC(0x10C) = 0;
    SC(0x084) = 47u; /* 2*4MHz*48/1 = 384MHz. */
#endif
    feed(); SC(0x080) = 1; feed();
    SC(0x104) = 3; /* CCO/4: 96MHz IRC or 100MHz external crystal. */
    while (!(SC(0x088) & (1u<<26))) {}
    SC(0x080) = 3; feed();
    while ((SC(0x088) & (3u<<24)) != (3u<<24)) {}
    SC(0x1A8) = 0; SC(0x1AC) = 0; /* All peripheral clocks /4. */
    REG32(0xE000ED08u) = 0; /* Vector table at flash address 0. */
}
void SysTick_Handler(void) { ++ticks; }
uint32_t millis(void) { return ticks; }
void delay_ms(uint32_t ms) {
    uint32_t start = ticks;
    while ((uint32_t)(ticks - start) < ms) {}
}
void board_init(void) {
    SC(0x0C4) |= (1u<<15); /* GPIO clock. */
    PINSEL(4) &= ~0x3FFFu; PINMODE(4) &= ~0x3FFFu; /* Keys with pullups. */
    DIR(2) &= ~0x7Fu;
    REG32(0xE000E014u) = CORE_HZ/1000u - 1u;
    REG32(0xE000E018u) = 0;
    REG8(0xE000ED23u) = 0xE0; /* SysTick lower priority than acquisition. */
    REG32(0xE000E010u) = 7;
    irq_on();
}
uint8_t keys_poll(void) {
    static uint32_t last;
    static uint8_t history[KEY_COUNT], down;
    uint8_t events = 0;
    uint32_t now = ticks;
    if ((uint32_t)(now-last) < 5u) return 0;
    last = now;
    uint32_t pins = PIN(2);
    for (uint32_t i = 0; i < KEY_COUNT; ++i) {
        history[i] = (uint8_t)((history[i]<<1) | ((pins>>i)&1u));
        if (history[i] == 0 && !(down & (1u<<i))) {
            down |= (uint8_t)(1u<<i); events |= (uint8_t)(1u<<i);
        }
        if (history[i] == 0xFF) down &= (uint8_t)~(1u<<i);
    }
    return events;
}
static void spi(uint8_t b) {
    while (!(SSP(0x0C) & 2u)) {}
    SSP(0x08) = b;
    while (!(SSP(0x0C) & 4u)) {}
    (void)SSP(0x08); /* Drain RX even though TFT MISO is not connected. */
}
static void select_tft(bool on) {
    while (SSP(0x0C)&16u) {}
    if (on) CLR(0) = 1u<<TFT_CS; else SET(0) = 1u<<TFT_CS;
}
void tft_reset(bool high) {
    if (high) SET(0) = 1u<<TFT_RST; else CLR(0) = 1u<<TFT_RST;
}
void tft_bus_init(void) {
    SC(0x0C4) |= 1u<<21;
    PINSEL(0) = (PINSEL(0) & ~(3u<<30)) | (2u<<30); /* P0.15 SCK0. */
    PINSEL(1) = (PINSEL(1) & ~((3u<<4)|3u|(3u<<2)|(3u<<12))) | (2u<<4);
    DIR(0) |= (1u<<TFT_CS)|(1u<<TFT_DC)|(1u<<TFT_RST);
    SET(0) = (1u<<TFT_CS)|(1u<<TFT_DC)|(1u<<TFT_RST);
    SSP(0x04) = 0; SSP(0x00) = 7; /* 8 bits, mode 0. */
    SSP(0x10) = TFT_SPI_DIV; SSP(0x04) = 2;
}
void tft_bus_command(uint8_t cmd, const uint8_t *data, uint32_t len) {
    select_tft(true); CLR(0) = 1u<<TFT_DC; spi(cmd);
    SET(0) = 1u<<TFT_DC;
    for (uint32_t i=0; i<len; ++i) spi(data[i]);
    select_tft(false);
}
void tft_bus_pixels(const uint16_t *p, uint32_t count) {
    select_tft(true); SET(0) = 1u<<TFT_DC;
    while (count--) { uint16_t c=*p++; spi((uint8_t)(c>>8)); spi((uint8_t)c); }
    select_tft(false);
}
void tft_bus_repeat(uint16_t c, uint32_t count) {
    select_tft(true); SET(0) = 1u<<TFT_DC;
    while (count--) { spi((uint8_t)(c>>8)); spi((uint8_t)c); }
    select_tft(false);
}
