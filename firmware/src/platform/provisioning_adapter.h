#pragma once

#include <cstddef>
#include <cstdint>

#include "services/provisioning_shell.h"

namespace naviga {

/**
 * Platform adapter: UART I/O + non-blocking line reader; calls ProvisioningShell;
 * performs reboot when shell requests it. Implementation uses Arduino/esp_* in .cpp only.
 */
class ProvisioningAdapter {
 public:
  /** Call once after radio modem begin(); result_enum = 0=Ok, 1=Repaired, 2=RepairFailed. */
  void set_radio_boot_info(int result_enum, const char* message);

  /** Optional: enable "debug on/off" in shell to toggle instrumentation (e.g. packet/peer logs). */
  void set_instrumentation_flag(bool* flag);

  /** Optional: enable "gnss off|nofix|fix|move" scenario override in shell (#288). */
  void set_gnss_override(class GnssScenarioOverride* ptr);

  /** Read one line (non-blocking), handle via shell, print response; at most one line per call. */
  void tick(uint32_t now_ms);

 private:
  ProvisioningShell shell_;
  char line_buf_[ProvisioningShell::kLineMax] = {};
  size_t line_len_ = 0;
};

}  // namespace naviga
