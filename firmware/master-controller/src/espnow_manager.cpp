#include <WiFi.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "espnow_manager.h"
#include "packet.h"
#include "commands.h"
#include "node_ids.h"
#include "mac_addresses.h"
#include "state_manager.h"
#include "node_manager.h"
#include "automation_manager.h"

#include <Arduino.h>

static volatile bool lastSendSuccess     = false;
static QueueHandle_t rxQueue             = nullptr;
static volatile uint32_t rxQueueOverflow = 0;
static volatile uint32_t rxQueueMaxUsed  = 0;
static volatile uint32_t rxPacketsQueued = 0;

static bool isValidCommand(uint8_t command)
{
    switch (command)
    {
        case CMD_HEARTBEAT:
        case CMD_MOTION:
        case CMD_ENVIRONMENT:
        case CMD_ACK:
            return true;

        default:
            return false;
    }
}

// ESP-NOW send callback - called from WiFi task context
void onDataSent(
    const uint8_t *mac_addr,
    esp_now_send_status_t status
)
{
    lastSendSuccess = (status == ESP_NOW_SEND_SUCCESS);
}

bool getLastSendSuccess()
{
    return lastSendSuccess;
}

// ESP-NOW receive callback - runs in ISR context, must be fast
void onDataRecv(
    const uint8_t *mac,
    const uint8_t *incomingData,
    int len
)
{
    if (len != sizeof(Packet))
    {
        return;
    }

    Packet packet;
    memcpy(&packet, incomingData, sizeof(packet));

    if (packet.receiverNode != MASTER_NODE)
    {
        return;
    }

    if (!isValidCommand(packet.command))
    {
        return;
    }

    if (rxQueue != nullptr)
    {
        if (xQueueSend(rxQueue, &packet, 0) != pdTRUE)
        {
            rxQueueOverflow++;
        }
        else
        {
            rxPacketsQueued++;

            UBaseType_t used = uxQueueMessagesWaiting(rxQueue);
            if (used > rxQueueMaxUsed)
            {
                rxQueueMaxUsed = used;
            }
        }
    }
}

void initEspNow()
{
    rxQueue = xQueueCreate(20, sizeof(Packet));

    if (rxQueue == nullptr)
    {
        Serial.println("[ESP-NOW] Failed to create RX queue");
        return;
    }

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("[ESP-NOW] Init Failed");
        return;
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    esp_now_peer_info_t peerInfo = {};
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Register Bedroom1 peer
    memcpy(peerInfo.peer_addr, BEDROOM1_MAC, 6);
    if (esp_now_add_peer(&peerInfo) == ESP_OK)
    {
        Serial.println("[ESP-NOW] Peer BEDROOM1 added");
    }
    else
    {
        Serial.println("[ESP-NOW] Failed to add peer BEDROOM1");
    }

    // Register LivingRoom peer
    memcpy(peerInfo.peer_addr, LIVINGROOM_MAC, 6);
    if (esp_now_add_peer(&peerInfo) == ESP_OK)
    {
        Serial.println("[ESP-NOW] Peer LIVINGROOM added");
    }
    else
    {
        Serial.println("[ESP-NOW] Failed to add peer LIVINGROOM");
    }

    // Register DiningHall peer
    memcpy(peerInfo.peer_addr, DININGHALL_MAC, 6);
    if (esp_now_add_peer(&peerInfo) == ESP_OK)
    {
        Serial.println("[ESP-NOW] Peer DININGHALL added");
    }
    else
    {
        Serial.println("[ESP-NOW] Failed to add peer DININGHALL");
    }

    Serial.println("[ESP-NOW] Ready");
}

void sendAck(uint8_t targetNode)
{
    Packet tx = {};

    tx.senderNode   = MASTER_NODE;
    tx.receiverNode = targetNode;
    tx.command      = CMD_ACK;
    tx.uptime       = millis() / 1000;

    const uint8_t* targetMac = nullptr;
    if (targetNode == BEDROOM1_NODE)
    {
        targetMac = BEDROOM1_MAC;
    }
    else if (targetNode == LIVINGROOM_NODE)
    {
        targetMac = LIVINGROOM_MAC;
    }
    else if (targetNode == DININGHALL_NODE)
    {
        targetMac = DININGHALL_MAC;
    }

    if (targetMac != nullptr)
    {
        esp_now_send(
            targetMac,
            (uint8_t*)&tx,
            sizeof(tx)
        );
    }
}

