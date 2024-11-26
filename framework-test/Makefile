BUILD := Build


$(BUILD)/build.ninja: CMakeLists.txt
	cmake . -G Ninja -B $(BUILD) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug

.PHONY: build
build: $(BUILD)/build.ninja
	ninja -C $(BUILD)

.PHONY: run_t
run_t: build
	stdbuf -o0 Build/framework-test | awk '{print strftime("%T"),": ",$$0;fflush()}'

.PHONY: run
run: build
	Build/framework-test

.PHONY: watch-threads
watch-threads:
	watch -n 1 -t ps H -C framework-test -o \"pid tid cmd comm\"