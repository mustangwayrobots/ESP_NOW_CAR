# Car Hardware – Functional Specification

Nov 11, 2026 · @Hog

This document describes the 2-wheel RC car board as drawn in your EasyEDA project (revisions V2, V3 and V4, with V4 the target) and lists hardware items worth checking. It is one of four specifications: Car Firmware, Controller Firmware, Car Hardware (this one) and Controller Hardware. Everything here comes from your project file and screenshots. Values not in the file are marked **\[?\]**.

## 1. Scope

- **Covers:** the car PCB, its power path, motor driver, servo output, battery sense, buttons and LED.
- **Does not cover:** firmware behavior (Car Firmware FSD) or the controller board (Controller Hardware FSD).
- **Status:** Draft 1.0. Board V4.0, dated Nov 4, 2026, 79 × 35 mm with four 3 mm mounting holes.

## 2. Revisions at a Glance

| Item | V2 | V3 | V4 (target) |
| --- | --- | --- | --- |
| MCU | ESP32-C3 SuperMini | ESP32-C3 SuperMini | ESP32-C3 Zero module |
| Boost converter | Mini DC-DC boost module | MT3608 | SX1308 |
| Power switch | SS12F23G5 | SS12F23G5 | SS12F44G4 |
| Status LED | On-board, GPIO8, active low | On-board, GPIO8, active low | WS2812 on the Zero module, GPIO10 |
| Motor driver | DRV8833 | DRV8833 | DRV8833 |
| Charger | TP4056 module | TP4056 module | TP4056 module |
| 3.3 V regulator | not drawn | HT7833 (TO-92) | HT7833 (TO-92) |

## 3. Hardware Functions

| ID | Requirement |
| --- | --- |
| HW-01 | The board shall run from one 1S Li-ion/LiPo cell (3.0 to 4.2 V) connected to a 2-pin JST-PH2.0 connector (CN1, B+ and B−). |
| HW-02 | The cell shall be charged through a TP4056 USB module. |
| HW-03 | A slide switch shall connect the charger and cell output to the load rail (`VBAT_SW`). |
| HW-04 | A boost converter shall produce the motor and servo rail (`VM`), set to 5.5 V by its trimmer. |
| HW-05 | A 3.3 V LDO shall feed the MCU. |
| HW-06 | A DRV8833 dual H-bridge, supplied from `VM`, shall drive two N20 motors on 2-pin headers (Motor A and Motor B). |
| HW-07 | A 3-pin header shall give the weapon servo a signal pin, `VM` and ground. |
| HW-08 | A divider shall scale `VBAT_SW` by 2:1 into an ADC pin. |
| HW-09 | A tactile button shall pull the bind pin to ground. |
| HW-10 | A status LED shall be available to firmware. |

## 4. Power Path (V4)

1. Cell (CN1) → TP4056 module (B+ and B−). The module's OUT+ goes to the slide switch.
2. Slide switch (SW1) → `VBAT_SW`.
3. `VBAT_SW` feeds three loads: the SX1308 boost input, the HT7833 LDO input, and the battery-sense divider.
4. Boost output → `VM`. `VM` feeds the DRV8833 supply pins, its EEP (sleep) input and the servo header.
5. LDO output → `3.3V` → the Zero's 3V3 pin. The Zero's 5V pin is not connected on the board.

| Rail | Source | Level | Feeds |
| --- | --- | --- | --- |
| Cell | 1S pouch, 500 mAh, with protection | 3.0–4.2 V | TP4056 module |
| `VBAT_SW` | Cell through the slide switch | 3.0–4.2 V | Boost, LDO, divider |
| `VM` | Boost converter | 5.5 V (trimmer, verify) | DRV8833, servo |
| `3.3V` | HT7833 LDO | 3.3 V | MCU module |

**Rule of thumb for current:** at an assumed 85% boost efficiency and 3.7 V cell, every 1 A drawn from `VM` costs about 1.75 A from the cell. For a 500 mAh cell that is 3.5C.

## 5. Parts (V4)

| Ref | Part | Function | Notes |
| --- | --- | --- | --- |
| U6 | ESP32-C3-ZERO | MCU module | Has USB-C and a WS2812 LED |
| U4 | DRV8833 | Dual H-bridge | EEP tied to `VM`, so always enabled. Fault output (ULT) not connected. |
| U3 | SX1308 | Step-up converter | Output set by trimmer |
| U1 | HT7833, TO-92 | 3.3 V LDO | See H-05 |
| U2 | TP4056 module | Charger | Charge current set on the module |
| SW1 | SS12F44G4 | Power switch | Library lists 300 mA, 50 V (see H-06) |
| SW3 | 6 × 6 × 7.3 mm tactile | Bind button | To ground |
| CN1 | SH-PH2.0-2PZZ | Battery connector | 2 A rating |
| MotorA, MotorB | 2.54 mm 1×2 headers | Motor outputs | A-OUT1/2, B-OUT1/2 |
| P1 | 2.54 mm 1×3 header | Servo | Signal, `VM`, ground |
| R2, R3 | 10 kΩ | Battery divider | Ratio 2:1 |
| J1–J4 | 3 mm holes | Mounting |  |

