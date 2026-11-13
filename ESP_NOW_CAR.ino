/**
 * ==============================================================================
 * ESP-NOW RC Receiver Core (Multi-Hardware Platform)    
 * ==============================================================================
 */
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h> 

// --- Board choice: change HARDWARE_V2 below to HARDWARE_V3 or HARDWARE_V4 ---
// (A build that passes -DHARDWARE_Vx, like the GitHub flasher build, overrides this.)
#if !defined(HARDWARE_V2) && !defined(HARDWARE_V3) && !defined(HARDWARE_V4)
  #define HARDWARE_V2
#endif
#include "HardwareConfig.h" 
#include "Protocol.h"        // Packet layout shared with the controller (keep both copies identical)

Preferences nvStorage; 

const bool DEBUG_MODE = true; 

bool NormalMode                     = true;
volatile bool NewDataFlag           = false; 
volatile bool ReceiveBindingComplete = false; // Set by the binding receive callback when a controller is captured
volatile bool HandshakeConfirmed    = false;  // Set true when the controller confirms (PKT_BIND_CONFIRM)
volatile bool MismatchFlag          = false;  // Set when our controller sends a packet in another protocol version

RC_Packet_Structure SendingData;   
RC_Packet_Structure ReceivingData;  // Written by the radio callback, read by the loop (guard with rxMux)
uint8_t capturedRemoteMac[6];       // Written once by the binding callback before ReceiveBindingComplete is set
uint8_t capturedChannel = 0;        // Drive channel asked for in that bind request
uint8_t cachedRemoteMac[6];        

// Lock shared by the radio callback and the loop, so a packet is never read half-copied
portMUX_TYPE rxMux = portMUX_INITIALIZER_UNLOCKED;

void setupNormalMode();   void setupBindingMode();
void loopNormalMode();    void loopBindingMode();
bool readStoredMac(uint8_t* macOut);
uint8_t readStoredChannel();
bool writeStoredPairing(const uint8_t* macIn, uint8_t channel);
void initStatusLed();
void setStatusLed(bool on);

void setup() {
  Serial.begin(115200); 
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db); 
  delay(50); 

  pinMode(A1Pin, OUTPUT); digitalWrite(A1Pin, LOW);
  pinMode(A2Pin, OUTPUT); digitalWrite(A2Pin, LOW);
  pinMode(B1Pin, OUTPUT); digitalWrite(B1Pin, LOW);
  pinMode(B2Pin, OUTPUT); digitalWrite(B2Pin, LOW);
  pinMode(WeaponPin, OUTPUT); digitalWrite(WeaponPin, LOW);

  Serial.println("\n=============================================");
  Serial.printf("[SYS] %s Mounted\n", BOARD_VERSION_STR);
  Serial.println("=============================================");

  initStatusLed();   // LED starts OFF (StatusLed.ino)
  pinMode(BindPin, INPUT_PULLUP);

  delay(50); 
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
