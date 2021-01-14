TARGET=$(shell $(CXX) -dumpmachine)

CXXFLAGS+=-std=c++14
INC+=-I../inc -I../deps/xmath.h/inc
LIB+=-L../lib/$(TARGET)/
LINK+=-lg -lpthread
SRCS=$(wildcard *.cpp)

../lib/$(TARGET)/libg.a:
	make -C .. static

bin:
	mkdir -p $@

bin/%: %.cpp bin ../lib/$(TARGET)/libg.a
	$(CXX) $(CXXFLAGS) $(INC) $(LIB) $< -o $@ $(LINK)

.PHONEY: all

all:
	@echo "Built all examples"