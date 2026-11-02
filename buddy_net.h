#pragma once
#include "buddy.h"

// The buddy radio. On boards without the game's wifi all of this does nothing.
bool buddyNetUp();                       // connected to wifi and listening
uint32_t buddyNetId();                   // this board's identifier, never 0
bool buddyNetPoll(BuddyMsg &out);        // next valid message from another board, if any
void buddyNetSend(const BuddyMsg &m);    // to the buddy's IP if known; otherwise to the whole network
void buddyNetSetPeer(uint32_t id);       // 0 = none
