# IncuTwin Firmware

**IncuTwin** is the embedded firmware for the IncuTwin device — an IoT-enabled emotional support companion designed for neonatal incubators in resource-limited hospital environments. Developed by **Medicina Abierta al Mundo (MOW)**.

## Overview

IncuTwin detects the presence of a caregiver or family member near the incubator using dual IR proximity sensors, and provides immediate audio-visual feedback to the newborn through an LED ring and piezoelectric buzzer. All events and device state are reported to a central [ThingsBoard](https://thingsboard.io/) IoT platform via MQTT, enabling real-time monitoring and integration with companion apps.

The device pairs wirelessly with **IncuNest** — a sibling device placed inside the incubator — establishing a bidirectional emotional link mediated through the cloud.

## Hardware

| Component | GPIO | Description |
|-----------|------|-------------|
| IR LED Left | 27 | Proximity emitter (left side) |
| IR LED Right | 15 | Proximity emitter (right side) |
| Phototransistor Left | 39 | Proximity receiver (left side) |
| Phototransistor Right | 33 | Proximity receiver (right side) |
| Ambient Light Sensor | 35 | Suppresses false detections in bright light |
| LED Ring (WS2812B x8) | 19 | Visual feedback |
| Buzzer (Piezo) | 13 | Audio feedback via PWM (LEDC) |

**MCU:** ESP32 (Espressif)
**Build system:** PlatformIO + Arduino framework

## Firmware Phases

The firmware is structured in three incremental phases, selected at compile time via the `PHASE` macro:

| Phase | Description |
|-------|-------------|
| **1** | Hardware bring-up: peripheral initialization and self-test |
| **2** | WiFi connectivity, captive portal provisioning, ThingsBoard MQTT |
| **3** | Full FSM, proximity logic, IncuNest pairing, shared attributes |

## System States

```
BOOT → WIFI_CONNECTING → WIFI_CONNECTED → PROVISIONING
     → SERVER_CONNECTED → IDLE
                        ↕ PROXIMITY_LEFT / PROXIMITY_RIGHT / PROXIMITY_BOTH
                        → SEARCHING → LINKED
                        → ERROR_STATE
```

Each state drives a unique LED animation on the WS2812B ring.

## Dependencies

| Library | Version |
|---------|---------|
| FastLED | ^3.6.0 |
| ArduinoJson | ^6.21.0 |
| ThingsBoard | ^0.9.0 |
| WiFiManager | ^2.0.17 |
| PubSubClient | ^2.8.0 |

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- ESP32 board

### Build & Flash

```bash
# Phase 1 — hardware bring-up
pio run -e esp32dev_phase1 -t upload

# Phase 2 — WiFi + cloud
pio run -e esp32dev_phase2 -t upload

# Phase 3 — full firmware
pio run -e esp32dev_phase3 -t upload
```

### Run Tests

```bash
pio test -e esp32dev_test
```

### First Boot (Phase 2+)

1. On first boot, the device opens a captive portal WiFi AP (`IncuTwin-XXXX`).
2. Connect from any phone/PC and enter your WiFi credentials.
3. The device provisions itself against the ThingsBoard server and stores the auth token in NVS flash.
4. Subsequent boots connect automatically.

## Project Structure

```
incutwin/
├── include/
│   └── config.h              # Global pin map, thresholds, server config
├── src/
│   ├── main.cpp              # Entry point, main loop, FSM orchestration
│   ├── sensors/              # Proximity (IR) and ambient light sensor
│   ├── leds/                 # WS2812B LED ring + SystemState enum
│   ├── buzzer/               # Piezo buzzer with rate limiting
│   ├── state_machine/        # Finite state machine
│   ├── comms/                # WiFi manager, provisioning, ThingsBoard MQTT
│   ├── incunest_link/        # IncuNest device pairing logic
│   ├── storage/              # NVS flash persistence
│   └── serial_debug/         # Debug logging macros
└── test/                     # PlatformIO Unity unit tests
```

## Cloud Architecture

ThingsBoard acts as the single source of truth for all inter-device communication:

```
IncuNest  →  ThingsBoard  ←  IncuTwin
                  ↑
             Mobile App (read-only)
```

Server: `mon.medicalopenworld.org:1883`

## License

Copyright (c) 2024 Medicina Abierta al Mundo (MOW). All rights reserved.
See [LICENSE](LICENSE) for full terms. Unauthorized use, reproduction, or distribution is strictly prohibited.
