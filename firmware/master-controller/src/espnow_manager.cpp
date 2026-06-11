#include <WiFi.h>
#include <esp_now.h>

#include "espnow_manager.h"

#include "packet.h"

#include "commands.h"

#include "node_manager.h"

#include "mac_addresses.h"

void onDataRecv(
    const uint8_t *mac,
    const uint8_t *incomingData,
    int len
)
{
    Packet packet;

    memcpy(
        &packet,
        incomingData,
        sizeof(packet)
    );

    switch(packet.command)
    {
        case CMD_HEARTBEAT:

            updateNodeHeartbeat(
                packet.senderNode
            );

            Serial.println(
                "Heartbeat Received"
            );

            break;

        case CMD_MOTION:

            Serial.println(
                "Motion Received"
            );

            break;
    }
}

void initEspNow()
{
    WiFi.mode(WIFI_STA);

    esp_now_init();

    esp_now_register_recv_cb(
        onDataRecv
    );

    Serial.println(
        "Master ESP-NOW Ready"
    );
}