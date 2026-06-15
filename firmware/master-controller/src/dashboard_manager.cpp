// ==============================================================
// dashboard_manager.cpp
// LOW MESSAGE MODE - stays within Blynk free tier
//
// Messages OUT (Master → Blynk):
//   V200 : Single status summary string, sent every 60 seconds
//           = ~1 msg/min = ~43,200/month
//
// Messages IN (Blynk → Master):
//   V0-V4    : Mode controls (only on user press, ~10/day)
//   V100-V104: Schedules (only on user set)
//
// Total estimated: ~1-2 msg/min, ~50,000-60,000/month MAX
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

#include <Arduino.h>

char auth[] = BLYNK_AUTH_TOKEN;

// ---------------------------------------------------------------
// V0-V4 : Mode Controls (incoming only, zero outgoing messages)
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
// V100-V104 : Schedule Inputs (incoming only)
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
// Blynk.run() every loop - FREE (no messages, just keeps connection)
// Status push every 60 seconds - 1 message per minute
// ---------------------------------------------------------------

void updateDashboard()
{
    Blynk.run();

    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate < 60000)
    {
        return;
    }

    lastUpdate = millis();

    // Build one compact status string
    // Example: "B1:ON | MOT:Y | LDR:820 | F:ON T:OFF BL:ON SK:OFF AC:OFF"

    char status[80];

    snprintf(
        status,
        sizeof(status),
        "B1:%s|MOT:%s|LDR:%d|F:%s T:%s B:%s S:%s A:%s",
        bedroom1.online         ? "ON"  : "OFF",
        bedroom1.motionDetected ? "Y"   : "N",
        bedroom1.brightness,
        bedroom1Fan.currentState    ? "ON" : "OFF",
        bedroom1Tube.currentState   ? "ON" : "OFF",
        bedroom1Bulb.currentState   ? "ON" : "OFF",
        bedroom1Socket.currentState ? "ON" : "OFF",
        bedroom1AC.currentState     ? "ON" : "OFF"
    );

    Blynk.virtualWrite(200, status);

    Serial.print("[BLYNK] STATUS -> ");
    Serial.println(status);
}