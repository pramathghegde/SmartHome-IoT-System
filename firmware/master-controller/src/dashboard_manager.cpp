// ==============================================================
// dashboard_manager.cpp
//
// TERMINAL ARCHITECTURE:
//   V200      : Terminal widget - full status dump every 60s
//               = 1 message/minute = 43,200/month
//   V150-V154 : LED per appliance - only on state change
//   V0-V4     : Mode controls - incoming only
//   V100-V104 : Schedules - incoming only
// ==============================================================

#include "secrets.h"
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <time.h>

#include "dashboard_manager.h"
#include "device_cache.h"
#include "state_manager.h"
#include "time_manager.h"
#include "modes.h"
#include "config.h"
#include "device_ids.h"
#include "node_manager.h"
#include "node_ids.h"

#include <Arduino.h>

char auth[] = BLYNK_AUTH_TOKEN;

static uint32_t terminalMessagesSent = 0;
static uint32_t ledMessagesSent      = 0;
static uint32_t otherMessagesSent    = 0;
static uint32_t totalMessagesSent    = 0;

// ---------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------

static const char* modeStr(uint8_t mode)
{
    switch(mode)
    {
        case 0:  return "OFF  ";
        case 1:  return "ON   ";
        case 2:  return "AUTO ";
        case 3:  return "SCHED";
        default: return "?    ";
    }
}

static const char* stateStr(bool state)
{
    return state ? " ON" : "OFF";
}

static void countBlynkMessage(bool terminalMessage, bool ledMessage)
{
    if (terminalMessage)
    {
        terminalMessagesSent++;
    }
    else if (ledMessage)
    {
        ledMessagesSent++;
    }
    else
    {
        otherMessagesSent++;
    }

    totalMessagesSent++;
}

static bool auditedVirtualWrite(uint8_t pin, const String& value)
{
    if (!Blynk.connected())
    {
        Serial.print("[BLYNK] Skipped V");
        Serial.print(pin);
        Serial.println(" write; not connected");
        return false;
    }

    Blynk.virtualWrite(pin, value);
    countBlynkMessage(pin == 200, pin >= 150 && pin <= 175);
    return true;
}

static bool auditedVirtualWrite(uint8_t pin, int value)
{
    if (!Blynk.connected())
    {
        Serial.print("[BLYNK] Skipped V");
        Serial.print(pin);
        Serial.println(" write; not connected");
        return false;
    }

    Blynk.virtualWrite(pin, value);
    countBlynkMessage(pin == 200, pin >= 150 && pin <= 175);
    return true;
}

static bool auditedVirtualWriteSchedule(uint8_t pin, uint8_t startHour, uint8_t startMinute, uint8_t stopHour, uint8_t stopMinute)
{
    if (!Blynk.connected())
    {
        Serial.print("[BLYNK] Skipped V");
        Serial.print(pin);
        Serial.println(" write; not connected");
        return false;
    }

    uint32_t startSec = startHour * 3600 + startMinute * 60;
    uint32_t stopSec = stopHour * 3600 + stopMinute * 60;
    Blynk.virtualWrite(pin, startSec, stopSec, "Asia/Kolkata");
    countBlynkMessage(false, false);
    return true;
}

static bool auditedVirtualWriteDuration(uint8_t pin, uint8_t hour, uint8_t minute, uint8_t second)
{
    if (!Blynk.connected())
    {
        Serial.print("[BLYNK] Skipped V");
        Serial.print(pin);
        Serial.println(" write; not connected");
        return false;
    }

    uint32_t startSec = (uint32_t)hour * 3600 + (uint32_t)minute * 60 + second;
    Blynk.virtualWrite(pin, startSec, -1, "Asia/Kolkata");
    countBlynkMessage(false, false);
    return true;
}

static void appendLine(String& buffer, const String& line = "")
{
    buffer += line;
    buffer += '\n';
}

static void printBlynkAudit()
{
    Serial.println();
    Serial.println("[BLYNK AUDIT]");
    Serial.print("Terminal Messages: ");
    Serial.println(terminalMessagesSent);
    Serial.print("LED Messages: ");
    Serial.println(ledMessagesSent);
    Serial.print("Other Messages: ");
    Serial.println(otherMessagesSent);
    Serial.print("Total Messages: ");
    Serial.println(totalMessagesSent);
    Serial.println();
}

// ---------------------------------------------------------------
// LED state tracking
// ---------------------------------------------------------------

static bool prevFanState    = false;
static bool prevTubeState   = false;
static bool prevBulbState   = false;
static bool prevSocketState = false;
static bool prevACState     = false;

static bool prevLrTube1State      = false;
static bool prevLrTube2State      = false;
static bool prevLrFanState        = false;
static bool prevLrEBikeState      = false;
static bool prevLrSocketState     = false;
static bool prevLrOutsideBulbState = false;
static bool prevLrExtra1State     = false;
static bool prevLrExtra2State     = false;

static bool prevDhBulbState   = false;
static bool prevDhTubeState   = false;
static bool prevDhFanState    = false;
static bool prevDhSocketState = false;
static bool prevDhExtra1State = false;
static bool prevDhExtra2State = false;

// ---------------------------------------------------------------
// Blynk synchronization cache
// ---------------------------------------------------------------
static uint8_t blynkFanModeCache        = 0xFF;
static uint8_t blynkTubeModeCache       = 0xFF;
static uint8_t blynkBulbModeCache       = 0xFF;
static uint8_t blynkSocketModeCache     = 0xFF;
static uint8_t blynkACModeCache         = 0xFF;

static uint8_t blynkLrTube1ModeCache      = 0xFF;
static uint8_t blynkLrTube2ModeCache      = 0xFF;
static uint8_t blynkLrFanModeCache        = 0xFF;
static uint8_t blynkLrEBikeModeCache      = 0xFF;
static uint8_t blynkLrSocketModeCache      = 0xFF;
static uint8_t blynkLrOutsideBulbModeCache = 0xFF;
static uint8_t blynkLrExtra1ModeCache     = 0xFF;
static uint8_t blynkLrExtra2ModeCache     = 0xFF;

static uint8_t blynkDhBulbModeCache       = 0xFF;
static uint8_t blynkDhTubeModeCache       = 0xFF;
static uint8_t blynkDhFanModeCache        = 0xFF;
static uint8_t blynkDhSocketModeCache     = 0xFF;
static uint8_t blynkDhExtra1ModeCache     = 0xFF;
static uint8_t blynkDhExtra2ModeCache     = 0xFF;

struct BlynkTimerCache
{
    uint8_t startHour;
    uint8_t startMinute;
    uint8_t stopHour;
    uint8_t stopMinute;
    bool hasStart;
    bool hasStop;
    uint8_t weekdays;
    char timezone[32];
};

