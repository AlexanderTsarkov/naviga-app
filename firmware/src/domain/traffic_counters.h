#pragma once

#include <cstdint>

namespace naviga {
namespace domain {

/**
 * Debug counters for traffic validation (#425).
 * Answers: what packet types are enqueued/sent/dropped, coalesce/replace/starve, RX accept/reject.
 * No protocol or profile semantics; read via M1Runtime::traffic_counters().
 */
struct TrafficCounters {
  // TX formation (BeaconLogic)
  uint32_t tx_enqueue_pos_full = 0;
  uint32_t tx_enqueue_alive    = 0;
  uint32_t tx_enqueue_status   = 0;
  uint32_t tx_slot_replaced    = 0;  ///< Enqueue replaced an existing slot (coalesce).
  uint32_t tx_starved          = 0;  ///< Dequeue chose one slot; others got starvation increment.

  // TX outcome (M1Runtime: after send attempt)
  uint32_t tx_sent_pos_full = 0;
  uint32_t tx_sent_alive    = 0;
  uint32_t tx_sent_status   = 0;
  uint32_t tx_drop_channel_busy = 0;
  uint32_t tx_drop_send_fail   = 0;

  // RX (M1Runtime: after on_rx)
  uint32_t rx_ok_pos_full = 0;
  uint32_t rx_ok_alive    = 0;
  uint32_t rx_ok_status   = 0;
  uint32_t rx_reject      = 0;  ///< Decode or validate failed (unknown type / bad payload).
};

}  // namespace domain
}  // namespace naviga
