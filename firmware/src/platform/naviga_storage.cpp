#include "platform/naviga_storage.h"

#include <Preferences.h>

namespace naviga {

namespace {

constexpr char kNamespace[] = "naviga";
constexpr char kKeyCurrentRole[] = "role_cur";
constexpr char kKeyCurrentRadio[] = "prof_cur";
constexpr char kKeyPreviousRole[] = "role_prev";
constexpr char kKeyPreviousRadio[] = "prof_prev";
constexpr char kKeyProfileIntervalSec[] = "role_interval_s";
constexpr char kKeyProfileSilence10s[] = "role_silence_10s";
constexpr char kKeyProfileDistM[] = "role_dist_m";

constexpr uint16_t kDefaultMinIntervalSec = 18;
constexpr uint8_t kDefaultMaxSilence10s = 9;
constexpr float kDefaultMinDisplacementM = 25.0f;

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

  RoleProfileRecord def;
  get_ootb_role_profile(0, &def);
  prefs.putUInt(kKeyProfileIntervalSec, def.min_interval_sec);
  prefs.putUInt(kKeyProfileSilence10s, def.max_silence_10s);
  prefs.putFloat(kKeyProfileDistM, def.min_displacement_m);

  prefs.end();
  return true;
}

}  // namespace naviga
