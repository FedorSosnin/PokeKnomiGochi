#pragma once
// Reparto de texto en lineas, sin hardware (se prueba en test/test_wifi_text.cpp).

// Cuantos caracteres de s entran en una linea de maxChars, cortando por un espacio si se
// puede. skip recibe lo que hay que avanzar en s para seguir (se come el espacio del corte).
inline int wrapNext(const char *s, int maxChars, int &skip) {
  int len = 0, lastSpace = -1;
  while (s[len] && len < maxChars) {
    if (s[len] == ' ') lastSpace = len;
    len++;
  }
  if (!s[len]) { skip = len; return len; }           // lo que queda cabe entero
  if (s[len] == ' ') { skip = len + 1; return len; }  // corte justo en un espacio
  if (lastSpace > 0) { skip = lastSpace + 1; return lastSpace; }
  skip = len;  // palabra mas larga que la linea: se parte
  return len;
}
