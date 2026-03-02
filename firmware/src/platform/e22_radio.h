#pragma once

// This header requires the EByte LoRa E22 library (xreef/EByte LoRa E22 library).
// ONLY include from translation units guarded by HW_PROFILE_DEVKIT_E22_OLED_GNSS.
// The canonical include point is radio_factory.cpp (conditionally) and e22_radio.cpp
// (guarded by #if defined(HW_PROFILE_DEVKIT_E22_OLED_GNSS) at the top of that file).

#include <cstddef>
#include <cstdint>

#include <Arduino.h>
#include <LoRa_E22.h>

#include "hw_profile.h"
#include "naviga/hal/interfaces.h"
#include "naviga/hal/radio_preset.h"

namespace naviga {

enum class E22BootConfigResult {
  Ok,          // Critical params already match; no repair.
  Repaired,    // Mismatch was repaired and re-verified.
  RepairFailed // Repair attempted but re-read did not match (or write failed).
};

class E22Radio : public IRadio {
 public:
  explicit E22Radio(const Pins& pins);

  // begin() with default preset (RadioPresetId::Default).
  bool begin();
  // begin() with an explicit preset; airRate is normalized if < kMinAirRate.
  bool begin(const RadioPreset& preset);
  bool is_ready() const;

  /** Result of last begin() verify-and-repair. */
  E22BootConfigResult last_boot_config_result() const { return last_boot_result_; }
  /** Short message for log: what was repaired or failure reason; empty if Ok. */
  const char* last_boot_config_message() const { return last_boot_message_; }

  bool send(const uint8_t* data, size_t len) override;
  bool recv(uint8_t* out, size_t max_len, size_t* out_len) override;
  int8_t last_rssi_dbm() const override;
  bool rssi_available() const override;
  RadioBootConfigResult boot_config_result() const override;
  const char* boot_config_message() const override;

 private:
  Pins pins_;
  HardwareSerial serial_{2};
  LoRa_E22 radio_;
  bool ready_ = false;
  bool rssi_enabled_ = false;
  int8_t last_rssi_dbm_ = 0;

  E22BootConfigResult last_boot_result_ = E22BootConfigResult::Ok;
  static constexpr size_t kBootMessageLen = 64;
  char last_boot_message_[kBootMessageLen] = {};
};

} // namespace naviga
