/**
 * ==============================================================================
 * StatusLed.ino - The one place that knows how the status LED is wired
 * ==============================================================================
 * V2/V3: plain on-board LED, ON when the pin is LOW.
 * V4:    WS2812 RGB LED on the ESP32-C3 Zero; shown in blue.
 * The LED is only rewritten when its state changes (an RGB write is slow).
 */
const uint8_t RGB_LED_BRIGHTNESS = 20;   // 0-255; the WS2812 is very bright

static int ledLastState = -1;            // -1 = not written yet

void setStatusLed(bool on) {
  if (ledLastState == (int)on) return;   // no change, nothing to do
  ledLastState = on;
#if defined(STATUS_LED_RGB)
  rgbLedWrite(STATUS_LED, 0, 0, on ? RGB_LED_BRIGHTNESS : 0);
#else
  digitalWrite(STATUS_LED, on ? LOW : HIGH);
#endif
}

void initStatusLed() {
#if !defined(STATUS_LED_RGB)
  pinMode(STATUS_LED, OUTPUT);
#endif
  setStatusLed(false);
}
