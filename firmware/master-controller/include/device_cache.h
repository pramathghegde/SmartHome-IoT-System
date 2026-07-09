#pragma once

#include <stdint.h>
#include <Preferences.h>

// TODO: Migrate to DeviceConfig devices[64] indexed by global device IDs
// (see device_ids.h for future BEDROOM1_FAN, BEDROOM2_FAN etc.)
// Current named variables kept to avoid breaking existing automation/dashboard code.

struct DeviceConfig
{
    uint8_t mode;

    bool currentState;

    // AUTO Mode Schedule
    uint8_t autoStartHour;
    uint8_t autoStartMinute;
    uint8_t autoStopHour;
    uint8_t autoStopMinute;

    // SCHEDULE Mode Schedule
    uint8_t schedStartHour;
    uint8_t schedStartMinute;
    uint8_t schedStopHour;
    uint8_t schedStopMinute;
};

extern DeviceConfig bedroom1Fan;
extern DeviceConfig bedroom1Tube;
extern DeviceConfig bedroom1Bulb;
extern DeviceConfig bedroom1Socket;
extern DeviceConfig bedroom1AC;
extern bool bedroom1LdrEnabled;

void initDeviceCache();
void loadConfiguration();
void saveConfiguration();
void saveDeviceConfiguration(Preferences &prefs, const char* prefix, const DeviceConfig &device);
bool loadDeviceConfiguration(Preferences &prefs, const char* prefix, DeviceConfig &device, const DeviceConfig &defaultConfig);
bool validateConfiguration(DeviceConfig &device, const DeviceConfig &defaultConfig);
void saveSingleDevice(const char* prefix, const DeviceConfig &device);
void saveRoomLdrEnabled(bool enabled);
void printRestoredConfiguration();


