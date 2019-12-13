CXX = clang++ -std=c++2a -stdlib=libc++ \
			-I${HOME}/joe/src/clang/build/include \
			-fimplicit-modules -fimplicit-module-maps \
			-fmodules-cache-path=build \
			-fprebuilt-module-path=build \
			-Wall -Wextra -pedantic

.PHONY: default opt debug all clean
.PRECIOUS: build/build.o

default: all

CXXFLAGS = -g3
LDFLAGS = -L${HOME}/joe/src/clang/build/lib
opt: CXXFLAGS = -Ofast -ffunction-sections -fdata-sections -flto -DNDEBUG
opt: LDFLAGS += -Wl,--gc-sections -s

opt: all
debug: all

clean:
	rm -rf bin build

MKBMI = ${CXX} -Xclang -emit-module-interface

bin:
	mkdir -p bin

build:
	mkdir -p build

build/%.pcm: | build
	${MKBMI} ${CXXFLAGS} -c $< -o $@

build/%.o: | build
	${CXX} ${CXXFLAGS} -c $< -o $@

bin/%: build/%.o | bin
	${CXX} ${CXXFLAGS} $^ ${LDFLAGS} -o $@

build/build.o: src/build.cc

-include build/depends.mk

build/depends.mk: bin/build $(shell find src -name '*.cc') | build
	$< > $@
