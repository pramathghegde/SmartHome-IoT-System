#include "state_manager.h"

RoomState bedroom1 =
{
    false, // online
    false, // syncPending
    false, // motionDetected
    0,     // brightness
    0,     // temperature
    0,     // humidity
    0,     // lastHeartbeat
    0,     // lastNodeUptime
    0,     // lastBootCount
    0,     // lastResetReason
    0,     // lastWiFiStatus
    0,     // lastMinHeapKb
    0      // lastLoopStackHighWater
};
