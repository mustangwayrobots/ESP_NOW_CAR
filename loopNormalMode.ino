/**
 * ==============================================================================
 * loopNormalMode.ino (Car Receiver - Calibrated Averaged Telemetry & 250ms Failsafe)
 * ==============================================================================
 */
void loopNormalMode(){
  static uint32_t rxPreviousMillis = 0; 
  static uint32_t lastTransmitTime = 0;
  static bool initialTelemetrySent = false; 
  static bool prevLinkedState = false;
  
  // Determine current link status based on the 250ms timeout window
  bool isLinked = (millis() - rxPreviousMillis) <= 250;

  // Detect link re-establishment (rising edge) to trigger an immediate telemetry packet
  if (isLinked && !prevLinkedState) {
    initialTelemetrySent = false;
  }
  prevLinkedState = isLinked;

  // --- 1. TELEMETRY BROADCAST ENGINE (Calibrated Battery Check - O-11) ---
  bool sendTelemetry = false;
  
  if (isLinked) {
    if (!initialTelemetrySent) {
      initialTelemetrySent = true;
      sendTelemetry = true;
      lastTransmitTime = millis();
    } else if ((millis() - lastTransmitTime) > 5000) {
      sendTelemetry = true;
      lastTransmitTime = millis();
    }
  }

  if (sendTelemetry) {
    SendingData.protocolMode = 1; 

    // O-11 Fix: Use analogReadMilliVolts() with 16-sample averaging to filter out motor noise,
    // sag, and correct for ESP32-C3 ADC non-linearity using factory eFuse calibration.
    uint32_t sumMv = 0;
    const int sampleCount = 16;
    for (int i = 0; i < sampleCount; i++) {
      sumMv += analogReadMilliVolts(BATTERY_ADC_PIN);
      delayMicroseconds(500); // Short window between samples
    }
    uint32_t avgPinVoltageMv = sumMv / sampleCount;
    uint32_t calculatedBatteryMv = avgPinVoltageMv * 2; // 2:1 ratio from 10k/10k divider

    // Cap displayed voltage at 4200mV (4.2V) max to keep readings realistic for a 1S LiPo
    SendingData.battery = constrain(calculatedBatteryMv, 0, 4200);

    // Send directly using the RAM-cached MAC address (Zero NVS flash overhead)
    esp_now_send(cachedRemoteMac, (uint8_t *)&SendingData, sizeof(SendingData));
  }

  // --- 2. PACKET INGESTION, DEADBAND & SAFETY CLAMPS ---
  if(NewDataFlag){
    NewDataFlag = false; 
    rxPreviousMillis = millis(); 

    int cleanA1 = constrain(ReceivingData.motor_A1, 0, 255);
    int cleanA2 = constrain(ReceivingData.motor_A2, 0, 255);
    int cleanB1 = constrain(ReceivingData.motor_B1, 0, 255);
    int cleanB2 = constrain(ReceivingData.motor_B2, 0, 255);

    // Deadband filter: ignore micro-values to eliminate motor whine/jitter at rest
    const int DEAD_BAND = 15;
    if (cleanA1 < DEAD_BAND) cleanA1 = 0;
    if (cleanA2 < DEAD_BAND) cleanA2 = 0;
    if (cleanB1 < DEAD_BAND) cleanB1 = 0;
    if (cleanB2 < DEAD_BAND) cleanB2 = 0;

    // Mutual Exclusivity Safety: Prevent opposing pins from firing together
    if (cleanA1 > 0 && cleanA2 > 0) {
      if (DEBUG_MODE) Serial.println("[WARN] Motor A opposing conflict detected! Forcing zero.");
      cleanA1 = 0; cleanA2 = 0;
    }
    if (cleanB1 > 0 && cleanB2 > 0) {
      if (DEBUG_MODE) Serial.println("[WARN] Motor B opposing conflict detected! Forcing zero.");
      cleanB1 = 0; cleanB2 = 0;
    }

    // Drive motor channels
    ledcWrite(A1Pin, cleanA1); 
    ledcWrite(A2Pin, cleanA2); 
    ledcWrite(B1Pin, cleanB1); 
    ledcWrite(B2Pin, cleanB2);

    // Drive weapon servo natively (0-180 mapped to 16-bit duty cycle for 50Hz)
    int targetAngle = constrain(ReceivingData.weapon, 0, 180);
    uint32_t servoDuty = map(targetAngle, 0, 180, 3276, 6553);
    ledcWrite(WeaponPin, servoDuty);
  }

  // --- 3. CONNECTION STATUS & TIMEOUT SAFETY (250ms Failsafe Cutoff) ---
  if(!isLinked){ 
    ledcWrite(A1Pin, 0); ledcWrite(A2Pin, 0);
    ledcWrite(B1Pin, 0); ledcWrite(B2Pin, 0);      

    // Slow blink routine (500ms interval) indicating link loss / searching
    static unsigned long lastBlinkTime = 0;
    static bool slowLedState = false;
    if (millis() - lastBlinkTime > 500) {
      lastBlinkTime = millis();
      slowLedState = !slowLedState;
      digitalWrite(STATUS_LED, slowLedState ? LOW : HIGH); 
    }
  } else {
    // Solid ON when actively linked and receiving packets
    digitalWrite(STATUS_LED, LOW); 
  }
}