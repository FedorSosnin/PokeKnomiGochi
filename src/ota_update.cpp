#include "ota_update.h"

#if defined(BOARD_KNOMI_V2) && !defined(BOARD_HOST)
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <NetworkClientSecure.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include "ota.h"
#include "ota_roots.h"
#include "buddy_net.h"

static OtaState state = OTA_IDLE;
static char newVer[16] = "", spritesSha[65] = "";
static uint32_t spritesSize = 0;
static bool fwNewer = false, spritesStale = false;
static uint8_t buf[4096];  // one flash sector: sprite reads, writes and the held-back header
static const char *gBase = OTA_BASE_URL;  // the site update.json last came from: the files come from the same one

// The buddy relay's TLS connection closed while an update talks to the site: two at once don't
// fit in memory (1.22 on a Knomi: 111 KB free before the relay, 66 KB after, http -1)
struct RelayHold {
  RelayHold() { buddyNetRelayHold(true); }
  ~RelayHold() { buddyNetRelayHold(false); }
};

OtaState otaState() { return state; }
const char *otaNewVersion() { return newVer; }

// the core marks every firmware good at boot unless this says to wait: we confirm it
// ourselves once it has run for a while (otaConfirmBoot), so a broken update rolls back
bool verifyRollbackLater() { return true; }

void otaConfirmBoot() {
  esp_ota_img_states_t st;
  const esp_partition_t *run = esp_ota_get_running_partition();
  if (esp_ota_get_state_partition(run, &st) == ESP_OK && st == ESP_OTA_IMG_PENDING_VERIFY) {
    esp_ota_mark_app_valid_cancel_rollback();
    Serial.println("ota: new firmware confirmed");
  }
}

static const esp_partition_t *spritesPart() {
  return esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "ffat");
}

static void toHex(const uint8_t *d, char *out) {
  for (int i = 0; i < 32; i++) sprintf(out + i * 2, "%02x", d[i]);
}

// do the sprites in flash match the site's? (sha256 of the first sprites_size bytes, ~0.5 s)
static bool spritesMatch() {
  const esp_partition_t *p = spritesPart();
  if (!p || !spritesSize || spritesSize > p->size) return false;
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  for (uint32_t off = 0; off < spritesSize; off += sizeof(buf)) {
    uint32_t n = spritesSize - off < sizeof(buf) ? spritesSize - off : sizeof(buf);
    if (esp_partition_read(p, off, buf, n) != ESP_OK) { mbedtls_sha256_free(&ctx); return false; }
    mbedtls_sha256_update(&ctx, buf, n);
  }
  uint8_t sum[32];
  char hex[65];
  mbedtls_sha256_finish(&ctx, sum);
  mbedtls_sha256_free(&ctx);
  toHex(sum, hex);
  return strcmp(hex, spritesSha) == 0;
}

