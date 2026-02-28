#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/node_table.h"
#include "../../protocol/geo_beacon_codec.h"
#include "../../protocol/alive_codec.h"
#include "../../protocol/tail1_codec.h"
#include "../../protocol/tail2_codec.h"
#include "../../protocol/info_codec.h"
#include "../../protocol/packet_header.h"

namespace naviga {
namespace domain {

/** Packet type for instrumentation logs. Dispatched from msg_type in the 2-byte frame header (#304). */
enum class PacketLogType {
  CORE,
  TAIL1,
  TAIL2,
  ALIVE,
  INFO,
};

/**
 * TX slot priority levels (lower value = higher priority).
 *
 * P0: Core_Pos / I_Am_Alive  — must send; drives liveness and position.
 * P1: Core_Tail              — best-effort but time-bound to Core sample.
 * P2: Operational / Info     — best-effort slow state; equal priority, fairness via replaced_count.
 */
enum class TxPriority : uint8_t {
  P0_HIGH  = 0,
  P1_MID   = 1,
  P2_LOW   = 2,
};

/**
 * One pending TX frame slot.
 *
 * Slots are keyed by packet type (one slot per type). Replacement increments
 * replaced_count and preserves created_at_ms so fairness accounting is correct.
 */
struct TxSlot {
  bool     present      = false;
  TxPriority priority   = TxPriority::P2_LOW;
  PacketLogType pkt_type = PacketLogType::CORE;
  uint32_t created_at_ms = 0;   ///< Set when slot first becomes present; preserved on replace.
  uint32_t replaced_count = 0;  ///< Incremented each time slot is replaced before being sent.
  uint16_t ref_core_seq16 = 0;  ///< For TAIL1 only: the Core_Pos seq16 this tail supplements.
  uint8_t  frame[protocol::kMaxFrameSize] = {};
  size_t   frame_len = 0;
};

/** Number of TX slot types. */
constexpr size_t kTxSlotCount = 5;

/** Index into the TX slot array for each packet type. */
constexpr size_t kSlotCore  = 0;  ///< Node_OOTB_Core_Pos (0x01)
constexpr size_t kSlotAlive = 1;  ///< Node_OOTB_I_Am_Alive (0x02)
constexpr size_t kSlotTail1 = 2;  ///< Node_OOTB_Core_Tail (0x03)
constexpr size_t kSlotTail2 = 3;  ///< Node_OOTB_Operational (0x04)
constexpr size_t kSlotInfo  = 4;  ///< Node_OOTB_Informative (0x05)

/**
 * Self-state fields used for Operational and Informative packet formation.
 * Populated by the runtime from device config / telemetry sources.
 */
struct SelfTelemetry {
  // Operational (0x04) fields
  bool    has_battery       = false;
  uint8_t battery_percent   = 0xFF;
  bool    has_uptime        = false;
  uint32_t uptime_sec       = 0xFFFFFFFFu;

  // Informative (0x05) fields
  bool     has_max_silence  = false;
  uint8_t  max_silence_10s  = 0;
  bool     has_hw_profile   = false;
  uint16_t hw_profile_id    = 0xFFFF;
  bool     has_fw_version   = false;
  uint16_t fw_version_id    = 0xFFFF;
};

class BeaconLogic {
 public:
  BeaconLogic();

  void set_min_interval_ms(uint32_t min_interval_ms);
  void set_max_silence_ms(uint32_t max_silence_ms);

  // ── Legacy single-packet TX API (preserved for existing tests) ──────────────

  /** \a allow_core_at_min_interval: when false and trigger is min_interval, do not send (NO_SEND).
   * Set true when position was just updated (SelfUpdatePolicy committed); per minDisplacement gating.
   * Alive is only used at maxSilence when no fix (field_cadence_v0). */
  bool build_tx(uint32_t now_ms,
                const protocol::GeoBeaconFields& self_fields,
                uint8_t* out,
                size_t out_cap,
                size_t* out_len,
                PacketLogType* out_type = nullptr,
                uint16_t* out_core_seq = nullptr,
                bool allow_core_at_min_interval = true);

