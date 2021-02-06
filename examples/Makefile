$(eval OS := $(shell uname))
TARGET=$(shell $(CXX) -dumpmachine)

CXXFLAGS+=-std=c++17 -g $(pkg-config --cflags glfw3)
INC+=-I../inc -I../deps/xmath.h/inc -I../deps/libpng -I../deps/zlib
LIB+=-L../lib/$(TARGET)/ -L../deps/libpng -L../deps/zlib
LINK+=-lg

ifeq ($(OS),Darwin)
	LINK += -lm -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
else
	CXXFLAGS += -D_XOPEN_SOURCE=500 -D_GNU_SOURCE -DGL_GLEXT_PROTOTYPES
ifeq ($(TARGET),wasm32-unknown-emscripten)
	LINK +=-s USE_GLFW=3 -lglfw3 -lGL -lrt -lm -ldl
else
	LINK+=$(shell pkg-config --libs glfw3) -lGL -lrt -lm -ldl -lX11 -lXi -lXrandr -lXxf86vm -lXinerama -lXcursor 
endif

endif

LINK+=-lpthread -lpng -lz
SRCS=$(wildcard *.cpp)

../lib/$(TARGET)/libg.a:
	make -C .. static

bin:
	mkdir -p $@

bin/%: %.cpp bin ../lib/$(TARGET)/libg.a
	$(CXX) $(CXXFLAGS) $(INC) $(LIB) $< -o $@ $(LINK)

bin/%.html: %.cpp bin ../lib/$(TARGET)/libg.a
	$(CXX) $(CXXFLAGS) $(INC) $(LIB) $< -o $@ --preload-file data $(LINK)

.PHONEY: all clean

all:
	@echo "Built all examples"

clean:
	rm -rf bin
