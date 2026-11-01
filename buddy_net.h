#pragma once
#include "buddy.h"

// La radio del buddy. En placas sin wifi del juego todo esto no hace nada.
bool buddyNetUp();                       // conectado a la wifi y escuchando
uint32_t buddyNetId();                   // identificador de esta placa, nunca 0
bool buddyNetPoll(BuddyMsg &out);        // siguiente mensaje valido de otra placa, si lo hay
void buddyNetSend(const BuddyMsg &m);    // a la IP del buddy si se conoce; si no, a toda la red
void buddyNetSetPeer(uint32_t id);       // 0 = ninguno
