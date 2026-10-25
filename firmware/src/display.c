#include "display.h"
#include "hw.h"
#include "config.h"
#include "font.h"
#define BG 0x0843u
#define GRID 0x21A7u
#define FG 0xEFBFu
#define TRACE 0x5FEAu
#define TRIGGER 0xFC40u
#define GW 300u
#define GH 150u
#define GX 10u
#define GY 64u
static void command(uint8_t c, uint8_t value) { tft_bus_command(c,&value,1); }
static void area(uint16_t x,uint16_t y,uint16_t w,uint16_t h) {
    uint16_t right=(uint16_t)(x+w-1u), bottom=(uint16_t)(y+h-1u);
    uint8_t a[4]={(uint8_t)(x>>8),(uint8_t)x,(uint8_t)(right>>8),(uint8_t)right};
    tft_bus_command(0x2A,a,4);
    a[0]=(uint8_t)(y>>8);a[1]=(uint8_t)y;a[2]=(uint8_t)(bottom>>8);a[3]=(uint8_t)bottom;
    tft_bus_command(0x2B,a,4);tft_bus_command(0x2C,0,0);
}
static void rect(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t c) {
    area(x,y,w,h);tft_bus_repeat(c,(uint32_t)w*h);
}
static void text(uint16_t x,uint16_t y,const char *s,uint16_t color) {
    uint16_t pixels[48];
    while (*s && x+6<=320) {
        const uint8_t *glyph=font_glyph(*s++);
        for (uint32_t row=0;row<8;++row)
            for (uint32_t col=0;col<6;++col)
                pixels[row*6+col]=(col<5 && row<7 && (glyph[row]&(1u<<(4-col))))?color:BG;
        area(x,y,6,8);tft_bus_pixels(pixels,48); x+=6;
    }
}
static char *number(char *p,uint32_t n) {
    char t[11]; uint32_t k=0;
    do { t[k++]=(char)('0'+n%10); n/=10; } while(n);
    while(k) *p++=t[--k];
    *p=0;return p;
}
static char *append(char *p,const char *s) { while(*s) *p++=*s++;*p=0;return p; }
static char *volts(char *p,uint32_t uv) {
    p=number(p,uv/1000000u);*p++='.';
    *p++=(char)('0'+(uv/100000u)%10);*p++=(char)('0'+(uv/10000u)%10);
    *p++=(char)('0'+(uv/1000u)%10);*p=0;return p;
}
void display_init(void) {
    tft_bus_init();tft_reset(false);delay_ms(20);tft_reset(true);delay_ms(150);
    tft_bus_command(0x01,0,0);delay_ms(150);
    tft_bus_command(0x28,0,0); /* Display off during setup. */
    const uint8_t powerb[]={0x00,0xC1,0x30}; tft_bus_command(0xCF,powerb,3);
    const uint8_t seq[]={0x64,0x03,0x12,0x81};tft_bus_command(0xED,seq,4);
    const uint8_t timing[]={0x85,0x00,0x78};tft_bus_command(0xE8,timing,3);
    const uint8_t powera[]={0x39,0x2C,0x00,0x34,0x02};tft_bus_command(0xCB,powera,5);
    command(0xF7,0x20);
    const uint8_t timingb[]={0,0};tft_bus_command(0xEA,timingb,2);
    command(0xC0,0x23);command(0xC1,0x10);
    const uint8_t vcom[]={0x3E,0x28};tft_bus_command(0xC5,vcom,2);
    command(0xC7,0x86);command(0x36,0x28);command(0x3A,0x55);
    const uint8_t fr[]={0,0x18};tft_bus_command(0xB1,fr,2);
    const uint8_t func[]={0x08,0x82,0x27};tft_bus_command(0xB6,func,3);
    command(0xF2,0);command(0x26,1);
    tft_bus_command(0x11,0,0);delay_ms(150);tft_bus_command(0x29,0,0);delay_ms(20);
    rect(0,0,320,240,BG);text(10,8,"LPC1768  /  DIGITAL STORAGE OSCILLOSCOPE",FG);
    text(10,224,"CH1  0-3.3V   1000 SAMPLES   FRAME CAPTURE",FG);
}
void display_status(ScopeConfig cfg,const char *status,uint16_t window) {
    char b[64],*p=b;
    rect(0,24,320,18,BG);
    p=append(p,"FS ");p=number(p,cfg.sample_hz);p=append(p," HZ  ");
    const char *modes[]={"AUTO","NORMAL","SINGLE"};
    p=append(p,modes[cfg.mode]);p=append(p,cfg.falling?" FALL  ":" RISE  ");append(p,status);
    text(10,24,b,FG);
    p=b;p=append(p,"TRIG ");p=volts(p,(uint32_t)(((uint64_t)cfg.threshold*REFERENCE_UV)/4095u));
    p=append(p,"V  ");p=number(p,(uint32_t)window*100000u/cfg.sample_hz);
    append(p," US/DIV  0.66 V/DIV");text(10,35,b,TRIGGER);
}
void display_frame(const uint16_t data[SCOPE_N],ScopeConfig cfg,
                   const char *status,uint16_t window) {
    display_status(cfg,status,window);
    Measurements m=scope_measure(data,SCOPE_N,cfg.sample_hz,REFERENCE_UV);
    char b[64],*p=b;
    rect(0,47,320,9,BG);
    p=append(p,"VPP ");p=volts(p,m.vpp_uv);p=append(p,"V  F ");
    if(m.frequency_mhz) { p=number(p,m.frequency_mhz/1000);p=append(p,".");
        *p++=(char)('0'+(m.frequency_mhz/100)%10);*p=0;p=append(p,"HZ"); }
    else p=append(p,"--");
    if(m.clipped) append(p,"  CLIP");
    text(10,47,b,FG);
    uint16_t col[GH],start=(uint16_t)(SCOPE_PRE-window/4u);
    uint16_t trig_y=scope_y(cfg.threshold,GH);
    uint16_t prev=scope_y(data[start],GH);
    for(uint16_t x=0;x<GW;++x) {
        uint16_t a=(uint16_t)(start+((uint32_t)x*window)/GW);
        uint16_t end=(uint16_t)(start+((uint32_t)(x+1u)*window)/GW);
        if(end<=a) end=(uint16_t)(a+1u);
        if(end>SCOPE_N) end=SCOPE_N;
        uint16_t lo=scope_y(data[a],GH),hi=lo;
        for(uint16_t i=(uint16_t)(a+1u);i<end;++i) {
            uint16_t y=scope_y(data[i],GH);if(y<lo)lo=y;if(y>hi)hi=y;
        }
        if(window<GW) {if(prev<lo)lo=prev;if(prev>hi)hi=prev;}
        prev=scope_y(data[end-1u],GH);
        for(uint16_t y=0;y<GH;++y) {
            col[y]=(x%30u==0 || y%30u==0 || x==GW-1u || y==GH-1u)?GRID:BG;
            if(y==trig_y && x%6u<3u) col[y]=TRIGGER;
            if(x==GW/4u && y%6u<3u) col[y]=TRIGGER;
            if(y>=lo && y<=hi)col[y]=TRACE;
        }
        area((uint16_t)(GX+x),GY,1,GH);tft_bus_pixels(col,GH);
    }
}

