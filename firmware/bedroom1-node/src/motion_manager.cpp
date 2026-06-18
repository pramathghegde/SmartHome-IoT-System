#include <Arduino.h>

#include "motion_manager.h"
#include "pins.h"

// Raw GPIO state from RCWL
static bool rawMotionState    = false;
static bool previousMotionState = false;

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
}

// Returns raw RCWL GPIO state
// Master is responsible for timeout and decision logic
bool isMotionDetected()
{
    return rawMotionState;
}

unsigned long getLastMotionTime()
{
    // Not used in bedroom1 anymore
    // Kept for header compatibility
    return 0;
}

bool hasMotionChanged()
{
    if (previousMotionState != rawMotionState)
    {
        previousMotionState = rawMotionState;

        Serial.print("[MOTION] ");
        Serial.println(
            rawMotionState ? "DETECTED" : "CLEARED"
        );

        return true;
    }

    return false;
}