#include "device_cache.h"

#include "modes.h"

DeviceConfig bedroom1Fan;
DeviceConfig bedroom1Tube;
DeviceConfig bedroom1Bulb;
DeviceConfig bedroom1Socket;
DeviceConfig bedroom1AC;

void initDeviceCache()
{
    bedroom1Fan =
    {
        MODE_AUTO,
        false,
        23,0,
        5,0
    };

    bedroom1Tube =
    {
        MODE_AUTO,
        false,
        18,0,
        23,0
    };

    bedroom1Bulb =
    {
        MODE_AUTO,
        false,
        18,0,
        23,0
    };

    bedroom1Socket =
    {
        MODE_OFF,
        false,
        0,0,
        0,0
    };

    bedroom1AC =
    {
        MODE_OFF,
        false,
        0,0,
        0,0
    };
}