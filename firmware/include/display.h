#ifndef DISPLAY_H
#define DISPLAY_H
#include "scope.h"
#include "fft.h"
void display_init(void);
void display_status(ScopeConfig cfg, const char *status, uint16_t window);
void display_frame(const uint16_t data[SCOPE_N], ScopeConfig cfg,
                   const char *status, uint16_t window);
/*
 * display_fft – render the FFT magnitude spectrum as vertical bars.
 * Draws over the same grid region as display_frame.
 * @param mag      FFT_BINS magnitude values from scope_fft().
 * @param cfg      Current scope config (for Fs labelling).
 * @param peak_hz  Frequency of the dominant bin in mHz (same unit as
 *                 Measurements.frequency_mhz), or 0 if not detected.
 */
void display_fft(const uint16_t mag[FFT_BINS], ScopeConfig cfg,
                 uint32_t peak_hz);
#endif

