#include "pet.h"
#include "battle.h"
#include "zones.h"
#include "minigames.h"
#include "dex.h"
#include "audio.h"

void Pet::begin() {
  prefs.begin("tamapoke", false);
  if (!prefs.getBool("init", false)) {
    prefs.putBool("init", true);
    newEgg();
  } else {
    load();
  }
  bootSeen = prefs.getUInt("seen", 0);
  bootLife = lifeMin;
  lastTick = millis();
}

void Pet::newEgg() {
  ceremony = CER_NONE;
  neglectTicks = 0;
  weight = 0;
  speciesId = -1;
  prevSpeciesId = -1;
  eggTarget = pickEggSpecies();  // hidden species based on rarity and pokedex
  starterPick = (registeredCount() == 0);  // first game: the player picks the starter
  eggPick = !starterPick && lastEnd != CER_RUNAWAY;  // after a runaway the egg is decided by the draw
  // shiny roll: 1/4 after a good goodbye; otherwise 1/48 base, better with high streak/bond
  int shinyBase = SHINY_AFTER_GOODBYE;
  if (lastEnd != CER_FAREWELL && lastEnd != CER_RELEASE) {
    shinyBase = 48 - careBonus();
    if (shinyBase < 8) shinyBase = 8;
  }
  eggShiny = (random(shinyBase) == 0);
  eggStars = 0;  // picking a form sets its band; a drawn egg rolls 90-110
  eggTaps = 0;
  fullness = 80;
  joy = 80;
  energy = 80;
  hygiene = 100;
  poops = 0;
  ageMinutes = 0;
  careMistakes = 0;
  mistakeCooldown = 0;
  sleeping = false;
  save();
}

// offline progression: time passed even while it was off, but with
// mercy: bars drop with a floor of 15 (it comes back hungry, not dead),
// with no neglect or runaways while away
static uint8_t dropTo(uint8_t v, uint8_t d, uint8_t fl) {
  if (v <= fl) return v;
  return (v - fl > d) ? v - d : fl;
}

void Pet::setClock(uint32_t nowEpoch) {
  lastSeenEpoch = nowEpoch;
  advanceDay();
  if (nowEpoch) save();  // saved right away: a power cut doesn't lose the reference
}

void Pet::noteTime(uint32_t nowEpoch) {
  if (!nowEpoch) return;
  lastSeenEpoch = nowEpoch;
  advanceDay();
}

// The day follows the clock's, with three guards, so setting the clock (the time screen, the computer's)
// can't hand out days: it never goes back; a new one needs DAY_MIN_LIVED minutes lived since the last
// began; and it skips ahead several only if the game lived through them. On network time the day is
// taken to begin at midnight, so starting it at 2 pm doesn't push tomorrow's candies to 2 am.
void Pet::advanceDay() {
  if (!lastSeenEpoch) return;
  uint32_t d = lastSeenEpoch / 86400;
  if (d <= gameDay) return;
  bool first = gameDay == 0;
  if (!first) {
    uint32_t lived = lifeMin - dayLife;
    if (lived < DAY_MIN_LIVED) return;
    uint32_t most = gameDay + 1 + (lived - DAY_MIN_LIVED) / 1440;
    if (d > most) d = most;
  }
  bool clockDay = d == lastSeenEpoch / 86400;  // not held back: the clock's own day
  gameDay = d;
  dayLife = lifeMin - (clockDay && (first || clockTrusted) ? (lastSeenEpoch % 86400) / 60 : 0);
  pendingSave = true;
}

void Pet::syncClock(uint32_t nowEpoch) {
  uint32_t seen = prefs.getUInt("seen", 0);
  lastSeenEpoch = nowEpoch;
  if (nowEpoch == 0) return;
  applyAway((seen && nowEpoch > seen) ? (nowEpoch - seen) / 60 : 0);
}

void Pet::trustedTime(uint32_t utc, uint32_t localNow, bool measure) {
  if (utc < CLOCK_SANE_UTC) return;
  if (localNow) lastSeenEpoch = localNow;
  // minutes since the last trusted time, against those the game lived meanwhile (ticks, time away). A
  // save from before this was counted measures once from the local time it was saved at.
  uint32_t real = 0, lived = 0;
  if (refUtc) {
    real = utc > refUtc ? (utc - refUtc) / 60 : 0;
    lived = lifeMin - refLife;
  } else {
    real = (bootSeen && localNow > bootSeen) ? (localNow - bootSeen) / 60 : 0;
    lived = lifeMin - bootLife;
  }
  applyAway(measure && real > lived ? real - lived : 0);
  refUtc = utc;  // a game ahead of it (a minute of drift) starts over from here: nothing is taken back
  refLife = lifeMin;
  save();
}

void Pet::awayMinutes(uint32_t mins) {
  if (mins) applyAway(mins);
}

void Pet::applyAway(uint32_t mins) {
  lifeMin += mins;  // lived, even while it waits for a starter or says goodbye
  if (mins < 2 || ceremony != CER_NONE || starterPick || eggPick) {
    advanceDay();
    save();  // first time, no time to apply, or still choosing what to raise
    return;
  }
  if (mins > AWAY_CAP_MIN) mins = AWAY_CAP_MIN;

  for (uint32_t i = 0; i < mins; i++) {
    ageMinutes++;
    if (isEgg()) {
      if (ageMinutes >= 3) hatch();  // hatches while you were away
      continue;
    }
    if (sleeping) {  // rest: drops slowly with a floor, same as live
      energy = clamp100(energy + 6);
      if (ageMinutes % 2 == 0) {
        fullness = dropTo(fullness, 1, 30);
        joy = dropTo(joy, 1, 35);
      }
      if (ageMinutes % 3 == 0) hygiene = dropTo(hygiene, 1, 45);
      careMinute(true);
      continue;
    }
    fullness = dropTo(fullness, 2, 15);
    energy = dropTo(energy, 1, 15);
    hygiene = dropTo(hygiene, 1, 15);
    joy = dropTo(joy, 1, 15);
    careMinute(false);
  }
  if (!isEgg()) {
    if (!sleeping) {  // no pooping while sleeping
      uint8_t p = poops + mins / 240;
      poops = p > 3 ? 3 : p;
    }
    // evolution is NOT applied offline: it is left ready and the user triggers it
    // by tapping the critter when it returns (so they see the transformation)
  }
  Serial.printf("offline: %u min applied (lv.%u)\n", mins, level());
  advanceDay();
  save();
}

