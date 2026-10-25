#include "hw.h"
#include "config.h"
#if ENABLE_LCD
static void lcd_nibble(uint8_t x) {
    CLR(1)=15u<<20;SET(1)=(uint32_t)(x&15u)<<20;
    SET(1)=1u<<19;delay_ms(1);CLR(1)=1u<<19;delay_ms(1);
}
static void lcd_byte(uint8_t b,bool data) {
    if(data)SET(1)=1u<<18;else CLR(1)=1u<<18;
    lcd_nibble(b>>4);lcd_nibble(b);delay_ms(2);
}
static void lcd_line(uint8_t address,const char *s) {
    lcd_byte(address,false);
    for(uint32_t i=0;i<16;++i)lcd_byte(*s?(uint8_t)*s++:' ',true);
}
#endif
#if ENABLE_TM1637
/* DIO and CLK are open drain. External 4.7k pullups go to 3.3V. */
static void tm_pause(void) { for(volatile uint32_t i=0;i<100;++i){} }
static void tm_clock(bool hi) {if(hi)SET(1)=1u<<24;else CLR(1)=1u<<24;tm_pause();}
static void tm_data(bool hi) {if(hi)SET(1)=1u<<25;else CLR(1)=1u<<25;tm_pause();}
static void tm_start(void) {tm_data(true);tm_clock(true);tm_data(false);tm_clock(false);}
static void tm_stop(void) {tm_clock(false);tm_data(false);tm_clock(true);tm_data(true);}
static void tm_byte(uint8_t value) {
    for(uint32_t i=0;i<8;++i){tm_clock(false);tm_data((value&1u)!=0);tm_clock(true);value>>=1;}
    tm_clock(false);tm_data(true);tm_clock(true);tm_clock(false); /* ACK clock. */
}
#endif
void aux_init(void) {
#if ENABLE_LCD
    PINSEL(3)&=~(0xFFFu<<4);DIR(1)|=0x3Fu<<18;CLR(1)=0x3Fu<<18;
    delay_ms(40);lcd_nibble(3);delay_ms(5);lcd_nibble(3);lcd_nibble(3);lcd_nibble(2);
    lcd_byte(0x28,false);lcd_byte(0x0C,false);lcd_byte(0x06,false);lcd_byte(0x01,false);
#endif
#if ENABLE_TM1637
    PINSEL(3)&=~(15u<<16);PINOD(1)|=3u<<24;SET(1)=3u<<24;DIR(1)|=3u<<24;
#endif
}
void aux_update(uint32_t hz,const char *status) {
    (void)hz;(void)status;
#if ENABLE_LCD
    char line[]="FS:00 KHZ N:1000";
    line[3]=(char)('0'+hz/10000u);line[4]=(char)('0'+(hz/1000u)%10);
    lcd_line(0x80,line);lcd_line(0xC0,status);
#endif
#if ENABLE_TM1637
    static const uint8_t segments[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
    uint32_t value=hz/10u;
    uint8_t digits[4];
    for(int i=3;i>=0;--i){digits[i]=segments[value%10u];value/=10u;}
    digits[1]|=0x80; /* 05.00 kHz, on modules with decimal-point digits. */
    tm_start();tm_byte(0x40);tm_stop();tm_start();tm_byte(0xC0);
    for(uint32_t i=0;i<4;++i)tm_byte(digits[i]);
    tm_stop();
    tm_start();tm_byte(0x8A);tm_stop();
#endif
}
