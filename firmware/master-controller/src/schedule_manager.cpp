#include "schedule_manager.h"

#include "time_manager.h"

static bool isTimeInSchedule(uint8_t startHour, uint8_t startMinute, uint8_t stopHour, uint8_t stopMinute)
{
    int current = getHour() * 60 + getMinute();
    int start   = startHour * 60 + startMinute;
    int stop    = stopHour * 60 + stopMinute;

    if (start < stop)
    {
        return (current >= start && current < stop);
    }
    return (current >= start || current < stop);
}

bool isAutoScheduleActive(const DeviceConfig &device)
{
    return isTimeInSchedule(device.autoStartHour, device.autoStartMinute, device.autoStopHour, device.autoStopMinute);
}

bool isSchedScheduleActive(const DeviceConfig &device)
{
    return isTimeInSchedule(device.schedStartHour, device.schedStartMinute, device.schedStopHour, device.schedStopMinute);
}