void Pet::update(uint32_t nowMs) {
  // end of ceremony: the creature leaves and a new egg remains
  if (ceremony != CER_NONE && !timeLeft(ceremonyUntil)) {
    newEgg();
    return;
  }
  while (nowMs - lastTick >= PET_TICK_MS) {
    lastTick += PET_TICK_MS;
    lifeMin++;  // even while choosing or saying goodbye: the trainer's time
    tick();
    advanceDay();
  }
}

// Good care forges defense: every GOOD_CARE_MIN minutes of it = +1 DEF. Sleep counts (needs dip
// overnight by design, and waking it starts the grace again); awake, a need under 40 is forgiven for
// CARE_GRACE_MIN minutes, about the time to notice and feed it, and then the streak starts over.
void Pet::careMinute(bool asleep) {
  if (asleep || lowestStat() >= 40) lowTicks = 0;
  else if (lowTicks <= CARE_GRACE_MIN) lowTicks++;
  if (lowTicks > CARE_GRACE_MIN) {
    goodTicks = 0;
    return;
  }
  if (++goodTicks >= GOOD_CARE_MIN) {
    goodTicks = 0;
    if (trDef < 100) trDef++;
  }
}

void Pet::tick() {
  if (ceremony != CER_NONE) return;  // time stops during the goodbye
  if (eggPick) return;      // same as when picking the starter: the egg has no species yet
  if (starterPick) return;  // the game doesn't start until a starter is picked: if
                            // time ran here, the egg would hatch on its own at
                            // 3 min with the drawn species and the player's
                            // choice would be lost
  ageMinutes++;

  if (isEgg()) {
    if (ageMinutes >= 3) hatch();  // if you don't touch it, it hatches on its own after 3 min
    return;
  }

  // sleep is rest: energy recovers and needs drop MUCH
  // slower than awake and with a floor (it wakes up wanting some cuddles, not at
  // zero, no neglect or runaways). awake: food -2/min, hyg/joy -1/min.
  // Weight still burns, and the good-care streak keeps counting.
  if (sleeping) {
    energy = clamp100(energy + 6);
    if (weight > 0 && ageMinutes % (2 * WGT_BURN_MIN) == 0) weight--;
    if (ageMinutes % 2 == 0) {                 // ~4x slower than awake
      fullness = dropTo(fullness, 1, 30);
      joy = dropTo(joy, 1, 35);
    }
    if (ageMinutes % 3 == 0) hygiene = dropTo(hygiene, 1, 45);
    careMinute(true);
    checkMedals();  // can still cross a level by age while sleeping
    if (++ticksSinceSave >= 5) pendingSave = true;
    return;
  }

  if (ageMinutes % MINUTES_PER_LEVEL == 0 && ageMinutes / MINUTES_PER_LEVEL < LEVEL_MAX) sfxPlay(SFX_LEVEL);  // leveled up (awake)

  fullness = clamp100(fullness - 2);
  // with company (Party Boost) energy drops a quarter slower: three minutes in four
  bool tires = !partyBoost || ageMinutes % PARTY_ENERGY_SKIP_MIN != 0;
  if (tires) energy = clamp100(energy - 1);
  if (fullness > 40 && poops < 3 && random(100) < 15) poops++;

  hygiene = clamp100(hygiene - 1 - 4 * poops);
  // overweight makes it lazy: energy drops twice as fast
  if (weight > 50 && tires) energy = clamp100(energy - 1);
  if (weight > 0 && ageMinutes % WGT_BURN_MIN == 0) weight--;

  careMinute(false);

  int dJoy = -1;
  if (fullness < 30) dJoy -= 2;
  if (hygiene < 30) dJoy -= 2;
  joy = clamp100(joy + dJoy);

  // Neglect: leaving a stat on the floor counts as a care
  // mistake (with a cooldown so the same neglect isn't counted every minute)
  if (mistakeCooldown > 0) mistakeCooldown--;
  if (lowestStat() <= 10 && mistakeCooldown == 0) {
    careMistakes++;
    mistakeCooldown = 60;
    if (bond > 1) bond--;  // neglect cools the bond, but without wiping it out:
                           // at -3 every 30 min it lost far more than could be
                           // gained in a whole day and the bond got stuck
  }

  checkMedals();  // evolution is triggered by the user (canEvolveNow + tap), not the tick

  // total abandonment: with EVERYTHING at zero for an hour it is ready to run away;
  // it does NOT leave on its own, the user triggers it with the button (sad ending, witnessed)
  if (fullness == 0 && joy == 0 && energy == 0 && hygiene == 0) {
    if (neglectTicks < RUNAWAY_TICKS) neglectTicks++;
  } else {
    neglectTicks = 0;  // a single act of care saves it
  }

  // full cycle (final form + 7 days): the farewell does NOT fire on its own; it is
  // ready (canFarewellNow) and the user triggers it with the button, so they see it

  // periodic autosave: do NOT write to flash here (runs inside the loop,
  // while animating); just flag it and let the loop flush it when dimming
  if (++ticksSinceSave >= 5) pendingSave = true;
}

// flushes the pending periodic save (called by the loop at a moment with no
// animation so the write stall to flash isn't visible)
void Pet::flushSave() {
  if (pendingSave) save();
}

// are there unregistered members left in this base's evolution line?
bool Pet::lineHasUnregistered(int16_t base) const {
  int16_t cur = base;
  for (int guard = 0; cur >= 1 && cur <= 151 && guard < 6; guard++) {
    if (!isRegistered(cur)) return true;
    if (cur == DEX_EEVEE) {
      for (int16_t b = 134; b <= 136; b++)
        if (!isRegistered(b)) return true;
      return false;
    }
    cur = DEX_TBL[cur].evolvesTo;
  }
  return false;
}

