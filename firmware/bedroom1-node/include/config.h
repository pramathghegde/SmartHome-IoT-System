#pragma once

#include "node_ids.h"

#define NODE_ID            BEDROOM1_NODE
#define NODE_NAME          "BEDROOM1"

// Testing : 5000   Production: 30000
#define HEARTBEAT_INTERVAL 5000

// Must be > HEARTBEAT_INTERVAL * 3
// Testing : 20000  Production: 120000
#define MASTER_TIMEOUT     20000