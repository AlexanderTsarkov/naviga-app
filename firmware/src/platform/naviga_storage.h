#pragma once

#include <cstdint>

namespace naviga {

/**
 * Persisted pointers for Phase B (boot_pipeline_v0) and provisioning (provisioning_interface_v0).
 * CurrentRoleId / CurrentRadioProfileId / PreviousRoleId / PreviousRadioProfileId.
 * Opaque uint32 ids; F5/F6 resolve to role/profile content.
 */
struct PersistedPointers {
  uint32_t current_role_id = 0;
  uint32_t current_radio_profile_id = 0;
  uint32_t previous_role_id = 0;
  uint32_t previous_radio_profile_id = 0;
  bool has_current_role = false;
  bool has_current_radio = false;
  bool has_previous_role = false;
  bool has_previous_radio = false;
};

/**
 * Current role profile record (cadence params) stored in flash (#289).
 * Resolved via persisted "current" record; fallback to default when missing/invalid.
 */
struct RoleProfileRecord {
  uint16_t min_interval_sec = 18;
  uint8_t max_silence_10s = 9;
  float min_displacement_m = 25.0f;
};

/**
 * Load the current role profile record from NVS.
 * If missing or invalid, out is filled with default (Person) and *valid = false.
 * Returns true if NVS was read; valid is set to true only when stored values pass validation.
 */
bool load_current_role_profile_record(RoleProfileRecord* out, bool* valid);

/**
 * Save the current role profile record to NVS.
 * Caller should ensure values satisfy role_profiles_policy_v0 (e.g. maxSilence >= 3*minInterval).
 * Returns true on success.
 */
bool save_current_role_profile_record(const RoleProfileRecord& record);

/**
 * Fill record with OOTB defaults for the given role id (0=Person, 1=Dog, 2=Infra).
 * Used when "set role X" persists the profile and when falling back to default.
 */
void get_ootb_role_profile(uint32_t role_id, RoleProfileRecord* out);

/**
 * Load persisted pointers from NVS (namespace "naviga").
 * Returns true if NVS was opened and read; out is filled (missing keys leave has_* false).
 */
bool load_pointers(PersistedPointers* out);

/**
 * Save pointers to NVS. Immediate persistence per provisioning_interface_v0.
 * Returns true on success.
 */
bool save_pointers(uint32_t current_role_id,
                   uint32_t current_radio_profile_id,
                   uint32_t previous_role_id,
                   uint32_t previous_radio_profile_id);

/**
 * Clear all role/radio pointer keys and reset current role profile record to default (Person).
 * Returns true on success.
 */
bool factory_reset_pointers();

}  // namespace naviga
