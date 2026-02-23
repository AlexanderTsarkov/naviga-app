#include "services/provisioning_shell.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "platform/naviga_storage.h"

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

void ProvisioningShell::set_radio_boot_info(int result_enum, const char* message) {
  radio_boot_result_ = result_enum;
  if (message) {
    std::snprintf(radio_boot_message_, sizeof(radio_boot_message_), "%s", message);
  } else {
    radio_boot_message_[0] = '\0';
  }
}

const char* ProvisioningShell::radio_boot_result_str(int result_enum) {
  switch (result_enum) {
    case 0: return "OK";
    case 1: return "REPAIRED";
    case 2: return "REPAIR_FAILED";
    default: return "?";
  }
}

bool ProvisioningShell::handle_line(const char* line,
                                    char* out_response,
                                    size_t out_response_size,
                                    bool* reboot_requested) {
  if (!line || !out_response || out_response_size == 0) return false;
  out_response[0] = '\0';
  if (reboot_requested) *reboot_requested = false;

  char t0[32] = {};
  char t1[32] = {};
  char t2[32] = {};
  split_tokens(line, t0, sizeof(t0), t1, sizeof(t1), t2, sizeof(t2));

  if (std::strcmp(t0, "help") == 0) {
    std::snprintf(out_response, out_response_size,
                  "help|status|get role|get radio|set role <0-2>|set radio <0>|reset|reboot|debug on|off");
    return true;
  }
  if (std::strcmp(t0, "debug") == 0) {
    if (!instrumentation_flag_) {
      std::snprintf(out_response, out_response_size, "ERR: instrumentation not available");
      return true;
    }
    if (std::strcmp(t1, "on") == 0) {
      *instrumentation_flag_ = true;
      std::snprintf(out_response, out_response_size, "OK; instrumentation on");
      return true;
    }
    if (std::strcmp(t1, "off") == 0) {
      *instrumentation_flag_ = false;
      std::snprintf(out_response, out_response_size, "OK; instrumentation off");
      return true;
    }
    std::snprintf(out_response, out_response_size, "ERR: debug on|off");
    return true;
  }
  if (std::strcmp(t0, "status") == 0) {
    PersistedPointers ptrs{};
    const bool loaded = load_pointers(&ptrs);
    const char* source = (loaded && ptrs.has_current_role && ptrs.has_current_radio) ? "persisted" : "default";
    std::snprintf(out_response, out_response_size,
                 "role_cur=%lu role_prev=%lu radio_cur=%lu radio_prev=%lu source=%s radio_boot=%s radio_boot_msg=\"%s\"",
                 static_cast<unsigned long>(ptrs.current_role_id),
                 static_cast<unsigned long>(ptrs.previous_role_id),
                 static_cast<unsigned long>(ptrs.current_radio_profile_id),
                 static_cast<unsigned long>(ptrs.previous_radio_profile_id),
                 source,
                 radio_boot_result_str(radio_boot_result_),
                 radio_boot_message_);
    return true;
  }
  if (std::strcmp(t0, "get") == 0) {
    if (!t1[0] || (std::strcmp(t1, "role") != 0 && std::strcmp(t1, "radio") != 0)) {
      std::snprintf(out_response, out_response_size, "ERR: get role|radio");
      return true;
    }
    PersistedPointers ptrs{};
    load_pointers(&ptrs);
    if (std::strcmp(t1, "role") == 0) {
      std::snprintf(out_response, out_response_size, "role_cur=%lu role_prev=%lu",
                    static_cast<unsigned long>(ptrs.current_role_id),
                    static_cast<unsigned long>(ptrs.previous_role_id));
    } else {
      std::snprintf(out_response, out_response_size, "radio_cur=%lu radio_prev=%lu",
                    static_cast<unsigned long>(ptrs.current_radio_profile_id),
                    static_cast<unsigned long>(ptrs.previous_radio_profile_id));
    }
    return true;
  }
  if (std::strcmp(t0, "set") == 0) {
    if (!t1[0] || !t2[0]) {
      std::snprintf(out_response, out_response_size, "ERR: set role <id>|set radio <id>");
      return true;
    }
    if (std::strcmp(t1, "role") == 0) {
      const unsigned long id = std::strtoul(t2, nullptr, 10);
      if (id > kMaxRoleId) {
        std::snprintf(out_response, out_response_size, "ERR: role id 0-2");
        return true;
      }
      PersistedPointers ptrs{};
      load_pointers(&ptrs);
      const uint32_t new_prev_role = ptrs.has_current_role ? ptrs.current_role_id : 0;
      if (!save_pointers(static_cast<uint32_t>(id), ptrs.current_radio_profile_id, new_prev_role, ptrs.previous_radio_profile_id)) {
        std::snprintf(out_response, out_response_size, "ERR: save failed");
        return true;
      }
      std::snprintf(out_response, out_response_size, "OK; reboot to apply");
      return true;
    }
    if (std::strcmp(t1, "radio") == 0) {
      const unsigned long id = std::strtoul(t2, nullptr, 10);
      if (id != 0) {
        std::snprintf(out_response, out_response_size, "ERR: radio id 0 (only default profile)");
        return true;
      }
      PersistedPointers ptrs{};
      load_pointers(&ptrs);
      const uint32_t new_prev_radio = ptrs.has_current_radio ? ptrs.current_radio_profile_id : 0;
      if (!save_pointers(ptrs.current_role_id, static_cast<uint32_t>(id), ptrs.previous_role_id, new_prev_radio)) {
        std::snprintf(out_response, out_response_size, "ERR: save failed");
        return true;
      }
      std::snprintf(out_response, out_response_size, "OK; reboot to apply");
      return true;
    }
    std::snprintf(out_response, out_response_size, "ERR: set role|radio <id>");
    return true;
  }
  if (std::strcmp(t0, "reset") == 0) {
    if (!factory_reset_pointers()) {
      std::snprintf(out_response, out_response_size, "ERR: factory reset failed");
      return true;
    }
    std::snprintf(out_response, out_response_size, "OK; reboot to apply");
    return true;
  }
  if (std::strcmp(t0, "reboot") == 0) {
    std::snprintf(out_response, out_response_size, "rebooting...");
    if (reboot_requested) *reboot_requested = true;
    return true;
  }
  std::snprintf(out_response, out_response_size, "ERR: unknown command (type help)");
  return true;
}

}  // namespace naviga
