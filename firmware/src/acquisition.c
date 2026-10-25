#include "acquisition.h"
#include "hw.h"
#include "config.h"
static Scope capture;
static volatile bool active, complete, failed;
static volatile uint32_t fault_count;
static uint32_t half_ticks;
#define ADC_CONFIG (1u | (1u<<8) | (1u<<21)) /* CH0; 12MHz (12.5MHz external). */
static void stop_hardware(void) {
    TIM(0x04)=0;
    ADC(0x00) &= ~(7u<<24);
    active=false;
}
static void fault(void) {
    stop_hardware(); ++fault_count; failed=true;
}
void acquisition_init(void) {
    SC(0x0C4) |= (1u<<1)|(1u<<12);
    PINSEL(1) = (PINSEL(1)&~(3u<<14))|(1u<<14); /* P0.23 = AD0.0. */
    PINMODE(1) = (PINMODE(1)&~(3u<<14))|(2u<<14); /* No pulls. */
    PINOD(0) &= ~(1u<<23);
    PINSEL(3) = (PINSEL(3)&~(3u<<26))|(3u<<26); /* P1.29 MAT0.1 probe. */
    TIM(0x04)=2; TIM(0x70)=0; TIM(0x0C)=0;
    TIM(0x14)=1u<<3; /* Interrupt on MR1, free-running TC: no reset. */
    TIM(0x3C)=3u<<6; /* Toggle EM1 on MR1. Rising edges occur at Fs. */
    TIM(0x00)=0x3F; ADC(0x00)=ADC_CONFIG;
    ADC(0x0C)=1u<<8; /* Global DONE interrupt, read ADGDR consistently. */
    REG8(0xE000E401u)=0; REG8(0xE000E416u)=0x20;
    NVIC_CLR=(1u<<1)|(1u<<22);
    NVIC_EN=(1u<<1)|(1u<<22);
}
void acquisition_stop(void) {
    irq_off(); stop_hardware();
    ADC(0x00)=0; /* Power down: abandon a possible in-flight conversion. */
    (void)ADC(0x04); TIM(0x00)=0x3F;
    NVIC_CLR=(1u<<1)|(1u<<22); complete=false;
    irq_on();
}
void acquisition_start(ScopeConfig cfg) {
    acquisition_stop();
    scope_arm(&capture,cfg);
    half_ticks=PCLK_HZ/(2u*cfg.sample_hz);
    failed=false; ADC(0x00)=ADC_CONFIG;
    delay_ms(1); /* ADC settling, with SysTick running. */
    irq_off();
    (void)ADC(0x04); NVIC_CLR=(1u<<1)|(1u<<22);
    TIM(0x04)=2; TIM(0x08)=0; TIM(0x10)=0;
    TIM(0x1C)=half_ticks; TIM(0x3C)=3u<<6; TIM(0x00)=0x3F;
    ADC(0x00)=ADC_CONFIG|(4u<<24); /* START=MAT0.1 rising edge. */
    active=true; barrier(); TIM(0x04)=1; irq_on();
}
void TIMER0_IRQHandler(void) {
    if (TIM(0x00)&2u) {
        TIM(0x00)=2u;
        if (!active) return;
        /* Absolute match schedule avoids reset/off-by-one ambiguity and ISR jitter.
           The hardware edge already started this conversion. Schedule the next edge. */
        uint32_t next=TIM(0x1C)+half_ticks;
        TIM(0x1C)=next;
        if ((int32_t)(TIM(0x08)-next)>=0) fault();
    }
}
void ADC_IRQHandler(void) {
    uint32_t word=ADC(0x04);
    if (!active || !(word&(1u<<31))) return;
    if (word&(1u<<30)) { fault(); return; }
    if (scope_feed(&capture,(uint16_t)((word>>4)&4095u))) {
        stop_hardware(); barrier(); complete=true;
    }
}
bool acquisition_ready(void) { bool r=complete; barrier(); return r; }
bool acquisition_failed(void) { return failed; }
uint32_t acquisition_faults(void) { return fault_count; }
bool acquisition_copy(uint16_t data[SCOPE_N]) {
    scope_copy(&capture,data); return capture.forced;
}
