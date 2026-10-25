#include "scope.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static Scope s;
static uint16_t frame[SCOPE_N];
static ScopeConfig cfg={5000,2048,16,MODE_NORMAL,false};
static void edge_capture(bool fall) {
    cfg.falling=fall;cfg.mode=MODE_NORMAL;scope_arm(&s,cfg);
    for(unsigned i=0;i<1600;++i)assert(!scope_feed(&s,fall?3000:1000));
    assert(!scope_feed(&s,fall?1000:3000));
    for(unsigned i=0;i<748;++i)assert(!scope_feed(&s,fall?1000:3000));
    assert(scope_feed(&s,fall?1000:3000));scope_copy(&s,frame);
    for(unsigned i=0;i<SCOPE_PRE;++i)assert(frame[i]==(fall?3000:1000));
    for(unsigned i=SCOPE_PRE;i<SCOPE_N;++i)assert(frame[i]==(fall?1000:3000));
    assert(!s.forced); assert(scope_feed(&s,0));
    uint16_t frozen=frame[999];scope_copy(&s,frame);assert(frame[999]==frozen);
}
int main(void) {
    edge_capture(false);edge_capture(true);
    cfg.falling=false;cfg.mode=MODE_AUTO;scope_arm(&s,cfg);
    unsigned samples=0;
    while(!scope_feed(&s,1777)) {assert(++samples<3000);}
    assert(samples+1==1250+749);assert(s.forced);scope_copy(&s,frame);
    for(unsigned i=0;i<SCOPE_N;++i)assert(frame[i]==1777);
    cfg.mode=MODE_NORMAL;scope_arm(&s,cfg);
    for(unsigned i=0;i<20000;++i)assert(!scope_feed(&s,1777));
    cfg.mode=MODE_SINGLE;scope_arm(&s,cfg);
    for(unsigned i=0;i<20000;++i)assert(!scope_feed(&s,1777));
    assert(!scope_feed(&s,2500));
    for(unsigned i=0;i<749;++i)scope_feed(&s,2500);
    assert(s.ready && !s.forced);
    scope_arm(&s,cfg);
    for(unsigned i=0;i<10000;++i)assert(!scope_feed(&s,(uint16_t)(2048+(i%2?4:-4))));
    cfg.mode=MODE_NORMAL;scope_arm(&s,cfg);
    for(unsigned i=0;i<100;++i)scope_feed(&s,1000);
    for(unsigned i=0;i<1000;++i)assert(!scope_feed(&s,3000));
    assert(!s.triggered); /* An edge before the pre-trigger buffer is full is ignored. */
    for(unsigned i=0;i<SCOPE_N;++i)frame[i]=(uint16_t)lround(2048+1000*sin(2*3.141592653589793*100*i/5000));
    Measurements m=scope_measure(frame,SCOPE_N,5000,3300000);
    assert(abs((int)m.frequency_mhz-100000)<50);
    assert(m.vpp_uv>1600000 && m.vpp_uv<1620000 && !m.clipped);
    for(unsigned i=0;i<SCOPE_N;++i)frame[i]=2048;
    m=scope_measure(frame,SCOPE_N,5000,3300000);assert(m.frequency_mhz==0 && m.vpp_uv==0);
    frame[0]=0;frame[999]=4095;m=scope_measure(frame,SCOPE_N,5000,3300000);assert(m.clipped);
    assert(scope_y(0,150)==149 && scope_y(4095,150)==0 && scope_y(2,0)==0);
    /* Randomized captures exercise wraparound and all legal output coordinates. */
    for(unsigned trial=0;trial<100;++trial) {
        cfg.mode=MODE_AUTO;cfg.falling=(trial&1)!=0;scope_arm(&s,cfg);
        for(unsigned i=0;i<3000 && !s.ready;++i)scope_feed(&s,(uint16_t)(rand()&4095));
        assert(s.ready);scope_copy(&s,frame);
        for(unsigned i=0;i<SCOPE_N;++i)assert(scope_y(frame[i],150)<150);
    }
    puts("PASS: rising/falling, ring wrap, pre-trigger alignment, AUTO timeout, NORMAL wait, SINGLE, hysteresis, frozen frames, measurements, clipping, randomized captures");
    return 0;
}
