#pragma once
#include <Arduino.h>
#include <Preferences.h>

// 1 tick = 1 game minute. Lower this value to test faster
// (e.g. 5000UL = stats drop 12x faster).
#define PET_TICK_MS 60000UL
// Game minutes per level. With 60, CHARMANDER evolves at ~16 h
// of play with perfect care. Lower to 1 to see evolutions instantly.
#define MINUTES_PER_LEVEL 60
#define BTL_WIN_TRAIN 6      // ATK per battle won
#define BTL_ENERGY_COST 12
#define BTL_LOSS_ENERGY 5    // losing leaves it exhausted: it must sleep to fight again
#define BTL_LOSS_GRACE_MIN 10  // minutes to put it to bed before it counts as neglect
#define BTL_MIN_ENERGY 15    // below this it won't fight
#define EAT_ANIM_MS 2500UL
#define HEART_MS 1500UL
#define PET_JOY_TAPS 3                 // pets per hour that raise happiness
#define PET_JOY_WINDOW_MS 3600000UL
#define CANDY_PER_DAY 2
// weight: every meal adds some, eating while already full adds more; it burns slowly and with exercise
#define WGT_BERRY 4
#define WGT_CANDY 12
#define WGT_OVERFULL 6           // extra if it was already this full or more when it ate:
#define WGT_FULL_AT 80
#define WGT_BURN_MIN 6           // awake: -1 every 6 minutes (asleep: every 12)
#define WGT_FIT 20               // the Fit medal: this or less
#define EVOLVE_ANIM_MS 5200UL              // evolution animation (longer = more epic)
#define CEREMONY_MS 10000UL                // duration of the goodbye on screen
#define FAREWELL_AGE_MIN (3UL * 24 * 60)   // says goodbye after 3 game days (in final form)
#define SHINY_AFTER_GOODBYE 4              // 1 in 4 after a goodbye or release
#define RUNAWAY_TICKS 60                   // runs away after 1 h with EVERYTHING at zero

// milliseconds left until `deadline` (0 if already passed, or if deadline==0:
// "timer inactive"). All game timers (dialogs,
// animations) are stored as an absolute instant "deadline = millis() +
// duration" and were compared with millis() < deadline / millis() > deadline;
// that breaks when millis() wraps around at ~49.7 days of uptime. The
// unsigned subtraction, reinterpreted as signed, is safe against that
// wrap as long as the real interval is far below ~24.8 days (here the
// longest timer is ~12 s), without changing how the deadline is stored.
static inline uint32_t timeLeft(uint32_t deadline) {
  if (!deadline) return 0;
  int32_t left = (int32_t)(deadline - millis());
  return left > 0 ? (uint32_t)left : 0;
}

// end-of-cycle ceremonies
enum : uint8_t { CER_NONE = 0, CER_FAREWELL, CER_RUNAWAY, CER_RELEASE };

enum PetMood : uint8_t { MOOD_HAPPY, MOOD_SAD, MOOD_EATING, MOOD_SLEEPING };

// individual's medals (bitmask)
enum : uint32_t {
  MED_LV10 = 1 << 0, MED_LV25 = 1 << 1, MED_LV50 = 1 << 2,
  MED_BERRY = 1 << 3, MED_STREAK7 = 1 << 4, MED_BOND = 1 << 5,
  MED_FINAL = 1 << 6, MED_FIT = 1 << 7,
  // the trainer's: kept across every pet raised (trainerMedals)
  MED_SHINY = 1 << 8, MED_DEX10 = 1 << 9, MED_DEX50 = 1 << 10, MED_DEX100 = 1 << 11,
  MED_DEX151 = 1 << 12, MED_LV100 = 1 << 13,
  MED_BUDDY = 1 << 14, MED_PARTY = 1 << 15,  // a buddy on screen; a party of 5 Pokemon on screen
  MED_PLAYMATE = 1UL << 16,                  // played a game with a buddy (Who's That? Showdown)
  // a zone completed: every Pokemon in it caught, the legendary too (in zone order)
  MED_ZONE_FOREST = 1UL << 17, MED_ZONE_BEACH = 1UL << 18, MED_ZONE_CAVE = 1UL << 19,
  MED_ZONE_MANSION = 1UL << 20, MED_ZONE_FIELD = 1UL << 21,
};
#define MED_PET_COUNT 8  // medals 0..7 belong to the individual, the rest to the trainer
#define MED_FIRST_ZONE 17  // MED_ZONE_FOREST's bit: the zone medals have their own card page
#define MED_COUNT 22
#define PARTY_SIZE 5  // MED_PARTY: yours and 4 buddies at once

