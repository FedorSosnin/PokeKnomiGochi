#pragma once
// V2, the new look (built on the v2 branch until it launches): 1 draws it, 0 draws 1.51's screens. Build with
// -DUI_V2=0 to compare the two. Nothing in the save depends on it. The sketch and sdmon.cpp (V2's walk rows)
// both read it from here.
#ifndef UI_V2
#define UI_V2 1
#endif
// a condition only V2 adds: `if (a V2_AND(b))` is `if (a && (b))` with V2 and plain `if (a)` with V1. (An `if`
// line split by #if would break the sketch's prototype generator, which counts braces without reading #if.)
#if UI_V2
#define V2_AND(c) && (c)
#else
#define V2_AND(c)
#endif
// an amount only V2 adds: `y V2_PLUS(d)` is `y + (d)` with V2 and plain `y` with V1
#if UI_V2
#define V2_PLUS(d) + (d)
#else
#define V2_PLUS(d)
#endif
// the sprite of your side in a battle: in V2's Knomi Tower, whoever of your team fights (tower_v2.h); else yours
#if UI_V2
#define V2_TOWER_ME(own) towerMePmd(own)
#else
#define V2_TOWER_ME(own) own  // (no brackets: V1 keeps its tokens)
#endif
