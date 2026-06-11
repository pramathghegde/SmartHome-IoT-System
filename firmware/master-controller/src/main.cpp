#include <Arduino.h>

#include "espnow_manager.h"
#include "ota_manager.h"

void setup()
{
    Serial.begin(115200);

    initEspNow();

    initOTA();
}

void loop()
{
    handleOTA();
}