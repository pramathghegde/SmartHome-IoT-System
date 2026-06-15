#pragma once

enum CommandType
{
    CMD_ON = 1,

    CMD_OFF,

    CMD_SET_DEVICE_STATE,

    CMD_SET_MODE,

    CMD_STATUS,

    CMD_HEARTBEAT,

    CMD_MOTION,

    CMD_ENVIRONMENT,

    CMD_FAN_SPEED

    // Future:
    // CMD_LOCK
    // CMD_UNLOCK
    // CMD_TANK_LEVEL
    // CMD_OTA_STATUS
    // CMD_ERROR
};
