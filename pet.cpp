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
  if (nowEpoch) save();  // saved right away: a power cut doesn't lose the reference
}

void Pet::syncClock(uint32_t nowEpoch) {
  uint32_t seen = prefs.getUInt("seen", 0);
  lastSeenEpoch = nowEpoch;
  if (nowEpoch == 0) return;
  uint32_t mins = (seen && nowEpoch > seen) ? (nowEpoch - seen) / 60 : 0;
  if (mins < 2 || ceremony != CER_NONE || starterPick || eggPick) {
    save();  // first time, no time to apply, or still choosing what to raise
    return;
  }
  if (mins > 14UL * 24 * 60) mins = 14UL * 24 * 60;  // cap: 2 weeks

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
      continue;
    }
    fullness = dropTo(fullness, 2, 15);
    energy = dropTo(energy, 1, 15);
    hygiene = dropTo(hygiene, 1, 15);
    joy = dropTo(joy, 1, 15);
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
    tick();
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
  // Weight still burns; the good-care streak (goodTicks) is paused.
  if (sleeping) {
    energy = clamp100(energy + 6);
    if (weight > 0 && ageMinutes % (2 * WGT_BURN_MIN) == 0) weight--;
    if (ageMinutes % 2 == 0) {                 // ~4x slower than awake
      fullness = dropTo(fullness, 1, 30);
      joy = dropTo(joy, 1, 35);
    }
    if (ageMinutes % 3 == 0) hygiene = dropTo(hygiene, 1, 45);
    checkMedals();  // can still cross a level by age while sleeping
    if (++ticksSinceSave >= 5) pendingSave = true;
    return;
  }

  if (ageMinutes % MINUTES_PER_LEVEL == 0) sfxPlay(SFX_LEVEL);  // leveled up (awake)

  fullness = clamp100(fullness - 2);
  energy = clamp100(energy - 1);
  if (fullness > 40 && poops < 3 && random(100) < 15) poops++;

  hygiene = clamp100(hygiene - 1 - 4 * poops);
  // overweight makes it lazy: energy drops twice as fast
  if (weight > 50) energy = clamp100(energy - 1);
  if (weight > 0 && ageMinutes % WGT_BURN_MIN == 0) weight--;

  // discipline forges defense: 12 h in a row well cared for = +1 DEF
  if (lowestStat() >= 40) {
    if (++goodTicks >= 720) {
      goodTicks = 0;
      if (trDef < 100) trDef++;
    }
  } else {
    goodTicks = 0;
  }

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

bool Pet::chooseEgg(int16_t dex) {
  if (!eggPick || !isRegistered(dex)) return false;
  eggTarget = dex;
  eggPick = false;
  save();
  return true;
}

int16_t Pet::registeredAt(int idx) const {
  for (int16_t d = 1; d <= 151; d++)
    if (isRegistered(d) && idx-- == 0) return d;
  return 0;
}

