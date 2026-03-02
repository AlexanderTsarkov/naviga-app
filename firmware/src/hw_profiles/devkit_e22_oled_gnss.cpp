#include "hw_profile.h"

namespace naviga {

// E22-400T30D devkit profile — pin-to-pin identical to devkit_e220_oled_gnss.
// RadioType::E22_UART selects E22Radio adapter in app_services.cpp.
extern const HwProfile kDevkitE22OledGnssProfile = {
    "devkit_e22_oled_gnss",
    Pins{
        .lora_m0  = 4,   // M0 — same as E220 devkit
        .lora_m1  = 5,   // M1 — same as E220 devkit
        .lora_rx  = 1,   // E22 RXD → GPIO1 (ESP32 UART2 TX)
        .lora_tx  = 2,   // E22 TXD → GPIO2 (ESP32 UART2 RX)
        .lora_aux = 47,  // AUX — same as E220 devkit
        .role_pin = 8,
        .i2c_scl  = 17,
        .i2c_sda  = 18,
        .gps_rx   = 16,
        .gps_tx   = 15,
    },
    Caps{
        .has_screen = true,
        .has_gnss   = true,
        .radio_type = RadioType::E22_UART,
        .band_mhz   = 433,
    },
};

} // namespace naviga
