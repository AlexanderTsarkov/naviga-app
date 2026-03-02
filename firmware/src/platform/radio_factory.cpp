#include "platform/radio_factory.h"

#include "naviga/hal/radio_preset.h"

// E220 and E22 libraries both define ConfigurationMessage in global scope,
// so they MUST NOT be included in the same translation unit.
// This file includes exactly one, selected by HW_PROFILE build flag.
#if defined(HW_PROFILE_DEVKIT_E22_OLED_GNSS)
  #include "platform/e22_radio.h"
#else
  #include "platform/e220_radio.h"
#endif

namespace naviga {

IRadio* create_radio(const HwProfile& profile, bool* radio_ready_out) {
  // Default preset (airRate=2, channel=1). Future: derive from persisted radio profile id.
  const RadioPreset preset = get_radio_preset(RadioPresetId::Default);
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
