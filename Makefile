TARGET=$(shell $(CXX) -dumpmachine)

CXXFLAGS+=-std=c++11 -g
CXXFLAGS+=-D_XOPEN_SOURCE=500 -D_GNU_SOURCE -DGL_GLEXT_PROTOTYPES -DGL_SILENCE_DEPRECATION
INC+=-I inc -Ideps/xmath.h/inc
LIB+=
LINK+=
SRCS=$(wildcard src/*.cpp)
OBJS=$(patsubst src/%.cpp,out/$(TARGET)/%.cpp.o,$(wildcard src/*.cpp))

lib/$(TARGET):
	mkdir -p $@

out/$(TARGET):
	mkdir -p $@

out/$(TARGET)/%.cpp.o: src/%.cpp out/$(TARGET) lib/$(TARGET)
	$(CXX) $(CXXFLAGS) $(INC) $(LIB) -c $< -o $@

lib/$(TARGET)/libg.a: $(OBJS)
	$(AR) rcs $@ $^

.PHONEY: clean static what
clean:
	rm -rf lib out

what:
	@echo $(OBJS)

static: lib/$(TARGET)/libg.a
	@echo "Built libg.a"
