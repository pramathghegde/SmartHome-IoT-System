#include "device_manager.h"

#include "device_ids.h"
#include "modes.h"
#include <Arduino.h>

#include "device_manager.h"
Device devices[5];

void initDevices()
{
    devices[0] = {FAN_DEVICE, false, MODE_AUTO, 0};

    devices[1] = {TUBELIGHT_DEVICE, false, MODE_AUTO, 0};

    devices[2] = {BULB_DEVICE, false, MODE_AUTO, 0};

    devices[3] = {SOCKET_DEVICE, false, MODE_MANUAL, 0};

    devices[4] = {AC_DEVICE, false, MODE_MANUAL, 0};
}

Device* getDevice(uint8_t deviceID)
{
    for(int i = 0; i < 5; i++)
    {
        if(devices[i].id == deviceID)
        {
            return &devices[i];
        }
    }

    return nullptr;
}

void setDeviceState(uint8_t deviceID, bool state)
{
    Device* device = getDevice(deviceID);

    if(device != nullptr)
    {
        device->state = state;

        Serial.print("[DEVICE] ID=");
        Serial.print(deviceID);

        Serial.print(" STATE=");

        Serial.println(state);
    }
}

bool getDeviceState(uint8_t deviceID)
{
    Device* device = getDevice(deviceID);

    if(device != nullptr)
    {
        return device->state;
    }

    return false;
}

void setDeviceMode(uint8_t deviceID, uint8_t mode)
{
    Device* device = getDevice(deviceID);

if(device != nullptr)
{
        if(device->mode != mode)
        {
            device->mode = mode;

        Serial.print("[DEVICE] ID=");
        Serial.print(deviceID);

        Serial.print(" MODE=");

        Serial.println(mode);
        }
    }
}

uint8_t getDeviceMode(uint8_t deviceID)
{
    Device* device = getDevice(deviceID);

    if(device != nullptr)
    {
        return device->mode;
    }

    return MODE_MANUAL;
}