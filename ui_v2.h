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
