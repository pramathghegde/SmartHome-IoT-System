#include "environment_manager.h"

#include "ldr_manager.h"

#include <Arduino.h>

EnvironmentState environment =
{
    0
};

void updateEnvironment()
{
    environment.brightness = getBrightness();

    static unsigned long lastPrint = 0;

    if(
        millis() - lastPrint >
        3000
    )
    {
        lastPrint = millis();

        Serial.print("[LDR] ");

        Serial.println(
            environment.brightness
        );
    }
}