#include "platform/naviga_storage.h"

#include <Preferences.h>

namespace naviga {

namespace {

constexpr char kNamespace[] = "naviga";
constexpr char kKeyCurrentRole[] = "role_cur";
constexpr char kKeyCurrentRadio[] = "prof_cur";
constexpr char kKeyPreviousRole[] = "role_prev";
constexpr char kKeyPreviousRadio[] = "prof_prev";
constexpr char kKeyRadioProfileVer[] = "rprof_ver";
constexpr char kKeyProfileIntervalSec[] = "role_interval_s";
constexpr char kKeyProfileSilence10s[] = "role_silence_10";
constexpr char kKeyProfileDistM[] = "role_dist_m";
constexpr char kKeySeq16[] = "seq16";
constexpr char kKeyNodeTableSnapshotLen[] = "nt_snap_len";
constexpr char kKeyNodeTableSnapshot[] = "nt_snap";

// Person (default role) OOTB values per #453 / role_profiles_policy_v0 §3.1
constexpr uint16_t kDefaultMinIntervalSec = 22;
constexpr uint8_t kDefaultMaxSilence10s = 11;
constexpr float kDefaultMinDisplacementM = 30.0f;

}  // namespace

bool load_pointers(PersistedPointers* out) {
  if (!out) {
    return false;
  }
  *out = PersistedPointers{};

  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) {  // read-only
    return false;
  }

  out->current_role_id = prefs.getUInt(kKeyCurrentRole, 0);
  out->has_current_role = prefs.isKey(kKeyCurrentRole);

  out->current_radio_profile_id = prefs.getUInt(kKeyCurrentRadio, 0);
  out->has_current_radio = prefs.isKey(kKeyCurrentRadio);

  out->previous_role_id = prefs.getUInt(kKeyPreviousRole, 0);
  out->has_previous_role = prefs.isKey(kKeyPreviousRole);

  out->previous_radio_profile_id = prefs.getUInt(kKeyPreviousRadio, 0);
  out->has_previous_radio = prefs.isKey(kKeyPreviousRadio);

  prefs.end();
  return true;
}

bool save_pointers(uint32_t current_role_id,
                   uint32_t current_radio_profile_id,
                   uint32_t previous_role_id,
                   uint32_t previous_radio_profile_id) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {  // read-write
    return false;
  }

  prefs.putUInt(kKeyCurrentRole, current_role_id);
  prefs.putUInt(kKeyCurrentRadio, current_radio_profile_id);
  prefs.putUInt(kKeyPreviousRole, previous_role_id);
  prefs.putUInt(kKeyPreviousRadio, previous_radio_profile_id);
  prefs.putUChar(kKeyRadioProfileVer, kRadioProfileSchemaVersion);

  prefs.end();
  return true;
}

bool load_current_role_profile_record(RoleProfileRecord* out, bool* valid) {
  if (!out || !valid) return false;
  *out = RoleProfileRecord{};
  *valid = false;

  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) {
    out->min_interval_sec = kDefaultMinIntervalSec;
    out->max_silence_10s = kDefaultMaxSilence10s;
    out->min_displacement_m = kDefaultMinDisplacementM;
    return false;
  }

  if (!prefs.isKey(kKeyProfileIntervalSec) || !prefs.isKey(kKeyProfileSilence10s) ||
      !prefs.isKey(kKeyProfileDistM)) {
    out->min_interval_sec = kDefaultMinIntervalSec;
    out->max_silence_10s = kDefaultMaxSilence10s;
    out->min_displacement_m = kDefaultMinDisplacementM;
    prefs.end();
    return true;
  }

  out->min_interval_sec = static_cast<uint16_t>(prefs.getUInt(kKeyProfileIntervalSec, kDefaultMinIntervalSec));
  out->max_silence_10s = static_cast<uint8_t>(prefs.getUInt(kKeyProfileSilence10s, kDefaultMaxSilence10s));
  out->min_displacement_m = prefs.getFloat(kKeyProfileDistM, kDefaultMinDisplacementM);

  const bool interval_ok = out->min_interval_sec >= 1 && out->min_interval_sec <= 3600;
  const bool silence_ok = out->max_silence_10s >= 1 && out->max_silence_10s <= 255;
  const bool dist_ok = out->min_displacement_m >= 0.0f && out->min_displacement_m <= 1000.0f;
  const bool invariant = (static_cast<uint32_t>(out->max_silence_10s) * 10U) >= (3U * out->min_interval_sec);

  if (!interval_ok || !silence_ok || !dist_ok || !invariant) {
    out->min_interval_sec = kDefaultMinIntervalSec;
    out->max_silence_10s = kDefaultMaxSilence10s;
    out->min_displacement_m = kDefaultMinDisplacementM;
  } else {
    *valid = true;
  }

  prefs.end();
  return true;
}

