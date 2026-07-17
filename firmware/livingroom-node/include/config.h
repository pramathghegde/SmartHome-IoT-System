#pragma once

#include "node_ids.h"

#define NODE_ID            LIVINGROOM_NODE
#define NODE_NAME          "LIVINGROOM"

// How often livingroom sends heartbeat to master
// Testing : 5000   Production: 30000
// when combined with ACK packets and Blynk WiFi traffic
#define HEARTBEAT_INTERVAL 1000

// MASTER_TIMEOUT REMOVED intentionally.
// LivingRoom must NEVER change relay state because master is absent.
// LivingRoom keeps last relay states and waits for master to return.
// This was a major source of spurious relay shutdowns.

// MOTION_TIMEOUT REMOVED intentionally.
// Motion timeout is a decision. Decisions belong to master only.
// LivingRoom reports raw Motion Sensor GPIO state. Master decides duration.
