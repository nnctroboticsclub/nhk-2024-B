TARGET?=NUCLEO_F446RE
BUILD?=BUILD/${TARGET}

.PHONY: all
all: build

$(BUILD)/build.ninja: CMakeLists.txt src/CMakeLists.txt
	[ -d $(BUILD) ] || mkdir -p $(BUILD)
	cd $(BUILD); cmake -DCMAKE_BUILD_TYPE=Develop -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DMBED_TARGET=$(TARGET) -B . -S ../../ -G Ninja

.PHONY: write
write: build
	sudo st-flash --connect-under-reset --format ihex write $(BUILD)/src/app/app.hex

.PHONY: clean
clean:
	rm -rf $(BUILD)

.PHONY: rebuild
rebuild: clean build

.PHONY: build
build: $(BUILD)/build.ninja
	cd $(BUILD); ninja -j 12