// Tests for the host layer, without a window. The first ones are individual pieces; the last one starts the whole
// game against the real sprites and a save in a temporary folder.
#include <arpa/inet.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include "U8g2lib.h"
#include "buddy_net.h"
#include "audio.h"
#include "savename.h"
#include "sfx_notes.h"
#include <math.h>
#include "glcdfont.h"
#include "host.h"
#include "host_crt.h"
#include "host_hal.h"
#include "host_internal.h"
#include "rtcbat.h"
#include "i18n.h"
#include "pet.h"
#include "press.h"
#include "sdmon.h"
#include "power.h"
#include <Preferences.h>

static int gFails = 0, gChecks = 0;
#define CHECK(cond)                                                     \
  do {                                                                  \
    gChecks++;                                                          \
    if (!(cond)) {                                                      \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);           \
      gFails++;                                                         \
    }                                                                   \
  } while (0)

static std::string tempDir() {
  char t[] = "/tmp/pokehost.XXXXXX";
  return std::string(mkdtemp(t));
}

static void removeDir(const std::string &dir) {
  if (DIR *d = opendir(dir.c_str())) {
    while (dirent *e = readdir(d))
      if (e->d_name[0] != '.' || strlen(e->d_name) > 2) unlink((dir + "/" + e->d_name).c_str());
    closedir(d);
  }
  rmdir(dir.c_str());
}

static std::vector<uint8_t> readFile(const std::string &path) {
  std::vector<uint8_t> v;
  if (FILE *f = fopen(path.c_str(), "rb")) {
    fseek(f, 0, SEEK_END);
    v.resize((size_t)ftell(f));
    rewind(f);
    if (fread(v.data(), 1, v.size(), f) != v.size()) v.clear();
    fclose(f);
  }
  return v;
}

static int countColor(Arduino_Canvas &c, uint16_t color) {
  int n = 0;
  for (int i = 0; i < c.width() * c.height(); i++) n += c.framebuffer()[i] == color;
  return n;
}

static void testCanvas() {
  Arduino_Canvas c(466, 466);
  CHECK(c.begin());
  c.fillScreen(0);
  c.fillRect(-5, -5, 10, 10, 0xFFFF);  // clipped to 5x5
  CHECK(countColor(c, 0xFFFF) == 25);
  c.fillScreen(0);
  c.fillRect(460, 460, 100, 100, 0xFFFF);  // via the opposite corner
  CHECK(countColor(c, 0xFFFF) == 36);

  c.fillScreen(0);
  c.fillCircle(233, 233, 100, 0xF800);
  uint16_t *fb = c.framebuffer();
  CHECK(fb[233 * 466 + 233] == 0xF800);
  CHECK(fb[233 * 466 + 133] == 0xF800);  // left edge
  CHECK(fb[233 * 466 + 132] == 0);
  CHECK(fb[133 * 466 + 133] == 0);       // the square's corner falls outside
  int area = countColor(c, 0xF800);
  CHECK(area > 31000 && area < 31800);   // pi * r^2 = 31416

  c.fillScreen(0);
  c.fillRoundRect(10, 10, 100, 50, 12, 0x07E0);
  CHECK(fb[10 * 466 + 10] == 0);          // rounded corner
  CHECK(fb[35 * 466 + 10] == 0x07E0);     // left side
  CHECK(fb[35 * 466 + 109] == 0x07E0);    // right side, inclusive
  CHECK(fb[35 * 466 + 110] == 0);

  c.fillScreen(0);
  c.fillTriangle(100, 100, 200, 100, 150, 200, 0x001F);
  CHECK(fb[100 * 466 + 150] == 0x001F);
  CHECK(fb[200 * 466 + 150] == 0x001F);
  CHECK(fb[201 * 466 + 150] == 0);

  // classic text: pixel by pixel against the font table, and the same measurement textW() uses
  c.fillScreen(0);
  c.setTextColor(0xFFFF);
  c.setTextSize(2);
  c.setCursor(50, 60);
  c.print("A");
  for (int col = 0; col < 5; col++)
    for (int row = 0; row < 8; row++) {
      bool on = (GLCD_FONT['A' * 5 + col] >> row) & 1;
      CHECK((fb[(60 + row * 2) * 466 + 50 + col * 2] != 0) == on);
    }
  CHECK(c.getCursorX() == 62);
  int16_t x1, y1;
  uint16_t w, h;
  c.getTextBounds("ABC", 0, 0, &x1, &y1, &w, &h);
  CHECK(w == 36 && h == 16 && y1 == 0);
}

static void testU8g2() {
  std::vector<uint8_t> ja = readFile("fonts/unifont_t_japanese3.u8g2");
  std::vector<uint8_t> ko = readFile("fonts/unifont_t_korean2.u8g2");
  CHECK(!ja.empty() && !ko.empty());
  if (ja.empty() || ko.empty()) return;
  Arduino_Canvas c(466, 466);
  c.begin();
  c.setTextColor(0xFFFF);
  c.setFont(ja.data());
  c.setUTF8Print(true);
  int16_t x1, y1;
  uint16_t w, h;
  c.setTextSize(1);
  c.getTextBounds("A", 0, 0, &x1, &y1, &w, &h);
  CHECK(y1 < 0);  // anchored at the baseline: applyLangFont() measures the ascent this way
  CHECK(w > 0 && h > 0);
  c.setCursor(100, 100);
  c.print("\xE3\x81\x82");  // kana "a": full width in unifont
  CHECK(c.getCursorX() == 116);
  int ink = countColor(c, 0xFFFF);
  CHECK(ink > 20 && ink < 16 * 16);
  c.setFont(ko.data());
  c.fillScreen(0);
  c.setCursor(100, 100);
  c.print("\xEC\x8B\x9C");  // hangul "si"
  CHECK(countColor(c, 0xFFFF) > 10);

  // every Japanese and Korean text draws all its characters: both fonts are subsets of unifont
  // (korean2 most of all), and a missing one is skipped without a trace
  c.setCursor(100, 100);
  c.print("\xE0\xB8\x81");  // a Thai letter, in neither font: drawn as nothing
  CHECK(c.getCursorX() == 100);
  int missing = 0;
  for (Lang l : { LANG_JA, LANG_KO }) {
    c.setFont(l == LANG_JA ? ja.data() : ko.data());
    gLang = l;
    std::vector<const char *> texts;
    for (int id = 0; id < STR_COUNT; id++) texts.push_back(T((StrId)id));
    for (int m = 0; m < MED_COUNT; m++) {
      texts.push_back(medalName(m));
      texts.push_back(medalLabel(m));
      texts.push_back(medalDesc(m));
      texts.push_back(medalHow(m));
    }
    for (const char *t : texts) {
      for (const char *p = t; *p;) {
        int n = (*p & 0x80) == 0 ? 1 : (*p & 0xE0) == 0xC0 ? 2 : (*p & 0xF0) == 0xE0 ? 3 : 4;
        std::string ch(p, n);
        p += n;
        if (ch == " " || ch == "\n") continue;
        if (n == 4) {  // beyond the fonts' reach (an emoji)
          missing++;
          continue;
        }
        c.setCursor(100, 100);
        c.print(ch.c_str());
        if (c.getCursorX() == 100) {
          if (missing++ < 10) printf("  no glyph for \"%s\" in: %s\n", ch.c_str(), t);
        }
      }
    }
  }
  gLang = LANG_DEFAULT;
  CHECK(missing == 0);

  c.setFont();
  c.setUTF8Print(false);
  c.setCursor(0, 0);
  c.print("A");
  CHECK(c.getCursorX() == 6);
}

