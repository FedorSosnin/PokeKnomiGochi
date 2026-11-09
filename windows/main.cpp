// PokeWinGochi: the game's round window on a Windows desktop, what mac/main.mm is on a Mac.
// Only the window; the game and everything else live in host/ (see host/host.h).
#define _WIN32_WINNT 0x0A00  // Windows 10 and up
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <shlobj.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <xinput.h>
#include <math.h>
#include <stdio.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <initguid.h>  // the sensor ids, defined here
#include <sensors.h>
#include <sensorsapi.h>
#include "host.h"
#include "resources.h"
#include "shim/Preferences.h"  // the save folder's storage, as the game uses it
#include "version.h"
#include "update_key.h"  // checks the updates' signature

#define APP_NAME "PokeWinGochi"
#define APP_NAME_W L"PokeWinGochi"
// About box
#define AUTHOR_EMAIL "fedor@3dprintsos.com"
#define GITHUB_URL L"https://github.com/FedorSosnin/PokeKnomiGochi"
#define YOUTUBE_URL L"https://youtube.com/3dprintsos"
#define TICK_MS 16  // 60 a second: the game wants 50 or more
// The Pokeball frame around the screen, in points (1/96 inch, scaled with the display). Only the frame
// drags the window: the screen is all touch. With the mouse over the window it grows outward to
// RING_HOVER, easier to grab; the window keeps a clear margin of RING_PAD for that, which clicks pass through.
#define RING 6.0
#define RING_HOVER 12.0
#define RING_PAD (RING_HOVER - RING)
#define RING_ANIM_MS 150.0  // the grow and shrink
#define RING_OUTLINE 1.0
#define RING_LINE 20.0  // the black band in the middle
static const int kSizes[] = {190, 240, 300};  // side of the window at rest, frame included, in points
#define DEFAULT_SIZE 240

// New versions: the newest "win-v*" release on GitHub. Checked at launch and once a day; the player is
// told once per version and can always check from the menu. The save lives outside the app, so
// a new .exe keeps the pet.
// Updating in place, as the Knomi does: the release also carries the .exe's signature
// (tools/sign_update.sh). The app downloads both itself, checks the signature against
// UPDATE_PUBLIC_KEY and writes the new .exe beside itself; then it saves the game, renames itself out
// of the way (Windows lets a running program be renamed, not replaced), puts the new one in its place
// and starts it. Written by the app and not by a browser, the new .exe has no "from the internet" mark:
// no SmartScreen warning after the first install. When it can't (an older release, a folder it can't
// write to), the menu offers the download as before.
#define GITHUB_API_HOST L"api.github.com"
#define RELEASES_PATH L"/repos/FedorSosnin/PokeKnomiGochi/releases?per_page=30"
#define RELEASE_TAG "win-v"
#define RELEASE_ASSET "PokeWinGochi.exe"
#define RELEASE_SIG RELEASE_ASSET ".sig"
#define UPDATE_EVERY_MS (24 * 60 * 60 * 1000)

// The window's size and place live in the save folder (namespace "win"), next to the game they belong
// to: a copy launched on another save (tests, POKEMACGOCHI_SAVE_DIR) can't move this one
#define PREFS_NS "win"

enum {
  WM_TRAY = WM_APP + 1,  // the notification-area icon
  WM_RELAY_MSG,          // a message from the relay thread (wParam: 1 text, 0 binary; lParam: std::string*)
  WM_RELAY_DOWN,         // the relay connection dropped
  WM_UPDATE_FOUND,       // the update check is back (wParam: asked; lParam: UpdateResult*)
  WM_UPDATE_READY,       // the new .exe is checked and beside this one (lParam: std::wstring*, its path, or empty: failed)
  WM_TEST,               // --console: a test event (lParam: std::string*)
  WM_NET_TIME,           // the time server's answer (lParam: seconds since 1970 UTC, 0: none)
};
enum { TIMER_TICK = 1, TIMER_PING, TIMER_UPDATE };
enum {
  ID_ABOUT = 100, ID_SHOW, ID_SIZE0, ID_SIZE1, ID_SIZE2, ID_MUTE, ID_UPDATE, ID_QUIT, ID_VIBE, ID_TILT, ID_AUTO_UPDATE,
};

static HWND gWnd;
static HINSTANCE gInst;
static bool gConsole = false;  // --console: launched for testing
static bool gOwnSave = true;   // false with POKEMACGOCHI_SAVE_DIR: another save than the user's
static HANDLE gLock = INVALID_HANDLE_VALUE;
static std::string gNewVersion;  // newer than this app, once a check found one
static std::wstring gNewURL;     // its .exe (or the release page)
static std::wstring gNewExe, gNewSig;  // the new .exe and its signature: to update in place
static bool gUpdating = false;
// Update Automatically (the game's setting: on unless turned off): the version found waits here until
// nobody is using the game (hostQuietForUpdate), then goes in without asking. A failed one says nothing
// and is tried again with the next day's check.
static bool gAutoPending = false;
static bool gAutoInstall = false;  // the update running now went in by itself
static bool canUpdateInPlace() { return !gNewExe.empty() && !gNewSig.empty(); }

// ---- text ----
static std::wstring wide(const std::string &s) {
  std::wstring w(MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0), 0);
  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], (int)w.size());
  w.pop_back();
  return w;
}
static std::string utf8(const std::wstring &w) {
  std::string s(WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL), 0);
  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], (int)s.size(), NULL, NULL);
  s.pop_back();
  return s;
}
// "1.10" > "1.9"
static int compareVersions(const std::string &a, const std::string &b) {
  const char *p = a.c_str(), *q = b.c_str();
  while (*p || *q) {
    long x = strtol(p, (char **)&p, 10), y = strtol(q, (char **)&q, 10);
    if (x != y) return x < y ? -1 : 1;
    if (*p == '.') p++;
    if (*q == '.') q++;
    if ((*p && (*p < '0' || *p > '9')) || (*q && (*q < '0' || *q > '9'))) break;
  }
  return 0;
}

// ---- files: a known folder as a UTF-8 path with forward slashes, the way host/ joins them ----
// and back to backslashes, for the Windows calls that insist on them (SHCreateDirectoryEx)
static std::wstring winPath(const std::string &path) {
  std::wstring w = wide(path);
  for (wchar_t &c : w)
    if (c == L'/') c = L'\\';
  return w;
}

static std::string knownFolder(REFKNOWNFOLDERID id) {
  PWSTR p = NULL;
  std::string out;
  if (SHGetKnownFolderPath(id, KF_FLAG_CREATE, NULL, &p) == S_OK) out = utf8(p);
  CoTaskMemFree(p);
  for (char &c : out)
    if (c == '\\') c = '/';
  return out;
}

// The game's files travel inside the .exe; the game reads them from a folder, so they are written
// out next to the save's folder, in %LOCALAPPDATA%: again only when they differ (a new version)
static bool writeResource(int id, const std::string &path) {
  HRSRC r = FindResourceW(gInst, MAKEINTRESOURCEW(id), (LPCWSTR)RT_RCDATA);
  if (!r) return false;
  DWORD n = SizeofResource(gInst, r);
  const void *data = LockResource(LoadResource(gInst, r));
  if (!data) return false;
  std::wstring wpath = wide(path);
  HANDLE f = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (f != INVALID_HANDLE_VALUE) {
    std::vector<char> have(n + 1);
    DWORD got = 0;
    bool same = ReadFile(f, have.data(), n + 1, &got, NULL) && got == n && memcmp(have.data(), data, n) == 0;
    CloseHandle(f);
    if (same) return true;
  }
  f = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
  if (f == INVALID_HANDLE_VALUE) return false;
  DWORD put = 0;
  bool ok = WriteFile(f, data, n, &put, NULL) && put == n;
  CloseHandle(f);
  return ok;
}

static std::string unpackResources() {
  std::string dir = knownFolder(FOLDERID_LocalAppData) + "/" APP_NAME "/res";
  SHCreateDirectoryExW(NULL, winPath(dir + "/fonts").c_str(), NULL);
  bool ok = writeResource(IDR_SPRITES, dir + "/sprites.tpkz") &&
            writeResource(IDR_FONT_JA, dir + "/fonts/unifont_t_japanese3.u8g2") &&
            writeResource(IDR_FONT_KO, dir + "/fonts/unifont_t_korean2.u8g2");
  if (!ok) fprintf(stderr, "cannot write the game's files to %s\n", dir.c_str());
  return dir;
}

