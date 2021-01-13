TARGET=$(shell $(CXX) -dumpmachine)

CXXFLAGS+=
INC+=-I inc -Ideps/xmath.h/inc
LIB+=
LINK+=

lib/$(TARGET):
	mkdir -p $@

out/$(TARGET):
	mkdir -p $@

out/$(TARGET)/%.o: src/%.cpp out/$(TARGET)
	$(CXX) $(CXXFLAGS) $(INC) $(LIB) -c $< -o $@

.PHONEY: clean
clean:
	rm -rf lib out