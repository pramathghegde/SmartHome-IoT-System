#include "automation_manager.h"

#include "state_manager.h"

#include "espnow_manager.h"

#include "device_ids.h"

#include "node_ids.h"

void runAutomation()
{
    if(
        bedroom1.motionDetected
        &&
        bedroom1.brightness < 1200
    )
    {
        sendDeviceCommand(
            BEDROOM1_NODE,
            TUBELIGHT_DEVICE,
            true
        );

        sendDeviceCommand(
            BEDROOM1_NODE,
            BULB_DEVICE,
            true
        );
    }
    else
    {
        sendDeviceCommand(
            BEDROOM1_NODE,
            TUBELIGHT_DEVICE,
            false
        );

        sendDeviceCommand(
            BEDROOM1_NODE,
            BULB_DEVICE,
            false
        );
    }
}