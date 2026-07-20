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
#include "node_manager.h"

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

// Index 0: BEDROOM1, Index 1: LIVINGROOM, Index 2: DININGHALL
static PendingCmd pending[3][8] = {};

static int getRoomIndex(uint8_t nodeID)
{
    if (nodeID == BEDROOM1_NODE) return 0;
    if (nodeID == LIVINGROOM_NODE) return 1;
    if (nodeID == DININGHALL_NODE) return 2;
    return -1;
}

#define MAX_RETRIES 10
#define COMMAND_RETRY_INTERVAL 150

// ---------------------------------------------------------------
// Motion timeout - master owns this completely
// Receives raw Motion Sensor state from rooms, applies duration logic here
// ---------------------------------------------------------------

static unsigned long lastMotionTimeB1 = 0;
static unsigned long lastMotionTimeLR = 0;
static unsigned long lastMotionTimeDH = 0;

void recordMotionEvent(uint8_t nodeID)
{
    if (nodeID == BEDROOM1_NODE)
    {
        lastMotionTimeB1 = millis();
        if (!bedroom1.motionDetected)
        {
            bedroom1.motionDetected = true;
            Serial.println("[MOTION] B1 Active");
        }
    }
    else if (nodeID == LIVINGROOM_NODE)
    {
        lastMotionTimeLR = millis();
        if (!livingroom.motionDetected)
        {
            livingroom.motionDetected = true;
            Serial.println("[MOTION] LR Active");
        }
    }
    else if (nodeID == DININGHALL_NODE)
    {
        lastMotionTimeDH = millis();
        if (!dininghall.motionDetected)
        {
            dininghall.motionDetected = true;
            Serial.println("[MOTION] DH Active");
        }
    }
}

static void updateMotionTimeout()
{
    if (
        bedroom1.motionDetected &&
        (millis() - lastMotionTimeB1) >= bedroom1Config.motionTimeoutMs
    )
    {
        bedroom1.motionDetected = false;
        Serial.println("[MOTION] B1 Timeout");
    }

    if (
        livingroom.motionDetected &&
        (millis() - lastMotionTimeLR) >= livingroomConfig.motionTimeoutMs
    )
    {
        livingroom.motionDetected = false;
        Serial.println("[MOTION] LR Timeout");
    }

    if (
        dininghall.motionDetected &&
        (millis() - lastMotionTimeDH) >= dininghallConfig.motionTimeoutMs
    )
    {
        dininghall.motionDetected = false;
        Serial.println("[MOTION] DH Timeout");
    }
}

static bool updateDarkState(
    bool &darkState,
    uint16_t brightness,
    uint16_t enterThreshold,
    uint16_t exitThreshold,
    const char* roomName
)
{
    if (!darkState)
    {
        if (brightness <= enterThreshold)
        {
            darkState = true;
            Serial.println("[MASTER][LDR]");
            Serial.printf("Room : %s\n", roomName);
            Serial.printf("Brightness : %u\n", brightness);
            Serial.printf("Enter Threshold : %u\n", enterThreshold);
            Serial.printf("Exit Threshold : %u\n", exitThreshold);
            Serial.println("State : BRIGHT -> DARK");
            Serial.println("--------------------------------");
        }
    }
    else
    {
        if (brightness >= exitThreshold)
        {
            darkState = false;
            Serial.println("[MASTER][LDR]");
            Serial.printf("Room : %s\n", roomName);
            Serial.printf("Brightness : %u\n", brightness);
            Serial.printf("Enter Threshold : %u\n", enterThreshold);
            Serial.printf("Exit Threshold : %u\n", exitThreshold);
            Serial.println("State : DARK -> BRIGHT");
            Serial.println("--------------------------------");
        }
    }
    return darkState;
}

