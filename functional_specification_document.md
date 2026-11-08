# Functional Specification Document (FSD)
## ESP-NOW Remote Controlled Vehicle Platform
**Document Version:** 4.0  
**Target Hardware:** ESP32-C3 SuperMini / DRV8833 Dual H-Bridge  
**System Architecture:** Modular ESP-NOW Connectionless Telemetry & Control Engine  

---

## 1. System Architecture Overview

The ESP-NOW RC Vehicle platform is an embedded robotics ecosystem designed for high-density, low-latency control environments. The software architecture decouples motor safety, system configuration, binding state machine loops, non-volatile storage management, and packet telemetry into dedicated operational modules.

```
                  +-----------------------------------+
                  |             POWER-UP              |
                  +-----------------------------------+
                                    |
                       [GPIO 5 (BindPin) == LOW?]
                               /         \
                             YES          NO
                             /             \
             +--------------------+   +--------------------+
             | setupBindingMode() |   | setupNormalMode()  |
             +--------------------+   +--------------------+
                       |                        |
             +--------------------+   +--------------------+
             | loopBindingMode()  |   |  loopNormalMode()  |
             +--------------------+   +--------------------+
```

---

## 2. Hardware Abstraction & Pin Definitions (`HardwareConfig.h`)

The system uses compile-time macro switches to maintain backward compatibility with legacy PCB layouts (`V2`, `V3`, and `V4`). 

```cpp
#pragma once

#if defined(HARDWARE_V4) || defined(HARDWARE_V3)
  #define BOARD_VERSION_STR "ESP32-C3 RC Receiver V4/V3 Layout"
  #define WIFI_CHANNEL      1
  #define STATUS_LED        8   // Active-LOW Status LED
  #define A1Pin             1   // Motor A Forward (PWM)
  #define A2Pin             0   // Motor A Reverse (PWM)
  #define B1Pin             20  // Motor B Forward (PWM)
  #define B2Pin             10  // Motor B Reverse (PWM)
  #define BindPin           5   // Hardware Pair Pushbutton (INPUT_PULLUP)
  #define WeaponPin         4   // Servo / Auxiliary Channel Output (PWM)
  #define BATTERY_ADC_PIN   3   // Battery Sensing Voltage Divider Input
#else
  #define BOARD_VERSION_STR "ESP32-C3 RC Receiver V2.0 (Legacy)"
  #define WIFI_CHANNEL      1
  #define STATUS_LED        8
  #define A1Pin             1
  #define A2Pin             0
  #define B1Pin             20
  #define B2Pin             10
  #define BindPin           5
  #define WeaponPin         4
  #define BATTERY_ADC_PIN   3
#endif
```

---

## 3. Communication Protocol & Data Structure

Control signals and telemetry packets are encapsulated in a single binary packed structure.

```cpp
struct __attribute__((packed)) RC_Packet_Structure {
  uint8_t protocolMode; // 0 = Bind Request, 1 = Normal Drive Data, 2 = Bind Acknowledgement
  int motor_A1;         // Motor A PWM Duty (0 to 255)
  int motor_A2;         // Motor A Reverse PWM Duty (0 to 255)
  int motor_B1;         // Motor B PWM Duty (0 to 255)
  int motor_B2;         // Motor B Reverse PWM Duty (0 to 255)
  int weapon;           // Auxiliary Servo Mapped Angle (0 to 180 degrees)
  int battery;          // Battery Telemetry Voltage Reading in Millivolts (mV)
};
```

### Protocol Protocol Modes (`protocolMode`)
* **`0` (Bind Request):** Transmitted by remote controllers during broadcast pairing mode.
* **`1` (Normal Drive):** Continuous high-frequency control payload sent from paired remote to receiver; telemetry returned to remote.
* **`2` (Bind Acknowledgement):** Broadcast confirmation reply sent by car to lock pairing state.

---

## 4. State Machine Specification

