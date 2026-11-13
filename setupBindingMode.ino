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
  
  esp_wifi_set_channel(BINDING_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (DEBUG_MODE) Serial.printf("[INIT-BIND] ESP-NOW initialized on binding channel %d (protocol v%d).\n",
                                BINDING_CHANNEL, PROTOCOL_VERSION);

  uint8_t broadTarget[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadTarget, 6);
  peerInfo.channel = BINDING_CHANNEL;
  peerInfo.ifidx   = WIFI_IF_STA;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  esp_now_register_recv_cb([](const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (!isCurrentProtocol(data, len)) return;   // wrong size or version: ignore (Protocol.h)
    const RC_Packet_Structure *incoming = (const RC_Packet_Structure*)data;

    // Bind request: the first controller asking to pair is captured
    if (incoming->packetType == PKT_BIND_REQUEST && !ReceiveBindingComplete) {
      if (!isDriveChannel(incoming->channel)) return;       // bad drive channel, ignore
      memcpy(capturedRemoteMac, recv_info->src_addr, 6);   // copy first...
      capturedChannel = incoming->channel;
      ReceiveBindingComplete = true;                        // ...then raise the flag
      return;
    }

    // Bind confirm: that same controller got our acknowledgement
    if (incoming->packetType == PKT_BIND_CONFIRM && ReceiveBindingComplete &&
        memcmp(recv_info->src_addr, capturedRemoteMac, 6) == 0 &&
        incoming->channel == capturedChannel) {
      HandshakeConfirmed = true;
    }
  });
}
