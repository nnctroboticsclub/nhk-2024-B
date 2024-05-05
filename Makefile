all:

-include env.home.mk
-include syoch-robotics/makefile.d/all.mk

lc:
	wc -ml \
		$$(find ./stm32-main/src -type f -a \( -name *.c -o -name *.cpp -o -name *.h -o -name *.hpp \)) \
		$$(find ./stm32-enc/src -type f -a \( -name *.c -o -name *.cpp -o -name *.h -o -name *.hpp \)) \
		$$(find ./esp32/main -type f -a \( -name *.c -o -name *.cpp -o -name *.h -o -name *.hpp \)) \
		$$(find ./syoch-robotics -type f -a \( -name *.c -o -name *.cpp -o -name *.h -o -name *.hpp \))

S1_LOG_TAG      := SerialProxy (UART: 0)
S1_SKIP_COMPILE ?= 0
S1_SERIAL       ?= 066AFF495057717867162927
$(eval $(call STM32_DefineRules,s1,$(ESP32_IP),$(S1_LOG_TAG),$(PWD)/robot1-main,$(S1_SKIP_COMPILE),NUCLEO_F446RE,/mnt/st1,$(S1_SERIAL)))

S2_LOG_TAG      := SerialProxy (UART: 0)
S2_SKIP_COMPILE ?= 0
S2_SERIAL       ?= 066BFF333535554157134434
$(eval $(call STM32_DefineRules,s2,$(ESP32_IP),$(S2_LOG_TAG),$(PWD)/robot-bridge,$(S2_SKIP_COMPILE),NUCLEO_F446RE,/mnt/st2,$(S2_SERIAL)))

S3_LOG_TAG      := SerialProxy (UART: 0)
S3_SKIP_COMPILE ?= 0
S3_SERIAL       ?= 066BFF333535554157134434
$(eval $(call STM32_DefineRules,s3,$(ESP32_IP),$(S3_LOG_TAG),$(PWD)/robot-collect,$(S3_SKIP_COMPILE),NUCLEO_F446RE,/mnt/st2,$(S3_SERIAL)))

E_SKIP_COMPILE ?= 0
$(eval $(call ESP32_DefineRules,e,$(PWD)/esp32,$(E_SKIP_COMPILE)))

ds1:
	git switch robot-main
	PATH=$(PATH):/opt/gcc-arm-none-eabi-10.3-2021.10/bin $(MAKE) -C robot1-main -j 12

ds2:
	git switch robot-bridge
	PATH=$(PATH):/opt/gcc-arm-none-eabi-10.3-2021.10/bin $(MAKE) -C robot-bridge -j 12

ds3:
	git switch robot-collect
	PATH=$(PATH):/opt/gcc-arm-none-eabi-10.3-2021.10/bin $(MAKE) -C robot-collect -j 12
