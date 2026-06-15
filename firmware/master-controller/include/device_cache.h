#pragma once

#include <stdint.h>

// TODO: Migrate to DeviceConfig devices[64] indexed by global device IDs
// (see device_ids.h for future BEDROOM1_FAN, BEDROOM2_FAN etc.)
// Current named variables kept to avoid breaking existing automation/dashboard code.

struct DeviceConfig
{
    uint8_t mode;

    bool currentState;

    uint8_t startHour;
    uint8_t startMinute;

    uint8_t stopHour;
    uint8_t stopMinute;
};

extern DeviceConfig bedroom1Fan;
extern DeviceConfig bedroom1Tube;
extern DeviceConfig bedroom1Bulb;
extern DeviceConfig bedroom1Socket;
extern DeviceConfig bedroom1AC;

void initDeviceCache();
