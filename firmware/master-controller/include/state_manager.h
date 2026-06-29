#pragma once

#include <stdint.h>

struct RoomState
{
    bool online;
    bool syncPending;

    bool motionDetected;

    int brightness;

    float temperature;

    float humidity;

    unsigned long lastHeartbeat;

    uint32_t lastNodeUptime;

    uint32_t lastBootCount;

    uint8_t lastResetReason;
    uint8_t lastWiFiStatus;
    uint32_t lastMinHeapKb;
    uint32_t lastLoopStackHighWater;
};

extern RoomState bedroom1;
