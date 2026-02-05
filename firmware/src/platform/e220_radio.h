#pragma once

#include <cstddef>
#include <cstdint>

#include <Arduino.h>
#include <LoRa_E220.h>

#include "hw_profile.h"
#include "naviga/hal/interfaces.h"

namespace naviga {

class E220Radio : public IRadio {
 public:
  explicit E220Radio(const Pins& pins);

  bool begin();
  bool is_ready() const;
  bool rssi_available() const;

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
};

} // namespace naviga