static BlynkTimerCache blynkFanCache       = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkTubeCache      = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkBulbCache      = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkSocketCache    = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkACCache        = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};

static BlynkTimerCache blynkLrTube1Cache      = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkLrTube2Cache      = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkLrFanCache        = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkLrEBikeCache      = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkLrSocketCache     = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkLrOutsideBulbCache = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkLrExtra1Cache     = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkLrExtra2Cache     = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};

static BlynkTimerCache blynkDhBulbCache      = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkDhTubeCache      = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkDhFanCache       = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkDhSocketCache    = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkDhExtra1Cache    = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};
static BlynkTimerCache blynkDhExtra2Cache    = {0xFF, 0xFF, 0xFF, 0xFF, false, false, 0x00, ""};

static uint8_t blynkLdrEnableCache      = 0xFF;
static uint8_t blynkLrLdrEnableCache    = 0xFF;
static uint8_t blynkDhLdrEnableCache    = 0xFF;

struct BlynkMotionTimeoutCache
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    bool hasStart;
};

static BlynkMotionTimeoutCache blynkMotionTimeoutCache = {0xFF, 0xFF, 0xFF, false};
static BlynkMotionTimeoutCache blynkLrMotionTimeoutCache = {0xFF, 0xFF, 0xFF, false};
static BlynkMotionTimeoutCache blynkDhMotionTimeoutCache = {0xFF, 0xFF, 0xFF, false};


static bool blynkCacheInitialized = false;

static bool writeModeIfChanged(uint8_t pin, uint8_t currentVal, uint8_t &cachedVal)
{
    if (!blynkCacheInitialized || currentVal != cachedVal)
    {
        if (auditedVirtualWrite(pin, currentVal))
        {
            cachedVal = currentVal;
            return true;
        }
    }
    return false;
}

static bool timerSyncInProgress = false;

static bool writeTimerIfChanged(uint8_t pin, uint8_t startHour, uint8_t startMinute, uint8_t stopHour, uint8_t stopMinute, BlynkTimerCache &cache)
{
    bool changed = !blynkCacheInitialized ||
                   startHour != cache.startHour ||
                   startMinute != cache.startMinute ||
                   stopHour != cache.stopHour ||
                   stopMinute != cache.stopMinute ||
                   !cache.hasStart || !cache.hasStop ||
                   strcmp(cache.timezone, "Asia/Kolkata") != 0 ||
                   cache.weekdays != 0x7F;

    if (changed)
    {
        timerSyncInProgress = true;
        bool success = auditedVirtualWriteSchedule(pin, startHour, startMinute, stopHour, stopMinute);
        timerSyncInProgress = false;
        if (success)
        {
            cache.startHour = startHour;
            cache.startMinute = startMinute;
            cache.stopHour = stopHour;
            cache.stopMinute = stopMinute;
            cache.hasStart = true;
            cache.hasStop = true;
            cache.weekdays = 0x7F;
            snprintf(cache.timezone, sizeof(cache.timezone), "Asia/Kolkata");
            return true;
        }
    }
    return false;
}

static void syncTimerWidget(uint8_t pin, const DeviceConfig &device, BlynkTimerCache &cache)
{
    if (device.mode == 2) // MODE_AUTO
    {
        writeTimerIfChanged(pin, device.autoStartHour, device.autoStartMinute, device.autoStopHour, device.autoStopMinute, cache);
    }
    else // MODE_SCHEDULED, MODE_ON, MODE_OFF
    {
        writeTimerIfChanged(pin, device.schedStartHour, device.schedStartMinute, device.schedStopHour, device.schedStopMinute, cache);
    }
}

static bool writeLdrEnableIfChanged(uint8_t pin, bool currentVal, uint8_t &cachedVal)
{
    uint8_t currentByte = currentVal ? 1 : 0;
    if (!blynkCacheInitialized || currentByte != cachedVal)
    {
        if (auditedVirtualWrite(pin, currentByte))
        {
            cachedVal = currentByte;
            return true;
        }
    }
    return false;
}

static bool writeLedIfChanged(uint8_t pin, bool currentVal, bool &cachedVal)
{
    if (!blynkCacheInitialized || currentVal != cachedVal)
    {
        if (auditedVirtualWrite(pin, currentVal ? 255 : 0))
        {
            cachedVal = currentVal;
            return true;
        }
    }
    return false;
}

