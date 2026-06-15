#pragma once

#include "node_ids.h"

#define NODE_ID BEDROOM1_NODE

#define NODE_NAME "BEDROOM1"

// ---- TESTING VALUES (restore before production) ----
// Production: HEARTBEAT_INTERVAL 30000
// Production: MOTION_TIMEOUT     300000
#define HEARTBEAT_INTERVAL 3000

#define MOTION_TIMEOUT     6000
// ----------------------------------------------------

// Time (ms) before master is considered offline
// Must be > HEARTBEAT_INTERVAL * 3 at minimum
#define MASTER_TIMEOUT     9000

// LDR threshold: below this = dark, trigger automation
// ESP32-C3 ADC is 12-bit (0-4095), lower value = brighter
#define DARK_THRESHOLD     1200
