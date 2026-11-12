# Car Receiver Firmware – Functional Specification

Nov 12, 2026 · @Hog

This document describes what the car receiver firmware (Nov 12, 2026 code) does today, which earlier findings it fixed, and what is still open. The controller firmware now matches the car's confirmed pairing handshake (mode 3). It is one of four specifications: Car Firmware (this one), Controller Firmware, Car Hardware and Controller Hardware.

## 1. Scope

- **Covers:** the car sketch `ESP_NOW_CAR.ino` with `setupNormalMode`, `loopNormalMode`, `setupBindingMode`, `loopBindingMode`, `Storage`, `StatusLed` and `HardwareConfig.h`.
- **Does not cover:** pins, power and parts (Car Hardware FSD), or the controller side of the protocol (Controller Firmware FSD).
- **Status:** Draft 1.2, as-built. It describes what the code does, not what it should do. Unknowns are marked **\[?\]**.

## 2. What the Car Firmware Does

1. Pairs with exactly one controller over ESP-NOW and remembers its MAC address in flash.
2. Accepts drive and weapon commands only from that controller.
3. Drives two DC motors through a DRV8833 H-bridge and one hobby servo.
4. Stops both motors if no valid packet arrives for 250 ms.
5. Reports battery voltage to the controller when the link comes up and every 5 s after that.

## 3. What Changed Since the First Review

| Earlier ID | Status | What the code does now |
| --- | --- | --- |
| O-01 Telemetry flood | Fixed | Telemetry is sent only while linked: once when the link comes up, then every 5 s. |
| O-02 Sender not checked | Fixed, one gap left | The receive callback accepts only mode 1 packets whose source MAC equals the stored controller MAC. MAC addresses can still be spoofed (see N-06). |
| O-03, O-04 Pairing | Improved | The car now sends its acknowledgement directly to the captured controller and waits for a new mode 3 confirmation before saving. The controller sends mode 3 as of its Nov 12 code. |
| O-08 Length check | Fixed | Both receive callbacks check the packet length first. |
| O-11 Battery reading | Fixed | `analogReadMilliVolts()` averaged over 16 samples, times 2. |
| O-12 Flash read per send | Fixed | The MAC is cached in RAM at startup. |
| O-14, N-02 Binding timeout | Fixed | The car waits up to 60 s for a bind request, then up to 15 s for the handshake. Either timeout restarts it. |
| O-09 Pin maps and V4 LED | Fixed | One board line at the top of `ESP_NOW_CAR.ino` selects V2, V3 or V4. The V4 WS2812 LED is driven with `rgbLedWrite()`. |
| O-17, N-05 Dead code, flash write | Fixed | `SendBindingComplete` and the binding send callback are gone. The flash write result is checked. |
| O-05 Race on received packet | Fixed | The callback copies the packet under a lock (`rxMux`) and the loop takes its own copy under the same lock. |
| N-04 Phantom link at boot | Fixed | The car does not count as linked until its first valid packet. |
| N-08 Peer added in callback | Fixed | The binding callback only copies the MAC and sets a flag. The loop prints it and adds the peer. |
| O-06, O-07, O-10, O-13, O-15, O-16, O-18 | Unchanged | See Section 12. |

## 4. Hardware Dependencies

The firmware reaches hardware only through macros in `HardwareConfig.h`. GPIO numbers for each board revision are in the Car Hardware FSD.

| Macro | Role |
| --- | --- |
| `A1Pin`, `A2Pin` | Motor A inputs (PWM) |
| `B1Pin`, `B2Pin` | Motor B inputs (PWM) |
| `WeaponPin` | Servo signal (50 Hz PWM) |
| `BindPin` | Bind button, active low, internal pull-up |
| `BATTERY_ADC_PIN` | Battery divider input (ratio 2:1) |
| `STATUS_LED` | Status LED pin |
| `STATUS_LED_ACTIVE_LOW` / `STATUS_LED_RGB` | LED type: plain LED on when LOW (V2, V3) or WS2812 (V4) |
| `WIFI_CHANNEL` | Binding channel, 1 |
| `BOARD_VERSION_STR` | Printed at boot |

**Revision selection:** the line `#define HARDWARE_V2` at the top of `ESP_NOW_CAR.ino` picks the board; change it to `HARDWARE_V3` or `HARDWARE_V4`. A build that passes `-DHARDWARE_Vx` (the GitHub flasher build) overrides that line. Any other value (for example `HARDWARE_V1`) falls back to V2. `HardwareConfig.h` stops the build with an error if no board is chosen.