void Pet::registerSpecies(int16_t dex) {
  if (dex < 1 || dex > 151) return;
  dexReg[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
  if (shiny) dexShinyReg[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
}

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
    milestoneUntil = millis() + 4500;
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
  uint16_t before = medals | trainerMedals;
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
  if (level() >= 100) trainerMedals |= MED_LV100;
  if (buddyPeak >= 1) trainerMedals |= MED_BUDDY;
  if (buddyPeak + 1 >= PARTY_SIZE) trainerMedals |= MED_PARTY;
  uint16_t gained = (medals | trainerMedals) & ~before;
  if (gained) {
    for (uint16_t m = gained; m; m &= (m - 1)) totalMedals++;
    newMedal = gained;
    medalUntil = millis() + 4000;
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
  // individual's genes: 90-110% per stat (every raising is unique)
  geneAtk = 90 + random(21);
  geneDef = 90 + random(21);
  geneSpe = 90 + random(21);
  trAtk = trDef = trSpe = 0;
  battleWins = 0;
  movesSeen = 1;
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
uint8_t Pet::battleResult(bool won) {
  if (ceremony != CER_NONE || isEgg()) return 0;
  uint8_t antes = trAtk;
  uint16_t v = (uint16_t)trAtk + (won ? BTL_WIN_TRAIN : BTL_LOSS_TRAIN);
  trAtk = v > 100 ? 100 : (uint8_t)v;
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
  addBond(2);
  registerCare();
  save();
  return gain;
}

bool Pet::catchWild(int16_t dex) {
  if (dex < 1 || dex > 151 || isRegistered(dex)) return false;
  // straight to the normal bit: registerSpecies() would mark shiny if the pet is one
  dexReg[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
  save();
  return true;
}

void Pet::quizResult(uint8_t correct) {
  if (ceremony != CER_NONE || isEgg()) return;
  joy = clamp100(joy + 3 + correct * 3);
  if (correct >= QUIZ_ROUNDS) heartUntil = millis() + HEART_MS;
  addBond(1);
  registerCare();
  save();
}

bool Pet::visitorDueNow() const {
  if (isEgg() || sleeping || ceremony != CER_NONE || !lastSeenEpoch) return false;
  return visitorDue(today(), (lastSeenEpoch / 3600) % 24, visitorDay);
}

void Pet::visitorArrived() {
  visitorDay = today();
  save();
}

void Pet::visitorLeft() {
  if (isEgg() || ceremony != CER_NONE) return;
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
  bool complete = true;
  for (int16_t d = 1; d <= 151 && complete; d++)
    if (zoneOf(d) == zone && d != legend && !isRegistered(d)) complete = false;
  if (complete && random(100) < (isRegistered(legend) ? ZONE_LEGEND_SEEN_PCT : ZONE_LEGEND_NEW_PCT)) return legend;

  bool preferNew = random(100) < ZONE_NEW_PCT;
  for (int pass = 0; pass < 2; pass++) {
    int16_t cand[151];
    int n = 0;
    for (int16_t d = 1; d <= 151; d++) {
      if (zoneOf(d) != zone || d == legend || btlMinLevel(d) > lvl) continue;
      if (pass == 0 && preferNew && isRegistered(d)) continue;
      cand[n++] = d;
    }
    if (n > 0) return cand[random(n)];
  }
  return zoneMember(zone, 0);  // unreachable: every zone has base forms
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
  prefs.putBool("bk", berryKnown);
  prefs.putBool("shy", shiny);
  prefs.putBool("eshy", eggShiny);
  prefs.putBool("stpk", starterPick);
  prefs.putBool("egpk", eggPick);
  prefs.putBytes("dexsh", dexShinyReg, sizeof(dexShinyReg));
  prefs.putUInt("age", ageMinutes);
  prefs.putShort("dexn", speciesId);
  prefs.putShort("eggT2", eggTarget);
  prefs.putUChar("crack", eggTaps);
  prefs.putUChar("mist", careMistakes);
  prefs.putBool("sleep", sleeping);
  prefs.putUChar("lend", lastEnd);
  if (lastSeenEpoch) prefs.putUInt("seen", lastSeenEpoch);
  prefs.putBytes("dexreg", dexReg, sizeof(dexReg));
  prefs.putUShort("strk", streak);
  prefs.putUShort("bstrk", bestStreak);
  prefs.putUInt("cday", lastCareDay);
  prefs.putUChar("bond", bond);
  prefs.putUShort("medal", medals);
  prefs.putUShort("tmedal", totalMedals);
  prefs.putUShort("trmedal", trainerMedals);
  prefs.putUShort("mstone", lastMilestone);
  prefs.putUShort("ghi", gameHi);
  prefs.putUShort("rhi", runHi);
  prefs.putUChar("dhi", dungeonHi);
  prefs.putUChar("cdyn", candyToday);
  prefs.putUInt("cdyd", candyDay);
  prefs.putUShort("bwin", battleWins);
  prefs.putUChar("bmov", movesSeen);
  prefs.putUInt("vday", visitorDay);
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
  berryKnown = prefs.getBool("bk", false);
  shiny = prefs.getBool("shy", false);
  eggShiny = prefs.getBool("eshy", false);
  starterPick = prefs.getBool("stpk", false);
  eggPick = prefs.getBool("egpk", false);
  prefs.getBytes("dexsh", dexShinyReg, sizeof(dexShinyReg));
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
  prefs.getBytes("dexreg", dexReg, sizeof(dexReg));
  streak = prefs.getUShort("strk", 0);
  bestStreak = prefs.getUShort("bstrk", 0);
  lastCareDay = prefs.getUInt("cday", 0);
  bond = prefs.getUChar("bond", 0);
  medals = prefs.getUShort("medal", 0);
  totalMedals = prefs.getUShort("tmedal", 0);
  trainerMedals = prefs.getUShort("trmedal", 0);
  lastMilestone = prefs.getUShort("mstone", 0);
  gameHi = prefs.getUShort("ghi", 0);
  runHi = prefs.getUShort("rhi", 0);
  dungeonHi = prefs.getUChar("dhi", 0);
  candyToday = prefs.getUChar("cdyn", 0);
  candyDay = prefs.getUInt("cdyd", 0);
  battleWins = prefs.getUShort("bwin", 0);
  movesSeen = prefs.getUChar("bmov", 1);
  visitorDay = prefs.getUInt("vday", 0);
  prefs.getString("nick", nick, sizeof(nick));
  // seed: the current pet counts as raised (old saves)
  if (speciesId >= 1) registerSpecies(speciesId);
}
