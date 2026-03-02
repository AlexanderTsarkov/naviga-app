// E220 airtime profiling bench — issue #332
// Measures on-air TX time for payload sizes 1..50 bytes at 2.4 kbps and 4.8 kbps.
// Method: AUX pin goes LOW when E220 starts TX, returns HIGH when TX complete.
// Timing: micros() delta from sendMessage() call to AUX HIGH.
//
// Hardware: devkit_e220_oled_gnss
//   M0  = GPIO4, M1 = GPIO5
//   RX  = GPIO1 (E220 RXD → ESP TX)
//   TX  = GPIO2 (E220 TXD → ESP RX)
//   AUX = GPIO47
//
// Usage: flash to one node, open serial monitor at 115200.
// Output: CSV lines — preset,size_bytes,sample,airtime_us

#include <Arduino.h>
#include <HardwareSerial.h>
#include "LoRa_E220.h"

// Pin mapping (matches devkit_e220_oled_gnss profile)
constexpr int PIN_M0  = 4;
constexpr int PIN_M1  = 5;
constexpr int PIN_AUX = 47;
constexpr int PIN_E220_TX = 2;  // E220 TXD → ESP RX
constexpr int PIN_E220_RX = 1;  // E220 RXD → ESP TX

HardwareSerial loraSerial(2);
LoRa_E220 e220(&loraSerial, PIN_AUX, PIN_M0, PIN_M1, UART_BPS_RATE_9600);

constexpr int SAMPLES_PER_SIZE = 5;
constexpr int MIN_SIZE = 1;
constexpr int MAX_SIZE = 50;
constexpr unsigned long AUX_TIMEOUT_US = 5000000UL;  // 5s max wait

// Air data rate constants from E220 library
struct Preset {
  const char* name;
  byte air_rate;
};

const Preset kPresets[] = {
  { "2400bps", AIR_DATA_RATE_000_24 },
  { "4800bps", AIR_DATA_RATE_011_48 },
};

bool configure_preset(byte air_rate) {
  // #326 fix: pre-assert M0=M1=HIGH and wait 200ms for config-mode settling.
  digitalWrite(PIN_M0, HIGH);
  digitalWrite(PIN_M1, HIGH);
  delay(200);
  loraSerial.flush();
  while (loraSerial.available()) loraSerial.read();

  ResponseStructContainer c = e220.getConfiguration();
  if (c.status.code != E220_SUCCESS) {
    Serial.printf("ERR: getConfiguration failed: %d\n", c.status.code);
    c.close();
    return false;
  }
  Configuration cfg = *(Configuration*)c.data;
  c.close();

  cfg.SPED.airDataRate = air_rate;
  cfg.SPED.uartBaudRate = UART_BPS_9600;
  cfg.SPED.uartParity = MODE_00_8N1;
  cfg.ADDH = 0x00;
  cfg.ADDL = 0x00;
  cfg.CHAN = 0x01;
  cfg.TRANSMISSION_MODE.fixedTransmission = FT_TRANSPARENT_TRANSMISSION;
  cfg.TRANSMISSION_MODE.enableRSSI = RSSI_DISABLED;
  cfg.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
  cfg.OPTION.subPacketSetting = SPS_200_00;
  cfg.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;

  ResponseStatus rs = e220.setConfiguration(cfg, WRITE_CFG_PWR_DWN_SAVE);
  if (rs.code != E220_SUCCESS) {
    Serial.printf("ERR: setConfiguration failed: %d\n", rs.code);
    return false;
  }
  delay(200);
  return true;
}

// Measure airtime for a single frame of `size` bytes.
// Returns airtime in microseconds, or 0 on timeout/error.
unsigned long measure_one(int size) {
  uint8_t payload[50] = {};
  for (int i = 0; i < size; i++) payload[i] = (uint8_t)(i & 0xFF);

  // Wait for AUX to be HIGH (idle) before sending
  unsigned long t0 = micros();
  while (digitalRead(PIN_AUX) == LOW) {
    if (micros() - t0 > AUX_TIMEOUT_US) return 0;
  }

  // Send — AUX will go LOW during TX
  ResponseStatus rs = e220.sendMessage(payload, (uint8_t)size);
  if (rs.code != E220_SUCCESS) return 0;

  // Wait for AUX to go LOW (TX started)
  t0 = micros();
  while (digitalRead(PIN_AUX) == HIGH) {
    if (micros() - t0 > 50000UL) break;  // 50ms max for AUX to go LOW
  }
  unsigned long tx_start = micros();

  // Wait for AUX to go HIGH (TX complete)
  while (digitalRead(PIN_AUX) == LOW) {
    if (micros() - tx_start > AUX_TIMEOUT_US) return 0;
  }
  unsigned long tx_end = micros();

  return tx_end - tx_start;
}

void run_preset(const Preset& preset) {
  Serial.printf("\n# Preset: %s\n", preset.name);
  Serial.printf("# preset,size_bytes,sample,airtime_us\n");

  // Return to normal mode before reconfiguring (M0=M1=LOW = normal TX/RX mode).
  digitalWrite(PIN_M0, LOW);
  digitalWrite(PIN_M1, LOW);
  delay(200);

  if (!configure_preset(preset.air_rate)) {
    Serial.printf("ERR: failed to configure preset %s\n", preset.name);
    return;
  }
  delay(500);

  for (int size = MIN_SIZE; size <= MAX_SIZE; size++) {
    for (int s = 0; s < SAMPLES_PER_SIZE; s++) {
      delay(200);  // inter-frame gap
      unsigned long airtime = measure_one(size);
      Serial.printf("%s,%d,%d,%lu\n", preset.name, size, s + 1, airtime);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // Wait up to 10s for serial monitor, then proceed regardless.
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 10000) {}
  delay(500);
  Serial.println("# E220 airtime bench — issue #332");
  Serial.println("# Send any character to start...");
  // Wait for trigger character from host
  t0 = millis();
  while (!Serial.available() && millis() - t0 < 30000) {}
  while (Serial.available()) Serial.read();
  Serial.println("# Starting...");

  loraSerial.begin(9600, SERIAL_8N1, PIN_E220_TX, PIN_E220_RX);
  delay(100);

  if (!e220.begin()) {
    Serial.println("ERR: e220.begin() failed");
    return;
  }
  delay(200);

  for (const auto& preset : kPresets) {
    run_preset(preset);
  }

  Serial.println("\n# DONE");
}

void loop() {}
