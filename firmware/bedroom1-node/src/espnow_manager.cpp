#include <WiFi.h>
#include <esp_now.h>

#include "espnow_manager.h"

#include "packet.h"
#include "commands.h"
#include "config.h"
#include "node_ids.h"
#include "mac_addresses.h"

Packet txPacket;

unsigned long lastHeartbeat = 0;

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

void initEspNow()
{
    WiFi.mode(WIFI_STA);

    if(esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW Init Failed");

        return;
    }

    esp_now_register_send_cb(onDataSent);

    esp_now_peer_info_t peerInfo = {};

    memcpy(
        peerInfo.peer_addr,
        MASTER_MAC,
        6
    );

    peerInfo.channel = 0;

    peerInfo.encrypt = false;

    esp_now_add_peer(&peerInfo);

    Serial.println("ESP-NOW Ready");
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

    txPacket.senderNode = NODE_ID;

    txPacket.receiverNode = MASTER_NODE;

    txPacket.command = CMD_HEARTBEAT;

    txPacket.uptime = millis();

    esp_now_send(
        MASTER_MAC,
        (uint8_t*)&txPacket,
        sizeof(txPacket)
    );

    Serial.println("Heartbeat Sent");
}

void sendMotionStatus()
{
    txPacket.senderNode = NODE_ID;

    txPacket.receiverNode = MASTER_NODE;

    txPacket.command = CMD_MOTION;

    esp_now_send(
        MASTER_MAC,
        (uint8_t*)&txPacket,
        sizeof(txPacket)
    );
}

void processIncomingPackets()
{
}