class Pet {
public:
  // Stats 0..100
  uint8_t fullness = 80;  // food
  uint8_t joy = 80;       // happiness
  uint8_t energy = 80;    // energy
  uint8_t hygiene = 100;  // cleanliness
  uint8_t poops = 0;      // poops on screen (max 3)
  uint8_t weight = 0;     // 0-100: treats fatten, the minigame burns
  // genes (90-110%, rolled at hatching) and training (0-100)
  uint8_t geneAtk = 100, geneDef = 100, geneSpe = 100;
  uint8_t trAtk = 0, trDef = 0, trSpe = 0;
  bool berryKnown = false;  // already discovered its favorite berry
  bool shiny = false;       // rare color variant (rolled in the egg)
  uint32_t ageMinutes = 0;
  int16_t speciesId = -1;      // Pokedex number (1-151), -1 = egg
  int16_t prevSpeciesId = -1;  // for the evolution animation
  uint8_t careMistakes = 0;   // neglects: each delays evolution by 1 level
  bool sleeping = false;
  uint32_t lastSeenEpoch = 0;   // last RTC time seen (for offline progression)
  uint8_t ceremony = CER_NONE;  // goodbye/runaway/release in progress
  uint8_t lastEnd = CER_NONE;   // how the previous one ended (affects the egg)
  uint8_t dexReg[19] = { 0 };       // pokedex of raised ones (151-bit bitmap)
  uint8_t dexShinyReg[19] = { 0 };  // the shiny form is in the Pokedex (raised or caught)
  uint8_t dexShinyOnly[19] = { 0 }; // ...and only the shiny one: the regular form isn't
  // daily care streak (the player's: persists across raisings)
  uint16_t streak = 0, bestStreak = 0;
  uint32_t lastCareDay = 0;
  // bond (the critter's: rises slowly with care, resets when another is born)
  uint8_t bond = 0;
  char nick[12] = "";    // nickname (empty = species name)
  // medals: the individual's + cumulative counter across all raisings
  uint16_t medals = 0, totalMedals = 0;
  uint32_t trainerMedals = 0;  // MED_SHINY..MED_PLAYMATE: never reset by a new pet
  uint32_t newMedal = 0;   // just earned, to celebrate
  uint16_t lastMilestone = 0;  // streak milestone already celebrated
  uint16_t gameHi = 0;     // minigame record (the player's)
  uint16_t runHi = 0;      // PokeRun record, in seconds
  uint8_t petJoyTaps = 0;  // pets that have counted this hour (not saved)
  uint32_t petJoyStart = 0;
  uint8_t candyToday = 0;  // treats given on day candyDay
  uint32_t candyDay = 0;
  uint8_t dungeonHi = 0;   // deepest floor of PokeDungeon
  uint16_t battleWins = 0; // battles won by this individual
  uint32_t visitorDay = 0;  // last day with a visit
  uint8_t movesSeen = 1;   // moves already announced (to announce the new one only once)

  void begin();                 // loads state from NVS (or creates the first egg)
  void update(uint32_t nowMs);  // call every loop()

