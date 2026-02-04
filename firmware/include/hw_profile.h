#pragma once

namespace naviga {

struct Pins {
  int lora_m0;
  int lora_m1;
  int lora_rx;
  int lora_tx;
  int lora_aux;
  int i2c_scl;
  int i2c_sda;
  int gps_rx; // optional, -1 if not present
  int gps_tx; // optional, -1 if not present
};

enum class RadioType {
  E220_UART,
  E22_UART,
  UNKNOWN,
};

struct Caps {
  bool has_screen;
  bool has_gnss;
  RadioType radio_type;
  int band_mhz;
};

struct HwProfile {
  const char* name;
  Pins pins;
  Caps caps;
};

const HwProfile& get_hw_profile();

} // namespace naviga
