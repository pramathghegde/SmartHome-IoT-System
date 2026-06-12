#pragma once

struct Bedroom1State
{
    bool online;

    bool motionDetected;

    int brightness;

    bool fanState;

    bool tubeState;

    bool bulbState;

    bool socketState;

    bool acState;

    unsigned long lastHeartbeat;
};

extern Bedroom1State bedroom1;