## 5. Software Structure

| File | Role |
| --- | --- |
| `ESP_NOW_CAR.ino` | Globals, packet struct, `setup()`, `loop()` dispatch |
| `setupNormalMode.ino` | Radio, peer, receive callback, PWM and servo init |
| `loopNormalMode.ino` | Link status, telemetry, command handling, failsafe, LED |
| `setupBindingMode.ino` | Radio and callbacks for pairing |
| `loopBindingMode.ino` | Handshake, timeout, flash write, restart |
| `Storage.ino` | Reads and writes the controller MAC (6 bytes) |
| `StatusLed.ino` | `setStatusLed()`: the one place that knows the LED type; writes only on change |
| `HardwareConfig.h` | Pin and revision macros |

Built with Arduino-ESP32 core 3.3.12. The design is a cooperative loop. ESP-NOW callbacks run in the Wi-Fi task and pass data to `loop()` through globals.

| Shared variable | `volatile` | Written by | Read by |
| --- | --- | --- | --- |
| `NewDataFlag` | Yes, plus `rxMux` | Receive callback | Normal loop |
| `ReceivingData` | Guarded by `rxMux` | Receive callback | Normal loop (takes a copy) |
| `HandshakeConfirmed` | Yes | Binding receive callback | Binding loop |
| `ReceiveBindingComplete` | Yes | Binding receive callback | Binding loop |
| `capturedRemoteMac` | No; written once before `ReceiveBindingComplete` is set | Binding receive callback | Binding loop |

Modes change only by restart, except that Normal setup falls through to Binding setup when no valid MAC is stored.

## 6. Communication Protocol

### 6.1 Radio

- Wi-Fi station mode, 19.5 dBm, ESP-NOW, no encryption, default PHY rate.
- **Binding channel:** 1.
- **Driving channel:** last byte of the controller's MAC modulo 3: 0 → channel 1, 1 → channel 6, 2 → channel 11.

### 6.2 Packet

One packed struct is used in both directions (25 bytes: 1 byte plus six 32-bit ints).

| Field | Type | Meaning |
| --- | --- | --- |
| `protocolMode` | uint8 | See 6.3 |
| `motor_A1`, `motor_A2` | int | Motor A PWM, 0–255 |
| `motor_B1`, `motor_B2` | int | Motor B PWM, 0–255 |
| `weapon` | int | Servo angle, 0–180 |
| `battery` | int | Battery voltage in mV (car to controller only) |

### 6.3 Modes and messages

| Mode | Name | From, to | When |
| --- | --- | --- | --- |
| 0 | Bind request | Controller to broadcast, channel 1 | Repeated by the controller while binding |
| 1 | Normal | Controller to car (drive). Car to controller (telemetry). | Drive: 20 ms linked, per controller. Telemetry: link-up, then 5 s. |
| 2 | Bind acknowledgement | Car to captured controller, unicast | Every 100 ms until confirmed |
| 3 | Bind confirmed | Controller to car, unicast | Sent by the controller every 100 ms for 1.5 s after it receives mode 2 |

## 7. Operating Modes

### 7.1 Boot

1. Start Serial at 115200. Set 11 dB attenuation on the battery pin. Wait 50 ms.
2. Drive all four motor pins and the weapon pin LOW as outputs.
3. Print the board name. Turn the status LED off and set the bind pin to pull-up.
4. Wait 50 ms and read the bind pin. LOW: Binding Mode. HIGH: Normal Mode.

### 7.2 Normal setup

1. Read the controller MAC from flash. If missing or all zeros, fall back to Binding Mode.
2. Cache the MAC in RAM and compute the channel.
3. Initialize ESP-NOW (restart on failure), set the channel, add the controller as a unicast peer.
4. Receive callback: accept a packet only if it is at least 25 bytes, has mode 1 and comes from the cached MAC. Copy it and set `NewDataFlag` under the `rxMux` lock.
5. Attach PWM: four motor pins at 5 kHz, 8-bit. Servo pin at 50 Hz, 16-bit, set to 0° (duty 3276).

### 7.3 Normal loop

Each pass:

1. **Command handling** (if `NewDataFlag` is set): take a copy of the packet under `rxMux`, clear the flag, mark that a packet has been received, record the time, then apply 7.4.
2. **Link status.** Linked means at least one packet has been received and the last one was processed within the last 250 ms.
3. **Telemetry.** On the rising edge of linked, and then every 5 s while linked: average 16 calibrated ADC readings (0.5 ms apart), multiply by 2, cap at 0–4200 mV, and send to the cached MAC with mode 1.
4. **Failsafe.** If not linked, set all four motor outputs to 0 and slow-blink the LED (500 ms toggle). If linked, hold the LED on.

