#pragma once
#include <stddef.h>
#include <string.h>
// Validacion y escape de la pagina de configuracion de wifi, sin hardware. Los nombres de red
// los pone cualquier vecino, asi que nunca entran sin escapar en el HTML.

// WPA2: 8-63 caracteres, o sin clave para una red abierta
inline bool wifiFormValid(const char *ssid, const char *pass) {
  size_t s = strlen(ssid), p = strlen(pass);
  return s >= 1 && s <= 32 && (p == 0 || (p >= 8 && p <= 63));
}

// escapa src en out (siempre termina en 0). Devuelve los bytes escritos sin contar el 0.
inline size_t htmlEscape(const char *src, char *out, size_t outSize) {
  size_t n = 0;
  if (outSize == 0) return 0;
  for (; *src; src++) {
    const char *rep = nullptr;
    switch (*src) {
      case '&': rep = "&amp;"; break;
      case '<': rep = "&lt;"; break;
      case '>': rep = "&gt;"; break;
      case '"': rep = "&quot;"; break;
      case '\'': rep = "&#39;"; break;
    }
    size_t len = rep ? strlen(rep) : 1;
    if (n + len >= outSize) break;  // sin media entidad a medias
    if (rep) memcpy(out + n, rep, len);
    else out[n] = *src;
    n += len;
  }
  out[n] = 0;
  return n;
}
