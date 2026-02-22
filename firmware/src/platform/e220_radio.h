#pragma once

#include <cstddef>
#include <cstdint>

#include <Arduino.h>
#include <LoRa_E220.h>

#include "hw_profile.h"
#include "naviga/hal/interfaces.h"

namespace naviga {

enum class E220BootConfigResult {
  Ok,          // Critical params already match; no repair.
  Repaired,    // Mismatch was repaired and re-verified.
  RepairFailed // Repair attempted but re-read did not match (or write failed).
};

class E220Radio : public IRadio {
 public:
  explicit E220Radio(const Pins& pins);

  bool begin();
  bool is_ready() const;
  bool rssi_available() const;

  /** Result of last begin() verify-and-repair (per module_boot_config_v0). */
  E220BootConfigResult last_boot_config_result() const { return last_boot_result_; }
  /** Short message for log: what was repaired or failure reason; empty if Ok. */
  const char* last_boot_config_message() const { return last_boot_message_; }

  bool send(const uint8_t* data, size_t len) override;
  bool recv(uint8_t* out, size_t max_len, size_t* out_len) override;
  int8_t last_rssi_dbm() const override;

 private:
  Pins pins_;
  HardwareSerial serial_{2};
  LoRa_E220 radio_;
  bool ready_ = false;
  bool rssi_enabled_ = false;
  int8_t last_rssi_dbm_ = 0;

  E220BootConfigResult last_boot_result_ = E220BootConfigResult::Ok;
  static constexpr size_t kBootMessageLen = 48;
  char last_boot_message_[kBootMessageLen] = {};
};

} // namespace naviga
