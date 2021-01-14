TARGET=$(shell $(CXX) -dumpmachine)

CXXFLAGS+=-std=c++11
INC+=-I inc -Ideps/xmath.h/inc
LIB+=
LINK+=
SRC=$(wildcard src/*.cpp)
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

# static: lib/$(TARGET)/libg.a:
# 	@echo "Built libg.a"
