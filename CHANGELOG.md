# Changelog

All notable changes to the car firmware. Newest first.
The car and the controller must run the same **protocol version**.

## 2026-11-12

### Protocol version 2 (update the controller at the same time)
- Every packet now starts with a protocol version number. Packets from mismatched firmware are ignored.
- Battery reports (telemetry) have their own packet type and are sent every 200 ms while driving. The controller shows CONNECTED only while these arrive.
- The controller sends its drive channel during pairing, and the car saves it. Pairings made with older firmware keep working.
- Quick-blinking LED: the paired controller is running a different protocol version.
- Battery voltage is now read without pausing the car.

### Web flasher
- Web flashing no longer erases the saved pairing.

### Repository
- Specifications and images moved to `docs/`. Added `LICENSE` (MIT) and this changelog.

### Earlier on Nov 12
- Board select line for V2, V3 and V4; V4 WS2812 status LED.
- Safer pairing handshake with confirmation, longer pairing window, safe packet copying, no false link at boot.

Earlier history: see the git log.
