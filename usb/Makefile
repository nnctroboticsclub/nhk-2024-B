CXXFLAGS := \
		-ICore/Inc \
		-ISTM32F4xx_HAL_Config/Inc \
		-IMiddlewares/ST/STM32_USB_Host_Library/Class/HID/Inc \
		-IMiddlewares/ST/STM32_USB_Host_Library/Core/Inc \
		-IMiddlewares/ST/Drivers/STM32F4xx_HAL_Driver/Inc \
		-IDrivers/CMSIS/Include \
		-IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
		-IDrivers/STM32F4xx_HAL_Driver/Inc \
		-IUSB_HOST/Target \
		-IUSB_HOST/App \
		-DDEBUG \
		-DUSE_HAL_DRIVER \
		-DSTM32F446xx \
		-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb \
		-O0 -g3 \
		-ffunction-sections -fdata-sections -fstack-usage

LDFLAGS := \
	-Wl,-T,STM32F446RETX_FLASH.ld \
	-Wl,--gc-sections \
	-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
	--specs=nosys.specs

SOURCES_C := $(shell find -name '*.c' -a -not -path build/\*)
SOURCES_ASM := $(shell find -name '*.s' -a -not -path build/\*)

conv_src2obj = $(patsubst %.c,build/%.o,$(1))
conv_src2dep = $(patsubst %.c,build/%.d,$(1))

flags_dep = -MMD -MP -MF $(call conv_src2dep,$(1)) -MT $(call conv_src2obj,$(1))

.PHONY: default
default: build

build/%.o: %.c
	@echo "$@"
	@mkdir -p $(dir $@)
	@arm-none-eabi-gcc $(CXXFLAGS) $(call flags_dep, $<) -c $< -o $@

build/%.o: %.s
	@echo "$@"
	@mkdir -p $(dir $@)
	@arm-none-eabi-gcc $(CXXFLAGS) -c $< -o $@

build/out.elf: $(patsubst %.c,build/%.o,$(SOURCES_C)) $(patsubst %.s,build/%.o,$(SOURCES_ASM))
	@echo "$@"
	@mkdir -p build
	@arm-none-eabi-gcc $(LDFLAGS) $^ -o $@

build/out.hex: build/out.elf
	@echo "$@"
	@arm-none-eabi-objcopy -O ihex $< $@

-include $(foreach src,$(SOURCES_C),$(call conv_src2dep,$(src)))

.PHONY: build
build: build/out.hex

.PHONY: write
write: build
	st-flash \
		--connect-under-reset \
		--format ihex \
		write build/out.hex


.PHONY: clean
clean:
	rm -rf build