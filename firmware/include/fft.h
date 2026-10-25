/*
 * fft.h – 256-point radix-2 DIT FFT, Q15 fixed-point arithmetic.
 *
 * Input : 256 signed 16-bit samples (Q15, full-scale ±32767).
 * Output: FFT_BINS magnitude bins (0 … Fs/2), 16-bit, linear scale.
 *         Bin k corresponds to frequency k * Fs / FFT_N.
 *
 * No dynamic allocation.  All state is held in the caller-supplied buffers.
 * Safe to call from the main loop after a frame capture is complete.
 */
#ifndef FFT_H
#define FFT_H

#include <stdint.h>

#define FFT_N    256u          /* Transform length (power of two).           */
#define FFT_BINS (FFT_N / 2u)  /* Useful output bins (DC … Nyquist).         */

/*
 * fft_256 – compute the FFT and return linear magnitude bins.
 *
 * @param samples  Input: FFT_N signed 16-bit values in Q15 format.
 *                 The array is used as scratch space during the transform
 *                 and its contents are undefined afterwards.
 * @param mag      Output: FFT_BINS unsigned 16-bit magnitude values.
 *                 mag[0] = DC bin, mag[FFT_BINS-1] ≈ Nyquist bin.
 *                 Scale: 0 … 32767 (relative, not calibrated to volts).
 */
void fft_256(int16_t samples[FFT_N], uint16_t mag[FFT_BINS]);

/*
 * fft_peak_bin – return the index of the bin with the highest magnitude,
 * excluding DC (bin 0).  Returns 0 if all bins are zero.
 */
uint16_t fft_peak_bin(const uint16_t mag[FFT_BINS]);

#endif /* FFT_H */
