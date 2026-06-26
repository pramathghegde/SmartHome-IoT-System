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
};

extern RoomState bedroom1;
