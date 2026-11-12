#pragma once
// V2 on a computer and the web: the control under the mouse lifts off the screen a little, its shadow left where it
// stood (Fedor: "If we are going to do mouse hover states, it should be the same icon but slightly lifted with a
// shadow. Since we have actual pop up style effects on press, we dont need the boxes on hover"). It was outlined in
// blue. The Knomi has no mouse: none of this is built for it, and V2_HOVER() is nothing there.
//
// How: a control's drawing starts with V2_HOVER(x, y, w, h), its own box. When that is the control under the mouse
// (pressTargetAt(): the same controls the press preview knows), the canvas marks each pixel drawn until the end of
// that scope, and keeps what was under it (host_canvas.cpp). At the flush those pixels get back what was under them,
// a shadow is laid a little below where the control stood, and the control is put back V2_LIFT px higher, with
// whatever was drawn over it since (its label, a count).
#ifdef BOARD_HOST
extern bool gHoverRec;
extern uint16_t *gHoverFb;
extern uint8_t *gHoverMask;
extern uint16_t *gHoverUnder;
extern int16_t gHoverBox[4];
extern volatile int16_t gHostHoverX, gHostHoverY;  // (host_runtime.cpp; -1: not over the screen)

#define V2_LIFT 4         // how far a control lifts (px)
#define V2_LIFT_SHADOW 2  // its shadow: this much below where it stood...
#define V2_LIFT_DARK 96   // ...and this dark (of 256: the panels' ink over what's behind)

static PressTarget v2HoverT;
static bool v2HoverOn = false;
static uint32_t v2HoverLifts = 0;
static bool v2HoverOwnShadow = false;  // what lifts has a shadow of its own on the ground (a buddy): none added  // how many frames have lifted something (the screenshot tool's check)
uint32_t v2HostHoverLifts() { return v2HoverLifts; }

// is (x, y, w, h) the control under the mouse? Its centre inside the hover's box, and the box's inside it
static bool v2HoverIs(int x, int y, int w, int h) {
  if (!v2HoverOn || pressOn) return false;
  const PressTarget &t = v2HoverT;
  int cx = x + w / 2, cy = y + h / 2, tx = t.x + t.w / 2, ty = t.y + t.h / 2;
  return cx >= t.x - 4 && cx <= t.x + t.w + 4 && cy >= t.y - 4 && cy <= t.y + t.h + 4 && tx >= x - 4 &&
         tx <= x + w + 4 && ty >= y - 4 && ty <= y + h + 4;
}
static bool v2HoverAlloc() {
  if (gHoverMask) return gHoverUnder != nullptr;
  size_t n = (size_t)gfx->width() * gfx->height();
  gHoverMask = (uint8_t *)calloc(n, 1);
  gHoverUnder = (uint16_t *)malloc(n * sizeof(uint16_t));
  return gHoverMask && gHoverUnder;
}
struct V2HoverScope {
  bool on = false;
  V2HoverScope(int x, int y, int w, int h) {
    if (gHoverRec || !v2HoverIs(x, y, w, h) || !v2HoverAlloc()) return;
    gHoverFb = (uint16_t *)gfx->getFramebuffer();
    gHoverRec = on = true;
  }
  ~V2HoverScope() {
    if (on) gHoverRec = false;
  }
};
// the same for what's drawn straight into the framebuffer, not through the canvas (the trophies): the box's pixels
// (and m round it) are kept first, and at the end of the scope those that changed are marked, with what was there
struct V2HoverDiff {
  int x0 = 0, y0 = 0, x1 = -1, y1 = -1;
  uint16_t *before = nullptr;
  V2HoverDiff(int x, int y, int w, int h, bool ownShadow = false, int m = 8) {
    if (gHoverRec || !v2HoverIs(x, y, w, h) || !v2HoverAlloc()) return;
    v2HoverOwnShadow = ownShadow;
    const int W = gfx->width(), H = gfx->height();
    x0 = x - m < 0 ? 0 : x - m;
    y0 = y - m < 0 ? 0 : y - m;
    x1 = x + w + m > W - 1 ? W - 1 : x + w + m;
    y1 = y + h + m > H - 1 ? H - 1 : y + h + m;
    if (x1 < x0 || y1 < y0) return;
    before = (uint16_t *)malloc((size_t)(x1 - x0 + 1) * (y1 - y0 + 1) * sizeof(uint16_t));
    if (!before) return;
    const uint16_t *fb = (const uint16_t *)gfx->getFramebuffer();
    for (int yy = y0; yy <= y1; yy++) memcpy(before + (yy - y0) * (x1 - x0 + 1), fb + yy * W + x0, (x1 - x0 + 1) * sizeof(uint16_t));
  }
  ~V2HoverDiff() {
    if (!before) return;
    const int W = gfx->width(), bw = x1 - x0 + 1;
    uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
    gHoverFb = fb;
    for (int yy = y0; yy <= y1; yy++)
      for (int xx = x0; xx <= x1; xx++) {
        int i = yy * W + xx;
        uint16_t was = before[(yy - y0) * bw + xx - x0];
        if (fb[i] == was || gHoverMask[i]) continue;
        gHoverMask[i] = 1;
        gHoverUnder[i] = was;
        if (xx < gHoverBox[0]) gHoverBox[0] = (int16_t)xx;
        if (yy < gHoverBox[1]) gHoverBox[1] = (int16_t)yy;
        if (xx > gHoverBox[2]) gHoverBox[2] = (int16_t)xx;
        if (yy > gHoverBox[3]) gHoverBox[3] = (int16_t)yy;
      }
    free(before);
  }
};
#define V2_HOVER_CAT2(a, b) a##b
#define V2_HOVER_CAT(a, b) V2_HOVER_CAT2(a, b)
#define V2_HOVER(x, y, w, h) V2HoverScope V2_HOVER_CAT(v2Hover_, __LINE__)(x, y, w, h)
#define V2_HOVER_DIFF(x, y, w, h) V2HoverDiff V2_HOVER_CAT(v2HoverD_, __LINE__)(x, y, w, h)
#define V2_HOVER_RISE(x, y, w, h) V2HoverDiff V2_HOVER_CAT(v2HoverD_, __LINE__)(x, y, w, h, true)  // (its own shadow)

