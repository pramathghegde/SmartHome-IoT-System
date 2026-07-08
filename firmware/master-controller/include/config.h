#pragma once

#include "node_ids.h"

#define NODE_ID   MASTER_NODE
#define NODE_NAME "MASTER"

// Must match bedroom1 MASTER_TIMEOUT
// Testing : 20000  Production: 120000
#define HEARTBEAT_TIMEOUT    120000

// Motion hold time after last Motion HIGH
// Testing : 10000  Production: 300000
#define MOTION_TIMEOUT       60000

// LDR hysteresis thresholds
#define DARK_THRESHOLD_LOW   1100
#define DARK_THRESHOLD_HIGH  1300
