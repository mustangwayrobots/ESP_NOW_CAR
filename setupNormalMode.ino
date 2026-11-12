/**
 * ==============================================================================
 * setupNormalMode.ino (Car Receiver - Authenticated Packets & Length Validation)
 * ==============================================================================
 */
void setupNormalMode(){
  if (DEBUG_MODE) Serial.println("[INIT-NORM] Entering drive configurations setup sequence...");

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm); 

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
  
  memcpy(cachedRemoteMac, remoteMac, 6);
  
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

  // Receive callback: length check (O-08), mode check, sender check (O-02).
  // The copy is done under rxMux so the loop never reads a half-copied packet (O-05).
  esp_now_register_recv_cb([](const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (len < (int)sizeof(RC_Packet_Structure)) return;                // too short
    const RC_Packet_Structure *incoming = (const RC_Packet_Structure*)data;
    if (incoming->protocolMode != 1) return;                           // not a drive packet
    if (memcmp(recv_info->src_addr, cachedRemoteMac, 6) != 0) return;  // not our controller

    portENTER_CRITICAL(&rxMux);
    memcpy(&ReceivingData, data, sizeof(RC_Packet_Structure));
    NewDataFlag = true;
    portEXIT_CRITICAL(&rxMux);
  });

  ledcAttach(A1Pin, 5000, 8);  
  ledcAttach(A2Pin, 5000, 8);  
  ledcAttach(B1Pin, 5000, 8);  
  ledcAttach(B2Pin, 5000, 8);  

  ledcAttach(WeaponPin, 50, 16);
  uint32_t initialDuty = map(0, 0, 180, 3276, 6553); 
  ledcWrite(WeaponPin, initialDuty);
  
  if (DEBUG_MODE) Serial.println("[INIT-NORM] Normal drive environment initialized successfully.");
}