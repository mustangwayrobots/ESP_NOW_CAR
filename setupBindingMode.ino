/**
 * ==============================================================================
 * setupBindingMode.ino (Car Receiver - Length Validation in Binding Mode)
 * ==============================================================================
 * The radio callback below only copies data and raises flags.
 * Slow work (printing, adding the controller as a peer, saving to flash)
 * is done in loopBindingMode.ino, because callbacks run inside the Wi-Fi
 * task and must return quickly. (Same pattern as the controller.)
 */
void setupBindingMode(){
  if (DEBUG_MODE) Serial.println("[INIT-BIND] Configuring radio network layers for pairing...");
  
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm); 

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERR] ESP_Now initialization failed in binding mode!");
    ESP.restart();
  }
  
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (DEBUG_MODE) Serial.println("[INIT-BIND] ESP-NOW initialized successfully on Station Channel 1.");

  uint8_t broadTarget[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadTarget, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.ifidx   = WIFI_IF_STA;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  esp_now_register_recv_cb([](const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (len < (int)sizeof(RC_Packet_Structure)) return;   // too short, ignore (O-08)
    const RC_Packet_Structure *incoming = (const RC_Packet_Structure*)data;

    // Mode 0: the first controller asking to pair is captured
    if (incoming->protocolMode == 0 && !ReceiveBindingComplete) {
      memcpy(capturedRemoteMac, recv_info->src_addr, 6);   // copy first...
      ReceiveBindingComplete = true;                        // ...then raise the flag
      return;
    }

    // Mode 3: that same controller confirms it got our acknowledgement
    if (incoming->protocolMode == 3 && ReceiveBindingComplete &&
        memcmp(recv_info->src_addr, capturedRemoteMac, 6) == 0) {
      HandshakeConfirmed = true;
    }
  });
}
