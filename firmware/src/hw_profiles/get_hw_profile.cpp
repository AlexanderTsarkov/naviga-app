#include "hw_profile.h"

namespace naviga {

extern const HwProfile kDevkitE220OledProfile;
extern const HwProfile kDevkitE220OledGnssProfile;

const HwProfile& get_hw_profile() {
#if defined(HW_PROFILE_DEVKIT_E220_OLED)
  return kDevkitE220OledProfile;
#elif defined(HW_PROFILE_DEVKIT_E220_OLED_GNSS)
  return kDevkitE220OledGnssProfile;
#else
#error "Select HW profile via PlatformIO env (HW_PROFILE_DEVKIT_E220_OLED or HW_PROFILE_DEVKIT_E220_OLED_GNSS)"
#endif
}

} // namespace naviga
