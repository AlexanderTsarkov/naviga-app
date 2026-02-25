#include "services/provisioning_shell.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "platform/naviga_storage.h"
#include "services/gnss_scenario_override.h"

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
                  "help|status|get role|radio|profile|set role <0-2>|set radio <0>|profile interval|silence|distance <val>|reset|reboot|debug on|off|gnss ...");
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
    if (!t1[0] || (std::strcmp(t1, "role") != 0 && std::strcmp(t1, "radio") != 0 && std::strcmp(t1, "profile") != 0)) {
      std::snprintf(out_response, out_response_size, "ERR: get role|radio|profile");
      return true;
    }
    if (std::strcmp(t1, "profile") == 0) {
      RoleProfileRecord rec{};
      bool valid = false;
      load_current_role_profile_record(&rec, &valid);
      std::snprintf(out_response, out_response_size,
                    "interval_s=%u silence_10s=%u dist_m=%.1f valid=%d",
                    static_cast<unsigned>(rec.min_interval_sec),
                    static_cast<unsigned>(rec.max_silence_10s),
                    static_cast<double>(rec.min_displacement_m),
                    valid ? 1 : 0);
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
      RoleProfileRecord ootb{};
      get_ootb_role_profile(static_cast<uint32_t>(id), &ootb);
      if (!save_current_role_profile_record(ootb)) {
        std::snprintf(out_response, out_response_size, "ERR: profile record save failed");
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
  if (std::strcmp(t0, "profile") == 0) {
    if (std::strcmp(t1, "interval") == 0 && t2[0]) {
      const unsigned long v = std::strtoul(t2, nullptr, 10);
      if (v < 1 || v > 3600) {
        std::snprintf(out_response, out_response_size, "ERR: interval 1-3600");
        return true;
      }
      RoleProfileRecord rec{};
      bool valid = false;
      load_current_role_profile_record(&rec, &valid);
      rec.min_interval_sec = static_cast<uint16_t>(v);
      if ((static_cast<uint32_t>(rec.max_silence_10s) * 10U) < (3U * rec.min_interval_sec)) {
        std::snprintf(out_response, out_response_size, "ERR: maxSilence must be >= 3*minInterval");
        return true;
      }
      if (!save_current_role_profile_record(rec)) {
        std::snprintf(out_response, out_response_size, "ERR: profile save failed");
        return true;
      }
      std::snprintf(out_response, out_response_size, "OK; interval_s=%u; reboot to apply", static_cast<unsigned>(rec.min_interval_sec));
      return true;
    }
    if (std::strcmp(t1, "silence") == 0 && t2[0]) {
      const unsigned long v = std::strtoul(t2, nullptr, 10);
      if (v < 1 || v > 255) {
        std::snprintf(out_response, out_response_size, "ERR: silence 1-255 (10s steps)");
        return true;
      }
      RoleProfileRecord rec{};
      bool valid = false;
      load_current_role_profile_record(&rec, &valid);
      rec.max_silence_10s = static_cast<uint8_t>(v);
      if ((static_cast<uint32_t>(rec.max_silence_10s) * 10U) < (3U * rec.min_interval_sec)) {
        std::snprintf(out_response, out_response_size, "ERR: maxSilence must be >= 3*minInterval");
        return true;
      }
      if (!save_current_role_profile_record(rec)) {
        std::snprintf(out_response, out_response_size, "ERR: profile save failed");
        return true;
      }
      std::snprintf(out_response, out_response_size, "OK; silence_10s=%u; reboot to apply", static_cast<unsigned>(rec.max_silence_10s));
      return true;
    }
    if (std::strcmp(t1, "distance") == 0 && t2[0]) {
      const float v = std::strtof(t2, nullptr);
      if (v < 0.0f || v > 1000.0f) {
        std::snprintf(out_response, out_response_size, "ERR: distance 0-1000 (m)");
        return true;
      }
      RoleProfileRecord rec{};
      bool valid = false;
      load_current_role_profile_record(&rec, &valid);
      rec.min_displacement_m = v;
      if (!save_current_role_profile_record(rec)) {
        std::snprintf(out_response, out_response_size, "ERR: profile save failed");
        return true;
      }
      std::snprintf(out_response, out_response_size, "OK; dist_m=%.1f; reboot to apply", static_cast<double>(rec.min_displacement_m));
      return true;
    }
    std::snprintf(out_response, out_response_size, "ERR: profile interval|silence|distance <val>");
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
  if (std::strcmp(t0, "gnss") == 0) {
    if (!gnss_override_) {
      std::snprintf(out_response, out_response_size, "ERR: gnss override not available");
      return true;
    }
    if (std::strcmp(t1, "off") == 0) {
      gnss_override_->set_off();
      std::snprintf(out_response, out_response_size, "OK; gnss override off (use real GNSS)");
      return true;
    }
    if (std::strcmp(t1, "nofix") == 0) {
      gnss_override_->set_nofix();
      std::snprintf(out_response, out_response_size, "OK; gnss override NO_FIX");
      return true;
    }
    if (std::strcmp(t1, "fix") == 0 && t2[0]) {
      char* end = nullptr;
      const long lat_e7 = std::strtol(t2, &end, 10);
      const long lon_e7 = end && *end ? std::strtol(end, nullptr, 10) : 0;
      gnss_override_->set_fix(static_cast<int32_t>(lat_e7), static_cast<int32_t>(lon_e7));
      std::snprintf(out_response, out_response_size, "OK; gnss override FIX lat_e7=%ld lon_e7=%ld",
                    static_cast<long>(lat_e7), static_cast<long>(lon_e7));
      return true;
    }
    if (std::strcmp(t1, "move") == 0 && t2[0]) {
      char* end = nullptr;
      const long dlat_e7 = std::strtol(t2, &end, 10);
      const long dlon_e7 = end && *end ? std::strtol(end, nullptr, 10) : 0;
      gnss_override_->move(static_cast<int32_t>(dlat_e7), static_cast<int32_t>(dlon_e7));
      std::snprintf(out_response, out_response_size, "OK; gnss override move dlat_e7=%ld dlon_e7=%ld",
                    static_cast<long>(dlat_e7), static_cast<long>(dlon_e7));
      return true;
    }
    std::snprintf(out_response, out_response_size,
                  "ERR: gnss off|nofix|fix <lat_e7> <lon_e7>|move <dlat_e7> <dlon_e7>");
    return true;
  }
  std::snprintf(out_response, out_response_size, "ERR: unknown command (type help)");
  return true;
}

}  // namespace naviga
