#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {

/**
 * Platform-agnostic provisioning command handler per provisioning_interface_v0.
 * No platform I/O or SoC APIs; uses naviga_storage for load/save/reset.
 * I/O and reboot are responsibility of the platform adapter.
 */
class ProvisioningShell {
 public:
  static constexpr size_t kResponseMax = 256;
  static constexpr size_t kLineMax = 128;

  /** Call once after radio modem begin(); result_enum = 0=Ok, 1=Repaired, 2=RepairFailed. */
  void set_radio_boot_info(int result_enum, const char* message);

  /**
   * Parse and execute one command line. Fills out_response with reply text (null-terminated).
   * If the command is "reboot", sets *reboot_requested = true (caller must perform restart).
   * Returns true if a response was written to out_response.
   */
  bool handle_line(const char* line,
                   char* out_response,
                   size_t out_response_size,
                   bool* reboot_requested = nullptr);

 private:
  static const char* radio_boot_result_str(int result_enum);

  int radio_boot_result_ = 0;
  char radio_boot_message_[48] = {};
};

}  // namespace naviga
