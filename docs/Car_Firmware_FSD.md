# Car Receiver Firmware – Functional Specification

Nov 12, 2026 · @Hog

This document describes what the car receiver firmware (Nov 12, 2026 code, protocol version 2) does today, which earlier findings it fixed, and what is still open. The car and controller must run the same protocol version; both carry an identical `Protocol.h`. It is one of four specifications: Car Firmware (this one), Controller Firmware, Car Hardware and Controller Hardware.

## 1. Scope

- **Covers:** the car sketch `ESP_NOW_CAR.ino` with `setupNormalMode`, `loopNormalMode`, `setupBindingMode`, `loopBindingMode`, `Storage`, `StatusLed` and `HardwareConfig.h`.
- **Does not cover:** pins, power and parts (Car Hardware FSD), or the controller side of the protocol (Controller Firmware FSD).
- **Status:** Draft 1.2, as-built. It describes what the code does, not what it should do. Unknowns are marked **\[?\]**.

## 2. What the Car Firmware Does

1. Pairs with exactly one controller over ESP-NOW and remembers its MAC address in flash.
2. Accepts drive and weapon commands only from that controller.
3. Drives two DC motors through a DRV8833 H-bridge and one hobby servo.
4. Stops both motors if no valid packet arrives for 250 ms.
5. Replies with battery voltage (telemetry) when the link comes up and every 200 ms after that. The controller treats these replies as proof the car is listening.

## 3. What Changed Since the First Review

| Earlier ID | Status | What the code does now |
| --- | --- | --- |
| O-01 Telemetry flood | Fixed | Telemetry is sent only while linked: once when the link comes up, then every 200 ms (it is now also the link reply). |
| O-02 Sender not checked | Fixed, one gap left | The receive callback accepts only drive packets whose source MAC equals the stored controller MAC. MAC addresses can still be spoofed (see N-06). |
| O-03, O-04 Pairing | Improved | The car now sends its acknowledgement directly to the captured controller and waits for the controller's confirmation before saving. |
| O-08 Length check | Fixed | Both receive callbacks require the exact packet size and the current protocol version. |
| N-07, O-06 Version and telemetry type | Fixed | Every packet starts with `PROTOCOL_VERSION` (2). Telemetry has its own type (`PKT_TELEMETRY`). |
| N-03 Blocking battery read | Fixed | `Battery.ino` takes one reading every 10 ms and keeps a 16-sample running average. |
| O-11 Battery reading | Fixed | `analogReadMilliVolts()` averaged over 16 samples, times 2. |
| Drive channel | New | The controller sends its drive channel during pairing and the car saves it. Older pairings without a saved channel fall back to the MAC rule. |
| O-12 Flash read per send | Fixed | The MAC is cached in RAM at startup. |
| O-14, N-02 Binding timeout | Fixed | The car waits up to 60 s for a bind request, then up to 15 s for the handshake. Either timeout restarts it. |
| O-09 Pin maps and V4 LED | Fixed | One board line at the top of `ESP_NOW_CAR.ino` selects V2, V3 or V4. The V4 WS2812 LED is driven with `rgbLedWrite()`. |
| O-17, N-05 Dead code, flash write | Fixed | `SendBindingComplete` and the binding send callback are gone. The flash write result is checked. |
| O-05 Race on received packet | Fixed | The callback copies the packet under a lock (`rxMux`) and the loop takes its own copy under the same lock. |
| N-04 Phantom link at boot | Fixed | The car does not count as linked until its first valid packet. |
| N-08 Peer added in callback | Fixed | The binding callback only copies the MAC and sets a flag. The loop prints it and adds the peer. |
| O-07, O-10, O-13, O-15, O-16, O-18 | Unchanged | See Section 12. |

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
| `BOARD_VERSION_STR` | Printed at boot |

**Revision selection:** the line `#define HARDWARE_V2` at the top of `ESP_NOW_CAR.ino` picks the board; change it to `HARDWARE_V3` or `HARDWARE_V4`. A build that passes `-DHARDWARE_Vx` (the GitHub flasher build) overrides that line. Any other value (for example `HARDWARE_V1`) falls back to V2. `HardwareConfig.h` stops the build with an error if no board is chosen.

## 5. Software Structure

