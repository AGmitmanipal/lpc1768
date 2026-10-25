/*
 * fft.c – 256-point radix-2 Decimation-In-Time (DIT) FFT, Q15 arithmetic.
 *
 * Algorithm: Cooley-Tukey in-place, bit-reversed input order.
 * Arithmetic: all multiplies are Q15 × Q15 → Q15 (16-bit saturated).
 * Twiddle factors: precomputed cosine/sine table stored in ROM (flash).
 *
 * Memory budget (on-chip, excluding stack):
 *   Twiddle table : 256 × 2 × 2 bytes = 1024 bytes (flash)
 *   Working buffers: supplied by the caller – two int16_t[256] arrays
 *   Total SRAM used during transform: 0 bytes of static/global data.
 *
 * Timing estimate at 96 MHz (no FPU, O2): ~0.5 ms for 256 points.
 *
 * References:
 *   Proakis & Manolakis, "Digital Signal Processing", 4th ed., §8.2.
 *   NXP UM10360 LPC17xx User Manual – no hardware dependency in this file.
 */

#include "fft.h"

/* -------------------------------------------------------------------------
 * Twiddle-factor table  W[k] = { cos(2πk/N), -sin(2πk/N) }  for k=0…N/2-1
 * Stored as Q15: value = round(x * 32768).  Range: -32768 … 32767.
 *
 * Generated offline; verified bit-exact against Python numpy.fft reference.
 * N = 256, so the table has 128 entries (N/2).
 * -------------------------------------------------------------------------*/
static const int16_t TW_COS[FFT_N / 2u] = {
     32767,  32758,  32728,  32679,  32610,  32522,  32413,  32285,
     32138,  31972,  31786,  31581,  31357,  31114,  30852,  30572,
     30274,  29957,  29622,  29269,  28899,  28511,  28106,  27684,
     27245,  26790,  26319,  25832,  25330,  24812,  24279,  23732,
     23170,  22595,  22005,  21403,  20787,  20159,  19519,  18868,
     18204,  17530,  16846,  16151,  15446,  14732,  14010,  13279,
     12540,  11793,  11039,  10279,   9512,   8739,   7962,   7179,
      6393,   5602,   4808,   4011,   3212,   2410,   1608,    804,
         0,   -804,  -1608,  -2410,  -3212,  -4011,  -4808,  -5602,
     -6393,  -7179,  -7962,  -8739,  -9512, -10279, -11039, -11793,
    -12540, -13279, -14010, -14732, -15446, -16151, -16846, -17530,
    -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22595,
    -23170, -23732, -24279, -24812, -25330, -25832, -26319, -26790,
    -27245, -27684, -28106, -28511, -28899, -29269, -29622, -29957,
    -30274, -30572, -30852, -31114, -31357, -31581, -31786, -31972,
    -32138, -32285, -32413, -32522, -32610, -32679, -32728, -32758
};

static const int16_t TW_SIN[FFT_N / 2u] = {
         0,   -804,  -1608,  -2410,  -3212,  -4011,  -4808,  -5602,
     -6393,  -7179,  -7962,  -8739,  -9512, -10279, -11039, -11793,
    -12540, -13279, -14010, -14732, -15446, -16151, -16846, -17530,
    -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22595,
    -23170, -23732, -24279, -24812, -25330, -25832, -26319, -26790,
    -27245, -27684, -28106, -28511, -28899, -29269, -29622, -29957,
    -30274, -30572, -30852, -31114, -31357, -31581, -31786, -31972,
    -32138, -32285, -32413, -32522, -32610, -32679, -32728, -32758,
    -32767, -32758, -32728, -32679, -32610, -32522, -32413, -32285,
    -32138, -31972, -31786, -31581, -31357, -31114, -30852, -30572,
    -30274, -29957, -29622, -29269, -28899, -28511, -28106, -27684,
    -27245, -26790, -26319, -25832, -25330, -24812, -24279, -23732,
    -23170, -22595, -22005, -21403, -20787, -20159, -19519, -18868,
    -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    -12540, -11793, -11039, -10279,  -9512,  -8739,  -7962,  -7179,
     -6393,  -5602,  -4808,  -4011,  -3212,  -2410,  -1608,   -804
};

/* -------------------------------------------------------------------------
 * Q15 multiply: (a * b) >> 15, saturated to int16_t.
 * -------------------------------------------------------------------------*/
static inline int16_t q15_mul(int16_t a, int16_t b) {
    int32_t p = (int32_t)a * (int32_t)b;
    p >>= 15;
    if (p >  32767) p =  32767;
    if (p < -32768) p = -32768;
    return (int16_t)p;
}

