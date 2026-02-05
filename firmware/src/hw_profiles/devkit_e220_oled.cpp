#include "hw_profile.h"

namespace naviga {

extern const HwProfile kDevkitE220OledProfile = {
    "devkit_e220_oled",
    Pins{
        .lora_m0 = 10,
        .lora_m1 = 11,
        .lora_rx = 12,
        .lora_tx = 13,
        .lora_aux = 14,
        .role_pin = 8,
        .i2c_scl = 17,
        .i2c_sda = 18,
        .gps_rx = -1,
        .gps_tx = -1,
    },
    Caps{
        .has_screen = true,
        .has_gnss = false,
        .radio_type = RadioType::E220_UART,
        .band_mhz = 433,
    },
};

} // namespace naviga
