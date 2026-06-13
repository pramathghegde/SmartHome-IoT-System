#include <WiFi.h>
#include <time.h>

#include "time_manager.h"

static bool timeValid = false;

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
        timeValid = true;

        Serial.println(
            "[TIME] NTP Synced"
        );
    }
    else
    {
        Serial.println(
            "[TIME] NTP Failed"
        );
    }
}

void updateTime()
{
    static unsigned long lastCheck = 0;

    if(
        millis() - lastCheck <
        60000
    )
    {
        return;
    }

    lastCheck = millis();

    struct tm timeinfo;

    if(getLocalTime(&timeinfo))
    {
        timeValid = true;
    }
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

bool isTimeValid()
{
    return timeValid;
}