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
 * Clear all role/radio pointer keys (factory reset of pointers only).
 * Returns true on success.
 */
bool factory_reset_pointers();

}  // namespace naviga
