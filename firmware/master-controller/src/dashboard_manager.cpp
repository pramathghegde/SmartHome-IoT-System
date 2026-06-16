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
#include <WidgetTerminal.h>
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

WidgetTerminal terminal(200);

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

// ---------------------------------------------------------------
// LED state tracking
// ---------------------------------------------------------------

static bool prevFanState    = false;
static bool prevTubeState   = false;
static bool prevBulbState   = false;
static bool prevSocketState = false;
static bool prevACState     = false;

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
    int pin = -1;

    switch(deviceID)
    {
        case FAN_DEVICE:
            if (newState == prevFanState) return;
            prevFanState = newState;
            pin = 150;
            break;

        case TUBELIGHT_DEVICE:
            if (newState == prevTubeState) return;
            prevTubeState = newState;
            pin = 151;
            break;

        case BULB_DEVICE:
            if (newState == prevBulbState) return;
            prevBulbState = newState;
            pin = 152;
            break;

        case SOCKET_DEVICE:
            if (newState == prevSocketState) return;
            prevSocketState = newState;
            pin = 153;
            break;

        case AC_DEVICE:
            if (newState == prevACState) return;
            prevACState = newState;
            pin = 154;
            break;

        default:
            return;
    }

    Blynk.virtualWrite(pin, newState ? 255 : 0);

    Serial.print("[BLYNK] LED V");
    Serial.print(pin);
    Serial.print(" -> ");
    Serial.println(newState ? "ON" : "OFF");
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
        (bedroom1.brightness < DARK_THRESHOLD) ?
        "NIGHT" : "DAY  ";

    // Clear terminal before printing fresh status
    terminal.clear();

    // ---- HEADER ----
    terminal.println("==============================");
    terminal.println("   ADVAITA SMART HOME");
    terminal.print  ("   ");
    terminal.println(timeStr);
    terminal.println("==============================");

    // ---- ENVIRONMENT ----
    terminal.println();
    terminal.println("--- ENVIRONMENT ----------");
    terminal.println("  Temp  : --.- C");        // Future DHT/BME
    terminal.println("  Humid : --.- %");        // Future DHT/BME
    terminal.print  ("  Light : ");
    terminal.println(lightStatus);
    terminal.print  ("  LDR   : ");
    terminal.println(bedroom1.brightness);
    terminal.println("  Door  : ------");        // Future door lock

    // ---- ROOM STATUS ----
    terminal.println();
    terminal.println("--- ROOM STATUS ----------");

    terminal.print("  Bedroom1 : ");
    terminal.println(
        bedroom1.online ? "ONLINE " : "OFFLINE"
    );

    terminal.print("  Motion  : ");
    terminal.println(
        bedroom1.motionDetected ? "DETECTED" : "CLEAR   "
    );

    terminal.println("  Bedroom2 : -------");    // Future node
    terminal.println("  Hall     : -------");    // Future node
    terminal.println("  Kitchen  : -------");    // Future node

    // ---- APPLIANCES ----
    terminal.println();
    terminal.println("--- APPLIANCES -----------");

    terminal.print("  Fan    : ");
    terminal.print(stateStr(bedroom1Fan.currentState));
    terminal.print("  [");
    terminal.print(modeStr(bedroom1Fan.mode));
    terminal.println("]");

    terminal.print("  Tube   : ");
    terminal.print(stateStr(bedroom1Tube.currentState));
    terminal.print("  [");
    terminal.print(modeStr(bedroom1Tube.mode));
    terminal.println("]");

    terminal.print("  Bulb   : ");
    terminal.print(stateStr(bedroom1Bulb.currentState));
    terminal.print("  [");
    terminal.print(modeStr(bedroom1Bulb.mode));
    terminal.println("]");

    terminal.print("  Socket : ");
    terminal.print(stateStr(bedroom1Socket.currentState));
    terminal.print("  [");
    terminal.print(modeStr(bedroom1Socket.mode));
    terminal.println("]");

    terminal.print("  AC     : ");
    terminal.print(stateStr(bedroom1AC.currentState));
    terminal.print("  [");
    terminal.print(modeStr(bedroom1AC.mode));
    terminal.println("]");

    // ---- SYSTEM ----
    terminal.println();
    terminal.println("--- SYSTEM ---------------");

    terminal.print("  WiFi  : ");
    terminal.print(WiFi.RSSI());
    terminal.println(" dBm");

    terminal.print("  Uptime: ");
    terminal.print(millis() / 60000);
    terminal.println(" min");

    terminal.println("==============================");

    // Flush sends everything as one message
    terminal.flush();

    Serial.println("[BLYNK] Terminal status sent");
}

// ---------------------------------------------------------------
// V0-V4 : Mode Controls (Blynk → Master)
// ---------------------------------------------------------------

BLYNK_WRITE(V0)
{
    bedroom1Fan.mode = param.asInt();
    Serial.print("[BLYNK] FAN MODE -> ");
    Serial.println(bedroom1Fan.mode);
}

BLYNK_WRITE(V1)
{
    bedroom1Tube.mode = param.asInt();
    Serial.print("[BLYNK] TUBE MODE -> ");
    Serial.println(bedroom1Tube.mode);
}

BLYNK_WRITE(V2)
{
    bedroom1Bulb.mode = param.asInt();
    Serial.print("[BLYNK] BULB MODE -> ");
    Serial.println(bedroom1Bulb.mode);
}

BLYNK_WRITE(V3)
{
    bedroom1Socket.mode = param.asInt();
    Serial.print("[BLYNK] SOCKET MODE -> ");
    Serial.println(bedroom1Socket.mode);
}

BLYNK_WRITE(V4)
{
    bedroom1AC.mode = param.asInt();
    Serial.print("[BLYNK] AC MODE -> ");
    Serial.println(bedroom1AC.mode);
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
}

// ---------------------------------------------------------------
// initDashboard
// ---------------------------------------------------------------

void initDashboard()
{
    Blynk.begin(
        auth,
        WIFI_SSID,
        WIFI_PASSWORD
    );

    Serial.println("[BLYNK] Connected");

    // Reset all LEDs on boot
    Blynk.virtualWrite(150, 0);
    Blynk.virtualWrite(151, 0);
    Blynk.virtualWrite(152, 0);
    Blynk.virtualWrite(153, 0);
    Blynk.virtualWrite(154, 0);

    // Send first status immediately
    sendStatusToTerminal();
}

// ---------------------------------------------------------------
// updateDashboard
// Blynk.run() every loop = free
// Terminal update every 60 seconds = 1 msg/min
// ---------------------------------------------------------------

void updateDashboard()
{
    Blynk.run();

    static unsigned long lastSend = 0;

    if (millis() - lastSend >= 60000)
    {
        lastSend = millis();
        sendStatusToTerminal();
    }
}