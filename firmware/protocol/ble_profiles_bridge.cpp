#include "ble_profiles_bridge.h"

#include "platform/naviga_storage.h"

#include <algorithm>
#include <cstring>

namespace naviga {
namespace protocol {

namespace {

// S04 #467 first-phase encoding (document in code; align with app BleProfilesParser). Little-endian throughout.
// List: n_radio(1), radio_id[0..n_radio-1](4 B LE each), n_user(1), user_id[0..n_user-1](4 B LE each).
// Read request (app→FW): type(1): 0=radio, 1=user; id(4 B LE).
// Radio response: profile_id(4), kind(1), channel_slot(1), rate_tier(1), tx_power_baseline_step(1), label_len(1), label(0..24).
// Role response: role_id(4), min_interval_sec(2), max_silence_10s(1), min_displacement_m(4 float LE).
constexpr unsigned kMaxListBytes = 64u;

inline void write_u32_le(uint8_t* p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v & 0xFFu);
  p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
  p[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
  p[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
}

// Radio profile response: profile_id(4), kind(1), channel_slot(1), rate_tier(1), tx_power_baseline_step(1), label_len(1), label(0..24).
constexpr unsigned kRadioProfileFixedHeader = 4 + 1 + 1 + 1 + 1 + 1;  // 9
constexpr unsigned kRadioProfileMaxBytes = kRadioProfileFixedHeader + 1 + naviga::RadioProfileRecord::kMaxLabelLen;

// Role profile response: role_id(4), min_interval_sec(2), max_silence_10s(1), min_displacement_m(4 float LE).
constexpr unsigned kRoleProfileBytes = 4 + 2 + 1 + 4;  // 11

}  // namespace

void BleProfilesBridge::update_profiles_list(IBleTransport& transport) const {
  uint8_t buf[kMaxListBytes];
  size_t off = 0;

  // n_radio = 1, radio_id = 0
  buf[off++] = 1u;
  write_u32_le(buf + off, 0u);
  off += 4;

  // n_user = 3, user_ids = 0, 1, 2
  buf[off++] = 3u;
  write_u32_le(buf + off, 0u);
  off += 4;
  write_u32_le(buf + off, 1u);
  off += 4;
  write_u32_le(buf + off, 2u);
  off += 4;

  transport.set_profiles_list(buf, off);
}

void BleProfilesBridge::update_profile_read(IBleTransport& transport) const {
  if (!transport.has_profile_read_request()) {
    transport.set_profile_read_response(nullptr, 0);
    return;
  }

  uint8_t type = 0;
  uint32_t id = 0;
  if (!transport.get_profile_read_request(&type, &id)) {
    transport.clear_profile_read_request();
    return;
  }

  uint8_t buf[kRadioProfileMaxBytes];
  size_t len = 0;

  if (type == 0) {
    // Radio profile: only id 0 supported in first phase
    if (id != 0) {
      transport.set_profile_read_response(nullptr, 0);
      transport.clear_profile_read_request();
      return;
    }
    naviga::RadioProfileRecord rec;
    naviga::get_factory_default_radio_profile(&rec);
    write_u32_le(buf, rec.profile_id);
    buf[4] = static_cast<uint8_t>(rec.kind);
    buf[5] = rec.channel_slot;
    buf[6] = rec.rate_tier;
    buf[7] = rec.tx_power_baseline_step;
    size_t label_len = strnlen(rec.label, naviga::RadioProfileRecord::kMaxLabelLen);
    buf[8] = static_cast<uint8_t>(label_len);
    if (label_len > 0) {
      memcpy(buf + 9, rec.label, label_len);
    }
    len = 9 + label_len;
  } else if (type == 1) {
    // User/role profile: ids 0, 1, 2 (OOTB Person, Dog, Infra)
    if (id > 2) {
      transport.set_profile_read_response(nullptr, 0);
      transport.clear_profile_read_request();
      return;
    }
    naviga::RoleProfileRecord rec;
    naviga::get_ootb_role_profile(id, &rec);
    write_u32_le(buf, id);
    buf[4] = static_cast<uint8_t>(rec.min_interval_sec & 0xFFu);
    buf[5] = static_cast<uint8_t>((rec.min_interval_sec >> 8) & 0xFFu);
    buf[6] = rec.max_silence_10s;
    uint32_t fbits;
    memcpy(&fbits, &rec.min_displacement_m, sizeof(fbits));
    write_u32_le(buf + 7, fbits);
    len = kRoleProfileBytes;
  } else {
    transport.set_profile_read_response(nullptr, 0);
    transport.clear_profile_read_request();
    return;
  }

  transport.set_profile_read_response(buf, len);
  transport.clear_profile_read_request();
}

}  // namespace protocol
}  // namespace naviga
