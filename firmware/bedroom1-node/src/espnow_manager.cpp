#include <WiFi.h>
#include <esp_now.h>

#include "espnow_manager.h"

#include "packet.h"
#include "commands.h"
#include "config.h"
#include "node_ids.h"
#include "mac_addresses.h"

#include "device_manager.h"
#include "motion_manager.h"
#include "environment_manager.h"

Packet txPacket;

unsigned long lastHeartbeat = 0;

static unsigned long lastMasterPacket = 0;

bool isMasterOnline()
{
    return
    (
        millis()
        -
        lastMasterPacket
    )
    <
    MASTER_TIMEOUT;
}

void onDataSent(
    const uint8_t *mac_addr,
    esp_now_send_status_t status)
{
    Serial.print("ESP-NOW Send Status: ");

    Serial.println(
        status == ESP_NOW_SEND_SUCCESS ?
        "Success" :
        "Failed"
    );
}

void onDataRecv(
    const uint8_t *mac,
    const uint8_t *incomingData,
    int len
)
{
    Serial.println("[ESP-NOW] Packet Received");

    Packet packet;

    memcpy(
        &packet,
        incomingData,
        sizeof(packet)
    );

    lastMasterPacket = millis();

    switch(packet.command)
    {
        case CMD_SET_DEVICE_STATE:

            setDeviceState(
                packet.deviceID,
                packet.state
            );
        Serial.print(
            "[CMD] SET_DEVICE_STATE Device="
        );

        Serial.print(
            packet.deviceID
        );

        Serial.print(" State=");

        Serial.println(
            packet.state
        );

            break;

        case CMD_SET_MODE:

            setDeviceMode(
                packet.deviceID,
                packet.mode
            );

            Serial.print(
                "[CMD] SET_MODE Device="
            );

            Serial.print(
                packet.deviceID
            );

            Serial.print(" Mode=");

            Serial.println(
                packet.mode
            );

            break;

        case CMD_FAN_SPEED:

            // Future fan controller

            break;
    }
}

void initEspNow()
{
    WiFi.mode(WIFI_STA);

    if(esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW Init Failed");

        return;
    }

    esp_now_register_send_cb(
        onDataSent
    );

    esp_now_register_recv_cb(
        onDataRecv
    );

    esp_now_peer_info_t peerInfo = {};

    memcpy(
        peerInfo.peer_addr,
        MASTER_MAC,
        6
    );

    peerInfo.channel = 0;

    peerInfo.encrypt = false;

    esp_now_add_peer(
        &peerInfo
    );

    Serial.println(
        "ESP-NOW Ready"
    );
}

void sendHeartbeat()
{
    if(
        millis() - lastHeartbeat <
        HEARTBEAT_INTERVAL
    )
    {
        return;
    }

    lastHeartbeat = millis();

    txPacket.senderNode =
        NODE_ID;

    txPacket.receiverNode =
        MASTER_NODE;

    txPacket.command =
        CMD_HEARTBEAT;

    txPacket.motionDetected =
        isMotionDetected();

    txPacket.brightness =
        environment.brightness;

    txPacket.uptime =
        millis();

    esp_now_send(
        MASTER_MAC,
        (uint8_t*)&txPacket,
        sizeof(txPacket)
    );

    Serial.print("[HEARTBEAT] ");

    Serial.print("Motion=");

    Serial.print(txPacket.motionDetected);

    Serial.print(" Brightness=");

    Serial.print(txPacket.brightness);

    Serial.print(" Uptime=");

    Serial.println(txPacket.uptime);
}

void sendMotionStatus(bool motion)
{
    txPacket.senderNode =
        NODE_ID;

    txPacket.receiverNode =
        MASTER_NODE;

    txPacket.command =
        CMD_MOTION;

    txPacket.motionDetected =
        motion;

    esp_now_send(
        MASTER_MAC,
        (uint8_t*)&txPacket,
        sizeof(txPacket)
    );
}

void sendEnvironmentStatus()
{
    txPacket.senderNode =
        NODE_ID;

    txPacket.receiverNode =
        MASTER_NODE;

    txPacket.command =
        CMD_ENVIRONMENT;

    txPacket.brightness =
        environment.brightness;

    esp_now_send(
        MASTER_MAC,
        (uint8_t*)&txPacket,
        sizeof(txPacket)
    );
}

void processIncomingPackets()
{
}