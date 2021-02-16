TARGET=$(shell $(CXX) -dumpmachine)

CXXFLAGS+=-std=c++11 -g
CXXFLAGS+=-D_XOPEN_SOURCE=500 -D_GNU_SOURCE -DGL_GLEXT_PROTOTYPES -DGL_SILENCE_DEPRECATION
INC+=-I./inc -Ideps/xmath.h/inc -Ideps/libpng -Ideps/sha1 -Ideps/opengametools/src
LIB+=-Ldeps/libpng -Ldeps/sha1/lib/$(TARGET)
LINK+=-lpng
SRCS=$(wildcard src/*.cpp)
OBJS=$(patsubst src/%.cpp,out/$(TARGET)/%.cpp.o,$(wildcard src/*.cpp))

lib/$(TARGET):
	mkdir -p $@

out/$(TARGET):
	mkdir -p $@

ex_libs/$(TARGET):
	gitman install --force
	mkdir -p $@

ex_libs/$(TARGET)/png: ex_libs/$(TARGET)
	mkdir -p $@
	cp deps/libpng/libpng.a $@

ex_libs/$(TARGET)/zlib: ex_libs/$(TARGET)
	mkdir -p $@
	cp deps/zlib/libz.a $@
	cp deps/zlib/libz.so* $@

out/$(TARGET)/%.cpp.o: src/%.cpp out/$(TARGET) lib/$(TARGET)
	$(CXX) $(CXXFLAGS) $(INC) $(LIB) -c $< -o $@

lib/$(TARGET)/libg.a: $(OBJS)
	$(AR) rcs $@ $^

.PHONEY: clean static deps what
clean:
	rm -rf lib out

deps: ex_libs/$(TARGET)/png ex_libs/$(TARGET)/zlib
	@echo "Built deps for $(TARGET)"

what:
	@echo $(OBJS)

static: lib/$(TARGET)/libg.a
	@echo "Built libg.a"
