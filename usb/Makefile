.PHONY: write
write:

.PHONY: build
build:
	arm-none-eabi-gcc\
		-DSTM32F446xx \
		-mcpu=cortex-m4 \
		-Wl,-T,STM32F446RETx_FLASH.ld \
		-IApp/Inc \
		-ISTM32F4xx_HAL_Config/Inc \
		-IMiddlewares/ST/STM32_USB_Host_Library/Class/HID/Inc \
		-IMiddlewares/ST/STM32_USB_Host_Library/Core/Inc \
		-IDrivers/CMSIS/Include \
		-IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
		-IDrivers/STM32F4xx_HAL_Driver/Inc \
		-IUSB_HOST/Target \
		-IUSB_HOST/App \
		`find -name '*.c'`

.PHONY: clean
clean:
	rm -rf build