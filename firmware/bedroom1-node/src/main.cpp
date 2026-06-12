#include <Arduino.h>

#include "device_manager.h"
#include "relay_manager.h"
#include "motion_manager.h"

#include "espnow_manager.h"
#include "ota_manager.h"

void setup()
{
    Serial.begin(115200);

    initDevices();

    initRelays();

    initMotionSensor();

    initEspNow();

    initOTA();
}

void loop()
{
    handleOTA();

    updateMotionSensor();

    if(hasMotionChanged())
    {
        sendMotionStatus(
            isMotionDetected()
        );
    }

    updateRelays();

    sendHeartbeat();
}