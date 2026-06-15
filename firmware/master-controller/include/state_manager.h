#pragma once

#include <stdint.h>

struct RoomState
{
    bool online;

    bool motionDetected;

    int brightness;

    float temperature;

    float humidity;

    unsigned long lastHeartbeat;
};

extern RoomState bedroom1;