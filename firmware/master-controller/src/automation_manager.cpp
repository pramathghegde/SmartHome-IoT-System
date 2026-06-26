#include "automation_manager.h"
#include "state_manager.h"
#include "device_cache.h"
#include "schedule_manager.h"
#include "espnow_manager.h"
#include "dashboard_manager.h"
#include "device_ids.h"
#include "node_ids.h"
#include "modes.h"
#include "config.h"

#include <Arduino.h>

// ---------------------------------------------------------------
// Per-device pending command tracker
// One send attempt per device per runAutomation() cycle.
// If send fails, retries on next cycle (non-blocking).
// ---------------------------------------------------------------

struct PendingCmd
{
    bool    active;
    bool    targetState;
    uint8_t retries;
    bool    awaitingAck;
    uint32_t createdAt;
    uint32_t lastSendAt;
};

// Index 0-4 maps to Device IDs 1-5
static PendingCmd pending[5] = {};

#define MAX_RETRIES 10
#define COMMAND_RETRY_INTERVAL 150

// ---------------------------------------------------------------
// Motion timeout - master owns this completely
// Receives raw RCWL state from bedroom1, applies duration logic here
// ---------------------------------------------------------------

static unsigned long lastMotionTime = 0;

void recordMotionEvent()
{
    lastMotionTime = millis();

    if (!bedroom1.motionDetected)
    {
        bedroom1.motionDetected = true;
        Serial.println("[MOTION] Active");
    }
}

static void updateMotionTimeout()
{
    if (
        bedroom1.motionDetected &&
        (millis() - lastMotionTime) >= MOTION_TIMEOUT
    )
    {
        bedroom1.motionDetected = false;
        Serial.println("[MOTION] Timeout");
    }
}

// ---------------------------------------------------------------
// LDR hysteresis - master owns this completely
// ---------------------------------------------------------------

static bool currentDarkState = false;

static bool isItDark()
{
    if (currentDarkState)
    {
        if (bedroom1.brightness > DARK_THRESHOLD_HIGH)
        {
            currentDarkState = false;
            Serial.println("[LDR] DAY");
        }
    }
    else
    {
        if (bedroom1.brightness < DARK_THRESHOLD_LOW)
        {
            currentDarkState = true;
            Serial.println("[LDR] NIGHT");
        }
    }

    return currentDarkState;
}

// ---------------------------------------------------------------
// getAutoState() - per device type logic
// Lighting (tube, bulb): motion AND dark
// Non-lighting (fan, socket, AC): motion only
// ---------------------------------------------------------------

static bool getAutoState(uint8_t deviceID)
{
    bool dark = isItDark();

    switch(deviceID)
    {
        case TUBELIGHT_DEVICE:
        case BULB_DEVICE:
            return (bedroom1.motionDetected && dark);

        case FAN_DEVICE:
        case SOCKET_DEVICE:
        case AC_DEVICE:
            return bedroom1.motionDetected;

        default:
            return false;
    }
}

// ---------------------------------------------------------------
// processDevice()
// Non-blocking reliable delivery:
// keep retrying until Bedroom1 ACKs after executing the relay command.
// currentState updates only on execution ACK, never on local send queueing.
// ---------------------------------------------------------------

