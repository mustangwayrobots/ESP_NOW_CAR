/**
 * ==============================================================================
 * ESP-NOW RC Receiver Core (Multi-Hardware Platform)  
 * ==============================================================================
 */
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h> 
#include "HardwareConfig.h" // Pulls in version-specific pin maps

Preferences nvStorage; 

const bool DEBUG_MODE = true; 

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
  Serial.printf("[SYS] %s Mounted\n", BOARD_VERSION_STR);
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