#include <Arduino.h>

#include "relay_manager.h"

#include "pins.h"

#include "device_manager.h"

#include "device_ids.h"

void initRelays()
{
    pinMode(FAN_RELAY_PIN, OUTPUT);
    pinMode(TUBE_RELAY_PIN, OUTPUT);
    pinMode(BULB_RELAY_PIN, OUTPUT);
    pinMode(SOCKET_RELAY_PIN, OUTPUT);
    pinMode(AC_RELAY_PIN, OUTPUT);

    digitalWrite(FAN_RELAY_PIN, HIGH);
    digitalWrite(TUBE_RELAY_PIN, HIGH);
    digitalWrite(BULB_RELAY_PIN, HIGH);
    digitalWrite(SOCKET_RELAY_PIN, HIGH);
    digitalWrite(AC_RELAY_PIN, HIGH);
}

void updateRelays()
{
    digitalWrite(
        FAN_RELAY_PIN,
        getDeviceState(FAN_DEVICE) ? LOW : HIGH
    );

    digitalWrite(
        TUBE_RELAY_PIN,
        getDeviceState(TUBELIGHT_DEVICE) ? LOW : HIGH
    );

    digitalWrite(
        BULB_RELAY_PIN,
        getDeviceState(BULB_DEVICE) ? LOW : HIGH
    );

    digitalWrite(
        SOCKET_RELAY_PIN,
        getDeviceState(SOCKET_DEVICE) ? LOW : HIGH
    );

    digitalWrite(
        AC_RELAY_PIN,
        getDeviceState(AC_DEVICE) ? LOW : HIGH
    );
}