#include "platform/naviga_storage.h"

namespace naviga {

void get_ootb_role_profile(uint32_t role_id, RoleProfileRecord* out) {
  if (!out) return;
  switch (role_id) {
    case 0:
      out->min_interval_sec = 18;
      out->max_silence_10s = 9;
      out->min_displacement_m = 25.0f;
      break;
    case 1:
      out->min_interval_sec = 9;
      out->max_silence_10s = 3;
      out->min_displacement_m = 15.0f;
      break;
    case 2:
      out->min_interval_sec = 360;
      out->max_silence_10s = 255;
      out->min_displacement_m = 100.0f;
      break;
    default:
      out->min_interval_sec = 18;
      out->max_silence_10s = 9;
      out->min_displacement_m = 25.0f;
      break;
  }
}

}  // namespace naviga