// V2's save under a name of its own (savename.h): a save from before the rename is read as the new one's and
// written under the new name at the next flush, the old file as it was; a new file already there wins; starting
// over empties the old one too, so it can't come back
static void testSaveRename() {
  std::string d = tempDir(), oldF = d + "/" SAVE_NS_OLD ".prefs", newF = d + "/" SAVE_NS_NEW ".prefs";
  FILE *f = fopen(oldF.c_str(), "w");
  fputs("lvl 2a\nnm 41736800\n", f);
  fclose(f);
  std::vector<uint8_t> before = readFile(oldF);
  hostPrefsSetDir(d.c_str());
  saveMigrate();
  {
    Preferences p;
    p.begin(SAVE_NS, true);
    CHECK(p.getUChar("lvl") == 0x2a);
    p.end();
  }
  CHECK(readFile(newF).empty());  // (to disk at the flush)
  hostPrefsFlush();
  CHECK(readFile(newF) == before);
  CHECK(readFile(oldF) == before);
  {
    Preferences p;
    p.begin(SAVE_NS);
    p.putUChar("lvl", 7);
    p.end();
  }
  hostPrefsFlush();
  hostPrefsSetDir(d.c_str());  // the next start
  saveMigrate();
  {
    Preferences p;
    p.begin(SAVE_NS, true);
    CHECK(p.getUChar("lvl") == 7);
    p.end();
  }
  saveForgetOld();
  hostPrefsFlush();
  CHECK(readFile(oldF).empty());
  CHECK(!readFile(newF).empty());
  unlink(oldF.c_str());
  unlink(newF.c_str());
  rmdir(d.c_str());
  hostPrefsSetDir(nullptr);
}

static void testPrefs() {
  std::string dir = tempDir();
  hostPrefsSetDir(dir.c_str());
  {
    Preferences p;
    p.begin("ns");
    p.putUChar("uc", 200);
    p.putShort("s", -5);
    p.putUInt("ui", 4000000000u);
    p.putBool("b", true);
    p.putString("str", "hey!");
    const uint8_t raw[3] = {1, 0, 255};
    p.putBytes("raw", raw, 3);
    p.putBytes("nothing", raw, 0);
    p.end();
    Preferences ro;
    ro.begin("ns", true);
    CHECK(ro.putUChar("uc", 1) == 0);  // read-only
    CHECK(ro.getUChar("uc") == 200);
    ro.end();
  }
  CHECK(readFile(dir + "/ns.prefs").empty());  // to disk on flush, not on every put
  hostPrefsFlush();
  CHECK(!readFile(dir + "/ns.prefs").empty());
  CHECK(readFile(dir + "/ns.prefs.tmp").empty());

  hostPrefsSetDir(dir.c_str());  // like a fresh start
  {
    Preferences p;
    p.begin("ns");
    CHECK(p.getUChar("uc") == 200);
    CHECK(p.getShort("s") == -5);
    CHECK(p.getUInt("ui") == 4000000000u);
    CHECK(p.getBool("b"));
    char s[8];
    CHECK(p.getString("str", s, sizeof s) == 4 && strcmp(s, "hey!") == 0);
    uint8_t raw[4] = {9, 9, 9, 9};
    CHECK(p.getBytes("raw", raw, 4) == 3 && raw[0] == 1 && raw[1] == 0 && raw[2] == 255 && raw[3] == 9);
    CHECK(p.isKey("nothing"));
    CHECK(p.getUChar("no-such-key", 7) == 7);
    CHECK(p.getUShort("uc", 7) == 7);  // different size: the default value
    CHECK(p.remove("uc") && !p.isKey("uc"));
    p.clear();
    CHECK(!p.isKey("s"));
    p.end();
  }
  hostPrefsFlush();
  hostPrefsSetDir(dir.c_str());
  {
    Preferences p;
    p.begin("ns");
    CHECK(!p.isKey("s"));  // the removal reaches disk too
    p.end();
  }
  hostPrefsSetDir(nullptr);
  removeDir(dir);
}

// a UDP socket on loopback, port chosen by the system
static int loopSock(uint16_t &port) {
  int s = socket(AF_INET, SOCK_DGRAM, 0);
  sockaddr_in a = {};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  socklen_t len = sizeof(a);
  if (s < 0 || bind(s, (sockaddr *)&a, sizeof(a)) != 0 || getsockname(s, (sockaddr *)&a, &len) != 0) return -1;
  timeval tv = {1, 0};  // a lost packet fails the test instead of hanging it
  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  port = ntohs(a.sin_port);
  return s;
}

static void sendLoop(int s, uint16_t port, const void *data, size_t len) {
  sockaddr_in a = {};
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  sendto(s, data, len, 0, (sockaddr *)&a, sizeof(a));
}

// what the host takes out of buddyNetPoll() within a second
static bool pollFor(BuddyMsg &m) {
  for (int i = 0; i < 100; i++) {
    if (buddyNetPoll(m)) return true;
    usleep(10000);
  }
  return false;
}

