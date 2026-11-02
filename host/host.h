// Game interface for a front end: the Mac window today, a WebAssembly canvas tomorrow.
// No platform dependencies, with C linkage. Everything is called from a single thread except
// hostSerialPush() and hostAudioRender(), which may come from another.
#pragma once
#include <stdbool.h>
#include <stdint.h>

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
// Tilt keys; the game sees a ramp, not a jump, so it can be fine-tuned
void hostTiltKeys(bool left, bool right);
// A real tilt sensor, -1 (left) .. 1 (right), like the Knomi's accelerometer; NaN goes back to the keys
void hostTiltSensor(float x);
// The Knomi's physical button: tap = wild rival, 1-3 s = screen, 10 s = erase save
void hostButton(bool down);

// A line for the game's serial console (the same commands as on the Knomi)
void hostSerialPush(const char *line);

// The game's sound setting (the SND button on the clock screen), saved with the game
bool hostSoundEnabled(void);
void hostSetSoundEnabled(bool on);

// Buddy relay (relay/ on Cloudflare): the front end keeps a WebSocket to HOST_RELAY_URL, adding
// "?v4=<its public IPv4>" when it can learn it, and passes things through on the game's thread.
#define HOST_RELAY_URL "wss://pokeknomigochi-relay.fedor-dd0.workers.dev/ws"
#define HOST_RELAY_V4_URL "https://api4.ipify.org"  // answers with the IPv4 alone, over IPv4 only
typedef void (*HostRelaySend)(const uint8_t *data, int len);
void hostRelayConnected(HostRelaySend send);  // connected (how to send), or NULL when it drops
void hostRelayPeers(int n);                   // the relay's {"n": n}: how many others share the group
void hostRelayReceived(const uint8_t *data, int len);  // a binary message from the relay

// Audio: 16-bit mono PCM at HOST_AUDIO_RATE. Always fills frames samples (silence if nothing is playing).
#define HOST_AUDIO_RATE 16000
void hostAudioRender(int16_t *out, int frames);

#ifdef __cplusplus
}
#endif
