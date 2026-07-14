# 🏠 Advaita Smart Home IoT

<div align="center">

![Platform](https://img.shields.io/badge/Platform-ESP32--C3%20%7C%20ESP8266-blue)
![Framework](https://img.shields.io/badge/Framework-Arduino-green)
![IDE](https://img.shields.io/badge/IDE-PlatformIO-orange)
![Communication](https://img.shields.io/badge/Communication-ESP--NOW-red)
![Mobile App](https://img.shields.io/badge/App-Blynk-purple)
![OTA](https://img.shields.io/badge/OTA-Supported-success)
![Status](https://img.shields.io/badge/Status-Under%20Development-yellow)

**A scalable, professional-grade Smart Home Automation System built using ESP32-C3, ESP8266, ESP-NOW, OTA, and Blynk.**

Designed for real-world deployment, learning embedded systems, and demonstrating production-level IoT architecture.

</div>

---

# 📖 Overview

**Advaita Smart Home IoT** is a distributed home automation system built from scratch using low-cost ESP microcontrollers.

Unlike many hobby projects where every microcontroller independently connects to Wi-Fi and the cloud, this project follows a **Master–Node architecture**.

A single **Master Controller** manages:

- Home automation
- Scheduling
- Mobile app communication
- OTA updates
- Time synchronization
- Device configuration
- Automation logic

while lightweight **Room Nodes** only perform hardware interfacing:

- Reading sensors
- Controlling relays
- Reporting events
- Executing commands

This architecture provides excellent scalability, reliability, and easier maintenance for larger homes.

---

# 🎯 Project Goals

This project has four major objectives:

### 🏡 1. Real Smart Home

Develop a reliable home automation system for everyday use.

Features include:

- Smart Lighting
- Smart Fans
- Smart Sockets
- Smart AC Control
- Motion-based Automation
- Ambient Light Automation
- Mobile Control
- OTA Firmware Updates
- Local Automation without Internet

---

### 📚 2. Learning Embedded Systems

The project serves as a complete learning platform for:

- ESP32-C3 Development
- ESP8266 Development
- ESP-NOW
- OTA
- PlatformIO
- Git
- Embedded C++
- Modular Firmware Design

---

### 💼 3. Portfolio Project

The goal is to demonstrate knowledge in:

- Embedded Systems
- IoT
- Distributed Systems
- Firmware Architecture
- Wireless Communication
- Software Design
- Version Control

---

### 🔄 4. Reusable Architecture

The firmware is intentionally designed to scale from:

```
1 Room
    ↓
2 BHK
    ↓
3 BHK
    ↓
Entire House
```

without redesigning the architecture.

---

# ✨ Features

## 🔥 Current Features

- ✅ Master–Node Architecture
- ✅ ESP-NOW Communication
- ✅ OTA Updates
- ✅ Motion-based Automation
- ✅ Ambient Light Detection (LDR)
- ✅ Manual ON/OFF Control
- ✅ AUTO Mode
- ✅ SCHEDULE Mode
- ✅ Persistent Configuration (NVS)
- ✅ Dual Schedule System
- ✅ Per-Room Motion Timeout
- ✅ Per-Room LDR Enable
- ✅ Reliable ACK-based Communication
- ✅ Automatic State Synchronization
- ✅ Brownout Recovery
- ✅ Node Reboot Recovery
- ✅ Device State Feedback
- ✅ Blynk Dashboard Integration
- ✅ NTP Time Synchronization
- ✅ Production-grade Logging

---

# 🚀 Upcoming Features

- ⏳ Bedroom 2 Node
- ⏳ Hall Node
- ⏳ Kitchen Node
- ⏳ Outdoor Node
- ⏳ Water Tank Automation
- ⏳ Smart Door Lock
- ⏳ Temperature & Humidity Monitoring
- ⏳ Energy Monitoring
- ⏳ AI-based Automation
- ⏳ Voice Assistant Integration
- ⏳ Google Home
- ⏳ Alexa
- ⏳ Web Dashboard
- ⏳ Scene Management
- ⏳ Notification System

---

# 🏗️ System Architecture

```
                    Internet
                        │
                        │
                  Blynk Cloud
                        │
                        │
                Wi-Fi Network
                        │
                        │
               ┌────────────────┐
               │ Master Controller│
               │    ESP8266       │
               └────────┬─────────┘
                        │
                 ESP-NOW Network
                        │
 ───────────────────────┼────────────────────────
        │               │               │
        │               │               │
 ┌────────────┐  ┌────────────┐  ┌────────────┐
 │ Bedroom 1  │  │ Bedroom 2  │  │ Hall Node  │
 │ ESP32-C3   │  │ ESP32-C3   │  │ ESP32-C3   │
 └────────────┘  └────────────┘  └────────────┘
        │               │               │
        │               │               │
     Sensors         Sensors         Sensors
     Relays          Relays          Relays

```

---

# 🧠 Design Philosophy

Unlike traditional IoT systems where each room directly communicates with the cloud, this project follows a layered architecture.

```
User
   │
   ▼
Blynk Mobile App
   │
   ▼
Master Controller
   │
   ▼
ESP-NOW
   │
   ▼
Room Nodes
   │
   ▼
Sensors & Relays
```

The Master Controller is the **only decision maker**.

Room Nodes are intentionally designed as **pure I/O devices**, making the system easier to maintain, debug, and scale.

---

# 🌟 Why This Architecture?

## ❌ Traditional Approach

```
Room 1 → Wi-Fi
Room 2 → Wi-Fi
Room 3 → Wi-Fi
Room 4 → Wi-Fi

All communicate directly with cloud
```

Problems:

- High Wi-Fi traffic
- Harder OTA management
- Difficult synchronization
- Duplicate automation logic
- Poor scalability

---

## ✅ Advaita Smart Home Architecture

```
                Wi-Fi
                  │
                  ▼
          Master Controller
                  │
          ESP-NOW Network
                  │
─────────────────────────────────
Bedroom
Hall
Kitchen
Outdoor
Tank
Door Lock
─────────────────────────────────
```

Advantages:

- Centralized automation
- Cleaner firmware
- Reduced Wi-Fi traffic
- Faster communication
- Easier debugging
- Easier OTA updates
- Excellent scalability
- Professional architecture

---

# 🎥 Project Demonstration

> **Coming Soon**

The repository will include:

- 📸 Hardware Images
- 📸 Wiring Images
- 📸 Blynk Dashboard
- 🎥 Live Demonstration
- 🎥 OTA Update Demo
- 🎥 Motion Automation Demo
- 🎥 Complete Smart Home Walkthrough

---

# ⭐ Project Highlights

- 🚀 Distributed IoT Architecture
- 🛰️ ESP-NOW Communication
- 📱 Blynk Integration
- ⚡ OTA Updates
- 🧠 Centralized Automation Engine
- 💾 Persistent Configuration (NVS)
- 🔄 Reliable ACK-based Communication
- 🏠 Multi-room Scalable Design
- 🛠️ Modular Firmware Architecture
- 📦 PlatformIO Based Development

---


# 📦 Repository Structure

The project follows a modular and scalable repository layout. Each firmware project is completely independent while sharing a common communication protocol through the `shared` directory.

```text
Advaita Smart Home IoT/
│
├── firmware/
│   │
│   ├── master-controller/
│   │   │
│   │   ├── include/
│   │   ├── src/
│   │   ├── platformio.ini
│   │   └── .gitignore
│   │
│   ├── bedroom1-node/
│   │   │
│   │   ├── include/
│   │   ├── src/
│   │   ├── platformio.ini
│   │   └── .gitignore
│   │
│   └── shared/
│
├── docs/
│
├── hardware/
│
├── images/
│
├── LICENSE
│
└── README.md
```

---

# 📁 Repository Overview

| Folder | Purpose |
|---------|----------|
| firmware | Contains all embedded firmware |
| master-controller | Master ESP8266 firmware |
| bedroom1-node | Bedroom ESP32-C3 firmware |
| shared | Common protocol definitions |
| docs | Project documentation |
| hardware | Circuit diagrams & PCB files |
| images | Screenshots and project photos |

---

# 📂 Shared Folder

The **shared** folder contains protocol definitions used by every firmware project.

This guarantees that all devices communicate using the exact same packet definitions.

Current shared files:

```text
shared/
│
├── commands.h
├── device_ids.h
├── mac_addresses.h
├── modes.h
├── node_ids.h
└── packet.h
```

---

## commands.h

Defines every command exchanged over ESP-NOW.

Current commands:

```cpp
enum CommandType
{
    CMD_ON = 1,
    CMD_OFF,
    CMD_SET_MODE,
    CMD_STATUS,
    CMD_HEARTBEAT,
    CMD_MOTION,
    CMD_ENVIRONMENT,
    CMD_FAN_SPEED,
    CMD_SET_DEVICE_STATE,
    CMD_ACK
};
```

Future commands:

- Door Lock
- Tank Level
- OTA Status
- Energy Monitoring
- Notifications

---

## device_ids.h

Defines appliance identifiers.

Current:

```cpp
#define FAN_DEVICE         1
#define TUBELIGHT_DEVICE   2
#define BULB_DEVICE        3
#define SOCKET_DEVICE      4
#define AC_DEVICE          5
```

Future:

```text
BEDROOM1_FAN

BEDROOM2_FAN

HALL_FAN

KITCHEN_FAN

...
```

---

## node_ids.h

Defines every ESP node.

Current:

```cpp
#define MASTER_NODE       1
#define BEDROOM1_NODE     2
```

Future:

```text
BEDROOM2_NODE

HALL_NODE

KITCHEN_NODE

OUTDOOR_NODE

TANK_NODE

DOORLOCK_NODE
```

---

## modes.h

Every appliance operates in exactly one mode.

```cpp
enum DeviceMode
{
    MODE_OFF = 0,
    MODE_ON = 1,
    MODE_AUTO = 2,
    MODE_SCHEDULED = 3
};
```

### MODE_OFF

- Always OFF
- Ignores motion
- Ignores schedules
- Ignores LDR

---

### MODE_ON

- Always ON
- Ignores motion
- Ignores schedules
- Ignores LDR

---

### MODE_AUTO

Automation controlled.

Uses:

- Motion Sensor
- LDR (optional)
- AUTO Schedule
- Motion Timeout

---

### MODE_SCHEDULED

Time based only.

Uses:

- SCHEDULE Timer

Ignores:

- Motion
- LDR

---

## packet.h

The entire communication protocol uses one common packet.

```cpp
struct Packet
{
    uint8_t senderNode;
    uint8_t receiverNode;

    uint8_t command;

    uint8_t deviceID;

    uint8_t state;

    uint8_t mode;

    uint8_t fanSpeed;

    bool motionDetected;

    int brightness;

    uint32_t uptime;
};
```

The packet size is intentionally fixed.

Future protocol changes should avoid increasing packet size unless absolutely necessary.

---

## mac_addresses.h

Stores every node MAC address.

Example:

```cpp
MASTER_MAC

BEDROOM1_MAC

BEDROOM2_MAC

HALL_MAC

...
```

This centralizes peer management.

---

# 🖥️ Master Controller

## Hardware

Board:

**ESP8266**

Responsibilities:

- Wi-Fi Connection
- OTA
- ESP-NOW
- Blynk
- Automation Engine
- NTP
- Scheduling
- Persistent Storage
- Diagnostics

---

## Master Firmware Structure

```text
master-controller/

include/

automation_manager.h

dashboard_manager.h

device_cache.h

espnow_manager.h

node_manager.h

ota_manager.h

schedule_manager.h

state_manager.h

time_manager.h

config.h

secrets.h


src/

automation_manager.cpp

dashboard_manager.cpp

device_cache.cpp

espnow_manager.cpp

node_manager.cpp

ota_manager.cpp

schedule_manager.cpp

state_manager.cpp

time_manager.cpp

main.cpp
```

---

# 🛏️ Bedroom Node

## Hardware

Board:

ESP32-C3 Mini

Responsibilities:

- Motion Sensor
- LDR
- Relay Control
- OTA
- ESP-NOW
- Heartbeats
- ACK

The Bedroom node never performs automation.

It simply executes commands from the Master.

---

## Bedroom Firmware Structure

```text
bedroom1-node/

include/

config.h

pins.h

device_manager.h

relay_manager.h

motion_manager.h

environment_manager.h

espnow_manager.h

ota_manager.h

secrets.h


src/

device_manager.cpp

relay_manager.cpp

motion_manager.cpp

environment_manager.cpp

espnow_manager.cpp

ota_manager.cpp

main.cpp
```

---

# ⚙️ Hardware Components

## Master Controller

| Component | Purpose |
|------------|----------|
| ESP8266 | Central Controller |

---

## Bedroom Node

| Component | Purpose |
|------------|----------|
| ESP32-C3 Mini | Room Controller |
| Motion Sensor | Human Motion Detection |
| LDR | Ambient Light Detection |
| 5-Channel Relay | Appliance Control |

---

## Controlled Appliances

- Ceiling Fan
- Tube Light
- LED Bulb
- Socket
- Air Conditioner

---

# 🔌 Power Architecture

The project uses a distributed power design.

Each room node has its own regulated 5V power supply.

Recommendations:

- Good quality 5V Mobile Charger
- Buck Converter (preferred for permanent installation)

Avoid cheap adapters because they can cause:

- Brownout resets
- Random ESP reboots
- Wi-Fi instability

---

# 📡 Communication Architecture

```
             User

              │

          Blynk App

              │

           Internet

              │

        Master ESP8266

              │

          ESP-NOW

              │

──────────────────────────────────

Bedroom Node

Hall Node

Kitchen Node

Outdoor Node

Door Lock Node

Tank Node

──────────────────────────────────

              │

           Relays

              │

         Home Appliances
```

---

# 🧠 Automation Philosophy

The project follows one important rule:

> **Only the Master Controller makes automation decisions.**

Room Nodes:

✅ Read sensors

✅ Execute commands

❌ Never perform automation

❌ Never evaluate schedules

❌ Never calculate motion timeout

This keeps all decision-making centralized and scalable.

---

# 🔄 Communication Flow

### User Command

```text
User

↓

Blynk

↓

Master

↓

ESP-NOW

↓

Bedroom Node

↓

Relay

↓

ACK

↓

Master

↓

Update Dashboard
```

---

### Motion Event

```text
Motion Sensor

↓

Bedroom Node

↓

ESP-NOW

↓

Master

↓

Automation Engine

↓

Decision

↓

Bedroom Node

↓

Relay
```

---

# 📦 Design Principles

The firmware is built around modular managers.

Each subsystem has its own responsibility:

- OTA Manager
- ESP-NOW Manager
- Dashboard Manager
- Automation Manager
- Device Cache
- Schedule Manager
- Motion Manager
- Relay Manager

Benefits:

- Easier debugging
- Better scalability
- Cleaner code
- Independent testing
- Future extensibility

---

# ⚙️ Software Setup Guide

This section explains how to set up the complete Smart Home IoT project from scratch.

---

# 🛠 Development Environment

The project is developed using:

| Software | Version |
|----------|----------|
| VS Code | Latest |
| PlatformIO | Latest |
| Arduino Framework | Latest |
| Git | Latest |

---

# 💻 Recommended IDE

Visual Studio Code

Required Extensions:

- PlatformIO IDE
- C/C++
- GitLens
- Error Lens
- Better Comments

---

# 📂 PlatformIO Configuration

## ESP32-C3 Room Node

```ini
[env]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino

monitor_speed = 115200

build_flags =
    -I../shared
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1

[env:esp32c3_serial]
upload_protocol = esptool

[env:esp32c3_ota]
upload_protocol = espota

[platformio]
default_envs = esp32c3_serial
```

---

## ESP8266 Master Controller

```ini
[env]
platform = espressif8266
board = esp12e
framework = arduino

monitor_speed = 115200

build_flags =
    -I../shared

[env:esp8266_serial]
upload_protocol = esptool

[env:esp8266_ota]
upload_protocol = espota
```

---

# 📡 OTA Configuration

The project supports wireless firmware updates.

OTA Hostnames:

```
MASTER.local

BEDROOM1.local

BEDROOM2.local

HALL.local

KITCHEN.local
```

Advantages:

- No need to remember IP addresses
- Faster development
- Easy deployment

---

# 🔐 OTA Authentication

Each node is protected using an OTA password.

Example:

```cpp
#define OTA_PASSWORD "123456"
```

PlatformIO OTA Upload:

```ini
upload_flags =
    --auth=123456
```

---

# 📱 Blynk Dashboard

Each room has its own dedicated **Blynk Template**.

Example:

```
Advaita Bedroom1

Advaita Hall

Advaita Kitchen

Advaita Outdoor
```

This keeps the dashboard clean and scalable.

---

# 📊 Bedroom Dashboard Layout

Each appliance contains:

- Mode Selector
- Timer Widget
- Status LED

Room Controls:

- Motion Timeout
- LDR Enable

Room Status:

- Motion
- Brightness
- Node Status

Terminal:

- System Logs

---

# 🎛 Appliance Modes

Each appliance supports four modes.

---

## OFF

Always OFF.

Ignores:

- Motion
- LDR
- Timer

---

## ON

Always ON.

Ignores:

- Motion
- LDR
- Timer

---

## AUTO

Uses:

- Motion Sensor
- Motion Timeout
- LDR (optional)
- AUTO Schedule

---

## SCHEDULED

Uses:

- SCHEDULE Timer

Ignores:

- Motion
- LDR

---

# 🌙 LDR Enable

Every room has one switch.

```
LDR Enable
```

When ON

```
Motion

AND

Darkness
```

controls lighting.

When OFF

```
Motion Only
```

controls lighting.

Only affects:

- Tube Light
- Bulb

Fan, Socket and AC ignore it.

---

# ⏱ Motion Timeout

Every room has one configurable timeout.

Configured from Blynk.

Format:

```
HH:MM:SS
```

Example:

```
00:05:00
```

The timeout is:

- Stored in NVS
- Restored after reboot
- Used only by the Master

---

# 📅 Dual Schedule Architecture

Every appliance stores two completely independent schedules.

## AUTO Schedule

Used only when the appliance is in AUTO mode.

---

## SCHEDULE Schedule

Used only when the appliance is in SCHEDULED mode.

---

Only **one Timer Widget** exists in Blynk.

Depending on the selected mode:

```
AUTO Mode

↓

Timer displays AUTO Schedule
```

or

```
SCHEDULE Mode

↓

Timer displays SCHEDULE Schedule
```

This allows one widget to manage two independent schedules.

---

# 💾 Persistent Configuration (NVS)

The Master automatically stores:

- Device Mode
- AUTO Schedule
- SCHEDULE Schedule
- Motion Timeout
- LDR Enable

After power loss:

```
Power ON

↓

Load NVS

↓

Restore Configuration

↓

Reconnect Blynk

↓

Synchronize Dashboard

↓

Resume Automation
```

No manual reconfiguration is required.

---

# 📶 ESP-NOW Communication

The project uses ESP-NOW for node communication.

Master sends:

- Relay Commands

Bedroom sends:

- Heartbeats
- Motion Events
- ACK Packets

---

## Reliable ACK System

Every command follows:

```
Master

↓

Send Command

↓

Bedroom Executes

↓

Send ACK

↓

Master Confirms
```

State changes occur only after a valid ACK.

---

# 🔄 Automatic State Recovery

If a Bedroom node restarts:

```
Bedroom Boot

↓

Heartbeat

↓

Master Detects Reboot

↓

Synchronize All Relay States

↓

Bedroom Restores Outputs
```

No manual intervention required.

---

# 📋 Serial Monitor

The firmware provides categorized logs.

Examples:

```
[NODE]

[HEARTBEAT]

[ESP-NOW]

[BLYNK]

[OTA]

[AUTOMATION]

[DIAG]
```

This makes debugging much easier.

---

# 🚀 Flashing Firmware

## Master

```bash
pio run -e esp8266_serial -t upload
```

OTA:

```bash
pio run -e esp8266_ota -t upload
```

---

## Bedroom

```bash
pio run -e esp32c3_serial -t upload
```

OTA:

```bash
pio run -e esp32c3_ota -t upload
```

---

# 🧪 Recommended Testing

Before installing inside switchboards:

- OTA Testing
- ESP-NOW Testing
- Motion Detection
- Relay Testing
- Brownout Testing
- Long-duration stability testing (24–48 hours)

Only install after successful bench testing.

---

# 🚀 Future Roadmap

Advaita Smart Home IoT is designed as a long-term project. The current implementation focuses on a single room (Bedroom1), but the architecture is intentionally built to scale into a complete smart home ecosystem.

---

# 🏠 Planned Room Nodes

The current architecture can be expanded without redesign.

Future nodes include:

- ✅ Bedroom 1
- ⏳ Bedroom 2
- ⏳ Hall / Living Room
- ⏳ Kitchen
- ⏳ Outdoor
- ⏳ Water Tank
- ⏳ Door Lock
- ⏳ Garage
- ⏳ Garden
- ⏳ Energy Monitoring

Each node will reuse the same firmware architecture with only configuration changes.

---

# 🤖 AI Features

Future AI integrations include:

- AI Occupancy Prediction
- AI Energy Optimization
- Adaptive Lighting
- Smart Fan Speed Control
- Usage Pattern Learning
- Electricity Consumption Prediction
- Weather-aware Automation
- Automatic Scene Recommendations

---

# 🌐 Future Integrations

- Google Home
- Amazon Alexa
- MQTT
- Home Assistant
- Web Dashboard
- REST API
- Voice Commands
- Telegram Notifications
- WhatsApp Notifications
- Email Alerts

---

# 📈 Future Hardware

Planned hardware additions:

- DHT22 / BME280
- Water Level Sensors
- Smart Door Lock
- Energy Meter
- PIR/mmWave Sensors
- Smoke Detector
- Gas Leakage Sensor
- Rain Sensor
- Soil Moisture Sensor
- Servo-based Door Lock
- RGB Lighting
- Motorized Curtains

---

# 🔒 Safety Features

Future safety improvements:

- Overload Detection
- Power Failure Notifications
- Relay Failure Detection
- Node Health Monitoring
- Automatic Watchdog Recovery
- OTA Rollback
- Secure OTA Authentication
- ESP-NOW Encryption

---

# 🧪 Testing Strategy

Every firmware update follows:

1. Build Verification
2. Static Analysis
3. Serial Testing
4. ESP-NOW Testing
5. OTA Testing
6. Relay Testing
7. Motion Testing
8. 24–48 Hour Stability Test
9. Brownout Recovery Test
10. Node Reboot Recovery Test

Only after all tests pass should firmware be installed inside wall switchboards.

---

# 🌳 Git Workflow

The project follows a professional Git workflow.

## Branch Structure

```text
main

master-controller

bedroom1-node

future:

bedroom2-node

hall-node

kitchen-node

outdoor-node

tank-node

doorlock-node
```

---

## Development Workflow

```text
Create Feature

↓

Checkout Branch

↓

Develop

↓

Compile

↓

Hardware Test

↓

Commit

↓

Push

↓

Merge
```

---

## Commit Style

Example commits:

```text
Added OTA Support

Implemented ESP-NOW Communication

Implemented Reliable ACK System

Added Persistent Configuration

Added Per-Room LDR Enable

Implemented Motion Timeout

Implemented Dual Schedule Architecture
```

Commits should be small, descriptive, and focused on one feature.

---

# 📖 Coding Standards

The project follows a modular architecture.

## Naming

Files:

```
snake_case
```

Examples:

```
automation_manager.cpp

dashboard_manager.cpp

espnow_manager.cpp
```

---

Functions:

```
camelCase
```

Examples:

```cpp
sendHeartbeat()

processIncomingPackets()

updateMotionTimeout()

saveDeviceConfiguration()
```

---

Constants:

```
UPPER_CASE
```

Examples:

```cpp
MASTER_NODE

BEDROOM1_NODE

FAN_DEVICE

MODE_AUTO
```

---

# 📝 Logging Standard

Every important event should be logged.

Examples:

```text
[OTA]

[ESP-NOW]

[HEARTBEAT]

[BLYNK]

[NODE]

[AUTOMATION]

[DIAG]

[ERROR]

[WARN]
```

Logs should be informative without excessive spam.

---

# 🤝 Contributing

Contributions are welcome!

If you'd like to contribute:

1. Fork the repository.
2. Create a feature branch.
3. Commit your changes.
4. Test thoroughly.
5. Submit a pull request.

Please follow the existing coding standards and architecture.

---

# 📜 License

This project is licensed under the **MIT License**.

You are free to:

- Use
- Modify
- Distribute
- Learn from

this project, provided that the original license and copyright notice are retained.

---

# 👨‍💻 Author

**Pramath Hegde**

🎓 Computer Science & Engineering Student

🏫 PES University, Bengaluru

💡 Interests:

- Embedded Systems
- IoT
- Artificial Intelligence
- Full Stack Development
- System Design
- Home Automation

---

# 🙏 Acknowledgements

Special thanks to:

- ESP32 Community
- ESP8266 Community
- PlatformIO
- Arduino Framework
- Espressif Systems
- Blynk IoT Platform
- Open-source contributors whose libraries made this project possible.

---

# ⭐ Support

If you found this project useful:

⭐ Star this repository

🍴 Fork it

📢 Share it

Your support motivates future development!

---

# 📬 Contact

If you have suggestions, ideas, or feedback:

- Open an Issue on GitHub
- Submit a Pull Request
- Start a Discussion

---

# 🏆 Project Status

> **Current Version:** Active Development 🚧

### Completed

- ✅ Distributed Master–Node Architecture
- ✅ ESP-NOW Communication
- ✅ OTA Updates
- ✅ Blynk Integration
- ✅ Motion Automation
- ✅ LDR-based Lighting
- ✅ Per-Room LDR Enable
- ✅ Configurable Motion Timeout
- ✅ Dual Schedule Architecture
- ✅ Persistent NVS Configuration
- ✅ Reliable ACK-based Communication
- ✅ Automatic Node Recovery
- ✅ Dashboard Synchronization
- ✅ Production-grade Logging

### In Progress

- 🚧 Additional Room Nodes
- 🚧 Temperature & Humidity Monitoring
- 🚧 Water Tank Automation
- 🚧 Smart Door Lock
- 🚧 Energy Monitoring

### Planned

- 📋 Google Home Integration
- 📋 Alexa Integration
- 📋 AI Automation
- 📋 Web Dashboard
- 📋 Mobile Notifications
- 📋 Voice Commands

---

<div align="center">

## ⭐ If you like this project, please consider giving it a star! ⭐

**Built with ❤️ using ESP32, ESP8266, ESP-NOW, PlatformIO, Arduino, and Blynk.**

**Advaita Smart Home IoT — A Professional, Scalable & Production-Ready Home Automation System.**

</div>
