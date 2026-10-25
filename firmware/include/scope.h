#ifndef SCOPE_H
#define SCOPE_H
#include <stdbool.h>
#include "fft.h"
#include <stdint.h>
#define SCOPE_N 1000u
#define SCOPE_PRE 250u
typedef enum { MODE_AUTO, MODE_NORMAL, MODE_SINGLE } ScopeMode;
typedef struct {
    uint32_t sample_hz;
    uint16_t threshold, hysteresis;
    ScopeMode mode;
    bool falling;
} ScopeConfig;
typedef struct {
    ScopeConfig cfg;
    uint16_t ring[SCOPE_N];
    uint32_t waited;
    uint16_t write, filled, remaining;
    bool armed_edge, triggered, ready, forced;
} Scope;
typedef struct {
    uint16_t minimum, maximum;
    uint32_t mean_uv, vpp_uv, frequency_mhz;
    bool clipped;
} Measurements;
void scope_arm(Scope *s, ScopeConfig cfg);
bool scope_feed(Scope *s, uint16_t code);
void scope_copy(const Scope *s, uint16_t output[SCOPE_N]);
Measurements scope_measure(const uint16_t *data, uint16_t n,
                           uint32_t sample_hz, uint32_t reference_uv);
uint16_t scope_y(uint16_t code, uint16_t height);
/*
 * scope_fft – extract FFT_N ADC samples from a captured frame, centre-DC
 * remove, window (Hann), convert to Q15, and compute the FFT magnitude.
 *
 * @param data      Full SCOPE_N-sample frame from scope_copy().
 * @param mag       Output: FFT_BINS magnitude bins (caller-supplied).
 */
void scope_fft(const uint16_t data[SCOPE_N], uint16_t mag[FFT_BINS]);
#endif
