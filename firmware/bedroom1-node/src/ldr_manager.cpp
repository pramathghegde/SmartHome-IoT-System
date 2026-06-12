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
    brightness = 1000;
}

int getBrightness()
{
    return brightness;
}