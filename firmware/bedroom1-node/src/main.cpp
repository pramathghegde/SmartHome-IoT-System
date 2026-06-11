#include <Arduino.h>

#include "device_manager.h"
#include "relay_manager.h"
#include "motion_manager.h"

void setup()
{
    Serial.begin(115200);

    initDevices();

    initRelays();

    initMotionSensor();
}

void loop()
{
    updateMotionSensor();

    updateRelays();

    Serial.print("Motion: ");

    Serial.println(isMotionDetected());

    delay(500);
}