#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "ui_v2.h"
#include "savename.h"  // the save's storage name

// 1 tick = 1 game minute. Lower this value to test faster
// (e.g. 5000UL = stats drop 12x faster).
#define PET_TICK_MS 60000UL
// Game minutes per level. With 60, CHARMANDER evolves at ~16 h
// of play with perfect care. Lower to 1 to see evolutions instantly.
#define MINUTES_PER_LEVEL 60
#define GOOD_CARE_MIN 360  // 6 h of good care (sleep included) = +1 defense training
#define CARE_GRACE_MIN 15  // a need may stay under 40 this long (awake) before the streak is lost
#define VISIT_MINUTES 60     // the daily visitor stays this long (awake minutes, kept over sleep and restarts)
#define BTL_WIN_TRAIN 6      // ATK per battle won
#define BTL_ENERGY_COST 12
#define BTL_LOSS_ENERGY 5    // losing leaves it exhausted: it must sleep to fight again
#define BTL_LOSS_GRACE_MIN 10  // minutes to put it to bed before it counts as neglect
#define BTL_MIN_ENERGY 15    // below this it won't fight
#define EAT_ANIM_MS 2500UL
#define HEART_MS 1500UL
#define PARTY_ENERGY_SKIP_MIN 4   // Party Boost: energy skips one minute in this many (25% less drain)
#define PARTY_ENERGY_SAVE_PCT (100 / PARTY_ENERGY_SKIP_MIN)
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
#define CELEBRATE_MS 5000UL                // a new medal or streak milestone on screen
#define FAREWELL_AGE_MIN (3UL * 24 * 60)   // says goodbye after 3 game days (in final form)
#if UI_V2
#define MASTER_FIGHTS 100  // V2: fights won for a Master Ball (Fedor: REALLY rare)
#define ULTRA_FIGHTS 25    // ...and for an Ultra Ball (Fedor)
#define ULTRA_MAX 99
#define MASTER_MAX 99     // the most you can hold
#define REPEL_PLAY_S 600  // V2: seconds of minigames played for a Repel (Fedor: 10 minutes; it was 20, and 10 at first)
#define REPEL_MIN 3       // how long one lasts, in the game's minutes (Fedor: 3; 10 was long)
#define REPEL_MAX 99
#endif
#define SHINY_AFTER_GOODBYE 4              // 1 in 4 after a goodbye or release
#define RUNAWAY_TICKS 60                   // runs away after 1 h with EVERYTHING at zero
// Time (docs/HOW_IT_WORKS.md, "Time"): the game counts its own minutes, and only a clock the player
// can't set (network time, a monotonic clock) adds time away. A new day needs this many of them.
#define DAY_MIN_LIVED 720                  // 12 h lived since the last day began
#define AWAY_CAP_MIN (14UL * 24 * 60)      // the most time away applied at once: 2 weeks
#define CLOCK_SANE_UTC 1767225600UL        // 2026-01-01: an earlier UTC time can't be the network's

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
  // the end game: 3, 5 and 10 Pokemon raised to level 100 (lv100Count)
  MED_LV100_3 = 1UL << 22, MED_LV100_5 = 1UL << 23, MED_LV100_10 = 1UL << 24,
#if UI_V2
  // V2: Knomi Tower beaten, its Legend won (tower_v2.h); its badge goes in the case with the zones' badges
  MED_TOWER = 1UL << 25,
