#pragma once
#include <stdint.h>
// Wifi change on the screen itself: scans for networks, one is chosen and the password is typed. The new
// network is only saved once it has connected; if it fails, the previous one stays saved.

enum WifiSetupState : uint8_t { WS_OFF, WS_SCAN, WS_LIST, WS_PASS, WS_TRY, WS_OK, WS_FAIL };

void wifiSetupStart();
void wifiSetupStop();
void wifiSetupPoll();              // poll in the loop
WifiSetupState wifiSetupState();
int wifiSetupNetCount();
const char *wifiSetupNet(int i);
void wifiSetupRescan();
void wifiSetupPick(int i);         // open network: connects now; with password: goes to the keyboard
void wifiSetupBackToList();
const char *wifiSetupSsid();       // chosen network
const char *wifiSetupPass();
void wifiSetupType(char c);
void wifiSetupDelete();
bool wifiSetupCanSubmit();
void wifiSetupSubmit();
void wifiSetupTry(const char *ssid, const char *pass);  // also from the serial console
