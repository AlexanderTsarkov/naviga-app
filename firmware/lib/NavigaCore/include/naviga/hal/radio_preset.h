#pragma once

#include <cstdint>

namespace naviga {

// ── RadioPreset ───────────────────────────────────────────────────────────────
// Represents a named radio configuration for UART EByte modules (E220/E22).
//
// airRate encoding matches the EByte SPED register bits [2:0]:
//   0 = 0.3 kbps, 1 = 1.2 kbps, 2 = 2.4 kbps, 3 = 4.8 kbps,
//   4 = 9.6 kbps, 5 = 19.2 kbps, 6 = 38.4 kbps, 7 = 62.5 kbps
//
// NOTE: E22-400T30D V2 firmware clamps airRate < 2 to 2 (2.4 kbps).
// Proven by controlled harness (issue #336). normalize_air_rate() enforces this.
//
// txPower is advisory — not part of the UART config frame on these modules;
// stored here for future SPI-module support and documentation purposes.
struct RadioPreset {
  uint8_t air_rate;    // SPED bits [2:0]; see encoding above
  uint8_t channel;     // CHAN register value (e.g. 1 = 411 MHz for E22-400T30D)
  uint8_t tx_power_dbm; // advisory; not applied via UART config frame
};

// ── Preset IDs ────────────────────────────────────────────────────────────────
// Code-level constants for future product UI/command selection.
// Only preset 0 (Default) is used as OOTB v1-A baseline.
// Presets 1/2 are defined here so the apply path can be exercised in tests
// and future firmware without adding new user-selection mechanisms now.
enum class RadioPresetId : uint8_t {
  Default   = 0,  // 2.4 kbps  / SF9-equivalent UART baseline
  Fast      = 1,  // 4.8 kbps  / urban / short-range
  LongRange = 2,  // 2.4 kbps  / same as Default for UART; SPI will use SF10
};

// ── Normalization ─────────────────────────────────────────────────────────────

// Minimum airRate enforced by E22-400T30D V2 module firmware (issue #336).
// E220 also uses 2.4 kbps as its lowest supported rate.
constexpr uint8_t kMinAirRate = 2;

// Returns true if air_rate was clamped (i.e. input < kMinAirRate).
inline bool normalize_air_rate(uint8_t requested, uint8_t* out) {
  if (requested < kMinAirRate) {
    *out = kMinAirRate;
    return true;  // clamped
  }
  *out = requested;
  return false;
}

// ── Preset table ─────────────────────────────────────────────────────────────
// Returns the canonical preset for the given ID.
// channel=1 → 411 MHz for E22-400T30D (base 410 + 1).
inline RadioPreset get_radio_preset(RadioPresetId id) {
  switch (id) {
    case RadioPresetId::Fast:
      return RadioPreset{/*.air_rate=*/3, /*.channel=*/1, /*.tx_power_dbm=*/30};
    case RadioPresetId::LongRange:
      return RadioPreset{/*.air_rate=*/2, /*.channel=*/1, /*.tx_power_dbm=*/30};
    case RadioPresetId::Default:
    default:
      return RadioPreset{/*.air_rate=*/2, /*.channel=*/1, /*.tx_power_dbm=*/30};
  }
}

// ── Readback verification ─────────────────────────────────────────────────────

// Result of comparing a normalized preset against what the module reported back.
struct RadioPresetVerifyResult {
  bool air_rate_ok;
  bool channel_ok;
  bool all_ok() const { return air_rate_ok && channel_ok; }
};

// Pure function: compare normalized preset against readback values.
// readback_air_rate / readback_channel come from getConfiguration() after write.
inline RadioPresetVerifyResult verify_preset_readback(
    const RadioPreset& normalized,
    uint8_t readback_air_rate,
    uint8_t readback_channel) {
  return RadioPresetVerifyResult{
    /*.air_rate_ok=*/ readback_air_rate == normalized.air_rate,
    /*.channel_ok=*/  readback_channel  == normalized.channel,
  };
}

} // namespace naviga
