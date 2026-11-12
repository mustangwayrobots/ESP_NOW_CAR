/**
 * ==============================================================================
 * loopBindingMode.ino (Car Receiver - Handshake Loop)
 * ==============================================================================
 * 1. Wait up to 60 s for a controller's bind request (mode 0).
 * 2. Once captured, send acknowledgements (mode 2) every 100 ms and wait
 *    up to 15 s for the controller's confirmation (mode 3).
 * 3. Confirmed: save the controller MAC and restart into Normal Mode.
 * Either timeout restarts the car; the old pairing (if any) stays saved.
 */
const unsigned long BIND_WAIT_MS      = 60000;  // wait this long for a bind request
const unsigned long HANDSHAKE_MS      = 15000;  // after capture, wait this long for mode 3
const unsigned long ACK_SEND_MS       = 100;    // acknowledgement repeat (mode 2)
const unsigned long BIND_LED_TOGGLE_MS = 50;    // fast blink = Binding Mode

void loopBindingMode(){
  static unsigned long bindStartTime = millis();
  static unsigned long captureTime = 0;   // 0 = no controller captured yet
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  static unsigned long lastDebugPrint = 0;
  static unsigned long lastTxTime = 0;

  // Fast blink while binding
  if (millis() - lastBlink > BIND_LED_TOGGLE_MS) {
    lastBlink = millis();
    ledState = !ledState;
    setStatusLed(ledState);
  }

  if (DEBUG_MODE && millis() - lastDebugPrint > 500) {
    lastDebugPrint = millis();
    Serial.printf("[CAR-FLAGS] ReceiveBindingComplete = %d | HandshakeConfirmed = %d\n", 
                  ReceiveBindingComplete, HandshakeConfirmed);
  }

  // Runs once, right after a controller is captured: print it and add it as a peer
  if (ReceiveBindingComplete && captureTime == 0) {
    captureTime = millis();
    if (captureTime == 0) captureTime = 1;   // 0 means "not captured"

    Serial.printf("[COMM-BIND] Controller MAC captured: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  capturedRemoteMac[0], capturedRemoteMac[1], capturedRemoteMac[2],
                  capturedRemoteMac[3], capturedRemoteMac[4], capturedRemoteMac[5]);

    esp_now_peer_info_t unicastPeer = {};
    memcpy(unicastPeer.peer_addr, capturedRemoteMac, 6);
    unicastPeer.channel = WIFI_CHANNEL;
    unicastPeer.ifidx   = WIFI_IF_STA;
    unicastPeer.encrypt = false;
    esp_now_add_peer(&unicastPeer);
  }

  // Timeouts (N-02): long wait for a request, short wait for the handshake
  if (captureTime == 0 && millis() - bindStartTime > BIND_WAIT_MS) {
    Serial.println("[ERR] No controller bind request within 60 s. Restarting...");
    ESP.restart();
  }
  if (captureTime != 0 && !HandshakeConfirmed && millis() - captureTime > HANDSHAKE_MS) {
    Serial.println("[ERR] Controller did not confirm within 15 s. Restarting...");
    ESP.restart();
  }

  // Send acknowledgement (mode 2) to the captured controller until it confirms
  if (captureTime != 0 && !HandshakeConfirmed && millis() - lastTxTime > ACK_SEND_MS) {
    lastTxTime = millis();
    RC_Packet_Structure ackPacket = {};   // zeroed, no garbage data
    ackPacket.protocolMode = 2; 
    esp_now_send(capturedRemoteMac, (uint8_t *)&ackPacket, sizeof(ackPacket));
  }

  // Confirmed: save and restart into Normal Mode
  if (HandshakeConfirmed) {
    Serial.printf("[SUCCESS] VERIFIED PAIR COMPLETE! Committing address to NVS: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  capturedRemoteMac[0], capturedRemoteMac[1], capturedRemoteMac[2],
                  capturedRemoteMac[3], capturedRemoteMac[4], capturedRemoteMac[5]);
    writeStoredMac(capturedRemoteMac);   // prints an error if the write fails (N-05)
      
    setStatusLed(false); 
    delay(500); 
    ESP.restart();
  }
}