static bool isItDark(uint8_t nodeID)
{
    if (nodeID == BEDROOM1_NODE)
    {
        return bedroom1.darkState;
    }
    else if (nodeID == LIVINGROOM_NODE)
    {
        return livingroom.darkState;
    }
    else if (nodeID == DININGHALL_NODE)
    {
        return bedroom1.darkState;
    }
    return false;
}


// ---------------------------------------------------------------
// getAutoState() - per device type logic
// Lighting (tube, bulb): motion AND dark (if LDR enabled)
// Non-lighting (fan, socket, AC): motion only
// ---------------------------------------------------------------

static bool getAutoState(uint8_t nodeID, uint8_t deviceID)
{
    bool dark = isItDark(nodeID);

    if (nodeID == BEDROOM1_NODE)
    {
        switch(deviceID)
        {
            case TUBELIGHT_DEVICE:
            case BULB_DEVICE:
                if (!bedroom1LdrEnabled)
                {
                    return bedroom1.motionDetected;
                }
                return (bedroom1.motionDetected && dark);

            case FAN_DEVICE:
            case SOCKET_DEVICE:
            case AC_DEVICE:
                return bedroom1.motionDetected;

            default:
                return false;
        }
    }
    else if (nodeID == LIVINGROOM_NODE)
    {
        switch(deviceID)
        {
            case 1: // LED Tube Light 1
            case 2: // LED Tube Light 2
            case 5: // General Purpose Socket
                if (!livingroomLdrEnabled)
                {
                    return livingroom.motionDetected;
                }
                return (livingroom.motionDetected && dark);

            case 3: // Ceiling Fan
            case 7: // Extra Socket 1
            case 8: // Extra Socket 2
                return livingroom.motionDetected;

            case 4: // E-Bike Charging
                return true;

            case 6: // Outside LED Bulb
                if (!livingroomLdrEnabled)
                {
                    return true;
                }
                return dark;

            default:
                return false;
        }
    }
    else if (nodeID == DININGHALL_NODE)
    {
        switch(deviceID)
        {
            case 1: // LED Bulb
            case 2: // LED Tube Light
            case 4: // Socket
                if (!dininghallLdrEnabled)
                {
                    return dininghall.motionDetected;
                }
                return (dininghall.motionDetected && dark);

            case 3: // Fan (never depends on darkness)
                return dininghall.motionDetected;

            case 5: // Extra Socket 1 - manual only, never AUTO
            case 6: // Extra Socket 2 - manual only, never AUTO
                return false;

            default:
                return false;
        }
    }
    return false;
}

// ---------------------------------------------------------------
// processDevice()
// Non-blocking reliable delivery:
// keep retrying until Node ACKs after executing the relay command.
// currentState updates only on execution ACK, never on local send queueing.
// ---------------------------------------------------------------

static bool isNodeOnline(uint8_t nodeID)
{
    if (nodeID == BEDROOM1_NODE)   return bedroom1.online;
    if (nodeID == LIVINGROOM_NODE) return livingroom.online;
    if (nodeID == DININGHALL_NODE) return dininghall.online;
    return false;
}