static bool writeMotionTimeoutIfChanged(uint8_t pin, uint8_t hour, uint8_t minute, uint8_t second, BlynkMotionTimeoutCache &cache)
{
    bool changed = !blynkCacheInitialized ||
                   hour != cache.hour ||
                   minute != cache.minute ||
                   second != cache.second ||
                   !cache.hasStart;

    if (changed)
    {
        timerSyncInProgress = true;
        bool success = auditedVirtualWriteDuration(pin, hour, minute, second);
        timerSyncInProgress = false;
        if (success)
        {
            cache.hour = hour;
            cache.minute = minute;
            cache.second = second;
            cache.hasStart = true;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------
// notifyDeviceStateChange()
// Called from automation_manager on every relay change
// Sends LED update for that device only = 1 message per change
// ---------------------------------------------------------------

void notifyDeviceStateChange(
    uint8_t nodeID,
    uint8_t deviceID,
    bool newState
)
{
    bool success = false;
    if (nodeID == BEDROOM1_NODE)
    {
        switch(deviceID)
        {
            case FAN_DEVICE:       success = writeLedIfChanged(150, newState, prevFanState);    break;
            case TUBELIGHT_DEVICE: success = writeLedIfChanged(151, newState, prevTubeState);   break;
            case BULB_DEVICE:      success = writeLedIfChanged(152, newState, prevBulbState);   break;
            case SOCKET_DEVICE:    success = writeLedIfChanged(153, newState, prevSocketState); break;
            case AC_DEVICE:        success = writeLedIfChanged(154, newState, prevACState);     break;
            default: return;
        }
    }
    else if (nodeID == LIVINGROOM_NODE)
    {
        switch(deviceID)
        {
            case 1: success = writeLedIfChanged(160, newState, prevLrTube1State);       break;
            case 2: success = writeLedIfChanged(161, newState, prevLrTube2State);       break;
            case 3: success = writeLedIfChanged(162, newState, prevLrFanState);         break;
            case 4: success = writeLedIfChanged(163, newState, prevLrEBikeState);       break;
            case 5: success = writeLedIfChanged(164, newState, prevLrSocketState);      break;
            case 6: success = writeLedIfChanged(165, newState, prevLrOutsideBulbState); break;
            case 7: success = writeLedIfChanged(166, newState, prevLrExtra1State);      break;
            case 8: success = writeLedIfChanged(167, newState, prevLrExtra2State);      break;
            default: return;
        }
    }
    else if (nodeID == DININGHALL_NODE)
    {
        switch(deviceID)
        {
            case 1: success = writeLedIfChanged(170, newState, prevDhBulbState);   break;
            case 2: success = writeLedIfChanged(171, newState, prevDhTubeState);   break;
            case 3: success = writeLedIfChanged(172, newState, prevDhFanState);    break;
            case 4: success = writeLedIfChanged(173, newState, prevDhSocketState); break;
            case 5: success = writeLedIfChanged(174, newState, prevDhExtra1State); break;
            case 6: success = writeLedIfChanged(175, newState, prevDhExtra2State); break;
            default: return;
        }
    }

    if (success)
    {
        int pin = -1;
        if (nodeID == BEDROOM1_NODE)
        {
            pin = 150 + (deviceID - 1);
        }
        else if (nodeID == LIVINGROOM_NODE)
        {
            pin = 160 + (deviceID - 1);
        }
        else if (nodeID == DININGHALL_NODE)
        {
            pin = 170 + (deviceID - 1);
        }
        Serial.print("[BLYNK] LED Node=");
        Serial.print(getNodeName(nodeID));
        Serial.print(" V");
        Serial.print(pin);
        Serial.print(" -> ");
        Serial.println(newState ? "ON" : "OFF");
    }
}


// ---------------------------------------------------------------
// Asynchronous Event Queue (V200)
// Lightweight 8-element FIFO queue drained 1 event every 200ms
// ---------------------------------------------------------------

static String eventQueue[8];
static int eventQueueHead = 0;
static int eventQueueTail = 0;
static int eventQueueMaxOccupancy = 0;
static int eventQueueDroppedCount = 0;

static int getEventQueueOccupancy()
{
    int count = eventQueueHead - eventQueueTail;
    if (count < 0) count += 8;
    return count;
}

static void queueEventLine(const String& line)
{
    int nextHead = (eventQueueHead + 1) % 8;
    if (nextHead != eventQueueTail)
    {
        eventQueue[eventQueueHead] = line;
        eventQueueHead = nextHead;

        int currentOccupancy = getEventQueueOccupancy();
        if (currentOccupancy > eventQueueMaxOccupancy)
        {
            eventQueueMaxOccupancy = currentOccupancy;
        }
    }
    else
    {
        eventQueueDroppedCount++;
        Serial.println("[BLYNK] Event queue full, dropping event!");
    }
}

static void clearEventQueue()
{
    eventQueueHead = 0;
    eventQueueTail = 0;
}

void getTerminalQueueDiagnostics(int &maxOccupancy, int &droppedCount)
{
    maxOccupancy = eventQueueMaxOccupancy;
    droppedCount = eventQueueDroppedCount;
}

// Public: send an important one-off event via async FIFO queue (V200)
void blynkTerminalEvent(const String& msg)
{
    queueEventLine("[EVT] " + msg);
    Serial.print("[BLYNK] Terminal event queued: ");
    Serial.println(msg);
}

// ---------------------------------------------------------------
// sendStatusToTerminal()
// Rate-limited: called every 60s from updateDashboard()
// Sends complete multi-line status report to V200 terminal in
// EXACTLY ONE Blynk.virtualWrite(200, report) call.
// ---------------------------------------------------------------

static void sendStatusToTerminal()
{
    if (!Blynk.connected())
    {
        Serial.println("[BLYNK] Terminal report skipped: not connected");
        return;
    }

    String report = "";
    report.reserve(512);

    report += "------------------------------------------------\n";
    report += "SYSTEM STATUS\n\n";

    report += "WiFi      : ";
    report += (WiFi.status() == WL_CONNECTED ? "Connected\n" : "Disconnected\n");

    report += "RSSI      : ";
    report += String(WiFi.RSSI());
    report += " dBm\n";

    report += "ESP-NOW   : OK\n\n";

    report += "Temp      : ";
    if (isnan(globalTemperature)) {
        report += "--.- °C\n";
    } else {
        report += String(globalTemperature, 1);
        report += " °C\n";
    }

    report += "Humidity  : ";
    if (isnan(globalHumidity)) {
        report += "-- %\n";
    } else {
        report += String(globalHumidity, 0);
        report += " %\n";
    }

    report += "Brightness: ";
    report += String(bedroom1.brightness);
    report += "\n";

    report += "Mode      : ";
    report += (bedroom1.darkState ? "NIGHT\n\n" : "DAY\n\n");

    report += "Bedroom1\n";
    report += "Motion : ";
    report += (bedroom1.motionDetected ? "YES\n" : "NO\n");
    report += "Bulb   : ";
    report += (bedroom1Bulb.currentState ? "ON\n" : "OFF\n");
    report += "Fan    : ";
    report += (bedroom1Fan.currentState ? "ON\n" : "OFF\n");
    report += "Socket : ";
    report += (bedroom1Socket.currentState ? "ON\n\n" : "OFF\n\n");

    report += "LivingRoom\n";
    report += "Motion : ";
    report += (livingroom.motionDetected ? "YES\n" : "NO\n");
    report += "Bulb   : ";
    report += (livingroomOutsideBulb.currentState ? "ON\n" : "OFF\n");
    report += "Fan    : ";
    report += (livingroomFan.currentState ? "ON\n\n" : "OFF\n\n");

    report += "DiningHall\n";
    report += "Motion : ";
    report += (dininghall.motionDetected ? "YES\n" : "NO\n");
    report += "Bulb   : ";
    report += (dininghallBulb.currentState ? "ON\n" : "OFF\n");
    report += "Fan    : ";
    report += (dininghallFan.currentState ? "ON\n" : "OFF\n");
    report += "Socket : ";
    report += (dininghallSocket.currentState ? "ON\n" : "OFF\n");
    report += "Extra1 : ";
    report += (dininghallExtra1.currentState ? "ON\n" : "OFF\n");
    report += "Extra2 : ";
    report += (dininghallExtra2.currentState ? "ON\n\n" : "OFF\n\n");

    report += "------------------------------------------------";

    Blynk.virtualWrite(200, report);
    countBlynkMessage(true, false);

    Serial.println("[BLYNK] Sent 60s Terminal Status Report (V200) as 1 single virtualWrite");
}


static void handleIncomingTimer(const char* prefix, DeviceConfig &device, const TimeInputParam &t, BlynkTimerCache &cache)
{
    if (device.mode == 0) // MODE_OFF
    {
        Serial.println("[TIMER] Ignored (Mode OFF)");
        return;
    }
    if (device.mode == 1) // MODE_ON
    {
        Serial.println("[TIMER] Ignored (Mode ON)");
        return;
    }

    // Extract complete state from t
    uint8_t incomingStartH = t.hasStartTime() ? t.getStartHour() : 0;
    uint8_t incomingStartM = t.hasStartTime() ? t.getStartMinute() : 0;
    uint8_t incomingStopH  = t.hasStopTime() ? t.getStopHour() : 0;
    uint8_t incomingStopM  = t.hasStopTime() ? t.getStopMinute() : 0;
    bool incomingHasStart   = t.hasStartTime();
    bool incomingHasStop    = t.hasStopTime();

    uint8_t incomingWeekdays = 0;
    for (int i = 1; i <= 7; i++)
    {
        if (t.isWeekdaySelected(i))
        {
            incomingWeekdays |= (1 << (i - 1));
        }
    }

    String incomingTz = t.getTZ();

    // Check against cache
    bool cacheMatches = blynkCacheInitialized &&
                        incomingStartH == cache.startHour &&
                        incomingStartM == cache.startMinute &&
                        incomingStopH == cache.stopHour &&
                        incomingStopM == cache.stopMinute &&
                        incomingHasStart == cache.hasStart &&
                        incomingHasStop == cache.hasStop &&
                        incomingWeekdays == cache.weekdays &&
                        incomingTz.equals(cache.timezone);

    if (cacheMatches)
    {
        // Everything matches the cache (including weekdays, tz, etc.) -> suppress redundant logic!
        return;
    }

    // Cache did not match, update NVS if hours/minutes changed
    if (device.mode == 2) // MODE_AUTO
    {
        uint8_t newStartH = t.hasStartTime() ? t.getStartHour() : device.autoStartHour;
        uint8_t newStartM = t.hasStartTime() ? t.getStartMinute() : device.autoStartMinute;
        uint8_t newStopH  = t.hasStopTime() ? t.getStopHour() : device.autoStopHour;
        uint8_t newStopM  = t.hasStopTime() ? t.getStopMinute() : device.autoStopMinute;

        bool nvsChanged = (newStartH != device.autoStartHour ||
                           newStartM != device.autoStartMinute ||
                           newStopH  != device.autoStopHour ||
                           newStopM  != device.autoStopMinute);

        if (nvsChanged)
        {
            device.autoStartHour   = newStartH;
            device.autoStartMinute = newStartM;
            device.autoStopHour   = newStopH;
            device.autoStopMinute = newStopM;
            saveSingleDevice(prefix, device);
            Serial.printf("[BLYNK] %s AUTO SCHEDULE updated\n", prefix);
        }
    }
    else if (device.mode == 3) // MODE_SCHEDULED
    {
        uint8_t newStartH = t.hasStartTime() ? t.getStartHour() : device.schedStartHour;
        uint8_t newStartM = t.hasStartTime() ? t.getStartMinute() : device.schedStartMinute;
        uint8_t newStopH  = t.hasStopTime() ? t.getStopHour() : device.schedStopHour;
        uint8_t newStopM  = t.hasStopTime() ? t.getStopMinute() : device.schedStopMinute;

        bool nvsChanged = (newStartH != device.schedStartHour ||
                           newStartM != device.schedStartMinute ||
                           newStopH  != device.schedStopHour ||
                           newStopM  != device.schedStopMinute);

        if (nvsChanged)
        {
            device.schedStartHour   = newStartH;
            device.schedStartMinute = newStartM;
            device.schedStopHour   = newStopH;
            device.schedStopMinute = newStopM;
            saveSingleDevice(prefix, device);
            Serial.printf("[BLYNK] %s SCHEDULE SCHEDULE updated\n", prefix);
        }
    }

    // Always update cache to match the exact state that came from Blynk
    cache.startHour = incomingStartH;
    cache.startMinute = incomingStartM;
    cache.stopHour = incomingStopH;
    cache.stopMinute = incomingStopM;
    cache.hasStart = incomingHasStart;
    cache.hasStop = incomingHasStop;
    cache.weekdays = incomingWeekdays;
    snprintf(cache.timezone, sizeof(cache.timezone), "%s", incomingTz.c_str());
}

void setDeviceMode(uint8_t nodeID, uint8_t deviceID, uint8_t newMode)
{
    DeviceConfig* device = nullptr;
    BlynkTimerCache* cache = nullptr;
    uint8_t* modeCache = nullptr;
    const char* prefix = nullptr;

    if (nodeID == BEDROOM1_NODE)
    {
        switch (deviceID)
        {
            case FAN_DEVICE:
                device = &bedroom1Fan;
                cache = &blynkFanCache;
                modeCache = &blynkFanModeCache;
                prefix = "fan";
                break;
            case TUBELIGHT_DEVICE:
                device = &bedroom1Tube;
                cache = &blynkTubeCache;
                modeCache = &blynkTubeModeCache;
                prefix = "tube";
                break;
            case BULB_DEVICE:
                device = &bedroom1Bulb;
                cache = &blynkBulbCache;
                modeCache = &blynkBulbModeCache;
                prefix = "bulb";
                break;
            case SOCKET_DEVICE:
                device = &bedroom1Socket;
                cache = &blynkSocketCache;
                modeCache = &blynkSocketModeCache;
                prefix = "sock";
                break;
            case AC_DEVICE:
                device = &bedroom1AC;
                cache = &blynkACCache;
                modeCache = &blynkACModeCache;
                prefix = "ac";
                break;
            default:
                return;
        }
    }
    else if (nodeID == LIVINGROOM_NODE)
    {
        switch (deviceID)
        {
            case 1:
                device = &livingroomTube1;
                cache = &blynkLrTube1Cache;
                modeCache = &blynkLrTube1ModeCache;
                prefix = "lr_t1";
                break;
            case 2:
                device = &livingroomTube2;
                cache = &blynkLrTube2Cache;
                modeCache = &blynkLrTube2ModeCache;
                prefix = "lr_t2";
                break;
            case 3:
                device = &livingroomFan;
                cache = &blynkLrFanCache;
                modeCache = &blynkLrFanModeCache;
                prefix = "lr_fan";
                break;
            case 4:
                device = &livingroomEBike;
                cache = &blynkLrEBikeCache;
                modeCache = &blynkLrEBikeModeCache;
                prefix = "lr_ebk";
                break;
            case 5:
                device = &livingroomSocket;
                cache = &blynkLrSocketCache;
                modeCache = &blynkLrSocketModeCache;
                prefix = "lr_soc";
                break;
            case 6:
                device = &livingroomOutsideBulb;
                cache = &blynkLrOutsideBulbCache;
                modeCache = &blynkLrOutsideBulbModeCache;
                prefix = "lr_ob";
                break;
            case 7:
                device = &livingroomExtra1;
                cache = &blynkLrExtra1Cache;
                modeCache = &blynkLrExtra1ModeCache;
                prefix = "lr_ex1";
                break;
            case 8:
                device = &livingroomExtra2;
                cache = &blynkLrExtra2Cache;
                modeCache = &blynkLrExtra2ModeCache;
                prefix = "lr_ex2";
                break;
            default:
                return;
        }
    }
    else if (nodeID == DININGHALL_NODE)
    {
        switch (deviceID)
        {
            case 1:
                device = &dininghallBulb;
                cache = &blynkDhBulbCache;
                modeCache = &blynkDhBulbModeCache;
                prefix = "dh_bulb";
                break;
            case 2:
                device = &dininghallTube;
                cache = &blynkDhTubeCache;
                modeCache = &blynkDhTubeModeCache;
                prefix = "dh_tube";
                break;
            case 3:
                device = &dininghallFan;
                cache = &blynkDhFanCache;
                modeCache = &blynkDhFanModeCache;
                prefix = "dh_fan";
                break;
            case 4:
                device = &dininghallSocket;
                cache = &blynkDhSocketCache;
                modeCache = &blynkDhSocketModeCache;
                prefix = "dh_sock";
                break;
            case 5:
                device = &dininghallExtra1;
                cache = &blynkDhExtra1Cache;
                modeCache = &blynkDhExtra1ModeCache;
                prefix = "dh_ex1";
                break;
            case 6:
                device = &dininghallExtra2;
                cache = &blynkDhExtra2Cache;
                modeCache = &blynkDhExtra2ModeCache;
                prefix = "dh_ex2";
                break;
            default:
                return;
        }
    }

    if (nodeID == DININGHALL_NODE && (deviceID == 5 || deviceID == 6))
    {
        // Extra Socket 1 / Extra Socket 2: manual only, never AUTO or SCHEDULE
        if (newMode == MODE_AUTO || newMode == MODE_SCHEDULED)
        {
            newMode = MODE_OFF;
        }
    }

    if (device != nullptr && device->mode != newMode)
    {
        device->mode = newMode;
        saveSingleDevice(prefix, *device);
        uint8_t pin;
        if (nodeID == BEDROOM1_NODE)
        {
            pin = deviceID - 1;
        }
        else if (nodeID == LIVINGROOM_NODE)
        {
            if (deviceID <= 5) pin = 10 + (deviceID - 1);
            else pin = 17 + (deviceID - 6);
        }
        else
        {
            pin = 20 + (deviceID - 1);
        }
        writeModeIfChanged(pin, newMode, *modeCache);
        uint8_t timerPin;
        if (nodeID == BEDROOM1_NODE)
        {
            timerPin = 100 + (deviceID - 1);
        }
        else if (nodeID == LIVINGROOM_NODE)
        {
            timerPin = 110 + (deviceID - 1);
        }
        else
        {
            timerPin = 120 + (deviceID - 1);
        }
        syncTimerWidget(timerPin, *device, *cache);
        Serial.printf("[MODE CHANGE] Node=%s Device=%d Mode=%d\n", getNodeName(nodeID), deviceID, newMode);
    }
}


// ---------------------------------------------------------------
// V0-V4 : Mode Controls (Blynk → Master)
// ---------------------------------------------------------------

// ---------------------------------------------------------------
// V0-V4 : Bedroom1 Mode Controls (Blynk → Master)
// ---------------------------------------------------------------

BLYNK_WRITE(V0)
{
    setDeviceMode(BEDROOM1_NODE, FAN_DEVICE, param.asInt());
}

BLYNK_WRITE(V1)
{
    setDeviceMode(BEDROOM1_NODE, TUBELIGHT_DEVICE, param.asInt());
}

BLYNK_WRITE(V2)
{
    setDeviceMode(BEDROOM1_NODE, BULB_DEVICE, param.asInt());
}

BLYNK_WRITE(V3)
{
    setDeviceMode(BEDROOM1_NODE, SOCKET_DEVICE, param.asInt());
}

BLYNK_WRITE(V4)
{
    setDeviceMode(BEDROOM1_NODE, AC_DEVICE, param.asInt());
}

// ---------------------------------------------------------------
// V10-V14, V17-V19 : LivingRoom Mode Controls (Blynk → Master)
// ---------------------------------------------------------------

BLYNK_WRITE(V10)
{
    setDeviceMode(LIVINGROOM_NODE, 1, param.asInt());
}

BLYNK_WRITE(V11)
{
    setDeviceMode(LIVINGROOM_NODE, 2, param.asInt());
}

BLYNK_WRITE(V12)
{
    setDeviceMode(LIVINGROOM_NODE, 3, param.asInt());
}

BLYNK_WRITE(V13)
{
    setDeviceMode(LIVINGROOM_NODE, 4, param.asInt());
}

BLYNK_WRITE(V14)
{
    setDeviceMode(LIVINGROOM_NODE, 5, param.asInt());
}

BLYNK_WRITE(V17)
{
    setDeviceMode(LIVINGROOM_NODE, 6, param.asInt());
}

BLYNK_WRITE(V18)
{
    setDeviceMode(LIVINGROOM_NODE, 7, param.asInt());
}

BLYNK_WRITE(V19)
{
    setDeviceMode(LIVINGROOM_NODE, 8, param.asInt());
}

// ---------------------------------------------------------------
// V20-V25 : DiningHall Mode Controls (Blynk → Master)
// ---------------------------------------------------------------

BLYNK_WRITE(V20)
{
    setDeviceMode(DININGHALL_NODE, 1, param.asInt());
}

BLYNK_WRITE(V21)
{
    setDeviceMode(DININGHALL_NODE, 2, param.asInt());
}

BLYNK_WRITE(V22)
{
    setDeviceMode(DININGHALL_NODE, 3, param.asInt());
}

BLYNK_WRITE(V23)
{
    setDeviceMode(DININGHALL_NODE, 4, param.asInt());
}

BLYNK_WRITE(V24)
{
    setDeviceMode(DININGHALL_NODE, 5, param.asInt());
}

BLYNK_WRITE(V25)
{
    setDeviceMode(DININGHALL_NODE, 6, param.asInt());
}

// ---------------------------------------------------------------
// V100-V104 : Bedroom1 Schedule Inputs (Blynk → Master)
// ---------------------------------------------------------------

BLYNK_WRITE(V100)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("fan", bedroom1Fan, t, blynkFanCache);
}

BLYNK_WRITE(V101)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("tube", bedroom1Tube, t, blynkTubeCache);
}

BLYNK_WRITE(V102)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("bulb", bedroom1Bulb, t, blynkBulbCache);
}

