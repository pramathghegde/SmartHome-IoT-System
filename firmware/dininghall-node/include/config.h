#pragma once

#include "node_ids.h"

#define NODE_ID            DININGHALL_NODE
#define NODE_NAME          "DININGHALL"

// How often dininghall sends heartbeat to master
// Testing : 5000   Production: 30000
// when combined with ACK packets and Blynk WiFi traffic
#define HEARTBEAT_INTERVAL 1000

// MASTER_TIMEOUT REMOVED intentionally.
// DiningHall must NEVER change relay state because master is absent.
// DiningHall keeps last relay states and waits for master to return.
// This was a major source of spurious relay shutdowns.

// MOTION_TIMEOUT REMOVED intentionally.
// Motion timeout is a decision. Decisions belong to master only.
// DiningHall reports raw Motion Sensor GPIO state. Master decides duration.
