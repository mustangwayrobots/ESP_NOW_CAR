# Mustang Way Robots — Engineering & Development Portal

Welcome to the **Mustang Way Robots** project documentation and repository. This project covers the design, testing, and deployment of custom ESP32-C3-powered mini robotics and handheld remote controller units designed for hands-on STEM student learning.

---

## 🛠 Project Status & Hardware Roadmap

> **Current Phase:** 🧪 **Testing & Optimization Stage**  
> We are currently validating prototype revisions and testing component performance. **No production PCB runs have been manufactured for student deployment yet.**

### Hardware Generations & Iterations

* **Hardware v1–v3 (Prototypes):**
  * Core MCU: **ESP32-C3 Super Mini**
  * Validated initial circuit design, pin mapping, DRV8833 motor driver control, and ESP-NOW/Wi-Fi telemetry protocols.
* **Hardware v4 (Active Testing):**
  * Core MCU: **ESP32-C3 Zero** *(Testing phase)*
  * **Objective:** Evaluating the ESP32-C3 Zero board layout to measure potential Wi-Fi signal strength and link reliability gains over the Super Mini model.
  * **Note:** Board pinout configurations are being verified before finalizing the official student PCB layout.

---

## 📸 Media & Hardware Gallery

Below are visual references of prototype builds, hardware testing setups, and controller units. 

*(Make sure your image files in the `images/` directory match the filename paths listed below).*

| Feature / Assembly | Preview |
| :--- | :--- |
| **RC Car Prototype Chassis** | `![RC Car Chassis](images/car_prototype.jpg)` |
| **Handheld Controller Unit** | `![Remote Controller](images/controller_prototype.jpg)` |
| **ESP32-C3 MCU Comparison** | `![ESP32 SuperMini vs Zero](images/esp32_c3_comparison.jpg)` |
| **Component & Wiring Layout** | `![Wiring Diagram](images/wiring_layout.jpg)` |

---

## 🏎 Hardware Components (BOM Overview)

* **Robot Unit:**
  * ESP32-C3 Microcontroller (v4 Zero testing / v1-v3 Super Mini)
  * DRV8833 Dual H-Bridge Motor Driver
  * Dual N20 Gear Motors (6V ~500 RPM) + Rubber Wheels
  * LiPo Battery Pack + JST Connectors
  * MT3608 Boost / HT7833 Voltage Regulation
* **Handheld Remote Controller:**
  * ESP32-C3 Microcontroller with integrated OLED display (1.3" / 0.96")
  * Dual-Axis PS2 Joystick Module + Cap
  * 12x12mm Pushbuttons & Power Switches
  * On-board LiPo Charging & Telemetry Feedback

---

## 🚀 Software & Setup

1. **Firmware:** Developed in Arduino IDE / ESP-IDF supporting ESP-NOW low-latency wireless pairing.
2. **Web Portal:** Web-based status dashboard tracking build updates and repository documentation.

---

## 🤝 Project Maintainers
* **Lead Developer / Engineering:** Mustang Way Robotics Team
* **Target Audience:** High School STEM / Robotics Curriculum