// each frame (the loop): the control under the mouse, if any; a change draws the screen again
void v2HoverUpdate() {
  int16_t x = gHostHoverX, y = gHostHoverY;
  PressTarget t;
  bool on = x >= 0 && !pressOn && !wasPressed && pressTargetAt(x, y, t);
  if (on == v2HoverOn && (!on || (t.x == v2HoverT.x && t.y == v2HoverT.y && t.w == v2HoverT.w && t.h == v2HoverT.h))) return;
  v2HoverOn = on;
  if (on) v2HoverT = t;
  pressRepaint();
}

// at the flush (pressOverlay()): lift what was marked. Nothing marked: the screen wasn't drawn again since (a still
// screen), and what's there already has its lift
void v2HoverOverlay() {
  int16_t *b = gHoverBox;
  if (b[2] < 0 || !gHoverMask) return;
  uint16_t *fb = gHoverFb;
  const int W = gfx->width(), H = gfx->height(), x0 = b[0], y0 = b[1], x1 = b[2], y1 = b[3], bw = x1 - x0 + 1;
  if (v2HoverOn && !pressOn && fb == (uint16_t *)gfx->getFramebuffer()) {
    static uint16_t *keep = nullptr;
    static size_t keepN = 0;
    size_t need = (size_t)bw * (y1 - y0 + 1);
    if (need > keepN) {
      free(keep);
      keep = (uint16_t *)malloc(need * sizeof(uint16_t));
      keepN = keep ? need : 0;
    }
    if (keep) {
      // the control off: what's there now kept, what was under it put back
      for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
          int i = y * W + x;
          if (!gHoverMask[i]) continue;
          keep[(y - y0) * bw + x - x0] = fb[i];
          fb[i] = gHoverUnder[i];
        }
      // its shadow, its own shape a little below where it stood (each pixel darkened once)
      const uint16_t ink = C565(0x1b, 0x1b, 0x25);
      for (int y = y0; y <= (v2HoverOwnShadow ? y0 - 1 : y1); y++)
        for (int x = x0; x <= x1; x++) {
          int ys = y + V2_LIFT_SHADOW;
          if (gHoverMask[y * W + x] && ys < H) fb[ys * W + x] = v2Blend(fb[ys * W + x], ink, V2_LIFT_DARK);
        }
      v2HoverLifts++;
      // and the control again, lifted
      for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
          int yl = y - V2_LIFT;
          if (gHoverMask[y * W + x] && yl >= 0) fb[yl * W + x] = keep[(y - y0) * bw + x - x0];
        }
    }
  }
  for (int y = y0; y <= y1; y++) memset(gHoverMask + y * W + x0, 0, bw);
  v2HoverOwnShadow = false;
  b[0] = b[1] = 32767;
  b[2] = b[3] = -1;
}
#else
#define V2_HOVER(x, y, w, h) ((void)0)
#define V2_HOVER_DIFF(x, y, w, h) ((void)0)
#define V2_HOVER_RISE(x, y, w, h) ((void)0)
#define v2HoverIs(x, y, w, h) false
#endif
