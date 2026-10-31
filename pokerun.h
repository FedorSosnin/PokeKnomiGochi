#pragma once
#include <stdint.h>
// PokeRun: reglas sin hardware (test/test_minigames.cpp). Pixeles logicos del lienzo de 466 y segundos.
// La dificultad se ajusta con estas constantes.

#define RUN_GROUND_Y 340
#define RUN_PET_X 120
#define RUN_SPAWN_X 490.0f
#define RUN_SEEN_X 438.0f     // donde el circulo deja ver el obstaculo a ras de suelo

#define RUN_SPEED_START 230.0f
#define RUN_SPEED_GAIN 5.0f   // px/s que se ganan cada segundo
#define RUN_SPEED_MAX 760.0f

#define RUN_JUMP_V 620.0f
#define RUN_GRAVITY 1500.0f   // 0.83 s en el aire, 128 px de alto
#define RUN_JUMP_BUFFER 0.12f // pulsar justo antes de aterrizar tambien cuenta

#define RUN_GAP_START 1.6f    // segundos entre obstaculos
#define RUN_GAP_MIN 0.95f     // nunca menos: siempre da tiempo a aterrizar y volver a saltar
#define RUN_GAP_TIGHTEN 45.0f // segundos de partida hasta llegar al minimo
#define RUN_DOUBLE_FROM 20.0f // a partir de aqui pueden salir rocas dobles

// caja de choque algo menor que el dibujo: rozar no cuenta
#define RUN_PET_HALF_W 15
#define RUN_FORGIVE 4

enum RunKind : uint8_t { RUN_ROCK, RUN_BUSH, RUN_DOUBLE };
struct RunObstacle { float x; RunKind kind; bool on; };

struct RunState {
  float t, dist;       // tiempo de partida y distancia recorrida
  float h, vy;         // altura sobre el suelo y velocidad vertical
  float buffer;        // salto pendiente de aterrizar
  float spawnIn;
  RunObstacle ob[4];
  uint32_t rng;
  bool dead, jumped;   // jumped: ha saltado en este paso
};

inline int runObstacleW(RunKind k) { return k == RUN_BUSH ? 44 : k == RUN_DOUBLE ? 78 : 34; }
inline int runObstacleH(RunKind k) { return k == RUN_BUSH ? 44 : 30; }

inline float runSpeed(float t) {
  float v = RUN_SPEED_START + RUN_SPEED_GAIN * t;
  return v > RUN_SPEED_MAX ? RUN_SPEED_MAX : v;
}

inline float runMinGap(float t) {
  if (t >= RUN_GAP_TIGHTEN) return RUN_GAP_MIN;
  return RUN_GAP_START + (RUN_GAP_MIN - RUN_GAP_START) * t / RUN_GAP_TIGHTEN;
}

inline uint32_t runRand(RunState &s) {
  uint32_t x = s.rng ? s.rng : 1;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return s.rng = x;
}
inline float runRand01(RunState &s) { return (runRand(s) >> 8) / 16777216.0f; }

inline void runStart(RunState &s, uint32_t seed) {
  s = RunState();
  s.rng = seed;
  s.spawnIn = 1.2f;  // un momento para situarse antes del primero
}

inline void runSpawn(RunState &s) {
  RunKind k = (runRand(s) & 1) ? RUN_BUSH : RUN_ROCK;
  // las dobles aparecen poco a poco: hasta una de cada tres
  float dbl = (s.t - RUN_DOUBLE_FROM) / 90.0f;
  if (dbl > 0.33f) dbl = 0.33f;
  if (dbl > 0 && runRand01(s) < dbl) k = RUN_DOUBLE;
  for (auto &o : s.ob)
    if (!o.on) {
      o.x = RUN_SPAWN_X;
      o.kind = k;
      o.on = true;
      break;
    }
  float gap = runMinGap(s.t) * (1.0f + 0.5f * runRand01(s));
  s.spawnIn = runObstacleW(k) / runSpeed(s.t) + gap;
}

inline bool runHits(const RunState &s, const RunObstacle &o) {
  if (!o.on) return false;
  int w = runObstacleW(o.kind), hh = runObstacleH(o.kind);
  return o.x < RUN_PET_X + RUN_PET_HALF_W - 2 && o.x + w > RUN_PET_X - RUN_PET_HALF_W + 2 && s.h < hh - RUN_FORGIVE;
}

// un paso de fisica. Un parón (guardado en flash) no debe matar: el paso se acota.
inline void runStep(RunState &s, float dt, bool jump) {
  s.jumped = false;
  if (s.dead) return;
  if (dt > 0.12f) dt = 0.12f;
  s.t += dt;
  float v = runSpeed(s.t);
  s.dist += v * dt;

  if (jump) s.buffer = RUN_JUMP_BUFFER;
  if (s.h > 0 || s.vy > 0) {
    s.vy -= RUN_GRAVITY * dt;
    s.h += s.vy * dt;
    if (s.h <= 0) { s.h = 0; s.vy = 0; }
  }
  if (s.h == 0 && s.buffer > 0) {
    s.vy = RUN_JUMP_V;
    s.h = 0.001f;
    s.buffer = 0;
    s.jumped = true;
  }
  if (s.buffer > 0) s.buffer -= dt;

  for (auto &o : s.ob) {
    if (!o.on) continue;
    o.x -= v * dt;
    if (o.x + runObstacleW(o.kind) < -20) o.on = false;
    else if (runHits(s, o)) s.dead = true;
  }
  s.spawnIn -= dt;
  if (s.spawnIn <= 0) runSpawn(s);
}

// el obstaculo que viene, o null
inline const RunObstacle *runNext(const RunState &s) {
  const RunObstacle *best = nullptr;
  for (auto &o : s.ob)
    if (o.on && o.x + runObstacleW(o.kind) > RUN_PET_X - RUN_PET_HALF_W && (!best || o.x < best->x)) best = &o;
  return best;
}

// segundos hasta tocar el obstaculo que viene; negativo si no hay ninguno
inline float runTimeToContact(const RunState &s) {
  const RunObstacle *o = runNext(s);
  if (!o) return -1;
  float d = o->x - (RUN_PET_X + RUN_PET_HALF_W);
  return d <= 0 ? 0 : d / runSpeed(s.t);
}

// jugador automatico para pruebas: salta poco antes de llegar, a cualquier ritmo de frames
inline bool runBotWantsJump(const RunState &s) {
  float c = runTimeToContact(s);
  return s.h == 0 && c > 0 && c < 0.22f;
}
