/**
 * ==============================================================================
 * loopNormalMode.ino (Car Receiver - Restored & Verified Baseline)
 * ==============================================================================
 */
void loopNormalMode(){
  static uint32_t rxPreviousMillis = 0; 
  static uint32_t lastTransmitTime = 0;
  static unsigned long buttonPressedTime = 0; 
  static bool lastButtonState = HIGH;
  
  // --- BUTTON LONG-PRESS CAPTURE ENGINE ---
  // FIXED: Strictly matches the "BindPin" declaration from your main tab configuration
  bool currentButtonState = digitalRead(BindPin);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    buttonPressedTime = millis();
    Serial.println("[INPUT] Bind button engaged down. Keep holding to pair...");
  } 
  else if (currentButtonState == LOW && lastButtonState == LOW) {
    if (millis() - buttonPressedTime > 3000) { 
      Serial.println("\n[SYS] Long Press Verified! Switching runtime to Pairing Mode...");
      ledcWrite(A1Pin, 0); ledcWrite(A2Pin, 0); ledcWrite(B1Pin, 0); ledcWrite(B2Pin, 0);
      ReceiveBindingComplete = false; 
      SendBindingComplete = false; 
      NormalMode = false; 
      setupBindingMode(); 
      return; 
    }
  }
  lastButtonState = currentButtonState;

  // --- TELEMETRY BROADCAST ENGINES ---
  if((millis() - lastTransmitTime) > 5000){
    lastTransmitTime = millis();
    SendingData.protocolMode = 1; 
    SendingData.battery = 3700; // Static dummy bypasses ADC2 lock
    
    uint8_t remoteMac[6];
    if (readStoredMac(remoteMac)) {
      if (DEBUG_MODE) Serial.printf("[COMM-TX] Telemetry burst out. Battery Sim: %dmV\n", SendingData.battery);
      esp_now_send(remoteMac, (uint8_t *)&SendingData, sizeof(SendingData));
    }
  }

  // --- CONTROLLER COMMAND INGESTION AND HARDWARE OUTS ---
  if(NewDataFlag == true){
    NewDataFlag = false; 
    rxPreviousMillis = millis();    
    digitalWrite(STATUS_LED, LOW); // LED SOLID ON when receiving data stream

    if (DEBUG_MODE) {
      Serial.printf("[DATA-RX] Motors A1/A2: %4d/%4d | Motors B1/B2: %4d/%4d | Weapon: %3d\n",
                    ReceivingData.motor_A1, ReceivingData.motor_A2, 
                    ReceivingData.motor_B1, ReceivingData.motor_B2, ReceivingData.weapon);
    }


    if((ReceivingData.motor_A1!=0 && ReceivingData.motor_A2!=0) || (ReceivingData.motor_B1!=0 && ReceivingData.motor_B2!=0)){
      // Ground the independent channels to kill power safely
      ledcWrite(0, 0); ledcWrite(1, 0); ledcWrite(2, 0); ledcWrite(3, 0);      
      Serial.println("[CRIT] Directional Conflict Found! Motors killed for safety.");
    } else {
      // Write your speed constraints directly to the isolated hardware channel registers!
      ledcWrite(0, constrain(ReceivingData.motor_A1, 0, 255)); // Channel 0 -> Pin 4 (A1)
      ledcWrite(1, constrain(ReceivingData.motor_A2, 0, 255)); // Channel 1 -> Pin 2 (A2)
      ledcWrite(2, constrain(ReceivingData.motor_B1, 0, 255)); // Channel 2 -> Pin 1 (B1)
      ledcWrite(3, constrain(ReceivingData.motor_B2, 0, 255)); // Channel 3 -> Pin 0 (B2)  
      weapon.write(ReceivingData.weapon);
    }  

  }

  // --- CONNECTION TIMEOUT SAFETY BREAKS ---
  if((millis() - rxPreviousMillis) > 500){ 
    ledcWrite(A1Pin, 0); ledcWrite(A2Pin, 0);
    ledcWrite(B1Pin, 0); ledcWrite(B2Pin, 0);      
    
    static unsigned long lastSlowBlink; 
    static bool slowLedState = false;
    if(millis() - lastSlowBlink > 500) {
      lastSlowBlink = millis(); 
      slowLedState = !slowLedState;
      digitalWrite(STATUS_LED, slowLedState ? LOW : HIGH); 
      if (DEBUG_MODE) Serial.println("[WARN] Signal lost from controller for >500ms. Failsafe locks active.");
    }
  } 
}


