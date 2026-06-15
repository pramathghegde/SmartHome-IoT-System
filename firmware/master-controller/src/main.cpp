#include <Arduino.h>

#include "espnow_manager.h"
#include "ota_manager.h"
#include "node_manager.h"
#include "automation_manager.h"
#include "time_manager.h"
#include "state_manager.h"
#include "dashboard_manager.h"
#include "device_cache.h"

void setup()
{
    Serial.begin(115200);

    initDeviceCache();      // BUG FIX: was missing, device configs were uninitialized

    initEspNow();

    initOTA();

    initTime();

    initDashboard();        // Blynk.begin() is called here (connects WiFi + Blynk)

    Serial.println("[MASTER] BOOT COMPLETE");
}

void loop()
{
    handleOTA();

    updateDashboard();      // Blynk.run() + push status/feedback every 2s

    checkNodeStatus();

    runAutomation();

    updateTime();

    // Periodic serial status dump every 5 seconds
    static unsigned long lastStatus = 0;

    if (millis() - lastStatus > 5000)
    {
        lastStatus = millis();

        Serial.println("\n========== MASTER STATUS ==========");

        Serial.print("Bedroom1 Online: ");
        Serial.println(bedroom1.online);

        Serial.print("Motion: ");
        Serial.println(bedroom1.motionDetected);

        Serial.print("Brightness: ");
        Serial.println(bedroom1.brightness);

        Serial.print("Time: ");
        Serial.print(getHour());
        Serial.print(":");
        Serial.println(getMinute());

        Serial.println("===================================");
    }
}