#endif
};
#define MED_PET_COUNT 8  // medals 0..7 belong to the individual, the rest to the trainer (V2: all kept, earned once)
#define MED_FIRST_ZONE 17  // MED_ZONE_FOREST's bit: the zone medals have their own card page
#define MED_FIRST_LV100 22  // MED_LV100_3's bit: the level 100 medals too
#if UI_V2
#define MED_COUNT 26  // (V2: with Knomi Tower's)
#else
#define MED_COUNT 25
#endif
#define LEVEL_MAX 100  // past it a Pokemon would be too strong to battle: it stays at 100
// Every form in the Pokedex has 1-3 stars: its genes, kept simple. A raised one's are the band of its
// genes' average (90-96, 97-103, 104-110); a catch rolls them: 1 star 50%, 2 stars 35%, 3 stars 15%
#define STAR_PCT_2 35
#define STAR_PCT_3 15
#define GENE_BAND_1 96   // the top gene of 1 star
#define GENE_BAND_2 103  // ...and of 2
enum CatchResult : uint8_t { CR_KEPT, CR_NEW, CR_BETTER };  // yours stays (as good or better), a new form, a better one
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
  uint16_t goodTicks = 0;  // well-cared streak, in minutes (asleep too): forges DEF
  uint8_t lowTicks = 0;    // awake minutes in a row with a need under 40 (forgiven up to CARE_GRACE_MIN)
  bool berryKnown = false;  // already discovered its favorite berry
  bool shiny = false;       // rare color variant (rolled in the egg)
  uint32_t ageMinutes = 0;
  int16_t speciesId = -1;      // Pokedex number (1-151), -1 = egg
  int16_t prevSpeciesId = -1;  // for the evolution animation
  uint8_t careMistakes = 0;   // neglects: each delays evolution by 1 level
  bool sleeping = false;
  uint32_t lastSeenEpoch = 0;   // last RTC time seen (local, as if UTC): the scene's hour and the day
  uint32_t lifeMin = 0;         // minutes the game has lived (live ticks + time away): the trainer's, never reset
  bool clockTrusted = false;    // the clock is network time (set by the sketch): a new day starts at its midnight
  uint8_t ceremony = CER_NONE;  // goodbye/runaway/release in progress
  uint8_t lastEnd = CER_NONE;   // how the previous one ended (affects the egg)
  uint8_t dexReg[19] = { 0 };       // pokedex of raised ones (151-bit bitmap)
  uint8_t dexShinyReg[19] = { 0 };  // the shiny form is in the Pokedex (raised or caught)
  uint8_t dexShinyOnly[19] = { 0 }; // ...and only the shiny one: the regular form isn't
  uint8_t dexStars[76] = { 0 };     // 2 bits per form, (dex - 1) * 2 + shiny: 1-3 stars, 0 = none
  // daily care streak (the player's: persists across raisings)
  uint16_t streak = 0, bestStreak = 0;
  uint32_t lastCareDay = 0;
  // bond (the critter's: rises slowly with care, resets when another is born)
  uint8_t bond = 0;
  char nick[12] = "";    // nickname (empty = species name)
  // medals: the individual's + cumulative counter across all raisings
  uint16_t medals = 0, totalMedals = 0;
  uint32_t trainerMedals = 0;  // MED_SHINY..MED_LV100_10: never reset by a new pet
  uint8_t lv100Count = 0;      // Pokemon raised to LEVEL_MAX, this one included once it gets there
  bool counted100 = false;     // this one is in lv100Count
  uint32_t newMedal = 0;   // just earned, to celebrate
  uint16_t lastMilestone = 0;  // streak milestone already celebrated
  uint16_t gameHi = 0;     // minigame record (the player's)
  uint16_t runHi = 0;      // PokeRun record, in seconds
  uint16_t surfHi = 0;     // PikaSurf record, in seconds
  uint8_t petJoyTaps = 0;  // pets that have counted this hour (not saved)
  uint32_t petJoyStart = 0;
  uint8_t candyToday = 0;  // treats given on day candyDay
  uint32_t candyDay = 0;
  uint32_t candyGiftDay = 0;  // the day the visitor brought one more candy
  uint8_t dungeonHi = 0;   // deepest floor of PokeDungeon
  uint16_t battleWins = 0; // battles won by this individual
  uint32_t visitorDay = 0;  // last day with a visit
  int16_t visitDex = 0;     // today's visitor, while it has minutes left
  uint8_t visitMin = 0;     // its minutes left: it steps out while yours sleeps and comes back
  bool partyBoost = false;  // a buddy or the visitor around (set by the sketch): energy drains a quarter slower
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
  void surfResult(uint16_t secs);  // PikaSurf: the same as PokeRun
  void dungeonResult(uint8_t floor);
  int candiesLeft() const {  // today's candies still to have (the visitor's included)
    int limit = CANDY_PER_DAY + (today() != 0 && candyGiftDay == today() ? 1 : 0);
    int had = (today() != 0 && today() != candyDay) ? 0 : candyToday;
    return had < limit ? limit - had : 0;
  }
  bool canHaveCandy() const {  // without a clock it isn't known yet whether it's another day: keeps counting the last one
    int limit = CANDY_PER_DAY + (today() != 0 && candyGiftDay == today() ? 1 : 0);
    return (today() != 0 && today() != candyDay) || candyToday < limit;
  }
  void friendlyBattle(bool won);  // against a buddy: cheers up, but doesn't tire or train
  // battle: trains ATK and tires. Returns the ATK gain.
  uint8_t battleResult(bool won);
  // a catch with these stars (0: rolled). A new form comes in with them; one you have takes them if
  // they're more, and otherwise stays as it is
  CatchResult catchWild(int16_t dex, bool shiny = false, uint8_t stars = 0);
  uint8_t starsOf(int16_t dex, bool shiny) const;
  uint8_t petStars() const { return starsFromGenes(geneAtk, geneDef, geneSpe); }
  static uint8_t starsFromGenes(int atk, int def, int spe);
  static uint8_t rollStars();
  void dbgSetStars(int16_t dex, bool shiny, uint8_t stars) { if (hasForm(dex, shiny) && stars >= 1 && stars <= 3) setStars(dex, shiny, stars); }  // test
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
  // getStars: the received form's, from the buddy (outside 1-3, as older versions send: rolled)
  bool tradeAway(int16_t give, int16_t get, bool giveShiny = false, bool getShiny = false, uint8_t getStars = 0);
  void quizResult(uint8_t correct);      // who's that pokemon
  void showdownResult(uint8_t mine, uint8_t theirs);  // Who's That? Showdown with a buddy
  bool visitorDueNow() const;            // a visit is due today
  void visitorArrived(int16_t dex);      // records the day and who: doesn't repeat even after a restart
  bool visitorResumable();               // a visitor stepped out (sleep, a restart) with minutes left today
  bool visitorMinute();                  // one more minute together: true when its time is up
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
  void syncClock(uint32_t nowEpoch);  // applies the time elapsed while off, by the saved local time (an RTC, older saves)
  void setClock(uint32_t nowEpoch);   // sets the time without applying progression
  void noteTime(uint32_t nowEpoch);   // the clock's local time now, without saving (the loop, every 30 s)
  // A clock the player can't set (NTP, a time server) says it is utc (localNow there): applies the minutes
  // the game missed since the last one (off, asleep, closed). measure false: just start counting from
  // here (the clock it replaces was a made-up seed, so there is nothing to measure).
  void trustedTime(uint32_t utc, uint32_t localNow, bool measure = true);
  void awayMinutes(uint32_t mins);    // a monotonic clock says the game missed these minutes (the host asleep)
  uint32_t dayNumber() const { return gameDay; }  // the game's day (today()), for the tests and the console
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
#if UI_V2
  // V2: "keep form" keeps it (Fedor: "For those that dont want to evolve ... it will be very annoying to getting
  // asked again"): this form's button doesn't come back by itself. Held (the release question), it has EVOLVE while
  // it can, and the hint says so for a few seconds. The next form is asked about once again, when it's ready
  bool wantEvolveButton() const { return canEvolveNow() && speciesId != evoKept; }
  bool wantFarewellButton() const { return canFarewellNow() && !farewellQuiet; }
  void declineEvolve() {
    evoKept = speciesId;
    evoHintUntil = millis() + 6000;
    pendingSave = true;  // (a restart soon after mustn't ask again)
  }
  int16_t evoKept = 0;          // the form "keep form" was chosen for (0: none); saved
  uint32_t evoHintUntil = 0;    // just kept: the home says how to evolve it later
