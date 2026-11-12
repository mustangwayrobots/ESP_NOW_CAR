/**
 * ==============================================================================
 * HardwareConfig.h - Multi-Revision Pin & Radio Layout Definitions
 * ==============================================================================
 * The board is chosen at the top of ESP_NOW_CAR.ino (one line to edit).
 * A build that passes -DHARDWARE_Vx (like the GitHub flasher build) overrides it.
 *
 * Status LED type, one per board:
 *   STATUS_LED_ACTIVE_LOW  plain LED, ON when the pin is LOW
 *   STATUS_LED_RGB         WS2812 RGB LED (needs rgbLedWrite, not digitalWrite)
 */
#pragma once

#if defined(HARDWARE_V3)
  #define BOARD_VERSION_STR "ESP32-C3 RC Receiver V3 Layout"
  #define WIFI_CHANNEL      1
  #define STATUS_LED        8
  #define STATUS_LED_ACTIVE_LOW
  #define A1Pin             1
  #define A2Pin             0
  #define B1Pin             7
  #define B2Pin             4
  #define BindPin           6
  #define WeaponPin         10
  #define BATTERY_ADC_PIN   3

#elif defined(HARDWARE_V4)
  #define BOARD_VERSION_STR "ESP32-C3 RC Receiver V4 Layout"
  #define WIFI_CHANNEL      1
  #define STATUS_LED        10  // WS2812 RGB LED on the ESP32-C3 Zero module
  #define STATUS_LED_RGB
  #define A1Pin             6
  #define A2Pin             7
  #define B1Pin             3
  #define B2Pin             4
  #define BindPin           0
  #define WeaponPin         5
  #define BATTERY_ADC_PIN   1

#elif defined(HARDWARE_V2)
  #define BOARD_VERSION_STR "ESP32-C3 RC Receiver V2.0 (Legacy)"
  #define WIFI_CHANNEL      1
  #define STATUS_LED        8
  #define STATUS_LED_ACTIVE_LOW
  #define A1Pin             1
  #define A2Pin             0
  #define B1Pin             20
  #define B2Pin             10
  #define BindPin           5
  #define WeaponPin         4
  #define BATTERY_ADC_PIN   3

#else
  #error "No board chosen. Set the board line at the top of ESP_NOW_CAR.ino."
#endif