BLYNK_WRITE(V103)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("sock", bedroom1Socket, t, blynkSocketCache);
}

BLYNK_WRITE(V104)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("ac", bedroom1AC, t, blynkACCache);
}

// ---------------------------------------------------------------
// V110-V117 : LivingRoom Schedule Inputs (Blynk → Master)
// ---------------------------------------------------------------

BLYNK_WRITE(V110)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("lr_t1", livingroomTube1, t, blynkLrTube1Cache);
}

BLYNK_WRITE(V111)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("lr_t2", livingroomTube2, t, blynkLrTube2Cache);
}

BLYNK_WRITE(V112)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("lr_fan", livingroomFan, t, blynkLrFanCache);
}

BLYNK_WRITE(V113)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("lr_ebk", livingroomEBike, t, blynkLrEBikeCache);
}

BLYNK_WRITE(V114)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("lr_soc", livingroomSocket, t, blynkLrSocketCache);
}

BLYNK_WRITE(V115)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("lr_ob", livingroomOutsideBulb, t, blynkLrOutsideBulbCache);
}

BLYNK_WRITE(V116)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("lr_ex1", livingroomExtra1, t, blynkLrExtra1Cache);
}

BLYNK_WRITE(V117)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("lr_ex2", livingroomExtra2, t, blynkLrExtra2Cache);
}

