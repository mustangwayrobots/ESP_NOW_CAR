/**
 * ==============================================================================
 * loopNormalMode.ino (Car Receiver - High-Responsiveness, 250ms Failsafe & Telemetry)
 * ==============================================================================
 */
void loopNormalMode(){
  static uint32_t rxPreviousMillis = 0; 
  static uint32_t lastTransmitTime = 0;
  static bool initialTelemetrySent = false; // Tracks if we've sent the welcome telemetry packet
  
  // --- 1. TELEMETRY BROADCAST ENGINE (Battery Check) ---
  // Sends immediately upon connecting/booting, then every 5 seconds thereafter
  bool sendTelemetry = false;
  
  if (!initialTelemetrySent) {
    initialTelemetrySent = true;
    sendTelemetry = true;
  } else if ((millis() - lastTransmitTime) > 5000) {
    sendTelemetry = true;
  }

  if (sendTelemetry) {
    lastTransmitTime = millis();
    SendingData.protocolMode = 1; 

    uint32_t rawAdc = analogRead(BATTERY_ADC_PIN);
    uint32_t pinVoltageMv = (rawAdc * 3300) / 4095;
    uint32_t calculatedBatteryMv = pinVoltageMv * 2.0; 

    // Cap the displayed voltage at 4200mV (4.2V) max to keep readings realistic for a 1S LiPo
    SendingData.battery = constrain(calculatedBatteryMv, 0, 4200);

    uint8_t remoteMac[6];
    if (readStoredMac(remoteMac)) {
      esp_now_send(remoteMac, (uint8_t *)&SendingData, sizeof(SendingData));
    }
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
  // Perfectly matches controller 20ms active / 100ms heartbeat pacing.
  // If no packets arrive for 250ms, instantly kills motors and slow-blinks LED.
  if((millis() - rxPreviousMillis) > 250){ 
    ledcWrite(A1Pin, 0); ledcWrite(A2Pin, 0);
    ledcWrite(B1Pin, 0); ledcWrite(B2Pin, 0);      

    // Reset the initial telemetry flag on link loss so that when it re-links, 
    // it immediately spits out a fresh battery reading again!
    initialTelemetrySent = false; 

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