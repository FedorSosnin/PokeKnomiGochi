// Datos curiosos, reparto de lineas y formulario de wifi.
#include "framework.h"
#include "shim/Arduino.h"
#include "../textwrap.h"
#include "../wifi_form.h"
#include "../dex.h"
#include "../dexfacts.h"
#include <string.h>

TEST(facts, las_151_tienen_dato_y_caben_en_cuatro_lineas) {
  for (int d = 1; d <= 151; d++) {
    const char *p = DEX_FACT[d];
    CHECK(strlen(p) > 10);
    int lines = 0;
    while (*p) {
      int skip, n = wrapNext(p, 29, skip);
      CHECK(n <= 29);
      CHECK(skip > 0);
      p += skip;
      lines++;
    }
    CHECK_MSG(lines <= 4, "no cabe en la vista detalle");
  }
}

TEST(facts, cada_tipo_tiene_nombre) {
  for (int t = 0; t < TY_COUNT; t++) CHECK(strlen(TYPE_NAME[t]) > 0);
}

TEST(wrap, corta_por_espacios_y_parte_palabras_largas) {
  int skip;
  CHECK_EQ(wrapNext("hola mundo", 7, skip), 4);
  CHECK_EQ(skip, 5);
  CHECK_EQ(wrapNext("hola", 7, skip), 4);
  CHECK_EQ(skip, 4);
  CHECK_EQ(wrapNext("abcd efgh", 4, skip), 4);
  CHECK_EQ(skip, 5);
  CHECK_EQ(wrapNext("abcdefghij", 4, skip), 4);
  CHECK_EQ(skip, 4);
}

TEST(wifi, valida_nombre_y_clave) {
  CHECK(wifiFormValid("Casa", "12345678"));
  CHECK(wifiFormValid("Abierta", ""));
  CHECK(!wifiFormValid("", "12345678"));
  CHECK(!wifiFormValid("Casa", "corta"));
  CHECK(!wifiFormValid("123456789012345678901234567890123", "12345678"));
  char larga[70];
  memset(larga, 'a', 64);
  larga[64] = 0;
  CHECK(!wifiFormValid("Casa", larga));
}

TEST(wifi, un_nombre_de_red_hostil_no_entra_en_el_html) {
  char out[200];
  htmlEscape("\"><script>alert('x')</script>&", out, sizeof(out));
  CHECK(strchr(out, '<') == nullptr);
  CHECK(strchr(out, '>') == nullptr);
  CHECK(strchr(out, '"') == nullptr);
  CHECK(strchr(out, '\'') == nullptr);
  CHECK(strstr(out, "&lt;script&gt;") != nullptr);
}

TEST(wifi, escapar_no_se_sale_del_buffer) {
  char out[8];
  size_t n = htmlEscape("<<<<<<<<", out, sizeof(out));
  CHECK(n < sizeof(out));
  CHECK_EQ(out[n], 0);
  CHECK(strcmp(out, "&lt;") == 0);  // sin entidades a medias
}
