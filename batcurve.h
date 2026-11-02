#pragma once
#include <stdint.h>

// Single-cell LiPo/Li-ion battery (charged to 4.2 V, as the Waveshare example configures): from
// resting voltage to percentage, and when to shut down so as not to discharge it deeply. Pure logic, with
// tests in test/test_batcurve.cpp.

#define BAT_CUTOFF_MV 3300     // below this (while discharging) the firmware saves and shuts down
#define BAT_CUTOFF_READS 3     // consecutive readings below (besides the time, see BatMonitor)
#define BAT_HW_CUTOFF_MV 3100  // backup in the PMU (AXP2101, 2600 to 3300 in steps of 100)

// typical LiPo cell discharge curve, (mV, %) from highest to lowest
struct BatPoint { int16_t mv; uint8_t pct; };
static const BatPoint BAT_CURVE[] = {
  { 4200, 100 }, { 4150, 95 }, { 4110, 90 }, { 4080, 85 }, { 4020, 80 }, { 3980, 75 }, { 3950, 70 },
  { 3910, 65 }, { 3870, 60 }, { 3850, 55 }, { 3840, 50 }, { 3820, 45 }, { 3800, 40 }, { 3790, 35 },
  { 3770, 30 }, { 3750, 25 }, { 3730, 20 }, { 3710, 15 }, { 3690, 10 }, { 3610, 5 }, { BAT_CUTOFF_MV, 0 },
};

// linear interpolation between points; 100 above 4.2 V and 0 at shutdown
inline int batPercentFromMv(int mv) {
  const int n = sizeof(BAT_CURVE) / sizeof(BAT_CURVE[0]);
  if (mv >= BAT_CURVE[0].mv) return 100;
  if (mv <= BAT_CURVE[n - 1].mv) return 0;
  for (int i = 1; i < n; i++) {
    if (mv >= BAT_CURVE[i].mv) {
      int dmv = BAT_CURVE[i - 1].mv - BAT_CURVE[i].mv, dp = BAT_CURVE[i - 1].pct - BAT_CURVE[i].pct;
      return BAT_CURVE[i].pct + (mv - BAT_CURVE[i].mv) * dp / dmv;
    }
  }
  return 0;
}

// Smooths the voltage (exponential moving average, 1/4 per reading), makes the displayed percentage only
// go down while discharging lasts (the charge spike right after unplugging, or a dip from load, neither
// raises it nor makes it dance) and decides shutdown: the smoothed voltage must be below the limit
// for BAT_CUTOFF_MS in a row (and at least BAT_CUTOFF_READS readings), not just a few readings.
#define BAT_CUTOFF_MS 30000UL
#define BAT_BOOT_GRACE_MS 30000UL  // no shutdown after boot: the first voltage doesn't say anything yet

struct BatMonitor {
  int smoothMv = 0;
  int pct = -1;            // last percentage shown (-1 = no reading yet)
  uint32_t lowSince = 0;
  uint8_t lowReads = 0;

  void reset() { smoothMv = 0; pct = -1; lowSince = 0; lowReads = 0; }
  // upper limit of the percentage when discharge starts (the one the PMU was showing)
  void seed(int p) { if (pct < 0 && p >= 0) pct = p; }
  // One reading while discharging (no USB or charging). Returns the percentage, -1 if there was no reading.
  int sample(int mv, uint32_t now) {
    if (mv <= 0) { lowReads = 0; lowSince = 0; return -1; }
    smoothMv = smoothMv ? smoothMv + (mv - smoothMv) / 4 : mv;
    if (smoothMv < BAT_CUTOFF_MV) {
      if (!lowReads) lowSince = now ? now : 1;
      if (lowReads < 255) lowReads++;
    } else {
      lowReads = 0;
      lowSince = 0;
    }
    int c = batPercentFromMv(smoothMv);
    if (pct < 0 || c < pct) pct = c;
    return pct;
  }
  bool shutdownNeeded(uint32_t now) const {
    return now >= BAT_BOOT_GRACE_MS && lowReads >= BAT_CUTOFF_READS && now - lowSince >= BAT_CUTOFF_MS;
  }
};
