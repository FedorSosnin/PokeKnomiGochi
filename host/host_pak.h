#pragma once
// The same usage as SD_MMC/File (and flashpak.h), reading the sprites folder.
// The game's paths are "/mons/p001.bin", relative to hostInit()'s resources.
#include <Arduino.h>

#define FILE_READ "r"

const char *hostResourcesDir();

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
static HostPak PAK;
#define SD_MMC PAK
