// PokeWebGochi: the browser front end of the host layer (host/host.h), like mac/main.mm is
// the Mac one. The page (shell.html) owns the canvas, the input and the sound; this file
// starts the game once the saved files are back from IndexedDB and exposes what the page calls.
#include <emscripten.h>
#include <stdint.h>
#include <stdlib.h>
#include "host.h"

#define RES_DIR "/res"    // sprites.tpkz and fonts/, packed into the .data file
#define SAVE_DIR "/save"  // IndexedDB-backed: survives reloads and closing the browser

static uint8_t *gFrame = nullptr;
static bool gStarted = false;

extern "C" {

// the page has mounted /save and read it back from IndexedDB: the game can start
EMSCRIPTEN_KEEPALIVE void webStart(void) {
  if (gStarted) return;
  gStarted = true;
  gFrame = (uint8_t *)malloc(HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4);
  hostInit(RES_DIR, SAVE_DIR);
}

// one animation frame: a game pass, and the new picture if there is one (else 0)
EMSCRIPTEN_KEEPALIVE uint8_t *webFrame(void) {
  if (!gStarted) return nullptr;
  hostTick();
  if (hostRestartRequested()) {  // erased the save: start over in a fresh page
    hostFlushSave();
    EM_ASM({ Module.saveAndReload(); });
    return nullptr;
  }
  if (!hostFrameDirty()) return nullptr;
  hostFrameRGBA(gFrame);
  return gFrame;
}

EMSCRIPTEN_KEEPALIVE void webPointer(int x, int y, int down) { hostPointer(x, y, down != 0); }
EMSCRIPTEN_KEEPALIVE void webHover(void) { hostPointerHover(); }
EMSCRIPTEN_KEEPALIVE void webTilt(int left, int right) { hostTiltKeys(left != 0, right != 0); }
EMSCRIPTEN_KEEPALIVE void webButton(int down) { hostButton(down != 0); }
EMSCRIPTEN_KEEPALIVE void webSave(void) {
  if (gStarted) hostFlushSave();
}
EMSCRIPTEN_KEEPALIVE int webSoundOn(void) { return hostSoundEnabled(); }
EMSCRIPTEN_KEEPALIVE void webSetSound(int on) { hostSetSoundEnabled(on != 0); }
EMSCRIPTEN_KEEPALIVE void webAudio(int16_t *out, int frames) { hostAudioRender(out, frames); }
EMSCRIPTEN_KEEPALIVE int webAudioRate(void) { return HOST_AUDIO_RATE; }
EMSCRIPTEN_KEEPALIVE void webTiltSensor(float x) { hostTiltSensor(x); }

// ---- the buddy relay: the page owns the WebSocket (Module.relaySend) ----
EM_JS(void, jsRelaySend, (const uint8_t *data, int len), { Module.relaySend(HEAPU8.slice(data, data + len)); });
static void relaySend(const uint8_t *data, int len) { jsRelaySend(data, len); }
EMSCRIPTEN_KEEPALIVE void webRelayUp(int up) { hostRelayConnected(up ? relaySend : nullptr); }
EMSCRIPTEN_KEEPALIVE void webRelayPeers(int n) { hostRelayPeers(n); }
EMSCRIPTEN_KEEPALIVE void webRelayIn(const uint8_t *data, int len) { hostRelayReceived(data, len); }
EMSCRIPTEN_KEEPALIVE const char *webRelayUrl(void) { return HOST_RELAY_URL; }
EMSCRIPTEN_KEEPALIVE const char *webRelayV4Url(void) { return HOST_RELAY_V4_URL; }

}  // extern "C"

int main() { return 0; }  // the page calls webStart() when the save is ready