bool save_current_role_profile_record(const RoleProfileRecord& record) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) return false;
  prefs.putUInt(kKeyProfileIntervalSec, record.min_interval_sec);
  prefs.putUInt(kKeyProfileSilence10s, record.max_silence_10s);
  prefs.putFloat(kKeyProfileDistM, record.min_displacement_m);
  prefs.end();
  return true;
}

bool factory_reset_pointers() {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }

  prefs.remove(kKeyCurrentRole);
  prefs.remove(kKeyCurrentRadio);
  prefs.remove(kKeyPreviousRole);
  prefs.remove(kKeyPreviousRadio);

  // Persist default pointers (0,0,0,0) so next boot sees consistent state (provisioning_baseline_v0).
  prefs.putUInt(kKeyCurrentRole, 0);
  prefs.putUInt(kKeyCurrentRadio, 0);
  prefs.putUInt(kKeyPreviousRole, 0);
  prefs.putUInt(kKeyPreviousRadio, 0);

  RoleProfileRecord def;
  get_ootb_role_profile(0, &def);
  prefs.putUInt(kKeyProfileIntervalSec, def.min_interval_sec);
  prefs.putUInt(kKeyProfileSilence10s, def.max_silence_10s);
  prefs.putFloat(kKeyProfileDistM, def.min_displacement_m);

  prefs.end();
  return true;
}

void get_factory_default_radio_profile(RadioProfileRecord* out) {
  if (!out) return;
  *out = RadioProfileRecord{};
  out->profile_id = kRadioProfileIdFactoryDefault;
  out->kind = RadioProfileKind::FACTORY;
  out->channel_slot = 1;   // slot 0 reserved dev/test; 1 = factory default
  out->rate_tier = 2;      // 2.4 kbps product default; adapter maps to air_rate
  out->tx_power_baseline_step = 0;  // step 0 = MIN (21 dBm); OOTB uses MIN per product model
}

bool load_seq16(uint16_t* out) {
  if (!out) return false;
  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) return false;
  if (!prefs.isKey(kKeySeq16)) {
    prefs.end();
    return false;
  }
  *out = static_cast<uint16_t>(prefs.getUInt(kKeySeq16, 0));
  prefs.end();
  return true;
}

bool save_seq16(uint16_t value) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) return false;
  prefs.putUInt(kKeySeq16, static_cast<uint32_t>(value));
  prefs.end();
  return true;
}

bool save_nodetable_snapshot(const uint8_t* data, size_t len) {
  if (!data || len > kMaxNodeTableSnapshotBytes) return false;
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) return false;
  prefs.putUInt(kKeyNodeTableSnapshotLen, static_cast<uint32_t>(len));
  prefs.putBytes(kKeyNodeTableSnapshot, data, len);
  prefs.end();
  return true;
}

bool load_nodetable_snapshot(uint8_t* out, size_t cap, size_t* out_len) {
  if (!out || !out_len || cap == 0) return false;
  *out_len = 0;
  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) return false;
  if (!prefs.isKey(kKeyNodeTableSnapshotLen)) {
    prefs.end();
    return false;
  }
  const size_t len = prefs.getUInt(kKeyNodeTableSnapshotLen, 0);
  prefs.end();
  if (len == 0 || len > cap) return false;
  Preferences prefs2;
  if (!prefs2.begin(kNamespace, true)) return false;
  const size_t read = prefs2.getBytes(kKeyNodeTableSnapshot, out, len);
  prefs2.end();
  if (read != len) return false;
  *out_len = len;
  return true;
}

}  // namespace naviga