static void processDevice(
    uint8_t nodeID,
    DeviceConfig &device,
    uint8_t deviceID
)
{
    int roomIdx = getRoomIndex(nodeID);
    if (roomIdx < 0) return;

    uint8_t idx = deviceID - 1;
    uint32_t now = millis();

    bool desiredState = false;

    switch(device.mode)
    {
        case MODE_OFF:       desiredState = false;                                                    break;
        case MODE_ON:        desiredState = true;                                                     break;
        case MODE_AUTO:
        {
            // STEP 8: expose the schedule gate — THIS IS THE MOST COMMON HIDDEN FAILURE
            bool schedActive = isAutoScheduleActive(device);
            bool autoState   = schedActive ? getAutoState(nodeID, deviceID) : false;
            desiredState = autoState;
            if (nodeID == DININGHALL_NODE)
            {
                static unsigned long lastDhAutoLog = 0;
                if (millis() - lastDhAutoLog >= 5000)
                {
                    lastDhAutoLog = millis();
                }
            }
            break;
        }
        case MODE_SCHEDULED: desiredState = isSchedScheduleActive(device);                            break;
    }

    device.desiredState = desiredState; // Update desiredState runtime state

    // If the node is offline, suppress all queueing, retries, and commands.
    if (!isNodeOnline(nodeID))
    {
        // Cancel any active pending commands for this device if node went offline.
        if (pending[roomIdx][idx].active)
        {
            pending[roomIdx][idx].active = false;
            pending[roomIdx][idx].retries = 0;
        }
        return;
    }

    // No pending command: check if one is needed
    if (!pending[roomIdx][idx].active)
    {
        if (desiredState == device.currentState)
        {
            return; // Already correct
        }

        pending[roomIdx][idx].active      = true;
        pending[roomIdx][idx].targetState = desiredState;
        pending[roomIdx][idx].retries     = 0;
        pending[roomIdx][idx].awaitingAck = false;
        pending[roomIdx][idx].createdAt   = now;
        pending[roomIdx][idx].lastSendAt  = 0;

        Serial.print("[COMMAND CREATED] Node=");
        Serial.print(getNodeName(nodeID));
        Serial.print(" Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.println(desiredState ? "ON" : "OFF");
        Serial.print("[QUEUE PUSH] Master pending Node=");
        Serial.print(getNodeName(nodeID));
        Serial.print(" Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.println(desiredState ? "ON" : "OFF");
    }
    else if (desiredState != pending[roomIdx][idx].targetState)
    {
        pending[roomIdx][idx].targetState = desiredState;
        pending[roomIdx][idx].retries     = 0;
        pending[roomIdx][idx].awaitingAck = false;
        pending[roomIdx][idx].createdAt   = now;
        pending[roomIdx][idx].lastSendAt  = 0;

        Serial.print("[COMMAND UPDATED] Node=");
        Serial.print(getNodeName(nodeID));
        Serial.print(" Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.println(desiredState ? "ON" : "OFF");
        Serial.print("[QUEUE PUSH] Master pending Node=");
        Serial.print(getNodeName(nodeID));
        Serial.print(" Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.println(desiredState ? "ON" : "OFF");
    }

    if (
        pending[roomIdx][idx].awaitingAck &&
        (now - pending[roomIdx][idx].lastSendAt) < COMMAND_RETRY_INTERVAL
    )
    {
        return;
    }

    // Give up after MAX_RETRIES
    if (pending[roomIdx][idx].retries >= MAX_RETRIES)
    {
        Serial.print("[AUTOMATION] Node=");
        Serial.print(getNodeName(nodeID));
        Serial.print(" Device=");
        Serial.print(deviceID);
        Serial.println(" gave up after max retries");

        pending[roomIdx][idx].active  = false;
        pending[roomIdx][idx].retries = 0;
        return;
    }

    pending[roomIdx][idx].retries++;
    pending[roomIdx][idx].awaitingAck = true;
    pending[roomIdx][idx].lastSendAt  = now;

    Serial.print("[QUEUE POP] Master pending Node=");
    Serial.print(getNodeName(nodeID));
    Serial.print(" Device=");
    Serial.print(deviceID);
    Serial.print(" State=");
    Serial.print(pending[roomIdx][idx].targetState ? "ON" : "OFF");
    Serial.print(" Retry=");
    Serial.println(pending[roomIdx][idx].retries);

    bool queued = sendDeviceCommand(
        nodeID,
        deviceID,
        pending[roomIdx][idx].targetState
    );

    if (!queued)
    {
        pending[roomIdx][idx].awaitingAck = false;
        return;
    }
}

void confirmDeviceCommand(uint8_t nodeID, uint8_t deviceID, bool state)
{
    int roomIdx = getRoomIndex(nodeID);
    if (roomIdx < 0) return;

    if (nodeID == BEDROOM1_NODE)
    {
        if (deviceID < 1 || deviceID > 5) return;
    }
    else if (nodeID == LIVINGROOM_NODE)
    {
        if (deviceID < 1 || deviceID > 8) return;
    }
    else if (nodeID == DININGHALL_NODE)
    {
        if (deviceID < 1 || deviceID > 6) return;
    }

    uint8_t idx = deviceID - 1;

    if (!pending[roomIdx][idx].active || pending[roomIdx][idx].targetState != state)
    {
        Serial.print("[ACK STALE] Node=");
        Serial.print(getNodeName(nodeID));
        Serial.print(" Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.println(state ? "ON" : "OFF");
        return;
    }

    DeviceConfig* device = nullptr;

    if (nodeID == BEDROOM1_NODE)
    {
        switch(deviceID)
        {
            case FAN_DEVICE:       device = &bedroom1Fan;    break;
            case TUBELIGHT_DEVICE: device = &bedroom1Tube;   break;
            case BULB_DEVICE:      device = &bedroom1Bulb;   break;
            case SOCKET_DEVICE:    device = &bedroom1Socket; break;
            case AC_DEVICE:        device = &bedroom1AC;     break;
            default: return;
        }
    }
    else if (nodeID == LIVINGROOM_NODE)
    {
        switch(deviceID)
        {
            case 1: device = &livingroomTube1;        break;
            case 2: device = &livingroomTube2;        break;
            case 3: device = &livingroomFan;          break;
            case 4: device = &livingroomEBike;        break;
            case 5: device = &livingroomSocket;       break;
            case 6: device = &livingroomOutsideBulb;  break;
            case 7: device = &livingroomExtra1;       break;
            case 8: device = &livingroomExtra2;       break;
            default: return;
        }
    }
    else if (nodeID == DININGHALL_NODE)
    {
        switch(deviceID)
        {
            case 1: device = &dininghallBulb;   break;
            case 2: device = &dininghallTube;   break;
            case 3: device = &dininghallFan;    break;
            case 4: device = &dininghallSocket; break;
            case 5: device = &dininghallExtra1; break;
            case 6: device = &dininghallExtra2; break;
            default: return;
        }
    }

    if (device != nullptr)
    {
        device->currentState = state;
        uint32_t age = millis() - pending[roomIdx][idx].createdAt;
        pending[roomIdx][idx] = {};

        notifyDeviceStateChange(nodeID, deviceID, state);

        Serial.print("[COMMAND ACKED] Node=");
        Serial.print(getNodeName(nodeID));
        Serial.print(" Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.print(state ? "ON" : "OFF");
        Serial.print(" AgeMs=");
        Serial.println(age);
    }
}

// ---------------------------------------------------------------
// runAutomation()
// ---------------------------------------------------------------

void runAutomation()
{
    // Update persistent LDR darkStates using hysteresis helper
    updateDarkState(bedroom1.darkState, bedroom1.brightness, BEDROOM1_DARK_ENTER_THRESHOLD, BEDROOM1_DARK_EXIT_THRESHOLD, "Bedroom1");
    updateDarkState(livingroom.darkState, bedroom1.brightness, LIVINGROOM_DARK_ENTER_THRESHOLD, LIVINGROOM_DARK_EXIT_THRESHOLD, "LivingRoom");

    updateMotionTimeout();

    // Bedroom1 transition online check
    if (bedroom1.syncPending)
    {
        int mismatchCount = 0;
        if (bedroom1Fan.desiredState    != bedroom1Fan.currentState)    mismatchCount++;
        if (bedroom1Tube.desiredState   != bedroom1Tube.currentState)   mismatchCount++;
        if (bedroom1Bulb.desiredState   != bedroom1Bulb.currentState)   mismatchCount++;
        if (bedroom1Socket.desiredState != bedroom1Socket.currentState) mismatchCount++;
        if (bedroom1AC.desiredState     != bedroom1AC.currentState)     mismatchCount++;

        Serial.printf("[SYNC] Bedroom1 ONLINE - Synchronizing %d pending device states.\n", mismatchCount);
        bedroom1.syncPending = false;
    }

    // LivingRoom transition online check
    if (livingroom.syncPending)
    {
        int mismatchCount = 0;
        if (livingroomTube1.desiredState       != livingroomTube1.currentState)       mismatchCount++;
        if (livingroomTube2.desiredState       != livingroomTube2.currentState)       mismatchCount++;
        if (livingroomFan.desiredState         != livingroomFan.currentState)         mismatchCount++;
        if (livingroomEBike.desiredState       != livingroomEBike.currentState)       mismatchCount++;
        if (livingroomSocket.desiredState      != livingroomSocket.currentState)      mismatchCount++;
        if (livingroomOutsideBulb.desiredState != livingroomOutsideBulb.currentState) mismatchCount++;
        if (livingroomExtra1.desiredState      != livingroomExtra1.currentState)      mismatchCount++;
        if (livingroomExtra2.desiredState      != livingroomExtra2.currentState)      mismatchCount++;

        Serial.printf("[SYNC] LivingRoom ONLINE - Synchronizing %d pending device states.\n", mismatchCount);
        livingroom.syncPending = false;
    }

    // DiningHall transition online check
    if (dininghall.syncPending)
    {
        int mismatchCount = 0;
        if (dininghallBulb.desiredState   != dininghallBulb.currentState)   mismatchCount++;
        if (dininghallTube.desiredState   != dininghallTube.currentState)   mismatchCount++;
        if (dininghallFan.desiredState    != dininghallFan.currentState)    mismatchCount++;
        if (dininghallSocket.desiredState != dininghallSocket.currentState) mismatchCount++;
        if (dininghallExtra1.desiredState != dininghallExtra1.currentState) mismatchCount++;
        if (dininghallExtra2.desiredState != dininghallExtra2.currentState) mismatchCount++;

        Serial.printf("[SYNC] DiningHall ONLINE - Synchronizing %d pending device states.\n", mismatchCount);
        dininghall.syncPending = false;
    }

    // Unconditional device processing (updates desiredState internally, suppresses queueing/retries/sending if offline)
    processDevice(BEDROOM1_NODE, bedroom1Fan,    FAN_DEVICE);
    processDevice(BEDROOM1_NODE, bedroom1Tube,   TUBELIGHT_DEVICE);
    processDevice(BEDROOM1_NODE, bedroom1Bulb,   BULB_DEVICE);
    processDevice(BEDROOM1_NODE, bedroom1Socket, SOCKET_DEVICE);
    processDevice(BEDROOM1_NODE, bedroom1AC,     AC_DEVICE);

    processDevice(LIVINGROOM_NODE, livingroomTube1,       1);
    processDevice(LIVINGROOM_NODE, livingroomTube2,       2);
    processDevice(LIVINGROOM_NODE, livingroomFan,         3);
    processDevice(LIVINGROOM_NODE, livingroomEBike,       4);
    processDevice(LIVINGROOM_NODE, livingroomSocket,      5);
    processDevice(LIVINGROOM_NODE, livingroomOutsideBulb, 6);
    processDevice(LIVINGROOM_NODE, livingroomExtra1,      7);
    processDevice(LIVINGROOM_NODE, livingroomExtra2,      8);

    processDevice(DININGHALL_NODE, dininghallBulb,   1);
    processDevice(DININGHALL_NODE, dininghallTube,   2);
    processDevice(DININGHALL_NODE, dininghallFan,    3);
    processDevice(DININGHALL_NODE, dininghallSocket, 4);
    processDevice(DININGHALL_NODE, dininghallExtra1, 5);
    processDevice(DININGHALL_NODE, dininghallExtra2, 6);
}
