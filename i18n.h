#pragma once
#include <Arduino.h>
#include "ui_v2.h"

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
#if UI_V2
#define STRID_BASE uint16_t  // V2: more than 256 strings
#else
#define STRID_BASE uint8_t
#endif
enum StrId : STRID_BASE {
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
  S_BUDDIES_FMT,
  S_PLAY_BTN, S_PLAY_TOGETHER, S_SHOWDOWN, S_BUDDY_INV_PLAY_FMT, S_SD_READY, S_SD_YOU_LBL, S_SD_YOU_GOT,
  S_SD_THEY_GOT_FMT, S_SD_NOBODY, S_SD_WIN, S_SD_LOSE_FMT, S_SD_DRAW, S_GOT_AWAY_FMT,
  S_VOLLEY, S_VB_SERVE, S_VB_POINT, S_VB_THEY_SCORE_FMT,
  S_ZONE_MEDALS_FMT, S_SHINY_WILD_FMT,
  S_TRAINING, S_TRAIN_HOW_ATK, S_TRAIN_HOW_DEF, S_TRAIN_HOW_SPE, S_TRAIN_HOW_WGT,
  S_PARTY_BOOST, S_BOOST_ENERGY, S_VISIT_MIN_FMT, S_STAT_HP, S_VISIT_CANDY_FMT, S_FAREWELL_HINT1, S_FAREWELL_HINT2, S_BTL_NO_ESCAPE,
  S_LV100_FMT, S_RANDOM_BATTLE,
  S_STARS_BETTER, S_STARS_YOURS_BETTER, S_STARS_SAME, S_STARS_REPLACES, S_STARS_TO_WILD,
  S_DEX_TITLE, S_PAGE_OF_FMT,  // "POKEDEX", "Page %d of %d"
  S_TRAVEL,  // the zones' screen: "Travel"
  S_CLOCK, S_SET_BY_WIFI, S_SET_AUTO, S_TIME_ZONE,  // the time page when the network sets it; the zone picker
  // each game's how-to-play card, wrapped at the spaces: the ways to play it and the point. _KEYS: on a
  // computer or the web (the arrow keys too); the others the Knomi's. Neither names a device
  S_HOW_BERRY, S_HOW_BERRY_KEYS, S_HOW_WHO, S_HOW_RUN, S_HOW_DN, S_HOW_DN_KEYS, S_HOW_SURF, S_HOW_SURF_KEYS,
  S_GAME_SURF,  // "PikaSurf"
  S_MEDAL_GOT, S_MEDAL_TODO,  // a medal's card: earned, or not yet
  S_READY_GO,  // a game's how-to-play card: its button
  S_DUEL_LOSE, S_DUEL_WAIT_FMT, S_DUEL_CRASHED_FMT,  // PikaSurf with a buddy: out first; waiting; they're out
  S_HOW_VB, S_HOW_VB_KEYS, S_HOW_SD, S_HOW_DUEL, S_HOW_DUEL_KEYS,  // Play together's cards, like the games' ones
  S_AUTO_UPDATE,  // settings: the switch that lets a new version go in by itself
#if UI_V2
  S_RESET_BTN, S_RESET_WARN,  // V2: the clock page's START OVER, and what "Start over?" erases
  S_CART_VOLLEY, S_CART_SHOWDOWN, S_PLAY_WITH_FMT,  // V2: Play together's cartridge titles ("\n" breaks a line), "with %s"
  S_PLAY_AGAIN,  // V2: the button at the end of a game
  S_TAP_ONE, S_CHOOSE_Q_FMT,  // V2: the starter screen's line under its board, and "Choose PIKACHU?"
  S_THANKS_DAYS_FMT,  // V2: under a goodbye's "Goodbye, GENGAR!" (the visitor's string): "Thank you for 4 days"
  S_RELEASE_WARN, S_RELEASE_BTN, S_KEEP_BTN,  // V2: the release question's line and its two buttons
  S_MASTER_GOT, S_MASTER_HOW,  // V2: the Master Ball earned ("Master Ball!"), and what it does
  S_REPEL, S_REPEL_ON_FMT,  // V2: the Repel's name, and "Repel: 3 min" when one is used
  S_BROKE_FREE_FMT,  // V2: it broke out of a Poke Ball, and there are more to throw
  S_ITEMS, S_MASTER, S_MASTER_EARN_FMT, S_REPEL_EARN_FMT,  // V2: the card's Items page, and how to earn each
  S_THROW_HINT_KEYS,  // V2: the catch's hint where the arrow keys aim too (a computer, the web)
  S_ST_GREAT, S_ST_HUM, S_ST_ROUND, S_ST_JOG, S_ST_RUMBLE, S_ST_SMELLY, S_ST_STINKS,  // V2: more lines under the name
  S_ST_NAP, S_ST_PLAY, S_ST_FULL, S_ST_CLEAN, S_ST_SLEEPY, S_ST_MORNING,             // (Fedor), taking turns
  S_RESET_SURE, S_RESET_NOUNDO,  // V2: ERASE EVERYTHING asks once more: "Are you sure?" "There's no going back."
  S_MASTER_DOES, S_REPEL_DOES_FMT,  // V2: what each does, on its own page of the card
  S_RAD_MEDALS, S_RAD_STATS,  // V2: the radial menu's Medals and Stats (Settings and the Pokedex are older)
  S_ULTRA, S_ULTRA_DOES, S_ULTRA_HOW,  // V2: the Ultra Ball: its name, what it does, and the line on the one earned
  S_ISLAND,  // V2: Travel's title, the island's name (Fedor: "Lets name the island Knomi Island")
#endif
  STR_COUNT
};

const char *T(StrId id);       // text in the active language
const char *medalName(int i);  // medal banner (MED_COUNT)
const char *medalLabel(int i); // short medal label
const char *medalDesc(int i);  // long medal description
const char *medalHow(int i);   // how to earn it, for its card

void loadLang();             // reads the language from NVS (call in setup)
void setLang(Lang l);        // changes and persists the language
