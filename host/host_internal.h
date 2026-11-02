#pragma once
// Cableado interno del host; un front end solo necesita host.h.
#include <stdint.h>

// host_prefs.cpp
void hostPrefsSetDir(const char *dir);
void hostPrefsFlush();

// sketch_unit.cpp (generado por gen_sketch_unit.py): el juego, visto desde fuera
void hostGameSetup();
void hostGameLoop();
void hostGameSave();
void hostGameWake();
