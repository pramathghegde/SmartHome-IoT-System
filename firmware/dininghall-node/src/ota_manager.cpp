#include <WiFi.h>
#include <ArduinoOTA.h>

#include "ota_manager.h"
#include "config.h"
#include "secrets.h"

void initOTA()
{
    // WiFi.mode(WIFI_STA) must be called here ONCE before ESP-NOW init.
    // Do NOT call WiFi.mode() again in initEspNow().
    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("[OTA] Connecting WiFi");

    // Timeout added: if WiFi fails, do not block forever.
    // Without timeout, a missing router causes infinite hang.
    // Timeout: 15 seconds. If expired, continue without OTA.
    unsigned long wifiStart = millis();

    while (
        WiFi.status() != WL_CONNECTED &&
        (millis() - wifiStart) < 15000
    )
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[OTA] WiFi FAILED - OTA disabled, continuing without WiFi");
        // ESP-NOW works in WIFI_STA mode even without WiFi association.
        // DiningHall will still communicate via ESP-NOW.
        return;
    }

    Serial.print("[OTA] Connected. IP=");
    Serial.println(WiFi.localIP());

    ArduinoOTA.setHostname(NODE_NAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();

    Serial.println("[OTA] Ready");
}

void handleOTA()
{
    ArduinoOTA.handle();
}
