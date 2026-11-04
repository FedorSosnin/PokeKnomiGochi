#pragma once
#include "buddy.h"

// The buddy radio. On boards without the game's wifi all of this does nothing.
bool buddyNetUp();                       // connected to wifi and listening
uint32_t buddyNetId();                   // this board's identifier, never 0
bool buddyNetPoll(BuddyMsg &out);        // next valid message from another board, if any
void buddyNetSend(const BuddyMsg &m);    // to the buddy's IP if known; otherwise to the whole network
void buddyNetSetPeer(uint32_t id);       // 0 = none
// Holds the relay's connection closed while true. Wi-Fi updates need it: two TLS connections
// at once don't fit in memory (with the relay open, update checks failed with http -1).
void buddyNetRelayHold(bool hold);
