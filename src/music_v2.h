// V2: which music each screen wants (music.h), asked each pass of the loop; host_audio.cpp fades from one track to the
// next. Only where there is music (HAS_MUSIC: a computer and the web): scene_v2.h includes this at its end.
#if HAS_MUSIC

#define MUSIC_HOME_IDLE_MS 120000UL  // the home's music fades out after this long untouched, and is back on a tap (Fedor)
static int8_t v2MusicForced = -1;    // MUSIC n on the console: that track whatever the screen (-1: the screens')
static void v2MusicForce(int track) { v2MusicForced = track >= 0 && track < MUS_COUNT ? (int8_t)track : -1; }

// a battle's: its zone's (a wild one from home or the island: the zone its Pokemon lives in), a legend's, the tower's,
// a buddy's; a win and a catch, the victory jingle; a loss, running and a catch that got away, quiet
static uint8_t v2MusicBattle() {
  if (bPhase == BP_END) return bWon ? MUS_VICTORY : MUS_NONE;
  if (bPhase == BP_CAUGHT || bPhase == BP_RESULT) return MUS_VICTORY;
  if (bPhase == BP_RAN || bPhase == BP_FLED) return MUS_NONE;
  if (bPhase >= BP_AIM) return MUS_NONE;  // the throw and the shakes, in the hush before the catch
  if (bPvp) return MUS_BUDDY;
  if (bTower) return twStep == TOWER_LEGEND ? MUS_LEGEND : MUS_TOWER_FIGHT;
  if (bFoe.dex >= 1 && bFoe.dex <= DEX_COUNT && DEX_TBL[bFoe.dex].rarity == R_LEGENDARY) return MUS_LEGEND;
  int z = bZone >= 0 ? bZone : (bFoe.dex >= 1 && bFoe.dex <= DEX_COUNT ? zoneOf(bFoe.dex) : Z_FIELD);
  return (uint8_t)(MUS_FOREST + z);
}

// Knomi Tower's screens: the tower's own music, its Legend's before that fight, the jingle after a floor won
static uint8_t v2MusicTower() {
  switch (twView) {
    case TW_WON: case TW_MASTER: case TW_CHAL: return MUS_VICTORY;
    case TW_OVER: return MUS_NONE;
    case TW_LEGEND: return MUS_LEGEND;
    case TW_DRAIN: return MUS_TOWER_FIGHT;
    default: return MUS_TOWER;
  }
}

// in the order render() draws the screens: the first open one is the one showing
static uint8_t v2MusicPick(uint32_t now) {
  if (v2MusicForced >= 0) return (uint8_t)v2MusicForced;
  if (pet.awaitingStarter() || pet.awaitingEggPick()) return MUS_HELLO;
  if (sdOpen || quizOpen) return MUS_QUIZ;
  if (vbOpen) return MUS_VOLLEY;
  if (gameOpen) return MUS_BERRY;
  if (dungeonOpen) return MUS_DUNGEON;
  if (runOpen) return MUS_RUN;
  if (surfOpen) return MUS_SURF;
  if (battleOpen && bPhase != BP_TIRED) return v2MusicBattle();  // (tired: a notice over what was there)
  if (buddyMenuOpen || partyOpen || playMenuOpen || tradeOpen || galleryOpen || gamesMenuOpen || kbOpen || clockOpen ||
      cardOpen)
    return MUS_MENU;
  if (zoneScreen == ZS_TOWER) return v2MusicTower();
  if (zoneScreen) return MUS_TRAVEL;
  if (pet.ceremony) return MUS_GOODBYE;
  if (pet.evolving()) return MUS_EVOLVE;
  if (pet.sleeping) return MUS_NONE;
  if (idleSince(now, lastInteract) > MUSIC_HOME_IDLE_MS) return MUS_NONE;
  return v2HourTod() ? MUS_NIGHT : MUS_HOME;  // dusk, night and dawn: the quiet version
}

#endif  // HAS_MUSIC