// ---------------------------------------------------------------
// V120-V125 : DiningHall Schedule Inputs (Blynk → Master)
// ---------------------------------------------------------------

BLYNK_WRITE(V120)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("dh_bulb", dininghallBulb, t, blynkDhBulbCache);
}

BLYNK_WRITE(V121)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("dh_tube", dininghallTube, t, blynkDhTubeCache);
}

BLYNK_WRITE(V122)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("dh_fan", dininghallFan, t, blynkDhFanCache);
}

BLYNK_WRITE(V123)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("dh_sock", dininghallSocket, t, blynkDhSocketCache);
}

BLYNK_WRITE(V124)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("dh_ex1", dininghallExtra1, t, blynkDhExtra1Cache);
}

BLYNK_WRITE(V125)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);
    handleIncomingTimer("dh_ex2", dininghallExtra2, t, blynkDhExtra2Cache);
}


#define BLYNK_WRITE_PIN(pin) BLYNK_WRITE_PIN_HIDDEN(pin)
#define BLYNK_WRITE_PIN_HIDDEN(pin) BLYNK_WRITE(pin)

BLYNK_WRITE_PIN(VPIN_B1_LDR_ENABLE)
{
    bedroom1LdrEnabled = (param.asInt() == 1);
    Serial.print("[BLYNK] B1 LDR ENABLE -> ");
    Serial.println(bedroom1LdrEnabled ? "ON" : "OFF");
    saveRoomLdrEnabled(BEDROOM1_NODE, bedroom1LdrEnabled);
    blynkLdrEnableCache = param.asInt();
}