### 7.4 Command handling

1. Clamp each motor value to 0–255.
2. Values below 15 become 0 (deadband).
3. If both inputs of one motor are above 0, set both to 0.
4. Write the four PWM values.
5. Clamp the weapon angle to 0–180 and map it to a servo duty of 3276 to 6553 (about 1 ms to 2 ms at 50 Hz).

Motors are driven sign-magnitude: one input carries PWM and the other is 0. Both at 0 is coast. There is no brake.

### 7.5 Binding setup

Station mode, 19.5 dBm, ESP-NOW (restart on failure), channel 1, broadcast peer.

- **Receive callback** (packet at least 25 bytes): the first mode 0 packet is captured. Its sender MAC is copied, then `ReceiveBindingComplete` is set. A mode 3 packet from the captured MAC sets `HandshakeConfirmed`. The callback does nothing else.

### 7.6 Binding loop

1. Toggle the LED every 50 ms.
2. Print flag states to Serial every 500 ms.
3. Once, right after capture: print the controller MAC and add it as a unicast peer on channel 1.
4. If no controller is captured within 60 s of the loop first running, restart.
5. If captured but not confirmed within 15 s of capture, restart.
6. After capture and until confirmed, send a zeroed mode 2 packet to the captured MAC every 100 ms.
7. On confirmation: write the MAC to flash (an error is printed if the write fails), turn the LED off, wait 500 ms, restart.

### 7.7 States

| State | Entry | Exit |
| --- | --- | --- |
| Binding | Bind button held at power-up, or no valid stored MAC | Confirmed pair then restart, or timeout (60 s no request, 15 s no confirmation) then restart |
| Normal, not linked | Normal setup done, no packet for more than 250 ms | A valid mode 1 packet |
| Normal, linked | Valid packet within 250 ms | 250 ms of silence |

## 8. Functional Requirements

| ID | Requirement |
| --- | --- |
| CF-01 | All motor and weapon pins shall be driven LOW as outputs immediately after reset. |
| CF-02 | The car shall enter Binding Mode if the bind button is held at power-up. |
| CF-03 | The car shall enter Binding Mode if no non-zero controller MAC is stored. |
| CF-04 | Otherwise the car shall start in Normal Mode. |
| CF-05 | If ESP-NOW fails to initialize, the car shall restart. |
| CF-10 | In Binding Mode the car shall listen on channel 1 for a mode 0 packet and capture the first sender's MAC. |
| CF-11 | After capture the car shall send mode 2 packets to that MAC every 100 ms. |
| CF-12 | Pairing shall complete only when a mode 3 packet arrives from the captured MAC. |
| CF-13 | On completion the car shall store the MAC (namespace `rc_car`, key `controller_mac`), wait 500 ms and restart. |
| CF-14 | The car shall restart if no mode 0 request is captured within 60 s, or if no mode 3 confirmation arrives within 15 s of capture. |
| CF-15 | Binding Mode shall be shown by a 10 Hz LED toggle. |
| CF-20 | Normal Mode shall use the channel derived from the stored MAC (1, 6 or 11). |
| CF-21 | The car shall act only on mode 1 packets of at least 25 bytes from the stored MAC. |
| CF-22 | Motor values shall be clamped to 0–255, with values below 15 treated as 0. |
| CF-23 | If both inputs of a motor are non-zero, both shall be set to 0. |
| CF-24 | The weapon angle shall be clamped to 0–180 and output as a 50 Hz servo pulse of about 1–2 ms. |
| CF-25 | Motor PWM shall be 5 kHz at 8-bit resolution. |
| CF-30 | If no valid packet arrives for more than 250 ms, or none has arrived since boot, all four motor outputs shall be set to 0. |
| CF-31 | The LED shall be solid on while linked and toggle every 500 ms while not linked. |
| CF-32 | The failsafe shall be evaluated on every pass of the main loop. |
| CF-40 | While linked the car shall send battery voltage to the stored MAC when the link comes up and every 5 s. |
| CF-41 | Voltage shall be the average of 16 calibrated readings times 2, capped to 0–4200 mV. |
| CF-42 | Telemetry shall use `protocolMode` 1. |
| CF-43 | No telemetry shall be sent while the link is down. |
| CF-50 | With `DEBUG_MODE` true the car shall print status and warnings to Serial at 115200 baud. |

## 9. Key Parameters

