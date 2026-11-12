#include "framework.h"
#include "../src/batcurve.h"

TEST(batcurve, extremes) {
  CHECK_EQ(batPercentFromMv(4300), 100);
  CHECK_EQ(batPercentFromMv(4200), 100);
  CHECK_EQ(batPercentFromMv(BAT_CUTOFF_MV), 0);
  CHECK_EQ(batPercentFromMv(3000), 0);
  CHECK_EQ(batPercentFromMv(3840), 50);
}

TEST(batcurve, monotonic_and_covers_full_range) {
  int prev = 100;
  for (int mv = 4200; mv >= BAT_CUTOFF_MV; mv--) {
    int p = batPercentFromMv(mv);
    CHECK(p <= prev);
    CHECK(p >= 0 && p <= 100);
    prev = p;
  }
  CHECK_EQ(batPercentFromMv(3660), 8);  // between (3610, 5) and (3690, 10)
}

// readings every 2 s from t0 at the same voltage; returns whether a shutdown is needed at the end
static bool lowFor(BatMonitor &m, int mv, uint32_t t0, uint32_t ms) {
  bool r = false;
  for (uint32_t t = t0; t <= t0 + ms; t += 2000) { m.sample(mv, t); r = m.shutdownNeeded(t); }
  return r;
}

TEST(batcurve, shutdown_requires_sustained_low_voltage) {
  BatMonitor m;
  CHECK(!lowFor(m, 3250, 100000, 20000));  // 20 s is not enough
  CHECK(lowFor(m, 3250, 120000, 12000));   // 30+ s in a row is
}

TEST(batcurve, load_dip_does_not_shut_down) {
  BatMonitor m;
  for (uint32_t t = 100000; t < 130000; t += 2000) m.sample(3700, t);
  // a long load (sound, radio) sinks the voltage below the limit for 20 s and then it recovers
  CHECK(!lowFor(m, 3200, 130000, 20000));
  m.sample(3700, 152000); m.sample(3700, 154000); m.sample(3700, 156000); m.sample(3700, 158000);
  CHECK(!m.shutdownNeeded(160000));
}

TEST(batcurve, no_shutdown_right_after_boot) {
  BatMonitor m;
  for (uint32_t t = 0; t <= 20000; t += 2000) m.sample(3200, t);
  CHECK(!m.shutdownNeeded(20000));
  CHECK(!m.shutdownNeeded(BAT_BOOT_GRACE_MS - 1));
  for (uint32_t t = 22000; t <= 60000; t += 2000) m.sample(3200, t);
  CHECK(m.shutdownNeeded(60000));
}

TEST(batcurve, failed_reading_resets_the_count) {
  BatMonitor m;
  lowFor(m, 3250, 100000, 24000);
  CHECK_EQ(m.sample(0, 126000), -1);
  CHECK(!m.shutdownNeeded(128000));
  CHECK(!lowFor(m, 3250, 128000, 20000));
}

TEST(batcurve, percentage_only_drops_while_discharging) {
  BatMonitor m;
  m.seed(60);                              // the PMU said 60 when unplugged
  CHECK_EQ(m.sample(4100, 1000), 60);      // voltage inflated by charging does not raise it
  int a = 60;
  for (uint32_t t = 3000; t < 23000; t += 2000) a = m.sample(3700, t);  // the voltage really drops
  CHECK(a < 60 && a > 10);
  int b = m.sample(4000, 5000);            // a spike does not raise it either
  CHECK_EQ(b, a);
  CHECK_EQ(m.sample(3840, 7000) <= a, true);
}

TEST(batcurve, without_seed_starts_on_the_curve) {
  BatMonitor m;
  CHECK_EQ(m.sample(3840, 1000), 50);
  m.reset();
  CHECK_EQ(m.pct, -1);
}
