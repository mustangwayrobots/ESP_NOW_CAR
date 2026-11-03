/**
 * ==============================================================================
 * setupNormalMode.ino (Car Receiver - Crash-Proof NVS, Motors & Native Servo)
 * ==============================================================================
 */
void setupNormalMode(){
  if (DEBUG_MODE) Serial.println("[INIT-NORM] Entering drive configurations setup sequence...");

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm); // Mitigate classroom RF congestion across 20 active cars[cite: 11]

  uint8_t remoteMac[6];
  if (!readStoredMac(remoteMac)) {
    Serial.println("[WARN] NVS Storage empty or invalid! Switching to Binding Mode...");
    NormalMode = false;
    setupBindingMode();
    return;
  }

  if (remoteMac[0] == 0 && remoteMac[1] == 0 && remoteMac[2] == 0 && 
      remoteMac[3] == 0 && remoteMac[4] == 0 && remoteMac[5] == 0) {
    Serial.println("[WARN] Stored MAC is null/zeros! Switching to Binding Mode...");
    NormalMode = false;
    setupBindingMode();
    return;
  }
  
  uint8_t calculatedChannel = 1;
  uint8_t macRemainder = remoteMac[5] % 3; 
  if (macRemainder == 1)      calculatedChannel = 6;
  else if (macRemainder == 2) calculatedChannel = 11;
  
  Serial.printf("[INIT-NORM] Loaded Whitelisted Controller MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                remoteMac[0], remoteMac[1], remoteMac[2], remoteMac[3], remoteMac[4], remoteMac[5]);
  Serial.printf("[INIT-NORM] Automated Classroom Channel Allocation -> Channel %d\n", calculatedChannel);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERR] ESP-NOW initialization crashed in drive mode setup!");
    ESP.restart();
  }

  esp_wifi_set_channel(calculatedChannel, WIFI_SECOND_CHAN_NONE);
 
  esp_now_peer_info_t peerInfo = {};
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, remoteMac, 6);
  peerInfo.channel = calculatedChannel; 
  peerInfo.ifidx   = WIFI_IF_STA;       
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[WARN] Failed to add unicast peer or peer already exists.");
  } else {
    if (DEBUG_MODE) Serial.println("[INIT-NORM] Exclusive STA communication pipe established.");
  }

  esp_now_register_recv_cb([](const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    RC_Packet_Structure *incoming = (RC_Packet_Structure*)data;
    if (incoming->protocolMode == 1) { 
      memcpy(&ReceivingData, data, min(len, (int)sizeof(ReceivingData)));
      NewDataFlag = true;
    }
  });

  // --- HARDWARE PWM MOTOR BRIDGES (5kHz, 8-bit) ---
  ledcAttach(A1Pin, 5000, 8);  
  ledcAttach(A2Pin, 5000, 8);  
  ledcAttach(B1Pin, 5000, 8);  
  ledcAttach(B2Pin, 5000, 8);  

  // --- NATIVE SERVO FLIPPER SETUP (50Hz, 16-bit) ---
  ledcAttach(WeaponPin, 50, 16);
  uint32_t initialDuty = map(0, 0, 180, 3276, 6553); 
  ledcWrite(WeaponPin, initialDuty);
  
  if (DEBUG_MODE) Serial.println("[INIT-NORM] Normal drive environment initialized successfully.");
}