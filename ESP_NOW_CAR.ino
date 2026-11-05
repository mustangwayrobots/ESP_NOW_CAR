/**
 * ==============================================================================
 * ESP-NOW RC Receiver Core (ESP32-C3 Platform - V1 Hardware Layout)  
 * ==============================================================================
 */
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h> 

Preferences nvStorage; 

const bool DEBUG_MODE = true; 

#pragma once

#ifdef BOARD_V3
  #define WIFI_CHANNEL 1
  #define STATUS_LED   8   
  #define A1Pin     1      
  #define A2Pin     0      
  #define B1Pin     20      
  #define B2Pin     10     
  #define BindPin   5      
  #define WeaponPin 4      
  #define BATTERY_ADC_PIN 3 
#elif defined(BOARD_V4)
  #define WIFI_CHANNEL 1
  #define STATUS_LED   8   
  #define A1Pin     1      
  #define A2Pin     0      
  #define B1Pin     20      
  #define B2Pin     10     
  #define BindPin   5      
  #define WeaponPin 4      
  #define BATTERY_ADC_PIN 3 
#else
  // Default / Version 2.0 (Legacy)
  #define WIFI_CHANNEL 1
  #define STATUS_LED   8   
  #define A1Pin     1      
  #define A2Pin     0      
  #define B1Pin     20      
  #define B2Pin     10     
  #define BindPin   5      
  #define WeaponPin 4      
  #define BATTERY_ADC_PIN 3 
#endif

bool NormalMode             = true;
volatile bool NewDataFlag   = false; // Marked volatile for safe cross-task access
bool SendBindingComplete    = false; 
bool ReceiveBindingComplete = false;

struct __attribute__((packed)) RC_Packet_Structure {
  uint8_t protocolMode; // 0 = Bind Request, 1 = Normal Drive, 2 = Bind Acknowledgement
  int motor_A1; int motor_A2;
  int motor_B1; int motor_B2;
  int weapon;   int battery;
};

RC_Packet_Structure SendingData;   
RC_Packet_Structure ReceivingData; 
uint8_t capturedRemoteMac[6];      

void setupNormalMode();   void setupBindingMode();
void loopNormalMode();    void loopBindingMode();
bool readStoredMac(uint8_t* macOut);
void writeStoredMac(const uint8_t* macIn);

void setup() {
  Serial.begin(115200); 
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db); 
  delay(50); 

  // --- IMMEDIATE MOTOR KILL / SAFETY PINS INIT ---
  // Force motor pins to OUTPUT and pull them LOW right at power-up 
  // so no wheels spin while waiting for a connection/binding.
  pinMode(A1Pin, OUTPUT); digitalWrite(A1Pin, LOW);
  pinMode(A2Pin, OUTPUT); digitalWrite(A2Pin, LOW);
  pinMode(B1Pin, OUTPUT); digitalWrite(B1Pin, LOW);
  pinMode(B2Pin, OUTPUT); digitalWrite(B2Pin, LOW);
  pinMode(WeaponPin, OUTPUT); digitalWrite(WeaponPin, LOW);

  Serial.println("\n=============================================");
  Serial.println("[SYS] ESP32-C3 RC Receiver V1 Firmware Mounted");
  Serial.println("=============================================");

  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH); 
  pinMode(BindPin, INPUT_PULLUP);

  // --- IMMEDIATE POWER-UP BIND CHECK ---
  delay(50); // Small debounce stabilization window
  if (digitalRead(BindPin) == LOW) {
    Serial.println("[SYS] Bind button detected LOW at startup! Entering Binding Mode...");
    NormalMode = false;
    setupBindingMode();
  } else {
    Serial.println("[SYS] Booting straight to Normal Mode.");
    setupNormalMode();      
  }
}

void loop() {
  yield(); 
  if (NormalMode) {
    loopNormalMode(); 
  } else {
    loopBindingMode();
  }
}