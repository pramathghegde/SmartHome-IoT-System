#include <Arduino.h>

#include "espnow_manager.h"

#include "ota_manager.h"

#include "node_manager.h"

#include "automation_manager.h"

#include "time_manager.h"

void setup()
{
    Serial.begin(115200);

    initEspNow();

    initOTA();

    initTime();
}

void loop()
{
    handleOTA();

    checkNodeStatus();

    runAutomation();
}