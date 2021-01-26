TARGET=$(shell $(CXX) -dumpmachine)

CXXFLAGS+=-std=c++11 -g $(pkg-config --cflags glfw3)
INC+=-I../inc -I../deps/xmath.h/inc
LIB+=-L../lib/$(TARGET)/
LINK+=-lg -lpthread -lcrypto
LINK+=$(shell pkg-config --libs glfw3) -lGL -lX11 -lXi -lXrandr -lXxf86vm -lXinerama -lXcursor -ldl
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
