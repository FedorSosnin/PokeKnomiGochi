// Consistency of the translations (i18n.cpp).
#include "framework.h"
#include "shim/Arduino.h"
#include "shim/Preferences.h"
#include "../i18n.h"
#include "../pet.h"  // MED_COUNT
#include <ctype.h>
#include <string.h>
#include <vector>

static const char *LANG_NAME[LANG_COUNT] = { "ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO" };

// extracts the signature of the printf specifiers from a string: "%lud" -> "lu"
static std::vector<std::string> specs(const char *s) {
  std::vector<std::string> out;
  const char *p = s;
  while (*p) {
    if (*p != '%') { p++; continue; }
    p++;
    if (*p == '%') { p++; continue; }   // %% is a literal
    std::string sig;
    while (*p && strchr("-+ #0", *p)) p++;             // flags
    while (*p && isdigit((unsigned char)*p)) p++;      // width
    if (*p == '.') { p++; while (*p && isdigit((unsigned char)*p)) p++; }
    while (*p && strchr("lhzjt", *p)) { sig += *p; p++; }  // modifier
    if (!*p) { out.push_back(sig + "<incomplete>"); break; }
    sig += *p++;
    out.push_back(sig);
  }
  return out;
}

static std::string joinSpecs(const std::vector<std::string> &v) {
  std::string s;
  for (size_t i = 0; i < v.size(); i++) { if (i) s += ","; s += "%" + v[i]; }
  return s.empty() ? "(no format)" : s;
}

// expected signature of each format string, taken from the sketch's calls
struct ExpectedFmt { StrId id; const char *sig; };
static const ExpectedFmt EXPECTED[] = {
  { S_BUDDY_HERE_FMT, "%s" },
  { S_BUDDY_INV_BTL_FMT, "%s" },
  { S_BUDDY_INV_TRD_FMT, "%s" },
  { S_BUDDY_LEFT_FMT, "%s" },
  { S_BUDDY_BUSY_FMT, "%s" },
  { S_TRADE_GOT_FMT, "%s" },
  { S_ZONE_CAUGHT_FMT, "%d,%d" },
  { S_POKEDEX_FMT,     "%u" },
  { S_NAME_FMT,        "%s,%s,%u" },
  { S_RELEASE_FMT,     "%s" },
  { S_WINS_FMT,        "%u" },
  { S_STR_GAIN_FMT,    "%u" },
  { S_RECORD_FMT,      "%u" },
  { S_SCORE_FMT,       "%u" },
  { S_STREAK_DAYS_FMT, "%u" },
  { S_STREAK_FMT,      "%u,%u" },
  { S_INFO_FMT,        "%s,%lu" },
  { S_MEDALS_FMT,      "%d,%d" },
  { S_TRAINER_FMT,     "%d,%d" },
  { S_UPD_TO_FMT,      "%s" },
  { S_UPD_BUSY_FMT,    "%u" },
  { S_BUDDIES_FMT,     "%u" },
  { S_BUDDY_INV_PLAY_FMT, "%s" },
  { S_SD_THEY_GOT_FMT, "%s" },
  { S_SD_LOSE_FMT,     "%s" },
  { S_GOT_AWAY_FMT,    "%s" },
  { S_VB_THEY_SCORE_FMT, "%s" },
  { S_ZONE_MEDALS_FMT, "%d,%d" },
  { S_LV100_FMT,       "%d,%d" },
  { S_SHINY_WILD_FMT,  "%s" },
  { S_VISIT_MIN_FMT,   "%u" },
  { S_VISIT_CANDY_FMT, "%s" },
  { S_REC_FMT,         "%u" },
  { S_LVL_FMT,         "%u" },
  { S_NEXT_LVL_FMT,    "%u,%u" },
  { S_EVO_IN_FMT,      "%u" },
  { S_MISTAKES_FMT,    "%u" },
  { S_FAREWELL_BTN,    "%s" },
  { S_RUNAWAY_BTN,     "%s" },
  { S_BTL_WILD,        "%s" },
  { S_BTL_LOST,        "%s" },
  { S_BTL_LEARNED,     "%s" },
  { S_ITS_FMT,         "%s" },
  { S_CAUGHT_FMT,      "%s" },
  { S_FLED_FMT,        "%s" },
  { S_VISIT_BYE_FMT,   "%s" },
};
static const int NUM_EXPECTED = (int)(sizeof(EXPECTED) / sizeof(EXPECTED[0]));

