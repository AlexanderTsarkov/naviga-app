#include "platform/naviga_storage.h"

#include <Preferences.h>

namespace naviga {

namespace {

constexpr char kNamespace[] = "naviga";
constexpr char kKeyCurrentRole[] = "role_cur";
constexpr char kKeyCurrentRadio[] = "prof_cur";
constexpr char kKeyPreviousRole[] = "role_prev";
constexpr char kKeyPreviousRadio[] = "prof_prev";

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

bool factory_reset_pointers() {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }

  prefs.remove(kKeyCurrentRole);
  prefs.remove(kKeyCurrentRadio);
  prefs.remove(kKeyPreviousRole);
  prefs.remove(kKeyPreviousRadio);

  prefs.end();
  return true;
}

}  // namespace naviga
