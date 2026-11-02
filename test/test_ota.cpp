// Wi-Fi updates: version order and reading update.json (ota.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../ota.h"

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
