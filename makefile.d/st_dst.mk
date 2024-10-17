dst1:
	FLASH_ARGS="--serial 066AFF495057717867162927" ninja -C Build/NUCLEO_F446RE upload_connection-test

dst2:
	FLASH_ARGS="--serial 066EFF303435554157113125" ninja -C Build/NUCLEO_F446RE upload_connection-test

dst:
	clear
	rm Build/NUCLEO_F446RE/connection-test/CMakeFiles/connection-test.dir/main.cpp.obj || true
	$(MAKE)
	$(MAKE) -j 2 dst1 dst2
