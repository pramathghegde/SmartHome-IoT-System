#pragma once

#include "node_ids.h"

#define NODE_ID            BEDROOM1_NODE
#define NODE_NAME          "BEDROOM1"

// How often bedroom1 sends heartbeat to master
// Testing : 5000   Production: 30000
// NOTE: 1000ms was too fast - caused ESP-NOW channel congestion
// when combined with ACK packets and Blynk WiFi traffic
#define HEARTBEAT_INTERVAL 2000

// MASTER_TIMEOUT REMOVED intentionally.
// Bedroom1 must NEVER change relay state because master is absent.
// Bedroom1 keeps last relay states and waits for master to return.
// This was a major source of spurious relay shutdowns.

// MOTION_TIMEOUT REMOVED intentionally.
// Motion timeout is a decision. Decisions belong to master only.
// Bedroom1 reports raw RCWL GPIO state. Master decides duration.
