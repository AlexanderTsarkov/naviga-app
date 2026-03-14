#include "platform/naviga_storage.h"

namespace naviga {

void get_ootb_role_profile(uint32_t role_id, RoleProfileRecord* out) {
  if (!out) return;
  switch (role_id) {
    case 0:  // Person (Human): 22s / 30m / 110s (#453)
      out->min_interval_sec = 22;
      out->max_silence_10s = 11;
      out->min_displacement_m = 30.0f;
      break;
    case 1:  // Dog: 11s / 15m / 50s (#453)
      out->min_interval_sec = 11;
      out->max_silence_10s = 5;
      out->min_displacement_m = 15.0f;
      break;
    case 2:
      out->min_interval_sec = 360;
      out->max_silence_10s = 255;
      out->min_displacement_m = 100.0f;
      break;
    default:
      out->min_interval_sec = 22;
      out->max_silence_10s = 11;
      out->min_displacement_m = 30.0f;
      break;
  }
}

}  // namespace naviga
