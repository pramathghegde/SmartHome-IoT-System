#include "schedule_manager.h"

#include "time_manager.h"

bool isNightRestrictionActive()
{
    int hour = getHour();

    return (
        hour >= 0
        &&
        hour < 8
    );
}

bool isOutdoorLightTime()
{
    int hour = getHour();

    return (
        hour >= 18
        &&
        hour < 23
    );
}