bool sendDeviceCommand(
    uint8_t targetNode,
    uint8_t deviceID,
    bool state
)
{
    Packet tx = {};

    tx.senderNode   = MASTER_NODE;
    tx.receiverNode = targetNode;
    tx.command      = CMD_SET_DEVICE_STATE;
    tx.deviceID     = deviceID;
    tx.state        = state;
    tx.uptime       = millis();

    lastSendSuccess = false;

    const uint8_t* targetMac = nullptr;
    if (targetNode == BEDROOM1_NODE)
    {
        targetMac = BEDROOM1_MAC;
    }
    else if (targetNode == LIVINGROOM_NODE)
    {
        targetMac = LIVINGROOM_MAC;
    }
    else if (targetNode == DININGHALL_NODE)
    {
        targetMac = DININGHALL_MAC;
    }

    if (targetMac == nullptr)
    {
        return false;
    }

    Serial.print("[ESP SEND] CMD_SET_DEVICE_STATE Node=");
    Serial.print(getNodeName(targetNode));
    Serial.print(" Device=");
    Serial.print(deviceID);
    Serial.print(" State=");
    Serial.println(state ? "ON" : "OFF");

    esp_err_t result = esp_now_send(
        targetMac,
        (uint8_t*)&tx,
        sizeof(tx)
    );

    Serial.print("[ESP SEND RESULT] Node=");
    Serial.print(getNodeName(targetNode));
    Serial.print(" Device=");
    Serial.print(deviceID);
    Serial.print(" State=");
    Serial.print(state ? "ON" : "OFF");
    Serial.println(result == ESP_OK ? " queued" : " queue-failed");

    return result == ESP_OK;
}


