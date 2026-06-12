#include <Arduino.h>

#include "motion_manager.h"
#include "pins.h"
#include "config.h"

bool motionDetected = false;

static bool previousMotionState = false;

unsigned long lastMotionTime = 0;

void initMotionSensor()
{
    pinMode(RCWL_PIN, INPUT);
}

void updateMotionSensor()
{
    bool currentState = digitalRead(RCWL_PIN);

    if(currentState)
    {
        lastMotionTime = millis();
    }

    motionDetected =
        (millis() - lastMotionTime) < MOTION_TIMEOUT;
}

bool isMotionDetected()
{
    return motionDetected;
}

unsigned long getLastMotionTime()
{
    return lastMotionTime;
}

bool hasMotionChanged()
{
    if(previousMotionState != motionDetected)
    {
        previousMotionState = motionDetected;

        return true;
    }

    return false;
}