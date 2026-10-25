#ifndef HW_H
#define HW_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* Minimal register definitions, independently written from NXP UM10360. */
#define REG32(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define REG8(a) (*(volatile uint8_t *)(uintptr_t)(a))
#define SC(o) REG32(0x400FC000u + (o))
#define PINSEL(i) REG32(0x4002C000u + 4u*(i))
#define PINMODE(i) REG32(0x4002C040u + 4u*(i))
#define PINOD(p) REG32(0x4002C068u + 4u*(p))
#define GPIO(p,o) REG32(0x2009C000u + 0x20u*(p) + (o))
#define DIR(p) GPIO(p,0x00u)
#define PIN(p) GPIO(p,0x14u)
#define SET(p) GPIO(p,0x18u)
#define CLR(p) GPIO(p,0x1Cu)
#define TIM(o) REG32(0x40004000u + (o))
#define ADC(o) REG32(0x40034000u + (o))
#define SSP(o) REG32(0x40088000u + (o))
#define NVIC_EN REG32(0xE000E100u)
#define NVIC_CLR REG32(0xE000E280u)
#if defined(__arm__) || defined(__thumb__)
static inline void irq_off(void) { __asm volatile("cpsid i" ::: "memory"); }
static inline void irq_on(void) { __asm volatile("cpsie i" ::: "memory"); }
static inline void barrier(void) { __asm volatile("dmb" ::: "memory"); }
#else
/* Host compiler syntax-check only. Hardware functions must not be executed. */
static inline void irq_off(void) {}
static inline void irq_on(void) {}
static inline void barrier(void) {}
#endif
void clock_init(void);
void board_init(void);
void delay_ms(uint32_t ms);
uint32_t millis(void);
uint8_t keys_poll(void);
void tft_bus_init(void);
void tft_bus_command(uint8_t cmd, const uint8_t *data, uint32_t len);
void tft_bus_pixels(const uint16_t *pixels, uint32_t count);
void tft_bus_repeat(uint16_t color, uint32_t count);
void tft_reset(bool high);
void aux_init(void);
void aux_update(uint32_t sample_hz, const char *status);
#endif