bool otaCheck(const char *current) {
  if (WiFi.status() != WL_CONNECTED) {
    state = OTA_FAILED;
    return false;
  }
  RelayHold hold;
  NetworkClientSecure client;
  client.setCACert(OTA_ROOTS);
  String body;
  int code = -1;
  // Catch 151's site first, then the old one (ota.h); redirects followed, in case a site moves again
  static const char *const BASES[2] = { OTA_BASE_URL, OTA_BASE_URL_OLD };
  for (int b = 0; b < 2 && code != 200; b++) {
    HTTPClient http;
    http.setTimeout(8000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if (http.begin(client, String(BASES[b]) + "update.json")) {
      code = http.GET();
      if (code == 200) {
        body = http.getString();
        gBase = BASES[b];
      }
      http.end();
    }
  }
  char sz[16];
  if (code != 200 || !otaJsonField(body.c_str(), "version", newVer, sizeof(newVer)) ||
      !otaJsonField(body.c_str(), "sprites_size", sz, sizeof(sz)) ||
      !otaJsonField(body.c_str(), "sprites_sha256", spritesSha, sizeof(spritesSha))) {
    Serial.printf("ota: couldn't read update.json from either site (http %d)\n", code);
    state = OTA_FAILED;
    return false;
  }
  spritesSize = strtoul(sz, nullptr, 10);
  fwNewer = otaVersionCmp(newVer, current) > 0;
  spritesStale = !spritesMatch();  // also repairs sprites left half-written by a cut update
  state = fwNewer || spritesStale ? OTA_AVAILABLE : OTA_LATEST;
  Serial.printf("ota: site v%s, running v%s, sprites %s\n", newVer, current, spritesStale ? "differ" : "match");
  return true;
}

// Downloads sprites.bin over the sprites partition. The first sector (with the "TPKZ" magic)
// is written last and only if the whole file arrived with the right sha256: a cut download
// leaves no valid header, so the game just runs without sprites until the next update.
static bool writeSprites(OtaProgressCb cb, uint8_t from, uint8_t to) {
  const esp_partition_t *p = spritesPart();
  if (!p || spritesSize > p->size) return false;
  NetworkClientSecure client;
  client.setCACert(OTA_ROOTS);
  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, String(gBase) + "firmware/sprites.bin")) return false;
  bool ok = false;
  if (http.GET() == 200 && (uint32_t)http.getSize() == spritesSize) {
    uint32_t span = (spritesSize + 4095) & ~4095UL;
    static uint8_t head[4096];
    if (esp_partition_erase_range(p, 0, span) == ESP_OK) {
      NetworkClient *s = http.getStreamPtr();
      mbedtls_sha256_context ctx;
      mbedtls_sha256_init(&ctx);
      mbedtls_sha256_starts(&ctx, 0);
      uint32_t got = 0, last = millis();
      ok = true;
      while (ok && got < spritesSize) {
        uint32_t want = spritesSize - got < sizeof(buf) ? spritesSize - got : sizeof(buf);
        size_t n = s->readBytes(buf, want);
        if (!n) {
          if (millis() - last > 15000) ok = false;
          continue;
        }
        last = millis();
        mbedtls_sha256_update(&ctx, buf, n);
        // the header sector is held back; everything else goes straight to flash
        uint32_t off = got;
        const uint8_t *src = buf;
        size_t left = n;
        if (off < sizeof(head)) {
          size_t h = sizeof(head) - off < left ? sizeof(head) - off : left;
          memcpy(head + off, src, h);
          off += h;
          src += h;
          left -= h;
        }
        if (left && esp_partition_write(p, off, src, left) != ESP_OK) ok = false;
        got += n;
        if (cb) cb(from + (uint32_t)(to - from) * got / spritesSize);
      }
      uint8_t sum[32];
      char hex[65];
      mbedtls_sha256_finish(&ctx, sum);
      mbedtls_sha256_free(&ctx);
      toHex(sum, hex);
      ok = ok && strcmp(hex, spritesSha) == 0;
      uint32_t headLen = spritesSize < sizeof(head) ? spritesSize : sizeof(head);
      if (ok) ok = esp_partition_write(p, 0, head, headLen) == ESP_OK;
    }
  }
  http.end();
  Serial.printf("ota: sprites %s\n", ok ? "written" : "FAILED");
  return ok;
}

static OtaProgressCb gCb = nullptr;
static uint8_t gFrom = 0, gTo = 100;

bool otaInstall(OtaProgressCb cb) {
  if (state != OTA_AVAILABLE || WiFi.status() != WL_CONNECTED) return false;
  RelayHold hold;
  bool both = fwNewer && spritesStale;
  if (cb) cb(0);
  // firmware first: if it fails nothing has changed. It only becomes the boot slot once complete.
  if (fwNewer) {
    NetworkClientSecure client;
    client.setCACert(OTA_ROOTS);
    gCb = cb;
    gFrom = 0;
    gTo = both ? 70 : 100;
    httpUpdate.rebootOnUpdate(false);
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpUpdate.onProgress([](int cur, int total) {
      if (gCb && total > 0) gCb(gFrom + (uint32_t)(gTo - gFrom) * cur / total);
    });
    t_httpUpdate_return r = httpUpdate.update(client, String(gBase) + "firmware/firmware.bin");
    if (r != HTTP_UPDATE_OK) {
      Serial.printf("ota: firmware FAILED: %s\n", httpUpdate.getLastErrorString().c_str());
      state = OTA_FAILED;
      return false;
    }
    Serial.println("ota: firmware written");
  }
  if (spritesStale && !writeSprites(cb, both ? 70 : 0, 100)) {
    // a new firmware is already in place: restart into it anyway, the next check repairs the sprites
    if (!fwNewer) {
      state = OTA_FAILED;
      return false;
    }
  }
  if (cb) cb(100);
  Serial.println("ota: restarting");
  delay(300);
  ESP.restart();
  return true;
}

#else

OtaState otaState() { return OTA_IDLE; }
const char *otaNewVersion() { return ""; }
bool otaCheck(const char *) { return false; }
bool otaInstall(OtaProgressCb) { return false; }
void otaConfirmBoot() {}

#endif
