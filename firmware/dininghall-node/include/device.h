#pragma once

#include <stdint.h>

struct Device
{
    uint8_t id;

    bool state;

    uint8_t mode;

    uint8_t speed;
};