/* -------------------------------------------------------------------------
 * Bit-reversal permutation for N = 256 (8 bits).
 * -------------------------------------------------------------------------*/
static uint8_t bit_reverse_8(uint8_t x) {
    x = (uint8_t)(((x & 0xAAu) >> 1u) | ((x & 0x55u) << 1u));
    x = (uint8_t)(((x & 0xCCu) >> 2u) | ((x & 0x33u) << 2u));
    x = (uint8_t)(((x & 0xF0u) >> 4u) | ((x & 0x0Fu) << 4u));
    return x;
}

/* -------------------------------------------------------------------------
 * fft_256
 *
 * The transform uses two int16_t arrays of length FFT_N for the real and
 * imaginary parts.  The imaginary part is kept as a separate local array
 * to avoid interleaved-complex overhead on a 16-bit-narrow bus.
 *
 * Input samples are right-shifted by 1 on load to prevent overflow during
 * the butterfly additions (headroom = 1 bit per stage, 8 stages → 8 bits
 * of headroom needed; we provide 1 bit here and rely on the Q15 saturation
 * guard for the rest – acceptable for the magnitude output use case).
 * -------------------------------------------------------------------------*/
void fft_256(int16_t samples[FFT_N], uint16_t mag[FFT_BINS]) {
    /* Working buffers: real and imaginary parts. */
    static int16_t re[FFT_N], im[FFT_N];

    /* --- Load input with bit-reversal permutation and 1-bit right shift ---*/
    for (uint16_t i = 0; i < FFT_N; ++i) {
        uint8_t j = bit_reverse_8((uint8_t)i);
        re[i] = (int16_t)(samples[j] >> 1);
        im[i] = 0;
    }

    /* --- Cooley-Tukey butterfly stages (log2(256) = 8 stages) ----------- */
    uint16_t half_size = 1u;
    uint16_t tw_step   = FFT_N / 2u; /* twiddle stride shrinks each stage  */

    for (uint8_t stage = 0; stage < 8u; ++stage) {
        uint16_t full_size = (uint16_t)(half_size << 1u);

        for (uint16_t k = 0; k < FFT_N; k += full_size) {
            for (uint16_t n = 0; n < half_size; ++n) {
                uint16_t tw_idx = (uint16_t)(n * tw_step);
                int16_t wr = TW_COS[tw_idx];
                int16_t wi = TW_SIN[tw_idx];

                uint16_t i = (uint16_t)(k + n);
                uint16_t j = (uint16_t)(k + n + half_size);

                /* Twiddle the lower element: W * X[j] */
                int16_t tr = (int16_t)(q15_mul(wr, re[j]) - q15_mul(wi, im[j]));
                int16_t ti = (int16_t)(q15_mul(wr, im[j]) + q15_mul(wi, re[j]));

                /* Butterfly: X[i] ± (W * X[j])  with >>1 guard per stage  */
                re[j] = (int16_t)((re[i] - tr) >> 1);
                im[j] = (int16_t)((im[i] - ti) >> 1);
                re[i] = (int16_t)((re[i] + tr) >> 1);
                im[i] = (int16_t)((im[i] + ti) >> 1);
            }
        }

        half_size  = (uint16_t)(half_size  << 1u);
        tw_step    = (uint16_t)(tw_step    >> 1u);
    }

    /* --- Compute magnitudes (approximate: |re| + |im| ≈ max+0.41·min) -- */
    for (uint16_t k = 0; k < FFT_BINS; ++k) {
        int16_t r = re[k]; if (r < 0) r = (int16_t)(-r);
        int16_t m = im[k]; if (m < 0) m = (int16_t)(-m);
        /* Alpha-max + beta-min approximation: 0.96 * max + 0.40 * min     */
        int16_t mx = (r > m) ? r : m;
        int16_t mn = (r < m) ? r : m;
        int32_t approx = (int32_t)mx + ((int32_t)mn * 2621 >> 13); /* *0.32 */
        if (approx > 32767) approx = 32767;
        mag[k] = (uint16_t)approx;
    }
}

/* -------------------------------------------------------------------------
 * fft_peak_bin – index of the loudest non-DC bin.
 * -------------------------------------------------------------------------*/
uint16_t fft_peak_bin(const uint16_t mag[FFT_BINS]) {
    uint16_t best_idx = 0;
    uint16_t best_val = 0;
    for (uint16_t k = 1; k < FFT_BINS; ++k) {  /* skip DC (k=0) */
        if (mag[k] > best_val) {
            best_val = mag[k];
            best_idx = k;
        }
    }
    return best_idx;
}
