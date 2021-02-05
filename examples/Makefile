$(eval OS := $(shell uname))
TARGET=$(shell $(CXX) -dumpmachine)

CXXFLAGS+=-std=c++17 -g $(pkg-config --cflags glfw3)
INC+=-I../inc -I../deps/xmath.h/inc #-I../deps/libpng
LIB+=-L../lib/$(TARGET)/ #-L../deps/libpng
LINK+=-lg

ifeq ($(OS),Darwin)
	LINK += -lm -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
else
	LINK +=$(shell pkg-config --libs glfw3) -lGL -lX11 -lXi -lXrandr -lXxf86vm -lXinerama -lXcursor -lrt -lm -ldl
	CXXFLAGS += -D_XOPEN_SOURCE=500 -D_GNU_SOURCE -DGL_GLEXT_PROTOTYPES
endif

LINK+=-lpthread -lpng
SRCS=$(wildcard *.cpp)

../lib/$(TARGET)/libg.a:
	make -C .. static

bin:
	mkdir -p $@

bin/%: %.cpp bin ../lib/$(TARGET)/libg.a
	$(CXX) $(CXXFLAGS) $(INC) $(LIB) $< -o $@ $(LINK)

bin/%.html: %.cpp bin ../lib/$(TARGET)/libg.a
	$(CXX) $(CXXFLAGS) $(INC) $(LIB) $< -o $@ $(LINK)

.PHONEY: all clean

all:
	@echo "Built all examples"

clean:
	rm -rf bin