// ---- HTTPS (WinHTTP): one GET, the body as a string ----
static HINTERNET httpSession() {
  static HINTERNET s = WinHttpOpen(APP_NAME_W L"/" L"" APP_VERSION, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                   WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  return s;
}

static bool httpGet(const wchar_t *host, const wchar_t *path, const wchar_t *headers, std::string &body) {
  body.clear();
  HINTERNET c = WinHttpConnect(httpSession(), host, INTERNET_DEFAULT_HTTPS_PORT, 0);
  HINTERNET r = c ? WinHttpOpenRequest(c, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                       WINHTTP_FLAG_SECURE)
                  : NULL;
  bool ok = false;
  if (r) {
    WinHttpSetTimeouts(r, 8000, 8000, 15000, 15000);
    DWORD status = 0, len = sizeof(status);
    ok = WinHttpSendRequest(r, headers ? headers : WINHTTP_NO_ADDITIONAL_HEADERS, headers ? (DWORD)-1L : 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
         WinHttpReceiveResponse(r, NULL) &&
         WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                             &status, &len, WINHTTP_NO_HEADER_INDEX) &&
         status == 200;
    for (DWORD avail = 0; ok && WinHttpQueryDataAvailable(r, &avail) && avail;) {
      size_t at = body.size();
      body.resize(at + avail);
      DWORD got = 0;
      if (!WinHttpReadData(r, &body[at], avail, &got)) ok = false;
      body.resize(at + got);
    }
    WinHttpCloseHandle(r);
  }
  if (c) WinHttpCloseHandle(c);
  return ok;
}

// one GET of a full URL (GitHub's download links redirect, which WinHTTP follows); http too, for tests
static bool httpGetUrl(const std::wstring &url, std::string &body) {
  body.clear();
  URL_COMPONENTS u = {sizeof(u)};
  wchar_t host[256] = L"", path[2048] = L"";
  u.lpszHostName = host;
  u.dwHostNameLength = 256;
  u.lpszUrlPath = path;
  u.dwUrlPathLength = 2048;
  wchar_t extra[1024] = L"";
  u.lpszExtraInfo = extra;
  u.dwExtraInfoLength = 1024;
  if (!WinHttpCrackUrl(url.c_str(), 0, 0, &u)) return false;
  std::wstring fullPath = std::wstring(path) + extra;
  HINTERNET c = WinHttpConnect(httpSession(), host, u.nPort, 0);
  HINTERNET r = c ? WinHttpOpenRequest(c, L"GET", fullPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                       u.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
                  : NULL;
  bool ok = false;
  if (r) {
    WinHttpSetTimeouts(r, 8000, 8000, 30000, 30000);
    DWORD status = 0, len = sizeof(status);
    ok = WinHttpSendRequest(r, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
         WinHttpReceiveResponse(r, NULL) &&
         WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                             &status, &len, WINHTTP_NO_HEADER_INDEX) &&
         status == 200;
    for (DWORD avail = 0; ok && WinHttpQueryDataAvailable(r, &avail) && avail;) {
      size_t at = body.size();
      body.resize(at + avail);
      DWORD got = 0;
      if (!WinHttpReadData(r, &body[at], avail, &got)) ok = false;
      body.resize(at + got);
    }
    WinHttpCloseHandle(r);
  }
  if (c) WinHttpCloseHandle(c);
  return ok;
}

// ---- the buddy relay (relay/): Knomis, Macs and PCs meet on the Wi-Fi by themselves; the relay adds
// the browsers, which can't. One WebSocket on a thread of its own, retried with a growing pause when it
// drops; what arrives is handed to the game's thread. ----
static std::mutex gRelayLock;
static HINTERNET gRelay = NULL;

static void relaySend(const uint8_t *data, int len) {  // the game's thread
  std::lock_guard<std::mutex> l(gRelayLock);
  if (gRelay) WinHttpWebSocketSend(gRelay, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, (PVOID)data, len);
}

static void relayPing() {  // keeps a quiet connection open; the relay answers "pong" without waking up
  std::lock_guard<std::mutex> l(gRelayLock);
  if (gRelay) WinHttpWebSocketSend(gRelay, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, (PVOID) "ping", 4);
}

static HINTERNET relayOpen() {
  // the home's shared IPv4: a PC connecting over IPv6 would otherwise not meet the Knomi on its Wi-Fi
  std::string v4;
  httpGet(L"api4.ipify.org", L"/", NULL, v4);
  while (!v4.empty() && (v4.back() == '\n' || v4.back() == '\r' || v4.back() == ' ')) v4.pop_back();
  int dots = 0;
  bool ok = !v4.empty() && v4.size() <= 15;
  for (char ch : v4) {
    dots += ch == '.';
    ok = ok && (ch == '.' || (ch >= '0' && ch <= '9'));
  }
  // HOST_RELAY_URL is "wss://<host>/<path>"
  std::string url = HOST_RELAY_URL;
  size_t hostAt = url.find("://") + 3, pathAt = url.find('/', hostAt);
  std::wstring host = wide(url.substr(hostAt, pathAt - hostAt));
  std::wstring path = wide(url.substr(pathAt) + (ok && dots == 3 ? "?v4=" + v4 : ""));
  HINTERNET c = WinHttpConnect(httpSession(), host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
  HINTERNET r = c ? WinHttpOpenRequest(c, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                       WINHTTP_FLAG_SECURE)
                  : NULL;
  HINTERNET ws = NULL;
  if (r && WinHttpSetOption(r, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0) &&
      WinHttpSendRequest(r, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0) && WinHttpReceiveResponse(r, NULL))
    ws = WinHttpWebSocketCompleteUpgrade(r, 0);
  if (r) WinHttpCloseHandle(r);
  if (c && !ws) WinHttpCloseHandle(c);  // with a WebSocket the connection stays with it
  return ws;
}

static void relayThread() {
  int tries = 0;
  for (;;) {
    HINTERNET ws = relayOpen();
    if (ws) {
      { std::lock_guard<std::mutex> l(gRelayLock); gRelay = ws; }
      std::string msg;
      uint8_t buf[2048];
      for (;;) {
        DWORD got = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE type;
        if (WinHttpWebSocketReceive(ws, buf, sizeof(buf), &got, &type) != NO_ERROR) break;
        if (type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) break;
        msg.append((const char *)buf, got);
        if (type == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE || type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE)
          continue;
        bool text = type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;
        if (text) tries = 0;  // {"n": others}: it also says we are in
        PostMessageW(gWnd, WM_RELAY_MSG, text, (LPARAM) new std::string(msg));
        msg.clear();
      }
      { std::lock_guard<std::mutex> l(gRelayLock); gRelay = NULL; }
      WinHttpCloseHandle(ws);
      PostMessageW(gWnd, WM_RELAY_DOWN, 0, 0);
    }
    int wait = 2 << (tries < 5 ? tries : 5);
    tries++;
    Sleep((wait < 60 ? wait : 60) * 1000);
  }
}

static void relayMessage(bool text, const std::string &m) {  // the game's thread
  if (!text) {
    hostRelayReceived((const uint8_t *)m.data(), (int)m.size());
    return;
  }
  hostRelayConnected(relaySend);
  size_t at = m.find("\"n\"");
  if (at != std::string::npos && (at = m.find(':', at)) != std::string::npos) hostRelayPeers(atoi(m.c_str() + at + 1));
}

// ---- sound: the game's 16 kHz mono, pulled on a thread of its own ----
#define AUDIO_BUFFERS 3
#if UI_V2
#define AUDIO_FRAMES 1536  // ~32 ms per buffer at 48 kHz
#else
#define AUDIO_FRAMES 512  // ~32 ms per buffer at 16 kHz
#endif

static void audioThread() {
  HANDLE ev = CreateEventW(NULL, FALSE, FALSE, NULL);
  WAVEFORMATEX fmt = {};
  fmt.wFormatTag = WAVE_FORMAT_PCM;
  fmt.nChannels = 1;
  fmt.nSamplesPerSec = HOST_AUDIO_RATE;
  fmt.wBitsPerSample = 16;
  fmt.nBlockAlign = 2;
  fmt.nAvgBytesPerSec = HOST_AUDIO_RATE * 2;
  HWAVEOUT out;
  if (waveOutOpen(&out, WAVE_MAPPER, &fmt, (DWORD_PTR)ev, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
    fprintf(stderr, "no audio output: the game continues without sound\n");
    return;
  }
  static int16_t pcm[AUDIO_BUFFERS][AUDIO_FRAMES];
  WAVEHDR hdr[AUDIO_BUFFERS] = {};
  for (int i = 0; i < AUDIO_BUFFERS; i++) {
    hdr[i].lpData = (LPSTR)pcm[i];
    hdr[i].dwBufferLength = sizeof(pcm[i]);
    waveOutPrepareHeader(out, &hdr[i], sizeof(WAVEHDR));
    hostAudioRender(pcm[i], AUDIO_FRAMES);
    waveOutWrite(out, &hdr[i], sizeof(WAVEHDR));
  }
  for (;;) {
    WaitForSingleObject(ev, INFINITE);
    for (int i = 0; i < AUDIO_BUFFERS; i++)
      if (hdr[i].dwFlags & WHDR_DONE) {
        hostAudioRender(pcm[i], AUDIO_FRAMES);
        waveOutWrite(out, &hdr[i], sizeof(WAVEHDR));
      }
  }
}

// ---- vibration: a buzz with each sound effect on a game controller, through XInput: the built-in one
// of a handheld PC (ROG Ally and the like) or an Xbox controller. Its own switch in the menu, on by
// default and kept with the window's settings; muting the sound doesn't stop it. ----
typedef DWORD(WINAPI *XInputGetStateFn)(DWORD, XINPUT_STATE *);
typedef DWORD(WINAPI *XInputSetStateFn)(DWORD, XINPUT_VIBRATION *);
static XInputGetStateFn gXGet = NULL;
static XInputSetStateFn gXSet = NULL;
static std::atomic<bool> gVibeOn{true};
static std::mutex gVibeLock;
static std::condition_variable gVibeWake;
static int gVibeNext = -1;  // the effect to play, from the game's thread

// per effect, in audio.h's Sfx order: milliseconds on, off, on... and how hard. The browser's patterns
// (webgame/shell.html), a little longer: a rumble motor needs a moment to spin up
struct Buzz { uint8_t count; uint16_t ms[5]; uint16_t strength; };
static const Buzz kBuzz[] = {
  {1, {30}, 0x5000},                       // tap
  {1, {45}, 0x9000},                       // eat
  {1, {30}, 0x6000},                       // minigame point
  {3, {30, 50, 30}, 0x8000},               // likes it
  {3, {60, 60, 100}, 0xC000},              // hatch
  {5, {80, 60, 80, 60, 150}, 0xC000},      // evolution
  {3, {50, 50, 80}, 0xB000},               // medal
  {3, {60, 40, 60}, 0x9000},               // not allowed
  {1, {120}, 0xA000},                      // goodbye
  {3, {40, 50, 60}, 0xB000},               // level up
  {1, {50}, 0x9000}, {1, {50}, 0x9000}, {1, {50}, 0xA000},  // the ball rocks: one, two, three
};

static int controllerIndex() {  // the first one connected, or -1
  XINPUT_STATE st;
  for (DWORD i = 0; gXGet && i < XUSER_MAX_COUNT; i++)
    if (gXGet(i, &st) == ERROR_SUCCESS) return (int)i;
  return -1;
}

static void haptic(uint8_t sfx) {  // the game's thread: hands the effect over, never waits
  if (!gVibeOn || !gXSet || sfx >= sizeof(kBuzz) / sizeof(kBuzz[0])) return;
  std::lock_guard<std::mutex> l(gVibeLock);
  gVibeNext = sfx;
  gVibeWake.notify_one();
}

static void vibeThread() {
  std::unique_lock<std::mutex> l(gVibeLock);
  for (;;) {
    gVibeWake.wait(l, [] { return gVibeNext >= 0; });
    const Buzz &b = kBuzz[gVibeNext];
    gVibeNext = -1;
    l.unlock();
    int pad = controllerIndex();
    l.lock();
    if (pad < 0) continue;
    for (int i = 0; i < b.count && gVibeNext < 0; i++) {  // a new effect cuts this one short
      XINPUT_VIBRATION v = {};
      if (i % 2 == 0) v.wLeftMotorSpeed = v.wRightMotorSpeed = b.strength;
      gXSet(pad, &v);
      gVibeWake.wait_for(l, std::chrono::milliseconds(b.ms[i]), [] { return gVibeNext >= 0; });
    }
    XINPUT_VIBRATION off = {};
    gXSet(pad, &off);
  }
}

static void startVibration() {
  HMODULE x = LoadLibraryW(L"xinput1_4.dll");  // Windows 10 and 11; older ones have 9_1_0
  if (!x) x = LoadLibraryW(L"xinput9_1_0.dll");
  if (!x) return;
  gXGet = (XInputGetStateFn)(void *)GetProcAddress(x, "XInputGetState");
  gXSet = (XInputSetStateFn)(void *)GetProcAddress(x, "XInputSetState");
  if (!gXGet || !gXSet) return;
  Preferences p;
  p.begin(PREFS_NS, true);
  gVibeOn = p.getBool("vibe", true);
  p.end();
  hostSetHaptic(haptic);
  std::thread(vibeThread).detach();
}

// ---- tilt, like the Knomi's accelerometer and a phone's: a motion sensor that Windows knows about (a
// tablet's or a handheld's) tilts the game as the device tilts, and a controller's left stick tilts it
// smoothly (with the gyro mapped to that stick, in Armoury Crate on a ROG Ally, tilting moves the
// stick). The stick and the D-pad also walk in PokeDungeon. Without either: the arrow keys, as before.
// Both only while this window is the active one, so playing something else doesn't move the pet. ----
#define TILT_FULL_G 0.5  // sin(30 degrees): tilting 30 degrees is all the way, as on a phone
#define PAD_SCAN_MS 2000 // with no controller, the slots are asked again this often (an empty one is slow)
static ISensor *gAccel = NULL;
static std::atomic<bool> gTiltOn{true};  // "Tilt with the device" in the menu, shown with a sensor
static int gPad = -1;
static DWORD gPadScanT = 0;
static bool gPadL = false, gPadR = false, gPadU = false, gPadD = false;
static bool gLeft = false, gRight = false, gUp = false, gDown = false;  // the keyboard's arrows

static void startTiltSensor() {
  ISensorManager *mgr = NULL;
  if (FAILED(CoCreateInstance(CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER, IID_ISensorManager, (void **)&mgr))) return;
  ISensorCollection *list = NULL;
  ULONG n = 0;
  if (SUCCEEDED(mgr->GetSensorsByType(SENSOR_TYPE_ACCELEROMETER_3D, &list)) && SUCCEEDED(list->GetCount(&n)) && n)
    list->GetAt(0, &gAccel);
  if (list) list->Release();
  mgr->Release();
  Preferences p;
  p.begin(PREFS_NS, true);
  gTiltOn = p.getBool("tilt", true);
  p.end();
}

// across the screen, in g: positive with the right side down, held flat or upright
static bool accelX(double &x) {
  ISensorDataReport *r = NULL;
  if (!gAccel || FAILED(gAccel->GetData(&r)) || !r) return false;
  PROPVARIANT v;
  PropVariantInit(&v);
  bool ok = SUCCEEDED(r->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_X_G, &v)) && v.vt == VT_R8;
  if (ok) x = v.dblVal;
  PropVariantClear(&v);
  r->Release();
  return ok;
}

static float stickAxis(SHORT v, SHORT dead) {  // -1..1 outside the dead zone, 0 inside
  int a = v < 0 ? -(int)v : v;
  if (a <= dead) return 0;
  float t = (float)(a - dead) / (32767 - dead);
  return v < 0 ? -fminf(t, 1) : fminf(t, 1);
}

static void applyKeys();
static void readTilt() {  // each tick, on the game's thread
  bool active = GetForegroundWindow() == gWnd;
  float sx = 0, sy = 0;
  WORD buttons = 0;
  XINPUT_STATE st;
  if (gXGet && active && (gPad >= 0 || timeGetTime() - gPadScanT > PAD_SCAN_MS)) {
    if (gPad < 0) {
      gPadScanT = timeGetTime();
      gPad = controllerIndex();
    }
    if (gPad >= 0 && gXGet(gPad, &st) == ERROR_SUCCESS) {
      sx = stickAxis(st.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
      sy = stickAxis(st.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
      buttons = st.Gamepad.wButtons;
    } else {
      gPad = -1;
    }
  }
  bool l = sx < -0.5f || (buttons & XINPUT_GAMEPAD_DPAD_LEFT), r = sx > 0.5f || (buttons & XINPUT_GAMEPAD_DPAD_RIGHT);
  bool u = sy > 0.5f || (buttons & XINPUT_GAMEPAD_DPAD_UP), d = sy < -0.5f || (buttons & XINPUT_GAMEPAD_DPAD_DOWN);
  if (l != gPadL || r != gPadR || u != gPadU || d != gPadD) {
    gPadL = l, gPadR = r, gPadU = u, gPadD = d;
    applyKeys();
  }
  double ax;
  if (sx != 0) hostTiltSensor(sx);                                   // the stick, when it's pushed
  else if (gLeft || gRight || gPadL || gPadR) hostTiltSensor(NAN);   // an arrow key or the D-pad, held
  else if (active && gTiltOn && accelX(ax)) hostTiltSensor((float)(ax / TILT_FULL_G));
  else hostTiltSensor(NAN);                                          // the keys
}

// ---- the window: one layered (per-pixel transparent) bitmap, drawn here: the Pokeball frame and the
// game's canvas inside. Outside the circle the window does not exist: clicks pass through. ----
static int gSize = DEFAULT_SIZE;  // points, at rest
static double gScale = 1.0;       // pixels per point (the display's scaling)
static double gRing = RING;       // the frame's width right now, in points: RING at rest, RING_HOVER with the mouse over it
static bool gHover = false;
static int gSide = 0;             // the window's side in pixels, margin included
static uint32_t *gPixels = NULL;  // gSide^2 premultiplied BGRA, top-down
static HBITMAP gBitmap = NULL;
static HDC gDC = NULL;
static uint8_t gCanvas[HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4];
static int gTouchX, gTouchY;  // where the finger is, in canvas coordinates
static bool gHoverWatch = false;  // asked to be told when the mouse leaves the window (TrackMouseEvent)
static bool gTouching = false;
static POINT gLastCursor = {-1, -1};

static int screenOffset() { return (int)lround(RING_HOVER * gScale); }
static int screenSide() { return gSide - 2 * screenOffset(); }

// Resampling the 466-pixel canvas to the screen's pixels: each output pixel averages the canvas pixels
// it covers (smaller), or takes the nearest one (bigger: the pixel art stays sharp), like the Mac's layer
struct Tap { int from, count; std::vector<float> w; };
static std::vector<Tap> gTaps;
static std::vector<float> gRows;  // the canvas resampled across, before down

static void buildTaps(int n) {
  gTaps.assign(n, Tap());
  double k = (double)HOST_CANVAS_SIZE / n;
  for (int i = 0; i < n; i++) {
    Tap &t = gTaps[i];
    if (k <= 1) {
      t.from = (int)((i + 0.5) * k);
      t.w.assign(1, 1.0f);
    } else {
      double a = i * k, b = (i + 1) * k;
      t.from = (int)a;
      for (int s = t.from; s < b && s < HOST_CANVAS_SIZE; s++) t.w.push_back((float)((fmin(b, s + 1) - fmax(a, s)) / k));
    }
    t.count = (int)t.w.size();
  }
  gRows.assign((size_t)HOST_CANVAS_SIZE * n * 3, 0);
}

static void resizeSurface() {
  gSide = (int)lround((gSize + 2 * RING_PAD) * gScale);
  if (gBitmap) DeleteObject(gBitmap);
  if (!gDC) gDC = CreateCompatibleDC(NULL);
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = gSide;
  bi.bmiHeader.biHeight = -gSide;  // top-down
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  gBitmap = CreateDIBSection(gDC, &bi, DIB_RGB_COLORS, (void **)&gPixels, NULL, 0);
  SelectObject(gDC, gBitmap);
  buildTaps(screenSide());
}

static inline uint32_t premul(double r, double g, double b, double a) {
  return ((uint32_t)lround(a * 255) << 24) | ((uint32_t)lround(r * a * 255) << 16) | ((uint32_t)lround(g * a * 255) << 8) |
         (uint32_t)lround(b * a * 255);
}

static void compose() {
  int n = screenSide(), off = screenOffset();
  // across: every canvas row to n columns
  for (int y = 0; y < HOST_CANVAS_SIZE; y++) {
    const uint8_t *row = gCanvas + (size_t)y * HOST_CANVAS_SIZE * 4;
    float *out = &gRows[(size_t)y * n * 3];
    for (int x = 0; x < n; x++) {
      const Tap &t = gTaps[x];
      float r = 0, g = 0, b = 0;
      for (int i = 0; i < t.count; i++) {
        const uint8_t *p = row + (t.from + i) * 4;
        r += p[0] * t.w[i], g += p[1] * t.w[i], b += p[2] * t.w[i];
      }
      out[x * 3] = r, out[x * 3 + 1] = g, out[x * 3 + 2] = b;
    }
  }
  double c = gSide / 2.0, rScreen = c - RING_HOVER * gScale, rBall = c - (RING_HOVER - gRing) * gScale;
  double band = RING_LINE / 2 * gScale, line = RING_OUTLINE * gScale;
  for (int y = 0; y < gSide; y++) {
    uint32_t *dst = gPixels + (size_t)y * gSide;
    double dy = y + 0.5 - c;
    for (int x = 0; x < gSide; x++) {
      double dx = x + 0.5 - c, d = sqrt(dx * dx + dy * dy);
      double cover = fmin(1, fmax(0, rBall - d + 0.5));
      if (cover <= 0) { dst[x] = 0; continue; }
      // the frame: red over white, the black band, and a thin outline so the white half shows on a light desktop
      double fr, fg, fb;
      if (fabs(dy) < band) fr = fg = fb = 0;
      else if (dy < 0) fr = 0.87, fg = 0.13, fb = 0.13;
      else fr = fg = fb = 1;
      double edge = fmin(1, fmax(0, d - (rBall - line) + 0.5));
      fr *= 1 - edge, fg *= 1 - edge, fb *= 1 - edge;
      // the screen, with a soft edge into the frame
      double inside = fmin(1, fmax(0, rScreen - d + 0.5));
      if (inside > 0) {
        int sx = x - off, sy = y - off;
        float r = 0, g = 0, b = 0;
        if (sx >= 0 && sy >= 0 && sx < n && sy < n) {
          const Tap &t = gTaps[sy];
          for (int i = 0; i < t.count; i++) {
            const float *p = &gRows[((size_t)(t.from + i) * n + sx) * 3];
            r += p[0] * t.w[i], g += p[1] * t.w[i], b += p[2] * t.w[i];
          }
        }
        fr = fr * (1 - inside) + r / 255 * inside;
        fg = fg * (1 - inside) + g / 255 * inside;
        fb = fb * (1 - inside) + b / 255 * inside;
      }
      dst[x] = premul(fr, fg, fb, cover);
    }
  }
}

static RECT restRect() {  // the window at rest, in screen pixels: what the size menu and the saved place mean
  RECT r;
  GetWindowRect(gWnd, &r);
  int pad = (int)lround(RING_PAD * gScale);
  InflateRect(&r, -pad, -pad);
  return r;
}

static void present(int x, int y) {
  POINT src = {0, 0}, dst = {x, y};
  SIZE sz = {gSide, gSide};
  BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
  UpdateLayeredWindow(gWnd, NULL, &dst, &sz, gDC, &src, 0, &blend, ULW_ALPHA);
}

static void redraw() {
  compose();
  RECT r;
  GetWindowRect(gWnd, &r);
  present(r.left, r.top);
}

static void saveWindowPlace() {
  RECT r = restRect();
  Preferences p;
  p.begin(PREFS_NS);
  p.putInt("size", gSize);
  p.putInt("x", r.left);
  p.putInt("y", r.top);
  p.end();
}

// a new size (points) or scale, keeping the center where it is
static void applySize(int size, double scale) {
  RECT r;
  GetWindowRect(gWnd, &r);
  int cx = (r.left + r.right) / 2, cy = (r.top + r.bottom) / 2;
  gSize = size;
  gScale = scale;
  resizeSurface();
  present(cx - gSide / 2, cy - gSide / 2);
  redraw();
}

// where the pointer is: the screen (a finger), the frame (drag), or nothing (passes through)
enum Zone { ZONE_NONE, ZONE_SCREEN, ZONE_RING };
static Zone zoneAt(int x, int y) {  // client pixels
  double c = gSide / 2.0, d = hypot(x + 0.5 - c, y + 0.5 - c);
  if (d <= c - RING_HOVER * gScale) return ZONE_SCREEN;
  if (d <= c - (RING_HOVER - gRing) * gScale) return ZONE_RING;
  return ZONE_NONE;
}

static void canvasPoint(int x, int y, int &cx, int &cy) {
  double k = (double)HOST_CANVAS_SIZE / screenSide();
  cx = (int)((x - screenOffset()) * k);
  cy = (int)((y - screenOffset()) * k);
}

static void applyKeys() {  // the keyboard's arrows and a controller's, together
  hostTiltKeys(gLeft || gPadL, gRight || gPadR);
  hostArrowKeys(gUp || gPadU, gDown || gPadD);
}

// without focus the key-up never arrives: the key would stay pressed forever
static void releaseKeys() {
  gLeft = gRight = gUp = gDown = false;
  applyKeys();
  hostButton(false);
}

static bool handleKey(WPARAM vk, bool down) {
  switch (vk) {
    case VK_LEFT: gLeft = down; applyKeys(); return true;
    case VK_RIGHT: gRight = down; applyKeys(); return true;
    case VK_UP: gUp = down; applyKeys(); return true;     // PokeDungeon
    case VK_DOWN: gDown = down; applyKeys(); return true;
    case VK_SPACE: hostButton(down); return true;
  }
  return false;
}

// ---- the menu: the same on right-click and on the notification-area icon ----
static void showMenu(POINT at) {
  HMENU m = CreatePopupMenu(), sizes = CreatePopupMenu();
  AppendMenuW(m, MF_STRING, ID_ABOUT, L"About " APP_NAME_W);
  AppendMenuW(m, MF_SEPARATOR, 0, NULL);
  AppendMenuW(m, MF_STRING, ID_SHOW, IsWindowVisible(gWnd) ? L"Hide " APP_NAME_W : L"Show " APP_NAME_W);
  const wchar_t *names[] = {L"Small", L"Medium", L"Large"};
  for (int i = 0; i < 3; i++) AppendMenuW(sizes, MF_STRING | (gSize == kSizes[i] ? MF_CHECKED : 0), ID_SIZE0 + i, names[i]);
  AppendMenuW(m, MF_POPUP, (UINT_PTR)sizes, L"Size");
  // the same setting as the game's SND button on the clock screen, saved with the game
  AppendMenuW(m, MF_STRING | (hostSoundEnabled() ? 0 : MF_CHECKED), ID_MUTE, L"Mute Sound");
  if (gXSet && controllerIndex() >= 0)  // only with a controller that can buzz
    AppendMenuW(m, MF_STRING | (gVibeOn ? MF_CHECKED : 0), ID_VIBE, L"Vibration");
  if (gAccel)  // only with a motion sensor
    AppendMenuW(m, MF_STRING | (gTiltOn ? MF_CHECKED : 0), ID_TILT, L"Tilt with the Device");
  std::wstring upd = gUpdating            ? L"Updating…"
                    : gNewVersion.empty() ? L"Check for Updates…"
                    : (!gNewExe.empty() && !gNewSig.empty() ? L"Update to Version " : L"Download Version ") + wide(gNewVersion) + L"…";
  AppendMenuW(m, MF_STRING | (gUpdating ? MF_GRAYED : 0), ID_UPDATE, upd.c_str());
  AppendMenuW(m, MF_STRING | (hostAutoUpdate() ? MF_CHECKED : 0), ID_AUTO_UPDATE, L"Update Automatically");
  AppendMenuW(m, MF_SEPARATOR, 0, NULL);
  AppendMenuW(m, MF_STRING | MF_GRAYED, 0, L"Drag the ring to move");
  AppendMenuW(m, MF_STRING | MF_GRAYED, 0, L"← → tilt  ·  Space: wild battle");
  AppendMenuW(m, MF_SEPARATOR, 0, NULL);
  AppendMenuW(m, MF_STRING, ID_QUIT, L"Quit " APP_NAME_W);
  SetForegroundWindow(gWnd);  // or the menu doesn't close when clicking elsewhere
  TrackPopupMenu(m, TPM_RIGHTBUTTON, at.x, at.y, 0, gWnd, NULL);
  PostMessageW(gWnd, WM_NULL, 0, 0);
  DestroyMenu(m);  // and its submenu
}

static HRESULT CALLBACK aboutLink(HWND, UINT msg, WPARAM, LPARAM lp, LONG_PTR) {
  if (msg == TDN_HYPERLINK_CLICKED) ShellExecuteW(NULL, L"open", (LPCWSTR)lp, NULL, NULL, SW_SHOWNORMAL);
  return S_OK;
}

static void showAbout() {
  std::wstring text = L"Version " + wide(APP_VERSION) +
                      L"\n\nBy <a href=\"mailto:" AUTHOR_EMAIL L"\">" AUTHOR_EMAIL L"</a>"
                      L"\n<a href=\"" GITHUB_URL L"\">PokeKnomiGochi on GitHub</a>"
                      L"\n<a href=\"" YOUTUBE_URL L"\">YouTube channel</a>"
                      L"\n\nSprites from PMDCollab (CC BY-NC). "
                      L"Pokémon © Nintendo / Game Freak / The Pokémon Company.";
  TASKDIALOGCONFIG cfg = {};
  cfg.cbSize = sizeof(cfg);
  cfg.hwndParent = gWnd;
  cfg.hInstance = gInst;
  cfg.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_POSITION_RELATIVE_TO_WINDOW;
  cfg.pszWindowTitle = L"About " APP_NAME_W;
  cfg.pszMainIcon = MAKEINTRESOURCEW(IDI_APP);
  cfg.pszMainInstruction = APP_NAME_W;
  cfg.pszContent = text.c_str();
  cfg.dwCommonButtons = TDCBF_OK_BUTTON;
  cfg.pfCallback = aboutLink;
  TaskDialogIndirect(&cfg, NULL, NULL, NULL);
}

// ---- new versions ----
struct UpdateResult { bool answered = false; std::string version; std::wstring url, exe, sig; };

// the signature (ECDSA P-256 over SHA-256, DER) of an update, against the key the app carries
static bool updateSignatureOK(const std::string &data, const std::string &sigDer) {
  // DER: 30 len 02 lenR R 02 lenS S -> R and S as 32 bytes each, as BCrypt wants them
  unsigned char rs[64] = {0};
  const unsigned char *d = (const unsigned char *)sigDer.data();
  size_t n = sigDer.size(), at = 2;
  if (n < 8 || d[0] != 0x30) return false;
  for (int k = 0; k < 2; k++) {
    if (at + 2 > n || d[at] != 0x02) return false;
    size_t len = d[at + 1];
    at += 2;
    if (at + len > n) return false;
    const unsigned char *v = d + at;
    while (len > 32 && *v == 0) { v++; len--; }  // a leading zero keeps the number positive
    if (len > 32) return false;
    memcpy(rs + k * 32 + (32 - len), v, len);
    at += v - (d + at) + len;
  }
  BCRYPT_ALG_HANDLE hash = NULL, ecdsa = NULL;
  BCRYPT_KEY_HANDLE key = NULL;
  unsigned char digest[32];
  bool ok = BCryptOpenAlgorithmProvider(&hash, BCRYPT_SHA256_ALGORITHM, NULL, 0) == 0 &&
            BCryptHash(hash, NULL, 0, (PUCHAR)data.data(), (ULONG)data.size(), digest, sizeof(digest)) == 0;
  if (ok) {
    unsigned char blob[sizeof(BCRYPT_ECCKEY_BLOB) + 64];
    BCRYPT_ECCKEY_BLOB *h = (BCRYPT_ECCKEY_BLOB *)blob;
    h->dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    h->cbKey = 32;
    memcpy(blob + sizeof(BCRYPT_ECCKEY_BLOB), UPDATE_PUBLIC_KEY + 1, 64);  // X then Y, after the 0x04
    ok = BCryptOpenAlgorithmProvider(&ecdsa, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0) == 0 &&
         BCryptImportKeyPair(ecdsa, NULL, BCRYPT_ECCPUBLIC_BLOB, &key, blob, sizeof(blob), 0) == 0 &&
         BCryptVerifySignature(key, NULL, digest, sizeof(digest), rs, sizeof(rs), 0) == 0;
  }
  if (key) BCryptDestroyKey(key);
  if (ecdsa) BCryptCloseAlgorithmProvider(ecdsa, 0);
  if (hash) BCryptCloseAlgorithmProvider(hash, 0);
  return ok;
}

static std::wstring exePath() {
  wchar_t p[MAX_PATH * 2];
  DWORD n = GetModuleFileNameW(NULL, p, MAX_PATH * 2);
  return std::wstring(p, n);
}
static std::wstring besideExe(const wchar_t *name) {
  std::wstring p = exePath();
  return p.substr(0, p.find_last_of(L"\\/") + 1) + name;
}

// Asks GitHub for the newest Windows release. From the menu (asked) it always answers; the
// automatic check only speaks up the first time it finds each new version.
static void offerUpdate();
static void removeTrayIcon();
static void checkForUpdates(bool asked) {
  if (gUpdating) return;
  if (asked && !gNewVersion.empty()) {  // already known: update, or straight to the download
    if (!gNewExe.empty() && !gNewSig.empty()) offerUpdate();
    else ShellExecuteW(NULL, L"open", gNewURL.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return;
  }
  std::thread([asked] {
    std::string body;
    UpdateResult *res = new UpdateResult;
    // POKEMACGOCHI_UPDATE_API: another release list, for testing the update on a local server
    const char *api = getenv("POKEMACGOCHI_UPDATE_API");
    if ((res->answered = api ? httpGetUrl(wide(api), body)
                             : httpGet(GITHUB_API_HOST, RELEASES_PATH, L"Accept: application/vnd.github+json\r\n", body))) {
      // each release's fields after its "tag_name" (draft, prerelease, assets) come before the next one's
      const std::string key = "\"tag_name\":\"";
      for (size_t at = body.find(key); at != std::string::npos;) {
        size_t next = body.find(key, at + key.size());
        std::string rel = body.substr(at, next == std::string::npos ? std::string::npos : next - at);
        at = next;
        std::string tag = rel.substr(key.size(), rel.find('"', key.size()) - key.size());
        if (tag.compare(0, strlen(RELEASE_TAG), RELEASE_TAG) != 0) continue;
        if (rel.find("\"draft\":true") != std::string::npos || rel.find("\"prerelease\":true") != std::string::npos) continue;
        std::string v = tag.substr(strlen(RELEASE_TAG));
        if (!res->version.empty() && compareVersions(v, res->version) <= 0) continue;
        res->version = v;
        res->url = wide("https://github.com/FedorSosnin/PokeKnomiGochi/releases/tag/" + tag);
        res->exe.clear();
        res->sig.clear();
        const std::string dl = "\"browser_download_url\":\"";
        auto endsWith = [](const std::string &u, const char *tail) {
          return u.size() >= strlen(tail) && u.compare(u.size() - strlen(tail), std::string::npos, tail) == 0;
        };
        for (size_t a = rel.find(dl); a != std::string::npos; a = rel.find(dl, a + 1)) {
          std::string u = rel.substr(a + dl.size(), rel.find('"', a + dl.size()) - a - dl.size());
          if (endsWith(u, "/" RELEASE_ASSET)) res->url = res->exe = wide(u);
          if (endsWith(u, "/" RELEASE_SIG)) res->sig = wide(u);
        }
      }
    }
    PostMessageW(gWnd, WM_UPDATE_FOUND, asked, (LPARAM)res);
  }).detach();
}

static void updateCheckFound(const UpdateResult &r, bool asked) {
  bool newer = !r.version.empty() && compareVersions(r.version, APP_VERSION) > 0;
  if (newer) {
    gNewVersion = r.version;
    gNewURL = r.url;
    gNewExe = r.exe;
    gNewSig = r.sig;
  }
  if (!asked && newer && canUpdateInPlace() && hostAutoUpdate()) {  // goes in by itself, once nobody's using it
    if (!gAutoPending) fprintf(stderr, "update: %s found, updating once it's quiet\n", r.version.c_str());
    gAutoPending = true;
    return;
  }
  Preferences p;
  p.begin(PREFS_NS);
  char told[32] = "";
  p.getString("told", told, sizeof(told));
  if (!asked && (!newer || r.version == told)) { p.end(); return; }
  if (newer) p.putString("told", r.version.c_str());
  p.end();
  if (newer && !gNewExe.empty() && !gNewSig.empty()) {
    offerUpdate();
    return;
  }
  std::wstring title, text;
  UINT type = MB_ICONINFORMATION | MB_OK;
  if (newer) {
    title = APP_NAME_W L" " + wide(r.version) + L" is available";
    text = L"You have version " + wide(APP_VERSION) +
           L". Your pet is saved separately from the app, so it stays when you replace the app with the new one: "
           L"quit " APP_NAME_W L", download the new " RELEASE_ASSET L" and put it where this one is.\n\nDownload it now?";
    type = MB_ICONINFORMATION | MB_YESNO;
  } else if (r.answered) {
    title = L"You're up to date";
    text = APP_NAME_W L" " + wide(APP_VERSION) + L" is the newest version.";
  } else {
    title = L"Couldn't check for updates";
    text = L"Check your internet connection and try again.";
  }
  if (MessageBoxW(gWnd, text.c_str(), title.c_str(), type | MB_TOPMOST) == IDYES)
    ShellExecuteW(NULL, L"open", r.url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

// downloads the new .exe and its signature, checks it and writes it beside this one
static void installUpdate() {
  if (gUpdating || gNewExe.empty() || gNewSig.empty()) return;
  gUpdating = true;
  std::wstring exeUrl = gNewExe, sigUrl = gNewSig;
  fprintf(stderr, "update: downloading %s\n", gNewVersion.c_str());
  std::thread([exeUrl, sigUrl] {
    std::string sig, exe;
    std::wstring *out = new std::wstring;
    if (httpGetUrl(sigUrl, sig) && httpGetUrl(exeUrl, exe) && exe.size() > 4096 && exe[0] == 'M' && exe[1] == 'Z') {
      if (updateSignatureOK(exe, sig)) {
        std::wstring fresh = besideExe(L"" APP_NAME_W L".new.exe");
        HANDLE f = CreateFileW(fresh.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        DWORD wrote = 0;
        bool ok = f != INVALID_HANDLE_VALUE && WriteFile(f, exe.data(), (DWORD)exe.size(), &wrote, NULL) && wrote == exe.size();
        if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
        if (ok) *out = fresh;
        else {
          DeleteFileW(fresh.c_str());
          *out = L"!The update couldn't be written next to " APP_NAME_W L".";
        }
      } else {
        *out = L"!The update didn't pass its safety check, so it wasn't installed.";
      }
    } else {
      *out = L"!The update couldn't be downloaded.";
    }
    PostMessageW(gWnd, WM_UPDATE_READY, 0, (LPARAM)out);
  }).detach();
}

static void updateFailed(const std::wstring &why) {
  gUpdating = false;
  fprintf(stderr, "update: failed: %s\n", utf8(why).c_str());
  if (gConsole || gAutoInstall) {  // by itself: no message for nobody; tomorrow's check tries again
    gAutoInstall = false;
    return;
  }
  std::wstring text = why + L"\n\nDownload the new version instead?";
  if (MessageBoxW(gWnd, text.c_str(), L"Couldn't update", MB_ICONWARNING | MB_YESNO | MB_TOPMOST) == IDYES)
    ShellExecuteW(NULL, L"open", gNewURL.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

// the new .exe is beside this one: save, swap the two, start it and quit
static void swapInUpdate(const std::wstring &fresh) {
  hostFlushSave();  // the pet first
  std::wstring cur = exePath(), old = besideExe(L"" APP_NAME_W L".old.exe");
  DeleteFileW(old.c_str());
  if (!MoveFileExW(cur.c_str(), old.c_str(), MOVEFILE_REPLACE_EXISTING)) {
    DeleteFileW(fresh.c_str());
    updateFailed(L"This copy of " APP_NAME_W L" couldn't be replaced.");
    return;
  }
  if (!MoveFileExW(fresh.c_str(), cur.c_str(), MOVEFILE_REPLACE_EXISTING)) {
    MoveFileExW(old.c_str(), cur.c_str(), MOVEFILE_REPLACE_EXISTING);  // put this one back
    DeleteFileW(fresh.c_str());
    updateFailed(L"The new " APP_NAME_W L" couldn't be put in place.");
    return;
  }
  fprintf(stderr, "update: installed, reopening\n");
  // the new one starts with the same arguments and environment (a test copy keeps its own save); the
  // save's lock goes first, or it would think this one is still open
  if (gLock != INVALID_HANDLE_VALUE) {
    CloseHandle(gLock);
    gLock = INVALID_HANDLE_VALUE;
  }
  std::wstring cmd = GetCommandLineW();
  STARTUPINFOW si = {sizeof(si)};
  PROCESS_INFORMATION pi = {};
  if (CreateProcessW(cur.c_str(), &cmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
  removeTrayIcon();
  ExitProcess(0);
}

// "Update now?", then the update in place (a test copy doesn't wait for a click)
static void offerUpdate() {
  std::wstring title = APP_NAME_W L" " + wide(gNewVersion) + L" is available";
  std::wstring text = L"You have version " + wide(APP_VERSION) + L". Updating takes a few seconds: " APP_NAME_W
                      L" closes and opens again on the new version, with your pet just as it was.\n\nUpdate now?";
  if (gConsole || MessageBoxW(gWnd, text.c_str(), title.c_str(), MB_ICONINFORMATION | MB_YESNO | MB_TOPMOST) == IDYES)
    installUpdate();
}

// ---- the notification-area icon (the Mac's menu bar Poke Ball) ----
static UINT gTaskbarCreated;
static void addTrayIcon() {
  NOTIFYICONDATAW n = {};
  n.cbSize = sizeof(n);
  n.hWnd = gWnd;
  n.uID = 1;
  n.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  n.uCallbackMessage = WM_TRAY;
  LoadIconMetric(gInst, MAKEINTRESOURCEW(IDI_APP), LIM_SMALL, &n.hIcon);
  wcscpy(n.szTip, APP_NAME_W);
  Shell_NotifyIconW(NIM_ADD, &n);
}
static void removeTrayIcon() {
  NOTIFYICONDATAW n = {};
  n.cbSize = sizeof(n);
  n.hWnd = gWnd;
  n.uID = 1;
  Shell_NotifyIconW(NIM_DELETE, &n);
}

// while hidden it stays alive: the game keeps running, like a Knomi with the screen off
static void toggleWindow() {
  if (IsWindowVisible(gWnd)) {
    ShowWindow(gWnd, SW_HIDE);
  } else {
    ShowWindow(gWnd, SW_SHOWNOACTIVATE);
    redraw();
    if (!gConsole) SetForegroundWindow(gWnd);
  }
}

static void quit() {
  hostFlushSave();
  removeTrayIcon();
  ExitProcess(0);  // the audio and relay threads go with it
}

// The game has erased the save: a fresh process on it, the same way it was started
static void restart() {
  hostFlushSave();
  removeTrayIcon();
  if (gLock != INVALID_HANDLE_VALUE) CloseHandle(gLock);
  wchar_t exe[MAX_PATH];
  GetModuleFileNameW(NULL, exe, MAX_PATH);
  std::wstring cmd = GetCommandLineW();
  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  if (CreateProcessW(exe, &cmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
  ExitProcess(0);
}

// ---- testing (--console): what a person would do, from stdin ----
// "MOUSE down|drag|up|right x y" in canvas coordinates, "KEY down|up vk", "HOVER on|off", "MENU id" (see the
// ID_ list) and "SHOT file.bmp" (the window as drawn, frame included); anything else is a serial command
static void writeShot(const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f) return;
  uint32_t bytes = (uint32_t)gSide * gSide * 4;
  BITMAPFILEHEADER fh = {0x4d42, (DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bytes), 0, 0,
                         sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)};
  BITMAPINFOHEADER ih = {sizeof(ih), gSide, -gSide, 1, 32, BI_RGB, bytes, 0, 0, 0, 0};
  fwrite(&fh, sizeof(fh), 1, f);
  fwrite(&ih, sizeof(ih), 1, f);
  fwrite(gPixels, bytes, 1, f);
  fclose(f);
  printf("shot %s\n", path);
  fflush(stdout);
}

static void testEvent(const std::string &line) {
  char what[16] = "", path[MAX_PATH] = "";
  int a = 0, b = 0;
  if (sscanf(line.c_str(), "MENU %d", &a) == 1) {
    SendMessageW(gWnd, WM_COMMAND, a, 0);
  } else if (sscanf(line.c_str(), "HOVER %15s", what) == 1) {
    gHover = !strcmp(what, "on");
  } else if (sscanf(line.c_str(), "SHOT %259s", path) == 1) {
    writeShot(path);
  } else if (sscanf(line.c_str(), "MOUSE %15s %d %d", what, &a, &b) == 3) {
    double k = (double)screenSide() / HOST_CANVAS_SIZE;
    LPARAM at = MAKELPARAM((int)(screenOffset() + a * k), (int)(screenOffset() + b * k));
    UINT msg = !strcmp(what, "down") ? WM_LBUTTONDOWN : !strcmp(what, "drag") ? WM_MOUSEMOVE
               : !strcmp(what, "right") ? WM_RBUTTONUP : WM_LBUTTONUP;
    SendMessageW(gWnd, msg, !strcmp(what, "drag") ? MK_LBUTTON : 0, at);
  } else if (sscanf(line.c_str(), "KEY %15s %d", what, &a) == 2) {
    SendMessageW(gWnd, !strcmp(what, "down") ? WM_KEYDOWN : WM_KEYUP, a, 0);
  }
}

static void consoleThread() {
  char line[256];
  while (fgets(line, sizeof line, stdin)) {
    if (!strncmp(line, "MOUSE ", 6) || !strncmp(line, "KEY ", 4) || !strncmp(line, "MENU ", 5) ||
        !strncmp(line, "HOVER ", 6) || !strncmp(line, "SHOT ", 5)) {
      std::string *s = new std::string(line);
      while (!s->empty() && (s->back() == '\n' || s->back() == '\r')) s->pop_back();
      PostMessageW(gWnd, WM_TEST, 0, (LPARAM)s);
    } else {
      hostSerialPush(line);
    }
  }
}

// ---- network time: the game's clock follows a time server (the Date of an answer from HOST_TIME_URL),
// not the PC's clock, so setting the PC's clock can't fake time away. A HEAD request on its own thread;
// the answer goes to the game's thread (0: none) ----
static void fetchNetTime() {
  std::thread([] {
    const char *env = getenv("POKEMACGOCHI_TIME_URL");  // testing
    std::wstring url = wide(env && *env ? env : HOST_TIME_URL);
    URL_COMPONENTS u = {sizeof(u)};
    wchar_t host[256] = L"", path[2048] = L"";
    u.lpszHostName = host;
    u.dwHostNameLength = 256;
    u.lpszUrlPath = path;
    u.dwUrlPathLength = 2048;
    uint32_t utc = 0;
    if (WinHttpCrackUrl(url.c_str(), 0, 0, &u)) {
      HINTERNET c = WinHttpConnect(httpSession(), host, u.nPort, 0);
      HINTERNET r = c ? WinHttpOpenRequest(c, L"HEAD", path[0] ? path : L"/", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           u.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
                      : NULL;
      SYSTEMTIME st;
      DWORD len = sizeof(st);
      if (r) {
        WinHttpSetTimeouts(r, 8000, 8000, 15000, 15000);
        if (WinHttpSendRequest(r, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(r, NULL) &&
            WinHttpQueryHeaders(r, WINHTTP_QUERY_DATE | WINHTTP_QUERY_FLAG_SYSTEMTIME, WINHTTP_HEADER_NAME_BY_INDEX,
                                &st, &len, WINHTTP_NO_HEADER_INDEX)) {
          FILETIME ft;  // 100 ns since 1601
          if (SystemTimeToFileTime(&st, &ft))
            utc = (uint32_t)(((((uint64_t)ft.dwHighDateTime) << 32 | ft.dwLowDateTime) - 116444736000000000ULL) / 10000000ULL);
        }
        WinHttpCloseHandle(r);
      }
      if (c) WinHttpCloseHandle(c);
    }
    PostMessageW(gWnd, WM_NET_TIME, 0, (LPARAM)utc);
  }).detach();
}

// ---- the game loop and the window's messages ----
static DWORD gLastTick = 0;  // timeGetTime(): to the millisecond with timeBeginPeriod(1)
static void tick() {
  gLastTick = timeGetTime();
  if (!gConsole) readTilt();  // tests steer with their own commands
  hostTick();
  if (hostNetTimeWanted()) fetchNetTime();
  bool dirty = hostFrameDirty();
  if (dirty) hostFrameRGBA(gCanvas);
  // the frame grows while the mouse is over the ball (a look at the cursor each tick is all it takes)
  POINT cur;
  bool over = gHover;
  if (!gConsole && GetCursorPos(&cur) && IsWindowVisible(gWnd)) {
    RECT r;
    GetWindowRect(gWnd, &r);
    Zone z = zoneAt(cur.x - r.left, cur.y - r.top);
    over = z != ZONE_NONE || gTouching || GetCapture() == gWnd;
    if (z == ZONE_SCREEN && (cur.x != gLastCursor.x || cur.y != gLastCursor.y)) hostPointerHover();
    gLastCursor = cur;
  }
  double target = over ? RING_HOVER : RING, step = (RING_HOVER - RING) * TICK_MS / RING_ANIM_MS;
  bool anim = gRing != target;
  gRing = gRing < target ? fmin(target, gRing + step) : fmax(target, gRing - step);
  if ((dirty || anim) && IsWindowVisible(gWnd)) redraw();
  if (hostRestartRequested()) restart();
  if (gAutoPending && !gUpdating && hostAutoUpdate() && hostQuietForUpdate()) {
    gAutoPending = false;
    gAutoInstall = true;
    fprintf(stderr, "update: quiet, updating to %s by itself\n", gNewVersion.c_str());
    installUpdate();
  }
}

static LRESULT CALLBACK wndProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == gTaskbarCreated && gTaskbarCreated) {  // Explorer restarted: the icon is gone
    addTrayIcon();
    return 0;
  }
  switch (msg) {
    case WM_TIMER:
      if (wp == TIMER_TICK) {  // only while Windows runs a loop of its own (dragging, a menu, a dialog)
        if (timeGetTime() - gLastTick >= TICK_MS) tick();
      }
      else if (wp == TIMER_PING) relayPing();
      else if (wp == TIMER_UPDATE) checkForUpdates(false);
      return 0;
    case WM_NCHITTEST: {
      POINT p = {(short)LOWORD(lp), (short)HIWORD(lp)};
      ScreenToClient(w, &p);
      Zone z = zoneAt(p.x, p.y);
      return z == ZONE_RING ? HTCAPTION : z == ZONE_SCREEN ? HTCLIENT : HTTRANSPARENT;
    }
    case WM_SETCURSOR: {  // over the screen the cursor is a finger, as on the Knomi; the frame moves the window
      WORD hit = LOWORD(lp);
      if (hit == HTCAPTION) { SetCursor(LoadCursor(NULL, IDC_SIZEALL)); return TRUE; }
      if (hit == HTCLIENT) { SetCursor(LoadCursor(NULL, IDC_HAND)); return TRUE; }
      break;
    }
    case WM_MOUSEACTIVATE: return MA_ACTIVATE;  // the first click is already a touch
    case WM_LBUTTONDOWN: {
      if (zoneAt((short)LOWORD(lp), (short)HIWORD(lp)) != ZONE_SCREEN && !gConsole) return 0;
      canvasPoint((short)LOWORD(lp), (short)HIWORD(lp), gTouchX, gTouchY);
      gTouching = true;
      hostPointer(gTouchX, gTouchY, true);
      SetCapture(w);
      return 0;
    }
    case WM_MOUSEMOVE:
      if (gTouching) {
        canvasPoint((short)LOWORD(lp), (short)HIWORD(lp), gTouchX, gTouchY);
        hostPointer(gTouchX, gTouchY, true);
      } else {  // no button: V2 outlines the control under it; told when it leaves the window
        if (zoneAt((short)LOWORD(lp), (short)HIWORD(lp)) == ZONE_SCREEN) {
          int hx, hy;
          canvasPoint((short)LOWORD(lp), (short)HIWORD(lp), hx, hy);
          hostPointerHoverAt(hx, hy);
        } else {
          hostPointerHoverAt(-1, -1);
        }
        if (!gHoverWatch) {
          TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, w, 0 };
          gHoverWatch = TrackMouseEvent(&tme) != 0;
        }
      }
      return 0;
    case WM_MOUSELEAVE:
      gHoverWatch = false;
      hostPointerHoverAt(-1, -1);
      return 0;
    case WM_LBUTTONUP:
      if (gTouching) {
        canvasPoint((short)LOWORD(lp), (short)HIWORD(lp), gTouchX, gTouchY);
        gTouching = false;
        ReleaseCapture();
        hostPointer(gTouchX, gTouchY, false);
      }
      return 0;
    case WM_CAPTURECHANGED:
      if (gTouching && (HWND)lp != w) {  // lost the mouse mid-touch: the finger lifts where it was
        gTouching = false;
        hostPointer(gTouchX, gTouchY, false);
      }
      return 0;
    case WM_RBUTTONUP:
    case WM_NCRBUTTONUP: {
      POINT p;
      GetCursorPos(&p);
      showMenu(p);
      return 0;
    }
    case WM_NCRBUTTONDOWN:
      return 0;  // not the system menu
    case WM_CONTEXTMENU:
      return 0;
    case WM_EXITSIZEMOVE:
      saveWindowPlace();
      return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      if (lp & (1 << 30)) return 0;  // held down: the game counts the first press only
      if (handleKey(wp, true)) return 0;
      break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
      if (handleKey(wp, false)) return 0;
      break;
    case WM_KILLFOCUS:
      releaseKeys();
      return 0;
    case WM_DPICHANGED:  // moved to a display with other scaling
      applySize(gSize, HIWORD(wp) / 96.0);
      return 0;
    case WM_TRAY:
      if (LOWORD(lp) == WM_LBUTTONUP || LOWORD(lp) == WM_RBUTTONUP) {
        POINT p;
        GetCursorPos(&p);
        showMenu(p);
      }
      return 0;
    case WM_COMMAND:
      switch (LOWORD(wp)) {
        case ID_ABOUT: showAbout(); break;
        case ID_SHOW: toggleWindow(); break;
        case ID_SIZE0: case ID_SIZE1: case ID_SIZE2:
          applySize(kSizes[LOWORD(wp) - ID_SIZE0], gScale);
          saveWindowPlace();
          break;
        case ID_MUTE: hostSetSoundEnabled(!hostSoundEnabled()); break;
        case ID_VIBE: {
          gVibeOn = !gVibeOn;
          Preferences p;
          p.begin(PREFS_NS);
          p.putBool("vibe", gVibeOn);
          p.end();
          if (gVibeOn) haptic(0);  // a taste of it
          break;
        }
        case ID_TILT: {
          gTiltOn = !gTiltOn;
          Preferences p;
          p.begin(PREFS_NS);
          p.putBool("tilt", gTiltOn);
          p.end();
          break;
        }
        case ID_UPDATE: checkForUpdates(true); break;
        case ID_AUTO_UPDATE:
          hostSetAutoUpdate(!hostAutoUpdate());
          gAutoPending = hostAutoUpdate() && !gNewVersion.empty() && canUpdateInPlace();  // one already found goes in too
          break;
        case ID_QUIT: quit(); break;
      }
      return 0;
    case WM_RELAY_MSG: {
      std::string *m = (std::string *)lp;
      relayMessage(wp != 0, *m);
      delete m;
      return 0;
    }
    case WM_RELAY_DOWN:
      hostRelayConnected(NULL);
      return 0;
    case WM_UPDATE_FOUND: {
      UpdateResult *r = (UpdateResult *)lp;
      updateCheckFound(*r, wp != 0);
      delete r;
      return 0;
    }
    case WM_NET_TIME:
      hostNetTime((uint32_t)lp);
      return 0;
    case WM_UPDATE_READY: {
      std::wstring *path = (std::wstring *)lp;
      if (!path->empty() && (*path)[0] != L'!') swapInUpdate(*path);
      else updateFailed(path->empty() ? L"The update failed." : path->substr(1));
      delete path;
      return 0;
    }
    case WM_TEST: {
      std::string *s = (std::string *)lp;
      testEvent(*s);
      delete s;
      return 0;
    }
    case WM_POWERBROADCAST:
      if (wp == PBT_APMSUSPEND) hostFlushSave();  // the PC is going to sleep
      return TRUE;
    case WM_QUERYENDSESSION:
      hostFlushSave();
      return TRUE;
    case WM_ENDSESSION:
      if (wp) hostFlushSave();
      return 0;
    case WM_CLOSE:  // Alt+F4: hides it, like the Mac's window; Quit is in the menu
      toggleWindow();
      return 0;
  }
  return DefWindowProcW(w, msg, wp, lp);
}

// the saved place, if it's still on a display; else the bottom right corner of the main one
static RECT initialRest(int &size) {
  Preferences p;
  p.begin(PREFS_NS, true);
  bool known = p.isKey("x");
  size = p.getInt("size", 0);
  int x = p.getInt("x", 0), y = p.getInt("y", 0);
  p.end();
  if (size < kSizes[0] || size > kSizes[2]) size = DEFAULT_SIZE;
  if (known) {
    POINT at = {x, y};
    HMONITOR mon = MonitorFromPoint(at, MONITOR_DEFAULTTONULL);  // a display that is no longer there doesn't take the window with it
    if (mon) {
      UINT dpi = 96, dpiY = 96;
      typedef HRESULT(WINAPI * GetDpi)(HMONITOR, int, UINT *, UINT *);
      GetDpi getDpi = (GetDpi)(void *)GetProcAddress(LoadLibraryW(L"shcore.dll"), "GetDpiForMonitor");
      if (getDpi) getDpi(mon, 0, &dpi, &dpiY);
      gScale = dpi / 96.0;
      int side = (int)lround(size * gScale);
      return {x, y, x + side, y + side};
    }
  }
  HMONITOR mon = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO mi = {sizeof(mi)};
  GetMonitorInfoW(mon, &mi);
  gScale = GetDpiForSystem() / 96.0;
  int side = (int)lround(size * gScale), gap = (int)lround(24 * gScale);
  return {mi.rcWork.right - side - gap, mi.rcWork.bottom - side - gap, mi.rcWork.right - gap, mi.rcWork.bottom - gap};
}

// Two processes on the same save would overwrite each other's saves
static bool lockSave(const std::string &saveDir) {
  SHCreateDirectoryExW(NULL, winPath(saveDir).c_str(), NULL);
  gLock = CreateFileW(wide(saveDir + "/.lock").c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                      FILE_ATTRIBUTE_HIDDEN, NULL);
  return gLock != INVALID_HANDLE_VALUE;
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int) {
  gInst = inst;
  // after an update: the old .exe, renamed out of the way while it ran (it may still be closing)
  std::thread([] {
    for (int i = 0; i < 20 && GetFileAttributesW(besideExe(L"" APP_NAME_W L".old.exe").c_str()) != INVALID_FILE_ATTRIBUTES; i++) {
      DeleteFileW(besideExe(L"" APP_NAME_W L".old.exe").c_str());
      Sleep(500);
    }
  }).detach();
  int argc = 0;
  wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  for (int i = 1; i < argc; i++)
    if (!wcscmp(argv[i], L"--console")) gConsole = true;
  if (gConsole && AttachConsole(ATTACH_PARENT_PROCESS)) {  // a GUI app has no console of its own: the one it was started from
    if (!GetStdHandle(STD_OUTPUT_HANDLE)) freopen("CONOUT$", "w", stdout);
    if (!GetStdHandle(STD_ERROR_HANDLE)) freopen("CONOUT$", "w", stderr);
  }

  const char *env = getenv("POKEMACGOCHI_SAVE_DIR");
  gOwnSave = env == NULL;
  std::string saveDir = env ? env : knownFolder(FOLDERID_RoamingAppData) + "/" APP_NAME;
  for (char &c : saveDir)
    if (c == '\\') c = '/';
  if (!lockSave(saveDir)) {
    fprintf(stderr, APP_NAME " is already open\n");
    if (gOwnSave)  // opened again by hand: say where the running one is instead of doing nothing
      MessageBoxW(NULL,
                  L"It's the round window on your desktop. If it is hidden, use the Poké Ball icon in the "
                  L"taskbar's notification area to show it.",
                  APP_NAME_W L" is already open", MB_ICONINFORMATION | MB_OK | MB_TOPMOST);
    return 0;
  }
  std::string res = unpackResources();
  hostInit(res.c_str(), saveDir.c_str());
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);  // the sensors are COM
  startVibration();
  startTiltSensor();

  WNDCLASSEXW wc = {sizeof(wc)};
  wc.lpfnWndProc = wndProc;
  wc.hInstance = inst;
  wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(IDI_APP));
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = APP_NAME_W;
  RegisterClassExW(&wc);
  RECT rest = initialRest(gSize);
  int pad = (int)lround(RING_PAD * gScale);
  // floating above other windows, outside the taskbar (the notification-area icon finds it)
  gWnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, APP_NAME_W, APP_NAME_W, WS_POPUP,
                         rest.left - pad, rest.top - pad, 1, 1, NULL, NULL, inst, NULL);
  resizeSurface();
  hostFrameRGBA(gCanvas);
  compose();
  present(rest.left - pad, rest.top - pad);
  ShowWindow(gWnd, gConsole ? SW_SHOWNOACTIVATE : SW_SHOW);
  if (!gConsole) SetForegroundWindow(gWnd);  // with the console, without stealing focus from whoever is testing
  gTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
  addTrayIcon();
  if (gConsole) std::thread(consoleThread).detach();  // its events go to the window: not before there is one

  SetTimer(gWnd, TIMER_TICK, TICK_MS, NULL);  // the game while the window is being dragged or the menu is open
  std::thread(audioThread).detach();         // starting the audio device can take a moment: not on the game's thread
  // the relay: the real app always; test copies only when asked (POKEMACGOCHI_RELAY=1), so they don't
  // turn up as buddies on everyone's devices at home
  const char *relayEnv = getenv("POKEMACGOCHI_RELAY");
  if (relayEnv ? atoi(relayEnv) != 0 : gOwnSave) {
    std::thread(relayThread).detach();
    SetTimer(gWnd, TIMER_PING, 45000, NULL);
  }
  // test copies never nag, unless given a release list of their own to test updating with
  if ((gOwnSave && !gConsole) || getenv("POKEMACGOCHI_UPDATE_API")) {
    SetTimer(gWnd, TIMER_UPDATE, UPDATE_EVERY_MS, NULL);
    checkForUpdates(false);
  }

  // the game's loop: its tick every TICK_MS, and the window's messages in between
  timeBeginPeriod(1);  // sleeps to the millisecond, not the system's 15.6 ms step
  for (;;) {
    DWORD now = timeGetTime(), due = gLastTick + TICK_MS;
    if ((int)(due - now) <= 0) {
      tick();
      continue;
    }
    MsgWaitForMultipleObjects(0, NULL, FALSE, due - now, QS_ALLINPUT);
    MSG m;
    while (PeekMessageW(&m, NULL, 0, 0, PM_REMOVE)) {
      if (m.message == WM_QUIT) quit();
      TranslateMessage(&m);
      DispatchMessageW(&m);
    }
  }
  return 0;
}