BLYNK_WRITE_PIN(VPIN_B1_MOTION_TIMEOUT)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);

    uint8_t h = t.hasStartTime() ? t.getStartHour() : 0;
    uint8_t m = t.hasStartTime() ? t.getStartMinute() : 0;
    uint8_t s = t.hasStartTime() ? t.getStartSecond() : 0;

    RoomConfig tempConfig = { h, m, s, 0 };
    clampRoomMotionTimeout(tempConfig);

    bool changed = (tempConfig.motionTimeoutHour != bedroom1Config.motionTimeoutHour ||
                    tempConfig.motionTimeoutMinute != bedroom1Config.motionTimeoutMinute ||
                    tempConfig.motionTimeoutSecond != bedroom1Config.motionTimeoutSecond);

    if (changed)
    {
        bedroom1Config = tempConfig;
        saveRoomMotionTimeout(BEDROOM1_NODE, bedroom1Config);
        Serial.printf("[BLYNK] B1 Motion Timeout updated to %02u:%02u:%02u (%lu ms)\n", 
                      bedroom1Config.motionTimeoutHour, 
                      bedroom1Config.motionTimeoutMinute, 
                      bedroom1Config.motionTimeoutSecond, 
                      bedroom1Config.motionTimeoutMs);

        if (tempConfig.motionTimeoutHour != h || 
            tempConfig.motionTimeoutMinute != m || 
            tempConfig.motionTimeoutSecond != s)
        {
            writeMotionTimeoutIfChanged(VPIN_B1_MOTION_TIMEOUT_NUM, 
                                        bedroom1Config.motionTimeoutHour, 
                                        bedroom1Config.motionTimeoutMinute, 
                                        bedroom1Config.motionTimeoutSecond, 
                                        blynkMotionTimeoutCache);
        }
    }

    blynkMotionTimeoutCache.hour = bedroom1Config.motionTimeoutHour;
    blynkMotionTimeoutCache.minute = bedroom1Config.motionTimeoutMinute;
    blynkMotionTimeoutCache.second = bedroom1Config.motionTimeoutSecond;
    blynkMotionTimeoutCache.hasStart = true;
}

BLYNK_WRITE_PIN(VPIN_LR_LDR_ENABLE)
{
    livingroomLdrEnabled = (param.asInt() == 1);
    Serial.print("[BLYNK] LR LDR ENABLE -> ");
    Serial.println(livingroomLdrEnabled ? "ON" : "OFF");
    saveRoomLdrEnabled(LIVINGROOM_NODE, livingroomLdrEnabled);
    blynkLrLdrEnableCache = param.asInt();
}

BLYNK_WRITE_PIN(VPIN_LR_MOTION_TIMEOUT)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);

    uint8_t h = t.hasStartTime() ? t.getStartHour() : 0;
    uint8_t m = t.hasStartTime() ? t.getStartMinute() : 0;
    uint8_t s = t.hasStartTime() ? t.getStartSecond() : 0;

    RoomConfig tempConfig = { h, m, s, 0 };
    clampRoomMotionTimeout(tempConfig);

    bool changed = (tempConfig.motionTimeoutHour != livingroomConfig.motionTimeoutHour ||
                    tempConfig.motionTimeoutMinute != livingroomConfig.motionTimeoutMinute ||
                    tempConfig.motionTimeoutSecond != livingroomConfig.motionTimeoutSecond);

    if (changed)
    {
        livingroomConfig = tempConfig;
        saveRoomMotionTimeout(LIVINGROOM_NODE, livingroomConfig);
        Serial.printf("[BLYNK] LR Motion Timeout updated to %02u:%02u:%02u (%lu ms)\n", 
                      livingroomConfig.motionTimeoutHour, 
                      livingroomConfig.motionTimeoutMinute, 
                      livingroomConfig.motionTimeoutSecond, 
                      livingroomConfig.motionTimeoutMs);

        if (tempConfig.motionTimeoutHour != h || 
            tempConfig.motionTimeoutMinute != m || 
            tempConfig.motionTimeoutSecond != s)
        {
            writeMotionTimeoutIfChanged(VPIN_LR_MOTION_TIMEOUT_NUM, 
                                        livingroomConfig.motionTimeoutHour, 
                                        livingroomConfig.motionTimeoutMinute, 
                                        livingroomConfig.motionTimeoutSecond, 
                                        blynkLrMotionTimeoutCache);
        }
    }

    blynkLrMotionTimeoutCache.hour = livingroomConfig.motionTimeoutHour;
    blynkLrMotionTimeoutCache.minute = livingroomConfig.motionTimeoutMinute;
    blynkLrMotionTimeoutCache.second = livingroomConfig.motionTimeoutSecond;
    blynkLrMotionTimeoutCache.hasStart = true;
}