uint8_t Pet::eggRarity() const {
  return (eggTarget >= 1 && eggTarget <= 151) ? DEX_TBL[eggTarget].rarity : R_COMMON;
}

// picks the egg species: rarity roll (improved by a full goodbye,
// penalized by a runaway) and bias toward incomplete lines
int16_t Pet::pickEggSpecies() {
  // first game: classic starter
  if (registeredCount() == 0) {
    return CLASSIC_DEX[random(NUM_CLASSIC_DEX)];
  }

  uint8_t tier = R_COMMON;
  if (lastEnd != CER_RUNAWAY) {
    bool blessed = (lastEnd == CER_FAREWELL);
    int rare = (blessed ? 45 : 27) + careBonus();
    int leg = (registeredCount() >= 25) ? (blessed ? 10 : 3) + careBonus() / 3 : 0;
    int r = random(100);
    if (r < leg) tier = R_LEGENDARY;
    else if (r < leg + rare) tier = R_RARE;
  }

  // tier candidates with an incomplete line; if none, drop a tier;
  // if the tier's pokedex is complete, any of the tier will do
  for (int pass = 0; pass < 2; pass++) {
    for (int t = tier; t >= R_COMMON; t--) {
      int16_t cand[80];
      int n = 0;
      for (int16_t d = 1; d <= 151 && n < 80; d++) {
        if (DEX_TBL[d].rarity != t) continue;
        if (pass == 0 && !lineHasUnregistered(d)) continue;
        cand[n++] = d;
      }
      if (n > 0) return cand[random(n)];
    }
  }
  return CLASSIC_DEX[random(NUM_CLASSIC_DEX)];  // unreachable, just in case
}

bool Pet::chooseEgg(int16_t dex, bool shinyForm) {
  if (!eggPick || !eggChoice(dex, shinyForm)) return false;
  eggTarget = dex;
  if (shinyForm) eggShiny = true;  // otherwise the roll from newEgg() stays: a surprise
  eggStars = starsOf(dex, shinyForm);  // a 3-star form hatches a 3-star Pokemon
  eggPick = false;
  save();
  return true;
}

int16_t Pet::registeredAt(int idx) const {
  for (int16_t d = 1; d <= 151; d++)
    if (isRegistered(d) && idx-- == 0) return d;
  return 0;
}

bool Pet::eggChoice(int16_t dex, bool shinyForm) const {
  return hasForm(dex, shinyForm) && DEX_TBL[dex].rarity != R_EVO;
}

int16_t Pet::eggChoiceAt(int idx, bool &shinyForm) const {
  for (int16_t d = 1; d <= 151; d++)
    for (int f = 0; f < 2; f++)
      if (eggChoice(d, f) && idx-- == 0) {
        shinyForm = f;
        return d;
      }
  return 0;
}

uint16_t Pet::eggChoiceCount() const {
  uint16_t n = 0;
  for (int16_t d = 1; d <= 151; d++) n += eggChoice(d, false) + eggChoice(d, true);
  return n;
}

static void setDexBit(uint8_t *bits, int16_t dex, bool on) {
  uint8_t m = 1 << ((dex - 1) & 7);
  if (on) bits[(dex - 1) >> 3] |= m;
  else bits[(dex - 1) >> 3] &= ~m;
}

uint8_t Pet::starsOf(int16_t dex, bool shinyForm) const {
  if (dex < 1 || dex > 151) return 0;
  int i = (dex - 1) * 2 + (shinyForm ? 1 : 0);
  return (dexStars[i >> 2] >> ((i & 3) * 2)) & 3;
}

void Pet::setStars(int16_t dex, bool shinyForm, uint8_t stars) {
  if (dex < 1 || dex > 151) return;
  int i = (dex - 1) * 2 + (shinyForm ? 1 : 0), sh = (i & 3) * 2;
  dexStars[i >> 2] = (uint8_t)((dexStars[i >> 2] & ~(3 << sh)) | ((stars & 3) << sh));
}

uint8_t Pet::starsFromGenes(int atk, int def, int spe) {
  int avg = (atk + def + spe + 1) / 3;  // rounded
  return avg <= GENE_BAND_1 ? 1 : avg <= GENE_BAND_2 ? 2 : 3;
}

uint8_t Pet::rollStars() {
  int r = random(100);
  return r < STAR_PCT_3 ? 3 : r < STAR_PCT_3 + STAR_PCT_2 ? 2 : 1;
}

// one form into the Pokedex; true if this form is new. One you already have keeps the higher stars.
bool Pet::addForm(int16_t dex, bool shinyForm, uint8_t stars) {
  if (dex < 1 || dex > 151) return false;
  if (stars < 1 || stars > 3) stars = rollStars();
  if (hasForm(dex, shinyForm)) {
    if (stars > starsOf(dex, shinyForm)) setStars(dex, shinyForm, stars);
    return false;
  }
  setStars(dex, shinyForm, stars);
  bool had = isRegistered(dex);
  setDexBit(dexReg, dex, true);
  if (shinyForm) {
    setDexBit(dexShinyReg, dex, true);
    if (!had) setDexBit(dexShinyOnly, dex, true);  // only the shiny one so far
  } else {
    setDexBit(dexShinyOnly, dex, false);
  }
  return true;
}

// one form out of the Pokedex: the species stays while the other form is there
void Pet::removeForm(int16_t dex, bool shinyForm) {
  if (!hasForm(dex, shinyForm)) return;
  setStars(dex, shinyForm, 0);
  bool other = hasForm(dex, !shinyForm);
  if (shinyForm) setDexBit(dexShinyReg, dex, false);
  setDexBit(dexShinyOnly, dex, other && !shinyForm);
  if (!other) setDexBit(dexReg, dex, false);
}

