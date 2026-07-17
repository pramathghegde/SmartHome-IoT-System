#include "environment_manager.h"
#include "dht_manager.h"
#include <Arduino.h>

EnvironmentState environment = { 0.0f, 0.0f };

void initEnvironment()
{
    initDHT();
}

void updateEnvironment()
{
    // DHT11 should not be polled too frequently (minimum 2s interval)
    static unsigned long lastRead = 0;
    if (millis() - lastRead >= 2000)
    {
        lastRead = millis();
        environment.temperature = readTemperature();
        environment.humidity = readHumidity();
    }

    // Print every 5 seconds for monitoring
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 5000)
    {
        lastPrint = millis();

        Serial.print("[DHT] Temp: ");
        Serial.print(environment.temperature, 1);
        Serial.print("C | Humid: ");
        Serial.print(environment.humidity, 1);
        Serial.println("%");
    }
}