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

    initDevices();

    initRelays();

    initMotionSensor();

    initEspNow();

    initLDR();

    initOTA();
}

void loop()
{
    handleOTA();

    updateMotionSensor();

    updateLDR();

    updateEnvironment();

    processIncomingPackets();

    if(!isMasterOnline())
    {
        setDeviceState(
            FAN_DEVICE,
            false
        );

        setDeviceState(
            TUBELIGHT_DEVICE,
            false
        );

        setDeviceState(
            BULB_DEVICE,
            false
        );

        setDeviceState(
            SOCKET_DEVICE,
            false
        );

        setDeviceState(
            AC_DEVICE,
            false
        );
    }

    updateRelays();

    sendHeartbeat();
}