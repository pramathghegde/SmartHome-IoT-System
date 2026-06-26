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

    memcpy(peerInfo.peer_addr, BEDROOM1_MAC, 6);

    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_now_add_peer(&peerInfo);

    Serial.println("[ESP-NOW] Ready");
}

void sendAck(uint8_t targetNode)
{
    Packet tx = {};

    tx.senderNode   = MASTER_NODE;
    tx.receiverNode = targetNode;
    tx.command      = CMD_ACK;
    tx.uptime       = millis() / 1000;

    if (targetNode == BEDROOM1_NODE)
    {
        esp_now_send(
            BEDROOM1_MAC,
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

    if (targetNode == BEDROOM1_NODE)
    {
        // FIX: Removed blocking while(delay(1)) loop.
        // The old code blocked for up to 50ms per device = 250ms total
        // per automation cycle when all 5 devices change state.
        // This starved Blynk.run() and ArduinoOTA.handle(),
        // causing Blynk disconnects and WiFi reconnects which
        // disrupted ESP-NOW channel, causing the 5-10 minute blackouts.
        //
        // Now: fire-and-check. esp_now_send() queues the packet.
        // The send callback (onDataSent) updates lastSendSuccess
        // asynchronously. We check it on the NEXT automation cycle.
        // This is non-blocking and safe.

        Serial.print("[ESP SEND] CMD_SET_DEVICE_STATE Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.println(state ? "ON" : "OFF");

        esp_err_t result = esp_now_send(
            BEDROOM1_MAC,
            (uint8_t*)&tx,
            sizeof(tx)
        );

        Serial.print("[ESP SEND RESULT] Device=");
        Serial.print(deviceID);
        Serial.print(" State=");
        Serial.print(state ? "ON" : "OFF");
        Serial.println(result == ESP_OK ? " queued" : " queue-failed");

        return result == ESP_OK;
    }

    return false;
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
        Serial.print("[QUEUE POP] Master cmd=");
        Serial.print(packet.command);
        Serial.print(" waiting=");
        Serial.println(uxQueueMessagesWaiting(rxQueue));

        switch(packet.command)
        {
            case CMD_ACK:
                if (packet.senderNode == BEDROOM1_NODE)
                {
                    Serial.print("[COMMAND EXEC ACK] Device=");
                    Serial.print(packet.deviceID);
                    Serial.print(" State=");
                    Serial.println(packet.state ? "ON" : "OFF");

                    confirmDeviceCommand(packet.deviceID, packet.state);
                }
                break;

            case CMD_HEARTBEAT:
                Serial.print("[HEARTBEAT] Node=");
                Serial.print(packet.senderNode);
                Serial.print(" Motion=");
                Serial.print(packet.motionDetected);
                Serial.print(" Bright=");
                Serial.print(packet.brightness);
                Serial.print(" Uptime=");
                Serial.print(packet.uptime);
                Serial.print("s Boot=");
                Serial.println(packet.bootCount);

                if (packet.senderNode == BEDROOM1_NODE)
                {
                    if (
                        bedroom1.lastHeartbeat != 0 &&
                        (
                            packet.bootCount != bedroom1.lastBootCount ||
                            packet.uptime < bedroom1.lastNodeUptime
                        )
                    )
                    {
                        Serial.println("[NODE] BEDROOM1 REBOOT DETECTED - forcing state sync");
                        bedroom1.syncPending = true;
                    }

                    bedroom1.brightness     = packet.brightness;
                    bedroom1.lastNodeUptime = packet.uptime;
                    bedroom1.lastBootCount  = packet.bootCount;

                    updateHeartbeat(packet.senderNode);

                    sendAck(BEDROOM1_NODE);
                }
                break;

            case CMD_MOTION:
                if (packet.senderNode == BEDROOM1_NODE)
                {
                    if (packet.motionDetected)
                    {
                        recordMotionEvent();
                    }
                }
                break;

            case CMD_ENVIRONMENT:
                if (packet.senderNode == BEDROOM1_NODE)
                {
                    bedroom1.brightness = packet.brightness;
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
