#pragma once

#include "hw_profile.h"
#include "naviga/hal/interfaces.h"

namespace naviga {

// Creates the appropriate IRadio instance for the given HW profile.
// Returns pointer to a static instance (valid for program lifetime).
// Calls begin() internally; radio_ready_out receives the result.
IRadio* create_radio(const HwProfile& profile, bool* radio_ready_out);

} // namespace naviga
