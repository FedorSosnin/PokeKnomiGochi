// Preferences on files: <save folder>/<namespace>.prefs, one "key hex" line per
// entry. Like the NVS, each put is immediately visible to reads; it is written to disk at the end of the tick
// (the game writes dozens of keys per save) and always by renaming, so that a cut
// halfway does not leave a broken save.
#include "Preferences.h"
#include "host_internal.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <map>
#include <set>
#include <vector>

typedef std::map<std::string, std::vector<uint8_t> > KV;
static std::map<std::string, KV> gStore;
static std::set<std::string> gLoaded, gDirty;
static std::string gDir;

static std::string fileFor(const std::string &ns) { return gDir + "/" + ns + ".prefs"; }

static int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static void loadNs(const std::string &ns) {
  if (!gLoaded.insert(ns).second || gDir.empty()) return;
  FILE *f = fopen(fileFor(ns).c_str(), "r");
  if (!f) return;
  KV &kv = gStore[ns];
  char key[32];
  static char hex[8192];
  while (fscanf(f, "%31s %8191s", key, hex) == 2) {
    std::vector<uint8_t> v;
    for (const char *p = hex; p[0] && p[1]; p += 2) {
      if (hexVal(p[0]) < 0 || hexVal(p[1]) < 0) { v.clear(); break; }
      v.push_back((uint8_t)(hexVal(p[0]) << 4 | hexVal(p[1])));
    }
    if (strcmp(hex, "-") == 0) v.clear();  // empty value
    kv[key] = v;
  }
  fclose(f);
}

void hostPrefsSetDir(const char *dir) {
  gDir = dir ? dir : "";
  gStore.clear();
  gLoaded.clear();
  gDirty.clear();
}

void hostPrefsFlush() {
  if (gDirty.empty()) return;
  if (!gDir.empty()) {
    for (const std::string &ns : gDirty) {
      std::string path = fileFor(ns), tmp = path + ".tmp";
      FILE *f = fopen(tmp.c_str(), "w");
      if (!f) { fprintf(stderr, "cannot save to %s\n", tmp.c_str()); continue; }
      bool ok = true;
      for (const auto &e : gStore[ns]) {
        ok = fprintf(f, "%s ", e.first.c_str()) > 0 && ok;
        for (uint8_t b : e.second) ok = fprintf(f, "%02x", b) > 0 && ok;
        ok = fprintf(f, e.second.empty() ? "-\n" : "\n") > 0 && ok;
      }
      ok = fclose(f) == 0 && ok;
#ifdef _WIN32  // rename() won't replace a file that exists
      if (ok) ok = MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
      if (ok) ok = rename(tmp.c_str(), path.c_str()) == 0;
#endif
      if (!ok) { remove(tmp.c_str()); fprintf(stderr, "failed to save %s\n", path.c_str()); }
    }
  }
  gDirty.clear();
}

bool Preferences::begin(const char *name, bool readOnly) {
  ns_ = name;
  ro_ = readOnly;
  open_ = true;
  loadNs(ns_);
  return true;
}

void Preferences::end() { open_ = false; }

bool Preferences::clear() {
  if (!open_ || ro_) return false;
  gStore[ns_].clear();
  gDirty.insert(ns_);
  return true;
}

bool Preferences::isKey(const char *key) {
  return open_ && gStore[ns_].count(key) > 0;
}

bool Preferences::remove(const char *key) {
  if (!open_ || ro_ || !gStore[ns_].erase(key)) return false;
  gDirty.insert(ns_);
  return true;
}

size_t Preferences::putBytes(const char *k, const void *v, size_t len) {
  if (!open_ || ro_) return 0;
  const uint8_t *p = (const uint8_t *)v;
  std::vector<uint8_t> &slot = gStore[ns_][k];
  if (slot.size() != len || (len && memcmp(slot.data(), p, len) != 0)) {
    slot.assign(p, p + len);
    gDirty.insert(ns_);
  }
  return len;
}

size_t Preferences::putString(const char *k, const char *v) { return putBytes(k, v, strlen(v) + 1); }
size_t Preferences::putBool(const char *k, bool v)       { uint8_t b = v ? 1 : 0; return putBytes(k, &b, 1); }
size_t Preferences::putUChar(const char *k, uint8_t v)   { return putBytes(k, &v, sizeof v); }
size_t Preferences::putChar(const char *k, int8_t v)     { return putBytes(k, &v, sizeof v); }
size_t Preferences::putShort(const char *k, int16_t v)   { return putBytes(k, &v, sizeof v); }
size_t Preferences::putUShort(const char *k, uint16_t v) { return putBytes(k, &v, sizeof v); }
size_t Preferences::putInt(const char *k, int32_t v)     { return putBytes(k, &v, sizeof v); }
size_t Preferences::putUInt(const char *k, uint32_t v)   { return putBytes(k, &v, sizeof v); }

template <class T>
static T getScalar(const std::string &ns, bool open, const char *k, T def) {
  if (!open) return def;
  KV &kv = gStore[ns];
  KV::iterator it = kv.find(k);
  if (it == kv.end() || it->second.size() != sizeof(T)) return def;
  T out;
  memcpy(&out, it->second.data(), sizeof(T));
  return out;
}

bool Preferences::getBool(const char *k, bool def)          { return getScalar<uint8_t>(ns_, open_, k, def ? 1 : 0) != 0; }
uint8_t Preferences::getUChar(const char *k, uint8_t def)   { return getScalar(ns_, open_, k, def); }
int8_t Preferences::getChar(const char *k, int8_t def)      { return getScalar(ns_, open_, k, def); }
int16_t Preferences::getShort(const char *k, int16_t def)   { return getScalar(ns_, open_, k, def); }
uint16_t Preferences::getUShort(const char *k, uint16_t def){ return getScalar(ns_, open_, k, def); }
int32_t Preferences::getInt(const char *k, int32_t def)     { return getScalar(ns_, open_, k, def); }
uint32_t Preferences::getUInt(const char *k, uint32_t def)  { return getScalar(ns_, open_, k, def); }

size_t Preferences::getBytes(const char *k, void *buf, size_t maxLen) {
  if (!open_) return 0;
  KV &kv = gStore[ns_];
  KV::iterator it = kv.find(k);
  if (it == kv.end()) return 0;  // like the real NVS: leaves the buffer untouched
  size_t n = it->second.size() < maxLen ? it->second.size() : maxLen;
  if (n) memcpy(buf, it->second.data(), n);
  return n;
}

size_t Preferences::getString(const char *k, char *buf, size_t maxLen) {
  if (!open_ || maxLen == 0) return 0;
  KV &kv = gStore[ns_];
  KV::iterator it = kv.find(k);
  if (it == kv.end()) { buf[0] = 0; return 0; }
  size_t n = it->second.size() < maxLen ? it->second.size() : maxLen - 1;
  if (n) memcpy(buf, it->second.data(), n);
  buf[n] = 0;
  return strlen(buf);
}