#else
  bool wantEvolveButton() const { return canEvolveNow() && ageMinutes >= evoAskAgain; }
  bool wantFarewellButton() const { return canFarewellNow() && !farewellQuiet; }
  void declineEvolve() { evoAskAgain = ageMinutes + MINUTES_PER_LEVEL; }  // offered again an hour later
#endif
  void declineFarewell() {  // "stay together": not offered again (holding the pet brings it back)
    farewellQuiet = true;
    save();
  }
#if UI_V2
  // V2 (Fedor): fights pay. Every wild, Travel or tower battle won (a loss doesn't count) fills two bars a step:
  // ULTRA_FIGHTS of them earn an Ultra Ball, which triples the catch chance of a throw that hits, and MASTER_FIGHTS
  // a Master Ball, which always catches what it hits. They stack, and the bars and the balls are the trainer's:
  // they stay through a goodbye (START OVER erases them, as everything)
  uint8_t masterBar = 0, masterBalls = 0, ultraBar = 0, ultraBalls = 0;
  uint8_t fightPays();   // a fight won: what it earned, 1 a Master Ball, 2 an Ultra Ball (saved with it)
  bool masterFight() { return fightPays() & 1; }
  bool useMasterBall();  // one thrown that hit: false if there was none
  bool useUltraBall();
  // V2 (Fedor): the Repel, a spray in the backpack. For REPEL_MIN minutes, wild encounters keep weaker Pokemon away
  // (none below your level, nor any species weaker than yours), and in a Travel zone not yet complete every
  // non-legendary there is as likely (a complete one: as the wild). Shiny odds don't change. Earned by playing
  // minigames, REPEL_PLAY_S seconds each; they stack, and are the trainer's, as the Master Balls
  uint8_t repelHeld = 0;
  uint16_t repelPlaySec = 0;  // played toward the next
  uint32_t repelUntil = 0;    // the lifeMin it runs out at (0: none running)
  bool repelPlay(uint32_t ms);  // ms of a minigame played: true when it earned a Repel
  bool useRepel();              // from the backpack: false with none, or one already running
  bool repelOn() const { return repelUntil > lifeMin; }
  uint8_t repelMinutesLeft() const { return repelOn() ? (uint8_t)(repelUntil - lifeMin) : 0; }
  bool repelKeepsAway(int16_t dex) const;  // a running Repel keeps this species away from a wild encounter
  bool zoneComplete(uint8_t zone) const;
  // V2: Knomi Tower (tower.h). The trainer's, like the balls: kept through a goodbye. towerBest is the highest
  // floor ever won (9: the Legend, Tower Master); a badge per floor comes from it. The Hall of Fame keeps the
  // three that first won the Legend
  uint8_t towerBest = 0;
  bool towerHowSeen = false;          // the first visit's "how it works" card
  int16_t towerPick[2] = { 0, 0 };    // the two friends last picked, offered first next time
  bool towerPickShiny[2] = { false, false };
  int16_t hofDex[3] = { 0, 0, 0 };
  uint8_t hofShiny = 0, hofLevel = 0;  // hofShiny: a bit for each of the three
  uint32_t hofEpoch = 0;               // when (lastSeenEpoch, local time)
  uint8_t towerCheer = 0;  // climbs lost in a row since the last new best floor: the next climb's friends cheer (tower.h)
  // V2: the game's day this Pokemon came (hatch()). Its card's age and its goodbye's "Thank you for 4 days" count
  // calendar days from it (Fedor: "Lets make sure it counts days for real": they were its minutes lived / 1440, and
  // an absence counts at most 2 weeks). 0: not known yet (no clock when it came, a save from before): the next day
  // sets it, its age back from that day
  uint32_t metDay = 0;
  uint32_t daysHad() const { return metDay && gameDay >= metDay ? gameDay - metDay : ageMinutes / 1440; }  // 0 its first
  uint8_t towerChal = 0;   // the challenges done once the Legend is beaten (TOWER_CH_*)
  // a tower fight won: uses its energy; petFought: your own Pokemon won it, and gets a win's usual effects
  void towerFightWon(bool petFought);
  // floor (1..8, or the Legend's 9) climbed: the badge, and a gate's balls the first time. Returns what it gave:
  // bit 0 a new badge, 1 a Master Ball, then the Ultra Balls in bits 4..7
  uint8_t towerFloorWon(uint8_t floor);
  void towerLost();  // all three fainted: a loss, as a battle's (and the friends cheer the next climb, until the Legend)
  uint8_t towerChalDone(uint8_t got);  // a climb's challenges: the new ones (and they're kept)
  void towerSave() { save(); }  // a pick, the Hall of Fame: kept at once