BLYNK_WRITE_PIN(VPIN_DH_LDR_ENABLE)
{
    dininghallLdrEnabled = (param.asInt() == 1);
    Serial.print("[BLYNK] DH LDR ENABLE -> ");
    Serial.println(dininghallLdrEnabled ? "ON" : "OFF");
    saveRoomLdrEnabled(DININGHALL_NODE, dininghallLdrEnabled);
    blynkDhLdrEnableCache = param.asInt();
}

BLYNK_WRITE_PIN(VPIN_DH_MOTION_TIMEOUT)
{
    if (timerSyncInProgress) return;
    TimeInputParam t(param);

    uint8_t h = t.hasStartTime() ? t.getStartHour() : 0;
    uint8_t m = t.hasStartTime() ? t.getStartMinute() : 0;
    uint8_t s = t.hasStartTime() ? t.getStartSecond() : 0;

    RoomConfig tempConfig = { h, m, s, 0 };
    clampRoomMotionTimeout(tempConfig);

    bool changed = (tempConfig.motionTimeoutHour != dininghallConfig.motionTimeoutHour ||
                    tempConfig.motionTimeoutMinute != dininghallConfig.motionTimeoutMinute ||
                    tempConfig.motionTimeoutSecond != dininghallConfig.motionTimeoutSecond);

    if (changed)
    {
        dininghallConfig = tempConfig;
        saveRoomMotionTimeout(DININGHALL_NODE, dininghallConfig);
        Serial.printf("[BLYNK] DH Motion Timeout updated to %02u:%02u:%02u (%lu ms)\n", 
                      dininghallConfig.motionTimeoutHour, 
                      dininghallConfig.motionTimeoutMinute, 
                      dininghallConfig.motionTimeoutSecond, 
                      dininghallConfig.motionTimeoutMs);

        if (tempConfig.motionTimeoutHour != h || 
            tempConfig.motionTimeoutMinute != m || 
            tempConfig.motionTimeoutSecond != s)
        {
            writeMotionTimeoutIfChanged(VPIN_DH_MOTION_TIMEOUT_NUM, 
                                        dininghallConfig.motionTimeoutHour, 
                                        dininghallConfig.motionTimeoutMinute, 
                                        dininghallConfig.motionTimeoutSecond, 
                                        blynkDhMotionTimeoutCache);
        }
    }

    blynkDhMotionTimeoutCache.hour = dininghallConfig.motionTimeoutHour;
    blynkDhMotionTimeoutCache.minute = dininghallConfig.motionTimeoutMinute;
    blynkDhMotionTimeoutCache.second = dininghallConfig.motionTimeoutSecond;
    blynkDhMotionTimeoutCache.hasStart = true;
}

