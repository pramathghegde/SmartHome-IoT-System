#include "dht_manager.h"
#include "pins.h"
#include <DHT.h>

#define DHTTYPE DHT11

static DHT dht(DHT_PIN, DHTTYPE);

void initDHT()
{
    dht.begin();
}

float readTemperature()
{
    float t = dht.readTemperature();
    if (isnan(t))
    {
        return 0.0f;
    }
    return t;
}

float readHumidity()
{
    float h = dht.readHumidity();
    if (isnan(h))
    {
        return 0.0f;
    }
    return h;
}
