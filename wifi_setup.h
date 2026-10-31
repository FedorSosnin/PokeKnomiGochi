#pragma once
#include <stdint.h>
// Cambio de wifi en la propia pantalla: busca redes, se elige una y se teclea la clave. La red
// nueva solo se guarda cuando ha conectado; si falla, sigue guardada la anterior.

enum WifiSetupState : uint8_t { WS_OFF, WS_SCAN, WS_LIST, WS_PASS, WS_TRY, WS_OK, WS_FAIL };

void wifiSetupStart();
void wifiSetupStop();
void wifiSetupPoll();              // sondear en el loop
WifiSetupState wifiSetupState();
int wifiSetupNetCount();
const char *wifiSetupNet(int i);
void wifiSetupRescan();
void wifiSetupPick(int i);         // red abierta: conecta ya; con clave: pasa al teclado
void wifiSetupBackToList();
const char *wifiSetupSsid();       // red elegida
const char *wifiSetupPass();
void wifiSetupType(char c);
void wifiSetupDelete();
bool wifiSetupCanSubmit();
void wifiSetupSubmit();
void wifiSetupTry(const char *ssid, const char *pass);  // tambien desde la consola serie