// the host's buddy radio against a socket playing a Knomi, on loopback so nothing leaves the machine
static void testBuddy() {
  std::string dir = tempDir();
  hostPrefsSetDir(dir.c_str());
  uint32_t me = buddyNetId();
  CHECK(me != 0);
  CHECK(buddyNetId() == me);  // stable
  {
    Preferences p;
    p.begin("buddy", true);
    CHECK(p.getUInt("id") == me);  // kept with the save
    p.end();
  }
  hostPrefsFlush();
  CHECK(!readFile(dir + "/buddy.prefs").empty());

  uint16_t knomiPort = 0, hostPort = 0;
  int knomi = loopSock(knomiPort);
  int spare = loopSock(hostPort);  // only to get a free port for the host
  CHECK(knomi >= 0 && spare >= 0);
  close(spare);
  hostBuddyNetConfig(true, hostPort, "127.0.0.1", knomiPort);
  CHECK(buddyNetUp());

  // the host greets: the Knomi gets the same 65 bytes and understands them
  BuddyMsg hello;
  buddyInit(hello, BM_HELLO, me, 0, 0);
  hello.dex = 25;
  hello.level = 12;
  strcpy(hello.nick, "Sparky");
  hello.reg[3] = 0x81;
  buddyNetSend(hello);
  uint8_t buf[128];
  ssize_t got = recv(knomi, buf, sizeof(buf), 0);
  CHECK(got == 65);
  CHECK(memcmp(buf, "PKG1", 4) == 0 && buf[4] == BUDDY_VER && buf[5] == BM_HELLO);
  CHECK(buf[6] == (me & 0xff) && buf[9] == (me >> 24));  // little-endian, as on the ESP32
  CHECK(buf[22] == 25 && buf[23] == 0 && buf[24] == 12);
  BuddyMsg back;
  CHECK(got > 0 && buddyParse(buf, (size_t)got, back));
  CHECK(back.from == me && back.dex == 25 && strcmp(back.nick, "Sparky") == 0 && back.reg[3] == 0x81);

  // the Knomi answers; garbage, echoes of our own and messages for others are dropped
  const uint32_t knomiId = 0x12345679;
  sendLoop(knomi, hostPort, "garbage", 7);
  BuddyMsg echo = hello;
  sendLoop(knomi, hostPort, &echo, sizeof(echo));
  BuddyMsg other;
  buddyInit(other, BM_INVITE, knomiId, me + 2, 7);
  other.kind = BK_BATTLE;
  sendLoop(knomi, hostPort, &other, sizeof(other));
  BuddyMsg invite = other;
  invite.to = me;
  sendLoop(knomi, hostPort, &invite, sizeof(invite));
  BuddyMsg in;
  CHECK(pollFor(in));
  CHECK(in.type == BM_INVITE && in.from == knomiId && in.to == me && in.session == 7 && in.kind == BK_BATTLE);
  CHECK(!buddyNetPoll(in));

  // with the buddy known, replies still reach it
  buddyNetSetPeer(knomiId);
  sendLoop(knomi, hostPort, &invite, sizeof(invite));
  CHECK(pollFor(in));
  BuddyMsg accept;
  buddyInit(accept, BM_ACCEPT, me, knomiId, 7);
  buddyNetSend(accept);
  got = recv(knomi, buf, sizeof(buf), 0);
  CHECK(got == 65 && buddyParse(buf, (size_t)got, back) && back.type == BM_ACCEPT && back.to == knomiId);
  buddyNetSetPeer(0);

  hostBuddyNetConfig(false, 0, nullptr, 0);  // the rest of the tests stay off the network
  CHECK(!buddyNetUp());
  close(knomi);
  hostPrefsSetDir(nullptr);
  removeDir(dir);
}

// the relay: what a front end hands over reaches the game, and nothing goes out while alone
static std::vector<std::vector<uint8_t> > gRelayOut;
static void relayOut(const uint8_t *d, int n) { gRelayOut.push_back(std::vector<uint8_t>(d, d + n)); }

static void testRelay() {
  hostBuddyNetConfig(false, 0, nullptr, 0);  // UDP off: only the relay
  CHECK(!buddyNetUp());
  hostRelayConnected(relayOut);
  CHECK(buddyNetUp());
  uint32_t me = buddyNetId();
  BuddyMsg hello;
  buddyInit(hello, BM_HELLO, buddyNetId(), 0, 0);
  hello.dex = 25;
  hello.level = 5;
  buddyNetSend(hello);
  CHECK(gRelayOut.empty());  // alone: nothing sent
  hostRelayPeers(1);
  buddyNetSend(hello);
  CHECK(gRelayOut.size() == 1 && gRelayOut[0].size() == 65);

  BuddyMsg in, got;
  buddyInit(in, BM_HELLO, 0x1234, 0, 0);
  in.dex = 7;
  in.level = 9;
  hostRelayReceived((const uint8_t *)&in, sizeof(in));
  CHECK(buddyNetPoll(got) && got.from == 0x1234 && got.dex == 7);
  buddyInit(in, BM_INVITE, 0x1234, me + 2, 3);  // for someone else: not ours
  in.kind = BK_BATTLE;
  hostRelayReceived((const uint8_t *)&in, sizeof(in));
  hostRelayReceived((const uint8_t *)"junk", 4);
  CHECK(!buddyNetPoll(got));
  buddyInit(in, BM_HELLO, me, 0, 0);  // our own echo
  in.dex = 1;
  in.level = 1;
  hostRelayReceived((const uint8_t *)&in, sizeof(in));
  CHECK(!buddyNetPoll(got));

  hostRelayConnected(nullptr);  // dropped: off again, and nothing more goes out
  CHECK(!buddyNetUp());
  buddyNetSend(hello);
  CHECK(gRelayOut.size() == 1);
}

static bool anySound(int frames) {
  std::vector<int16_t> pcm((size_t)frames);
  hostAudioRender(pcm.data(), frames);
  for (int16_t s : pcm)
    if (s != 0) return true;
  return false;
}

static void testAudio() {
  hostPrefsSetDir(nullptr);
  audioSetEnabled(true);
  audioSetSleeping(false);
  CHECK(!anySound(256));
  sfxPlay(SFX_TAP);          // V2: four notes (132 ms), each in tune: its pitch from its zero crossings
  {
    int ms = 0;
    for (int i = 0; i < SFX[SFX_TAP].len; i++) ms += SFX[SFX_TAP].n[i].ms;
    std::vector<int16_t> pcm((size_t)(HOST_AUDIO_RATE * ms / 1000));
    hostAudioRender(pcm.data(), (int)pcm.size());
    int at = 0;
    for (int i = 0; i < SFX[SFX_TAP].len; i++) {
      const Note &n = SFX[SFX_TAP].n[i];
      int len = HOST_AUDIO_RATE * n.ms / 1000, z = 0;
      for (int k = at + 1; k < at + len; k++) z += (pcm[k - 1] < 0) != (pcm[k] < 0);
      CHECK(fabsf(z * (float)HOST_AUDIO_RATE / (2.0f * len) - n.f) < n.f * 0.04f);
      at += len;
    }
  }
  CHECK(!anySound(256));     // and it ends
  sfxPlay(SFX_COUNT);        // out of range
  CHECK(!anySound(256));
  audioSetEnabled(false);
  CHECK(!hostSoundEnabled());  // the Mac's Mute Sound item reads the game's own setting
  sfxPlay(SFX_TAP);
  CHECK(!anySound(256));
  hostSetSoundEnabled(true);
  CHECK(audioEnabled());
  hostSetSoundEnabled(false);
  audioSetEnabled(true);
  audioSetSleeping(true);    // no sound while asleep, as on the board
  sfxPlay(SFX_TAP);
  CHECK(!anySound(256));
  audioSetSleeping(false);
}

