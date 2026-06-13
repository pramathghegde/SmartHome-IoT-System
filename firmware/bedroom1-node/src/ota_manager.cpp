#include <WiFi.h>
#include <ArduinoOTA.h>

#include "ota_manager.h"

#include "config.h"

#include "secrets.h"

void initOTA()
{
    WiFi.mode(WIFI_STA);

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );

    Serial.print("Hostname: ");
    Serial.println(NODE_NAME);

    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    ArduinoOTA.setHostname(
        NODE_NAME
    );

    ArduinoOTA.setPassword(
        OTA_PASSWORD
    );

    ArduinoOTA.begin();

    Serial.println("OTA Ready");

    Serial.print("IP: ");

    Serial.println(
        WiFi.localIP()
    );
}

void handleOTA()
{
    ArduinoOTA.handle();
}