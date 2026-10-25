# Verification record

Prepared 5 October 2026.

## Completed in the development workspace

- Compiled and executed the platform-independent C acquisition tests with host GCC,
  AddressSanitizer and UndefinedBehaviorSanitizer. Leak detection was disabled because this
  execution environment cannot provide LeakSanitizer's process inspection.
- Tested rising and falling capture, ring wrap, exact 250-sample pre-trigger alignment,
  frozen completed frames, AUTO timeout, NORMAL waiting, SINGLE behavior, rejection of
  crossings before the pre-trigger buffer is full, and trigger hysteresis.
- Checked Vpp and frequency against a generated 100 Hz sinusoid and checked DC and clipping.
- Exercised 100 randomized captures for buffer and coordinate bounds.
- Tested LPC17xx checksum generation, idempotence, preservation of bytes after the vector
  table, and rejection of images shorter than 32 bytes.
- Host GCC C syntax checks passed with `-Wall -Wextra -Werror` for the default configuration
  and with both optional displays plus the external-clock configuration enabled.
- Ran the actual C display driver against a host pixel adapter, checked pixel/window bounds
  and visually inspected `firmware_preview.png`. Its waveform is synthetic.
- Exercised browser JavaScript startup and measurement/capture functions in Node with a
  minimal DOM/canvas adapter: sine amplitude/frequency, AUTO/NORMAL/SINGLE, falling and clipping.

## Not completed

- ARM target compilation/linking: an Arm compiler is not available in the execution workspace.
- Browser rendering in a real browser: no browser executable is available here.
- Flashing or booting an LPC1768 board.
- Board-specific pin/connector verification.
- TFT panel electrical compatibility or successful initialization on a purchased module.
- ADC/timer interrupt operation, clock measurement, display update rate or power consumption.
- Analogue front-end construction, calibration, noise, frequency response or alias rejection.
- Optional HD44780/TM1637 hardware behavior.

Software checks do not verify those electrical and target-specific items. Use the acceptance
table in the build guide to record actual measurements. No lab results are asserted.
