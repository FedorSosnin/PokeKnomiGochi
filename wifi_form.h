#pragma once
#include <stddef.h>
#include <string.h>

// WPA2: 8-63 caracteres, o sin clave para una red abierta
inline bool wifiFormValid(const char *ssid, const char *pass) {
  size_t s = strlen(ssid), p = strlen(pass);
  return s >= 1 && s <= 32 && (p == 0 || (p >= 8 && p <= 63));
}
