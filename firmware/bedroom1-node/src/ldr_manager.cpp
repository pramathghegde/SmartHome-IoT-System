#include <Arduino.h>

#include "ldr_manager.h"
#include "pins.h"

static int brightness = 0;

void initLDR()
{
    pinMode(LDR_PIN, INPUT);
}

void updateLDR()
{
    brightness = analogRead(LDR_PIN);
}

int getBrightness()
{
    return brightness;
}