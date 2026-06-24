#include <Arduino.h>
#include <Wire.h>

#include "TechoBoard.h"

#ifdef LILYGO_TECHO

void TechoBoard::begin() {
  NRF52Board::begin();

  Wire.begin();

  pinMode(SX126X_POWER_EN, OUTPUT);
  digitalWrite(SX126X_POWER_EN, HIGH);
  delay(10);   // give sx1262 some time to power up
}

uint16_t TechoBoard::getBattMilliVolts() {
  analogReference(AR_INTERNAL_3_0);
  analogReadResolution(12);

  // Make sure the pin is an analog input and give the divider/ADC time to
  // settle. When the battery is only read occasionally (e.g. on a remote
  // telemetry/status request to a repeater) the SAADC has been idle, and the
  // first conversion after re-enabling it / switching the reference tends to
  // return 0. The companion firmware never sees this because its UI polls the
  // battery every few seconds, keeping the ADC "warm". Prime the ADC with a
  // throwaway read and average several samples so the result is stable no
  // matter how often this is called.
  pinMode(PIN_VBAT_READ, INPUT);
  delay(10);

  (void)analogRead(PIN_VBAT_READ);  // discard first (cold) reading

  // ADC range is 0..3000mV and resolution is 12-bit (0..4095)
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(PIN_VBAT_READ);
    delayMicroseconds(100);
  }
  uint16_t adcvalue = (uint16_t)(sum / 8);

  // Convert the raw value to compensated mv, taking the resistor-
  // divider into account (providing the actual LIPO voltage)
  return (uint16_t)((float)adcvalue * REAL_VBAT_MV_PER_LSB);
}
#endif
