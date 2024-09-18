DIST_TIME := .dist/$(shell date +%Y%m%d-%H%M%S)

dist:
	mkdir -p $(DIST_TIME)
	cp robot1-main/BUILD/NUCLEO_F446RE/src/app.bin $(DIST_TIME)/r1.bin
	cp robot-bridge/BUILD/NUCLEO_F446RE/src/app/app.bin $(DIST_TIME)/r2.bin
	cp robot-collect/BUILD/NUCLEO_F446RE/src/app/app.bin $(DIST_TIME)/r3.bin
	arm-none-eabi-objcopy -O binary usb/build/app $(DIST_TIME)/controller.bin