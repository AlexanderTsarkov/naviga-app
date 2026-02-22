#include "platform/provisioning_adapter.h"

#include <Arduino.h>
#include <cstring>

#ifdef ESP32
#include <esp_system.h>
#endif

namespace naviga {

void ProvisioningAdapter::set_radio_boot_info(int result_enum, const char* message) {
  shell_.set_radio_boot_info(result_enum, message);
}

void ProvisioningAdapter::tick(uint32_t /*now_ms*/) {
  if (!Serial || Serial.available() <= 0) return;
  while (Serial.available() > 0 && line_len_ < ProvisioningShell::kLineMax - 1) {
    int c = Serial.read();
    if (c < 0) break;
    if (c == '\n' || c == '\r') {
      line_buf_[line_len_] = '\0';
      if (line_len_ > 0) {
        char response[ProvisioningShell::kResponseMax] = {};
        bool reboot_requested = false;
        if (shell_.handle_line(line_buf_, response, sizeof(response), &reboot_requested)) {
          if (response[0] != '\0') {
            Serial.println(response);
          }
          if (reboot_requested) {
#ifdef ESP32
            esp_restart();
#else
            Serial.println("ERR: reboot not available");
#endif
          }
        }
      }
      line_len_ = 0;
      return;
    }
    line_buf_[line_len_++] = static_cast<char>(c);
  }
  if (line_len_ >= ProvisioningShell::kLineMax - 1) {
    line_len_ = 0;
    Serial.println("ERR: line too long");
  }
}

}  // namespace naviga
