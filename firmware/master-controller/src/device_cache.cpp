#include "device_cache.h"

static bool tubeState = false;

static bool bulbState = false;

bool getTubeLightState()
{
    return tubeState;
}

bool getBulbState()
{
    return bulbState;
}

void setTubeLightState(bool state)
{
    tubeState = state;
}

void setBulbState(bool state)
{
    bulbState = state;
}