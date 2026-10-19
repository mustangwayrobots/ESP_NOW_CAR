/**
 * ==============================================================================
 * loopBindingMode.ino (Car Receiver - Deep Debug Flag Tracking Layout)
 * Drives the high-frequency pairing handshake routine and flashing strobe.
 * ==============================================================================
 */
void loopBindingMode(){
  // Fast blink strobe (50ms) denotes pairing activity runtime boundaries
  static unsigned long lastBlink;
  static bool ledState = false;
  if(millis() - lastBlink > 50) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(STATUS_LED, ledState ? LOW : HIGH); 
  }

  // --- NEW DIAGNOSTIC MONITOR BLOCK ---
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 500) {
    lastDebugPrint = millis();
    Serial.printf("[CAR-FLAGS] ReceiveBindingComplete = %d | SendBindingComplete = %d\n", 
                  ReceiveBindingComplete, SendBindingComplete);
  }

  // Phase 2: If we heard the transmitter, continuously reply back with our identity confirmation
  if (ReceiveBindingComplete && !SendBindingComplete) {
    static unsigned long lastTxTime;
    if (millis() - lastTxTime > 150) { 
      lastTxTime = millis();
      
      RC_Packet_Structure confirmationPacket;
      confirmationPacket.protocolMode = 2; // Code 2 = Handshake Validation
      
      if (DEBUG_MODE) Serial.println("[LOOP-BIND] Sending registration handshake back to Remote via Broadcast...");
      
      uint8_t broadTarget[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      esp_now_send(broadTarget, (uint8_t *) &confirmationPacket, sizeof(confirmationPacket));
    }
  }

  // Phase 3: Both flags are true! Safe to commit to NVS flash memory and reboot
  if (SendBindingComplete && ReceiveBindingComplete) {
      char remoteMacStr[18]; 
      snprintf(remoteMacStr, sizeof(remoteMacStr), "%02x:%02x:%02x:%02x:%02x:%02x",
               capturedRemoteMac[0], capturedRemoteMac[1], capturedRemoteMac[2],
               capturedRemoteMac[3], capturedRemoteMac[4], capturedRemoteMac[5]);      
      
      Serial.printf("[SUCCESS] PAIR COMPLETE! Committing address directly to NVS registers: %s\n", remoteMacStr);
      writeStoredMac(capturedRemoteMac);
      
      digitalWrite(STATUS_LED, HIGH); // Turn LED off cleanly before rebooting
      delay(1000); // Give file systems an explicit moment to safely close
      Serial.println("[SYS] Rebooting into Driving Loops now...");
      ESP.restart();
  }
}



