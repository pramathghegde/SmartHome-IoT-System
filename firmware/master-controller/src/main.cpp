#include <Arduino.h>

#include "espnow_manager.h"

#include "ota_manager.h"

#include "node_manager.h"

#include "automation_manager.h"

#include "time_manager.h"

#include "state_manager.h"

void setup()
{
    Serial.begin(115200);

    initEspNow();

    initOTA();

    initTime();
}

void loop()
{
    handleOTA();

    checkNodeStatus();

    runAutomation();

    static unsigned long lastStatus = 0;

    if(
        millis() - lastStatus >
        5000
    )
    {
        lastStatus = millis();

        Serial.println(
            "\n========== MASTER STATUS =========="
        );

        Serial.print(
            "Bedroom1 Online: "
        );

        Serial.println(
            bedroom1.online
        );

        Serial.print(
            "Motion: "
        );

        Serial.println(
            bedroom1.motionDetected
        );

        Serial.print(
            "Brightness: "
        );

        Serial.println(
            bedroom1.brightness
        );

        Serial.println(
            "==================================="
        );
    }
}