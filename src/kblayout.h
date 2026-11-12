#pragma once
#include <string.h>
// Layers of the wifi password keyboard: 27 character keys + layer + delete + OK. Between
// them all they cover printable ASCII (checked in test/test_wifi_text.cpp).
#define KB_LAYER_COUNT 4
#define KB_CHAR_KEYS 27
static const char *const KB_LAYERS[KB_LAYER_COUNT] = {
  "abcdefghijklmnopqrstuvwxyz.",
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ.",
  "1234567890!@#$%^&*()-_=+.,?",
  "/\\|:;'\"<>[]{}~` ",
};
// label of the layer key: which one it leads to
static const char *const KB_LAYER_NEXT[KB_LAYER_COUNT] = { "ABC", "123", "#+=", "abc" };

// character of key i in that layer; 0 if the key is empty
inline char kbLayerChar(int layer, int i) {
  const char *keys = KB_LAYERS[layer];
  return i >= 0 && i < (int)strlen(keys) ? keys[i] : 0;
}
