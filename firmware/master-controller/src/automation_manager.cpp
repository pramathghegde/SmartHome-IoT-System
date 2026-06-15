#include "automation_manager.h"

#include "state_manager.h"

#include "device_cache.h"

#include "schedule_manager.h"

#include "espnow_manager.h"

#include "device_ids.h"

#include "node_ids.h"

#include "modes.h"

#include <Arduino.h>

void processDevice(
    DeviceConfig &device,
    uint8_t deviceID
)
{
    bool desiredState = false;

    switch(device.mode)
    {
        case MODE_OFF:

            desiredState = false;

            break;

        case MODE_ON:

            desiredState = true;

            break;

        case MODE_SCHEDULED:

            desiredState =
                isScheduleActive(
                    device
                );

            break;

        case MODE_AUTO:

            desiredState =
            (
                bedroom1.motionDetected
                &&
                bedroom1.brightness < 1200
            );

            break;
    }

    if(
        desiredState
        !=
        device.currentState
    )
    {
        device.currentState =
            desiredState;

        sendDeviceCommand(
            BEDROOM1_NODE,
            deviceID,
            desiredState
        );

        Serial.print(
            "[AUTOMATION] Device="
        );

        Serial.print(
            deviceID
        );

        Serial.print(
            " State="
        );

        Serial.println(
            desiredState
        );
    }
}

void runAutomation()
{
    if(!bedroom1.online)
    {
        return;
    }

    processDevice(
        bedroom1Fan,
        FAN_DEVICE
    );

    processDevice(
        bedroom1Tube,
        TUBELIGHT_DEVICE
    );

    processDevice(
        bedroom1Bulb,
        BULB_DEVICE
    );

    processDevice(
        bedroom1Socket,
        SOCKET_DEVICE
    );

    processDevice(
        bedroom1AC,
        AC_DEVICE
    );
}