  // ── Slot-based TX queue API (#316) ──────────────────────────────────────────

  /**
   * Formation pass: inspect self_fields and telemetry, form packets, and enqueue
   * them into the slot-based TX queue.
   *
   * Rules:
   * - Core_Pos: enqueued when pos_valid and (min_interval elapsed or max_silence hit).
   *   When Core_Pos is enqueued, Core_Tail is also enqueued immediately with
   *   ref_core_seq16 = core_seq16. If the Core slot is replaced (new position before
   *   old one was sent), the Tail slot is also replaced.
   * - Alive: enqueued when !pos_valid and max_silence would be violated.
   *   Alive and Core are mutually exclusive in the same formation pass.
   * - Operational (0x04): enqueued when telemetry.has_battery or telemetry.has_uptime.
   * - Informative (0x05): enqueued when any informative field is present.
   *
   * @param now_ms              Current time.
   * @param self_fields         Self position/identity fields.
   * @param telemetry           Self telemetry for Operational/Informative formation.
   * @param allow_core          Whether Core_Pos is allowed this tick (minDisplacement gate).
   */
  void update_tx_queue(uint32_t now_ms,
                       const protocol::GeoBeaconFields& self_fields,
                       const SelfTelemetry& telemetry,
                       bool allow_core);

  /**
   * Dequeue the highest-priority pending slot and copy its frame into out.
   *
   * Selection: highest priority first; within same priority, highest replaced_count
   * wins; tie-break by oldest created_at_ms.
   *
   * Clears the slot after copying (caller is responsible for sending).
   *
   * @param out       Output buffer; MUST have at least kMaxFrameSize bytes.
   * @param out_cap   Capacity of out.
   * @param out_len   Set to frame length on success, 0 if no pending slot.
   * @param out_type  If non-null, set to the packet type of the dequeued slot.
   * @param out_core_seq If non-null, set to ref_core_seq16 for TAIL1, else 0.
   * @return true if a frame was dequeued; false if queue is empty.
   */
  bool dequeue_tx(uint8_t* out,
                  size_t out_cap,
                  size_t* out_len,
                  PacketLogType* out_type = nullptr,
                  uint16_t* out_core_seq = nullptr);

  /** Returns true if any TX slot is present (queue non-empty). */
  bool has_pending_tx() const;

  /** Direct read-only access to a slot for testing. */
  const TxSlot& slot(size_t slot_index) const { return slots_[slot_index]; }

  // ── RX dispatch ─────────────────────────────────────────────────────────────

  bool on_rx(uint32_t now_ms,
             const uint8_t* payload,
             size_t len,
             int8_t rssi_dbm,
             NodeTable& table,
             uint64_t* out_node_id = nullptr,
             uint16_t* out_seq = nullptr,
             bool* out_pos_valid = nullptr,
             PacketLogType* out_type = nullptr,
             uint16_t* out_core_seq = nullptr);

  uint16_t seq() const {
    return seq_;
  }

 private:
  uint32_t min_interval_ms_ = 5000;
  uint32_t max_silence_ms_ = 30000;
  uint32_t last_tx_ms_ = 0;
  uint16_t seq_ = 0;

  // Slot-based TX queue.
  TxSlot slots_[kTxSlotCount] = {};

  // Allocate the next global seq16 and advance the counter.
  uint16_t next_seq16();

  // Enqueue or replace a slot. If slot was already present, increments replaced_count
  // and preserves created_at_ms; otherwise sets created_at_ms = now_ms.
  void enqueue_slot(size_t slot_idx,
                    TxPriority priority,
                    PacketLogType pkt_type,
                    const uint8_t* frame,
                    size_t frame_len,
                    uint32_t now_ms,
                    uint16_t ref_core_seq16 = 0);
};

} // namespace domain
} // namespace naviga
