/**
 * ==============================================================================
 * loopNormalMode.ino (Car Receiver - Motors, Safety Clamps & Native Servo Flipper)
 * ==============================================================================
 */
void loopNormalMode(){
  static uint32_t rxPreviousMillis = 0; 
  static uint32_t lastTransmitTime = 0;
  
  // --- 1. BUTTON LONG-PRESS CAPTURE ENGINE (Pairing Trigger) ---
  bool currentButtonState = digitalRead(BindPin);
  static unsigned long buttonPressedTime = 0; 
  static bool lastButtonState = HIGH;
  
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    buttonPressedTime = millis();
  } 
  else if (currentButtonState == LOW && lastButtonState == LOW) {
    if (millis() - buttonPressedTime > 3000) { 
      ledcWrite(A1Pin, 0); ledcWrite(A2Pin, 0); ledcWrite(B1Pin, 0); ledcWrite(B2Pin, 0);
      NormalMode = false; 
      setupBindingMode(); 
      return; 
    }
  }
  lastButtonState = currentButtonState;

  // --- 2. TELEMETRY BROADCAST ENGINE (Battery Check) ---
  if((millis() - lastTransmitTime) > 5000){
    lastTransmitTime = millis();
    SendingData.protocolMode = 1; 

    uint32_t rawAdc = analogRead(BATTERY_ADC_PIN);
    uint32_t pinVoltageMv = (rawAdc * 3300) / 4095;
    SendingData.battery = pinVoltageMv * 2.0; 

    uint8_t remoteMac[6];
    if (readStoredMac(remoteMac)) {
      esp_now_send(remoteMac, (uint8_t *)&SendingData, sizeof(SendingData));
    }
  }

  // --- 3. PACKET INGESTION, SAFETY CLAMPS & NATIVE SERVO CONTROL ---
  if(NewDataFlag == true){
    NewDataFlag = false; 
    rxPreviousMillis = millis(); 
    digitalWrite(STATUS_LED, LOW); // LED solid when linked

    // Constrain incoming motor values safely to 8-bit PWM limits
    int cleanA1 = constrain(ReceivingData.motor_A1, 0, 255);
    int cleanA2 = constrain(ReceivingData.motor_A2, 0, 255);
    int cleanB1 = constrain(ReceivingData.motor_B1, 0, 255);
    int cleanB2 = constrain(ReceivingData.motor_B2, 0, 255);

    // Mutual Exclusivity Safety: Prevent opposing pins from firing together
    if (cleanA1 > 0 && cleanA2 > 0) {
      if (DEBUG_MODE) Serial.println("[WARN] Motor A opposing conflict detected! Forcing zero.");
      cleanA1 = 0; cleanA2 = 0;
    }
    if (cleanB1 > 0 && cleanB2 > 0) {
      if (DEBUG_MODE) Serial.println("[WARN] Motor B opposing conflict detected! Forcing zero.");
      cleanB1 = 0; cleanB2 = 0;
    }

    // Drive both motor channels with safety-checked values
    ledcWrite(A1Pin, cleanA1); 
    ledcWrite(A2Pin, cleanA2); 
    ledcWrite(B1Pin, cleanB1); 
    ledcWrite(B2Pin, cleanB2);

    // Drive the weapon flipper servo natively (0-180 mapped to 16-bit duty cycle for 50Hz)
    int targetAngle = constrain(ReceivingData.weapon, 0, 180);
    uint32_t servoDuty = map(targetAngle, 0, 180, 3276, 6553);
    ledcWrite(WeaponPin, servoDuty);
  }

  // --- 4. CONNECTION TIMEOUT SAFETY (Failsafe Cutoff) ---
  if((millis() - rxPreviousMillis) > 1000){ 
    ledcWrite(A1Pin, 0); ledcWrite(A2Pin, 0);
    ledcWrite(B1Pin, 0); ledcWrite(B2Pin, 0);      
    digitalWrite(STATUS_LED, HIGH); 
  } 
}