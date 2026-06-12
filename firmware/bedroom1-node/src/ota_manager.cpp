#include <WiFi.h>
#include <ArduinoOTA.h>

#include "ota_manager.h"

#include "secrets.h"

#include "config.h"

void initOTA()
{
    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );

    while(
        WiFi.status() != WL_CONNECTED
    )
    {
        delay(500);
    }

    ArduinoOTA.setHostname(NODE_NAME);

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