// the clock: the computer's until a time server answers, then the server's (setting the computer's
// clock changes nothing after that)
static void testClock() {
  uint32_t os = (uint32_t)time(nullptr);
  CHECK(!clockFromNetwork() && clockUtc() == 0);
  int32_t off = (int32_t)(rtcEpoch() - os);  // the computer's time zone
  CHECK(off > -15 * 3600 && off < 15 * 3600);
  CHECK(hostNetTimeWanted());   // asked at start...
  CHECK(!hostNetTimeWanted());  // ...once
  hostNetTime(0);               // no answer
  CHECK(clockPoll() == CLOCK_NONE && !clockFromNetwork());
  hostNetTime(12345);           // nonsense: ignored
  CHECK(clockPoll() == CLOCK_NONE && !clockFromNetwork());
  uint32_t server = os + 3 * 3600;  // the server says three hours later: the computer's clock was set back
  hostNetTime(server);
  CHECK(clockPoll() == CLOCK_TRUSTED);
  CHECK(clockFromNetwork());
  CHECK(clockUtc() >= server && clockUtc() <= server + 2);
  int32_t local = (int32_t)(rtcEpoch() - clockUtc());
  CHECK(local > -15 * 3600 && local < 15 * 3600);  // shown in the computer's time zone
  CHECK(clockPoll() == CLOCK_NONE);
  CHECK(!hostNetTimeWanted());  // no need to ask again yet
}

static void runTicks(int ms) {
  for (int i = 0; i < ms / 5; i++) {
    hostTick();
    usleep(5000);
  }
}

// the real game, from start to finish
extern bool gamesMenuOpen, gameOpen, runOpen;
enum ZoneScreen : uint8_t { ZS_NONE, ZS_PICK, ZS_ZONE };  // (the sketch's)
extern uint32_t runOverT;
extern int8_t introCard;
extern uint32_t lastInteract;