  // Actions (touch buttons)
  void feed();              // red berry (compatibility)
  void feedBerry(uint8_t color);  // 0 red, 1 blue, 2 green
  void feedCandy();
  void gainWeight(uint8_t base);
  bool lovesBerry(uint8_t color) const {
    return !isEgg() && (speciesId % 3) == color;  // hidden taste per species
  }
  void playResult(uint8_t score);  // minigame reward (trains SPE)
  void runResult(uint16_t secs);   // PokeRun: same, every 4 s counts as one point
  void dungeonResult(uint8_t floor);
  bool canHaveCandy() const {  // without a clock it isn't known yet whether it's another day: keeps counting the last one
    return (today() != 0 && today() != candyDay) || candyToday < CANDY_PER_DAY;
  }
  void friendlyBattle(bool won);  // against a buddy: cheers up, but doesn't tire or train
  // battle: trains ATK and tires. Returns the ATK gain.
  uint8_t battleResult(bool won);
  bool catchWild(int16_t dex, bool shiny = false);  // catch: registers that form; true if new
  // The Pokedex keeps the regular and the shiny form of each species apart: both can be had,
  // and each is traded on its own. A form is (dex, shiny).
  bool hasForm(int16_t dex, bool shiny) const {
    return shiny ? isShinyRegistered(dex) : isRegistered(dex) && !dexBit(dexShinyOnly, dex);
  }
  uint16_t formCount() const;                           // forms in the Pokedex
  int16_t formAt(int idx, bool &shiny) const;           // the idx-th form, by dex number, regular first
  // trading with a buddy: the form you give leaves the pokedex, theirs comes in (true if new).
  // The form you are raising can't be traded.
  bool canTrade(int16_t dex, bool shiny = false) const {
    return hasForm(dex, shiny) && !(dex == speciesId && shiny == this->shiny);
  }
  uint16_t tradeableCount() const { return formCount() - (hasForm(speciesId, shiny) ? 1 : 0); }
  bool tradeAway(int16_t give, int16_t get, bool giveShiny = false, bool getShiny = false);
  void quizResult(uint8_t correct);      // who's that pokemon
  void showdownResult(uint8_t mine, uint8_t theirs);  // Who's That? Showdown with a buddy
  bool visitorDueNow() const;            // a visit is due today
  void visitorArrived();                 // records the day: doesn't repeat even after a restart
  void visitorLeft();                    // the visit cheers it up
  bool noSave = false;                   // board tests: nothing reaches the NVS
  int16_t pickWildSpecies(uint8_t lvl);  // rival: bias toward what's missing from the pokedex
  int16_t pickZoneSpecies(uint8_t zone, uint8_t lvl);  // same, within a zone
  int zoneCaught(uint8_t zone) const;    // how many of the zone are in the pokedex

  // battle stats: real gen 1 base x genes + level + training
  uint16_t atkStat() const;
  uint16_t defStat() const;
  uint16_t speStat() const;
  void play();
  void toggleLight();  // sleep / wake
  void clean();
  void caress();  // touch the critter
  void eggTap();  // touch the egg: 3 touches and it hatches
  void newEgg();   // start over with a random starter
  void release();  // release (long press + confirm)
  void syncClock(uint32_t nowEpoch);  // applies the time elapsed while off
  void setClock(uint32_t nowEpoch);   // sets the time without applying progression
  void startFarewell();  // also usable from the serial console (BYE)
  void startRunaway();   // also usable from the serial console (RUN)
  void rescue();         // cares for it all at once: cancels the runaway

