/**
 * ==============================================================================
 * Storage.ino (Professional Non-Volatile Storage Engine)
 * Replaces LittleFS with native ESP32 NVS Flash Partitioning
 * ==============================================================================
 */

// FIXED: Removed duplicate 'Preferences nvStorage;' declaration from this tab!

void CheckFileSystem() {
  // We leave this function here as a clean, silent baseline so setup() remains intact
  if (DEBUG_MODE) Serial.println("[NVS] Native flash storage engine ready.");
}

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

void writeStoredMac(const uint8_t* macIn) {
  if (DEBUG_MODE) Serial.println("[NVS] Writing verified Controller MAC to non-volatile registers......");
  
  nvStorage.begin("rc_car", false); 
  nvStorage.putBytes("controller_mac", macIn, 6);
  nvStorage.end(); 
  
  if (DEBUG_MODE) Serial.println("[NVS] Storage transaction completed safely.");
}
