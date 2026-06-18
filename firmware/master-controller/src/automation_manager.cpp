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
// Only ONE send attempt per runAutomation() call per device
// Next call retries if previous failed
// ---------------------------------------------------------------

struct PendingCmd
{
    bool     active;
    bool     targetState;
    uint8_t  retries;
};

// Index 0-4 maps to Device IDs 1-5
static PendingCmd pending[5] = {};

#define MAX_RETRIES 10

// ---------------------------------------------------------------
// Motion timeout state - master owns this completely
// ---------------------------------------------------------------

static unsigned long lastMotionTime  = 0;
static bool          motionActive    = false;

static void updateMotionState()
{
    if (bedroom1.motionDetected)
    {
        lastMotionTime = millis();

        if (!motionActive)
        {
            motionActive = true;
            Serial.println("[MOTION] Active");
        }
    }
    else
    {
        if (
            motionActive &&
            (millis() - lastMotionTime) > MOTION_TIMEOUT
        )
        {
            motionActive = false;
            Serial.println("[MOTION] Timeout - cleared");
        }
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
// ---------------------------------------------------------------

static bool getAutoState(uint8_t deviceID)
{
    bool dark = isItDark();

    switch(deviceID)
    {
        // Lighting: needs motion AND darkness
        case TUBELIGHT_DEVICE:
        case BULB_DEVICE:
            return (motionActive && dark);

        // Non-lighting: motion only
        case FAN_DEVICE:
        case SOCKET_DEVICE:
        case AC_DEVICE:
            return motionActive;

        default:
            return false;
    }
}

// ---------------------------------------------------------------
// processDevice()
// One send attempt per call. Sets pending if failed.
// ---------------------------------------------------------------

static void processDevice(
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

    // If no pending command, check if we need a new one
    if (!pending[idx].active)
    {
        if (desiredState == device.currentState)
        {
            return;  // Already correct, nothing to do
        }

        // New command needed
        pending[idx].active      = true;
        pending[idx].targetState = desiredState;
        pending[idx].retries     = 0;
    }

    // One send attempt this cycle
    if (pending[idx].retries >= MAX_RETRIES)
    {
        Serial.print("[AUTOMATION] Device=");
        Serial.print(deviceID);
        Serial.println(" gave up after max retries");

        pending[idx].active  = false;
        pending[idx].retries = 0;

        // Do NOT update currentState - will retry on next state change
        return;
    }

    pending[idx].retries++;

    sendDeviceCommand(
        BEDROOM1_NODE,
        deviceID,
        pending[idx].targetState
    );

    if (getLastSendSuccess())
    {
        device.currentState    = pending[idx].targetState;
        pending[idx].active    = false;
        pending[idx].retries   = 0;

        notifyDeviceStateChange(deviceID, device.currentState);

        Serial.print("[AUTOMATION] Device=");
        Serial.print(deviceID);
        Serial.print(" -> ");
        Serial.print(device.currentState ? "ON" : "OFF");
        Serial.print(" [try ");
        Serial.print(pending[idx].retries);
        Serial.println("]");
    }
    // If failed, pending stays active, retried next cycle
}

// ---------------------------------------------------------------
// runAutomation()
// ---------------------------------------------------------------

void runAutomation()
{
    if (!bedroom1.online)
    {
        return;
    }

    updateMotionState();

    processDevice(bedroom1Fan,    FAN_DEVICE);
    processDevice(bedroom1Tube,   TUBELIGHT_DEVICE);
    processDevice(bedroom1Bulb,   BULB_DEVICE);
    processDevice(bedroom1Socket, SOCKET_DEVICE);
    processDevice(bedroom1AC,     AC_DEVICE);
}