#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
    initOTA();
    initEspNow();
    initTime();
    initDashboard();

    Serial.println("[MASTER] Boot complete");
}

static void printRuntimeDiagnostics()
{
    static unsigned long lastPrint = 0;
    static unsigned long lastLoop  = 0;
    static unsigned long maxGap    = 0;

    unsigned long now = millis();

    if (lastLoop != 0)
    {
        unsigned long gap = now - lastLoop;

        if (gap > maxGap)
        {
            maxGap = gap;
        }
    }

    lastLoop = now;

    if (now - lastPrint < 30000)
    {
        return;
    }

    lastPrint = now;

    Serial.println("[DIAG] MASTER");
    Serial.print("[HEAP] Free=");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" Min=");
    Serial.print(ESP.getMinFreeHeap());
    Serial.print(" Largest=");
    Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    Serial.print("[STACK] LoopHighWater=");
    Serial.println(uxTaskGetStackHighWaterMark(nullptr));
    Serial.print("[WDT] LoopTask=");
    Serial.println(esp_task_wdt_status(nullptr) == ESP_OK ? "SUBSCRIBED" : "NOT_SUBSCRIBED");
    Serial.print("[WIFI] Status=");
    Serial.print(WiFi.status());
    Serial.print(" Channel=");
    Serial.print(WiFi.channel());
    Serial.print(" RSSI=");
    Serial.println(WiFi.RSSI());
    Serial.print("[LOOP] MaxGapMs=");
    Serial.println(maxGap);
    maxGap = 0;

    printEspNowDiagnostics();
}

void loop()
{
    handleOTA();
    processIncomingPackets();
    updateDashboard();
    checkNodeStatus();
    runAutomation();
    updateTime();
    printRuntimeDiagnostics();

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
