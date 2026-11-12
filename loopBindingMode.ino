/**
 * ==============================================================================
 * loopBindingMode.ino (Car Receiver - Cleaned Handshake Loop)
 * ==============================================================================
 */
void loopBindingMode(){
  static unsigned long lastBlink;
  static bool ledState = false;
  if(millis() - lastBlink > 50) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(STATUS_LED, ledState ? LOW : HIGH); 
  }

  static unsigned long bindStartTime = millis();
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 500) {
    lastDebugPrint = millis();
    Serial.printf("[CAR-FLAGS] ReceiveBindingComplete = %d | HandshakeConfirmed = %d\n", 
                  ReceiveBindingComplete, HandshakeConfirmed);
  }

  if (millis() - bindStartTime > 15000) {
    Serial.println("[ERR] Binding handshake timed out waiting for controller confirmation. Restarting...");
    ESP.restart();
  }

  if (ReceiveBindingComplete && !HandshakeConfirmed) {
    static unsigned long lastTxTime = 0;
    
    if (millis() - lastTxTime > 100) { 
      lastTxTime = millis();
      
      // Properly zero-initialize confirmationPacket to prevent garbage data
      RC_Packet_Structure confirmationPacket = {};
      confirmationPacket.protocolMode = 2; 
      
      esp_now_send(capturedRemoteMac, (uint8_t *)&confirmationPacket, sizeof(confirmationPacket));
    }
  }

  if (HandshakeConfirmed) {
      char remoteMacStr[18]; 
      snprintf(remoteMacStr, sizeof(remoteMacStr), "%02x:%02x:%02x:%02x:%02x:%02x",
               capturedRemoteMac[0], capturedRemoteMac[1], capturedRemoteMac[2],
               capturedRemoteMac[3], capturedRemoteMac[4], capturedRemoteMac[5]);      
      
      Serial.printf("[SUCCESS] VERIFIED PAIR COMPLETE! Committing address to NVS: %s\n", remoteMacStr);
      writeStoredMac(capturedRemoteMac);
      
      digitalWrite(STATUS_LED, HIGH); 
      delay(500); 
      ESP.restart();
  }
}