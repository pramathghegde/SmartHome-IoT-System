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
    countBlynkMessage(pin == 200, pin >= 150 && pin <= 154);
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
    countBlynkMessage(pin == 200, pin >= 150 && pin <= 154);
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

static uint8_t blynkLdrEnableCache      = 0xFF;
static uint8_t blynkLrLdrEnableCache    = 0xFF;

struct BlynkMotionTimeoutCache
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    bool hasStart;
};

static BlynkMotionTimeoutCache blynkMotionTimeoutCache = {0xFF, 0xFF, 0xFF, false};
static BlynkMotionTimeoutCache blynkLrMotionTimeoutCache = {0xFF, 0xFF, 0xFF, false};


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
        Serial.print("[BLYNK] LED Node=");
        Serial.print(getNodeName(nodeID));
        Serial.print(" V");
        Serial.print(pin);
        Serial.print(" -> ");
        Serial.println(newState ? "ON" : "OFF");
    }
}


// ---------------------------------------------------------------
// sendStatusToTerminal()
// Prints full home status to Blynk Terminal
// Called every 60 seconds = 1 message/minute
// ---------------------------------------------------------------

static void sendStatusToTerminal()
{
    // Get current time
    char timeStr[9] = "--:--:--";

    if (isTimeValid())
    {
        struct tm timeinfo;

        if (getLocalTime(&timeinfo))
        {
            strftime(
                timeStr,
                sizeof(timeStr),
                "%H:%M:%S",
                &timeinfo
            );
        }
    }

    // Day/Night from B1 LDR (Global)
    const char* globalLightStatus =
        (bedroom1.brightness < DARK_THRESHOLD_LOW) ?
        "NIGHT" : "DAY  ";

    String status;
    status.reserve(1200);

    appendLine(status, "==============================");
    appendLine(status, "   ADVAITA SMART HOME");
    appendLine(status, String("   ") + timeStr);
    appendLine(status, "==============================");

    appendLine(status);
    appendLine(status, "--- ENVIRONMENT ----------");
    char tempBuf[40];
    char humBuf[40];
    snprintf(tempBuf, sizeof(tempBuf), "  Temp  : %.1f C", globalTemperature);
    snprintf(humBuf, sizeof(humBuf), "  Humid : %.1f %%", globalHumidity);
    appendLine(status, tempBuf);
    appendLine(status, humBuf);
    appendLine(status, String("  Light : ") + globalLightStatus);
    appendLine(status, "  Door  : ------");        // Future door lock

    appendLine(status);
    appendLine(status, "--- ROOM STATUS ----------");
    appendLine(status, String("  Bedroom1 : ") + (bedroom1.online ? "ONLINE " : "OFFLINE"));
    appendLine(status, String("    Motion : ") + (bedroom1.motionDetected ? "DETECTED" : "CLEAR   "));
    char toBuf[40];
    snprintf(toBuf, sizeof(toBuf), "    MotionTO: %02u:%02u:%02u",
             bedroom1Config.motionTimeoutHour,
             bedroom1Config.motionTimeoutMinute,
             bedroom1Config.motionTimeoutSecond);
    appendLine(status, toBuf);
    appendLine(status, String("    LDR Raw : ") + bedroom1.brightness + " [En:" + (bedroom1LdrEnabled ? "Y" : "N") + "]");

    appendLine(status, String("  LivingRoom: ") + (livingroom.online ? "ONLINE " : "OFFLINE"));
    appendLine(status, String("    Motion : ") + (livingroom.motionDetected ? "DETECTED" : "CLEAR   "));
    char toBufLr[40];
    snprintf(toBufLr, sizeof(toBufLr), "    MotionTO: %02u:%02u:%02u",
             livingroomConfig.motionTimeoutHour,
             livingroomConfig.motionTimeoutMinute,
             livingroomConfig.motionTimeoutSecond);
    appendLine(status, toBufLr);
    appendLine(status, String("    LDR En  : ") + (livingroomLdrEnabled ? "YES" : "NO"));

    appendLine(status);
    appendLine(status, "--- APPLIANCES -----------");
    appendLine(status, "  [Bedroom1]");
    appendLine(status, String("    Fan    : ") + stateStr(bedroom1Fan.currentState) +
        "  [" + modeStr(bedroom1Fan.mode) + "]");
    appendLine(status, String("    Tube   : ") + stateStr(bedroom1Tube.currentState) +
        "  [" + modeStr(bedroom1Tube.mode) + "]");
    appendLine(status, String("    Bulb   : ") + stateStr(bedroom1Bulb.currentState) +
        "  [" + modeStr(bedroom1Bulb.mode) + "]");
    appendLine(status, String("    Socket : ") + stateStr(bedroom1Socket.currentState) +
        "  [" + modeStr(bedroom1Socket.mode) + "]");
    appendLine(status, String("    AC     : ") + stateStr(bedroom1AC.currentState) +
        "  [" + modeStr(bedroom1AC.mode) + "]");

    appendLine(status, "  [LivingRoom]");
    appendLine(status, String("    Tube 1 : ") + stateStr(livingroomTube1.currentState) +
        "  [" + modeStr(livingroomTube1.mode) + "]");
    appendLine(status, String("    Tube 2 : ") + stateStr(livingroomTube2.currentState) +
        "  [" + modeStr(livingroomTube2.mode) + "]");
    appendLine(status, String("    Fan    : ") + stateStr(livingroomFan.currentState) +
        "  [" + modeStr(livingroomFan.mode) + "]");
    appendLine(status, String("    E-Bike : ") + stateStr(livingroomEBike.currentState) +
        "  [" + modeStr(livingroomEBike.mode) + "]");
    appendLine(status, String("    Socket : ") + stateStr(livingroomSocket.currentState) +
        "  [" + modeStr(livingroomSocket.mode) + "]");
    appendLine(status, String("    Bulb   : ") + stateStr(livingroomOutsideBulb.currentState) +
        "  [" + modeStr(livingroomOutsideBulb.mode) + "]");
    appendLine(status, String("    Extra 1: ") + stateStr(livingroomExtra1.currentState) +
        "  [" + modeStr(livingroomExtra1.mode) + "]");
    appendLine(status, String("    Extra 2: ") + stateStr(livingroomExtra2.currentState) +
        "  [" + modeStr(livingroomExtra2.mode) + "]");

    appendLine(status);
    appendLine(status, "--- SYSTEM ---------------");
    appendLine(status, String("  WiFi  : ") + WiFi.RSSI() + " dBm");
    appendLine(status, String("  Uptime: ") + (millis() / 60000) + " min");
    appendLine(status, "==============================");

    if (auditedVirtualWrite(200, status))
    {
        Serial.println("[BLYNK] Terminal status sent");
    }
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

    if (device != nullptr && device->mode != newMode)
    {
        device->mode = newMode;
        saveSingleDevice(prefix, *device);
        uint8_t pin;
        if (nodeID == BEDROOM1_NODE)
        {
            pin = deviceID - 1;
        }
        else
        {
            if (deviceID <= 5) pin = 10 + (deviceID - 1);
            else pin = 17 + (deviceID - 6);
        }
        writeModeIfChanged(pin, newMode, *modeCache);
        uint8_t timerPin = (nodeID == BEDROOM1_NODE) ? (100 + (deviceID - 1)) : (110 + (deviceID - 1));
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

void updateAllBlynkWidgets()
{
    Serial.println("[BLYNK] Updating all widgets to match current configuration (cache filtered)...");

    // 1. Mode widgets: V0-V4 (Bedroom1), V10-V14 (LivingRoom)
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

    // 2. LDR Enable Switch Widget
    writeLdrEnableIfChanged(VPIN_B1_LDR_ENABLE_NUM, bedroom1LdrEnabled, blynkLdrEnableCache);
    writeLdrEnableIfChanged(VPIN_LR_LDR_ENABLE_NUM, livingroomLdrEnabled, blynkLrLdrEnableCache);

    // Global Temperature & Humidity Widgets
    Blynk.virtualWrite(VPIN_GLOBAL_TEMPERATURE, globalTemperature);
    Blynk.virtualWrite(VPIN_GLOBAL_HUMIDITY, globalHumidity);

    // 2b. Motion Timeout Duration Widget
    writeMotionTimeoutIfChanged(VPIN_B1_MOTION_TIMEOUT_NUM, bedroom1Config.motionTimeoutHour, bedroom1Config.motionTimeoutMinute, bedroom1Config.motionTimeoutSecond, blynkMotionTimeoutCache);
    writeMotionTimeoutIfChanged(VPIN_LR_MOTION_TIMEOUT_NUM, livingroomConfig.motionTimeoutHour, livingroomConfig.motionTimeoutMinute, livingroomConfig.motionTimeoutSecond, blynkLrMotionTimeoutCache);

    // 3. LED widgets: V150-V154 (Bedroom1), V160-V167 (LivingRoom)
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

    // 4. Timer/Schedule widgets: V100-V104 (Bedroom1), V110-V117 (LivingRoom)
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
        static float prevTemp = -999.0f;
        static float prevHum = -999.0f;

        if (globalTemperature != prevTemp && !isnan(globalTemperature))
        {
            prevTemp = globalTemperature;
            Blynk.virtualWrite(VPIN_GLOBAL_TEMPERATURE, prevTemp);
        }
        if (globalHumidity != prevHum && !isnan(globalHumidity))
        {
            prevHum = globalHumidity;
            Blynk.virtualWrite(VPIN_GLOBAL_HUMIDITY, prevHum);
        }
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

