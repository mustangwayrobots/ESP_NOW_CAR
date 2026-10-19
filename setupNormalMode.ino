/**
 *
 * ==============================================================================
 * setupNormalMode.ino (Car Receiver - Complete Dual-Interface Layout)
 * Aligned to accept legacy ESP8266 AP packets and future ESP32-C3 STA packets.
 * Low-Frequency Drive Matrix: Overrides onboard LED filters for custom PCBs.
 * ==============================================================================
 */
void setupNormalMode(){
  if (DEBUG_MODE) Serial.println("[INIT-NORM] Entering drive configurations setup sequence...");
  
  // Force All PCB Tracks explicitly to Output Buffers first to maximize drive current
  pinMode(A1Pin, OUTPUT);
  pinMode(A2Pin, OUTPUT);
  pinMode(B1Pin, OUTPUT);
  pinMode(B2Pin, OUTPUT);

  // Force AP_STA Dual Interface Mode for generational controller compatibility
  WiFi.mode(WIFI_AP_STA);

  uint8_t remoteMac[6];
  if (!readStoredMac(remoteMac)) {
    Serial.println("[WARN] Storage space empty! Run pairing long-press routine.");
  }
  
  // --- AUTOMATIC DYNAMIC CHANNEL CALCULATION ---
  uint8_t calculatedChannel = 1;
  uint8_t macRemainder = remoteMac[5] % 3; 
  if (macRemainder == 0)      calculatedChannel = 1;
  else if (macRemainder == 1) calculatedChannel = 6;
  else if (macRemainder == 2) calculatedChannel = 11;
  
  Serial.printf("[INIT-NORM] Loaded Whitelisted Controller MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                remoteMac[0], remoteMac[1], remoteMac[2], remoteMac[3], remoteMac[4], remoteMac[5]);
  Serial.printf("[INIT-NORM] Automated Classroom Channel Allocation -> Channel %d\n", calculatedChannel);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERR] ESP-NOW initialization crashed in drive mode setup!");
    ESP.restart();
  }

  esp_wifi_set_channel(calculatedChannel, WIFI_SECOND_CHAN_NONE);
 
  // --- REGISTER PEER: INTERFACE 1 (For Future ESP32-C3 STA Controllers) ---
  esp_now_peer_info_t peerInfoSTA;
  memset(&peerInfoSTA, 0, sizeof(peerInfoSTA));
  memcpy(peerInfoSTA.peer_addr, remoteMac, 6);
  peerInfoSTA.channel = calculatedChannel; 
  peerInfoSTA.ifidx   = WIFI_IF_STA;       
  peerInfoSTA.encrypt = false;
  
  if (esp_now_add_peer(&peerInfoSTA) != ESP_OK) {
    Serial.println("[WARN] Station interface pipeline profile registration skipped or active.");
  }

  // --- REGISTER PEER: INTERFACE 2 (For Legacy ESP8266 AP Controllers) ---
  esp_now_peer_info_t peerInfoAP;
  memset(&peerInfoAP, 0, sizeof(peerInfoAP));
  memcpy(peerInfoAP.peer_addr, remoteMac, 6);
  peerInfoAP.channel = calculatedChannel;
  peerInfoAP.ifidx   = WIFI_IF_AP;         
  peerInfoAP.encrypt = false;
  
  if (esp_now_add_peer(&peerInfoAP) != ESP_OK) {
    Serial.println("[ERR] Failed to configure exclusive unicast communication pipe over AP interface.");
  } else {
    if (DEBUG_MODE) Serial.println("[INIT-NORM] Dual-interface hardware communication pipes established.");
  }

  esp_now_register_recv_cb([](const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    RC_Packet_Structure *incoming = (RC_Packet_Structure*)data;
    if (incoming->protocolMode == 1) { 
      memcpy(&ReceivingData, data, min(len, (int)sizeof(ReceivingData)));
      NewDataFlag = true;
    }
  });

  // --- HARDWARE PWM LOW-FREQUENCY BRIDGES ---
  // Dropping to 50Hz overrides the onboard hardware filter constraints on GPIO 2, 
  // letting the raw logic power flow directly into your DRV8833 chip traces!
  ledcAttach(A1Pin, 5000, 8);  // Motor A Forward (GPIO 4)
  ledcAttach(A2Pin, 5000, 8);  // Motor A Reverse (GPIO 2)
  ledcAttach(B1Pin, 5000, 8);  // Motor B Forward (GPIO 1)
  ledcAttach(B2Pin, 5000, 8);  // Motor B Reverse (GPIO 0)
  
  weapon.attach(WeaponPin);
  weapon.write(0); 
  if (DEBUG_MODE) Serial.println("[INIT-NORM] Normal drive environment initialized successfully.");
}

