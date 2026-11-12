#pragma once
#include <stdint.h>
// Wi-Fi updates (Knomi): reads update.json from the web installer's site and installs the new
// firmware in the other app slot, plus the sprites if they changed. The save (NVS) is never
// touched. A new firmware that doesn't reach otaConfirmBoot() is rolled back on the next boot.
// Elsewhere these are stubs that do nothing.

enum OtaState : uint8_t { OTA_IDLE, OTA_LATEST, OTA_AVAILABLE, OTA_FAILED };
typedef void (*OtaProgressCb)(uint8_t pct);

OtaState otaState();
const char *otaNewVersion();
bool otaCheck(const char *current);  // blocking (1-3 s); false if the site couldn't be read
bool otaInstall(OtaProgressCb cb);   // blocking; on success it restarts and never returns
void otaConfirmBoot();               // the running firmware works: no rollback
