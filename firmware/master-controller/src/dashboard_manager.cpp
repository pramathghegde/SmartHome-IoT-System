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

// ---------------------------------------------------------------
// Blynk synchronization cache
// ---------------------------------------------------------------
static uint8_t blynkFanModeCache        = 0xFF;
static uint8_t blynkTubeModeCache       = 0xFF;
static uint8_t blynkBulbModeCache       = 0xFF;
static uint8_t blynkSocketModeCache     = 0xFF;
static uint8_t blynkACModeCache         = 0xFF;

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

static uint8_t blynkLdrEnableCache      = 0xFF;

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

// ---------------------------------------------------------------
// notifyDeviceStateChange()
// Called from automation_manager on every relay change
// Sends LED update for that device only = 1 message per change
// ---------------------------------------------------------------

void notifyDeviceStateChange(
    uint8_t deviceID,
    bool newState
)
{
    bool success = false;
    switch(deviceID)
    {
        case FAN_DEVICE:       success = writeLedIfChanged(150, newState, prevFanState);    break;
        case TUBELIGHT_DEVICE: success = writeLedIfChanged(151, newState, prevTubeState);   break;
        case BULB_DEVICE:      success = writeLedIfChanged(152, newState, prevBulbState);   break;
        case SOCKET_DEVICE:    success = writeLedIfChanged(153, newState, prevSocketState); break;
        case AC_DEVICE:        success = writeLedIfChanged(154, newState, prevACState);     break;
        default: return;
    }

    if (success)
    {
        int pin = -1;
        switch(deviceID)
        {
            case FAN_DEVICE:       pin = 150; break;
            case TUBELIGHT_DEVICE: pin = 151; break;
            case BULB_DEVICE:      pin = 152; break;
            case SOCKET_DEVICE:    pin = 153; break;
            case AC_DEVICE:        pin = 154; break;
        }
        Serial.print("[BLYNK] LED V");
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

    // Day/Night from LDR
    const char* lightStatus =
        (bedroom1.brightness < DARK_THRESHOLD_LOW) ?
        "NIGHT" : "DAY  ";

    String status;
    status.reserve(900);

    appendLine(status, "==============================");
    appendLine(status, "   ADVAITA SMART HOME");
    appendLine(status, String("   ") + timeStr);
    appendLine(status, "==============================");

    appendLine(status);
    appendLine(status, "--- ENVIRONMENT ----------");
    appendLine(status, "  Temp  : --.- C");        // Future DHT/BME
    appendLine(status, "  Humid : --.- %");        // Future DHT/BME
    appendLine(status, String("  Light : ") + lightStatus);
    appendLine(status, String("  LDR   : ") + bedroom1.brightness);
    appendLine(status, "  Door  : ------");        // Future door lock

    appendLine(status);
    appendLine(status, "--- ROOM STATUS ----------");
    appendLine(status, String("  Bedroom1 : ") + (bedroom1.online ? "ONLINE " : "OFFLINE"));
    appendLine(status, String("  Motion  : ") + (bedroom1.motionDetected ? "DETECTED" : "CLEAR   "));
    appendLine(status, "  Bedroom2 : -------");    // Future node
    appendLine(status, "  Hall     : -------");    // Future node
    appendLine(status, "  Kitchen  : -------");    // Future node

    appendLine(status);
    appendLine(status, "--- APPLIANCES -----------");
    appendLine(status, String("  Fan    : ") + stateStr(bedroom1Fan.currentState) +
        "  [" + modeStr(bedroom1Fan.mode) + "]");
    appendLine(status, String("  Tube   : ") + stateStr(bedroom1Tube.currentState) +
        "  [" + modeStr(bedroom1Tube.mode) + "]");
    appendLine(status, String("  Bulb   : ") + stateStr(bedroom1Bulb.currentState) +
        "  [" + modeStr(bedroom1Bulb.mode) + "]");
    appendLine(status, String("  Socket : ") + stateStr(bedroom1Socket.currentState) +
        "  [" + modeStr(bedroom1Socket.mode) + "]");
    appendLine(status, String("  AC     : ") + stateStr(bedroom1AC.currentState) +
        "  [" + modeStr(bedroom1AC.mode) + "]");

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

void setDeviceMode(uint8_t deviceID, uint8_t newMode)
{
    DeviceConfig* device = nullptr;
    BlynkTimerCache* cache = nullptr;
    uint8_t* modeCache = nullptr;
    const char* prefix = nullptr;

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

    if (device->mode != newMode)
    {
        device->mode = newMode;
        saveSingleDevice(prefix, *device);
        writeModeIfChanged(deviceID - 1, newMode, *modeCache);
        syncTimerWidget(100 + (deviceID - 1), *device, *cache);
        Serial.printf("[MODE CHANGE] Device=%d Mode=%d\n", deviceID, newMode);
    }
}

// ---------------------------------------------------------------
// V0-V4 : Mode Controls (Blynk → Master)
// ---------------------------------------------------------------

BLYNK_WRITE(V0)
{
    setDeviceMode(FAN_DEVICE, param.asInt());
}

BLYNK_WRITE(V1)
{
    setDeviceMode(TUBELIGHT_DEVICE, param.asInt());
}

BLYNK_WRITE(V2)
{
    setDeviceMode(BULB_DEVICE, param.asInt());
}

BLYNK_WRITE(V3)
{
    setDeviceMode(SOCKET_DEVICE, param.asInt());
}

BLYNK_WRITE(V4)
{
    setDeviceMode(AC_DEVICE, param.asInt());
}

// ---------------------------------------------------------------
// V100-V104 : Schedule Inputs (Blynk → Master)
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

#define BLYNK_WRITE_PIN(pin) BLYNK_WRITE_PIN_HIDDEN(pin)
#define BLYNK_WRITE_PIN_HIDDEN(pin) BLYNK_WRITE(pin)

BLYNK_WRITE_PIN(VPIN_B1_LDR_ENABLE)
{
    bedroom1LdrEnabled = (param.asInt() == 1);
    Serial.print("[BLYNK] LDR ENABLE -> ");
    Serial.println(bedroom1LdrEnabled ? "ON" : "OFF");
    saveRoomLdrEnabled(bedroom1LdrEnabled);
    blynkLdrEnableCache = param.asInt();
}

void updateAllBlynkWidgets()
{
    Serial.println("[BLYNK] Updating all widgets to match current configuration (cache filtered)...");

    // 1. Mode widgets: V0-V4
    writeModeIfChanged(0, bedroom1Fan.mode, blynkFanModeCache);
    writeModeIfChanged(1, bedroom1Tube.mode, blynkTubeModeCache);
    writeModeIfChanged(2, bedroom1Bulb.mode, blynkBulbModeCache);
    writeModeIfChanged(3, bedroom1Socket.mode, blynkSocketModeCache);
    writeModeIfChanged(4, bedroom1AC.mode, blynkACModeCache);

    // 2. LDR Enable Switch Widget
    writeLdrEnableIfChanged(VPIN_B1_LDR_ENABLE_NUM, bedroom1LdrEnabled, blynkLdrEnableCache);

    // 3. LED widgets: V150-V154
    writeLedIfChanged(150, bedroom1Fan.currentState, prevFanState);
    writeLedIfChanged(151, bedroom1Tube.currentState, prevTubeState);
    writeLedIfChanged(152, bedroom1Bulb.currentState, prevBulbState);
    writeLedIfChanged(153, bedroom1Socket.currentState, prevSocketState);
    writeLedIfChanged(154, bedroom1AC.currentState, prevACState);

    // 4. Timer/Schedule widgets: V100-V104
    syncTimerWidget(100, bedroom1Fan, blynkFanCache);
    syncTimerWidget(101, bedroom1Tube, blynkTubeCache);
    syncTimerWidget(102, bedroom1Bulb, blynkBulbCache);
    syncTimerWidget(103, bedroom1Socket, blynkSocketCache);
    syncTimerWidget(104, bedroom1AC, blynkACCache);

    blynkCacheInitialized = true;
}

BLYNK_CONNECTED()
{
    Serial.println("[BLYNK] Connected callback triggered");
    updateAllBlynkWidgets();
}

// ---------------------------------------------------------------
// initDashboard
// ---------------------------------------------------------------

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
}

// ---------------------------------------------------------------
// updateDashboard
// Blynk.run() every loop = free
// Terminal update every 60 seconds = 1 msg/min
// ---------------------------------------------------------------

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
