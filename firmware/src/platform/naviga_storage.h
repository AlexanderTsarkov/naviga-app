#pragma once

#include <cstddef>
#include <cstdint>

namespace naviga {

// ── Radio profile (product-level, #382) ─────────────────────────────────────
// Schema and semantics: docs/product/wip/areas/radio/policy/radio_profiles_model_s03.md

/** Schema version for radio profile NVS keys (rprof_ver). */
constexpr uint8_t kRadioProfileSchemaVersion = 1;

/** Profile id 0 = FACTORY default (virtual, not stored in NVS). */
constexpr uint32_t kRadioProfileIdFactoryDefault = 0;

enum class RadioProfileKind : uint8_t {
  FACTORY = 0,
  USER = 1,
};

/**
 * Product-level radio profile record (baseline only; runtime does not mutate this).
 * Maps to NVS when kind == USER; FACTORY (id 0) is const in FW.
 */
struct RadioProfileRecord {
  uint32_t profile_id = 0;
  RadioProfileKind kind = RadioProfileKind::FACTORY;
  uint8_t channel_slot = 1;
  uint8_t rate_tier = 2;   // product step; adapter maps to air_rate / SF-BW
  uint8_t tx_power_baseline_step = 0;  // product step 0 = MIN (21 dBm); OOTB uses MIN; adapter maps to module levels
  static constexpr size_t kMaxLabelLen = 24;
  char label[kMaxLabelLen] = {0};
};

/** Fill record with FACTORY default (id 0). Used in Phase A when no NVS profile is needed for OOTB. */
void get_factory_default_radio_profile(RadioProfileRecord* out);

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

// ── Seq16 persistence (#417) ─────────────────────────────────────────────────
// Canon: rx_semantics_v0 §5.3 — restore on boot; first TX after reboot uses restored + 1.

/**
 * Load persisted seq16 from NVS (namespace "naviga", key "seq16").
 * Returns true if key exists and value was read; *out is set. If key missing, *out unchanged, returns false.
 */
bool load_seq16(uint16_t* out);

/**
 * Save seq16 to NVS. Per-TX write policy (#417): persist after each successful send.
 * Returns true on success.
 */
bool save_seq16(uint16_t value);

}  // namespace naviga
