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

static bool auditedVirtualWriteSchedule(uint8_t pin, const DeviceConfig &device)
{
    if (!Blynk.connected())
    {
        Serial.print("[BLYNK] Skipped V");
        Serial.print(pin);
        Serial.println(" write; not connected");
        return false;
    }

    uint32_t startSec = device.startHour * 3600 + device.startMinute * 60;
    uint32_t stopSec = device.stopHour * 3600 + device.stopMinute * 60;
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

static DeviceConfig blynkFanCache       = {0xFF, false, 0xFF, 0xFF, 0xFF, 0xFF};
static DeviceConfig blynkTubeCache      = {0xFF, false, 0xFF, 0xFF, 0xFF, 0xFF};
static DeviceConfig blynkBulbCache      = {0xFF, false, 0xFF, 0xFF, 0xFF, 0xFF};
static DeviceConfig blynkSocketCache    = {0xFF, false, 0xFF, 0xFF, 0xFF, 0xFF};
static DeviceConfig blynkACCache        = {0xFF, false, 0xFF, 0xFF, 0xFF, 0xFF};

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

static bool writeScheduleIfChanged(uint8_t pin, const DeviceConfig &device, DeviceConfig &cachedDevice)
{
    bool changed = !blynkCacheInitialized ||
                   device.startHour != cachedDevice.startHour ||
                   device.startMinute != cachedDevice.startMinute ||
                   device.stopHour != cachedDevice.stopHour ||
                   device.stopMinute != cachedDevice.stopMinute;

    if (changed)
    {
        if (auditedVirtualWriteSchedule(pin, device))
        {
            cachedDevice.startHour = device.startHour;
            cachedDevice.startMinute = device.startMinute;
            cachedDevice.stopHour = device.stopHour;
            cachedDevice.stopMinute = device.stopMinute;
            return true;
        }
    }
    return false;
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

// ---------------------------------------------------------------
// V0-V4 : Mode Controls (Blynk → Master)
// ---------------------------------------------------------------

BLYNK_WRITE(V0)
{
    bedroom1Fan.mode = param.asInt();
    Serial.print("[BLYNK] FAN MODE -> ");
    Serial.println(bedroom1Fan.mode);
    saveSingleDevice("fan", bedroom1Fan);
    blynkFanModeCache = bedroom1Fan.mode;
}

BLYNK_WRITE(V1)
{
    bedroom1Tube.mode = param.asInt();
    Serial.print("[BLYNK] TUBE MODE -> ");
    Serial.println(bedroom1Tube.mode);
    saveSingleDevice("tube", bedroom1Tube);
    blynkTubeModeCache = bedroom1Tube.mode;
}

BLYNK_WRITE(V2)
{
    bedroom1Bulb.mode = param.asInt();
    Serial.print("[BLYNK] BULB MODE -> ");
    Serial.println(bedroom1Bulb.mode);
    saveSingleDevice("bulb", bedroom1Bulb);
    blynkBulbModeCache = bedroom1Bulb.mode;
}

BLYNK_WRITE(V3)
{
    bedroom1Socket.mode = param.asInt();
    Serial.print("[BLYNK] SOCKET MODE -> ");
    Serial.println(bedroom1Socket.mode);
    saveSingleDevice("sock", bedroom1Socket);
    blynkSocketModeCache = bedroom1Socket.mode;
}

BLYNK_WRITE(V4)
{
    bedroom1AC.mode = param.asInt();
    Serial.print("[BLYNK] AC MODE -> ");
    Serial.println(bedroom1AC.mode);
    saveSingleDevice("ac", bedroom1AC);
    blynkACModeCache = bedroom1AC.mode;
}

// ---------------------------------------------------------------
// V100-V104 : Schedule Inputs (Blynk → Master)
// ---------------------------------------------------------------

BLYNK_WRITE(V100)
{
    TimeInputParam t(param);
    if (t.hasStartTime())
    {
        bedroom1Fan.startHour   = t.getStartHour();
        bedroom1Fan.startMinute = t.getStartMinute();
    }
    if (t.hasStopTime())
    {
        bedroom1Fan.stopHour   = t.getStopHour();
        bedroom1Fan.stopMinute = t.getStopMinute();
    }
    Serial.println("[BLYNK] FAN SCHEDULE updated");
    saveSingleDevice("fan", bedroom1Fan);
    blynkFanCache.startHour = bedroom1Fan.startHour;
    blynkFanCache.startMinute = bedroom1Fan.startMinute;
    blynkFanCache.stopHour = bedroom1Fan.stopHour;
    blynkFanCache.stopMinute = bedroom1Fan.stopMinute;
}

BLYNK_WRITE(V101)
{
    TimeInputParam t(param);
    if (t.hasStartTime())
    {
        bedroom1Tube.startHour   = t.getStartHour();
        bedroom1Tube.startMinute = t.getStartMinute();
    }
    if (t.hasStopTime())
    {
        bedroom1Tube.stopHour   = t.getStopHour();
        bedroom1Tube.stopMinute = t.getStopMinute();
    }
    Serial.println("[BLYNK] TUBE SCHEDULE updated");
    saveSingleDevice("tube", bedroom1Tube);
    blynkTubeCache.startHour = bedroom1Tube.startHour;
    blynkTubeCache.startMinute = bedroom1Tube.startMinute;
    blynkTubeCache.stopHour = bedroom1Tube.stopHour;
    blynkTubeCache.stopMinute = bedroom1Tube.stopMinute;
}

BLYNK_WRITE(V102)
{
    TimeInputParam t(param);
    if (t.hasStartTime())
    {
        bedroom1Bulb.startHour   = t.getStartHour();
        bedroom1Bulb.startMinute = t.getStartMinute();
    }
    if (t.hasStopTime())
    {
        bedroom1Bulb.stopHour   = t.getStopHour();
        bedroom1Bulb.stopMinute = t.getStopMinute();
    }
    Serial.println("[BLYNK] BULB SCHEDULE updated");
    saveSingleDevice("bulb", bedroom1Bulb);
    blynkBulbCache.startHour = bedroom1Bulb.startHour;
    blynkBulbCache.startMinute = bedroom1Bulb.startMinute;
    blynkBulbCache.stopHour = bedroom1Bulb.stopHour;
    blynkBulbCache.stopMinute = bedroom1Bulb.stopMinute;
}

BLYNK_WRITE(V103)
{
    TimeInputParam t(param);
    if (t.hasStartTime())
    {
        bedroom1Socket.startHour   = t.getStartHour();
        bedroom1Socket.startMinute = t.getStartMinute();
    }
    if (t.hasStopTime())
    {
        bedroom1Socket.stopHour   = t.getStopHour();
        bedroom1Socket.stopMinute = t.getStopMinute();
    }
    Serial.println("[BLYNK] SOCKET SCHEDULE updated");
    saveSingleDevice("sock", bedroom1Socket);
    blynkSocketCache.startHour = bedroom1Socket.startHour;
    blynkSocketCache.startMinute = bedroom1Socket.startMinute;
    blynkSocketCache.stopHour = bedroom1Socket.stopHour;
    blynkSocketCache.stopMinute = bedroom1Socket.stopMinute;
}

BLYNK_WRITE(V104)
{
    TimeInputParam t(param);
    if (t.hasStartTime())
    {
        bedroom1AC.startHour   = t.getStartHour();
        bedroom1AC.startMinute = t.getStartMinute();
    }
    if (t.hasStopTime())
    {
        bedroom1AC.stopHour   = t.getStopHour();
        bedroom1AC.stopMinute = t.getStopMinute();
    }
    Serial.println("[BLYNK] AC SCHEDULE updated");
    saveSingleDevice("ac", bedroom1AC);
    blynkACCache.startHour = bedroom1AC.startHour;
    blynkACCache.startMinute = bedroom1AC.startMinute;
    blynkACCache.stopHour = bedroom1AC.stopHour;
    blynkACCache.stopMinute = bedroom1AC.stopMinute;
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

    // Mode widgets: V0-V4
    writeModeIfChanged(0, bedroom1Fan.mode, blynkFanModeCache);
    writeModeIfChanged(1, bedroom1Tube.mode, blynkTubeModeCache);
    writeModeIfChanged(2, bedroom1Bulb.mode, blynkBulbModeCache);
    writeModeIfChanged(3, bedroom1Socket.mode, blynkSocketModeCache);
    writeModeIfChanged(4, bedroom1AC.mode, blynkACModeCache);

    // Schedule widgets: V100-V104
    writeScheduleIfChanged(100, bedroom1Fan, blynkFanCache);
    writeScheduleIfChanged(101, bedroom1Tube, blynkTubeCache);
    writeScheduleIfChanged(102, bedroom1Bulb, blynkBulbCache);
    writeScheduleIfChanged(103, bedroom1Socket, blynkSocketCache);
    writeScheduleIfChanged(104, bedroom1AC, blynkACCache);

    // LDR Enable Switch Widget
    writeLdrEnableIfChanged(VPIN_B1_LDR_ENABLE_NUM, bedroom1LdrEnabled, blynkLdrEnableCache);

    // LED widgets: V150-V154
    writeLedIfChanged(150, bedroom1Fan.currentState, prevFanState);
    writeLedIfChanged(151, bedroom1Tube.currentState, prevTubeState);
    writeLedIfChanged(152, bedroom1Bulb.currentState, prevBulbState);
    writeLedIfChanged(153, bedroom1Socket.currentState, prevSocketState);
    writeLedIfChanged(154, bedroom1AC.currentState, prevACState);

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
