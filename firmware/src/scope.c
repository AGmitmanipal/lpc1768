#include "scope.h"
#include "fft.h"


void scope_arm(Scope *s, ScopeConfig cfg) {
    if (cfg.threshold < cfg.hysteresis) cfg.threshold = cfg.hysteresis;
    if (cfg.threshold > 4095u - cfg.hysteresis)
        cfg.threshold = (uint16_t)(4095u - cfg.hysteresis);
    s->cfg = cfg;
    s->write = s->filled = s->remaining = 0;
    s->waited = 0;
    s->armed_edge = s->triggered = s->ready = s->forced = false;
}

bool scope_feed(Scope *s, uint16_t code) {
    if (s->ready) return true;
    code &= 4095u;
    s->ring[s->write] = code;
    if (++s->write == SCOPE_N) s->write = 0;
    if (s->filled < SCOPE_N) ++s->filled;
    if (s->triggered) {
        if (--s->remaining == 0) s->ready = true;
        return s->ready;
    }
    if (s->waited != UINT32_MAX) ++s->waited;
    bool crossing = s->armed_edge &&
        (s->cfg.falling ? code <= s->cfg.threshold : code >= s->cfg.threshold);
    if (s->cfg.falling ? code >= s->cfg.threshold + s->cfg.hysteresis :
                        code <= s->cfg.threshold - s->cfg.hysteresis)
        s->armed_edge = true;
    /* Ignore early crossings: all displayed pre-trigger samples must be valid. */
    if (crossing && s->filled <= SCOPE_PRE) s->armed_edge = false;
    if (s->filled > SCOPE_PRE) {
        bool timeout = s->cfg.mode == MODE_AUTO && s->waited >= s->cfg.sample_hz / 4u;
        if (crossing || timeout) {
            s->triggered = true;
            s->forced = !crossing;
            s->remaining = SCOPE_N - SCOPE_PRE - 1u;
        }
    }
    return false;
}

void scope_copy(const Scope *s, uint16_t output[SCOPE_N]) {
    /* When complete, the next write index is the oldest retained sample. */
    uint16_t j = s->write;
    for (uint16_t i = 0; i < SCOPE_N; ++i) {
        output[i] = s->ring[j];
        if (++j == SCOPE_N) j = 0;
    }
}

Measurements scope_measure(const uint16_t *data, uint16_t n,
                           uint32_t sample_hz, uint32_t reference_uv) {
    Measurements m = {4095, 0, 0, 0, 0, false};
    if (!n) { m.minimum = 0; return m; }
    uint32_t sum = 0;
    for (uint16_t i = 0; i < n; ++i) {
        if (data[i] < m.minimum) m.minimum = data[i];
        if (data[i] > m.maximum) m.maximum = data[i];
        sum += data[i];
    }
    m.mean_uv = (uint32_t)(((uint64_t)sum * reference_uv) / ((uint32_t)n * 4095u));
    m.vpp_uv = (uint32_t)(((uint64_t)(m.maximum - m.minimum) * reference_uv) / 4095u);
    m.clipped = m.minimum <= 2 || m.maximum >= 4093;
    if (m.maximum - m.minimum < 32) return m;
    uint16_t mid = (uint16_t)((m.minimum + m.maximum) / 2u);
    uint16_t hyst = (uint16_t)((m.maximum - m.minimum) / 16u);
    if (hyst < 4) hyst = 4;
    bool armed = false;
    uint32_t first = 0, last = 0, count = 0;
    for (uint16_t i = 1; i < n; ++i) {
        if (data[i-1] <= mid - hyst) armed = true;
        if (armed && data[i-1] < mid && data[i] >= mid) {
            uint32_t t = ((uint32_t)(i - 1u) << 16) +
                (((uint32_t)(mid - data[i-1]) << 16) / (data[i] - data[i-1]));
            if (!count) first = t;
            last = t; ++count; armed = false;
        }
    }
    if (count >= 2 && last > first)
        m.frequency_mhz = (uint32_t)(((uint64_t)sample_hz * 1000u * 65536u *
                                      (count - 1u)) / (last - first));
    return m;
}

uint16_t scope_y(uint16_t code, uint16_t height) {
    if (!height) return 0;
    return (uint16_t)(((uint32_t)(4095u - (code & 4095u)) * (height - 1u)) / 4095u);
}

/*
 * scope_fft
 *
 * Takes FFT_N samples from the captured frame starting at the trigger point
 * (SCOPE_PRE), computes the mean for DC removal, applies a Hann window with
 * integer arithmetic, scales to Q15, then delegates to fft_256().
 *
 * Hann window: w[n] = 0.5 - 0.5 * cos(2πn / (N-1))
 * Integer approximation per sample (avoids libm on the target):
 *   w[n] ≈ (FFT_N - 1 - |2n - (FFT_N-1)|) scaled to Q15 range
 * More precisely we use the identity:
 *   w[n] = (N/2 - N/2 * cos(2πn/N)) but via a lookup is overkill;
 * instead we use a triangular approximation good enough for DSO display.
 *
 * For a teaching DSO the triangular (Bartlett) window is acceptable and
 * avoids a second twiddle table. Replace with TW_COS if better sidelobes
 * are needed.
 */
void scope_fft(const uint16_t data[SCOPE_N], uint16_t mag[FFT_BINS]) {
    static int16_t buf[FFT_N];

    /* Pick FFT_N samples starting at the trigger point. */
    const uint16_t start = SCOPE_PRE;
    const uint16_t end   = (uint16_t)(start + FFT_N);   /* 250 … 506 */

    /* DC removal: compute mean of the selected window. */
    uint32_t sum = 0;
    for (uint16_t i = start; i < end; ++i) sum += data[i];
    uint16_t mean = (uint16_t)(sum / FFT_N);

    /* Bartlett (triangular) window + DC removal + Q15 conversion.
     * ADC codes are 0…4095 (12-bit); centring and scaling to ±32767:
     *   centred = (data[i] - mean) as signed, range ±2047 in practice
     *   Q15 = centred * 16  (shifts 12-bit to 16-bit, keeps 1 sign bit)
     *   windowed = Q15 * w[n], where w[n] = 2*(half - |n - half|) / FFT_N
     */
    const uint16_t half = FFT_N / 2u;
    for (uint16_t n = 0; n < FFT_N; ++n) {
        int32_t s = (int32_t)(int16_t)(data[(uint16_t)(start + n)] - mean);
        s *= 16;  /* scale 12-bit signed to 16-bit signed */
        /* Bartlett window weight in Q15: w = 2*min(n, N-1-n) / (N-1) * 32767 */
        uint16_t dist = (n < half) ? n : (uint16_t)(FFT_N - 1u - n);
        int32_t w15 = (int32_t)dist * 32767 / (int32_t)(half);
        s = (s * w15) >> 15;
        if (s >  32767) s =  32767;
        if (s < -32768) s = -32768;
        buf[n] = (int16_t)s;
    }

    fft_256(buf, mag);
}
