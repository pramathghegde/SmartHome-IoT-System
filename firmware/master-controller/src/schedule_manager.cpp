#include "schedule_manager.h"

bool isWithinSchedule(
    int currentHour,
    int currentMinute,
    int startHour,
    int startMinute,
    int stopHour,
    int stopMinute
)
{
    int current =
        currentHour * 60 +
        currentMinute;

    int start =
        startHour * 60 +
        startMinute;

    int stop =
        stopHour * 60 +
        stopMinute;

    if(start <= stop)
    {
        return
        (
            current >= start
            &&
            current < stop
        );
    }

    return
    (
        current >= start
        ||
        current < stop
    );
}