  bool isEgg() const { return speciesId < 0; }
  uint8_t eggCracks() const { return eggTaps; }
  bool eating() const { return timeLeft(eatUntil) > 0; }
  bool showHeart() const { return timeLeft(heartUntil) > 0; }
  bool evolving() const { return timeLeft(evolveUntil) > 0; }
  float evolveT() const {     // evolution animation progress 0..1
    return 1.0f - (float)timeLeft(evolveUntil) / (float)EVOLVE_ANIM_MS;
  }
  bool canEvolveNow() const;  // evolution conditions met (ready)
  void evolve();              // triggers the transformation (called by a user touch)
  bool canFarewellNow() const;  // final form + 7 days: ready to say goodbye (button)
  bool canRunawayNow() const;   // total abandonment 1h: ready to run away (sad button)
  // the user decides in a dialog; "keep/stay together" postpones and re-offers later
  bool wantEvolveButton() const { return canEvolveNow() && level() > evoDeclinedLv; }
  bool wantFarewellButton() const { return canFarewellNow() && ageMinutes >= farDeclinedAge; }
  void declineEvolve() { evoDeclinedLv = level(); }              // re-offers on level up
  void declineFarewell() { farDeclinedAge = ageMinutes + 1440; } // re-offers within 1 day
  // first game: the player picks a starter (Bulbasaur/Charmander/Squirtle)
  bool awaitingStarter() const { return starterPick; }
  void chooseStarter(int16_t dex) { eggTarget = dex; starterPick = eggPick = false; save(); }
  // after a goodbye or release the next egg is chosen from those already registered
  bool awaitingEggPick() const { return eggPick; }
  bool chooseEgg(int16_t dex);
  int16_t registeredAt(int idx) const;  // the idx-th registered by dex number, 0 if none
  void factoryReset() { prefs.clear(); }  // wipes the NVS (test: serial command WIPE)
  void dbgRunawayReady() { fullness = joy = energy = hygiene = 0; neglectTicks = RUNAWAY_TICKS; }  // test
  // uint16_t, not uint8_t: with MINUTES_PER_LEVEL=60 the level is hours, so
  // a uint8_t wraps to 0 at level 256, after ~10.6 days. And that is
  // reached in normal play, because "stay together" allows postponing the
  // goodbye indefinitely. It is capped at 999 instead of overflowing.
  uint16_t level() const {
    uint32_t lv = 1 + ageMinutes / MINUTES_PER_LEVEL;
    return lv > 999 ? 999 : (uint16_t)lv;
  }
  bool isRegistered(int16_t dex) const {
    return dex >= 1 && dex <= 151 && (dexReg[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  bool isShinyRegistered(int16_t dex) const { return dexBit(dexShinyReg, dex); }
  static bool dexBit(const uint8_t *bits, int16_t dex) {
    return dex >= 1 && dex <= 151 && (bits[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  uint16_t registeredCount() const;
  bool lineHasUnregistered(int16_t base) const;
  uint8_t eggRarity() const;       // current egg's rarity (without revealing the species)
  int16_t pickEggSpecies();        // public so rolls can be simulated (EGGS)
  uint8_t lowestStat() const { return min(min(fullness, joy), min(energy, hygiene)); }
  PetMood mood() const;
  // progress of the goodbye/runaway ceremony, 0..1 (to animate it)
  float ceremonyT() const {
    if (ceremony == CER_NONE) return 0.0f;
    return 1.0f - (float)timeLeft(ceremonyUntil) / (float)CEREMONY_MS;
  }

  // streak / bond / medals / name
  void rename(const char *name);
  void buddiesOnScreen(uint8_t n);  // buddies showing right now: MED_BUDDY, MED_PARTY
  bool hasMedal(uint32_t m) const { return (medals | trainerMedals) & m; }
  bool showMedal() const { return timeLeft(medalUntil) > 0; }
  bool showMilestone() const { return timeLeft(milestoneUntil) > 0; }
  int careBonus() const;  // egg improvement from streak + bond

  // deferred periodic save: tick() flags it pending and the loop flushes it
  // when the screen is dimmed/off (the flash write freezes
  // both cores for ~1s: this way it isn't visible and doesn't cut off the touch)
  bool savePending() const { return pendingSave; }
  // last real time persisted; used to reseed an RTC that lost the time
  uint32_t savedEpoch() { return prefs.getUInt("seen", 0); }
  void flushSave();

private:
  void exerciseReward(uint8_t score, bool costsFood = true);
  Preferences prefs;
  uint32_t lastTick = 0;
  uint32_t eatUntil = 0;
  uint32_t heartUntil = 0;
  uint32_t evolveUntil = 0;
  int16_t eggTarget = 1;       // hidden dex that will come out of the egg
  bool eggShiny = false;       // surprise rolled when the egg is created
  uint8_t eggTaps = 0;
  uint8_t mistakeCooldown = 0;
  uint8_t ticksSinceSave = 0;
  bool pendingSave = false;     // periodic save pending flush
  uint16_t evoDeclinedLv = 0;   // "keep form": don't offer evolution until leveling up
  uint32_t farDeclinedAge = 0;  // "stay together": don't offer goodbye until this age
  bool starterPick = false;     // first game: waiting for the player to pick a starter
  bool eggPick = false;         // new egg: waiting for the player to pick from the pokedex
  uint8_t neglectTicks = 0;
  uint16_t goodTicks = 0;  // well-cared streak: forges DEF
  uint32_t ceremonyUntil = 0;
  uint8_t bondToday = 0;       // daily cap on bond gain
  uint32_t medalUntil = 0;     // medal celebration on screen
  uint8_t buddyPeak = 0;       // most buddies on screen since boot (the medals keep the record)
  uint32_t milestoneUntil = 0; // streak milestone celebration

  uint32_t today() const { return lastSeenEpoch ? lastSeenEpoch / 86400 : 0; }
  void registerCare();   // first care of the day: streak + bond
  void addBond(uint8_t amt);
  void checkMedals();
  void tick();
  void hatch();
  void registerSpecies(int16_t dex);
  bool addForm(int16_t dex, bool shinyForm);
  void removeForm(int16_t dex, bool shinyForm);
  void save();
  void load();
  static uint8_t clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
};
