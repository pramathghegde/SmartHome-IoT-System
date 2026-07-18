#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "device_manager.h"
#include "relay_manager.h"
#include "motion_manager.h"
#include "espnow_manager.h"
#include "ota_manager.h"

RTC_DATA_ATTR int bootCount = 0;

static const char* resetReasonName(esp_reset_reason_t reason)
{
    switch (reason)
    {
        case ESP_RST_POWERON:   return "POWERON";
        case ESP_RST_EXT:       return "EXTERNAL";
        case ESP_RST_SW:        return "SOFTWARE";
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_INT_WDT:   return "INT_WDT";
        case ESP_RST_TASK_WDT:  return "TASK_WDT";
        case ESP_RST_WDT:       return "WDT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "UNKNOWN";
    }
}

static void printBootDiagnostics()
{
    bootCount++;
    esp_reset_reason_t reason = esp_reset_reason();

    Serial.println("[BOOT] System Starting...");
    Serial.print("[RESET] Reason=");
    Serial.print(resetReasonName(reason));
    Serial.print(" (");
    Serial.print(static_cast<int>(reason));
    Serial.println(")");

    Serial.print("[RESET] BootCount=");
    Serial.println(bootCount);

    Serial.print("[HEAP] Free=");
    Serial.println(ESP.getFreeHeap());

    Serial.print("[HEAP] Min=");
    Serial.println(ESP.getMinFreeHeap());

    Serial.print("[HEAP] Largest=");
    Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
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

    Serial.println("[DIAG] DININGHALL");
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

#include "mac_addresses.h"
#include "packet.h"

void setup()
{
    Serial.begin(115200);
    delay(500);

    printBootDiagnostics();
    Serial.printf("[BOOT] LIVINGROOM_MAC Configured = %02X:%02X:%02X:%02X:%02X:%02X\n",
                  LIVINGROOM_MAC[0], LIVINGROOM_MAC[1], LIVINGROOM_MAC[2], LIVINGROOM_MAC[3], LIVINGROOM_MAC[4], LIVINGROOM_MAC[5]);
    Serial.printf("[BOOT] sizeof(Packet) = %d\n", sizeof(Packet));

    initDevices();

    initRelays();

    initMotionSensor();


    // initOTA() connects WiFi and sets WiFi.mode(WIFI_STA).
    // This MUST happen before initEspNow().
    // ESP-NOW requires WIFI_STA mode to be set first.
    initOTA();

    // initEspNow() must NOT call WiFi.mode() again.
    // WiFi mode is already set by initOTA().
    initEspNow();

    Serial.println("[DININGHALL] Boot complete");
}

void loop()
{
    handleOTA();

    updateMotionSensor();   // read raw Motion Sensor GPIO

    if (hasMotionChanged() && isMotionDetected())
    {
        sendMotionStatus(true);
    }

    processIncomingPackets();

    // -------------------------------------------------------
    // MASTER WATCHDOG COMPLETELY REMOVED.
    //
    // DiningHall must NEVER change relay state because master
    // is absent, disconnected, or timed out.
    //
    // If master disappears:
    //   - DiningHall keeps last received relay states
    //   - ESP-NOW stays alive
    //   - DiningHall waits for master to return
    //   - When master returns, it re-syncs all device states
    //
    // Removing this was the primary fix for:
    //   - Spurious appliance shutdowns
    //   - Unexpected relay OFF events
    //   - Communication blackout periods caused by the
    //     restart cascade from safety shutdowns
    // -------------------------------------------------------

    sendHeartbeat();

    printRuntimeDiagnostics();
}
