// Wi-Fi updates: version order and reading update.json (ota.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../src/ota.h"

TEST(ota, versions_compare_number_by_number) {
  CHECK_EQ(otaVersionCmp("1.19", "1.18"), 1);
  CHECK_EQ(otaVersionCmp("1.18", "1.18"), 0);
  CHECK_EQ(otaVersionCmp("1.9", "1.10"), -1);   // not alphabetical
  CHECK_EQ(otaVersionCmp("2.0", "1.99"), 1);
  CHECK_EQ(otaVersionCmp("1.18-knomi", "1.18"), 0);
  CHECK_EQ(otaVersionCmp("1.18.1", "1.18"), 1);
}

TEST(ota, reads_update_json) {
  const char *j = "{\n  \"version\": \"1.19\",\n  \"sprites_size\": 1290000,\n  \"sprites_sha256\": \"ab12cd\"\n}\n";
  char v[16], sz[16], sha[65];
  CHECK(otaJsonField(j, "version", v, sizeof(v)));
  CHECK(strcmp(v, "1.19") == 0);
  CHECK(otaJsonField(j, "sprites_size", sz, sizeof(sz)));
  CHECK(strcmp(sz, "1290000") == 0);
  CHECK(otaJsonField(j, "sprites_sha256", sha, sizeof(sha)));
  CHECK(strcmp(sha, "ab12cd") == 0);
}

TEST(ota, missing_or_too_long_is_refused) {
  const char *j = "{\"version\":\"1.19\",\"sprites\":\"x\"}";
  char v[4];
  CHECK(!otaJsonField(j, "sprites_size", v, sizeof(v)));  // "sprites" is not "sprites_size"
  CHECK(!otaJsonField(j, "version", v, sizeof(v)));       // "1.19" + 0 doesn't fit in 4
  CHECK(!otaJsonField("<html>404</html>", "version", v, sizeof(v)));
}

#if UI_V2
TEST(ota, auto_update_waits_for_an_idle_knomi_by_day) {
  CHECK_MSG(otaAutoNow(true, false, false, 9), "idle, awake, by day: now");
  CHECK_MSG(!otaAutoNow(false, false, false, 9), "not while someone's using it");
  CHECK_MSG(!otaAutoNow(true, true, false, 9), "not with something a restart would lose");
  CHECK_MSG(!otaAutoNow(true, false, false, 6) && otaAutoNow(true, false, false, 7) && otaAutoNow(true, false, false, 19) &&
            !otaAutoNow(true, false, false, 20), "awake: 7:00 to 19:59");
  CHECK_MSG(otaAutoNow(true, false, true, 12), "the light left off all day no longer holds it off for good");
  CHECK_MSG(!otaAutoNow(true, false, true, 9) && otaAutoNow(true, false, true, 10) && otaAutoNow(true, false, true, 17) &&
            !otaAutoNow(true, false, true, 18), "asleep: 10:00 to 17:59 only");
  CHECK_MSG(!otaAutoNow(true, false, true, 2) && !otaAutoNow(true, false, false, 23), "never at night");
}
#endif
