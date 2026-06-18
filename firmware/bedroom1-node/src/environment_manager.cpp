#include "environment_manager.h"
#include "ldr_manager.h"
#include <Arduino.h>

EnvironmentState environment = { 0 };

void updateEnvironment()
{
    environment.brightness = getBrightness();

    // Print every 5 seconds for monitoring
    static unsigned long lastPrint = 0;

    if (millis() - lastPrint > 5000)
    {
        lastPrint = millis();

        Serial.print("[LDR] Raw brightness: ");
        Serial.println(environment.brightness);
    }
}