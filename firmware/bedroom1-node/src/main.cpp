#include <Arduino.h>

#include "device_manager.h"
#include "relay_manager.h"

#include "device_ids.h"

void setup()
{
    Serial.begin(115200);

    initDevices();

    initRelays();
}

void loop()
{
    updateRelays();
}