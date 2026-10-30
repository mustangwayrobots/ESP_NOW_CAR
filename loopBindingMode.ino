/**
 * ==============================================================================
 * loopBindingMode.ino (Car Receiver - Robust Handshake with Guaranteed Delivery)
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

  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 500) {
    lastDebugPrint = millis();
    Serial.printf("[CAR-FLAGS] ReceiveBindingComplete = %d | SendBindingComplete = %d\n", 
                  ReceiveBindingComplete, SendBindingComplete);
  }

  if (ReceiveBindingComplete && !SendBindingComplete) {
    static unsigned long lastTxTime = 0;
    static int confirmationCount = 0;
    
    if (millis() - lastTxTime > 100) { 
      lastTxTime = millis();
      
      RC_Packet_Structure confirmationPacket;
      confirmationPacket.protocolMode = 2; 
      
      Serial.println("[LOOP-BIND] Blasting registration handshake back to Remote...");
      uint8_t broadTarget[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      esp_now_send(broadTarget, (uint8_t *) &confirmationPacket, sizeof(confirmationPacket));
      
      confirmationCount++;
      if (confirmationCount > 10) {
        SendBindingComplete = true;
      }
    }
  }

  if (SendBindingComplete && ReceiveBindingComplete) {
      char remoteMacStr[18]; 
      snprintf(remoteMacStr, sizeof(remoteMacStr), "%02x:%02x:%02x:%02x:%02x:%02x",
               capturedRemoteMac[0], capturedRemoteMac[1], capturedRemoteMac[2],
               capturedRemoteMac[3], capturedRemoteMac[4], capturedRemoteMac[5]);      
      
      Serial.printf("[SUCCESS] PAIR COMPLETE! Committing address to NVS: %s\n", remoteMacStr);
      writeStoredMac(capturedRemoteMac);
      
      digitalWrite(STATUS_LED, HIGH); 
      
      Serial.println("[SYS] Waiting 1.5 seconds for controller to ingest packet before reboot...");
      delay(1500); 
      
      Serial.println("[SYS] Rebooting into Driving Loops now...");
      ESP.restart();
  }
}