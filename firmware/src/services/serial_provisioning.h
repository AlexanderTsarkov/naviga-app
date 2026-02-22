#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {

/**
 * Serial provisioning shell per provisioning_interface_v0.
 * Non-blocking: call tick() from main loop; at most one line processed per tick.
 * Commands: help, status, get role|radio, set role <id>|radio <id>, reset, reboot.
 * Apply semantics: set/reset persist immediately; reboot to apply on next boot.
 */
class SerialProvisioning {
 public:
  static constexpr size_t kLineMax = 128;

  /** Call once after E220 begin(); result_enum = E220BootConfigResult (0=Ok, 1=Repaired, 2=RepairFailed). */
  void set_e220_boot_info(int result_enum, const char* message);

  /** Process one line from Serial if available; non-blocking. */
  void tick(uint32_t now_ms);

 private:
  void process_line();
  void cmd_help();
  void cmd_status();
  void cmd_get(const char* arg);
  void cmd_set(const char* arg1, const char* arg2);
  void cmd_reset();
  void cmd_reboot();
  void reply(const char* line);
  static const char* e220_result_str(int result_enum);

  char line_buf_[kLineMax] = {};
  size_t line_len_ = 0;
  int e220_result_ = 0;  // E220BootConfigResult as int for storage
  char e220_message_[48] = {};
};

}  // namespace naviga
