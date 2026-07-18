#include <Arduino.h>

#include "motion_manager.h"
#include "pins.h"

//----------------------------------------------------------
// LivingRoom supports multiple PIR sensors.
//
// Motion is considered ACTIVE if ANY PIR detects motion.
//
// The Master Controller receives only one aggregated
// motionDetected state.
//----------------------------------------------------------
bool isLivingRoomMotionDetected()
{
    bool pir1Triggered = digitalRead(PIR1_PIN);
    bool pir2Triggered = digitalRead(PIR2_PIN);

    return pir1Triggered || pir2Triggered;
}

static bool rawMotionState      = false;
static bool previousMotionState = false;

void initMotionSensor()
{
    pinMode(PIR1_PIN, INPUT_PULLDOWN);
    pinMode(PIR2_PIN, INPUT_PULLDOWN);

    Serial.println("[MOTION] Sensors initialized");
}

void updateMotionSensor()
{
    bool currentReading = isLivingRoomMotionDetected();

    // Print only on GPIO state change to keep serial clean
    if (currentReading != rawMotionState)
    {
        Serial.print("[Motion Sensor] ");
        Serial.println(currentReading ? "Motion HIGH" : "Motion LOW");
    }

    rawMotionState = currentReading;
}

// Returns raw Motion Sensor GPIO state.
// True = Motion Sensor currently HIGH (motion pulse active right now).
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
    // Not used in livingroom. Kept for header compatibility.
    return 0;
}
