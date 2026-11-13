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

// each sound effect, for the page to buzz (Module.haptic decides how, and whether)
EM_JS(void, jsHaptic, (int sfx), { if (Module.haptic) Module.haptic(sfx); });
static void haptic(uint8_t sfx) { jsHaptic(sfx); }
EM_JS(void, jsOpenUrl, (const char *u), { window.open(UTF8ToString(u), '_blank', 'noopener'); });
static void openUrl(const char *u) { jsOpenUrl(u); }  // About's links: a new tab

// network time (host.h): the page asks its own site, past the cache, and hands back the answer's Date
// header in seconds (0: none). The page and the game come from the same place, so the header is readable.
EM_JS(void, jsNetTime, (), {
  fetch(location.origin + location.pathname, { method: 'HEAD', cache: 'no-store' })
    .then(r => { const t = Date.parse(r.headers.get('Date') || ''); Module._webNetTime(isNaN(t) ? 0 : Math.floor(t / 1000)); })
    .catch(() => Module._webNetTime(0));
});

extern "C" {

// the page has mounted /save and read it back from IndexedDB: the game can start
EMSCRIPTEN_KEEPALIVE void webStart(void) {
  if (gStarted) return;
  gStarted = true;
  gFrame = (uint8_t *)malloc(HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4);
  hostSetHaptic(haptic);
  hostSetOpenUrl(openUrl);
  hostInit(RES_DIR, SAVE_DIR);
}

// one animation frame: a game pass, and the new picture if there is one (else 0)
EMSCRIPTEN_KEEPALIVE uint8_t *webFrame(void) {
  if (!gStarted) return nullptr;
  hostTick();
  if (hostNetTimeWanted()) jsNetTime();
  if (hostRestartRequested()) {  // erased the save: start over in a fresh page
    hostFlushSave();
    EM_ASM({ Module.saveAndReload(); });
    return nullptr;
  }
  if (!hostFrameDirty()) return nullptr;
  hostFrameRGBA(gFrame);
  return gFrame;
}

EMSCRIPTEN_KEEPALIVE void webNetTime(uint32_t utc) { hostNetTime(utc); }
EMSCRIPTEN_KEEPALIVE void webPointer(int x, int y, int down) { hostPointer(x, y, down != 0); }
EMSCRIPTEN_KEEPALIVE void webHover(void) { hostPointerHover(); }
EMSCRIPTEN_KEEPALIVE void webHoverAt(int x, int y) { hostPointerHoverAt(x, y); }  // a mouse over it, no button (-1: gone)
EMSCRIPTEN_KEEPALIVE void webTilt(int left, int right) { hostTiltKeys(left != 0, right != 0); }
EMSCRIPTEN_KEEPALIVE void webArrows(int up, int down) { hostArrowKeys(up != 0, down != 0); }
EMSCRIPTEN_KEEPALIVE void webButton(int down) { hostButton(down != 0); }
EMSCRIPTEN_KEEPALIVE void webSave(void) {
  if (gStarted) hostFlushSave();
}
EMSCRIPTEN_KEEPALIVE int webSoundOn(void) { return hostSoundEnabled(); }
EMSCRIPTEN_KEEPALIVE void webSetSound(int on) { hostSetSoundEnabled(on != 0); }
// V2's music: 0 off, 1..3 its level, or -1 for a build without it (V1): the page hides its button
EMSCRIPTEN_KEEPALIVE int webMusicLevel(void) { return UI_V2 ? (hostMusicEnabled() ? hostMusicVolume() : 0) : -1; }
EMSCRIPTEN_KEEPALIVE void webSetMusicLevel(int level) {
  if (level <= 0) hostSetMusicEnabled(false);
  else hostSetMusicVolume(level);
}
EMSCRIPTEN_KEEPALIVE void webMusicHidden(int hidden) { hostMusicHidden(hidden != 0); }
EMSCRIPTEN_KEEPALIVE int webAutoUpdate(void) { return hostAutoUpdate(); }
EMSCRIPTEN_KEEPALIVE void webSetAutoUpdate(int on) { hostSetAutoUpdate(on != 0); }
EMSCRIPTEN_KEEPALIVE int webQuietForUpdate(void) { return hostQuietForUpdate(); }
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