| Parameter | Value |
| --- | --- |
| Failsafe timeout | 250 ms |
| Telemetry period | 5000 ms |
| Motor deadband | 15 of 255 |
| Motor PWM | 5 kHz, 8-bit |
| Servo PWM | 50 Hz, 16-bit, duty 3276–6553 |
| Battery averaging | 16 samples, 0.5 ms apart |
| Battery cap | 4200 mV |
| Bind request wait | 60 s |
| Handshake timeout (after capture) | 15 s |
| Acknowledgement interval | 100 ms |
| Binding LED toggle | 50 ms |
| Link-lost LED toggle | 500 ms |
| Post-pairing delay | 500 ms |
| TX power | 19.5 dBm |
| Binding channel | 1 |
| Driving channels | 1, 6, 11 |

## 10. Storage

- `Preferences` namespace `rc_car`, one record: `controller_mac`, 6 bytes.
- A missing or short record counts as unpaired and the output is zeroed.
- The MAC is written only after a confirmed pairing. A failed write prints an error and the old pairing is kept.
- The only way to change pairing is to pair again.

## 11. Safety Behavior

- Outputs are low from reset until PWM is attached.
- Motors stop within about 250 ms of link loss, and also while the controller menu is open (the controller sends nothing then).
- Both bridge inputs can never be driven at once.
- The servo holds its last angle on link loss and sits at 0° from boot until the first packet. Weapon behavior is deferred to next year.
- There is no low-battery cutoff. The user watches the voltage on the controller.

## 12. Remaining Observations

Severity is my estimate. Themes follow your priorities: Clarity, Pairing, Latency.

**Resolved since Draft 1.0:** N-01 (the controller now sends mode 3). **Resolved Nov 12:** N-02 binding window, O-05 race, O-09 board select and V4 LED, N-04 phantom link, N-05 flash write check, N-08 peer added in callback.

| ID | Severity | Theme | Status | Observation |
| --- | --- | --- | --- | --- |
| N-07 | Medium | Pairing | Open, needs both sides | **No protocol version in the packet.** A car running this firmware can capture a controller with older firmware that never sends mode 3, then time out with no explanation. A version byte would let each side ignore packets from mismatched firmware. |
| O-06 | Medium | Pairing, Clarity | Open, needs both sides | **Telemetry uses mode 1,** the same value as drive commands. The controller does not check the mode of received packets. A separate mode would remove the ambiguity. |
| O-07 | Medium | Clarity | Deferred | **Servo holds its angle on link loss** and starts at 0°. Deferred with the weapon design. |
| N-03 | Low | Latency | Open | **Battery sampling blocks the loop** for roughly 8 to 10 ms every 5 s (16 samples with 0.5 ms waits). Motors keep their last PWM, but packet handling is delayed. A non-blocking, one-sample-per-pass average avoids it. |
| N-06 | Low | Safety | Optional | **Authentication is by MAC address only.** A device that copies the controller's MAC could send commands, and there is no sequence number against replay. ESP-NOW encryption with a per-pair key would harden this. Optional for a club setting. |
| O-10 | Low | Clarity | Open | **GPIO20 shares UART0 RX** on V2 (motor B input 1). Arduino IDE (MakerGO ESP32 C3 SuperMini) and the GitHub flasher build both set "USB CDC On Boot" to Enabled, so Serial goes over USB, not GPIO20. Still to confirm on a web-flashed V2 car. V3 and V4 avoid it. |
| O-13 | Low | Clarity | Open | **The loop never sleeps.** Matters only for battery life. |
| O-15 | Low | Clarity | Open | **Channel is set after `esp_now_init()`.** Works, but verify the order. 19.5 dBm is maximum power and does not by itself reduce congestion. |
| O-16 | Low | Clarity | Deferred | **Servo pulse is 1–2 ms.** An MG90S usually spans about 0.5–2.4 ms, so the full 180° is not reached. Deferred with the weapon design. |
| O-18 | Low | Clarity | Accepted | **No brake.** Direction reversal on the car is instantaneous. The controller ramps speed. |

## 13. Open Questions

1. **Toolchain.** Core 3.3.12, board MakerGO ESP32 C3 SuperMini, "USB CDC On Boot" Enabled. The GitHub flasher build uses the same settings (pinned in `.github/workflows/build.yml`).
2. **V4 LED colour order.** The LED is set to blue, which is the same on RGB and GRB parts. Confirm it lights on a V4 board.
3. **Weapon.** Safe servo angle on link loss and the servo pulse range, when the flipper is designed.
