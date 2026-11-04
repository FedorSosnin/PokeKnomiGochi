#pragma once
// The same usage as SD_MMC/File (and flashpak.h), reading the sprites folder.
// The game's paths are "/mons/p001.bin", relative to hostInit()'s resources.
#include <Arduino.h>

#define FILE_READ "r"

const char *hostResourcesDir();

#if defined(__EMSCRIPTEN__) || defined(_WIN32)
// In the browser the 40 MB folder would be too much to download, and on Windows the app is one
// .exe: both read the Knomi's compressed pack (tools/pack_flash.py, 1.2 MB) from
// <resources>/sprites.tpkz instead.
#include <stdlib.h>
#include <string.h>
#include <string>
#include <zlib.h>

class PakFile {
public:
  PakFile() {}
  PakFile(uint8_t *d, uint32_t n) : data(d), total(n) {}
  explicit operator bool() const { return data != nullptr; }
  uint32_t size() const { return total; }
  size_t read(uint8_t *out, size_t n) {
    if (n > total - pos) n = total - pos;
    memcpy(out, data + pos, n);
    pos += n;
    return n;
  }
  void close() { free(data); data = nullptr; }
private:
  uint8_t *data = nullptr;
  uint32_t total = 0, pos = 0;
};
using File = PakFile;

class HostPak {
public:
  bool begin() {
    if (blob) return true;
    FILE *f = fopen((std::string(hostResourcesDir()) + "/sprites.tpkz").c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    size = (uint32_t)ftell(f);
    rewind(f);
    blob = (uint8_t *)malloc(size);
    bool ok = blob && fread(blob, 1, size, f) == size && size >= 8 && memcmp(blob, "TPKZ", 4) == 0;
    fclose(f);
    if (ok) {
      memcpy(&count, blob + 4, 4);
      ok = count <= 1024 && 8 + count * sizeof(Entry) <= size;
    }
    if (!ok) { free(blob); blob = nullptr; }
    return ok;
  }
  // decompresses one whole file; freed with close()
  PakFile open(const char *path, const char *) {
    if (!begin()) return PakFile();
    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    for (uint32_t i = 0; i < count; i++) {
      Entry e;
      memcpy(&e, blob + 8 + i * sizeof(Entry), sizeof(Entry));
      if (strncmp(e.name, name, sizeof(e.name)) != 0) continue;
      if ((uint64_t)e.off + e.comp > size) return PakFile();
      uint8_t *dst = (uint8_t *)malloc(e.raw ? e.raw : 1);
      uLongf out = e.raw;
      if (!dst || uncompress(dst, &out, blob + e.off, e.comp) != Z_OK || out != e.raw) {
        free(dst);
        return PakFile();
      }
      return PakFile(dst, e.raw);
    }
    return PakFile();
  }
private:
  struct Entry { char name[16]; uint32_t off, raw, comp; };
  uint8_t *blob = nullptr;
  uint32_t size = 0, count = 0;
};
#else

class PakFile {
public:
  PakFile() {}
  explicit PakFile(FILE *f) : file(f) {
    if (fseek(f, 0, SEEK_END) == 0) total = (uint32_t)ftell(f);
    rewind(f);
  }
  explicit operator bool() const { return file != nullptr; }
  uint32_t size() const { return total; }
  size_t read(uint8_t *out, size_t n) { return file ? fread(out, 1, n, file) : 0; }
  void close() { if (file) fclose(file); file = nullptr; }
private:
  FILE *file = nullptr;
  uint32_t total = 0;
};
using File = PakFile;

class HostPak {
public:
  bool begin() {
    std::string thumbs = std::string(hostResourcesDir()) + "/mons/thumbs.bin";
    FILE *f = fopen(thumbs.c_str(), "rb");
    if (f) fclose(f);
    return f != nullptr;
  }
  PakFile open(const char *path, const char *) {
    FILE *f = fopen((std::string(hostResourcesDir()) + path).c_str(), "rb");
    return f ? PakFile(f) : PakFile();
  }
};
#endif
static HostPak PAK;
#define SD_MMC PAK
