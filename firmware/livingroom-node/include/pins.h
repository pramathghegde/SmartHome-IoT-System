#pragma once

#define DHT_PIN            4
#define PIR1_PIN           0
#define PIR2_PIN           1

// TODO(Stage 6):
// Remove MOTION_SENSOR_PIN after all firmware references
// have been migrated to PIR1_PIN/PIR2_PIN.
#define MOTION_SENSOR_PIN  PIR1_PIN

// Living Room Relay Pins (GPIOs)
#define TUBE1_RELAY_PIN         5
#define TUBE2_RELAY_PIN         6
#define FAN_RELAY_PIN           7
#define EBIKE_RELAY_PIN         8
#define SOCKET_RELAY_PIN        9
#define OUTSIDE_BULB_RELAY_PIN  10
#define EXTRA1_RELAY_PIN        2
#define EXTRA2_RELAY_PIN        3

// Living Room Local Device IDs
#define TUBE1_DEVICE            1
#define TUBE2_DEVICE            2
#define FAN_DEVICE              3
#define EBIKE_DEVICE            4
#define SOCKET_DEVICE           5
#define OUTSIDE_BULB_DEVICE     6
#define EXTRA1_DEVICE           7
#define EXTRA2_DEVICE           8
