BUILD := Build


$(BUILD)/build.ninja: CMakeLists.txt
	cmake . -G Ninja -B $(BUILD) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

.PHONY: build
build: $(BUILD)/build.ninja
	ninja -C $(BUILD)