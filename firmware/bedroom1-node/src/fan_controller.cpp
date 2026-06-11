#include "fan_controller.h"

static uint8_t currentSpeed = 0;

void setFanSpeed(uint8_t speed)
{
    currentSpeed = speed;
}

uint8_t getFanSpeed()
{
    return currentSpeed;
}