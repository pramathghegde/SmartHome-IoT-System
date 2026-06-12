#include <WiFi.h>
#include <time.h>

#include "time_manager.h"

static bool synced = false;

void initTime()
{
    configTime(
        19800,
        0,
        "pool.ntp.org",
        "time.nist.gov"
    );

    struct tm timeinfo;

    if(getLocalTime(&timeinfo))
    {
        synced = true;
    }
}

bool isTimeSynced()
{
    return synced;
}

int getHour()
{
    struct tm timeinfo;

    if(getLocalTime(&timeinfo))
    {
        return timeinfo.tm_hour;
    }

    return 0;
}

int getMinute()
{
    struct tm timeinfo;

    if(getLocalTime(&timeinfo))
    {
        return timeinfo.tm_min;
    }

    return 0;
}