| File | Role |
| --- | --- |
| `ESP_NOW_CAR.ino` | Globals, `setup()`, `loop()` dispatch |
| `Protocol.h` | Packet struct, packet types, version, binding channel, telemetry rate. Identical copy in the controller repo. |
| `Battery.ino` | Non-blocking battery average (`updateBattery()`, `batteryMillivolts()`) |
| `setupNormalMode.ino` | Radio, peer, receive callback, PWM and servo init |
| `loopNormalMode.ino` | Link status, telemetry, mismatch reply, command handling, failsafe, LED |
| `setupBindingMode.ino` | Radio and callbacks for pairing |
| `loopBindingMode.ino` | Handshake, timeout, flash write, restart |
| `Storage.ino` | Reads and writes the controller MAC (6 bytes) and drive channel |
| `StatusLed.ino` | `setStatusLed()`: the one place that knows the LED type; writes only on change |
| `HardwareConfig.h` | Pin and revision macros |

Built with Arduino-ESP32 core 3.3.12. The design is a cooperative loop. ESP-NOW callbacks run in the Wi-Fi task and pass data to `loop()` through globals.

| Shared variable | `volatile` | Written by | Read by |
| --- | --- | --- | --- |
| `NewDataFlag` | Yes, plus `rxMux` | Receive callback | Normal loop |
| `ReceivingData` | Guarded by `rxMux` | Receive callback | Normal loop (takes a copy) |
| `HandshakeConfirmed` | Yes | Binding receive callback | Binding loop |
| `ReceiveBindingComplete` | Yes | Binding receive callback | Binding loop |
| `capturedRemoteMac`, `capturedChannel` | No; written once before `ReceiveBindingComplete` is set | Binding receive callback | Binding loop |
| `MismatchFlag` | Yes | Normal receive callback | Normal loop |

Modes change only by restart, except that Normal setup falls through to Binding setup when no valid MAC is stored.

## 6. Communication Protocol

### 6.1 Radio

- Wi-Fi station mode, 19.5 dBm, ESP-NOW, no encryption, default PHY rate.
- **Binding channel:** 1 (`BINDING_CHANNEL`).
- **Driving channel:** the channel the controller sent during pairing (1, 6 or 11), saved in flash. A pairing made by older firmware has no saved channel; then the old rule applies: last byte of the controller's MAC modulo 3: 0 → channel 1, 1 → channel 6, 2 → channel 11.

### 6.2 Packet

One packed struct, defined in `Protocol.h`, is used in both directions (27 bytes: 3 bytes plus six 32-bit ints). A packet is accepted only if it is exactly 27 bytes and starts with `PROTOCOL_VERSION` (2). The old 25-byte packet (version 1) is ignored.

| Field | Type | Meaning |
| --- | --- | --- |
| `protocolVersion` | uint8 | Always `PROTOCOL_VERSION` |
| `packetType` | uint8 | See 6.3 |
| `channel` | uint8 | Bind request, acknowledgement and confirmation: the drive channel. Otherwise 0. |
| `motor_A1`, `motor_A2` | int | Motor A PWM, 0–255 |
| `motor_B1`, `motor_B2` | int | Motor B PWM, 0–255 |
| `weapon` | int | Servo angle, 0–180 |
| `battery` | int | Battery voltage in mV (telemetry only) |

### 6.3 Modes and messages

| Type | Name | From, to | When |
| --- | --- | --- | --- |
| 0 `PKT_BIND_REQUEST` | Bind request | Controller to broadcast, channel 1. Carries the drive channel. | Repeated by the controller while binding |
| 1 `PKT_DRIVE` | Drive | Controller to car | 20 ms active, 50 ms idle, 100 ms not linked (controller side) |
| 2 `PKT_BIND_ACK` | Bind acknowledgement | Car to captured controller, unicast. Echoes the drive channel. | Every 100 ms until confirmed |
| 3 `PKT_BIND_CONFIRM` | Bind confirmed | Controller to car, unicast. Carries the drive channel. | Every 100 ms for 1.5 s after the acknowledgement |
| 4 `PKT_TELEMETRY` | Telemetry | Car to controller | On link-up, then every 200 ms while linked. Also once a second while the controller is sending another protocol version. |

The controller shows CONNECTED only while telemetry keeps arriving, so the telemetry doubles as the car's "I am listening" reply.

## 7. Operating Modes

### 7.1 Boot

1. Start Serial at 115200. Set 11 dB attenuation on the battery pin. Wait 50 ms.
2. Drive all four motor pins and the weapon pin LOW as outputs.
3. Print the board name. Turn the status LED off and set the bind pin to pull-up.
4. Wait 50 ms and read the bind pin. LOW: Binding Mode. HIGH: Normal Mode.

