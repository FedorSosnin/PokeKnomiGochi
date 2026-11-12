// The save's storage name: its Preferences namespace on a board, <name>.prefs in the save folder on a computer and
// on the web. V2 has a name of its own (Fedor: a standalone game). V1 keeps the one its saves have had from the
// first versions, and V2 carries a save over from there on its first start (saveMigrate(), pet.cpp).
#ifndef SAVENAME_H
#define SAVENAME_H
#include "ui_v2.h"
#define SAVE_NS_OLD "tamapoke"
#if UI_V2
#define SAVE_NS_NEW "pokeknomigochi"  // (15 characters at most: the NVS's limit)
const char *saveNs();  // the name in use: the new one, or the old one while a save couldn't be carried over yet
void saveMigrate();    // first thing at start, before anything reads the save
void saveForgetOld();  // starting over: the save from before the rename goes too, so it can't come back
#define SAVE_NS saveNs()
#else
#define SAVE_NS SAVE_NS_OLD
#endif
#endif
