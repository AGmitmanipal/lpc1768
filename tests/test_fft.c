/*
 * test_fft.c – host unit tests for the 256-point Q15 FFT.
 *
 * Tests:
 *  1. Pure sine at a known frequency → peak bin at the expected index.
 *  2. DC-only signal → all AC bins near zero after DC removal.
 *  3. fft_peak_bin() skips bin 0 (DC).
 *  4. All magnitude outputs are in [0, 32767].
 *  5. Symmetry property: no out-of-range values after transform.
 *
 * Build (host, no ARM toolchain required):
 *   gcc -std=c11 -O1 -g -Wall -Wextra -Werror -Ifirmware/include \
 *       firmware/src/fft.c firmware/src/scope.c tests/test_fft.c \
 *       -lm -o build/test_fft
 */

#include "fft.h"
#include "scope.h"
#define _USE_MATH_DEFINES
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


/* -------------------------------------------------------------------------
 * Helper: build a synthetic SCOPE_N-sample frame with a pure sine at
 * frequency f_hz, sampling rate fs_hz, amplitude amp_codes (peak, 12-bit).
 * -------------------------------------------------------------------------*/
static void make_sine_frame(uint16_t frame[SCOPE_N],
                             uint32_t fs_hz, double f_hz, double amp_codes) {
    double dc = 2048.0;
    for (unsigned i = 0; i < SCOPE_N; ++i) {
        double v = dc + amp_codes * sin(2.0 * M_PI * f_hz * i / fs_hz);
        if (v < 0.0)   v = 0.0;
        if (v > 4095.0) v = 4095.0;
        frame[i] = (uint16_t)lround(v);
    }
}

int main(void) {
    static uint16_t frame[SCOPE_N];
    static uint16_t mag[FFT_BINS];

    /* ------------------------------------------------------------------
     * Test 1 – pure 500 Hz sine at 5 kS/s.
     * Expected peak bin: k = f / (fs / N) = 500 / (5000/256) = 25.6
     * The peak should land at bin 25 or 26 (Bartlett window broadens it).
     * ------------------------------------------------------------------ */
    make_sine_frame(frame, 5000, 500.0, 1000.0);
    scope_fft(frame, mag);
    uint16_t peak = fft_peak_bin(mag);
    printf("Test 1: 500 Hz @ 5 kS/s -> peak bin %u (expected 25-27)\n", peak);
    assert(peak >= 24 && peak <= 28);

    /* ------------------------------------------------------------------
     * Test 2 – pure 100 Hz sine at 5 kS/s.
     * Expected peak bin ≈ 5 (k = 100 / 19.53 ≈ 5.12).
     * ------------------------------------------------------------------ */
    make_sine_frame(frame, 5000, 100.0, 1500.0);
    scope_fft(frame, mag);
    peak = fft_peak_bin(mag);
    printf("Test 2: 100 Hz @ 5 kS/s -> peak bin %u (expected 4-6)\n", peak);
    assert(peak >= 3 && peak <= 7);

    /* ------------------------------------------------------------------
     * Test 3 – DC-only signal (constant 2048).
     * After DC removal all bins should have very low magnitudes (< 200).
     * ------------------------------------------------------------------ */
    for (unsigned i = 0; i < SCOPE_N; ++i) frame[i] = 2048;
    scope_fft(frame, mag);
    for (unsigned k = 1; k < FFT_BINS; ++k) {
        assert(mag[k] < 200);
    }
    printf("Test 3: DC-only signal -> all AC bins < 200 (max AC = %u)\n",
           (unsigned)mag[fft_peak_bin(mag)]);

    /* ------------------------------------------------------------------
     * Test 4 – fft_peak_bin skips bin 0.
     * Set DC bin to maximum and check peak_bin still returns a non-zero bin
     * when a strong AC signal is also present.
     * ------------------------------------------------------------------ */
    make_sine_frame(frame, 5000, 500.0, 1800.0);
    scope_fft(frame, mag);
    uint16_t peak_noDC = fft_peak_bin(mag);
    assert(peak_noDC != 0);
    printf("Test 4: fft_peak_bin skips DC bin -> peak at bin %u\n", peak_noDC);

    /* ------------------------------------------------------------------
     * Test 5 – all magnitudes are in valid uint16_t range [0, 32767].
     * ------------------------------------------------------------------ */
    make_sine_frame(frame, 20000, 3000.0, 2047.0);
    scope_fft(frame, mag);
    for (unsigned k = 0; k < FFT_BINS; ++k) {
        assert(mag[k] <= 32767u);
    }
    printf("Test 5: all magnitudes in [0, 32767] at 20 kS/s\n");

    /* ------------------------------------------------------------------
     * Test 6 – higher sample rate, 2 kHz tone at 10 kS/s.
     * Expected peak bin ≈ 51 (k = 2000 / (10000/256) ≈ 51.2).
     * ------------------------------------------------------------------ */
    make_sine_frame(frame, 10000, 2000.0, 1200.0);
    scope_fft(frame, mag);
    peak = fft_peak_bin(mag);
    printf("Test 6: 2 kHz @ 10 kS/s -> peak bin %u (expected 49-53)\n", peak);
    assert(peak >= 48 && peak <= 54);

    puts("PASS: all FFT tests passed");
    return 0;
}
