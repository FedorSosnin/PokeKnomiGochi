// Fun facts, line wrapping and the wifi form.
#include "framework.h"
#include "shim/Arduino.h"
#include "../src/textwrap.h"
#include "../src/wifi_form.h"
#include "../src/kblayout.h"
#include "../src/dex.h"
#include "../src/dexfacts.h"
#include <string.h>

TEST(facts, all_151_have_a_fact_and_fit_in_four_lines) {
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
    CHECK_MSG(lines <= 4, "does not fit in the detail view");
  }
}

TEST(facts, every_type_has_a_name) {
  for (int t = 0; t < TY_COUNT; t++) CHECK(strlen(TYPE_NAME[t]) > 0);
}

TEST(wrap, breaks_at_spaces_and_splits_long_words) {
  int skip;
  CHECK_EQ(wrapNext("good world", 7, skip), 4);
  CHECK_EQ(skip, 5);
  CHECK_EQ(wrapNext("hell", 7, skip), 4);
  CHECK_EQ(skip, 4);
  CHECK_EQ(wrapNext("abcd efgh", 4, skip), 4);
  CHECK_EQ(skip, 5);
  CHECK_EQ(wrapNext("abcdefghij", 4, skip), 4);
  CHECK_EQ(skip, 4);
}

TEST(wifi, validates_name_and_password) {
  CHECK(wifiFormValid("Home", "12345678"));
  CHECK(wifiFormValid("Open", ""));
  CHECK(!wifiFormValid("", "12345678"));
  CHECK(!wifiFormValid("Home", "short"));
  CHECK(!wifiFormValid("123456789012345678901234567890123", "12345678"));
  char tooLong[70];
  memset(tooLong, 'a', 64);
  tooLong[64] = 0;
  CHECK(!wifiFormValid("Home", tooLong));
}

TEST(wifi, keyboard_covers_all_printable_ascii) {
  for (int c = 0x20; c <= 0x7E; c++) {
    bool found = false;
    for (int l = 0; l < KB_LAYER_COUNT && !found; l++)
      for (int i = 0; i < KB_CHAR_KEYS && !found; i++) found = kbLayerChar(l, i) == (char)c;
    CHECK_MSG(found, "a character is missing from the keyboard");
  }
}

TEST(wifi, no_layer_exceeds_its_keys) {
  for (int l = 0; l < KB_LAYER_COUNT; l++) {
    CHECK((int)strlen(KB_LAYERS[l]) <= KB_CHAR_KEYS);
    CHECK_EQ((int)kbLayerChar(l, KB_CHAR_KEYS), 0);
    CHECK_EQ((int)kbLayerChar(l, -1), 0);
  }
}