#endif
  // first game: the player picks a starter (Bulbasaur/Charmander/Squirtle)
  bool awaitingStarter() const { return starterPick; }
#if UI_V2
  // V2: one of the four starters, every time until a goodbye has been offered (pickOk). No egg: it's yours at
  // once, its shiny form if you own it and picked it
  void chooseStarter(int16_t dex, bool shinyForm = false);
  bool canPickFromDex() const { return pickOk; }
#else
  void chooseStarter(int16_t dex) { eggTarget = dex; starterPick = eggPick = false; save(); }
#endif
  // after a goodbye or release the next egg is chosen from those already registered
  bool awaitingEggPick() const { return eggPick; }
  bool chooseEgg(int16_t dex, bool shinyForm = false);  // a shiny form hatches shiny
  int16_t registeredAt(int idx) const;  // the idx-th registered by dex number, 0 if none
  // what an egg can be: a form in the Pokedex (regular or shiny) of a base form (Pikachu, not
  // Raichu), since it hatches at level 1
  bool eggChoice(int16_t dex, bool shinyForm) const;
  int16_t eggChoiceAt(int idx, bool &shinyForm) const;  // the idx-th of those by dex number, regular first; 0 if none
  uint16_t eggChoiceCount() const;
#if UI_V2
  void factoryReset() { prefs.clear(); saveForgetOld(); }  // V2: and the save from before the rename
