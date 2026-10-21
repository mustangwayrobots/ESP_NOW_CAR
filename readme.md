# ESP NOW CAR

An open-source, custom-designed remote-controlled (RC) car powered by the high-speed, low-latency **ESP-NOW** wireless protocol. Developed for robotics education, this project combines a custom mainboard PCB, modular off-the-shelf electronics, custom motor backplates, and 3D-printed chassis components into a compact, easily assemblable robotics platform.

---

## 🚀 Features

* **Wireless Protocol:** High-speed, low-latency control via Espressif's ESP-NOW protocol.
* **Microcontroller:** ESP32-C3 Super Mini (compact, cost-effective, and Wi-Fi/BLE capable).
* **Motor Control:** Dual H-bridge motor driver configuration driving N20 micro gear motors.
* **Power Management:** Onboard TP4056 charging module with lithium battery support and a DC-DC step-up boost module.
* **Telemetry & Sensing:** Integrated battery voltage divider for real-time voltage monitoring and binding button support.
* **Browser-Based Flashing:** Fully compatible with web-based firmware flashers for instant classroom deployment.

---

## 📷 Hardware & Assembly

### Assembled Chassis
The robot features a rigid 3D-printed chassis housing the custom mainboard, dual N20 gear motors, and battery compartment.

<img src="https://raw.githubusercontent.com/mustangwayrobots/ESP_NOW_CAR/main/images/PXL_20261020_210856805.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Assembled Chassis">

### Custom N20 Motor Backplates
Each N20 gear motor is equipped with a custom backplate and secure connector interface for reliable wiring and easy maintenance.

<img src="https://raw.githubusercontent.com/mustangwayrobots/ESP_NOW_CAR/main/images/PXL_20261020_210916029.MACRO_FOCUS.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="N20 Motor Backplate">

---

## ⚡ Schematics & PCB Design

### Circuit Schematic (Schematic_V2)
The system integrates the ESP32-C3 SuperMini, motor drivers, DC-DC boost module, battery charging circuit, battery sense voltage divider, and binding button.

<img src="https://raw.githubusercontent.com/mustangwayrobots/ESP_NOW_CAR/main/images/schematic.png" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Schematic V2">

### PCB Layout & Dimensions
Designed in EasyEDA Pro, the custom double-sided PCB measures **86.00mm x 42.00mm** with standardized mounting holes for secure attachment to the 3D-printed chassis.

<img src="https://raw.githubusercontent.com/mustangwayrobots/ESP_NOW_CAR/main/images/pcb-layout.png" style="max-width: 100%; height: auto; border-radius: 8px;" alt="PCB Layout">

---

## 📋 Bill of Materials (BOM)

| Component | Description | Qty |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32-C3 Super Mini | 1 |
| **Motor Drivers** | H-Bridge Motor Driver Module (DRV8833 / TB6612 compatible) | 1 Set |
| **Motors** | N20 Micro Gear Motors with custom backplates | 2 |
| **Power Management** | TP4056 Li-Ion Charging Module (with protection) | 1 |
| **Voltage Regulation** | Mini DC-DC Step-Up (Boost) Converter Module | 1 |
| **Switch & Inputs** | Slide switch (Power) & Tactile switch (Binding) | 1 Each |
| **Chassis** | Custom 3D-printed frame and wheels (PETG/TPU) | 1 Set |

---

## 🔌 Pinout & Wiring Reference

* **ESP32-C3 GPIO Bindings:**
  * **Binding Button:** Connected to GPIO via tactile switch (`SW3`).
  * **Motor Control:** Driven through dedicated input channels (`A1-IN`, `A2-IN`, `B1-IN`, `B2-IN`).
  * **Battery Sense:** Monitored via an analog voltage divider (`R2`/`R3` 10k resistors) mapped to `VBAT_ADC`.
  * **Servo / Aux:** Standard 3-pin header for optional accessories or steering mechanism.

---

## 💻 Firmware Installation & Flashing

1. Connect your ESP32-C3 board to your computer via USB.
2. Open the browser-based [Firmware Flasher Portal](https://mustangwayrobots.github.io/ESP_NOW_CAR/).
3. Click **Connect**, select the correct serial port, and flash the compiled binary directly in seconds.