static const char *expectedSig(int id) {
  for (int i = 0; i < NUM_EXPECTED; i++)
    if ((int)EXPECTED[i].id == id) return EXPECTED[i].sig;
  return "(no format)";
}

TEST(i18n, no_language_has_gaps) {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    for (int id = 0; id < STR_COUNT; id++) {
      const char *s = T((StrId)id);
      char where[64];
      snprintf(where, sizeof(where), "[%s][id %d]", LANG_NAME[lang], id);
      CHECK_MSG(s != NULL, where);
      if (s) CHECK_MSG(s[0] != 0, std::string("empty string ") + where);
    }
  }
  gLang = LANG_DEFAULT;
}

// The firmware font (glcdfont.h from Arduino_GFX) is a CP437 table of 256
// glyphs, so it DOES support accents, but each one must take ONE byte: the
// UI centers with strlen(t)*6, and a two-byte UTF-8 character would throw the
// label off center. This test does not forbid high bytes, it forbids UTF-8.
TEST(i18n, accents_are_a_single_byte_not_utf8) {
  // lead bytes of multibyte UTF-8 sequences: 0xC2-0xDF, 0xE0-0xEF, 0xF0-0xF4
  auto utf8Lead = [](unsigned char c) {
    return (c >= 0xC2 && c <= 0xDF) || (c >= 0xE0 && c <= 0xEF) || (c >= 0xF0 && c <= 0xF4);
  };
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    if (LANG_IS_CJK(lang)) continue;  // CJK is UTF-8 on purpose, with a U8g2 font
    gLang = (Lang)lang;
    for (int id = 0; id < STR_COUNT; id++) {
      const char *s = T((StrId)id);
      if (!s) continue;
      for (const char *c = s; *c; c++) {
        unsigned char u = (unsigned char)*c;
        if (u < 128) continue;                 // ASCII: always fine
        if (!utf8Lead(u)) continue;        // lone high byte: it is CP437, fine
        // a UTF-8 lead byte is only suspicious if a continuation byte
        // (0x80-0xBF) follows it: then it is two glyphs where there should be one
        unsigned char sig = (unsigned char)*(c + 1);
        char msg[200];
        snprintf(msg, sizeof(msg),
                 "looks like UTF-8 (0x%02X 0x%02X) in [%s][id %d]: accents go in octal, "
                 "one byte per character, or the centering is thrown off", u, sig, LANG_NAME[lang], id);
        CHECK_MSG(!(sig >= 0x80 && sig <= 0xBF), msg);
      }
    }
  }
  gLang = LANG_DEFAULT;
}

// an extra or missing %u in a translation reads garbage off the stack
TEST(i18n, formats_match_in_all_languages) {
  for (int id = 0; id < STR_COUNT; id++) {
    gLang = LANG_EN;
    std::vector<std::string> ref = specs(T((StrId)id));
    for (int lang = 0; lang < LANG_COUNT; lang++) {
      gLang = (Lang)lang;
      std::vector<std::string> got = specs(T((StrId)id));
      char msg[256];
      snprintf(msg, sizeof(msg), "[%s][id %d] \"%s\": %s instead of %s (EN)", LANG_NAME[lang], id,
               T((StrId)id), joinSpecs(got).c_str(), joinSpecs(ref).c_str());
      CHECK_MSG(got == ref, msg);
    }
  }
  gLang = LANG_DEFAULT;
}