uint16_t Pet::formCount() const {
  uint16_t n = 0;
  for (int16_t d = 1; d <= 151; d++) n += hasForm(d, false) + hasForm(d, true);
  return n;
}

int16_t Pet::formAt(int idx, bool &shinyForm) const {
  for (int16_t d = 1; d <= 151; d++)
    for (int f = 0; f < 2; f++)
      if (hasForm(d, f) && idx-- == 0) {
        shinyForm = f;
        return d;
      }
  return 0;
}

void Pet::registerSpecies(int16_t dex) { addForm(dex, shiny, petStars()); }  // the one you raised: its own stars

// streak and bond improve the egg draw (0..~14)
int Pet::careBonus() const {
  int s = streak > 30 ? 30 : streak;
  return s / 3 + bond / 25;
}

// first care of the day: advances the streak and strengthens the bond
void Pet::registerCare() {
  if (isEgg() || ceremony != CER_NONE) return;
  uint32_t d = today();
  if (d == 0 || d == lastCareDay) return;  // no clock, or already counted today
  if (lastCareDay == 0 || d == lastCareDay + 1) {
    streak++;
  } else {
    streak = 1;        // there was a gap of days
    lastMilestone = 0;
  }
  lastCareDay = d;
  bondToday = 0;
  if (streak > bestStreak) bestStreak = streak;
  bond = clamp100(bond + 4);
  uint16_t ms = (streak >= 100) ? 100 : (streak >= 30) ? 30
              : (streak >= 7)   ? 7   : (streak >= 3)  ? 3 : 0;
  if (ms > lastMilestone) {
    lastMilestone = ms;
    milestoneUntil = millis() + CELEBRATE_MS;
  }
  checkMedals();
  save();
}

void Pet::addBond(uint8_t amt) {
  if (bondToday >= 20) return;  // daily cap: the bond can't be farmed
  bond = clamp100(bond + amt);
  bondToday += amt;
}

void Pet::checkMedals() {
  if (isEgg()) return;
  uint32_t before = medals | trainerMedals;
  if (level() >= 10) medals |= MED_LV10;
  if (level() >= 25) medals |= MED_LV25;
  if (level() >= 50) medals |= MED_LV50;
  if (berryKnown) medals |= MED_BERRY;
  if (streak >= 7) medals |= MED_STREAK7;
  if (bond >= 100) medals |= MED_BOND;
  if (DEX_TBL[speciesId].evolvesTo == 0) medals |= MED_FINAL;
  if (weight <= WGT_FIT && level() >= 5 && careMistakes == 0) medals |= MED_FIT;
  bool anyShiny = shiny;
  for (uint8_t b : dexShinyReg) anyShiny |= b != 0;
  uint16_t caught = registeredCount();
  if (anyShiny) trainerMedals |= MED_SHINY;
  if (caught >= 10) trainerMedals |= MED_DEX10;
  if (caught >= 50) trainerMedals |= MED_DEX50;
  if (caught >= 100) trainerMedals |= MED_DEX100;
  if (caught >= 151) trainerMedals |= MED_DEX151;
  if (level() >= LEVEL_MAX) trainerMedals |= MED_LV100;
  if (level() >= LEVEL_MAX && !counted100) {  // each Pokemon counts once
    counted100 = true;
    lv100Count++;
    save();
  }
  if (lv100Count >= 3) trainerMedals |= MED_LV100_3;
  if (lv100Count >= 5) trainerMedals |= MED_LV100_5;
  if (lv100Count >= 10) trainerMedals |= MED_LV100_10;
  if (buddyPeak >= 1) trainerMedals |= MED_BUDDY;
  if (buddyPeak + 1 >= PARTY_SIZE) trainerMedals |= MED_PARTY;
  for (uint8_t z = 0; z < ZONE_COUNT; z++)
    if (zoneCaught(z) == zoneSize(z)) trainerMedals |= 1UL << (MED_FIRST_ZONE + z);
  uint32_t gained = (medals | trainerMedals) & ~before;
  if (gained) {
    for (uint32_t m = gained; m; m &= (m - 1)) totalMedals++;
    newMedal = gained;
    medalUntil = millis() + CELEBRATE_MS;
    if (!sleeping) sfxPlay(SFX_MEDAL);
    save();
  }
}

void Pet::buddiesOnScreen(uint8_t n) {
  if (n <= buddyPeak) return;
  buddyPeak = n;
  checkMedals();
}

void Pet::rename(const char *name) {
  strncpy(nick, name, sizeof(nick) - 1);
  nick[sizeof(nick) - 1] = 0;
  save();
}

static uint16_t calcStat(uint8_t base, uint8_t gene, uint16_t lvl, uint8_t tr) {
  return (uint16_t)base * gene / 100 + lvl + tr;
}

uint16_t Pet::atkStat() const {
  return isEgg() ? 0 : calcStat(DEX_TBL[speciesId].bAtk, geneAtk, level(), trAtk);
}
uint16_t Pet::defStat() const {
  return isEgg() ? 0 : calcStat(DEX_TBL[speciesId].bDef, geneDef, level(), trDef);
}
uint16_t Pet::speStat() const {
  return isEgg() ? 0 : calcStat(DEX_TBL[speciesId].bSpe, geneSpe, level(), trSpe);
}

uint16_t Pet::registeredCount() const {
  uint16_t n = 0;
  for (int i = 1; i <= 151; i++)
    if (isRegistered(i)) n++;
  return n;
}

// final form that has completed its cycle (7 days): ready to say goodbye. The
// goodbye is triggered by the user with the button (it doesn't fire on its own, so they see it)
bool Pet::canFarewellNow() const {
  return !isEgg() && !sleeping && ceremony == CER_NONE &&
         DEX_TBL[speciesId].evolvesTo == 0 && ageMinutes >= FAREWELL_AGE_MIN;
}

// total abandonment for 1h: ready to run away. The user triggers it with the
// button (sad ending); caring for it for a single tick saves it (neglectTicks resets)
bool Pet::canRunawayNow() const {
  return !isEgg() && !sleeping && ceremony == CER_NONE && neglectTicks >= RUNAWAY_TICKS;
}