### 4.1 Boot Sequence & Hardware Initialization (`ESP_NOW_CAR.ino`)
1. **Serial & ADC Setup:** Initializes baud rate (`115200`) and sets `BATTERY_ADC_PIN` attenuation to `ADC_11db` (enabling 0-3.3V measurement range).
2. **Immediate Safety Interlock:** All motor control pins (`A1Pin`, `A2Pin`, `B1Pin`, `B2Pin`) and `WeaponPin` are set to `OUTPUT` and explicitly forced `LOW`.
3. **Hardware State Evaluation:** Reads `BindPin` (`GPIO 5`). If `LOW` (button active), branches into `setupBindingMode()`. Otherwise, proceeds to `setupNormalMode()`.

### 4.2 Binding Engine (`setupBindingMode.ino` & `loopBindingMode.ino`)
* **Radio Config:** Operates in `WIFI_STA` mode on Wi-Fi Channel 1 with high output power (`WIFI_POWER_19_5dBm`).
* **Broadcast Whitelist Capture:** Adds broadcast peer (`FF:FF:FF:FF:FF:FF`). When a packet with `protocolMode == 0` is received:
  1. Extracted transmitter MAC address (`recv_info->src_addr`) is stored in `capturedRemoteMac`.
  2. Sets `ReceiveBindingComplete = true`.
* **Registration Handshake:** Blasts up to 10 acknowledgement packets (`protocolMode = 2`) back across the network to notify the remote control unit.
* **Flash Commit:** Writes the 6-byte remote MAC address to non-volatile storage (`rc_car` namespace, key `controller_mac`), sets status LED solid HIGH, waits 1.5 seconds, and triggers `ESP.restart()`.

### 4.3 Driving Engine & Safety Clamps (`setupNormalMode.ino` & `loopNormalMode.ino`)
* **NVS Peer Retrieval:** Reads stored MAC from flash memory. If key missing or all zeros, forces boot back to binding mode.
* **Automated Channel Allocation:** Dynamic Wi-Fi channel calculated using MAC address remainder math to minimize RF interference in dense environments:
  $$\text{Channel} = \begin{cases} 6 & \text{if } (\text{MAC}[5] \pmod 3) = 1 \\ 11 & \text{if } (\text{MAC}[5] \pmod 3) = 2 \\ 1 & \text{otherwise} \end{cases}$$
* **Motor PWM Configuration:** Attaches native hardware PWM channels at **5 kHz frequency, 8-bit resolution (0-255)**.
* **Weapon Servo Configuration:** Attaches auxiliary pin at **50 Hz frequency, 16-bit resolution**, mapping 0-180° inputs to pulse duty width range `3276` to `6553`.
* **Signal Deadband & Conflict Safety Clamps:**
  * **Deadband Filter:** Suppresses inputs where $\text{duty} < 15$ to eliminate motor whine at rest.
  * **Mutual Exclusivity Protection:** Prevents opposing directional bridge drivers from firing concurrently:
    $$\text{If } (A1 > 0 \text{ and } A2 > 0) \implies A1 = 0, A2 = 0$$
    $$\text{If } (B1 > 0 \text{ and } B2 > 0) \implies B1 = 0, B2 = 0$$

### 4.4 Connection Watchdog & Failsafe
* **Failsafe Timeout:** Evaluates system time against the last valid payload timestamp (`millis() - rxPreviousMillis`).
* **Trigger Threshold (250ms):** If no valid packet arrives within 250 milliseconds:
  1. PWM duty cycle to all motor outputs is immediately forced to `0`.
  2. Telemetry initial flag resets (forcing instantaneous voltage broadcast upon re-link).
  3. Status LED enters a slow flash state (500ms toggle rate).

---

## 5. Non-Volatile Storage Schema (`Storage.ino`)

The firmware utilizes Espressif's native `Preferences` library (NVS Flash Partition).

| Parameter | NVS Namespace | Key Name | Data Type | Size | Description |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Whitelisted Remote MAC** | `"rc_car"` | `"controller_mac"` | `uint8_t[]` | 6 Bytes | Hardware MAC address of bound controller |

---

## 6. Development Roadmap for Future Maintainers

1. **PlatformIO Toolchain Migration:** Convert current file structure into a standard PlatformIO repository layout (`src/`, `lib/`, `include/`).
2. **C++ Class Encapsulation:** Refactor state-machine procedural loops into object-oriented modules (`MotorDriver`, `BindingManager`, `TelemetryEngine`).
3. **Bi-Directional Telemetry Expansion:** Expand telemetry structure to support speed encoder metrics and IMU tilt feedback.