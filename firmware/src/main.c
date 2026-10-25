#include "config.h"
#include "hw.h"
#include "scope.h"
#include "fft.h"
#include "acquisition.h"
#include "display.h"

static uint16_t frame[SCOPE_N];
static uint16_t fft_mag[FFT_BINS];

static const uint32_t rates[]   = {1000, 5000, 10000, 20000};
static const uint16_t windows[] = {100, 200, 500, 1000};

typedef enum { VIEW_SCOPE, VIEW_FFT } ViewMode;

/* Refresh the screen based on the current view mode. */
static void show(ViewMode view, ScopeConfig cfg, const char *status,
                 uint16_t window, bool have_frame) {
    if (!have_frame) {
        display_status(cfg, status, window);
        return;
    }
    if (view == VIEW_FFT) {
        uint16_t peak = fft_peak_bin(fft_mag);
        /* Convert bin index to frequency in mHz (matching Measurements units) */
        uint32_t peak_hz = (uint32_t)(((uint64_t)peak * cfg.sample_hz * 1000u)
                                      / FFT_N);
        display_fft(fft_mag, cfg, peak_hz);
    } else {
        display_frame(frame, cfg, status, window);
    }
}

int main(void) {
    clock_init(); board_init(); display_init(); aux_init(); acquisition_init();
    ScopeConfig cfg = {5000, 2048, 16, MODE_AUTO, false};
    uint8_t  rate  = 1, zoom = 1;
    ViewMode view  = VIEW_SCOPE;
    bool running = true, have_frame = false, forced = false,
         fault_latched = false;
    display_status(cfg, "WAIT", windows[zoom]);
    aux_update(cfg.sample_hz, "WAIT");
    acquisition_start(cfg);

    for (;;) {
        if (acquisition_failed() && !fault_latched) {
            running = false; fault_latched = true; acquisition_stop();
            display_status(cfg, "ADC ERROR", windows[zoom]);
            aux_update(cfg.sample_hz, "ADC ERROR");
        }

        if (acquisition_ready()) {
            forced = acquisition_copy(frame);
            have_frame = true;
            acquisition_stop();
            /* Compute FFT whenever a new frame arrives. */
            scope_fft(frame, fft_mag);
            if (cfg.mode == MODE_SINGLE) running = false;
            const char *status = running ? (forced ? "AUTO" : "TRIG") : "HOLD";
            show(view, cfg, status, windows[zoom], have_frame);
            aux_update(cfg.sample_hz, status);
            if (running) acquisition_start(cfg);
        }

        uint8_t key = keys_poll();
        if (!key) continue;
        acquisition_stop();

        /* Key bit 1: Run / Hold */
        if (key & 1u)  { running = !running; fault_latched = false; }
        bool invalidate = false;
        /* Key bit 2: sample rate */
        if (key & 2u)  { rate = (uint8_t)((rate + 1u) % 4u);
                         cfg.sample_hz = rates[rate]; invalidate = true; }
        /* Key bit 4: trigger mode */
        if (key & 4u)  cfg.mode = (ScopeMode)((cfg.mode + 1u) % 3u);
        /* Key bit 8: trigger edge */
        if (key & 8u)  cfg.falling = !cfg.falling;
        /* Key bit 16: trigger level up */
        if (key & 16u) { if (cfg.threshold < 3951u) cfg.threshold += 128u; }
        /* Key bit 32: trigger level down */
        if (key & 32u) { if (cfg.threshold > 144u)  cfg.threshold -= 128u; }
        /* Key bit 64: toggle view (waveform ↔ FFT spectrum) */
        if (key & 64u) { view = (view == VIEW_SCOPE) ? VIEW_FFT : VIEW_SCOPE; }

        /* A previous frame must never be relabelled with a different rate. */
        if (invalidate) { have_frame = false; display_init(); }

        const char *status = running ? "WAIT" : "HOLD";
        show(view, cfg, status, windows[zoom], have_frame);
        aux_update(cfg.sample_hz, status);
        if (running) acquisition_start(cfg);
    }
}

