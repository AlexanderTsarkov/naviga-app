#include "hw_profile.h"

namespace naviga {

extern const HwProfile kDevkitE220OledGnssProfile = {
    "devkit_e220_oled_gnss",
    Pins{
        .lora_m0 = 10,
        .lora_m1 = 11,
        .lora_rx = 12,
        .lora_tx = 13,
        .lora_aux = 14,
        .role_pin = 8,
        .i2c_scl = 17,
        .i2c_sda = 18,
        .gps_rx = 16,
        .gps_tx = 15,
    },
    Caps{
        .has_screen = true,
        .has_gnss = true,
        .radio_type = RadioType::E220_UART,
        .band_mhz = 433,
    },
};

} // namespace naviga
