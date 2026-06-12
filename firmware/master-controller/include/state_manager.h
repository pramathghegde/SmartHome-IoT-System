#pragma once

struct Bedroom1State
{
    bool online;

    bool motionDetected;

    int brightness;

    unsigned long lastHeartbeat;
};

extern Bedroom1State bedroom1;