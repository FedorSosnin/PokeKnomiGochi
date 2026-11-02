// Pruebas de la capa de host, sin ventana. Las primeras son piezas sueltas; la ultima arranca el juego
// entero contra los sprites reales y una partida en una carpeta temporal.
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "U8g2lib.h"
#include "audio.h"
#include "glcdfont.h"
#include "host.h"
#include "host_hal.h"
#include "host_internal.h"
#include <Preferences.h>

static int gFails = 0, gChecks = 0;
#define CHECK(cond)                                                     \
  do {                                                                  \
    gChecks++;                                                          \
    if (!(cond)) {                                                      \
      printf("FALLO %s:%d: %s\n", __FILE__, __LINE__, #cond);           \
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
  c.fillRect(-5, -5, 10, 10, 0xFFFF);  // recortado a 5x5
  CHECK(countColor(c, 0xFFFF) == 25);
  c.fillScreen(0);
  c.fillRect(460, 460, 100, 100, 0xFFFF);  // por la esquina opuesta
  CHECK(countColor(c, 0xFFFF) == 36);

  c.fillScreen(0);
  c.fillCircle(233, 233, 100, 0xF800);
  uint16_t *fb = c.framebuffer();
  CHECK(fb[233 * 466 + 233] == 0xF800);
  CHECK(fb[233 * 466 + 133] == 0xF800);  // borde izquierdo
  CHECK(fb[233 * 466 + 132] == 0);
  CHECK(fb[133 * 466 + 133] == 0);       // la esquina del cuadrado queda fuera
  int area = countColor(c, 0xF800);
  CHECK(area > 31000 && area < 31800);   // pi * r^2 = 31416

  c.fillScreen(0);
  c.fillRoundRect(10, 10, 100, 50, 12, 0x07E0);
  CHECK(fb[10 * 466 + 10] == 0);          // esquina redondeada
  CHECK(fb[35 * 466 + 10] == 0x07E0);     // lado izquierdo
  CHECK(fb[35 * 466 + 109] == 0x07E0);    // lado derecho, inclusive
  CHECK(fb[35 * 466 + 110] == 0);

  c.fillScreen(0);
  c.fillTriangle(100, 100, 200, 100, 150, 200, 0x001F);
  CHECK(fb[100 * 466 + 150] == 0x001F);
  CHECK(fb[200 * 466 + 150] == 0x001F);
  CHECK(fb[201 * 466 + 150] == 0);

  // texto clasico: pixel a pixel con la tabla de la fuente, y la misma medida que usa textW()
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
  CHECK(y1 < 0);  // ancladas en la linea base: applyLangFont() mide el ascenso asi
  CHECK(w > 0 && h > 0);
  c.setCursor(100, 100);
  c.print("\xE3\x81\x82");  // kana "a": de ancho completo en unifont
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
    p.putString("str", "hola");
    const uint8_t raw[3] = {1, 0, 255};
    p.putBytes("raw", raw, 3);
    p.putBytes("nada", raw, 0);
    p.end();
    Preferences ro;
    ro.begin("ns", true);
    CHECK(ro.putUChar("uc", 1) == 0);  // solo lectura
    CHECK(ro.getUChar("uc") == 200);
    ro.end();
  }
  CHECK(readFile(dir + "/ns.prefs").empty());  // a disco al vaciar, no en cada put
  hostPrefsFlush();
  CHECK(!readFile(dir + "/ns.prefs").empty());
  CHECK(readFile(dir + "/ns.prefs.tmp").empty());

  hostPrefsSetDir(dir.c_str());  // como un arranque nuevo
  {
    Preferences p;
    p.begin("ns");
    CHECK(p.getUChar("uc") == 200);
    CHECK(p.getShort("s") == -5);
    CHECK(p.getUInt("ui") == 4000000000u);
    CHECK(p.getBool("b"));
    char s[8];
    CHECK(p.getString("str", s, sizeof s) == 4 && strcmp(s, "hola") == 0);
    uint8_t raw[4] = {9, 9, 9, 9};
    CHECK(p.getBytes("raw", raw, 4) == 3 && raw[0] == 1 && raw[1] == 0 && raw[2] == 255 && raw[3] == 9);
    CHECK(p.isKey("nada"));
    CHECK(p.getUChar("no-existe", 7) == 7);
    CHECK(p.getUShort("uc", 7) == 7);  // otro tamano: el valor por defecto
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
    CHECK(!p.isKey("s"));  // el borrado tambien llega a disco
    p.end();
  }
  hostPrefsSetDir(nullptr);
  removeDir(dir);
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
  CHECK(!anySound(256));     // y se acaba
  sfxPlay(SFX_COUNT);        // fuera de rango
  CHECK(!anySound(256));
  audioSetEnabled(false);
  sfxPlay(SFX_TAP);
  CHECK(!anySound(256));
  audioSetEnabled(true);
  audioSetSleeping(true);    // durmiendo no suena, como en la placa
  sfxPlay(SFX_TAP);
  CHECK(!anySound(256));
  audioSetSleeping(false);
}

static void runTicks(int ms) {
  for (int i = 0; i < ms / 5; i++) {
    hostTick();
    usleep(5000);
  }
}

// el juego de verdad, de principio a fin
static void testGame() {
  std::string res = tempDir(), save = tempDir(), cwd = getcwd(nullptr, 0);
  CHECK(symlink((cwd + "/../tools/sdcard/mons").c_str(), (res + "/mons").c_str()) == 0);
  CHECK(symlink((cwd + "/fonts").c_str(), (res + "/fonts").c_str()) == 0);
  std::string saveSub = save + "/a/b";  // tambien crea las carpetas intermedias
  hostInit(res.c_str(), saveSub.c_str());
  CHECK(!readFile(saveSub + "/tamapoke.prefs").empty());

  runTicks(200);
  CHECK(hostFrameDirty());
  std::vector<uint8_t> rgba(HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4);
  hostFrameRGBA(rgba.data());
  CHECK(!hostFrameDirty());
  CHECK(rgba[3] == 255);
  int center = (HOST_CANVAS_SIZE * HOST_CANVAS_SIZE / 2 + HOST_CANVAS_SIZE / 2) * 4;
  CHECK(rgba[center] > 200 && rgba[center + 1] > 200);  // el fondo claro de la eleccion de inicial

  // un tap mas corto que el sondeo del juego no se pierde
  hostPointer(233, 145, true);
  hostPointer(233, 145, false);
  int16_t x, y;
  CHECK(hostPointerRead(&x, &y) && x == 233 && y == 145);
  runTicks(150);
  CHECK(!hostPointerRead(&x, &y));
  runTicks(150);  // ha elegido a Bulbasaur: el fondo pasa a ser el del huevo
  CHECK(hostFrameDirty());

  // las flechas son una rampa, con el signo que espera playerTilt()
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
  testAudio();
  testGame();
  printf("capa de host: %d comprobaciones, %d fallos\n", gChecks, gFails);
  return gFails ? 1 : 0;
}
