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
#include "U8g2lib.h"
#include "buddy_net.h"
#include "audio.h"
#include "savename.h"
#include "sfx_notes.h"
#include <math.h>
#include "glcdfont.h"
#include "host.h"
#include "host_hal.h"
#include "host_internal.h"
#include "rtcbat.h"
#include "i18n.h"
#include "pet.h"
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
    static const int16_t icon[5][2] = { {94, 340}, {160, 394}, {230, 408}, {308, 394}, {374, 338} };  // (V2_BTN)
    for (int i = 0; i < 5; i++) CHECK(buttonAt(icon[i][0], icon[i][1]) == i);
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

int main() {
  testCanvas();
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