void Pet::startFarewell() {
  if (isEgg() || ceremony != CER_NONE) return;
  lastEnd = CER_FAREWELL;
  ceremony = CER_FAREWELL;
  ceremonyUntil = millis() + CEREMONY_MS;
  heartUntil = ceremonyUntil;  // hearts throughout the goodbye
  sfxPlay(SFX_BYE);
  save();
}

void Pet::startRunaway() {
  if (isEgg() || ceremony != CER_NONE) return;
  lastEnd = CER_RUNAWAY;
  ceremony = CER_RUNAWAY;
  ceremonyUntil = millis() + CEREMONY_MS;
  sfxPlay(SFX_BYE);
  save();
}

void Pet::rescue() {
  if (isEgg() || ceremony != CER_NONE) return;
  caress();
  feed();
  clean();
  neglectTicks = 0;
}

void Pet::release() {
  if (isEgg() || ceremony != CER_NONE) return;
  lastEnd = CER_RELEASE;
  ceremony = CER_RELEASE;
  ceremonyUntil = millis() + CEREMONY_MS;
  heartUntil = ceremonyUntil;
  sfxPlay(SFX_BYE);
  save();
}

void Pet::hatch() {
  speciesId = eggTarget;
  shiny = eggShiny;
  // individual's genes: 90-110% per stat (every raising is unique), or inside the band of the form it
  // was picked from, so its stars are that form's
  int lo = eggStars == 1 ? 90 : eggStars == 2 ? GENE_BAND_1 + 1 : eggStars == 3 ? GENE_BAND_2 + 1 : 90;
  int hi = eggStars == 1 ? GENE_BAND_1 : eggStars == 2 ? GENE_BAND_2 : 110;
  geneAtk = lo + random(hi - lo + 1);
  geneDef = lo + random(hi - lo + 1);
  geneSpe = lo + random(hi - lo + 1);
  eggStars = 0;
  trAtk = trDef = trSpe = 0;
  goodTicks = lowTicks = 0;
  farewellQuiet = false;
  battleWins = 0;
  movesSeen = 1;
  counted100 = false;
  evoAskAgain = 0;
  berryKnown = false;
  bond = 0;          // bond, medals and name belong to the individual
  bondToday = 0;
  medals = 0;
  newMedal = 0;
  nick[0] = 0;
  registerSpecies(speciesId);  // raised = registered in the pokedex
  checkMedals();     // in case it is born already in final form (legendary)
  sfxPlay(SFX_HATCH);
  save();
}

// do the conditions for evolving hold yet? Each neglect delays
// evolution by 1 level, and it must also be well cared for at that moment
// (no stat below 40). It does NOT evolve on its own: the user triggers it by
// tapping the critter (evolve()), so they see the transformation.
bool Pet::canEvolveNow() const {
  if (isEgg() || sleeping || ceremony != CER_NONE) return false;
  const DexEntry &d = DEX_TBL[speciesId];
  if (d.evolvesTo == 0) return false;
  return level() >= (uint16_t)d.evolveLevel + careMistakes && lowestStat() >= 40;
}

void Pet::evolve() {
  if (!canEvolveNow()) return;
  const DexEntry &d = DEX_TBL[speciesId];
  prevSpeciesId = speciesId;
  int16_t next = d.evolvesTo;
  if (speciesId == DEX_EEVEE) {
    // Eevee branch: prefers the evolution still missing from the pokedex
    int16_t opts[3];
    int n = 0;
    for (int16_t b = 134; b <= 136; b++)
      if (!isRegistered(b)) opts[n++] = b;
    next = n > 0 ? opts[random(n)] : (int16_t)(134 + random(3));
  }
  speciesId = next;
  registerSpecies(speciesId);
  sfxPlay(SFX_EVOLVE);
  evolveUntil = millis() + EVOLVE_ANIM_MS;
  save();
}

void Pet::feed() {
  feedBerry(0);
}

// every meal adds weight; eating while already full adds more
void Pet::gainWeight(uint8_t base) {
  weight = clamp100(weight + base + (fullness >= WGT_FULL_AT ? WGT_OVERFULL : 0));
}

void Pet::feedBerry(uint8_t color) {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  gainWeight(WGT_BERRY);
  if (lovesBerry(color)) {
    fullness = clamp100(fullness + 35);
    joy = clamp100(joy + 10);
    heartUntil = millis() + HEART_MS;  // "it loves it!"
    berryKnown = true;                 // discovered: shown on the info card
    addBond(2);
  } else {
    fullness = clamp100(fullness + 25);
  }
  eatUntil = millis() + EAT_ANIM_MS;
  registerCare();
  save();
}

void Pet::feedCandy() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping || !canHaveCandy()) return;
  if (today() != 0 && today() != candyDay) {
    candyDay = today();
    candyToday = 0;
  }
  candyToday++;
  gainWeight(WGT_CANDY);  // treats take their toll
  fullness = clamp100(fullness + 10);
  joy = clamp100(joy + 12);
  energy = 100;  // a sugar rush: worth it, being only 2 a day
  eatUntil = millis() + EAT_ANIM_MS;
  registerCare();
  save();
}

void Pet::exerciseReward(uint8_t score, bool costsFood) {
  uint8_t v = trSpe + score / 5;  // playing trains speed
  trSpe = v > 100 ? 100 : v;
  joy = clamp100(joy + 5 + (score > 15 ? 30 : score * 2));
  energy = dropTo(energy, 10 + score / 2, 5);
  if (costsFood) fullness = dropTo(fullness, 5, 5);
  int burn = (int)weight - score * 2;  // exercise burns weight
  weight = burn > 0 ? burn : 0;
  if (score >= 5) heartUntil = millis() + HEART_MS;
  addBond(2);
  registerCare();
}

