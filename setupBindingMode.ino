/**
 * ==============================================================================
 * setupBindingMode.ino (Car Receiver - Universal Ingestion Layout)
 * ==============================================================================
 */
void setupBindingMode(){
  if (DEBUG_MODE) Serial.println("[INIT-BIND] Configuring radio network layers for pairing...");
  
  WiFi.mode(WIFI_STA);
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
    RC_Packet_Structure *incoming = (RC_Packet_Structure*)data;
    
    if (incoming->protocolMode == 0 && !ReceiveBindingComplete) {
      memcpy(capturedRemoteMac, recv_info->src_addr, 6);
      ReceiveBindingComplete = true;
      
      Serial.printf("[COMM-BIND] Controller MAC Whitelisted: %02X:%02X:%02X:%02X:%02X:%02X\n",
                    capturedRemoteMac[0], capturedRemoteMac[1], capturedRemoteMac[2],
                    capturedRemoteMac[3], capturedRemoteMac[4], capturedRemoteMac[5]);
    }
  });  

  esp_now_register_send_cb([](const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS && ReceiveBindingComplete) {
      SendBindingComplete = true;
    }
  });
}
