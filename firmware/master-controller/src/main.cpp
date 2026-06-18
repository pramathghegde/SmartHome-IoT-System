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
    delay(500);

    Serial.println("[MASTER] Booting...");

    initDeviceCache();
    initEspNow();
    initOTA();
    initTime();
    initDashboard();

    Serial.println("[MASTER] Boot complete");
}

void loop()
{
    handleOTA();
    updateDashboard();
    checkNodeStatus();
    runAutomation();
    updateTime();

    static unsigned long lastStatus = 0;

    if (millis() - lastStatus > 10000)
    {
        lastStatus = millis();

        // Uptime overflow safe: millis() wraps at 49.7 days
        // Divide first to keep within uint32 range
        uint32_t uptimeSec = millis() / 1000;
        uint32_t days      = uptimeSec / 86400;
        uint32_t hours     = (uptimeSec % 86400) / 3600;
        uint32_t mins      = (uptimeSec % 3600) / 60;
        uint32_t secs      = uptimeSec % 60;

        Serial.println("\n===== MASTER STATUS =====");
        Serial.print("Uptime  : ");
        Serial.print(days);
        Serial.print("d ");
        Serial.print(hours);
        Serial.print("h ");
        Serial.print(mins);
        Serial.print("m ");
        Serial.print(secs);
        Serial.println("s");
        Serial.print("B1      : ");
        Serial.println(bedroom1.online ? "ONLINE" : "OFFLINE");
        Serial.print("Motion  : ");
        Serial.println(bedroom1.motionDetected ? "YES" : "NO");
        Serial.print("Bright  : ");
        Serial.println(bedroom1.brightness);
        Serial.print("Time    : ");
        Serial.print(getHour());
        Serial.print(":");
        if (getMinute() < 10) Serial.print("0");
        Serial.println(getMinute());
        Serial.println("=========================");
    }
}