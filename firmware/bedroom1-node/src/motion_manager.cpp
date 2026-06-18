#include <Arduino.h>

#include "motion_manager.h"
#include "config.h"
#include "pins.h"

// Raw GPIO state from RCWL
static bool rawMotionState     = false;
static bool previousRawState   = false;
static bool previousLatchState = false;

static bool motionLatched = false;
static bool motionReportedSinceLatch = false;
static unsigned long lastMotionTime = 0;

void initMotionSensor()
{
    pinMode(RCWL_PIN, INPUT);

    Serial.println("[MOTION] Sensor initialized");
}

void updateMotionSensor()
{
    bool currentReading = digitalRead(RCWL_PIN);

    // Debug: print only on GPIO state change
    if (currentReading != rawMotionState)
    {
        Serial.print("[RCWL RAW] GPIO=");
        Serial.println(currentReading ? "HIGH" : "LOW");
    }

    rawMotionState = currentReading;

    if (currentReading && !previousRawState)
    {
        Serial.println("[MOTION] Rising Edge");

        motionLatched = true;
        motionReportedSinceLatch = false;
        lastMotionTime = millis();

        Serial.println("[MOTION] Latched");
    }

    previousRawState = currentReading;

    if (
        motionLatched &&
        motionReportedSinceLatch &&
        (millis() - lastMotionTime) > MOTION_TIMEOUT
    )
    {
        motionLatched = false;
        Serial.println("[MOTION] Cleared");
    }
}

// Returns latched RCWL motion state, not the raw GPIO state.
bool isMotionDetected()
{
    return motionLatched;
}

void markMotionReported()
{
    if (motionLatched)
    {
        motionReportedSinceLatch = true;
    }
}

unsigned long getLastMotionTime()
{
    return lastMotionTime;
}

bool hasMotionChanged()
{
    if (previousLatchState != motionLatched)
    {
        previousLatchState = motionLatched;

        Serial.print("[MOTION] ");
        Serial.println(
            motionLatched ? "DETECTED" : "CLEARED"
        );

        return true;
    }

    return false;
}
