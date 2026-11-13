/**
 * ==============================================================================
 * loopNormalMode.ino (Car Receiver - Telemetry Replies & 250ms Failsafe)
 * ==============================================================================
 * Order each pass: 1) take any new packet, 2) work out link status,
 * 3) telemetry, 4) firmware mismatch check, 5) failsafe and LED.
 *
 * Telemetry doubles as the car's "I'm listening" reply: the controller only
 * shows CONNECTED while these keep arriving (period in Protocol.h).
 */
const unsigned long FAILSAFE_TIMEOUT_MS  = 250;   // motors stop after this long without a packet
const unsigned long MISMATCH_REPLY_MS    = 1000;  // on mismatch, tell the controller at most this often
const unsigned long MISMATCH_SHOW_MS     = 1500;  // keep the mismatch LED pattern this long after the last one
const unsigned long LED_SLOW_TOGGLE_MS   = 500;   // slow blink = no controller signal
const unsigned long LED_QUICK_TOGGLE_MS  = 150;   // quick blink = controller has different firmware
const int DEAD_BAND = 15;                          // motor values below this are treated as 0

// Sends our battery voltage to the paired controller
void sendTelemetry() {
  setPacketHeader(SendingData, PKT_TELEMETRY);
  SendingData.battery = batteryMillivolts();
  esp_now_send(cachedRemoteMac, (uint8_t *)&SendingData, sizeof(SendingData));
}

void loopNormalMode(){
  static uint32_t rxPreviousMillis = 0; 
  static bool everReceived = false;          // no "linked" before the first real packet (N-04)
  static uint32_t lastTransmitTime = 0;
  static bool prevLinkedState = false;
  static uint32_t lastMismatchTime = 0;      // 0 = no mismatch seen
  static uint32_t lastMismatchReply = 0;

  updateBattery();   // one quick reading every 10 ms (Battery.ino)

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

  // --- 3. TELEMETRY (on link-up, then every TELEMETRY_PERIOD_MS while linked) ---
  if (isLinked && (linkJustCameUp || (millis() - lastTransmitTime) >= TELEMETRY_PERIOD_MS)) {
    lastTransmitTime = millis();
    sendTelemetry();
  }

  // --- 4. FIRMWARE MISMATCH (our controller is sending another protocol version) ---
  // Reply with one of our own packets now and then, so a controller with a
  // different version can see the mismatch and show it on its screen.
  if (MismatchFlag) {
    MismatchFlag = false;
    lastMismatchTime = millis();
    if (lastMismatchTime == 0) lastMismatchTime = 1;   // 0 means "none seen"
    if (millis() - lastMismatchReply > MISMATCH_REPLY_MS) {
      lastMismatchReply = millis();
      Serial.printf("[WARN] Controller firmware uses a different protocol version (this car is v%d). Update both.\n",
                    PROTOCOL_VERSION);
      sendTelemetry();
    }
  }
  bool mismatchRecent = lastMismatchTime != 0 && (millis() - lastMismatchTime) < MISMATCH_SHOW_MS;

  // --- 5. CONNECTION STATUS & TIMEOUT SAFETY (250ms Failsafe Cutoff) ---
  if (!isLinked) { 
    ledcWrite(A1Pin, 0); ledcWrite(A2Pin, 0);
    ledcWrite(B1Pin, 0); ledcWrite(B2Pin, 0);      

    // Slow blink = no controller signal. Quick blink = controller has different firmware.
    static unsigned long lastBlinkTime = 0;
    static bool blinkLedState = false;
    unsigned long toggleMs = mismatchRecent ? LED_QUICK_TOGGLE_MS : LED_SLOW_TOGGLE_MS;
    if (millis() - lastBlinkTime > toggleMs) {
      lastBlinkTime = millis();
      blinkLedState = !blinkLedState;
      setStatusLed(blinkLedState);
    }
  } else {
    setStatusLed(true);   // Solid ON while linked
  }
}
