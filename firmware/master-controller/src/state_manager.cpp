#include "state_manager.h"

RoomState bedroom1 =
{
    false, // online
    false, // syncPending
    false, // motionDetected
    false, // darkState
    0,     // brightness
    0,     // temperature
    0,     // humidity
    0,     // lastHeartbeat
    0,     // lastNodeUptime
    0      // lastBootCount
};

RoomState livingroom =
{
    false, // online
    false, // syncPending
    false, // motionDetected
    false, // darkState
    0,     // brightness
    0,     // temperature
    0,     // humidity
    0,     // lastHeartbeat
    0,     // lastNodeUptime
    0      // lastBootCount
};

RoomState dininghall =
{
    false, // online
    false, // syncPending
    false, // motionDetected
    false, // darkState
    0,     // brightness
    0,     // temperature
    0,     // humidity
    0,     // lastHeartbeat
    0,     // lastNodeUptime
    0      // lastBootCount
};


float globalTemperature = 0.0f;
float globalHumidity = 0.0f;