void processIncomingPackets()
{
    if (rxQueue == nullptr)
    {
        return;
    }

    // Throttled overflow logging
    if (rxQueueOverflow > 0)
    {
        static unsigned long lastOverflowLog = 0;

        if (millis() - lastOverflowLog > 5000)
        {
            Serial.print("[ESP-NOW] Dropped packets: ");
            Serial.println(rxQueueOverflow);
            rxQueueOverflow  = 0;
            lastOverflowLog  = millis();
        }
    }

    Packet packet;

    while (xQueueReceive(rxQueue, &packet, 0) == pdTRUE)
    {
        switch(packet.command)
        {
            case CMD_ACK:
                if (packet.senderNode == BEDROOM1_NODE || packet.senderNode == LIVINGROOM_NODE || packet.senderNode == DININGHALL_NODE)
                {
                    Serial.print("[COMMAND EXEC ACK] Node=");
                    Serial.print(getNodeName(packet.senderNode));
                    Serial.print(" Device=");
                    Serial.print(packet.deviceID);
                    Serial.print(" State=");
                    Serial.println(packet.state ? "ON" : "OFF");
                    
                    Serial.printf("[ACK RX] Device=%d State=%s\n", packet.deviceID, packet.state ? "ON" : "OFF");

                    confirmDeviceCommand(packet.senderNode, packet.deviceID, packet.state);
                }
                break;

            case CMD_HEARTBEAT:
                if (packet.senderNode == BEDROOM1_NODE)
                {
                    Serial.print("[HEARTBEAT] Node=BEDROOM1");
                    Serial.print(" Motion=");
                    Serial.print(packet.motionDetected);
                    Serial.print(" Brightness=");
                    Serial.print(packet.brightness);
                    Serial.print(" Uptime=");
                    Serial.print(packet.uptime);
                    Serial.print("s Boot=");
                    Serial.println(packet.bootCount);

                    if (
                        bedroom1.lastHeartbeat != 0 &&
                        (
                            packet.bootCount != bedroom1.lastBootCount ||
                            packet.uptime < bedroom1.lastNodeUptime
                        )
                    )
                    {
                        Serial.print("[NODE] ");
                        Serial.print(getNodeName(packet.senderNode));
                        Serial.println(" REBOOT DETECTED - forcing state sync");
                        bedroom1.syncPending = true;
                    }

                    bedroom1.brightness     = packet.brightness;
                    bedroom1.lastNodeUptime = packet.uptime;
                    bedroom1.lastBootCount  = packet.bootCount;

                    updateHeartbeat(packet.senderNode);

                    sendAck(BEDROOM1_NODE);
                }
                else if (packet.senderNode == LIVINGROOM_NODE)
                {
                    Serial.print("[HEARTBEAT] Node=LIVINGROOM");
                    Serial.print(" Motion=");
                    Serial.print(packet.motionDetected);
                    Serial.print(" Temp=");
                    Serial.print(packet.temperature, 1);
                    Serial.print(" Humidity=");
                    Serial.print(packet.humidity, 1);
                    Serial.print(" Uptime=");
                    Serial.print(packet.uptime);
                    Serial.print("s Boot=");
                    Serial.println(packet.bootCount);

                    if (
                        livingroom.lastHeartbeat != 0 &&
                        (
                            packet.bootCount != livingroom.lastBootCount ||
                            packet.uptime < livingroom.lastNodeUptime
                        )
                    )
                    {
                        Serial.print("[NODE] ");
                        Serial.print(getNodeName(packet.senderNode));
                        Serial.println(" REBOOT DETECTED - forcing state sync");
                        livingroom.syncPending = true;
                    }

                    if (!isnan(packet.temperature))
                    {
                        globalTemperature = packet.temperature;
                    }
                    if (!isnan(packet.humidity))
                    {
                        globalHumidity = packet.humidity;
                    }
                    livingroom.lastNodeUptime = packet.uptime;
                    livingroom.lastBootCount  = packet.bootCount;

                    updateHeartbeat(packet.senderNode);

                    sendAck(LIVINGROOM_NODE);
                }
                else if (packet.senderNode == DININGHALL_NODE)
                {
                    Serial.print("[HEARTBEAT] Node=DININGHALL");
                    Serial.print(" Motion=");
                    Serial.print(packet.motionDetected);
                    Serial.print(" Brightness=");
                    Serial.print(packet.brightness);
                    Serial.print(" Uptime=");
                    Serial.print(packet.uptime);
                    Serial.print("s Boot=");
                    Serial.println(packet.bootCount);

                    if (
                        dininghall.lastHeartbeat != 0 &&
                        (
                            packet.bootCount != dininghall.lastBootCount ||
                            packet.uptime < dininghall.lastNodeUptime
                        )
                    )
                    {
                        Serial.print("[NODE] ");
                        Serial.print(getNodeName(packet.senderNode));
                        Serial.println(" REBOOT DETECTED - forcing state sync");
                        dininghall.syncPending = true;
                    }

                    dininghall.lastNodeUptime = packet.uptime;
                    dininghall.lastBootCount  = packet.bootCount;

                    updateHeartbeat(packet.senderNode);

                    sendAck(DININGHALL_NODE);
                }
                break;

            case CMD_MOTION:
                if (packet.senderNode == BEDROOM1_NODE || packet.senderNode == LIVINGROOM_NODE || packet.senderNode == DININGHALL_NODE)
                {
                    if (packet.motionDetected)
                    {
                        recordMotionEvent(packet.senderNode);
                    }
                }
                break;

            case CMD_ENVIRONMENT:
                if (packet.senderNode == BEDROOM1_NODE)
                {
                    bedroom1.brightness = packet.brightness;
                }
                else if (packet.senderNode == LIVINGROOM_NODE)
                {
                    if (!isnan(packet.temperature))
                    {
                        globalTemperature = packet.temperature;
                    }
                    if (!isnan(packet.humidity))
                    {
                        globalHumidity = packet.humidity;
                    }
                }
                break;

            default:
                break;
        }
    }
}

void printEspNowDiagnostics()
{
    if (rxQueue == nullptr)
    {
        Serial.println("[QUEUE] ESP-NOW RX not created");
        return;
    }

    Serial.print("[QUEUE] ESP-NOW RX free=");
    Serial.print(uxQueueSpacesAvailable(rxQueue));
    Serial.print("/20 dropped=");
    Serial.print(rxQueueOverflow);
    Serial.print(" maxUsed=");
    Serial.print(rxQueueMaxUsed);
    Serial.print(" queued=");
    Serial.print(rxPacketsQueued);
    Serial.print(" lastSend=");
    Serial.println(lastSendSuccess ? "OK" : "FAIL_OR_NONE");
}
