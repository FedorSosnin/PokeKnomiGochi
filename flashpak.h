#pragma once
// Sprites comprimidos en la particion "ffat" (formato en tools/pack_flash.py), con la
// misma forma de uso que SD_MMC/File para que sdmon.cpp no cambie en las placas sin SD.
#include <Arduino.h>
#include <esp_partition.h>
#include "miniz.h"

#ifndef FILE_READ
#define FILE_READ "r"
#endif

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

class FlashPak {
public:
  bool begin() {
    part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "ffat");
    if (!part) return false;
    uint32_t h[2];
    if (esp_partition_read(part, 0, h, 8) != ESP_OK || memcmp(h, "TPKZ", 4) != 0 || h[1] > 1024) return false;
    count = h[1];
    index = (Entry *)ps_malloc(count * sizeof(Entry));
    return index && esp_partition_read(part, 8, index, count * sizeof(Entry)) == ESP_OK;
  }

  // descomprime el fichero entero a PSRAM; se libera con close()
  PakFile open(const char *path, const char *) {
    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    for (uint32_t i = 0; i < count; i++) {
      if (strncmp(index[i].name, name, sizeof(index[i].name)) != 0) continue;
      return inflate(index[i]);
    }
    return PakFile();
  }

private:
  struct Entry { char name[16]; uint32_t off, raw, comp; };
  PakFile inflate(const Entry &e) {
    uint8_t *src = (uint8_t *)ps_malloc(e.comp);
    uint8_t *dst = (uint8_t *)ps_malloc(e.raw);
    // ~11 KB de estado: en el heap, no cabe en la pila del loop
    tinfl_decompressor *d = (tinfl_decompressor *)ps_malloc(sizeof(tinfl_decompressor));
    bool ok = src && dst && d && esp_partition_read(part, e.off, src, e.comp) == ESP_OK;
    if (ok) {
      tinfl_init(d);
      size_t in = e.comp, out = e.raw;
      tinfl_status st = tinfl_decompress(d, src, &in, dst, dst, &out,
                                         TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
      ok = st == TINFL_STATUS_DONE && out == e.raw;
    }
    free(src);
    free(d);
    if (!ok) { free(dst); return PakFile(); }
    return PakFile(dst, e.raw);
  }
  const esp_partition_t *part = nullptr;
  Entry *index = nullptr;
  uint32_t count = 0;
};
static FlashPak PAK;
#define SD_MMC PAK
