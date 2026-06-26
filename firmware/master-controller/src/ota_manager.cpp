#include <WiFi.h>
#include <ArduinoOTA.h>

#include "ota_manager.h"

#include "secrets.h"

#include "config.h"

void initOTA()
{
    WiFi.mode(WIFI_STA);

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );

    Serial.print("Hostname: ");
    Serial.println(NODE_NAME);

    unsigned long wifiStart = millis();

    while(
        WiFi.status() != WL_CONNECTED &&
        (millis() - wifiStart) < 15000
    )
    {
        delay(500);
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[OTA] WiFi FAILED - OTA disabled");
        return;
    }

    ArduinoOTA.setHostname(NODE_NAME);

    ArduinoOTA.setPassword(
        OTA_PASSWORD
    );

    ArduinoOTA.begin();

    Serial.println("OTA Test");

    Serial.print("IP: ");

    Serial.println(
        WiFi.localIP()
    );
}

void handleOTA()
{
    ArduinoOTA.handle();
}