No capacitors are drawn on the schematic, so any bulk or decoupling capacitance is whatever the modules carry **\[?\]** (see H-07).

## 6. Pin Assignments

These are decoded from the three car schematics. V2 matches the code. The firmware reads them from macros in `HardwareConfig.h`.

| Function | Macro | V2 | V3 | V4 |
| --- | --- | --- | --- | --- |
| Motor A input 1 | `A1Pin` | GPIO1 | GPIO1 | GP6 |
| Motor A input 2 | `A2Pin` | GPIO0 | GPIO0 | GP7 |
| Motor B input 1 | `B1Pin` | GPIO20 | GPIO7 | GP3 |
| Motor B input 2 | `B2Pin` | GPIO10 | GPIO4 | GP4 |
| Weapon servo | `WeaponPin` | GPIO4 | GPIO10 | GP5 |
| Bind button | `BindPin` | GPIO5 | GPIO6 | GP0 |
| Battery ADC | `BATTERY_ADC_PIN` | GPIO3 | GPIO3 | GP1 |
| Status LED | `STATUS_LED` | GPIO8 | GPIO8 | GPIO10 (WS2812) |

The ESP32-C3 has analog inputs on GPIO0 to GPIO5 only. The battery sense pin is on one of these in every revision. V3 and V4 stop using GPIO20 (UART0 RX) for a motor.

## 7. Battery Sense

- Divider: R2 (10 kΩ) from `VBAT_SW` to the ADC pin, R3 (10 kΩ) from the ADC pin to ground. The pin sees half the battery voltage.
- At 4.2 V the pin sees 2.1 V. At 3.0 V it sees 1.5 V. Both are inside the ADC range at 11 dB attenuation.
- The divider draws about 0.2 mA at 4.2 V whenever the switch is on.
- The reading is taken at the switched rail, so it shows voltage under load. It sags when the motors pull current.

## 8. Electrical Limits to Keep in Mind

| Item | Limit | Source |
| --- | --- | --- |
| DRV8833 | About 1.5 A continuous and 2 A peak per bridge, 2.7–10.8 V supply | General datasheet knowledge |
| N20 motor | 6 V rated. Stall current **\[?\]** | Not in the project |
| MG90S servo | 4.8–6 V, 5.5 V on `VM` is inside this | General knowledge |
| Slide switch SW1 | 300 mA, 50 V | Library entry in your project |
| Cell | 500 mAh protected. Peak discharge **\[?\]** | Not in the project |

## 9. Observations

Severity is my estimate.

| ID | Severity | Observation |
| --- | --- | --- |
| H-06 | High | **The power switch is rated 300 mA** in the library entry for the V4 part (SS12F44G4), but it carries the whole battery current into the boost converter. One motor and the servo can pull well over 1 A from the cell (see the rule of thumb in Section 4). An overloaded slide switch heats, gains resistance and drops voltage, which can reset the MCU. Check the datasheet and consider a higher-rated switch. |
| H-02 | Medium | **Peak current.** Two stalled N20 motors plus the servo on a 5.5 V boost can draw more than a 500 mAh protected cell supplies. That can trip the cell's protection circuit or brown out the 3.3 V rail. |
| H-01 | Medium | **Charge current.** TP4056 modules usually default to 1 A, which is 2C for a 500 mAh cell. Check the small programming resistor on the module ("122" is about 1 A, "242" is about 500 mA). |
| H-05 | Medium | **Regulator part is unclear.** The symbol says HT7833 (3.3 V) but the part's manufacturer field says "78L05 TO92" (a 5 V regulator, which cannot work from a 1S cell). Check the part actually fitted and its pin order. |
| H-07 | Medium | **No capacitors are drawn.** A LDO usually wants input and output capacitors, and a motor driver wants bulk capacitance on its supply. Check the LDO datasheet, and consider about 100 to 470 µF near the DRV8833 supply and 1 µF or more at the LDO. |
| H-03 | Low | **V4 3.3 V rail.** The LDO feeds the Zero's 3V3 pin while the 5V pin is unused. If USB is plugged in with the switch on, two regulators feed the same rail. Unplug the battery switch when using USB, or check the Zero's documentation. |
| H-08 | Low | **Motor driver is always enabled** (EEP on `VM`) and its fault output is not wired. Firmware cannot see an over-current fault. |
| H-09 | Low | **Boost setpoint is a trimmer.** Measure `VM` with a meter before connecting the motors or servo and confirm 5.5 V. |

## 10. Open Questions

1. Which regulator is actually fitted (H-05), and where does it get its pin order from?
2. Which slide switch is actually fitted, and is its current rating really 300 mA (H-06)?
3. N20 stall current from the datasheet or a meter. Optional for now.
4. TP4056 programming resistor marking (H-01).
5. Are there capacitors on the real board that the schematic does not show (H-07)?
