#include "hw_profile.h"

namespace naviga {

extern const HwProfile kDevkitE220OledProfile = {
    "devkit_e220_oled",
    Pins{
        .lora_m0 = 4,   // safe: RTC_GPIO4, no strapping/flash/JTAG/USB — fix #326
        .lora_m1 = 5,   // safe: RTC_GPIO5, no strapping/flash/JTAG/USB — fix #326
        .lora_rx = 1,   // E220 RXD → GPIO1 (ESP32 UART2 TX) — fix #326
        .lora_tx = 2,   // E220 TXD → GPIO2 (ESP32 UART2 RX) — fix #326
        .lora_aux = 47, // safe: no strapping/flash/JTAG/USB — fix #326
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
