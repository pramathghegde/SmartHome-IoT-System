#pragma once

#include <stdint.h>

#define BEDROOM2_NODE    3
#define HALL_NODE        4
#define KITCHEN_NODE     5
#define OUTDOOR_NODE     6
#define TANK_NODE        7
#define DOORLOCK_NODE    8

const char* getNodeName(uint8_t nodeID);

void updateHeartbeat(uint8_t nodeID);

void checkNodeStatus();