void Pet::playResult(uint8_t score) {
  if (ceremony != CER_NONE || isEgg()) return;
  if (score > gameHi) gameHi = score;  // new record
  exerciseReward(score, false);
  fullness = clamp100(fullness + 5);   // what falls are berries: catching also feeds
  save();
}

void Pet::runResult(uint16_t secs) {
  if (ceremony != CER_NONE || isEgg()) return;
  if (secs > runHi) runHi = secs;
  exerciseReward(secs / 4 > 255 ? 255 : secs / 4);
  save();
}

void Pet::friendlyBattle(bool won) {
  if (ceremony != CER_NONE || isEgg()) return;
  joy = clamp100(joy + (won ? 15 : 8));
  heartUntil = millis() + HEART_MS;
  addBond(2);
  registerCare();
  save();
}

void Pet::dungeonResult(uint8_t floor) {
  if (ceremony != CER_NONE || isEgg()) return;
  if (floor > dungeonHi) dungeonHi = floor;
  exerciseReward(floor > 100 ? 200 : floor * 2);
  save();
}

// battle: winning trains more than losing, and losing never does permanent damage
// a win trains, cheers and bonds; a loss earns nothing, it only tires
uint8_t Pet::battleResult(bool won) {
  if (ceremony != CER_NONE || isEgg()) return 0;
  uint8_t antes = trAtk;
  if (won) {
    uint16_t v = (uint16_t)trAtk + BTL_WIN_TRAIN;
    trAtk = v > 100 ? 100 : (uint8_t)v;
  }
  uint8_t gain = trAtk - antes;
  if (won) {
    energy = dropTo(energy, BTL_ENERGY_COST, 5);
  } else {
    if (energy > BTL_LOSS_ENERGY) energy = BTL_LOSS_ENERGY;
    // a defeat is not neglect by itself; ignoring the exhausted pet afterwards, if
    if (mistakeCooldown < BTL_LOSS_GRACE_MIN) mistakeCooldown = BTL_LOSS_GRACE_MIN;
  }
  fullness = dropTo(fullness, 5, 5);
  int burn = (int)weight - 5;  // exercise burns weight
  weight = burn > 0 ? burn : 0;
  if (won) {
    if (battleWins < 65535) battleWins++;
    joy = clamp100(joy + 10);
    heartUntil = millis() + HEART_MS;
  } else {
    joy = dropTo(joy, 3, 5);
  }
  if (won) {
    addBond(2);
    registerCare();
  }
  save();
  return gain;
}

CatchResult Pet::catchWild(int16_t dex, bool shinyForm, uint8_t stars) {
  if (dex < 1 || dex > 151) return CR_KEPT;
  if (stars < 1 || stars > 3) stars = rollStars();
  CatchResult r = !hasForm(dex, shinyForm) ? CR_NEW : stars > starsOf(dex, shinyForm) ? CR_BETTER : CR_KEPT;
  if (r == CR_KEPT) return r;  // it goes free: nothing changes
  addForm(dex, shinyForm, stars);
  checkMedals();  // a zone may be complete now
  save();
  return r;
}

bool Pet::tradeAway(int16_t give, int16_t get, bool giveShiny, bool getShiny, uint8_t getStars) {
  if (!canTrade(give, giveShiny) || get < 1 || get > 151) return false;
  if (give == get && giveShiny == getShiny) return false;  // the same both ways: nothing changes hands
  removeForm(give, giveShiny);
  bool isNew = addForm(get, getShiny, getStars);
  checkMedals();
  save();
  return isNew;
}

void Pet::quizResult(uint8_t correct) {
  if (ceremony != CER_NONE || isEgg()) return;
  joy = clamp100(joy + 3 + correct * 3);
  if (correct >= QUIZ_ROUNDS) heartUntil = millis() + HEART_MS;
  addBond(1);
  registerCare();
  save();
}

// a game with a buddy: fun for both, more for the winner, and the playmate medal
void Pet::showdownResult(uint8_t mine, uint8_t theirs) {
  if (ceremony != CER_NONE || isEgg()) return;
  joy = clamp100(joy + 12 + mine * 3 + (mine > theirs ? 10 : 0));
  if (mine > theirs) heartUntil = millis() + HEART_MS;
  addBond(3);
  registerCare();
  uint32_t before = trainerMedals;
  trainerMedals |= MED_PLAYMATE;
  if (trainerMedals != before) {
    totalMedals++;
    newMedal = MED_PLAYMATE;
    medalUntil = millis() + CELEBRATE_MS;
    if (!sleeping) sfxPlay(SFX_MEDAL);
  }
  save();
}

bool Pet::visitorDueNow() const {
  if (isEgg() || sleeping || ceremony != CER_NONE || !lastSeenEpoch) return false;
  return visitorDue(today(), (lastSeenEpoch / 3600) % 24, visitorDay);
}

void Pet::visitorArrived(int16_t dex) {
  visitorDay = today();
  candyGiftDay = today();  // it brings a candy: a third one today
  visitDex = dex;
  visitMin = VISIT_MINUTES;
  save();
}

bool Pet::visitorResumable() {
  if (visitMin && visitorDay != today()) visitMin = 0;  // yesterday's: gone
  return visitMin && visitDex >= 1 && visitDex <= DEX_COUNT && !isEgg() && !sleeping && ceremony == CER_NONE;
}

bool Pet::visitorMinute() {
  if (visitMin) visitMin--;
  save();
  return visitMin == 0;
}

void Pet::visitorLeft() {
  visitDex = 0;
  visitMin = 0;
  if (isEgg() || ceremony != CER_NONE) {
    save();
    return;
  }
  joy = clamp100(joy + 5);
  heartUntil = millis() + HEART_MS;
  addBond(1);
  save();
}

