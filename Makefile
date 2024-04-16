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
S1_SERIAL       ?= 0668FF485151717867193716
$(eval $(call STM32_DefineRules,s1,$(ESP32_IP),$(S1_LOG_TAG),$(PWD)/stm32-main,$(S1_SKIP_COMPILE),NUCLEO_F446RE,/mnt/st1,$(S1_SERIAL)))

