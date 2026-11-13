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
# UI_V2=0 or 1 (from the command line or the environment) picks the look; unset, the sketch's default
ifdef UI_V2
DEFS     += -DUI_V2=$(UI_V2)
endif
INCLUDES := -I. -I shim -I $(ROOT) -I $(ROOT)/src
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wno-unused-parameter
ALLFLAGS := $(CXXFLAGS) $(DEFS) $(INCLUDES)
# The oldest macOS the Mac app runs on (mac/Info.plist says the same); without it, clang targets the Mac it builds on
export MACOSX_DEPLOYMENT_TARGET ?= 12.0

LAYER_SRC := host_canvas.cpp host_prefs.cpp host_runtime.cpp host_board.cpp host_buddy.cpp host_audio.cpp host_crt.cpp
GAME_SRC  := $(ROOT)/src/pet.cpp $(ROOT)/src/i18n.cpp $(ROOT)/src/sdmon.cpp $(ROOT)/src/ota_update.cpp
UNIT      := $(BUILD)/sketch_unit.cpp
OBJS      := $(patsubst %.cpp,$(BUILD)/%.o,$(LAYER_SRC) $(notdir $(GAME_SRC))) $(BUILD)/sketch_unit.o
HEADERS   := $(wildcard *.h shim/*.h shim/driver/*.h *.inc $(ROOT)/src/*.h)

.PHONY: all test clean music
all: $(LIB)

$(LIB): $(OBJS)
	@rm -f $@
	ar rcs $@ $^

$(BUILD)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(ALLFLAGS) -c -o $@ $<

$(BUILD)/%.o: $(ROOT)/src/%.cpp $(HEADERS)
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

# every song into build/music/*.wav, to listen to (V2's music, music_synth.h): make music [ONLY=name]
music: $(BUILD)/music_wav
	@mkdir -p $(ROOT)/build/music
	$(BUILD)/music_wav $(ROOT)/build/music $(ONLY)

$(BUILD)/music_wav: music_wav.cpp $(ROOT)/src/music.h $(ROOT)/src/music_synth.h $(ROOT)/src/music_data.h
	@mkdir -p $(BUILD)
	$(CXX) $(ALLFLAGS) -o $@ music_wav.cpp

clean:
	rm -rf $(BUILD)