void updateAllBlynkWidgets()
{
    Serial.println("[BLYNK] Updating all widgets to match current configuration (cache filtered)...");

    // 1. Mode widgets: V0-V4 (Bedroom1), V10-V14/V17-V19 (LivingRoom), V20-V25 (DiningHall)
    writeModeIfChanged(0, bedroom1Fan.mode, blynkFanModeCache);
    writeModeIfChanged(1, bedroom1Tube.mode, blynkTubeModeCache);
    writeModeIfChanged(2, bedroom1Bulb.mode, blynkBulbModeCache);
    writeModeIfChanged(3, bedroom1Socket.mode, blynkSocketModeCache);
    writeModeIfChanged(4, bedroom1AC.mode, blynkACModeCache);

    writeModeIfChanged(10, livingroomTube1.mode, blynkLrTube1ModeCache);
    writeModeIfChanged(11, livingroomTube2.mode, blynkLrTube2ModeCache);
    writeModeIfChanged(12, livingroomFan.mode, blynkLrFanModeCache);
    writeModeIfChanged(13, livingroomEBike.mode, blynkLrEBikeModeCache);
    writeModeIfChanged(14, livingroomSocket.mode, blynkLrSocketModeCache);
    writeModeIfChanged(17, livingroomOutsideBulb.mode, blynkLrOutsideBulbModeCache);
    writeModeIfChanged(18, livingroomExtra1.mode, blynkLrExtra1ModeCache);
    writeModeIfChanged(19, livingroomExtra2.mode, blynkLrExtra2ModeCache);

    writeModeIfChanged(20, dininghallBulb.mode, blynkDhBulbModeCache);
    writeModeIfChanged(21, dininghallTube.mode, blynkDhTubeModeCache);
    writeModeIfChanged(22, dininghallFan.mode, blynkDhFanModeCache);
    writeModeIfChanged(23, dininghallSocket.mode, blynkDhSocketModeCache);
    writeModeIfChanged(24, dininghallExtra1.mode, blynkDhExtra1ModeCache);
    writeModeIfChanged(25, dininghallExtra2.mode, blynkDhExtra2ModeCache);

    // 2. LDR Enable Switch Widget
    writeLdrEnableIfChanged(VPIN_B1_LDR_ENABLE_NUM, bedroom1LdrEnabled, blynkLdrEnableCache);
    writeLdrEnableIfChanged(VPIN_LR_LDR_ENABLE_NUM, livingroomLdrEnabled, blynkLrLdrEnableCache);
    writeLdrEnableIfChanged(VPIN_DH_LDR_ENABLE_NUM, dininghallLdrEnabled, blynkDhLdrEnableCache);

    // 2b. Motion Timeout Duration Widget
    writeMotionTimeoutIfChanged(VPIN_B1_MOTION_TIMEOUT_NUM, bedroom1Config.motionTimeoutHour, bedroom1Config.motionTimeoutMinute, bedroom1Config.motionTimeoutSecond, blynkMotionTimeoutCache);
    writeMotionTimeoutIfChanged(VPIN_LR_MOTION_TIMEOUT_NUM, livingroomConfig.motionTimeoutHour, livingroomConfig.motionTimeoutMinute, livingroomConfig.motionTimeoutSecond, blynkLrMotionTimeoutCache);
    writeMotionTimeoutIfChanged(VPIN_DH_MOTION_TIMEOUT_NUM, dininghallConfig.motionTimeoutHour, dininghallConfig.motionTimeoutMinute, dininghallConfig.motionTimeoutSecond, blynkDhMotionTimeoutCache);

    // 3. LED widgets: V150-V154 (Bedroom1), V160-V167 (LivingRoom), V170-V175 (DiningHall)
    writeLedIfChanged(150, bedroom1Fan.currentState, prevFanState);
    writeLedIfChanged(151, bedroom1Tube.currentState, prevTubeState);
    writeLedIfChanged(152, bedroom1Bulb.currentState, prevBulbState);
    writeLedIfChanged(153, bedroom1Socket.currentState, prevSocketState);
    writeLedIfChanged(154, bedroom1AC.currentState, prevACState);

    writeLedIfChanged(160, livingroomTube1.currentState, prevLrTube1State);
    writeLedIfChanged(161, livingroomTube2.currentState, prevLrTube2State);
    writeLedIfChanged(162, livingroomFan.currentState, prevLrFanState);
    writeLedIfChanged(163, livingroomEBike.currentState, prevLrEBikeState);
    writeLedIfChanged(164, livingroomSocket.currentState, prevLrSocketState);
    writeLedIfChanged(165, livingroomOutsideBulb.currentState, prevLrOutsideBulbState);
    writeLedIfChanged(166, livingroomExtra1.currentState, prevLrExtra1State);
    writeLedIfChanged(167, livingroomExtra2.currentState, prevLrExtra2State);

    writeLedIfChanged(170, dininghallBulb.currentState, prevDhBulbState);
    writeLedIfChanged(171, dininghallTube.currentState, prevDhTubeState);
    writeLedIfChanged(172, dininghallFan.currentState, prevDhFanState);
    writeLedIfChanged(173, dininghallSocket.currentState, prevDhSocketState);
    writeLedIfChanged(174, dininghallExtra1.currentState, prevDhExtra1State);
    writeLedIfChanged(175, dininghallExtra2.currentState, prevDhExtra2State);

    // 4. Timer/Schedule widgets: V100-V104 (Bedroom1), V110-V117 (LivingRoom), V120-V125 (DiningHall)
    syncTimerWidget(100, bedroom1Fan, blynkFanCache);
    syncTimerWidget(101, bedroom1Tube, blynkTubeCache);
    syncTimerWidget(102, bedroom1Bulb, blynkBulbCache);
    syncTimerWidget(103, bedroom1Socket, blynkSocketCache);
    syncTimerWidget(104, bedroom1AC, blynkACCache);

    syncTimerWidget(110, livingroomTube1, blynkLrTube1Cache);
    syncTimerWidget(111, livingroomTube2, blynkLrTube2Cache);
    syncTimerWidget(112, livingroomFan, blynkLrFanCache);
    syncTimerWidget(113, livingroomEBike, blynkLrEBikeCache);
    syncTimerWidget(114, livingroomSocket, blynkLrSocketCache);
    syncTimerWidget(115, livingroomOutsideBulb, blynkLrOutsideBulbCache);
    syncTimerWidget(116, livingroomExtra1, blynkLrExtra1Cache);
    syncTimerWidget(117, livingroomExtra2, blynkLrExtra2Cache);

    syncTimerWidget(120, dininghallBulb, blynkDhBulbCache);
    syncTimerWidget(121, dininghallTube, blynkDhTubeCache);
    syncTimerWidget(122, dininghallFan, blynkDhFanCache);
    syncTimerWidget(123, dininghallSocket, blynkDhSocketCache);
    syncTimerWidget(124, dininghallExtra1, blynkDhExtra1Cache);
    syncTimerWidget(125, dininghallExtra2, blynkDhExtra2Cache);

    blynkCacheInitialized = true;
}

BLYNK_CONNECTED()
{
    Serial.println("[BLYNK] Connected callback triggered");
    updateAllBlynkWidgets();
}

void initDashboard()
{
    Blynk.config(auth);

    if (Blynk.connect(5000))
    {
        Serial.println("[BLYNK] Connected");
    }
    else
    {
        Serial.println("[BLYNK] Connect failed; updateDashboard will retry");
    }

    prevFanState    = bedroom1Fan.currentState;
    prevTubeState   = bedroom1Tube.currentState;
    prevBulbState   = bedroom1Bulb.currentState;
    prevSocketState = bedroom1Socket.currentState;
    prevACState     = bedroom1AC.currentState;

    prevLrTube1State       = livingroomTube1.currentState;
    prevLrTube2State       = livingroomTube2.currentState;
    prevLrFanState         = livingroomFan.currentState;
    prevLrEBikeState       = livingroomEBike.currentState;
    prevLrSocketState      = livingroomSocket.currentState;
    prevLrOutsideBulbState = livingroomOutsideBulb.currentState;
    prevLrExtra1State      = livingroomExtra1.currentState;
    prevLrExtra2State      = livingroomExtra2.currentState;

    prevDhBulbState   = dininghallBulb.currentState;
    prevDhTubeState   = dininghallTube.currentState;
    prevDhFanState    = dininghallFan.currentState;
    prevDhSocketState = dininghallSocket.currentState;
    prevDhExtra1State = dininghallExtra1.currentState;
    prevDhExtra2State = dininghallExtra2.currentState;
}

void updateDashboard()
{
    static unsigned long lastConnectAttempt = 0;

    if (
        WiFi.status() == WL_CONNECTED &&
        !Blynk.connected() &&
        millis() - lastConnectAttempt >= 10000
    )
    {
        lastConnectAttempt = millis();
        Blynk.connect(1000);
    }

    Blynk.run();

    if (Blynk.connected())
    {
        // Drain asynchronous event queue (at most 1 event every 200ms to respect Blynk rate limits)
        static unsigned long lastEventSend = 0;
        if (eventQueueTail != eventQueueHead && millis() - lastEventSend >= 200)
        {
            Blynk.virtualWrite(200, eventQueue[eventQueueTail]);
            countBlynkMessage(true, false);
            eventQueueTail = (eventQueueTail + 1) % 8;
            lastEventSend = millis();
        }
    }
    else
    {
        clearEventQueue();
    }

    static unsigned long lastSend = 0;
    static unsigned long lastAudit = 0;

    if (millis() - lastSend >= 60000)
    {
        lastSend = millis();
        sendStatusToTerminal();
    }

    if (millis() - lastAudit >= 300000)
    {
        lastAudit = millis();
        printBlynkAudit();
    }
}

