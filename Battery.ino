/**
 * ==============================================================================
 * Battery.ino - Battery voltage, measured without pausing the loop (N-03)
 * ==============================================================================
 * Call updateBattery() on every loop pass. It takes ONE reading every 10 ms
 * and keeps the last 16, so the average covers about 160 ms and filters out
 * motor noise. batteryMillivolts() returns that average.
 * analogReadMilliVolts() uses the chip's factory calibration.
 */
const int           BATTERY_SAMPLES     = 16;
const unsigned long BATTERY_SAMPLE_MS   = 10;
const int           BATTERY_DIVIDER     = 2;     // 10k/10k divider halves the battery voltage
const int           BATTERY_MAX_MV      = 4200;  // a full 1S LiPo; higher readings are capped

static uint32_t batterySamples[BATTERY_SAMPLES];
static uint32_t batterySum   = 0;
static int      batteryIndex = 0;
static int      batteryCount = 0;   // how many samples are filled in so far

void updateBattery() {
  static unsigned long lastSample = 0;
  if (batteryCount > 0 && millis() - lastSample < BATTERY_SAMPLE_MS) return;
  lastSample = millis();

  uint32_t mv = analogReadMilliVolts(BATTERY_ADC_PIN);
  batterySum -= batterySamples[batteryIndex];   // drop the oldest (0 until the buffer fills)
  batterySamples[batteryIndex] = mv;
  batterySum += mv;
  batteryIndex = (batteryIndex + 1) % BATTERY_SAMPLES;
  if (batteryCount < BATTERY_SAMPLES) batteryCount++;
}

int batteryMillivolts() {
  if (batteryCount == 0) return 0;
  uint32_t pinMv = batterySum / batteryCount;
  return constrain((int)(pinMv * BATTERY_DIVIDER), 0, BATTERY_MAX_MV);
}