/* -------------------------------------------------------------------------
 * display_fft
 *
 * Renders FFT_BINS (128) magnitude bins as vertical bars on the same 300×150
 * pixel grid used by display_frame().  Each bar is 300/FFT_BINS ≈ 2 px wide.
 * Bar height is normalised against the per-frame peak magnitude, excluding
 * the DC bin, so the tallest bar always fills the grid.
 *
 * Color scheme:
 *   BG     – grid background
 *   GRID   – gridlines (every 30 px)
 *   0x631F – muted purple for the DC bin (bin 0)
 *   TRACE  – teal-cyan for normal signal bins
 *   0xFD20 – amber / orange for the peak bin
 * -------------------------------------------------------------------------*/
#define BAR_DC   0x631Fu   /* muted purple  */
#define BAR_PEAK 0xFD20u   /* amber-orange  */

void display_fft(const uint16_t mag[FFT_BINS], ScopeConfig cfg,
                 uint32_t peak_hz) {
    /* --- Status strip (reuse display_status layout) ---------------------- */
    char b[64], *p = b;
    rect(0, 24, 320, 18, BG);
    p = append(p, "FFT  FS "); p = number(p, cfg.sample_hz);
    p = append(p, " HZ  PEAK ");
    if (peak_hz) {
        p = number(p, peak_hz / 1000u); *p++ = '.';
        *p++ = (char)('0' + (peak_hz / 100u) % 10u); *p = 0;
        p = append(p, " HZ");
    } else {
        p = append(p, "--");
    }
    text(10, 24, b, FG);

    /* Nyquist label on second row */
    p = b; p = append(p, "BW ");
    p = number(p, cfg.sample_hz / 2000u); *p++ = '.';
    *p++ = (char)('0' + (cfg.sample_hz / 200u) % 10u); *p = 0;
    p = append(p, " KHZ   ");
    p = number(p, FFT_BINS); p = append(p, " BINS  256-PT FFT");
    text(10, 35, b, TRIGGER);

    /* Clear measurement row */
    rect(0, 47, 320, 9, BG);
    p = b; p = append(p, "RES ");
    uint32_t res_mhz = (uint32_t)(((uint64_t)cfg.sample_hz * 1000u) / FFT_N);
    p = number(p, res_mhz / 1000u); *p++ = '.';
    *p++ = (char)('0' + (res_mhz / 100u) % 10u); *p = 0;
    append(p, " HZ/BIN");
    text(10, 47, b, FG);

    /* --- Find maximum magnitude (skip DC) for normalisation -------------- */
    uint16_t peak_mag = 1u;   /* avoid div-by-zero */
    uint16_t peak_idx = 0;
    for (uint16_t k = 1; k < FFT_BINS; ++k) {
        if (mag[k] > peak_mag) { peak_mag = mag[k]; peak_idx = k; }
    }

    /* --- Draw grid + bars ------------------------------------------------ */
    /* Each pixel column of the 300-wide grid maps to roughly 2 bins.
     * We group bins into GW pixel columns. */
    uint16_t col[GH];
    for (uint16_t x = 0; x < GW; ++x) {
        /* Map pixel column x to a bin index */
        uint16_t bin = (uint16_t)(((uint32_t)x * FFT_BINS) / GW);
        uint16_t bin_end = (uint16_t)((((uint32_t)x + 1u) * FFT_BINS) / GW);
        if (bin_end <= bin) bin_end = (uint16_t)(bin + 1u);
        if (bin_end > FFT_BINS) bin_end = FFT_BINS;

        /* Maximum magnitude across mapped bins */
        uint16_t m = mag[bin];
        bool is_peak = (bin == peak_idx);
        for (uint16_t b2 = (uint16_t)(bin + 1u); b2 < bin_end; ++b2) {
            if (mag[b2] > m) { m = mag[b2]; }
            if (b2 == peak_idx) is_peak = true;
        }

        /* Bar height in pixels: proportional to normalised magnitude */
        uint16_t bar_h = (uint16_t)(((uint32_t)m * (GH - 1u)) / peak_mag);
        uint16_t bar_top = (uint16_t)(GH - bar_h);   /* top pixel of bar */

        /* Choose bar colour */
        uint16_t bar_color = (bin == 0) ? BAR_DC :
                             is_peak    ? BAR_PEAK :
                                          TRACE;

        for (uint16_t y = 0; y < GH; ++y) {
            bool on_grid = (x % 30u == 0u || y % 30u == 0u ||
                            x == GW - 1u  || y == GH - 1u);
            if (on_grid) {
                col[y] = GRID;
            } else if (y >= bar_top) {
                col[y] = bar_color;
            } else {
                col[y] = BG;
            }
        }
        area((uint16_t)(GX + x), GY, 1, GH);
        tft_bus_pixels(col, GH);
    }
}
