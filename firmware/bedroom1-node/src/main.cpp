#include <Arduino.h>

#include "device_manager.h"
#include "relay_manager.h"
#include "motion_manager.h"
#include "espnow_manager.h"
#include "ota_manager.h"
#include "environment_manager.h"
#include "ldr_manager.h"
#include "device_ids.h"

void setup()
{
    Serial.begin(115200);

    delay(500);

    Serial.println("[BEDROOM1] Booting...");

    initDevices();

    initRelays();

    initMotionSensor();

    initLDR();

    initOTA();      // connects WiFi + starts ArduinoOTA

    initEspNow();   // registers ESP-NOW callbacks + peer

    Serial.println("[BEDROOM1] Boot complete");
}

void loop()
{
    handleOTA();

    updateMotionSensor();   // update RCWL edge latch

    updateLDR();            // read raw ADC

    updateEnvironment();    // store brightness into environment struct

    processIncomingPackets();

    static bool masterOfflineHandled = false;

    if (!isMasterOnline())
    {
        if (!masterOfflineHandled)
        {
            Serial.println("[SAFETY] MASTER OFFLINE - turning off all relays");

            setDeviceState(FAN_DEVICE,      false);
            setDeviceState(TUBELIGHT_DEVICE,false);
            setDeviceState(BULB_DEVICE,     false);
            setDeviceState(SOCKET_DEVICE,   false);
            setDeviceState(AC_DEVICE,       false);

            masterOfflineHandled = true;
        }
    }
    else
    {
        masterOfflineHandled = false;
    }

    updateRelays();

    sendHeartbeat();    // sends latched motion + raw brightness to master
}
