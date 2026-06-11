#pragma once

#include <stdint.h>

void updateNodeHeartbeat(
    uint8_t nodeID
);

bool isNodeOnline(
    uint8_t nodeID
);