// ...and they must also match what the sketch passes in each call
TEST(i18n, formats_match_the_sketchs_calls) {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    for (int id = 0; id < STR_COUNT; id++) {
      std::string got = joinSpecs(specs(T((StrId)id)));
      const char *want = expectedSig(id);
      char msg[256];
      snprintf(msg, sizeof(msg), "[%s][id %d] \"%s\": %s, expected %s", LANG_NAME[lang], id,
               T((StrId)id), got.c_str(), want);
      CHECK_MSG(got == want, msg);
    }
  }
  gLang = LANG_DEFAULT;
}

TEST(i18n, medals_exist_in_all_languages) {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    // the length caps are in BYTES, which only equal characters with the
    // CP437 font. In CJK each kana takes 3 bytes and the real limit is the width
    // on screen, which cannot be measured here: that is checked on the board.
    const bool cjk = LANG_IS_CJK(lang);
    for (int m = 0; m < MED_COUNT; m++) {
      const char *n = medalName(m), *l = medalLabel(m), *d = medalDesc(m);
      char where[64];
      snprintf(where, sizeof(where), "[%s][medal %d]", LANG_NAME[lang], m);
      CHECK_MSG(n && n[0], std::string("empty medalName ") + where);
      CHECK_MSG(l && l[0], std::string("empty medalLabel ") + where);
      CHECK_MSG(d && d[0], std::string("empty medalDesc ") + where);
      // the short label goes in a cell of the medal grid
      if (!cjk && l) CHECK_MSG(strlen(l) <= 6, std::string("label too long ") + where + ": " + l);
      if (!cjk && n) CHECK_MSG(strlen(n) <= 12, std::string("name too long ") + where + ": " + n);
      if (!cjk && d) CHECK_MSG(strlen(d) <= 16, std::string("description too long ") + where + ": " + d);
    }
  }
  gLang = LANG_DEFAULT;
}

TEST(i18n, medals_have_no_accents) {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    if (LANG_IS_CJK(lang)) continue;  // CJK is UTF-8 with a U8g2 font
    gLang = (Lang)lang;
    for (int m = 0; m < MED_COUNT; m++) {
      const char *tab[3] = { medalName(m), medalLabel(m), medalDesc(m) };
      for (int t = 0; t < 3; t++)
        for (const char *c = tab[t]; c && *c; c++)
          CHECK_MSG((unsigned char)*c < 128, std::string("non-ASCII: ") + tab[t]);
    }
  }
  gLang = LANG_DEFAULT;
}

TEST(i18n, language_is_saved_and_restored) {
  mockNvsReset();
  setLang(LANG_DE);
  CHECK_EQ((int)gLang, (int)LANG_DE);
  gLang = LANG_ES;
  loadLang();
  CHECK_EQ((int)gLang, (int)LANG_DE);
  gLang = LANG_DEFAULT;
}

TEST(i18n, without_a_saved_setting_starts_in_the_default_language) {
  mockNvsReset();
  gLang = LANG_IT;
  loadLang();
  CHECK_EQ((int)gLang, (int)LANG_DEFAULT);
  gLang = LANG_DEFAULT;
}

TEST(i18n, invalid_language_in_nvs_breaks_nothing) {
  mockNvsReset();
  Preferences p;
  p.begin("tamapoke", false);
  p.putUChar("lang", 200);  // corrupt value
  p.end();
  gLang = LANG_ES;
  loadLang();
  CHECK_EQ((int)gLang, (int)LANG_DEFAULT);
  gLang = LANG_DEFAULT;
}

TEST(i18n, setLang_rejects_out_of_range_values) {
  mockNvsReset();
  setLang(LANG_FR);
  setLang((Lang)LANG_COUNT);
  CHECK_EQ((int)gLang, (int)LANG_FR);
  setLang((Lang)250);
  CHECK_EQ((int)gLang, (int)LANG_FR);
  gLang = LANG_DEFAULT;
}
