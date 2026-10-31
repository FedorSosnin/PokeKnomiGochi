#pragma once
#include <string.h>
// Capas del teclado de la clave de wifi: 27 teclas de caracteres + capa + borrar + OK. Entre
// todas cubren el ASCII imprimible (se comprueba en test/test_wifi_text.cpp).
#define KB_LAYER_COUNT 4
#define KB_CHAR_KEYS 27
static const char *const KB_LAYERS[KB_LAYER_COUNT] = {
  "abcdefghijklmnopqrstuvwxyz.",
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ.",
  "1234567890!@#$%^&*()-_=+.,?",
  "/\\|:;'\"<>[]{}~` ",
};
// etiqueta de la tecla de capa: a cual lleva
static const char *const KB_LAYER_NEXT[KB_LAYER_COUNT] = { "ABC", "123", "#+=", "abc" };

// caracter de la tecla i en esa capa; 0 si la tecla esta vacia
inline char kbLayerChar(int layer, int i) {
  const char *keys = KB_LAYERS[layer];
  return i >= 0 && i < (int)strlen(keys) ? keys[i] : 0;
}
