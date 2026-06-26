#include <Arduino.h>

#include "motion_manager.h"
#include "pins.h"

// Raw GPIO state from RCWL only.
// No latch. No timeout. No motion duration decisions.
// All decision logic belongs to master exclusively.

static bool rawMotionState      = false;
static bool previousMotionState = false;

void initMotionSensor()
{
    pinMode(RCWL_PIN, INPUT);

    Serial.println("[MOTION] Sensor initialized");
}

void updateMotionSensor()
{
    bool currentReading = digitalRead(RCWL_PIN);

    // Print only on GPIO state change to keep serial clean
    if (currentReading != rawMotionState)
    {
        Serial.print("[RCWL] ");
        Serial.println(currentReading ? "HIGH" : "LOW");
    }

    rawMotionState = currentReading;
}

// Returns raw RCWL GPIO state.
// True = RCWL currently HIGH (motion pulse active right now).
// Master applies its own MOTION_TIMEOUT to this raw value.
bool isMotionDetected()
{
    return rawMotionState;
}

bool hasMotionChanged()
{
    if (previousMotionState != rawMotionState)
    {
        previousMotionState = rawMotionState;

        Serial.print("[MOTION] ");
        Serial.println(rawMotionState ? "DETECTED" : "CLEARED");

        return true;
    }

    return false;
}

unsigned long getLastMotionTime()
{
    // Not used in bedroom1. Kept for header compatibility.
    return 0;
}
