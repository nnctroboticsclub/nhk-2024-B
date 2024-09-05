TARGET?=NUCLEO_F446RE
BUILD?=BUILD/${TARGET}

.PHONY: all
all: build

$(BUILD)/build.ninja: CMakeLists.txt src/CMakeLists.txt
	[ -d $(BUILD) ] || mkdir -p $(BUILD)
	cd $(BUILD); cmake -DCMAKE_BUILD_TYPE=Develop -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DMBED_TARGET=$(TARGET) -B . -S ../../ -G Ninja

.PHONY: write
write: build
	st-flash --connect-under-reset --format ihex --serial 066AFF495057717867162927 write $(BUILD)/src/app/app.hex

.PHONY: clean
clean:
	rm -rf $(BUILD)

.PHONY: rebuild
rebuild: clean build

.PHONY: build
build: $(BUILD)/build.ninja
	rm BUILD/NUCLEO_F446RE/libnhk2024b_rs.a ../common_libs/nhk2023-b/target/thumbv7em-none-eabi/debug/libnhk2024b_rs.a || :
	cd ../common_libs/nhk2023-b; cargo build
	cd $(BUILD); ninja -j 12