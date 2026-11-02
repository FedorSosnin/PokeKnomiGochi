// Se incluye al final del sketch (ver gen_sketch_unit.py): la puerta de entrada al juego desde
// el host, con acceso a sus variables globales.
#include "host_internal.h"

void hostGameSetup() { setup(); }
void hostGameLoop() { loop(); }

void hostGameSave() {
  pet.setClock(rtcEpoch());  // guarda, y el tiempo de ausencia de la proxima vez se cuenta desde aqui
}

// como un toque que despierta la pantalla, pero sin tragarse el siguiente toque
void hostGameWake() {
  if (!screenOff && dimStage != DIM_AWAKE) wakeUI();
}
