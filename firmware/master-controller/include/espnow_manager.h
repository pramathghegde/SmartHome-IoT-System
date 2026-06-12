#pragma once

#include <stdint.h>

void initEspNow();

void sendDeviceCommand(
    uint8_t targetNode,
    uint8_t deviceID,
    bool state
);