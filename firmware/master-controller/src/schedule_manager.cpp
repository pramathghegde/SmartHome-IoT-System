#include "schedule_manager.h"

#include "time_manager.h"

bool isScheduleActive(
    DeviceConfig &device
)
{
    int current =
        getHour() * 60 +
        getMinute();

    int start =
        device.startHour * 60 +
        device.startMinute;

    int stop =
        device.stopHour * 60 +
        device.stopMinute;

    if(start < stop)
    {
        return
        (
            current >= start &&
            current < stop
        );
    }

    return
    (
        current >= start ||
        current < stop
    );
}