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
#include "glcdfont.h"
#include "host.h"
#include "host_hal.h"
#include "host_internal.h"
#include "rtcbat.h"
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
  c.setFont();
  c.setUTF8Print(false);
  c.setCursor(0, 0);
  c.print("A");
  CHECK(c.getCursorX() == 6);
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
  sfxPlay(SFX_TAP);          // 880 Hz, 35 ms
  CHECK(anySound(HOST_AUDIO_RATE * 35 / 1000));
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
static void testGame() {
  std::string res = tempDir(), save = tempDir(), cwd = getcwd(nullptr, 0);
  CHECK(symlink((cwd + "/../tools/sdcard/mons").c_str(), (res + "/mons").c_str()) == 0);
  CHECK(symlink((cwd + "/fonts").c_str(), (res + "/fonts").c_str()) == 0);
  std::string saveSub = save + "/a/b";  // also creates the intermediate folders
  hostInit(res.c_str(), saveSub.c_str());
  CHECK(!readFile(saveSub + "/tamapoke.prefs").empty());

  runTicks(200);
  CHECK(hostFrameDirty());
  std::vector<uint8_t> rgba(HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4);
  hostFrameRGBA(rgba.data());
  CHECK(!hostFrameDirty());
  CHECK(rgba[3] == 255);
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
  std::vector<uint8_t> saved = readFile(saveSub + "/tamapoke.prefs");
  std::string text(saved.begin(), saved.end());
  CHECK(text.find("spe ") != std::string::npos || text.find("sp ") != std::string::npos);
  CHECK(!hostRestartRequested());
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
  testBuddy();
  testRelay();
  testAudio();
  testClock();
  testGame();
  printf("host layer: %d checks, %d failures\n", gChecks, gFails);
  return gFails ? 1 : 0;
}
