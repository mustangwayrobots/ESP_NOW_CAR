# ESP NOW Car

An open-source, custom-designed remote-controlled (RC) car powered by the **ESP-NOW** wireless protocol. Created by **Mustang Way Robots (Downers Grove South High School)**, this project combines a custom mainboard PCB, modular off-the-shelf electronics, custom motor backplate PCBs, and 3D-printed chassis components into a compact, easily assembleable robotics platform.

---

## Features

- **Protocol:** High-speed, low-latency control via Espressif’s **ESP-NOW** protocol.
- **Microcontroller:** ESP32-C3 Super Mini.
- **Motor Control:** DRV8833 dual H-bridge motor driver controlling two N20 micro gear motors.
- **Modular Power Management:**
  - 1S LiPo battery powered.
  - Onboard TP4056 micro/USB-C charging module.
  - DC-DC Step-Up (Boost) Converter to regulate system voltage.
  - Onboard battery voltage sense divider for monitoring state of charge.
- **Modular Expansion & User Inputs:**
  - Integrated tactile **Binding Button** for quick remote pairing.
  - Dedicated 3-pin Servo output header (5V, GND, Signal).
  - Main power slide switch.
- **Solderless Motor Connection System:** Custom backplate PCBs attached to N20 motors feature right-angle female pin headers. The motors push directly onto male pin headers on the chassis/PCB, securing the electrical connection while the PCB screws onto the 3D-printed body to lock everything in place.

---

## Component Hardware Specifications

| Component | Description / Specification |
| :--- | :--- |
| **Microcontroller** | ESP32-C3 Super Mini |
| **Motor Driver** | DRV8833 Dual H-Bridge Module |
| **Battery Charger** | TP4056 1S LiPo Charging Module |
| **Power Converter** | MT3608 Mini DC-DC Boost Step-Up Converter |
| **Power Source** | 1S LiPo Battery (JST / SH-PH2.0 connector) |
| **Motors** | 2x N20 Micro Gear Motors with custom PCB backplates |
| **PCB Dimensions** | 86.00 mm × 42.00 mm (Mounting hole spacing: 79.00 mm × 35.00 mm) |

---

## Schematic & Pinout Overview

### ESP32-C3 Pin Connections

| ESP32-C3 Pin | Function / Destination |
| :--- | :--- |
| **GPIO5** | Binding Button (`Binding`) |
| **GPIO4** | Servo Signal Output (`Servo Pin`) |
| **GPIO3** | Battery Voltage Sensor (`VBAT_ADC`) |
| **GPIO10** | Motor Driver IN1 (`A1-IN`) |
| **GPIO20 / RX** | Motor Driver IN2 (`A2-IN`) |
| **GPIO21 / TX** | Motor Driver IN3 (`B1-IN`) |
| **GPIO10 / IO1**| Motor Driver IN4 (`B2-IN`) |
| **5V / 3V3 / GND**| System Power Rails |

---

## Firmware & Web Flasher

You can flash the latest firmware directly to your ESP32-C3 using a Web Serial supported browser (Chrome, Edge, Opera) without setting up a local IDE environment:

👉 **[Flash Firmware via Mustang Way Robots Web Tool](https://mustangwayrobots.github.io/flash/)**

### Building from Source
If building in Arduino IDE or PlatformIO:
1. Ensure the Espressif ESP32 board support packages are installed.
2. Select **ESP32C3 Dev Module** or **ESP32-C3 Super Mini** as your target board.
3. Include the necessary `WiFi` and `esp_now` libraries included with the ESP32 core.

---

## Mechanical & Assembly Guide

1. **Motor Preparation:** 
   - Solder the custom N20 backplate PCB to the terminals of each N20 gear motor.
   - Solder right-angle female header sockets onto the backplate PCBs.
2. **Chassis Assembly:**
   - Slide the N20 motors into the designated slots inside the 3D-printed chassis body.
3. **PCB Installation:**
   - Align the mainboard male headers with the female connectors on the motor backplates.
   - Press the mainboard down into position and fasten it to the 3D-printed chassis using four M3 screws through the mounting holes.

---

## Repository Structure

```text
├── 3D_Models/       # STL files for chassis and motor housings
├── Hardware/        # Schematic files, Gerber files, and EasyEDA project files
├── Firmware/        # Source code for the ESP32-C3 vehicle
└── README.md        # Project documentation
```

---

## License & Credits

Designed by **Mustang Way Robots** — Downers Grove South High School Robotics Club.  
Open-source hardware and software released under the [MIT License](LICENSE).