### 7.2 Normal setup

1. Read the controller MAC from flash. If missing or all zeros, fall back to Binding Mode.
2. Cache the MAC in RAM. Read the saved drive channel; if none is saved, work it out from the MAC.
3. Initialize ESP-NOW (restart on failure), set the channel, add the controller as a unicast peer.
4. Receive callback: ignore packets not from the cached MAC. From the cached MAC, a packet of the wrong size or version sets `MismatchFlag`. A current drive packet is copied and `NewDataFlag` set under the `rxMux` lock.
5. Attach PWM: four motor pins at 5 kHz, 8-bit. Servo pin at 50 Hz, 16-bit, set to 0° (duty 3276).

### 7.3 Normal loop

Each pass:

0. **Battery.** `updateBattery()` takes one reading if 10 ms have passed.
1. **Command handling** (if `NewDataFlag` is set): take a copy of the packet under `rxMux`, clear the flag, mark that a packet has been received, record the time, then apply 7.4.
2. **Link status.** Linked means at least one packet has been received and the last one was processed within the last 250 ms.
3. **Telemetry.** On the rising edge of linked, and then every 200 ms while linked: send the battery average (16 readings times 2, capped at 0–4200 mV) to the cached MAC as `PKT_TELEMETRY`.
4. **Mismatch.** If `MismatchFlag` is set: clear it, note the time, and at most once a second print a warning and send one telemetry packet so the controller can show the mismatch.
5. **Failsafe.** If not linked, set all four motor outputs to 0 and blink the LED: 500 ms toggle normally, 150 ms toggle if a mismatched packet arrived in the last 1.5 s. If linked, hold the LED on.

### 7.4 Command handling

1. Clamp each motor value to 0–255.
2. Values below 15 become 0 (deadband).
3. If both inputs of one motor are above 0, set both to 0.
4. Write the four PWM values.
5. Clamp the weapon angle to 0–180 and map it to a servo duty of 3276 to 6553 (about 1 ms to 2 ms at 50 Hz).

Motors are driven sign-magnitude: one input carries PWM and the other is 0. Both at 0 is coast. There is no brake.

### 7.5 Binding setup

Station mode, 19.5 dBm, ESP-NOW (restart on failure), channel 1, broadcast peer.

- **Receive callback** (current protocol only): the first bind request with a valid drive channel is captured. Its sender MAC and channel are copied, then `ReceiveBindingComplete` is set. A bind confirmation from the captured MAC with the same channel sets `HandshakeConfirmed`. The callback does nothing else.

### 7.6 Binding loop

1. Toggle the LED every 50 ms.
2. Print flag states to Serial every 500 ms.
3. Once, right after capture: print the controller MAC and add it as a unicast peer on channel 1.
4. If no controller is captured within 60 s of the loop first running, restart.
5. If captured but not confirmed within 15 s of capture, restart.
6. After capture and until confirmed, send a bind acknowledgement (echoing the channel) to the captured MAC every 100 ms.
7. On confirmation: write the MAC and drive channel to flash (an error is printed if the write fails), turn the LED off, wait 500 ms, restart.

### 7.7 States

| State | Entry | Exit |
| --- | --- | --- |
| Binding | Bind button held at power-up, or no valid stored MAC | Confirmed pair then restart, or timeout (60 s no request, 15 s no confirmation) then restart |
| Normal, not linked | Normal setup done, no packet for more than 250 ms | A valid drive packet |
| Normal, linked | Valid packet within 250 ms | 250 ms of silence |

## 8. Functional Requirements

