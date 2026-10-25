ARM_PREFIX ?= arm-none-eabi-
CC = $(ARM_PREFIX)gcc
HOST_CC ?= gcc
PYTHON ?= python3
INCLUDES = -Ifirmware/include
CFLAGS = -mcpu=cortex-m3 -mthumb -std=c11 -Os -g3 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns -ffunction-sections -fdata-sections $(INCLUDES)
SOURCES = $(wildcard firmware/src/*.c) firmware/startup/startup.c
OBJECTS = $(patsubst %.c,build/%.o,$(SOURCES))
.PHONY: all test syntax preview clean
all: build/dso.bin build/dso.hex
build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
build/dso.elf: $(OBJECTS) firmware/startup/lpc1768.ld
	$(CC) -mcpu=cortex-m3 -mthumb -nostdlib -Tfirmware/startup/lpc1768.ld -Wl,--gc-sections,-Map=build/dso.map $(OBJECTS) -lgcc -o $@
	$(ARM_PREFIX)size $@
build/dso.bin: build/dso.elf tools/checksum.py
	$(ARM_PREFIX)objcopy -O binary $< $@
	$(PYTHON) tools/checksum.py $@
build/dso.hex: build/dso.bin
	$(ARM_PREFIX)objcopy -I binary -O ihex $< $@
test:
	@mkdir -p build
	$(HOST_CC) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $(INCLUDES) firmware/src/scope.c firmware/src/fft.c tests/test_scope.c -lm -o build/test_scope
	./build/test_scope
	$(HOST_CC) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $(INCLUDES) firmware/src/fft.c firmware/src/scope.c tests/test_fft.c -lm -o build/test_fft
	./build/test_fft
	$(PYTHON) tests/test_checksum.py
syntax:
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -fsyntax-only $(INCLUDES) $(SOURCES)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -fsyntax-only -DENABLE_LCD=1 -DENABLE_TM1637=1 -DUSE_EXTERNAL_12MHZ=1 $(INCLUDES) $(SOURCES)
preview:
	@mkdir -p build
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror $(INCLUDES) firmware/src/display.c firmware/src/scope.c firmware/src/fft.c tests/render_display.c -lm -o build/render_display
	./build/render_display
clean:
	rm -rf build
-include $(OBJECTS:.o=.d)