int16_t Pet::pickWildSpecies(uint8_t lvl) {
  // legendaries are the end of the game, as with eggs
  bool legend = registeredCount() >= 25 && lvl >= 40 && random(100) < 3;
  bool preferNew = random(100) < 70;
  for (int pass = 0; pass < 3; pass++) {
    int16_t cand[151];
    int n = 0;
    for (int16_t d = 1; d <= 151; d++) {
      uint8_t r = DEX_TBL[d].rarity;
      if (legend ? r != R_LEGENDARY : r == R_LEGENDARY) continue;
      if (btlMinLevel(d) > lvl) continue;
      if (pass == 0 && preferNew && isRegistered(d)) continue;
      if (pass == 0 && r == R_RARE && random(100) < 60) continue;  // rares, less often
      cand[n++] = d;
    }
    if (n > 0) return cand[random(n)];
    legend = false;
  }
  return 16;  // unreachable: base forms are always candidates
}

int Pet::zoneCaught(uint8_t zone) const {
  int n = 0;
  for (int16_t d = 1; d <= 151; d++)
    if (zoneOf(d) == zone && isRegistered(d)) n++;
  return n;
}

int16_t Pet::pickZoneSpecies(uint8_t zone, uint8_t lvl) {
  int16_t legend = ZONE_LEGEND[zone];
  if (random(1000) < ZONE_LEGEND_PERMIL) return legend;  // any time, but seldom
  // now and then only the common ones you're missing; otherwise anyone strong enough to be here
  for (int pass = random(100) < ZONE_NEW_PCT ? 0 : 1; pass < 2; pass++) {
    int16_t cand[151];
    uint8_t w[151];
    int n = 0, total = 0;
    for (int16_t d = 1; d <= 151; d++) {
      if (zoneOf(d) != zone || d == legend || btlMinLevel(d) > lvl) continue;
      if (pass == 0 && (isRegistered(d) || zoneIsRare(d))) continue;
      cand[n] = d;
      w[n] = zoneWeight(d);
      total += w[n++];
    }
    if (n > 0) {
      int r = random(total);
      for (int i = 0; i < n; i++)
        if ((r -= w[i]) < 0) return cand[i];
    }
  }
  return zoneMember(zone, 0);  // unreachable: every zone has first forms
}

void Pet::play() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  joy = clamp100(joy + 25);
  energy = clamp100(energy - 10);
  fullness = clamp100(fullness - 5);
  heartUntil = millis() + HEART_MS;
  addBond(2);
  registerCare();
  save();
}

void Pet::toggleLight() {
  if (ceremony != CER_NONE) return;
  if (isEgg()) return;
  sleeping = !sleeping;
  save();
}

void Pet::clean() {
  if (ceremony != CER_NONE) return;
  poops = 0;
  hygiene = 100;
  addBond(1);
  registerCare();
  save();
}

void Pet::caress() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  // petting is always appreciated, but happiness has to be earned: only the first ones count
  uint32_t now = millis();
  if (petJoyTaps == 0 || now - petJoyStart >= PET_JOY_WINDOW_MS) {
    petJoyStart = now;
    petJoyTaps = 0;
  }
  if (petJoyTaps < PET_JOY_TAPS) {
    petJoyTaps++;
    joy = clamp100(joy + 5);
  }
  heartUntil = millis() + HEART_MS;
  addBond(1);
  registerCare();
}

void Pet::eggTap() {
  if (!isEgg()) return;
  if (++eggTaps >= 3) hatch();
  else save();
}

PetMood Pet::mood() const {
  if (sleeping) return MOOD_SLEEPING;
  if (eating()) return MOOD_EATING;
  if (lowestStat() < 25) return MOOD_SAD;
  return MOOD_HAPPY;
}

void Pet::save() {
  ticksSinceSave = 0;
  pendingSave = false;
  if (noSave) return;
  prefs.putUChar("full", fullness);
  prefs.putUChar("joy", joy);
  prefs.putUChar("ene", energy);
  prefs.putUChar("hyg", hygiene);
  prefs.putUChar("poop", poops);
  prefs.putUChar("wgt", weight);
  prefs.putUChar("gatk", geneAtk);
  prefs.putUChar("gdef", geneDef);
  prefs.putUChar("gspe", geneSpe);
  prefs.putUChar("tatk", trAtk);
  prefs.putUChar("tdef", trDef);
  prefs.putUChar("tspe", trSpe);
  prefs.putUShort("good", goodTicks);
  prefs.putUChar("lowt", lowTicks);
  prefs.putBool("bk", berryKnown);
  prefs.putBool("shy", shiny);
  prefs.putBool("eshy", eggShiny);
  prefs.putBool("stpk", starterPick);
  prefs.putBool("egpk", eggPick);
  prefs.putBytes("dexsh", dexShinyReg, sizeof(dexShinyReg));
  prefs.putBytes("dexsho", dexShinyOnly, sizeof(dexShinyOnly));
  prefs.putUInt("age", ageMinutes);
  prefs.putShort("dexn", speciesId);
  prefs.putShort("eggT2", eggTarget);
  prefs.putUChar("crack", eggTaps);
  prefs.putUChar("mist", careMistakes);
  prefs.putBool("sleep", sleeping);
  prefs.putUChar("lend", lastEnd);
  prefs.putBool("fquiet", farewellQuiet);
  if (lastSeenEpoch) prefs.putUInt("seen", lastSeenEpoch);
  prefs.putUInt("life", lifeMin);
  prefs.putUInt("gday", gameDay);
  prefs.putUInt("dlife", dayLife);
  prefs.putUInt("rutc", refUtc);
  prefs.putUInt("rlife", refLife);
  prefs.putBytes("dexreg", dexReg, sizeof(dexReg));
  prefs.putBytes("dexstr", dexStars, sizeof(dexStars));
  prefs.putUChar("estar", eggStars);
  prefs.putUShort("strk", streak);
  prefs.putUShort("bstrk", bestStreak);
  prefs.putUInt("cday", lastCareDay);
  prefs.putUChar("bond", bond);
  prefs.putUShort("medal", medals);
  prefs.putUShort("tmedal", totalMedals);
  prefs.putUShort("trmedal", (uint16_t)trainerMedals);  // the first 16, as older firmware reads them
  prefs.putUInt("trmed32", trainerMedals);
  prefs.putUShort("mstone", lastMilestone);
  prefs.putUShort("ghi", gameHi);
  prefs.putUShort("rhi", runHi);
  prefs.putUChar("dhi", dungeonHi);
  prefs.putUChar("cdyn", candyToday);
  prefs.putUInt("cdyd", candyDay);
  prefs.putUInt("cdyg", candyGiftDay);
  prefs.putUShort("bwin", battleWins);
  prefs.putUChar("bmov", movesSeen);
  prefs.putUChar("l100n", lv100Count);
  prefs.putBool("l100c", counted100);
  prefs.putUInt("vday", visitorDay);
  prefs.putShort("vdex", visitDex);
  prefs.putUChar("vmin", visitMin);
  prefs.putString("nick", nick);
}

