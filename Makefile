CXX = clang++ -std=c++2a -stdlib=libc++ \
			-fimplicit-modules -fimplicit-module-maps \
			-fmodules-cache-path=build \
			-fprebuilt-module-path=build

.PHONY: default opt debug all clean
.PRECIOUS: build/build.o

default: debug

opt: CXXFLAGS += -Ofast -ffunction-sections -fdata-sections -flto
opt: LDFLAGS += -Wl,--gc-sections -s

debug: CXXFLAGS += -Og -g3

opt: all
debug: all

clean:
	rm -rf bin build

MKBMI = ${CXX} -Xclang -emit-module-interface

bin:
	mkdir -p bin

build:
	mkdir -p build

build/%.pcm: src/%.cc | build
	${MKBMI} -c $< -o $@

build/%.o: src/%.cc | build
	${CXX} -c $< -o $@

bin/%: build/%.o | bin
	${CXX} $^ -o $@

-include build/depends.mk

build/depends.mk: bin/build | build
	$^ > $@
