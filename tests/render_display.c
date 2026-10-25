/* Host adapter runs the actual firmware renderer against a pixel buffer. */
#include "hw.h"
#include "display.h"
#include "scope.h"
#include "fft.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
static uint16_t screen[240][320];
static unsigned x0,y0,x1,y1,x,y;
void delay_ms(uint32_t n){(void)n;}
void tft_bus_init(void){}
void tft_reset(bool high){(void)high;}
void tft_bus_command(uint8_t c,const uint8_t *d,uint32_t n){
    if(c==0x2A){assert(n==4);x0=(d[0]<<8)|d[1];x1=(d[2]<<8)|d[3];}
    if(c==0x2B){assert(n==4);y0=(d[0]<<8)|d[1];y1=(d[2]<<8)|d[3];}
    if(c==0x2C){x=x0;y=y0;assert(x1<320&&y1<240);}
}
static void pixel(uint16_t p){
    assert(x<320&&y<240);screen[y][x]=p;
    if(++x>x1){x=x0;if(++y>y1)y=y0;}
}
void tft_bus_pixels(const uint16_t *p,uint32_t n){while(n--)pixel(*p++);}
void tft_bus_repeat(uint16_t c,uint32_t n){while(n--)pixel(c);}
int main(void){
    static Scope s;static uint16_t frame[SCOPE_N];
    ScopeConfig cfg={5000,2048,16,MODE_AUTO,false};scope_arm(&s,cfg);
    for(unsigned i=0;!s.ready&&i<10000;++i)
        scope_feed(&s,(uint16_t)lround(2048+1241*sin(2*3.141592653589793*100*i/5000)));
    assert(s.ready);scope_copy(&s,frame);display_init();display_frame(frame,cfg,"TRIG",200);
    FILE *f=fopen("build/firmware_preview.ppm","wb");assert(f);
    fprintf(f,"P6\n320 240\n255\n");
    for(unsigned j=0;j<240;++j)for(unsigned i=0;i<320;++i){uint16_t p=screen[j][i];
        fputc(((p>>11)&31)*255/31,f);fputc(((p>>5)&63)*255/63,f);fputc((p&31)*255/31,f);}
    fclose(f);
    static uint16_t mag[FFT_BINS];
    scope_fft(frame, mag);
    display_fft(mag, cfg, 100000);
    FILE *f2=fopen("build/fft_preview.ppm","wb");assert(f2);
    fprintf(f2,"P6\n320 240\n255\n");
    for(unsigned j=0;j<240;++j)for(unsigned i=0;i<320;++i){uint16_t p=screen[j][i];
        fputc(((p>>11)&31)*255/31,f2);fputc(((p>>5)&63)*255/63,f2);fputc((p&31)*255/31,f2);}
    fclose(f2);
    puts("PASS: actual TFT renderer, pixel/window bounds, simulated sine capture, FFT spectrum");return 0;
}
