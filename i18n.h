#pragma once
#include <Arduino.h>

// Supported languages. Arduino_GFX's 5x7 font is a full CP437 table
// (256 glyphs), so accents, umlauts and enyes DO exist: they go as a single byte in
// octal (\240 a, \244 n~, \202 e, \204 a"...), never in UTF-8, because the UI centers
// with strlen(t)*6 and a two-byte character would throw the labels off.
// Not in CP437: accented capitals except E (\220), nor Portuguese a~/o~.
enum Lang : uint8_t { LANG_ES = 0, LANG_EN, LANG_FR, LANG_DE, LANG_IT, LANG_PT,
                     LANG_JA, LANG_KO, LANG_COUNT };
// languages that need a U8g2 font and UTF-8 text instead of the CP437 table
#define LANG_IS_CJK(l) ((l) == LANG_JA || (l) == LANG_KO)
#define LANG_DEFAULT LANG_EN  // default language: English

extern Lang gLang;  // active language (defined in i18n.cpp)

// String IDs. The order must match the STRINGS table in i18n.cpp.
enum StrId : uint8_t {
  // pet status (statusMsg)
  S_EVOLVING, S_EATING, S_LIKES, S_HUNGRY, S_NEEDS_BATH,
  S_EXHAUSTED, S_SAD, S_CHUBBY, S_IS_SHINY, S_HAPPY,
  // goodbye ceremonies
  S_FAREWELL, S_RUNAWAY, S_GOODBYE,
  // egg
  S_EGG_HDR, S_EGG_LEGEND, S_EGG_RARE, S_EGG_TOUCH, S_EGG_MOVES, S_EGG_ALMOST,
  // shared formats
  S_POKEDEX_FMT,   // "POKEDEX %u/151"
  S_NAME_FMT,      // "%s%s Nv.%u"
  // release dialog
  S_RELEASE_FMT, S_YES, S_NO,
  // minigame and punching bag
  S_WINS_FMT, S_STR_GAIN_FMT, S_NEW_RECORD, S_RECORD_FMT, S_BTL_RUN,
  S_SCORE_FMT, S_GREAT_JOY, S_PLUS_JOY,
  // clock / settings
  S_SET_TIME, S_HOUR, S_MIN, S_CLOCK_CANCEL, S_LANG_LABEL,
  // celebration
  S_MEDAL_BANNER, S_GREAT, S_STREAK_DAYS_FMT,
  // info card: profile
  S_STREAK_FMT, S_VIN, S_BERRY_UNK, S_BERRY_RED, S_BERRY_BLUE, S_BERRY_GREEN,
  S_INFO_FMT, S_RENAME_HINT,
  // info card: battle
  S_BATTLE, S_STAT_ATK, S_STAT_DEF, S_STAT_SPE, S_STAT_WGT, S_BTL_START,
  // info card: medals
  S_MEDALS_FMT, S_BACK,
  // keyboard and gallery
  S_NAME, S_DETAIL_BACK,
  // bars
  S_BAR_FOOD, S_BAR_JOY, S_BAR_ENE, S_BAR_HYG,
  // live minigame scoreboard
  S_REC_FMT,
  // info card: progress page
  S_PROGRESS, S_LVL_FMT, S_NEXT_LVL_FMT, S_EVO_LABEL, S_FINAL_FORM,
  S_EVO_READY, S_EVO_BLOCKED, S_EVO_IN_FMT, S_MISTAKES_FMT,
  // sound switch (settings)
  S_SND_ON, S_SND_OFF,
  S_EVO_TAP,        // evolution button text
  S_FAREWELL_BTN,   // goodbye button text (carries the name: "%s ...")
  S_RUNAWAY_BTN,    // runaway-by-neglect button text (sad ending)
  // decision dialogs (evolve/keep, say goodbye/stay together)
  S_EVO_Q, S_EVO_KEEP, S_FAR_Q, S_FAR_GO, S_FAR_STAY,
  S_CHOOSE_STARTER,  // title of the starter choice (first time)
  S_NO_SPRITES, S_LOAD_SPRITES,  // notice when the sprite is missing from the SD
  // battle
  S_BTL_WILD, S_BTL_SUPER, S_BTL_WEAK, S_BTL_IMMUNE, S_BTL_WON, S_BTL_LOST, S_BTL_RAN,
  S_BTL_NEWDEX, S_BTL_LEARNED, S_BTL_TIRED,
  // delete the save (Knomi button)
  S_RESET_Q, S_RESET_GO, S_RESET_KEEP,
  // settings and wifi change (Knomi)
  S_SETTINGS, S_WIFI_CHANGE, S_WIFI_SYNCED, S_WIFI_NOCONN, S_WIFI_SEARCH, S_WIFI_TRYING, S_WIFI_SAVED,
  S_WIFI_KEPT, S_CANCEL,
  // games, catch and visit
  S_GAMES, S_GAME_TILT, S_GAME_WHO, S_WHO_Q, S_ITS_FMT, S_TILT_HINT, S_THROW_HINT,
  S_CAUGHT_FMT, S_MISSED, S_FLED_FMT, S_VISITOR, S_VISIT_BYE_FMT,
  S_GAME_RUN, S_TAP_JUMP, S_SWIPE_QUIT,
  S_CHOOSE_POKEMON, S_CHOOSE_BTN, S_BACK_BTN,
  S_RUNAWAY_Q, S_RUNAWAY_CARE, S_RUNAWAY_GO,
  S_GAME_DUNGEON, S_DN_BERRY,
  S_BUDDY_HERE_FMT, S_BUDDY_BATTLE, S_BUDDY_TRADE, S_BUDDY_INV_BTL_FMT, S_BUDDY_INV_TRD_FMT, S_BUDDY_WAITING,
  S_BUDDY_LEFT_FMT, S_BUDDY_BUSY_FMT, S_TRADE_PICK, S_TRADE_GIVE, S_TRADE_GET, S_TRADE_GOT_FMT,
  S_GAME_BATTLE, S_ZONE_FOREST, S_ZONE_BEACH, S_ZONE_CAVE, S_ZONE_MANSION, S_ZONE_FIELD, S_ZONE_SEARCH,
  S_ZONE_CAUGHT_FMT, S_ZONE_MISSING, S_SWIPE_BACK,
  S_TRAINER_FMT,
  S_UPD_CHECK, S_UPD_LATEST, S_UPD_TO_FMT, S_UPD_BUSY_FMT, S_UPD_FAILED,
  STR_COUNT
};

const char *T(StrId id);       // text in the active language
const char *medalName(int i);  // medal banner (MED_COUNT)
const char *medalLabel(int i); // short medal label
const char *medalDesc(int i);  // long medal description

void loadLang();             // reads the language from NVS (call in setup)
void setLang(Lang l);        // changes and persists the language
