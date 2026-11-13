/**
 * ==============================================================================
 * Storage.ino (Professional Non-Volatile Storage Engine)
 * ==============================================================================
 */

bool readStoredMac(uint8_t* macOut) {
  nvStorage.begin("rc_car", true); 
  size_t bytesRead = nvStorage.getBytes("controller_mac", macOut, 6);
  nvStorage.end(); 

  if (bytesRead == 6) {
    return true; 
  }
  
  memset(macOut, 0, 6);
  return false;
}

// The drive channel saved at pairing (1, 6 or 11). Returns 0 if none is saved,
// for example a pairing made by older firmware that did not save the channel.
uint8_t readStoredChannel() {
  nvStorage.begin("rc_car", true);
  uint8_t channel = nvStorage.getUChar("drive_ch", 0);
  nvStorage.end();
  return channel;
}

// Saves the controller MAC and the drive channel. Returns true if both were written.
bool writeStoredPairing(const uint8_t* macIn, uint8_t channel) {
  if (DEBUG_MODE) Serial.println("[NVS] Writing verified Controller MAC and drive channel to flash...");
  
  nvStorage.begin("rc_car", false); 
  size_t macBytes = nvStorage.putBytes("controller_mac", macIn, 6);
  size_t chBytes  = nvStorage.putUChar("drive_ch", channel);
  nvStorage.end(); 
  
  if (macBytes != 6 || chBytes != 1) {
    Serial.println("[ERR] Flash write FAILED. The old pairing (if any) is kept.");
    return false;
  }
  if (DEBUG_MODE) Serial.println("[NVS] Storage transaction completed safely.");
  return true;
}
