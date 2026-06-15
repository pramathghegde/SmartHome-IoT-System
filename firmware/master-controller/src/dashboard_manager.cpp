// ==============================================================
// dashboard_manager.cpp
// Blynk dashboard: receives commands, sends feedback & status
//
// DATASTREAM MAP:
//   V0   - V4    : Bedroom1 device MODE controls (Fan/Tube/Bulb/Socket/AC)
//   V100 - V104  : Bedroom1 device SCHEDULES (Fan/Tube/Bulb/Socket/AC)
//   V150 - V154  : Bedroom1 device STATE feedback (Fan/Tube/Bulb/Socket/AC)
//   V200         : Bedroom1 room status (0=Offline, 1=Online, 2=Motion)
//   V220         : Brightness
//   V221         : Temperature (future)
//   V222         : Humidity (future)
//   V240         : Master Online
//   V241         : WiFi RSSI
//   V242         : Current Time (string)
//   V243         : Uptime (seconds)
// ==============================================================

#include "secrets.h"            // BLYNK macros must come before BlynkSimpleEsp32
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <time.h>

#include "dashboard_manager.h"
#include "device_cache.h"
#include "state_manager.h"
#include "time_manager.h"
#include "modes.h"

#include <Arduino.h>

char auth[] = BLYNK_AUTH_TOKEN;

// ---------------------------------------------------------------
// V0-V4 : Bedroom1 Device Mode Controls
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
// V100-V104 : Bedroom1 Schedule Time Inputs
// Blynk Time Input widget sends: startTime, stopTime in seconds since midnight
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

    Serial.print("[BLYNK] FAN SCHEDULE -> ");
    Serial.print(bedroom1Fan.startHour);
    Serial.print(":");
    Serial.print(bedroom1Fan.startMinute);
    Serial.print(" - ");
    Serial.print(bedroom1Fan.stopHour);
    Serial.print(":");
    Serial.println(bedroom1Fan.stopMinute);
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

    Serial.print("[BLYNK] TUBE SCHEDULE -> ");
    Serial.print(bedroom1Tube.startHour);
    Serial.print(":");
    Serial.print(bedroom1Tube.startMinute);
    Serial.print(" - ");
    Serial.print(bedroom1Tube.stopHour);
    Serial.print(":");
    Serial.println(bedroom1Tube.stopMinute);
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

    Serial.print("[BLYNK] BULB SCHEDULE -> ");
    Serial.print(bedroom1Bulb.startHour);
    Serial.print(":");
    Serial.print(bedroom1Bulb.startMinute);
    Serial.print(" - ");
    Serial.print(bedroom1Bulb.stopHour);
    Serial.print(":");
    Serial.println(bedroom1Bulb.stopMinute);
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

    Serial.print("[BLYNK] SOCKET SCHEDULE -> ");
    Serial.print(bedroom1Socket.startHour);
    Serial.print(":");
    Serial.print(bedroom1Socket.startMinute);
    Serial.print(" - ");
    Serial.print(bedroom1Socket.stopHour);
    Serial.print(":");
    Serial.println(bedroom1Socket.stopMinute);
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

    Serial.print("[BLYNK] AC SCHEDULE -> ");
    Serial.print(bedroom1AC.startHour);
    Serial.print(":");
    Serial.print(bedroom1AC.startMinute);
    Serial.print(" - ");
    Serial.print(bedroom1AC.stopHour);
    Serial.print(":");
    Serial.println(bedroom1AC.stopMinute);
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
}

// ---------------------------------------------------------------
// updateDashboard
// Runs every 2 seconds
// Pushes: device states (V150-V154), room status (V200),
//         environment (V220-V222), system metrics (V240-V243)
// ---------------------------------------------------------------

void updateDashboard()
{
    Blynk.run();

    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate < 2000)
    {
        return;
    }

    lastUpdate = millis();

    // --- V150-V154 : Device State Feedback ---

    Blynk.virtualWrite(150, bedroom1Fan.currentState    ? 1 : 0);
    Blynk.virtualWrite(151, bedroom1Tube.currentState   ? 1 : 0);
    Blynk.virtualWrite(152, bedroom1Bulb.currentState   ? 1 : 0);
    Blynk.virtualWrite(153, bedroom1Socket.currentState ? 1 : 0);
    Blynk.virtualWrite(154, bedroom1AC.currentState     ? 1 : 0);

    // --- V200 : Bedroom1 Room Status (0=Offline, 1=Online, 2=Motion) ---

    int roomStatus =
        bedroom1.online ?
        (bedroom1.motionDetected ? 2 : 1) :
        0;

    Blynk.virtualWrite(200, roomStatus);

    Serial.print("[BLYNK] BEDROOM1 STATUS -> ");
    Serial.println(roomStatus);

    // --- V220 : Brightness ---

    Blynk.virtualWrite(220, bedroom1.brightness);

    // V221 Temperature, V222 Humidity - future (DHT/BME not yet implemented)

    // --- V240 : Master Online ---

    Blynk.virtualWrite(240, 1);

    // --- V241 : WiFi RSSI ---

    Blynk.virtualWrite(241, WiFi.RSSI());

    // --- V242 : Current Time String ---

    if (isTimeValid())
    {
        struct tm timeinfo;

        if (getLocalTime(&timeinfo))
        {
            char timeStr[10];

            strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);

            Blynk.virtualWrite(242, timeStr);
        }
    }

    // --- V243 : Uptime in seconds ---

    Blynk.virtualWrite(243, millis() / 1000);
}
