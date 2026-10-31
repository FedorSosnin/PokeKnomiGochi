#pragma once
#include <stdint.h>
// Cambio de wifi desde el movil: el Knomi abre su propia red con clave aleatoria y sirve una
// pagina para elegir la nueva. La clave solo pasa por el movil y la NVS.

enum WifiSetupState : uint8_t { WS_OFF, WS_BOOT, WS_WAIT, WS_TRY, WS_OK, WS_FAIL };

void wifiSetupStart();
void wifiSetupStop();
void wifiSetupPoll();              // sondear en el loop
WifiSetupState wifiSetupState();
const char *wifiSetupApSsid();
const char *wifiSetupApPass();
int wifiSetupQrSize();             // modulos por lado; 0 si no hay QR
bool wifiSetupQrDark(int x, int y);
void wifiSetupTry(const char *ssid, const char *pass);  // tambien desde la consola serie
