#include "platform/radio_factory.h"

#include "naviga/hal/radio_preset.h"
#include "platform/naviga_storage.h"

// E220 and E22 libraries both define ConfigurationMessage in global scope,
// so they MUST NOT be included in the same translation unit.
// This file includes exactly one, selected by HW_PROFILE build flag.
#if defined(HW_PROFILE_DEVKIT_E22_OLED_GNSS)
  #include "platform/e22_radio.h"
#else
  #include "platform/e220_radio.h"
#endif

namespace naviga {

// Build HAL preset from product-level FACTORY default for Phase A (boot_pipeline_v0).
// Phase A applies FACTORY default to hardware; no NVS read. Adapter maps product step to module.
static RadioPreset preset_from_factory_default() {
  RadioProfileRecord factory{};
  get_factory_default_radio_profile(&factory);
  // E220 T30D mapping: step 0→21, 1→24, 2→27, 3→30 dBm (e220_radio_profile_mapping_s03).
  uint8_t tx_dbm = 21;
  if (factory.tx_power_baseline_step > 0u && factory.tx_power_baseline_step <= 3u) {
    tx_dbm = 21u + factory.tx_power_baseline_step * 3u;
  } else if (factory.tx_power_baseline_step > 3u) {
    tx_dbm = 30u;  // T30D max; T33D would allow 33
  }
  return RadioPreset{
      /*.air_rate=*/factory.rate_tier,
      /*.channel=*/factory.channel_slot,
      /*.tx_power_dbm=*/tx_dbm,
  };
}

IRadio* create_radio(const HwProfile& profile, bool* radio_ready_out) {
  // Phase A: apply FACTORY default RadioProfile to hardware (provisioning_baseline_v0). No NVS.
  const RadioPreset preset = preset_from_factory_default();
#if defined(HW_PROFILE_DEVKIT_E22_OLED_GNSS)
  static E22Radio instance(profile.pins);
  *radio_ready_out = instance.begin(preset);
  return &instance;
#else
  static E220Radio instance(profile.pins);
  *radio_ready_out = instance.begin(preset);
  return &instance;
#endif
}

} // namespace naviga
