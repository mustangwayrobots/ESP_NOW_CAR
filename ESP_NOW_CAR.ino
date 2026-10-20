/**
 * ================ =============================================================
 * ESP-NOW RC Receiver Core (ESP32-C3 Platform - V1 Hardware Layout)
 * Production Clean Version - Locked Structural Layout
 * ==============================================================================
 */
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ESP32Servo.h>
#include <Preferences.h> 

Servo weapon;
Preferences nvStorage; 

const bool DEBUG_MODE = true; 

#define WIFI_CHANNEL 1
#define STATUS_LED   8   

#define A1Pin     4      
#define A2Pin     2      
#define B1Pin     1      
#define B2Pin     0      
#define BindPin   6      
#define WeaponPin 7      
#define BATTERY_ADC_PIN 5 

bool NormalMode             = true;
bool TXConnectionStatus     = false;
bool NewDataFlag            = false;
bool SendBindingComplete    = false; 
bool ReceiveBindingComplete = false;

// THE EXACT MATCHING LOCKED STRUCTURE BLUEPRINT
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
  delay(500); 
  Serial.println("\n=============================================");
  Serial.println("[SYS] ESP32-C3 RC Receiver V1 Firmware Mounted");
  Serial.println("=============================================");

  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH); 
  pinMode(BindPin, INPUT_PULLUP);

  Serial.println("[SYS] Booting straight to Normal Mode. Hold Bind 3s to Pair.");
  setupNormalMode();      
}

void loop() {
  yield(); 
  if (NormalMode) loopNormalMode(); else loopBindingMode();
}
