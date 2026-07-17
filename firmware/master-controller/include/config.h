#pragma once

#include "node_ids.h"

#define NODE_ID   MASTER_NODE
#define NODE_NAME "MASTER"

// Must match bedroom1 MASTER_TIMEOUT
// Testing : 20000  Production: 120000
#define HEARTBEAT_TIMEOUT    120000

// Motion hold time after last Motion HIGH
// Testing : 10000  Production: 300000

// Room-specific LDR hysteresis thresholds
constexpr uint16_t BEDROOM1_DARK_ENTER_THRESHOLD    = 3100;
constexpr uint16_t BEDROOM1_DARK_EXIT_THRESHOLD     = 3300;

constexpr uint16_t LIVINGROOM_DARK_ENTER_THRESHOLD  = 2900;
constexpr uint16_t LIVINGROOM_DARK_EXIT_THRESHOLD   = 3500;