static void testGame() {
  std::string res = tempDir(), save = tempDir(), cwd = getcwd(nullptr, 0);
  CHECK(symlink((cwd + "/../tools/sdcard/mons").c_str(), (res + "/mons").c_str()) == 0);
  CHECK(symlink((cwd + "/fonts").c_str(), (res + "/fonts").c_str()) == 0);
  std::string saveSub = save + "/a/b";  // also creates the intermediate folders
  hostInit(res.c_str(), saveSub.c_str());
  CHECK(!readFile(saveSub + "/" SAVE_NS_NEW ".prefs").empty());

  runTicks(200);
  CHECK(hostFrameDirty());
  std::vector<uint8_t> rgba(HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4);
  hostFrameRGBA(rgba.data());
  CHECK(!hostFrameDirty());
  CHECK(rgba[3] == 255);
#if !defined(UI_V2) || UI_V2
  {  // V2: the four starters walk through the Field; a tap shorter than the game's polling interval is not lost:
     // Bulbasaur, then the question's YES, and it's yours at once
    extern Pet pet;
    CHECK(pet.awaitingStarter());
    hostPointer(100, 280, true);
    hostPointer(100, 280, false);
    int16_t x, y;
    CHECK(hostPointerRead(&x, &y) && x == 100 && y == 280);
    runTicks(150);
    CHECK(!hostPointerRead(&x, &y));
    runTicks(400);  // the question slides up
    hostPointer(169, 386, true);
    runTicks(100);
    hostPointer(169, 386, false);
    runTicks(300);
    CHECK(!pet.awaitingStarter() && !pet.isEgg() && pet.speciesId == 1);
    CHECK(hostFrameDirty());
  }
#else
  int center = (HOST_CANVAS_SIZE * HOST_CANVAS_SIZE / 2 + HOST_CANVAS_SIZE / 2) * 4;
  CHECK(rgba[center] > 200 && rgba[center + 1] > 200);  // the light background of the starter choice

  // a tap shorter than the game's polling interval is not lost
  hostPointer(233, 145, true);
  hostPointer(233, 145, false);
  int16_t x, y;
  CHECK(hostPointerRead(&x, &y) && x == 233 && y == 145);
  runTicks(150);
  CHECK(!hostPointerRead(&x, &y));
  runTicks(150);  // chose Bulbasaur: the background becomes the egg's
  CHECK(hostFrameDirty());
#endif

  // the arrow keys are a ramp, with the sign playerTilt() expects
  hostTiltKeys(false, true);
  runTicks(250);
  CHECK(hostTiltValue() == 1.0f);
  KnomiAccel accel;
  float g[3];
  CHECK(accel.read(g) && g[KNOMI_TILT_AXIS] * KNOMI_TILT_SIGN == 0.5f);
  hostTiltKeys(true, false);
  runTicks(250);
  CHECK(hostTiltValue() == -1.0f);
  hostTiltKeys(false, false);
  runTicks(150);
  CHECK(hostTiltValue() == 0.0f);

  // all four arrows give a step's direction, the last one pressed winning (PokeDungeon)
  int8_t dx = 0, dy = 0;
  CHECK(!hostArrowDir(&dx, &dy));
  hostArrowKeys(true, false);
  CHECK(hostArrowDir(&dx, &dy) && dx == 0 && dy == -1);
  hostTiltKeys(true, false);  // left too, pressed later: left
  CHECK(hostArrowDir(&dx, &dy) && dx == -1 && dy == 0);
  hostTiltKeys(false, false);  // left let go: back to up, still held
  CHECK(hostArrowDir(&dx, &dy) && dx == 0 && dy == -1);
  hostArrowKeys(false, true);
  CHECK(hostArrowDir(&dx, &dy) && dx == 0 && dy == 1);
  hostArrowKeys(false, false);
  CHECK(!hostArrowDir(&dx, &dy));

  hostFlushSave();
  std::vector<uint8_t> saved = readFile(saveSub + "/" SAVE_NS_NEW ".prefs");
  std::string text(saved.begin(), saved.end());
  CHECK(text.find("spe ") != std::string::npos || text.find("sp ") != std::string::npos);
  CHECK(!hostRestartRequested());
  // a game opens under its how-to-play card: the text does nothing, Ready, GO! sends it off and the game
  // starts; a swipe sends it off too, and doesn't quit the game
  auto tapAt = [](int16_t x, int16_t y) {
    hostPointer(x, y, true);
    runTicks(150);
    hostPointer(x, y, false);
    runTicks(300);
  };
  hostSerialPush("NOSAVE");
  runTicks(100);
  hostSerialPush("SPEC 5");
  runTicks(100);
  hostSerialPush("AWAKE");
  runTicks(100);
  hostSerialPush("GAMES");
  runTicks(300);
  CHECK(gamesMenuOpen && introCard == -1);
  tapAt(150, 128);  // Berry Drop's tile (V2: its cartridge, which stops at 166)
  CHECK(gameOpen && !gamesMenuOpen && introCard == 0);
  runTicks(500);    // landed
  tapAt(233, 240);  // the text
  CHECK(gameOpen && introCard == 0);
  tapAt(233, 335);  // Ready, GO!
  runTicks(400);    // off it goes
  CHECK(gameOpen && introCard == -1);
  hostSerialPush("GAMES");
  runTicks(300);
  gameOpen = false;  // (Berry Drop, left: the menu opens over it in this test)
  tapAt(150, 128);
  runTicks(500);
  CHECK(gameOpen && introCard == 0);
  hostPointer(233, 380, true);  // a swipe up from the bottom: it would quit a game, here it sends the card
  for (int yy = 380; yy >= 200; yy -= 30) {
    hostPointer(233, yy, true);
    runTicks(20);
  }
  hostPointer(233, 200, false);
  runTicks(500);
  CHECK(gameOpen && introCard == -1);

  // a game that ends goes back to the games menu, not home: PokeRun, with no jumps, ends on the first
  // obstacle; a tap on the result closes it (V2: its X; PLAY AGAIN! runs it again)
  gameOpen = false;
  hostSerialPush("GAMES");
  runTicks(300);
  tapAt(316, 128);  // PokeRun's tile (V2: its cartridge, from 299)
  runTicks(500);
  tapAt(233, 335);  // Ready, GO!
  for (int i = 0; i < 40 && !runOverT; i++) runTicks(200);  // until it has ended
  runTicks(1000);
  CHECK(runOpen && runOverT);  // the result, waiting for a tap
#if !defined(UI_V2) || UI_V2
  tapAt(233, 200);  // V2: a tap on its panel does nothing
  CHECK(runOpen && runOverT);
  tapAt(233, 287);  // PLAY AGAIN!: a new run at once
  CHECK(runOpen && !runOverT && introCard == -1);
  for (int i = 0; i < 40 && !runOverT; i++) runTicks(200);
  runTicks(1000);
  CHECK(runOpen && runOverT);
  {  // the Repel row under it: the game closed, the card on the Repel's page (Fedor: "Clicking on repel progress
     // after a mini game should take you to the repel screen")
    extern bool cardOpen, cardAch;
    extern uint8_t cardPage;
    tapAt(233, 354);
    CHECK(!runOpen && !gamesMenuOpen && cardOpen && !cardAch && cardPage == 6);
    cardOpen = false;
    cardPage = 0;
    hostSerialPush("GAMES");
    runTicks(300);
    tapAt(316, 128);  // PokeRun again
    runTicks(500);
    tapAt(233, 335);
    for (int i = 0; i < 40 && !runOverT; i++) runTicks(200);
    runTicks(1000);
    CHECK(runOpen && runOverT);
  }
  tapAt(233, 422);  // the X
#else
  tapAt(233, 200);
#endif
  CHECK(!runOpen && gamesMenuOpen);

  // auto-update: on unless turned off, and kept apart from the pet's save; an update found waits for the
  // home screen and a minute with nobody touching it
  CHECK(hostAutoUpdate());
  CHECK(!hostQuietForUpdate());  // the games menu is open
  gamesMenuOpen = false;
  tapAt(60, 233);                // a tap at the side of home
  CHECK(!hostQuietForUpdate());  // just touched
  lastInteract = millis() - 61000;
  CHECK(hostQuietForUpdate());
  hostSetAutoUpdate(false);
  CHECK(!hostAutoUpdate());
  hostFlushSave();
  std::vector<uint8_t> opts = readFile(saveSub + "/opts.prefs");
  CHECK(std::string(opts.begin(), opts.end()).find("aupd 00") != std::string::npos);
  hostSetAutoUpdate(true);

#if !defined(UI_V2) || UI_V2
  // V2's home: each icon answers at its new centre on the circle round the screen's middle, and the
  // hygiene bar on the rim is painted in its status colour from its lower end (layout_v2.h)
  {
    extern int buttonAt(int16_t x, int16_t y);
    extern Pet pet;
    extern ScaledCanvas *gfx;
    // bag, games, map, moon, drop (the numbers their taps go by), where they are now: the bag in the middle at the
    // bottom (Fedor: "Gameboy > Map > Backpack > Sleep > Bath"), so the left end is the Game Boy (V2_BTN)
    static const int16_t icon[5][2] = { {232, 408}, {90, 336}, {158, 392}, {306, 392}, {374, 338} };
    for (int i = 0; i < 5; i++) CHECK(buttonAt(icon[i][0], icon[i][1]) == i);
    CHECK(buttonAt(233, 412) == 0);  // straight down from the middle: the backpack
    CHECK(buttonAt(233, 300) == -1);  // between the pet and the band: no icon
    pet.hygiene = 100;
    runTicks(300);
    const uint16_t *fb = gfx->getFramebuffer();
    CHECK(fb[329 * 466 + 429] == 0x5DCD);  // r 218 at 26 degrees: near the bar's lower end, clear of its label
    // the scene follows the Pokemon's Travel zone, and is drawn again when that changes
    pet.speciesId = 74;  // Geodude: the Cave, the same at every hour
    runTicks(300);
    CHECK(fb[4 * 466 + 233] == 0x20E5);  // the cave's top, #201c2c
    pet.speciesId = 94;  // Gengar: the Mansion
    runTicks(300);
    CHECK(fb[4 * 466 + 233] == 0x1065);  // its night sky, #140f2c
    // the walk (walk_v2.h): once a walk starts the world's clock runs; the food picker stops it, and the
    // world holds still until it closes
    extern void v2HostStartWalk();
    extern uint32_t v2HostWalkMs();
    extern bool v2HostWalking();
    extern uint32_t feedMenuUntil;
    pet.fullness = pet.joy = pet.energy = pet.hygiene = 100;
    pet.sleeping = false;
    v2HostStartWalk();
    runTicks(300);
    CHECK(v2HostWalking());
    uint32_t walked = v2HostWalkMs();
    runTicks(300);
    CHECK(v2HostWalkMs() > walked);
    feedMenuUntil = millis() + 5000;
    runTicks(100);
    CHECK(!v2HostWalking());
    walked = v2HostWalkMs();
    runTicks(300);
    CHECK(v2HostWalkMs() == walked);
    feedMenuUntil = 0;
    runTicks(200);
    CHECK(v2HostWalking());
    // asleep, only the home is dim (Fedor): the sleep turns the brightness down at home, not over a menu
    extern PowerCtx powerCtx(uint32_t now);
    extern bool cardOpen;
    uint8_t awake = brightnessFor(powerCtx(millis()));
    pet.sleeping = true;
    runTicks(100);
    CHECK(brightnessFor(powerCtx(millis())) < awake);
    cardOpen = true;
    runTicks(100);
    CHECK(brightnessFor(powerCtx(millis())) == awake);
    cardOpen = false;
    runTicks(100);
    CHECK(brightnessFor(powerCtx(millis())) < awake);
    pet.sleeping = false;
    runTicks(100);
    // the games' X clicks, as every X does (Fedor)
    audioSetEnabled(true);
    std::vector<int16_t> drain(HOST_AUDIO_RATE * 10);
    hostAudioRender(drain.data(), (int)drain.size());  // what's queued so far, out of the way
    gamesMenuOpen = true;
    runTicks(200);
    hostAudioRender(drain.data(), (int)drain.size());
    hostPointer(233, 422, true);
    runTicks(100);
    hostPointer(233, 422, false);
    runTicks(200);
    CHECK(!gamesMenuOpen);
    CHECK(anySound(HOST_AUDIO_RATE / 10));
    // no swipes on home (Fedor: "Making the hold menu the main navigation there"): a quick swipe each way
    // opens nothing; the ring opens the pages, where a swipe still turns them
    extern bool galleryOpen, clockOpen, v2RadialOpen;
    extern int galleryPage;
    auto swipe = [](int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
      for (int i = 0; i <= 6; i++) {
        hostPointer(x0 + (x1 - x0) * i / 6, y0 + (y1 - y0) * i / 6, true);
        runTicks(30);
      }
      hostPointer(x1, y1, false);
      runTicks(300);
    };
    static const int16_t sw[4][4] = { {110, 230, 360, 230}, {360, 230, 110, 230}, {233, 150, 233, 290}, {233, 290, 233, 150} };
    for (auto &s : sw) {
      swipe(s[0], s[1], s[2], s[3]);
      CHECK(!galleryOpen && !cardOpen && !clockOpen && !v2RadialOpen);
    }
    hostPointer(233, 240, true);  // a hold on the Pokemon, let go in the ring's middle: it stays for a tap
    runTicks(700);
    hostPointer(233, 240, false);
    runTicks(200);
    CHECK(v2RadialOpen);
    tapAt(88, 206);  // the Pokedex, on the left
    CHECK(galleryOpen && !v2RadialOpen && galleryPage == 0);
    swipe(360, 230, 110, 230);  // right to left: the next page
    CHECK(galleryOpen && galleryPage == 1);
    galleryOpen = false;
    runTicks(200);
    // a Wild Encounter from Knomi Island goes back to the island, as a zone's Search goes back to the zone
    // (Fedor: "When hitting run, you should go back to the zone you were in right before you battled. Not back
    // to home screen"); the button's battle still ends at home
    {
      extern bool battleOpen, bRunFailed;
      extern ZoneScreen zoneScreen;
      extern uint8_t zoneCur;
      extern void closeBattle(), zoneSearch();
      hostSerialPush("ZONES");
      runTicks(300);
      CHECK(zoneScreen == ZS_PICK);
      tapAt(233, 230);  // the tall grass in the middle: Wild Encounter
      CHECK(battleOpen && zoneScreen == ZS_NONE);
      runTicks(4500);   // its intro, and a new move's if there is one
      tapAt(233, 412);  // RUN
      if (battleOpen && bRunFailed) closeBattle();  // (one run in 20 fails: closed as it would have)
      runTicks(1500);
      CHECK(!battleOpen && zoneScreen == ZS_PICK);
      zoneCur = 1;  // a zone's Search: back to the zone
      zoneScreen = ZS_ZONE;
      zoneSearch();
      CHECK(battleOpen && zoneScreen == ZS_NONE);
      closeBattle();
      CHECK(zoneScreen == ZS_ZONE);
      zoneScreen = ZS_NONE;
      hostSerialPush("BATTLE");  // the button's (Space on a keyboard): home
      runTicks(300);
      CHECK(battleOpen);
      closeBattle();
      CHECK(zoneScreen == ZS_NONE);
      runTicks(200);
      // the catch on a computer (Fedor: "you should be able to use space, or up arrow to throw the ball"): the up
      // arrow throws a Poke Ball; one held from before the catch doesn't; Space (the Knomi's button) does
      extern uint32_t bBallT;
      hostSerialPush("WILD 129");
      runTicks(300);
      CHECK(battleOpen);
      hostSerialPush("BTLWIN");
      runTicks(4300);  // its end, then the catch, waiting for a throw
      uint32_t t0 = bBallT;
      hostArrowKeys(true, false);
      runTicks(100);
      hostArrowKeys(false, false);
      runTicks(100);
      CHECK(bBallT != t0);
      closeBattle();
      runTicks(200);
      hostSerialPush("WILD 129");
      runTicks(300);
      hostSerialPush("BTLWIN");
      hostArrowKeys(true, false);  // held all through its end
      runTicks(4300);
      t0 = bBallT;
      runTicks(300);
      CHECK(bBallT == t0);
      hostArrowKeys(false, false);
      hostButton(true);  // Space
      runTicks(100);
      hostButton(false);
      runTicks(200);
      CHECK(battleOpen && bBallT != t0);
      closeBattle();
      runTicks(200);
      // a ball is chosen, then THROW throws it (Fedor: "clicking one selects it and there is a throw button to throw
      // it"): with an Ultra Ball held, a tap on it chooses it and throws nothing; THROW names it; the right arrow
      // chooses too; with the Poke Ball alone, a tap on it throws
      extern bool pressTargetAt(int16_t x, int16_t y, PressTarget &t);
      PressTarget pt;
      pet.ultraBalls = 1;
      hostSerialPush("WILD 129");
      runTicks(300);
      hostSerialPush("BTLWIN");
      runTicks(4300);
      t0 = bBallT;
      CHECK(pressTargetAt(233, 319, pt) && !strcmp(pt.label, "THROW POKE BALL"));
      tapAt(130, 405);  // the Ultra Ball, left of the Poke Ball
      CHECK(bBallT == t0 && pressTargetAt(233, 319, pt) && !strcmp(pt.label, "THROW ULTRA BALL"));
      hostTiltKeys(false, true);  // the right arrow: the Poke Ball again
      runTicks(100);
      hostTiltKeys(false, false);
      runTicks(100);
      CHECK(bBallT == t0 && pressTargetAt(233, 319, pt) && !strcmp(pt.label, "THROW POKE BALL"));
      tapAt(233, 319);  // THROW
      CHECK(bBallT != t0);
      closeBattle();
      pet.ultraBalls = 0;
      runTicks(200);
      hostSerialPush("WILD 129");
      runTicks(300);
      hostSerialPush("BTLWIN");
      runTicks(4300);
      t0 = bBallT;
      tapAt(233, 405);  // the Poke Ball alone: thrown
      CHECK(bBallT != t0);
      closeBattle();
      runTicks(200);
      // Settings, tidied, and About (Fedor: "an about page with my logo on it ... a way to get to the repo but also my
      // youtube channel"): the Sound and Language rows; the next page is About; its links open in the browser, and
      // with nothing to open them the link's QR code shows (as on the Knomi), a tap puts it away; the X closes
      {
        extern bool clockOpen;
        extern uint8_t clockPage;
        extern void applyLangFont();
        static std::string opened;
        clockOpen = true;
        clockPage = 0;
        runTicks(300);
        bool snd = audioEnabled();
        tapAt(328, 232);  // the Sound row's switch
        CHECK(audioEnabled() != snd);
        audioSetEnabled(snd);
        Lang was = gLang;
        tapAt(290, 280);  // the Language row's button: the next language
        CHECK(gLang == (Lang)((was + 1) % LANG_COUNT));
        setLang(was);
        applyLangFont();
        tapAt(436, 228);  // the side button: About
        CHECK(clockOpen && clockPage == 1);
        hostSetOpenUrl([](const char *u) { opened = u; });
        tapAt(164, 340);  // GITHUB
        CHECK(opened == "https://github.com/FedorSosnin/PokeKnomiGochi");
        tapAt(302, 340);  // YOUTUBE
        CHECK(opened == "https://youtube.com/3dprintsos");
        hostSetOpenUrl(nullptr);
        CHECK(pressTargetAt(164, 340, pt) && !strcmp(pt.label, "GITHUB"));
        tapAt(164, 340);  // nothing to open it with: its code, and the page under it takes no presses
        CHECK(!pressTargetAt(164, 340, pt));
        tapAt(233, 200);  // put away
        CHECK(pressTargetAt(164, 340, pt));
        tapAt(233, 422);  // the X
        CHECK(!clockOpen);
        runTicks(200);
      }
      {  // a menu's page turning slides (Fedor: "make the left and right arrows through the menus 'slide' the menu
         // screens over"): the card's Training page, the side button: for a moment neither page, the page line already
         // the new one's; then the Progress page, still; a swipe back slides too (slide_v2.h)
        extern bool cardOpen, cardAch;
        extern uint8_t cardPage;
        extern ScaledCanvas *gfx;
        const uint16_t *fb = gfx->getFramebuffer();
        auto grab = [&]() { return std::vector<uint16_t>(fb, fb + 466 * 466); };
        auto rows = [](const std::vector<uint16_t> &a, const std::vector<uint16_t> &b, int y0, int y1) {
          return std::equal(a.begin() + y0 * 466, a.begin() + y1 * 466, b.begin() + y0 * 466);
        };
        // every frame for ms after a turn, so the slide shows in one of them however slow the machine (on Linux CI a
        // frame taken at a fixed 120 ms had already landed)
        auto watch = [&](int ms) {
          std::vector<std::vector<uint16_t>> seen;
          for (int i = 0; i < ms / 5; i++) {
            hostTick();
            usleep(5000);
            seen.push_back(grab());
          }
          return seen;
        };
        auto between = [&](const std::vector<std::vector<uint16_t>> &seen, const std::vector<uint16_t> &a,
                           const std::vector<uint16_t> &b) {
          for (auto &f : seen)
            if (!rows(f, a, 0, 377) && !rows(f, b, 0, 377)) return true;
          return false;
        };
        cardOpen = true;
        cardAch = false;
        cardPage = 2;
        runTicks(400);
        std::vector<uint16_t> from = grab();
        hostPointer(436, 228, true);  // the side button (tapAt() would wait past the slide)
        runTicks(150);
        hostPointer(436, 228, false);
        auto seen = watch(800);
        std::vector<uint16_t> to = seen.back();
        CHECK(cardPage == 3);
        CHECK(between(seen, from, to));  // on the way: neither page
        bool lineFirst = false;  // "Page 4 of 7" while the page is still on its way
        for (auto &f : seen) lineFirst |= rows(f, to, 377, 466) && !rows(f, to, 0, 377);
        CHECK(lineFirst && !rows(from, to, 377, 466));
        runTicks(300);
        CHECK(grab() == to);  // landed, and still
        for (int i = 0; i <= 6; i++) {  // a swipe right: back
          hostPointer(100 + 280 * i / 6, 300, true);
          runTicks(30);
        }
        hostPointer(380, 300, false);
        seen = watch(800);
        CHECK(cardPage == 2);
        CHECK(between(seen, to, from));
        CHECK(rows(seen.back(), from, 0, 466));  // the Training page again, as it was
        cardOpen = false;
        cardPage = 0;
        runTicks(200);
      }
      {  // yours from behind in a battle, facing the foe (Fedor: "like in pokemon blue"): Charizard's pack has its
         // back views, standing, attacking and hurt (sdmon.h; tools/pack_pmd.py)
        PmdMon m;
        CHECK(m.load(6) && m.has(PMD_IDLEUR) && m.has(PMD_ATTACKUR) && m.has(PMD_HURTUR) && m.has(PMD_WALKDL));
      }
      // no swipe out of a battle (Fedor: "you can swipe out of a battle. Which means you can cancel out a run")
      hostSerialPush("WILD 129");
      runTicks(300);
      CHECK(battleOpen);
      swipe(233, 400, 233, 200);  // up from the lower half: it quits a game, and it did a battle
      CHECK(battleOpen);
      closeBattle();
      runTicks(200);
    }
  }
  // V2's game backgrounds, by day, as their mockups have them (games_px.py) where the game stands things
  {
    extern void v2HostBackdrop(int which, int tod);
    extern ScaledCanvas *gfx;
    const uint16_t *fb = gfx->getFramebuffer();
    v2HostBackdrop(0, 0);
    CHECK(fb[200 * 466 + 20] == 0xA388);   // Berry Drop: the left trunk's lit edge, #a07040
    v2HostBackdrop(1, 0);
    CHECK(fb[340 * 466 + 100] == 0x7DE9);  // PokeRun: the track's lit grass on the game's ground line, #79bd4f
    v2HostBackdrop(2, 0);
    CHECK(fb[60 * 466 + 100] == 0x2BD9);   // PikaSurf: the deep water, #2f78c8
    v2HostBackdrop(3, 0);
    CHECK(fb[300 * 466 + 66] == 0xFFFF);   // Volley: the side line on the court's left wall
    v2HostBackdrop(4, 0);
    CHECK(fb[200 * 466 + 233] == 0x8E5E);  // Who's That?: the burst's light middle, #8cc8f4
    v2HostBackdrop(6, 0);
    CHECK(fb[182 * 466 + 330] == 0x8651);  // a wild battle: the foe's base under its feet, #86c98a
    runTicks(100);
  }
#endif

  hostSerialPush("WIPE");
  runTicks(50);
  CHECK(hostRestartRequested());

  unlink((res + "/mons").c_str());
  unlink((res + "/fonts").c_str());
  rmdir(res.c_str());
  removeDir(saveSub);
  rmdir((save + "/a").c_str());
  rmdir(save.c_str());
}

