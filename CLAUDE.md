# CLAUDE.md

This file gives Claude Code the rules and commands for this repository.

ESP NOW CAR: firmware for the Mustang Way Robots high school RC car (ESP32-C3, DRV8833 dual H-bridge, two N20 motors, TP4056 + boost converter, bind button, status LED).
This folder is the clone of github.com/mustangwayrobots/ESP_NOW_CAR (the club's account; the user, hogfanboy, works on it as a collaborator).
Pushing to `main` makes GitHub Actions build every board version and publish the web flasher to https://mustangwayrobots.github.io/ESP_NOW_CAR/flash/ , which students use. Treat `main` as live.

**Sister repo:** the handheld controller is `../ESP_NOW_Controller` (github.com/mustangwayrobots/ESP_NOW_Controller).

## Docs (one job each)
- `README.md`: for students and end users (what it is, flashing, pairing, building).
- `docs/Car_Firmware_FSD.md` and `docs/Car_Hardware_FSD.md`: the spec and settled truth. Read only the part you need.
- `CHANGELOG.md`: user-facing changes by date, newest first. Add an entry for any change students would notice.
- `CLAUDE.md`: permanent rules and commands only.

## Rules
- `Protocol.h` must stay byte-for-byte identical to `../ESP_NOW_Controller/Protocol.h`. If you change it, change both, bump the protocol version, and note it in both CHANGELOGs.
- Pin numbers live only in `HardwareConfig.h`. The board is chosen with the `#define HARDWARE_Vx` line at the top of `ESP_NOW_CAR.ino` (V2, V3, V4). CI finds the boards by scanning `HardwareConfig.h`, so a new `#elif defined(HARDWARE_V5)` is all it takes to add a board to the flasher.
- Web flashing must not erase the saved pairing: keep the separate-pieces flashing and `new_install_prompt_erase: true` in `.github/workflows/build.yml`.
- `build.yml` and `flash/index.html` are shared, generic copies used by all Mustang Way Robots projects. Keep them identical across repos.
- No extra libraries are used. If one is added, put a pinned copy in `libraries/` (CI uses only that).
- Commit as hogfanboy (noreply), never a Gmail address. Don't push to `main` unless the user asks.

## Build (same toolchain as CI: esp32 core 3.3.12, board MakerGO ESP32 C3 SuperMini)
```bash
A="/c/Users/hogfa/AppData/Local/Programs/arduino-cli/arduino-cli.exe"   # not on PATH
"$A" compile --fqbn esp32:esp32:makergo_c3_supermini --build-property "compiler.cpp.extra_flags=-DHARDWARE_V4" --build-property "compiler.c.extra_flags=-DHARDWARE_V4" --build-path build/v4 .
```
This PC has esp32 core 3.3.13 installed (CI pins 3.3.12); both compile. `build/` is git-ignored. The user uploads to boards from Arduino IDE or the web flasher.
