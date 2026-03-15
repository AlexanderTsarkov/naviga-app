#pragma once

#include <cstddef>
#include <cstdint>

#include "naviga/hal/interfaces.h"

namespace naviga {
namespace protocol {

/**
 * S04 #467: BLE profiles list/read bridge.
 * Fills transport with profiles list (radio [0], user/role [0,1,2]) and profile read response.
 * Uses naviga_storage + role_profile_ootb; no domain changes.
 * First-phase encoding documented in .cpp.
 */
class BleProfilesBridge {
 public:
  /** Update profiles list buffer: n_radio(1), radio_ids(4×n), n_user(1), user_ids(4×n). LE. */
  void update_profiles_list(IBleTransport& transport) const;

  /** Consume profile read request (type + id); fill response (one profile); clear request. */
  void update_profile_read(IBleTransport& transport) const;
};

}  // namespace protocol
}  // namespace naviga
