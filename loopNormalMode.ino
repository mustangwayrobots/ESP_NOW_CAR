/**
 * ==============================================================================
 * loopNormalMode.ino (Car Receiver - Calibrated Averaged Telemetry & 250ms Failsafe)
 * ==============================================================================
 * Order each pass: 1) take any new packet, 2) work out link status,
 * 3) telemetry, 4) failsafe and LED.
 */
const unsigned long FAILSAFE_TIMEOUT_MS = 250;   // motors stop after this long without a packet
const unsigned long TELEMETRY_PERIOD_MS = 5000;  // battery report while linked
const int DEAD_BAND = 15;                         // motor values below this are treated as 0

void loopNormalMode(){
  static uint32_t rxPreviousMillis = 0; 
  static bool everReceived = false;          // no "linked" before the first real packet (N-04)
  static uint32_t lastTransmitTime = 0;
  static bool prevLinkedState = false;

  // --- 1. PACKET INGESTION, DEADBAND & SAFETY CLAMPS ---
  if (NewDataFlag) {
    RC_Packet_Structure cmd;
    portENTER_CRITICAL(&rxMux);              // take a clean copy of the packet (O-05)
    cmd = ReceivingData;
    NewDataFlag = false;
    portEXIT_CRITICAL(&rxMux);

    everReceived = true;
    rxPreviousMillis = millis(); 

    int cleanA1 = constrain(cmd.motor_A1, 0, 255);
    int cleanA2 = constrain(cmd.motor_A2, 0, 255);
    int cleanB1 = constrain(cmd.motor_B1, 0, 255);
    int cleanB2 = constrain(cmd.motor_B2, 0, 255);

    // Deadband filter: ignore micro-values to eliminate motor whine/jitter at rest
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
    int targetAngle = constrain(cmd.weapon, 0, 180);
    uint32_t servoDuty = map(targetAngle, 0, 180, 3276, 6553);
    ledcWrite(WeaponPin, servoDuty);
  }

  // --- 2. LINK STATUS ---
  bool isLinked = everReceived && (millis() - rxPreviousMillis) <= FAILSAFE_TIMEOUT_MS;
  bool linkJustCameUp = isLinked && !prevLinkedState;
  prevLinkedState = isLinked;

  // --- 3. TELEMETRY (on link-up, then every 5 s while linked) ---
  if (isLinked && (linkJustCameUp || (millis() - lastTransmitTime) > TELEMETRY_PERIOD_MS)) {
    lastTransmitTime = millis();
    SendingData.protocolMode = 1; 

    // analogReadMilliVolts() uses the factory eFuse calibration; 16 samples filter motor noise
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

  // --- 4. CONNECTION STATUS & TIMEOUT SAFETY (250ms Failsafe Cutoff) ---
  if (!isLinked) { 
    ledcWrite(A1Pin, 0); ledcWrite(A2Pin, 0);
    ledcWrite(B1Pin, 0); ledcWrite(B2Pin, 0);      

    // Slow blink (500ms) = no controller signal
    static unsigned long lastBlinkTime = 0;
    static bool slowLedState = false;
    if (millis() - lastBlinkTime > 500) {
      lastBlinkTime = millis();
      slowLedState = !slowLedState;
      setStatusLed(slowLedState);
    }
  } else {
    setStatusLed(true);   // Solid ON while linked
  }
}
