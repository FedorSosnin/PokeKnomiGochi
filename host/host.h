// Game interface for a front end: the Mac window today, a WebAssembly canvas tomorrow.
// No platform dependencies, with C linkage. Everything is called from a single thread except
// hostSerialPush() and hostAudioRender(), which may come from another.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../ui_v2.h"  // (relative: the Mac app builds main.mm with only -I host)

#ifdef __cplusplus
extern "C" {
#endif

#define HOST_CANVAS_SIZE 466  // logical game canvas, square

// Starts the game. resources holds mons/ and fonts/; saveDir is created if it does not exist.
void hostInit(const char *resources, const char *saveDir);
// One pass of the game loop; call it at 50 Hz or more. Never blocks.
void hostTick(void);
// Saves anything pending right now: on exit or before the system suspends.
void hostFlushSave(void);
// The game has erased the save and wants to restart: save and relaunch the process.
bool hostRestartRequested(void);

// true if there is a new frame since the last call to hostFrameRGBA()
bool hostFrameDirty(void);
// Copies the canvas to out (HOST_CANVAS_SIZE^2 8-bit RGBA pixels, in byte order R G B A),
// with the panel brightness applied.
void hostFrameRGBA(uint8_t *out);

// Pointer in canvas coordinates; simulates a finger. Touches shorter than the game's polling
// interval are stretched so that a trackpad tap is not lost.
void hostPointer(int x, int y, bool down);
// The mouse hovers over the window: wakes the dimmed screen, as approaching the device would.
void hostPointerHover(void);
// ...and where over the screen, in canvas coordinates, with no button down (x < 0: it left the screen): V2 outlines
// the control under it, as a finger's press does
void hostPointerHoverAt(int x, int y);
// Tilt keys; the game sees a ramp, not a jump, so it can be fine-tuned
void hostTiltKeys(bool left, bool right);
// Up and down arrows: with left and right (hostTiltKeys), PokeDungeon walks with all four
void hostArrowKeys(bool up, bool down);
// A real tilt sensor, -1 (left) .. 1 (right), like the Knomi's accelerometer; NaN goes back to the keys
void hostTiltSensor(float x);
// The Knomi's physical button: tap = wild rival, 1-3 s = screen, 10 s = erase save
void hostButton(bool down);

// A line for the game's serial console (the same commands as on the Knomi)
void hostSerialPush(const char *line);

// The game's sound setting (the SND button on the clock screen), saved with the game
bool hostSoundEnabled(void);
void hostSetSoundEnabled(bool on);

// Haptics: the front end gets each sound effect's id (audio.h's Sfx), even with the sound off,
// and may buzz for it (the browser does on Android phones). NULL: none.
typedef void (*HostHaptic)(uint8_t sfx);
void hostSetHaptic(HostHaptic fn);

// Links (V2's About page: the repository, the YouTube channel): the front end opens the URL in the browser. NULL:
// none, and the game shows the link's QR code instead, as the Knomi does
typedef void (*HostOpenUrl)(const char *url);
void hostSetOpenUrl(HostOpenUrl fn);

// Buddy relay (relay/ on Cloudflare): the front end keeps a WebSocket to HOST_RELAY_URL, adding
// "?v4=<its public IPv4>" when it can learn it, and passes things through on the game's thread.
#define HOST_RELAY_URL "wss://pokeknomigochi-relay.fedor-dd0.workers.dev/ws"
#define HOST_RELAY_V4_URL "https://api4.ipify.org"  // answers with the IPv4 alone, over IPv4 only
typedef void (*HostRelaySend)(const uint8_t *data, int len);
void hostRelayConnected(HostRelaySend send);  // connected (how to send), or NULL when it drops
void hostRelayPeers(int n);                   // the relay's {"n": n}: how many others share the group
void hostRelayReceived(const uint8_t *data, int len);  // a binary message from the relay

// Network time: the game's clock follows a time server, not the computer's clock (anyone can change
// that), so changing it can't fake time away. When hostNetTimeWanted() says so (poll it every tick;
// true once per request), fetch HOST_TIME_URL (a HEAD request, past any cache) and pass its Date header
// to hostNetTime() as seconds since 1970 UTC, or 0 if it failed. The browser asks its own site instead.
// Until an answer comes, the game counts no time away from before it started.
#define HOST_TIME_URL "https://pokewebgochi.3dprintsos.com/"
bool hostNetTimeWanted(void);
void hostNetTime(uint32_t utc);

// Auto-update (on unless turned off; kept with the game's save, like the Knomi's switch): with it on, a
// new version the app finds goes in by itself, without asking, once hostQuietForUpdate() says nobody is
// using the game (the home screen showing, nothing being decided or played with a buddy, no touch or key
// for a minute). With it off, the app asks as before. Checking by hand always asks.
bool hostAutoUpdate(void);
void hostSetAutoUpdate(bool on);
bool hostQuietForUpdate(void);

// Audio: 16-bit mono PCM at HOST_AUDIO_RATE. Always fills frames samples (silence if nothing is playing). V2 plays
// at 48 kHz, so the high square waves keep their upper harmonics clean (the tap); the Waveshare stays at 16 kHz.
#if UI_V2
#define HOST_AUDIO_RATE 48000
#else
#define HOST_AUDIO_RATE 16000
#endif
void hostAudioRender(int16_t *out, int frames);

#ifdef __cplusplus
}
#endif
