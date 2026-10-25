#include <stdint.h>
extern uint32_t _estack,_sidata,_sdata,_edata,_sbss,_ebss;
extern int main(void);
void Reset_Handler(void);
void Default_Handler(void) { for(;;) {} }
void SysTick_Handler(void);
void TIMER0_IRQHandler(void);
void ADC_IRQHandler(void);
typedef void (*Handler)(void);
/* LPC1768 boot ROM requires the vector checksum at offset 0x1C.
   tools/checksum.py patches and verifies that word in the final .bin/.hex. */
__attribute__((section(".isr_vector"),used))
const Handler vectors[51]={
    (Handler)&_estack,Reset_Handler,Default_Handler,Default_Handler,
    Default_Handler,Default_Handler,Default_Handler,0,0,0,0,
    Default_Handler,Default_Handler,0,Default_Handler,SysTick_Handler,
    Default_Handler,TIMER0_IRQHandler,Default_Handler,Default_Handler,
    Default_Handler,Default_Handler,Default_Handler,Default_Handler,
    Default_Handler,Default_Handler,Default_Handler,Default_Handler,
    Default_Handler,Default_Handler,Default_Handler,Default_Handler,
    Default_Handler,Default_Handler,Default_Handler,Default_Handler,
    Default_Handler,Default_Handler,ADC_IRQHandler,Default_Handler,
    Default_Handler,Default_Handler,Default_Handler,Default_Handler,
    Default_Handler,Default_Handler,Default_Handler,Default_Handler,
    Default_Handler,Default_Handler,Default_Handler
};
void Reset_Handler(void) {
    uint32_t *src=&_sidata;
    for(uint32_t *dst=&_sdata;dst<&_edata;) *dst++=*src++;
    for(uint32_t *dst=&_sbss;dst<&_ebss;) *dst++=0;
    (void)main();
    for(;;) {}
}
