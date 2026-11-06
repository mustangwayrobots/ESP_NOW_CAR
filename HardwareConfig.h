/**
 * ==============================================================================
 * HardwareConfig.h - Multi-Revision Pin & Radio Layout Definitions
 * ==============================================================================
 */
#pragma once

#ifdef HARDWARE_V3
  #define BOARD_VERSION_STR "ESP32-C3 RC Receiver V3 Layout"
  #define WIFI_CHANNEL      1
  #define STATUS_LED        8   
  #define A1Pin             1      
  #define A2Pin             0      
  #define B1Pin             20      
  #define B2Pin             10     
  #define BindPin           5      
  #define WeaponPin         4      
  #define BATTERY_ADC_PIN   3 

#elif defined(HARDWARE_V4)
  #define BOARD_VERSION_STR "ESP32-C3 RC Receiver V4 Layout"
  #define WIFI_CHANNEL      1
  #define STATUS_LED        8   
  #define A1Pin             1      
  #define A2Pin             0      
  #define B1Pin             20      
  #define B2Pin             10     
  #define BindPin           5      
  #define WeaponPin         4      
  #define BATTERY_ADC_PIN   3 

#else
  // Default / HARDWARE Version 2.0 (Legacy fallback)
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