#else
  void factoryReset() { prefs.clear(); }  // wipes the NVS (test: serial command WIPE)
#endif
  void dbgRunawayReady() { fullness = joy = energy = hygiene = 0; neglectTicks = RUNAWAY_TICKS; }  // test
  // with MINUTES_PER_LEVEL=60 the level is hours of age; "stay together" keeps a Pokemon as long as
  // you like, so it would keep rising: it stops at LEVEL_MAX (a Pokemon already past it comes back to it)
  uint16_t level() const {
    uint32_t lv = 1 + ageMinutes / MINUTES_PER_LEVEL;
    return lv > LEVEL_MAX ? LEVEL_MAX : (uint16_t)lv;
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
  uint32_t celebrateLeft() const { return timeLeft(showMedal() ? medalUntil : milestoneUntil); }
  void endCelebration() { medalUntil = milestoneUntil = 0; }  // tapped: it has been seen
  // not on screen (a battle, a game, the Pokedex, the screen off...): it waits, whole, for the home screen
  void holdCelebration() {
    if (showMedal()) medalUntil = millis() + CELEBRATE_MS;
    if (showMilestone()) milestoneUntil = millis() + CELEBRATE_MS;
  }
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
  uint8_t eggStars = 0;        // picked from a form with these stars: its genes are rolled in that band (0: 90-110)
  uint8_t eggTaps = 0;
  uint8_t mistakeCooldown = 0;
  uint8_t ticksSinceSave = 0;
  bool pendingSave = false;     // periodic save pending flush
  uint32_t evoAskAgain = 0;     // "keep form": the age (minutes) when evolving is offered again. By age, not
                                // level: at the top level the level never rises again
  bool farewellQuiet = false;  // chose "stay together" once: the goodbye isn't offered by itself again
  bool starterPick = false;     // first game: waiting for the player to pick a starter
  bool eggPick = false;         // new egg: waiting for the player to pick from the pokedex
#if UI_V2
  // V2 (Fedor): a goodbye has been offered once (a final form 3 days old): every new Pokemon after that is picked
  // from the Pokedex, not from the four starters
  bool pickOk = false;
  uint16_t repelMs = 0;  // the part of a second played not counted yet (not saved)
#endif
  uint8_t neglectTicks = 0;
  void careMinute(bool asleep);
  uint32_t ceremonyUntil = 0;
  uint8_t bondToday = 0;       // daily cap on bond gain
  uint32_t medalUntil = 0;     // medal celebration on screen
  uint8_t buddyPeak = 0;       // most buddies on screen since boot (the medals keep the record)
  uint32_t milestoneUntil = 0; // streak milestone celebration

  // the day: follows the clock's, but never goes back, and only moves on after DAY_MIN_LIVED minutes lived,
  // so setting the clock can't make days (candies, visitors, the streak). 0: no clock yet
  uint32_t gameDay = 0;
  uint32_t dayLife = 0;          // lifeMin when the day began
  uint32_t refUtc = 0, refLife = 0;  // the last trusted time, and lifeMin then
  uint32_t bootSeen = 0, bootLife = 0;  // the saved local time and lifeMin at start (saves from before refUtc)
  uint32_t today() const { return gameDay; }
  void advanceDay();
  void applyAway(uint32_t mins);  // time away: the bars drop, it ages, maybe hatches
  void registerCare();   // first care of the day: streak + bond
  void addBond(uint8_t amt);
  void checkMedals();
  void tick();
  void hatch();
  void registerSpecies(int16_t dex);
  bool addForm(int16_t dex, bool shinyForm, uint8_t stars);  // a form you already have keeps the higher
  void removeForm(int16_t dex, bool shinyForm);
  void setStars(int16_t dex, bool shinyForm, uint8_t stars);
  void save();
  void load();
  static uint8_t clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
};
