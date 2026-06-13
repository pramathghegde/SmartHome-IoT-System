#pragma once

#include <stdint.h>

struct DeviceConfig
{
    uint8_t mode;

    bool state;

    uint8_t startHour;

    uint8_t startMinute;

    uint8_t stopHour;

    uint8_t stopMinute;
};

struct Bedroom1State
{
    bool online;

    bool motionDetected;

    int brightness;

    unsigned long lastHeartbeat;
};

extern Bedroom1State bedroom1;

extern DeviceConfig fanConfig;

extern DeviceConfig tubeConfig;

extern DeviceConfig bulbConfig;

extern DeviceConfig socketConfig;

extern DeviceConfig acConfig;