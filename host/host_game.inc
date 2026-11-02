// Included at the end of the sketch (see gen_sketch_unit.py): the host's entry point into the game,
// with access to its global variables.
#include "host_internal.h"

void hostGameSetup() { setup(); }
void hostGameLoop() { loop(); }

void hostGameSave() {
  pet.setClock(rtcEpoch());  // saves, and next time's away time is counted from here
}

// like a touch that wakes the screen, but without swallowing the next touch
void hostGameWake() {
  if (!screenOff && dimStage != DIM_AWAKE) wakeUI();
}
