#include "services/serial_provisioning.h"

#include <Arduino.h>
#include <cstdlib>
#include <cstring>

#include "platform/naviga_storage.h"

#ifdef ESP32
#include <esp_system.h>
#endif

namespace naviga {

namespace {

constexpr uint32_t kMaxRoleId = 2;
constexpr uint32_t kDefaultRoleId = 0;
constexpr uint32_t kDefaultRadioProfileId = 0;

void split_tokens(const char* line, char* t0, size_t len0, char* t1, size_t len1, char* t2, size_t len2) {
  if (!line || len0 == 0) return;
  while (*line == ' ' || *line == '\t') line++;
  const char* p = line;
  while (*p && *p != ' ' && *p != '\t') p++;
  size_t n0 = static_cast<size_t>(p - line);
  if (n0 >= len0) n0 = len0 - 1;
  std::memcpy(t0, line, n0);
  t0[n0] = '\0';
  if (t1 && len1) {
    while (*p == ' ' || *p == '\t') p++;
    const char* start1 = p;
    while (*p && *p != ' ' && *p != '\t') p++;
    size_t n1 = static_cast<size_t>(p - start1);
    if (n1 >= len1) n1 = len1 - 1;
    std::memcpy(t1, start1, n1);
    t1[n1] = '\0';
  }
  if (t2 && len2) {
    while (*p == ' ' || *p == '\t') p++;
    size_t n2 = std::strlen(p);
    if (n2 >= len2) n2 = len2 - 1;
    std::memcpy(t2, p, n2 + 1);
  }
}

}  // namespace

void SerialProvisioning::set_e220_boot_info(int result_enum, const char* message) {
  e220_result_ = result_enum;
  if (message) {
    std::snprintf(e220_message_, sizeof(e220_message_), "%s", message);
  } else {
    e220_message_[0] = '\0';
  }
}

const char* SerialProvisioning::e220_result_str(int result_enum) {
  switch (result_enum) {
    case 0: return "OK";
    case 1: return "REPAIRED";
    case 2: return "REPAIR_FAILED";
    default: return "?";
  }
}

void SerialProvisioning::reply(const char* line) {
  if (Serial) {
    Serial.println(line);
  }
}

void SerialProvisioning::tick(uint32_t /*now_ms*/) {
  if (!Serial || Serial.available() <= 0) return;
  while (Serial.available() > 0 && line_len_ < kLineMax - 1) {
    int c = Serial.read();
    if (c < 0) break;
    if (c == '\n' || c == '\r') {
      line_buf_[line_len_] = '\0';
      if (line_len_ > 0) {
        process_line();
      }
      line_len_ = 0;
      return;
    }
    line_buf_[line_len_++] = static_cast<char>(c);
  }
  if (line_len_ >= kLineMax - 1) {
    line_len_ = 0;
    reply("ERR: line too long");
  }
}

void SerialProvisioning::process_line() {
  char t0[32] = {};
  char t1[32] = {};
  char t2[32] = {};
  split_tokens(line_buf_, t0, sizeof(t0), t1, sizeof(t1), t2, sizeof(t2));

  if (std::strcmp(t0, "help") == 0) {
    cmd_help();
    return;
  }
  if (std::strcmp(t0, "status") == 0) {
    cmd_status();
    return;
  }
  if (std::strcmp(t0, "get") == 0) {
    cmd_get(t1);
    return;
  }
  if (std::strcmp(t0, "set") == 0) {
    cmd_set(t1, t2);
    return;
  }
  if (std::strcmp(t0, "reset") == 0) {
    cmd_reset();
    return;
  }
  if (std::strcmp(t0, "reboot") == 0) {
    cmd_reboot();
    return;
  }
  reply("ERR: unknown command (type help)");
}

void SerialProvisioning::cmd_help() {
  reply("help|status|get role|get radio|set role <0-2>|set radio <0>|reset|reboot");
}

void SerialProvisioning::cmd_status() {
  PersistedPointers ptrs{};
  const bool loaded = load_pointers(&ptrs);
  const char* source = (loaded && ptrs.has_current_role && ptrs.has_current_radio) ? "persisted" : "default";
  char buf[160] = {};
  std::snprintf(buf, sizeof(buf),
               "role_cur=%lu role_prev=%lu radio_cur=%lu radio_prev=%lu source=%s e220_boot=%s e220_msg=\"%s\"",
               static_cast<unsigned long>(ptrs.current_role_id),
               static_cast<unsigned long>(ptrs.previous_role_id),
               static_cast<unsigned long>(ptrs.current_radio_profile_id),
               static_cast<unsigned long>(ptrs.previous_radio_profile_id),
               source,
               e220_result_str(e220_result_),
               e220_message_);
  reply(buf);
}

void SerialProvisioning::cmd_get(const char* arg) {
  if (!arg || (std::strcmp(arg, "role") != 0 && std::strcmp(arg, "radio") != 0)) {
    reply("ERR: get role|radio");
    return;
  }
  PersistedPointers ptrs{};
  load_pointers(&ptrs);
  char buf[64] = {};
  if (std::strcmp(arg, "role") == 0) {
    std::snprintf(buf, sizeof(buf), "role_cur=%lu role_prev=%lu",
                  static_cast<unsigned long>(ptrs.current_role_id),
                  static_cast<unsigned long>(ptrs.previous_role_id));
  } else {
    std::snprintf(buf, sizeof(buf), "radio_cur=%lu radio_prev=%lu",
                  static_cast<unsigned long>(ptrs.current_radio_profile_id),
                  static_cast<unsigned long>(ptrs.previous_radio_profile_id));
  }
  reply(buf);
}

void SerialProvisioning::cmd_set(const char* arg1, const char* arg2) {
  if (!arg1 || !arg2) {
    reply("ERR: set role <id>|set radio <id>");
    return;
  }
  if (std::strcmp(arg1, "role") == 0) {
    const unsigned long id = std::strtoul(arg2, nullptr, 10);
    if (id > kMaxRoleId) {
      reply("ERR: role id 0-2");
      return;
    }
    PersistedPointers ptrs{};
    load_pointers(&ptrs);
    const uint32_t new_prev_role = ptrs.has_current_role ? ptrs.current_role_id : 0;
    if (!save_pointers(static_cast<uint32_t>(id), ptrs.current_radio_profile_id, new_prev_role, ptrs.previous_radio_profile_id)) {
      reply("ERR: save failed");
      return;
    }
    reply("OK; reboot to apply");
    return;
  }
  if (std::strcmp(arg1, "radio") == 0) {
    const unsigned long id = std::strtoul(arg2, nullptr, 10);
    if (id != 0) {
      reply("ERR: radio id 0 (only default profile)");
      return;
    }
    PersistedPointers ptrs{};
    load_pointers(&ptrs);
    const uint32_t new_prev_radio = ptrs.has_current_radio ? ptrs.current_radio_profile_id : 0;
    if (!save_pointers(ptrs.current_role_id, static_cast<uint32_t>(id), ptrs.previous_role_id, new_prev_radio)) {
      reply("ERR: save failed");
      return;
    }
    reply("OK; reboot to apply");
    return;
  }
  reply("ERR: set role|radio <id>");
}

void SerialProvisioning::cmd_reset() {
  if (!factory_reset_pointers()) {
    reply("ERR: factory reset failed");
    return;
  }
  reply("OK; reboot to apply");
}

void SerialProvisioning::cmd_reboot() {
#ifdef ESP32
  reply("rebooting...");
  esp_restart();
#else
  reply("ERR: reboot not available");
#endif
}

}  // namespace naviga