| ID | Requirement |
| --- | --- |
| CF-01 | All motor and weapon pins shall be driven LOW as outputs immediately after reset. |
| CF-02 | The car shall enter Binding Mode if the bind button is held at power-up. |
| CF-03 | The car shall enter Binding Mode if no non-zero controller MAC is stored. |
| CF-04 | Otherwise the car shall start in Normal Mode. |
| CF-05 | If ESP-NOW fails to initialize, the car shall restart. |
| CF-06 | The car shall ignore any packet that is not exactly the current packet size with the current `PROTOCOL_VERSION`. |
| CF-10 | In Binding Mode the car shall listen on channel 1 for a bind request with a valid drive channel and capture the first sender's MAC and channel. |
| CF-11 | After capture the car shall send bind acknowledgements to that MAC every 100 ms, echoing the channel. |
| CF-12 | Pairing shall complete only when a bind confirmation with the same channel arrives from the captured MAC. |
| CF-13 | On completion the car shall store the MAC (namespace `rc_car`, key `controller_mac`) and drive channel (key `drive_ch`), wait 500 ms and restart. |
| CF-14 | The car shall restart if no bind request is captured within 60 s, or if no confirmation arrives within 15 s of capture. |
| CF-15 | Binding Mode shall be shown by a 10 Hz LED toggle. |
| CF-20 | Normal Mode shall use the saved drive channel, or, if none is saved, the channel derived from the stored MAC (1, 6 or 11). |
| CF-21 | The car shall act only on current-protocol drive packets from the stored MAC. |
| CF-22 | Motor values shall be clamped to 0–255, with values below 15 treated as 0. |
| CF-23 | If both inputs of a motor are non-zero, both shall be set to 0. |
| CF-24 | The weapon angle shall be clamped to 0–180 and output as a 50 Hz servo pulse of about 1–2 ms. |
| CF-25 | Motor PWM shall be 5 kHz at 8-bit resolution. |
| CF-30 | If no valid packet arrives for more than 250 ms, or none has arrived since boot, all four motor outputs shall be set to 0. |
| CF-31 | The LED shall be solid on while linked and toggle every 500 ms while not linked, or every 150 ms while its controller is sending another protocol version. |
| CF-32 | The failsafe shall be evaluated on every pass of the main loop. |
| CF-40 | While linked the car shall send battery voltage to the stored MAC when the link comes up and every 200 ms. |
| CF-41 | Voltage shall be the running average of the last 16 calibrated readings (one every 10 ms, without blocking the loop) times 2, capped to 0–4200 mV. |
| CF-42 | Telemetry shall use `PKT_TELEMETRY`. |
| CF-43 | No telemetry shall be sent while the link is down, except one packet a second while the stored controller is sending another protocol version. |
| CF-50 | With `DEBUG_MODE` true the car shall print status and warnings to Serial at 115200 baud. |

## 9. Key Parameters

| Parameter | Value |
| --- | --- |
| Failsafe timeout | 250 ms |
| Protocol version | 2 |
| Telemetry period | 200 ms |
| Motor deadband | 15 of 255 |
| Motor PWM | 5 kHz, 8-bit |
| Servo PWM | 50 Hz, 16-bit, duty 3276–6553 |
| Battery averaging | Last 16 samples, one every 10 ms |
| Battery cap | 4200 mV |
| Bind request wait | 60 s |
| Handshake timeout (after capture) | 15 s |
| Acknowledgement interval | 100 ms |
| Binding LED toggle | 50 ms |
| Link-lost LED toggle | 500 ms |
| Mismatch LED toggle | 150 ms |
| Post-pairing delay | 500 ms |
| TX power | 19.5 dBm |
| Binding channel | 1 |
| Driving channels | 1, 6, 11 (saved at pairing) |

## 10. Storage

- `Preferences` namespace `rc_car`, two records: `controller_mac` (6 bytes) and `drive_ch` (1 byte, 1, 6 or 11). Older pairings may have no `drive_ch`.
- A missing or short record counts as unpaired and the output is zeroed.
- The MAC and channel are written only after a confirmed pairing. A failed write prints an error and the old pairing is kept.
- The only way to change pairing is to pair again.

## 11. Safety Behavior

- Outputs are low from reset until PWM is attached.
- Motors stop within about 250 ms of link loss, and also while the controller menu is open (the controller sends nothing then).
- Both bridge inputs can never be driven at once.
- The servo holds its last angle on link loss and sits at 0° from boot until the first packet. Weapon behavior is deferred to next year.
- There is no low-battery cutoff. The user watches the voltage on the controller.

## 12. Remaining Observations

Severity is my estimate. Themes follow your priorities: Clarity, Pairing, Latency.

**Resolved since Draft 1.0:** N-01 (the controller now sends mode 3). **Resolved Nov 12:** N-02 binding window, O-05 race, O-09 board select and V4 LED, N-04 phantom link, N-05 flash write check, N-08 peer added in callback, and with protocol version 2: N-07 version byte, O-06 telemetry type, N-03 blocking battery read.

| ID | Severity | Theme | Status | Observation |
| --- | --- | --- | --- | --- |
| O-07 | Medium | Clarity | Deferred | **Servo holds its angle on link loss** and starts at 0°. Deferred with the weapon design. |
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
