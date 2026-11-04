# The game's host layer: a platform-independent library that a front end
# (mac/, and a WebAssembly one in the future) links together with its window. See README.md.
#   make        -> build/host/libpokehost.a
#   make test   -> builds and runs the layer's tests
CXX      ?= c++
ROOT     := ..
BUILD    := $(ROOT)/build/host
LIB      := $(BUILD)/libpokehost.a

# It is a Knomi with different hardware underneath: the game, the menus and the button are the Knomi's
DEFS     := -DBOARD_HOST -DBOARD_KNOMI_V2
INCLUDES := -I. -I shim -I $(ROOT)
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wno-unused-parameter
ALLFLAGS := $(CXXFLAGS) $(DEFS) $(INCLUDES)
# The oldest macOS the Mac app runs on (mac/Info.plist says the same); without it, clang targets the Mac it builds on
export MACOSX_DEPLOYMENT_TARGET ?= 12.0

LAYER_SRC := host_canvas.cpp host_prefs.cpp host_runtime.cpp host_board.cpp host_buddy.cpp host_audio.cpp
GAME_SRC  := $(ROOT)/pet.cpp $(ROOT)/i18n.cpp $(ROOT)/sdmon.cpp $(ROOT)/ota_update.cpp
UNIT      := $(BUILD)/sketch_unit.cpp
OBJS      := $(patsubst %.cpp,$(BUILD)/%.o,$(LAYER_SRC) $(notdir $(GAME_SRC))) $(BUILD)/sketch_unit.o
HEADERS   := $(wildcard *.h shim/*.h shim/driver/*.h *.inc $(ROOT)/*.h)

.PHONY: all test clean
all: $(LIB)

$(LIB): $(OBJS)
	@rm -f $@
	ar rcs $@ $^

$(BUILD)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(ALLFLAGS) -c -o $@ $<

$(BUILD)/%.o: $(ROOT)/%.cpp $(HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(ALLFLAGS) -c -o $@ $<

$(UNIT): $(ROOT)/PokeKnomiGochi.ino gen_sketch_unit.py
	@mkdir -p $(BUILD)
	python3 -I gen_sketch_unit.py $< $@

$(BUILD)/sketch_unit.o: $(UNIT) $(HEADERS)
	$(CXX) $(ALLFLAGS) -c -o $@ $<

test: $(LIB)
	$(CXX) $(ALLFLAGS) -I $(ROOT)/test -o $(BUILD)/host_tests test_host.cpp $(LIB)
	$(BUILD)/host_tests

clean:
	rm -rf $(BUILD)
