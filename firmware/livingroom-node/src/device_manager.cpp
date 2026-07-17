#include "device_manager.h"

#include "pins.h"
#include "modes.h"

Device devices[8];

void initDevices()
{
    devices[0] =
    {
        TUBE1_DEVICE,
        false,
        MODE_AUTO,
        0
    };

    devices[1] =
    {
        TUBE2_DEVICE,
        false,
        MODE_AUTO,
        0
    };

    devices[2] =
    {
        FAN_DEVICE,
        false,
        MODE_AUTO,
        0
    };

    devices[3] =
    {
        EBIKE_DEVICE,
        false,
        MODE_OFF,
        0
    };

    devices[4] =
    {
        SOCKET_DEVICE,
        false,
        MODE_OFF,
        0
    };

    devices[5] =
    {
        OUTSIDE_BULB_DEVICE,
        false,
        MODE_AUTO,
        0
    };

    devices[6] =
    {
        EXTRA1_DEVICE,
        false,
        MODE_OFF,
        0
    };

    devices[7] =
    {
        EXTRA2_DEVICE,
        false,
        MODE_OFF,
        0
    };
}

Device* getDevice(uint8_t deviceID)
{
    for(int i = 0; i < 8; i++)
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
    Device* device =
        getDevice(deviceID);

    if(device != nullptr)
    {
        device->state = state;
    }
}

bool getDeviceState(uint8_t deviceID)
{
    Device* device =
        getDevice(deviceID);

    if(device != nullptr)
    {
        return device->state;
    }

    return false;
}

void setDeviceMode(
    uint8_t deviceID,
    uint8_t mode
)
{
    Device* device =
        getDevice(deviceID);

    if(device != nullptr)
    {
        device->mode = mode;
    }
}

uint8_t getDeviceMode(
    uint8_t deviceID
)
{
    Device* device =
        getDevice(deviceID);

    if(device != nullptr)
    {
        return device->mode;
    }

    return MODE_OFF;
}