// the CRT screen (host_crt.h): its curved glass maps the middle to the middle, the rim past the picture (the tube's
// black edge), taps the same way the picture goes; drawn, a lit canvas lights the middle and leaves the rim dark
static void testCrt() {
  float cx, cy;
  CHECK(hostCrtPoint(500, 500, 1000, cx, cy) && fabsf(cx - 233) < 0.01f && fabsf(cy - 233) < 0.01f);
  const float d0 = 200 * 466.0f / 1000;  // 200 of 1000 screen pixels from the middle, in the canvas's pixels unbent
  CHECK(hostCrtPoint(300, 500, 1000, cx, cy) && cx > 233 - d0 * 0.97f && cx < 233 - d0 * 0.88f);  // magnified in the middle
  CHECK(!hostCrtPoint(998, 500, 1000, cx, cy) && cx >= 466);  // the rim: off the picture
  float ax, ay, bx, by;
  hostCrtPoint(250, 250, 500, ax, ay);  // the same place on any size
  hostCrtPoint(700, 700, 1400, bx, by);
  CHECK(fabsf(ax - bx) < 0.01f && fabsf(ay - by) < 0.01f);
  static uint8_t canvas[HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4];
  for (int i = 0; i < HOST_CANVAS_SIZE * HOST_CANVAS_SIZE; i++) canvas[i * 4] = 200, canvas[i * 4 + 1] = 180, canvas[i * 4 + 2] = 120;
  const int side = 300;
  std::vector<uint8_t> out(side * side * 4);
  hostCrtPrepare(canvas, side);
  hostCrtRows(out.data(), side * 4, false, 0, side / 2, 1);  // in two halves, as on two threads
  hostCrtRows(out.data(), side * 4, false, side / 2, side, 1);
  auto at = [&](int x, int y, int c) { return out[(y * side + x) * 4 + c]; };
  int mid = 0;
  for (int y = 140; y < 160; y++) mid = mid > at(150, y, 0) ? mid : at(150, y, 0);
  CHECK(mid > 150);                                   // lit in the middle
  CHECK(at(1, 150, 0) < 30 && at(150, 1, 1) < 30);   // the tube's black edge
  std::vector<uint8_t> bgr(side * side * 4);
  hostCrtRows(bgr.data(), side * 4, true, 140, 141, 1);
  CHECK(bgr[(140 * side + 150) * 4 + 2] == out[(140 * side + 150) * 4 + 0]);  // BGRA swaps red and blue only
  // Lite: the glass bulges half as much, so a point off the middle lands nearer where it would unbent
  float lx, ly;
  hostCrtPoint(300, 500, 1000, cx, cy);
  CHECK(hostCrtPoint(300, 500, 1000, lx, ly, true) && lx < cx && lx > 233 - d0 * 1.0f && fabsf(ly - 233) < 0.01f);
  CHECK(hostCrtPoint(500, 500, 1000, lx, ly, true) && fabsf(lx - 233) < 0.01f);
  // whites and light greys stay apart (2.1.2 glowed both to nearly 255): white on the left, the HP bar's beige on
  // the right; the beige stays darker than the white and keeps its tint
  for (int i = 0; i < HOST_CANVAS_SIZE * HOST_CANVAS_SIZE; i++) {
    bool white = i % HOST_CANVAS_SIZE < HOST_CANVAS_SIZE / 2;
    canvas[i * 4] = white ? 255 : 222, canvas[i * 4 + 1] = white ? 255 : 211, canvas[i * 4 + 2] = white ? 255 : 189;
  }
  for (int lite = 0; lite < 2; lite++) {
    hostCrtPrepare(canvas, side, lite);
    hostCrtRows(out.data(), side * 4, false, 0, side, 1);
    double sum[2][3] = {};
    for (int y = 120; y < 180; y++)
      for (int x = 0; x < 60; x++)
        for (int c = 0; c < 3; c++) sum[0][c] += at(60 + x, y, c), sum[1][c] += at(180 + x, y, c);
    auto luma = [&](int h) { return (0.299 * sum[h][0] + 0.587 * sum[h][1] + 0.114 * sum[h][2]) / 3600; };
    CHECK(luma(0) > 225 && luma(0) - luma(1) > 15);
    CHECK((sum[1][0] - sum[1][2]) / 3600 > 30);  // still beige, not white
  }
}

int main() {
  testCanvas();
  testCrt();
  testU8g2();
  testPrefs();
  testSaveRename();
  testBuddy();
  testRelay();
  testAudio();
  testClock();
  testGame();
  printf("host layer: %d checks, %d failures\n", gChecks, gFails);
  return gFails ? 1 : 0;
}
