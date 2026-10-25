# LPC1768 Digital Storage Oscilloscope

A standalone, single-channel teaching instrument with an ILI9341 SPI waveform display.
Prepared for the supplied fifth-semester ECE project materials. The LPC1768 is the acquisition
controller. No Arduino, LabVIEW, MATLAB or PC is required during normal operation.

**Status: source implementation and host-tested capture logic. Not yet ARM-cross-compiled,
flashed, electrically tested or calibrated. There is no prebuilt firmware binary in this package.**
The exact LPC1768 development-board model and TFT module are still unknown. Port names below
refer to MCU signals, not board connector numbers.

## Start here

1. Open `demo/index.html` in a browser. It works offline and generates **simulated** signals.
2. Open `docs/BUILD_GUIDE.html` for the parts, wiring, circuit, build steps and lab checklist.
3. Confirm your board exposes the assigned pins and has no conflicting onboard peripherals.
4. Build and flash the firmware using the instructions below. Start with a low-voltage DC test.

## Implemented scope

| Item | Implementation |
|---|---|
| MCU | LPC1768, nominal 96 MHz from its internal RC oscillator by default |
| Optional clock | 100 MHz from a verified external 12 MHz crystal |
| Input | One channel, P0.23 / AD0.0, nominal 0–3.3 V at the ADC pin |
| ADC | 12-bit, 12 MHz ADC clock by default |
| Sample rates | 1,000 / 5,000 / 10,000 / 20,000 samples/s; 5,000 default |
| Acquisition | Timer0 MAT0.1 hardware start, ADC-complete interrupt |
| Capture | 1,000 samples, including 250 before the trigger |
| Trigger | Rising/falling, adjustable threshold, 16-code hysteresis |
| Modes | AUTO / NORMAL / SINGLE, plus Run/Hold |
| Display | ILI9341, 320×240 landscape, 100 / 200 / 500 / 1,000 visible samples |
| Measurements | ADC-pin Vpp and threshold-crossing frequency estimate |
| Additional UI | Optional HD44780 LCD and TM1637 decimal-point display drivers |

The 20 kS/s option is a configuration target, not a measured capability. Firmware stops and
shows `ADC ERROR` if it detects ADC overrun or a missed Timer0 scheduling deadline.

Captures stop while the screen and optional displays update. This is a frame-capture instrument
with dead time, not continuous gap-free recording. SINGLE captures one triggered frame and holds.
Press RUN to re-arm. NORMAL retains the previous trace while waiting for a new edge. AUTO forces
a trigger after about 250 ms of waiting, then acquires the post-trigger section.

## Build with GNU Arm Embedded

Install `arm-none-eabi-gcc`, its binutils, GNU Make and Python 3 on your development computer.
No downloaded MCU SDK or runtime library is needed for this Makefile build.

```sh
make
```

Outputs: `build/dso.elf`, `build/dso.bin`, `build/dso.hex`, and `build/dso.map`.
The build patches and verifies the LPC17xx boot checksum in the BIN before producing the HEX.
**Use the BIN or HEX for standalone flashing. The ELF contains an unpatched boot-checksum word.**
Debuggers that download the ELF must insert the boot checksum themselves for standalone reset.

Flash at address `0x00000000` using your board's supported SWD/JTAG probe, onboard loader, or
LPC ISP tool. Use the correct LPC1768 flash algorithm. Flashing overwrites the existing program.
Do not use instructions intended for an Arduino Nano or a different LPC family.

For a Keil workflow, use the installed LPC1768 device pack's startup and linker settings, add the
files in `firmware/src`, and include `firmware/include`. Do **not** add this package's GNU startup
or GNU linker script in that workflow. See the build guide for the remaining settings.

## Source layout

| Path | Purpose |
|---|---|
| `firmware/src/scope.c` | Platform-independent capture and measurement algorithms |
| `firmware/src/acquisition.c` | ADC, Timer0 and interrupt coordination |
| `firmware/src/board.c` | Clock, GPIO, buttons and SPI transport |
| `firmware/src/display.c` | ILI9341 initialization and waveform drawing |
| `firmware/src/aux.c` | Optional character LCD and TM1637 drivers |
| `firmware/src/main.c` | Controls and application state |
| `firmware/include/config.h` | Clock selection, reference voltage and optional displays |
| `firmware/startup` | GNU vector table, reset code and linker script |
| `tools/checksum.py` | LPC17xx flash boot checksum |
| `tests` | Host capture tests, checksum tests and display adapter |
| `demo/index.html` | Offline software demonstration, not a hardware viewer |
| `docs` | Build guide, rendered firmware preview and verification record |

## Reproduce software checks

With GCC, Make and Python 3 available:

```sh
make test syntax preview
```

`make test` uses AddressSanitizer and UndefinedBehaviorSanitizer. In a sandbox where LeakSanitizer
cannot inspect processes, use `ASAN_OPTIONS=detect_leaks=0 make test`. The firmware does not
allocate heap memory. `make syntax` is a **host C syntax check**, not an ARM build.
`make preview` runs the actual TFT renderer against a host pixel buffer and writes a PPM image.

## Constraints

- Input signals must be limited to the ADC/reference range. This design is for low-voltage,
  common-ground lab signals. It has no isolation or mains measurement rating.
- The browser demo and preview PNG contain synthetic data, not experiment results.
- The default internal oscillator limits timing accuracy. A 100 MHz processor claim alone
  does not establish sampling accuracy, jitter or calibrated bandwidth.
- A 5 kS/s capture gives only five samples per cycle at 1 kHz. Start demonstrations at 100 Hz.
- The input filter is a prototype filter, not a guarantee against all aliasing.
- USB streaming, external triggering, FFT, dual channels and nonvolatile waveform storage are
  not implemented. Optional displays have not been hardware-tested.

See the source references and slide corrections in `docs/BUILD_GUIDE.html`.
