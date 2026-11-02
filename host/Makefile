# La capa de host del juego: una biblioteca sin dependencias de plataforma que un front end
# (mac/, en el futuro uno de WebAssembly) enlaza junto con su ventana. Ver README.md.
#   make        -> build/host/libpokehost.a
#   make test   -> compila y ejecuta las pruebas de la capa
CXX      ?= c++
ROOT     := ..
BUILD    := $(ROOT)/build/host
LIB      := $(BUILD)/libpokehost.a

# Es un Knomi con otro hardware debajo: el juego, los menus y el boton son los del Knomi
DEFS     := -DBOARD_HOST -DBOARD_KNOMI_V2
INCLUDES := -I. -I shim -I $(ROOT)
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wno-unused-parameter
ALLFLAGS := $(CXXFLAGS) $(DEFS) $(INCLUDES)

LAYER_SRC := host_canvas.cpp host_prefs.cpp host_runtime.cpp host_board.cpp host_audio.cpp
GAME_SRC  := $(ROOT)/pet.cpp $(ROOT)/i18n.cpp $(ROOT)/sdmon.cpp
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

$(UNIT): $(ROOT)/TamaPoke.ino gen_sketch_unit.py
	@mkdir -p $(BUILD)
	python3 -I gen_sketch_unit.py $< $@

$(BUILD)/sketch_unit.o: $(UNIT) $(HEADERS)
	$(CXX) $(ALLFLAGS) -c -o $@ $<

test: $(LIB)
	$(CXX) $(ALLFLAGS) -I $(ROOT)/test -o $(BUILD)/host_tests test_host.cpp $(LIB)
	$(BUILD)/host_tests

clean:
	rm -rf $(BUILD)
