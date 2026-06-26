#include <Arduino.h>

#include "relay_manager.h"

#include "pins.h"

#include "device_manager.h"

#include "device_ids.h"

static void writeRelay(
    uint8_t gpio,
    bool newLevel,
    const char* reason,
    const char* caller
)
{
    bool oldLevel = digitalRead(gpio);

    Serial.println("[RELAY_WRITE]");
    Serial.print("GPIO=");
    Serial.println(gpio);
    Serial.print("OLD=");
    Serial.println(oldLevel ? "HIGH" : "LOW");
    Serial.print("NEW=");
    Serial.println(newLevel ? "HIGH" : "LOW");
    Serial.print("Reason=");
    Serial.println(reason);
    Serial.print("Caller=");
    Serial.println(caller);

    digitalWrite(gpio, newLevel ? HIGH : LOW);

    Serial.print("[GPIO UPDATED] GPIO=");
    Serial.print(gpio);
    Serial.print(" Level=");
    Serial.println(newLevel ? "HIGH" : "LOW");
}

static bool relayLevelForState(bool state)
{
    return state ? LOW : HIGH;
}

static bool relayPinForDevice(uint8_t deviceID, uint8_t& pin)
{
    switch (deviceID)
    {
        case FAN_DEVICE:
            pin = FAN_RELAY_PIN;
            return true;

        case TUBELIGHT_DEVICE:
            pin = TUBE_RELAY_PIN;
            return true;

        case BULB_DEVICE:
            pin = BULB_RELAY_PIN;
            return true;

        case SOCKET_DEVICE:
            pin = SOCKET_RELAY_PIN;
            return true;

        case AC_DEVICE:
            pin = AC_RELAY_PIN;
            return true;

        default:
            return false;
    }
}

void initRelays()
{
    // Write inactive level (HIGH) before setting pinMode to prevent startup glitches
    digitalWrite(FAN_RELAY_PIN, HIGH);
    digitalWrite(TUBE_RELAY_PIN, HIGH);
    digitalWrite(BULB_RELAY_PIN, HIGH);
    digitalWrite(SOCKET_RELAY_PIN, HIGH);
    digitalWrite(AC_RELAY_PIN, HIGH);

    pinMode(FAN_RELAY_PIN, OUTPUT);
    pinMode(TUBE_RELAY_PIN, OUTPUT);
    pinMode(BULB_RELAY_PIN, OUTPUT);
    pinMode(SOCKET_RELAY_PIN, OUTPUT);
    pinMode(AC_RELAY_PIN, OUTPUT);

    writeRelay(FAN_RELAY_PIN, HIGH, "BOOT", "initRelays");
    writeRelay(TUBE_RELAY_PIN, HIGH, "BOOT", "initRelays");
    writeRelay(BULB_RELAY_PIN, HIGH, "BOOT", "initRelays");
    writeRelay(SOCKET_RELAY_PIN, HIGH, "BOOT", "initRelays");
    writeRelay(AC_RELAY_PIN, HIGH, "BOOT", "initRelays");
}

void applyRelayCommand(uint8_t deviceID, bool state)
{
    uint8_t pin = 0;

    if (!relayPinForDevice(deviceID, pin))
    {
        Serial.print("[RELAY] Invalid deviceID=");
        Serial.println(deviceID);
        return;
    }

    setDeviceState(deviceID, state);
    writeRelay(pin, relayLevelForState(state), "CMD_PACKET", "applyRelayCommand");
}
