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

// Returns true if all 6 bytes were written
bool writeStoredMac(const uint8_t* macIn) {
  if (DEBUG_MODE) Serial.println("[NVS] Writing verified Controller MAC to non-volatile registers...");
  
  nvStorage.begin("rc_car", false); 
  size_t bytesWritten = nvStorage.putBytes("controller_mac", macIn, 6);
  nvStorage.end(); 
  
  if (bytesWritten != 6) {
    Serial.println("[ERR] Flash write FAILED. The old pairing (if any) is kept.");
    return false;
  }
  if (DEBUG_MODE) Serial.println("[NVS] Storage transaction completed safely.");
  return true;
}