void Pet::load() {
  fullness = prefs.getUChar("full", 80);
  joy = prefs.getUChar("joy", 80);
  energy = prefs.getUChar("ene", 80);
  hygiene = prefs.getUChar("hyg", 100);
  poops = prefs.getUChar("poop", 0);
  weight = prefs.getUChar("wgt", 0);
  geneAtk = prefs.getUChar("gatk", 0);
  geneDef = prefs.getUChar("gdef", 0);
  geneSpe = prefs.getUChar("gspe", 0);
  if (geneAtk == 0) {  // pet predating genes: rolled once now
    geneAtk = 90 + random(21);
    geneDef = 90 + random(21);
    geneSpe = 90 + random(21);
  }
  trAtk = prefs.getUChar("tatk", 0);
  trDef = prefs.getUChar("tdef", 0);
  trSpe = prefs.getUChar("tspe", 0);
  goodTicks = prefs.getUShort("good", 0);
  lowTicks = prefs.getUChar("lowt", 0);
  berryKnown = prefs.getBool("bk", false);
  shiny = prefs.getBool("shy", false);
  eggShiny = prefs.getBool("eshy", false);
  starterPick = prefs.getBool("stpk", false);
  eggPick = prefs.getBool("egpk", false);
  prefs.getBytes("dexsh", dexShinyReg, sizeof(dexShinyReg));
  if (prefs.isKey("dexsho")) prefs.getBytes("dexsho", dexShinyOnly, sizeof(dexShinyOnly));  // older saves: both forms
  ageMinutes = prefs.getUInt("age", 0);
  if (prefs.isKey("dexn")) {
    speciesId = prefs.getShort("dexn", -1);
    eggTarget = prefs.getShort("eggT2", 4);
  } else {
    // migration from the version with flash indices (0-8)
    static const uint8_t OLD2DEX[9] = { 4, 5, 6, 1, 2, 3, 7, 8, 9 };
    int8_t old = prefs.getChar("spec", -1);
    speciesId = (old >= 0 && old < 9) ? OLD2DEX[old] : -1;
    int8_t oldT = prefs.getChar("eggT", 0);
    eggTarget = (oldT >= 0 && oldT < 9) ? OLD2DEX[oldT] : 4;
  }
  eggTaps = prefs.getUChar("crack", 0);
  careMistakes = prefs.getUChar("mist", 0);
  sleeping = prefs.getBool("sleep", false);
  lastEnd = prefs.getUChar("lend", CER_NONE);
  farewellQuiet = prefs.getBool("fquiet", false);
  prefs.getBytes("dexreg", dexReg, sizeof(dexReg));
  bool hadStars = prefs.isKey("dexstr");
  if (hadStars) prefs.getBytes("dexstr", dexStars, sizeof(dexStars));
  eggStars = prefs.getUChar("estar", 0);
  streak = prefs.getUShort("strk", 0);
  bestStreak = prefs.getUShort("bstrk", 0);
  lastCareDay = prefs.getUInt("cday", 0);
  bond = prefs.getUChar("bond", 0);
  medals = prefs.getUShort("medal", 0);
  totalMedals = prefs.getUShort("tmedal", 0);
  trainerMedals = prefs.isKey("trmed32") ? prefs.getUInt("trmed32", 0) : prefs.getUShort("trmedal", 0);
  lastMilestone = prefs.getUShort("mstone", 0);
  gameHi = prefs.getUShort("ghi", 0);
  runHi = prefs.getUShort("rhi", 0);
  dungeonHi = prefs.getUChar("dhi", 0);
  candyToday = prefs.getUChar("cdyn", 0);
  candyDay = prefs.getUInt("cdyd", 0);
  candyGiftDay = prefs.getUInt("cdyg", 0);
  battleWins = prefs.getUShort("bwin", 0);
  movesSeen = prefs.getUChar("bmov", 1);
  lv100Count = prefs.getUChar("l100n", 0);
  counted100 = prefs.getBool("l100c", false);
  // from before the count: the Level 100 medal means one got there. If it isn't this one, count it
  if (!prefs.isKey("l100n") && (trainerMedals & MED_LV100) && level() < LEVEL_MAX) lv100Count = 1;
  lifeMin = prefs.getUInt("life", 0);
  gameDay = prefs.getUInt("gday", 0);  // older saves: 0, and the first clock sets today without a wait
  dayLife = prefs.getUInt("dlife", 0);
  refUtc = prefs.getUInt("rutc", 0);
  refLife = prefs.getUInt("rlife", 0);
  visitorDay = prefs.getUInt("vday", 0);
  visitDex = prefs.getShort("vdex", 0);
  visitMin = prefs.getUChar("vmin", 0);
  prefs.getString("nick", nick, sizeof(nick));
  // seed: the current pet counts as raised (old saves)
  if (speciesId >= 1) registerSpecies(speciesId);
  // from before the stars: every form you have is rated once, the current Pokemon's by its genes (just
  // above). Only added: nothing else changes, and later loads keep these
  if (!hadStars) {
    for (int16_t d = 1; d <= 151; d++)
      for (int f = 0; f < 2; f++)
        if (hasForm(d, f) && !starsOf(d, f)) setStars(d, f, rollStars());
    pendingSave = true;
  }
}