static void processDevice(
    DeviceConfig &device,
    uint8_t deviceID
)
{
    uint8_t idx = deviceID - 1;
    uint32_t now = millis();

    bool desiredState = false;

    switch(device.mode)
    {
        case MODE_OFF:       desiredState = false;                    break;
        case MODE_ON:        desiredState = true;                     break;
        case MODE_AUTO:      desiredState = getAutoState(deviceID);   break;
        case MODE_SCHEDULED: desiredState = isScheduleActive(device); break;
    }

    // No pending command: check if one is needed
    if (!pending[idx].active)
    {
        if (desiredState == device.currentState)
        {
            return; // Already correct
        }

        pending[idx].active      = true;
        pending[idx].targetState = desiredState;
        pending[idx].retries     = 0;
        pending[idx].awaitingAck = false;
        pending[idx].createdAt   = now;
        pending[idx].lastSendAt  = 0;

        Serial.print("[COMMAND CREATED] Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.println(desiredState ? "ON" : "OFF");
        Serial.print("[QUEUE PUSH] Master pending Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.println(desiredState ? "ON" : "OFF");
    }
    else if (desiredState != pending[idx].targetState)
    {
        pending[idx].targetState = desiredState;
        pending[idx].retries     = 0;
        pending[idx].awaitingAck = false;
        pending[idx].createdAt   = now;
        pending[idx].lastSendAt  = 0;

        Serial.print("[COMMAND UPDATED] Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.println(desiredState ? "ON" : "OFF");
        Serial.print("[QUEUE PUSH] Master pending Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.println(desiredState ? "ON" : "OFF");
    }

    if (
        pending[idx].awaitingAck &&
        (now - pending[idx].lastSendAt) < COMMAND_RETRY_INTERVAL
    )
    {
        return;
    }

    // Give up after MAX_RETRIES
    if (pending[idx].retries >= MAX_RETRIES)
    {
        Serial.print("[AUTOMATION] Device=");
        Serial.print(deviceID);
        Serial.println(" gave up after max retries");

        pending[idx].active  = false;
        pending[idx].retries = 0;
        return;
    }

    pending[idx].retries++;
    pending[idx].awaitingAck = true;
    pending[idx].lastSendAt  = now;

    Serial.print("[QUEUE POP] Master pending Device=");
    Serial.print(deviceID);
    Serial.print(" State=");
    Serial.print(pending[idx].targetState ? "ON" : "OFF");
    Serial.print(" Retry=");
    Serial.println(pending[idx].retries);

    bool queued = sendDeviceCommand(
        BEDROOM1_NODE,
        deviceID,
        pending[idx].targetState
    );

    if (!queued)
    {
        pending[idx].awaitingAck = false;
        return;
    }
}

// ---------------------------------------------------------------
// forceDeviceSync() - called when node transitions OFFLINE->ONLINE
// ---------------------------------------------------------------

static void forceDeviceSync(
    DeviceConfig &device,
    uint8_t deviceID
)
{
    uint8_t idx = deviceID - 1;

    bool desiredState = false;

    switch(device.mode)
    {
        case MODE_OFF:       desiredState = false;                    break;
        case MODE_ON:        desiredState = true;                     break;
        case MODE_AUTO:      desiredState = getAutoState(deviceID);   break;
        case MODE_SCHEDULED: desiredState = isScheduleActive(device); break;
    }

    pending[idx].active      = true;
    pending[idx].targetState = desiredState;
    pending[idx].retries     = 0;
    pending[idx].awaitingAck = false;
    pending[idx].createdAt   = millis();
    pending[idx].lastSendAt  = 0;
}

void confirmDeviceCommand(uint8_t deviceID, bool state)
{
    if (deviceID < FAN_DEVICE || deviceID > AC_DEVICE)
    {
        return;
    }

    uint8_t idx = deviceID - 1;

    if (!pending[idx].active || pending[idx].targetState != state)
    {
        Serial.print("[ACK STALE] Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.println(state ? "ON" : "OFF");
        return;
    }

    DeviceConfig* device = nullptr;

    switch(deviceID)
    {
        case FAN_DEVICE:       device = &bedroom1Fan;    break;
        case TUBELIGHT_DEVICE: device = &bedroom1Tube;   break;
        case BULB_DEVICE:      device = &bedroom1Bulb;   break;
        case SOCKET_DEVICE:    device = &bedroom1Socket; break;
        case AC_DEVICE:        device = &bedroom1AC;     break;
        default: return;
    }

    device->currentState = state;
    uint32_t age = millis() - pending[idx].createdAt;
    pending[idx] = {};

    notifyDeviceStateChange(deviceID, state);

    Serial.print("[COMMAND ACKED] Device=");
    Serial.print(deviceID);
    Serial.print(" State=");
    Serial.print(state ? "ON" : "OFF");
    Serial.print(" AgeMs=");
    Serial.println(age);
}

// ---------------------------------------------------------------
// runAutomation()
// ---------------------------------------------------------------

void runAutomation()
{
    updateMotionTimeout();

    if (!bedroom1.online)
    {
        return;
    }

    if (bedroom1.syncPending)
    {
        Serial.println("[SYNC] OFFLINE->ONLINE: syncing all devices");

        forceDeviceSync(bedroom1Fan,    FAN_DEVICE);
        forceDeviceSync(bedroom1Tube,   TUBELIGHT_DEVICE);
        forceDeviceSync(bedroom1Bulb,   BULB_DEVICE);
        forceDeviceSync(bedroom1Socket, SOCKET_DEVICE);
        forceDeviceSync(bedroom1AC,     AC_DEVICE);

        bedroom1.syncPending = false;
    }

    processDevice(bedroom1Fan,    FAN_DEVICE);
    processDevice(bedroom1Tube,   TUBELIGHT_DEVICE);
    processDevice(bedroom1Bulb,   BULB_DEVICE);
    processDevice(bedroom1Socket, SOCKET_DEVICE);
    processDevice(bedroom1AC,     AC_DEVICE);
}
