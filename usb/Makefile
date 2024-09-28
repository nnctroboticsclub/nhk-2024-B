.PHONY: default
default: build/app.hex

build/build.ninja:
	cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON --graphviz=graph.dot

.PHONY: build/app.hex
build/app.hex: build/build.ninja
	ninja -C build -j 12 app.hex

.PHONY: write
write: write1

.PHONY: write1
write1: build/app.hex
	st-flash \
		--connect-under-reset \
		--serial 0672FF575251717867212233 \
		--format ihex \
		write $<

.PHONY: write2
write2: build/app.hex
	st-flash \
		--connect-under-reset \
		--format ihex \
		--serial 066FFF555054877567044942 \
		write $<


.PHONY: clean
clean:
	rm -rf build