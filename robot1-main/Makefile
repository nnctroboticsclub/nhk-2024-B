TARGET?=NUCLEO_F446RE
BUILD?=BUILD/${TARGET}
GRAPHVIZ?=Graphviz/${TARGET}

.PHONY: all
all: build

$(BUILD)/build.ninja: CMakeLists.txt src/CMakeLists.txt
	[ -d $(BUILD) ] || mkdir -p $(BUILD)
	cd $(BUILD); cmake \
		-DCMAKE_BUILD_TYPE=Develop \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DMBED_TARGET=$(TARGET) \
		-B . -S ../../ -G Ninja

.PHONY: cmake
cmake: $(BUILD)/build.ninja
	cd $(BUILD); cmake \
		-DCMAKE_BUILD_TYPE=Develop \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DMBED_TARGET=$(TARGET) \
		-B . -S ../../ -G Ninja

$(GRAPHVIZ)/source/dependency.dot: CMakeLists.txt src/CMakeLists.txt CMakeGraphVizOptions.cmake
	[ -d $(GRAPHVIZ) ] || mkdir -p $(GRAPHVIZ)
	cmake \
		-DCMAKE_BUILD_TYPE=Develop \
		-DMBED_TARGET=$(TARGET) \
		--graphviz=$(GRAPHVIZ)/source/dependency.dot \
		-B $(BUILD) -S $(CURDIR)

.PHONY: write
write: build
	sudo -E st-flash --connect-under-reset --format ihex write $(BUILD)/src/robot1.hex

.PHONY: clean
clean:
	rm -rf $(BUILD)

.PHONY: rebuild
rebuild: clean build

$(GRAPHVIZ)/svg/%.svg: $(GRAPHVIZ)/source/%
	[ -d $(GRAPHVIZ)/svg ] || mkdir -p $(GRAPHVIZ)/svg
	dot -Tsvg -o $@ $<

$(GRAPHVIZ)/png/%.png: $(GRAPHVIZ)/source/%
	[ -d $(GRAPHVIZ)/png ] || mkdir -p $(GRAPHVIZ)/png
	dot -Tpng -o $@ $<

$(GRAPHVIZ)/dest/%.pdf: $(GRAPHVIZ)/svg/%.svg
	@[ -d $(GRAPHVIZ)/dest ] || mkdir -p $(GRAPHVIZ)/dest
	rsvg-convert -f pdf -o $(GRAPHVIZ)/dest/$*.pdf $<

.PHONY: graph
graph: $(GRAPHVIZ)/source/dependency.dot
	$(MAKE) -j 12 $(filter-out %.dependers.png, $(filter-out $(GRAPHVIZ)/png/dependency.dot.mbed%, $(patsubst $(GRAPHVIZ)/source/%, $(GRAPHVIZ)/png/%.png, $(wildcard $(GRAPHVIZ)/source/*))))


.PHONY: build
build: $(BUILD)/build.ninja
	cd $(BUILD); ninja -j 13