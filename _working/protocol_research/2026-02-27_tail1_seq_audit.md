# Tail-1 Seq16 Audit: Common seq16 vs ref_core_seq16
**Date:** 2026-02-27  
**Iteration:** S02__2026-03__docs_promotion_and_arch_audit  
**Issues:** #283 (Tail-1 TX scheduling, blocked), #284 (Tail-2 TX scheduling)  
**Gate:** #277  
**Scope:** Clarify Tail-1 payload structure; canonize "Common prefix vs Functional payload" split; determine whether Tail-1 needs its own per-frame seq16.

---

## 1. The Ambiguity Being Resolved

The task asks: does Tail-1 carry its own per-frame `seq16` (for duplicate/OOO/wrap handling), or only a `core_seq16` (reference to the Core it qualifies)?

The concern is that `rx_semantics_v0.md §1` states:

> **seq16 scope:** seq16 is a **single per-node counter** across all transmitted packet types during uptime (BeaconCore, Tail-1/2, Alive). Receivers **MUST** treat seq16 ordering and deduplication across packet types accordingly. The seen seq16 set (or ordering window) is **per-node global**, not per packet type.

This reads as if Tail-1 carries `seq16`. But the Tail-1 byte layout (`tail1_packet_encoding_v0.md §3`) has no `seq16` field — only `core_seq16`.

---

## 2. Current Reality: Code

### 2.1 Tail-1 payload layout in code

**File:** `firmware/protocol/tail1_codec.h`

```
[0]      payloadVersion = 0x00
[1..6]   nodeId48 LE
[7..8]   core_seq16 LE   ← ONLY seq-like field; this is the Core reference
[9]      posFlags  (optional)
[10]     sats      (optional)
```

`Tail1Fields` struct:
```cpp
struct Tail1Fields {
  uint64_t node_id      = 0;
  uint16_t core_seq16   = 0;   // seq16 of the Core sample this Tail-1 qualifies
  bool     has_pos_flags = false;
  uint8_t  pos_flags    = 0x00;
  bool     has_sats     = false;
  uint8_t  sats         = 0x00;
};
```

**There is no `seq16` field in `Tail1Fields`.** The only seq-like field is `core_seq16`.

### 2.2 NodeTable apply logic

**File:** `firmware/src/domain/node_table.cpp`, `apply_tail1()`

```cpp
bool NodeTable::apply_tail1(uint64_t node_id,
                            uint16_t core_seq16, ...) {
  // ...
  if (!entry.has_core_seq16 || entry.last_core_seq16 != core_seq16) {
    // core_seq16 mismatch — drop silently.
    // Still update link metrics per §4.4.
    entry.last_seen_ms = now_ms;
    entry.last_rx_rssi = rssi_dbm;
    return false;
  }
  // Match: apply posFlags, sats. MUST NOT touch position.
  // ...
}
```

The gating is entirely on `core_seq16 == last_core_seq16[N]`. There is no per-Tail-1 seq16 check, no duplicate detection on Tail-1 itself.

### 2.3 BeaconCore comparison

**File:** `firmware/protocol/geo_beacon_codec.h` (BeaconCore)

```
[0]      payloadVersion
[1..6]   nodeId48 LE
[7..8]   seq16 LE        ← per-frame freshness counter
[9..11]  lat_u24 LE
[12..14] lon_u24 LE
```

BeaconCore carries `seq16` as a per-frame freshness counter. This is what `last_core_seq16` in NodeTable tracks.

### 2.4 BeaconAlive comparison

**File:** `firmware/protocol/alive_codec.h` (BeaconAlive)

```
[0]      payloadVersion
[1..6]   nodeId48 LE
[7..8]   seq16 LE        ← same per-node counter as Core
[9]      aliveStatus (optional)
```

`alive_packet_encoding_v0.md §3.1` explicitly states: "Alive uses the **same per-node seq16 counter** as BeaconCore and Tails (single counter across packet types during uptime)."

---

## 3. Current Reality: Canon Docs

### 3.1 Tail-1 byte layout (canon)

**File:** `docs/product/areas/nodetable/contract/tail1_packet_encoding_v0.md §3`

| Offset | Field | Type | Bytes |
|--------|-------|------|-------|
| 0 | `payloadVersion` | uint8 | 1 |
| 1 | `nodeId` | uint48 LE | 6 |
| 7 | `core_seq16` | uint16 LE | 2 |
| 9 | `posFlags` (optional) | uint8 | 1 |
| 10 | `sats` (optional) | uint8 | 1 |

No `seq16` field. The field at offset 7 is named `core_seq16` and its semantics are:

> `seq16` of the BeaconCore sample this Tail-1 qualifies. MUST match `last_core_seq16` for this node on RX.

### 3.2 The rx_semantics_v0.md §1 statement

**File:** `docs/product/areas/nodetable/policy/rx_semantics_v0.md §1`

> **seq16 scope:** seq16 is a **single per-node counter** across all transmitted packet types during uptime (BeaconCore, Tail-1/2, Alive). Receivers **MUST** treat seq16 ordering and deduplication across packet types accordingly.

This statement describes the **sender's counter** — the same counter increments for every packet the sender transmits (Core, Alive, and Tails when implemented). It does **not** say every packet carries `seq16` in its payload. It says the counter is shared.

**However:** Tail-1 does NOT carry this counter in its payload. The `core_seq16` field in Tail-1 is not the sender's current counter value — it is a **back-reference** to a previously transmitted Core's seq16.

### 3.3 The "Duplicate" definition in rx_semantics_v0.md §1

> **Duplicate:** A packet that carries the same logical sample as one already applied (e.g. same nodeId and seq16 for Core or Alive).

The parenthetical "e.g. same nodeId and seq16 for Core or Alive" notably does **not** include Tail-1. This is not an oversight — Tail-1 duplicate detection is handled differently (by the `core_seq16 == lastCoreSeq` gate, not by a per-Tail-1 seq16).

### 3.4 beacon_payload_encoding_v0.md §3 (summary table)

The summary table lists Tail-1 minimum fields as:
> `payloadVersion(1), nodeId48(6), core_seq16(2)` = **9 bytes**

And the note:
> Receiver applies only if **core_seq16 == lastCoreSeq**; else ignore.

No mention of a per-Tail-1 seq16.

---

## 4. Definitive Field Mapping

| Concept | BeaconCore | BeaconAlive | BeaconTail-1 | BeaconTail-2 |
|---------|-----------|-------------|--------------|--------------|
| Per-frame freshness counter (seq16) | ✓ offset 7 | ✓ offset 7 | **✗ absent** | **✗ absent** |
| Core reference (core_seq16) | n/a | n/a | ✓ offset 7 | **✗ absent** |
| Duplicate detection | seq16 wrap rule | seq16 wrap rule | core_seq16 gate | none (apply unconditionally) |
| OOO detection | seq16 wrap rule | seq16 wrap rule | core_seq16 gate | none |

**Key result:** Tail-1 has NO per-frame seq16. Its only seq-like field is `core_seq16`, which is a back-reference to the Core it qualifies. Duplicate/OOO detection for Tail-1 is handled entirely by the `core_seq16 == lastCoreSeq` gate — if two Tail-1 packets arrive for the same Core, both will be applied (idempotent, since they carry the same posFlags/sats for the same Core sample). This is intentional and correct: Tail-1 is loss-tolerant and applying it twice is harmless.

---

## 5. Ambiguity Analysis

### 5.1 Is there a real ambiguity?

**In the code:** No ambiguity. The codec and NodeTable are consistent and correct.

**In the docs:** There is a **terminology ambiguity** in `rx_semantics_v0.md §1`:

> seq16 is a **single per-node counter** across all transmitted packet types during uptime (BeaconCore, Tail-1/2, Alive).

This sentence is true for the **sender's counter** but misleading because it implies Tail-1 carries `seq16` in its payload. A reader could interpret this as "every packet type carries seq16" — which is false for Tail-1 and Tail-2.

The correct interpretation is:
- The **sender** increments one counter per transmitted packet, regardless of type.
- **BeaconCore** and **BeaconAlive** embed this counter as `seq16` in their payload.
- **BeaconTail-1** does NOT embed the current counter value; it embeds `core_seq16` (a back-reference to a prior Core's seq16).
- **BeaconTail-2** does NOT embed any seq-like field.

### 5.2 Does Tail-1 NEED a per-frame seq16?

**No.** The design is intentional:

1. **Tail-1 is sample-attached.** It qualifies exactly one Core sample. Its identity is `(nodeId, core_seq16)`. Two Tail-1 packets with the same `(nodeId, core_seq16)` carry the same logical content — applying both is idempotent.

2. **Duplicate Tail-1 is harmless.** If a Tail-1 is received twice (e.g. retransmit or relay), both applications write the same posFlags/sats to the same Core sample. No corruption.

3. **OOO Tail-1 is handled by the core_seq16 gate.** A stale Tail-1 (referencing an old Core) is dropped because `core_seq16 != lastCoreSeq`. A future Tail-1 (referencing a Core not yet received) is also dropped (no `last_core_seq16` to match). The gate is sufficient.

4. **Adding seq16 to Tail-1 would require a payloadVersion bump** and would add 2 bytes to every Tail-1 frame (9 → 11 bytes minimum) for no functional benefit.

**Conclusion: Tail-1 MUST NOT add a per-frame seq16. The current design is correct.**

### 5.3 The `rx_semantics_v0.md §1` sentence needs clarification

The sentence "seq16 is a single per-node counter across all transmitted packet types" should be clarified to distinguish:
- **Sender counter scope** (shared across all TX packet types)
- **Payload presence** (only Core and Alive embed it; Tail-1/2 do not)

---

## 6. Terminology: "Common Prefix" vs "Functional Payload"

The task asks to introduce an explicit structural split for beacon-family payloads.

### 6.1 Current implicit structure

Every beacon-family payload begins with the same 7-byte prefix:

```
[0]      payloadVersion  (1 byte)
[1..6]   nodeId48 LE     (6 bytes)
```

This is present in BeaconCore, BeaconAlive, BeaconTail-1, and BeaconTail-2. It is the minimum required to dispatch and identify the sender.

After this prefix, each packet type has its own functional payload:

| Packet | Functional payload (after prefix) |
|--------|----------------------------------|
| BeaconCore | `seq16(2) | lat_u24(3) | lon_u24(3)` |
| BeaconAlive | `seq16(2) [| aliveStatus(1)]` |
| BeaconTail-1 | `core_seq16(2) [| posFlags(1) | sats(1)]` |
| BeaconTail-2 | `[maxSilence10s(1) | batteryPercent(1) | hwProfileId(2) | fwVersionId(2) | uptimeSec(4)]` |

### 6.2 Proposed terminology

- **Common prefix** (mandatory, all beacon-family packets): `payloadVersion(1) | nodeId48(6)` = 7 bytes minimum.
- **Functional payload** (type-specific, follows common prefix): everything after byte 6.

Within the functional payload, BeaconCore and BeaconAlive have a **per-frame seq16** as the first functional field. BeaconTail-1 has **ref_core_seq16** (renamed from `core_seq16` to make the "reference" semantics explicit). BeaconTail-2 has no seq-like field.

### 6.3 Field rename: core_seq16 → ref_core_seq16

The field currently named `core_seq16` in Tail-1 docs and code should be renamed to `ref_core_seq16` to:
1. Make explicit that it is a **reference** to another packet's seq16, not a freshness counter.
2. Prevent confusion with the common `seq16` field in Core/Alive.
3. Align with the task's requirement.

**Impact of rename:**
- `tail1_packet_encoding_v0.md`: rename field in §3 table and all references.
- `beacon_payload_encoding_v0.md`: rename in §3 summary table and §4.2 summary.
- `firmware/protocol/tail1_codec.h`: rename `core_seq16` → `ref_core_seq16` in `Tail1Fields` struct and all usages.
- `firmware/src/domain/node_table.h/.cpp`: rename parameter `core_seq16` → `ref_core_seq16` in `apply_tail1`.
- `firmware/src/domain/beacon_logic.cpp`: rename usages.
- Tests: rename in `test_beacon_logic.cpp`.
- `rx_semantics_v0.md`, `field_cadence_v0.md`, other policy docs: update references.
- `nodetable_fields_inventory_v0.md`: update.
- The NodeTable stored field `last_core_seq16` / `has_core_seq16` in `node_table.h` should become `last_ref_core_seq16` / `has_ref_core_seq16` (or keep as `last_core_seq16` for brevity — see §7 below).

---

## 7. Recommendations

### 7.1 Doc changes (required before #283 implementation)

**Priority 1 — Clarify rx_semantics_v0.md §1:**

The sentence "seq16 is a single per-node counter across all transmitted packet types" must be split into two statements:
- Sender counter scope: "The sender maintains one seq16 counter per node, incremented on every transmitted packet (Core, Alive, and Tails when implemented)."
- Payload presence: "BeaconCore and BeaconAlive embed the current counter value as `seq16` in their payload. BeaconTail-1 embeds `ref_core_seq16` (a back-reference to the Core it qualifies), not the current counter. BeaconTail-2 carries no seq-like field."

**Priority 2 — Introduce "Common prefix / Functional payload" split:**

Add a §0.x or §2.x to `beacon_payload_encoding_v0.md` (and `tail1_packet_encoding_v0.md`) defining:
- **Common prefix:** `payloadVersion(1) | nodeId48(6)` — mandatory, all beacon-family packets.
- **Functional payload:** type-specific fields after the common prefix.

**Priority 3 — Rename `core_seq16` → `ref_core_seq16` in docs:**

Update `tail1_packet_encoding_v0.md §3`, `beacon_payload_encoding_v0.md §3/§4.2`, and all policy doc references.

### 7.2 Code changes (required for alignment)

The rename `core_seq16` → `ref_core_seq16` in the Tail-1 codec and NodeTable is a **mechanical rename** with no behavioral change. It is a payloadVersion-neutral change (wire format unchanged; only in-memory struct field names change).

Files to update:
- `firmware/protocol/tail1_codec.h`: `Tail1Fields::core_seq16` → `ref_core_seq16`; update encode/decode comments.
- `firmware/src/domain/node_table.h`: parameter `core_seq16` in `apply_tail1` → `ref_core_seq16`; stored field `last_core_seq16` → `last_ref_core_seq16` (optional — see note).
- `firmware/src/domain/node_table.cpp`: update `apply_tail1` parameter and stored field.
- `firmware/src/domain/beacon_logic.cpp`: update usages.
- `firmware/test/test_beacon_logic/test_beacon_logic.cpp`: update field accesses.

**Note on stored field name:** `NodeEntry::last_core_seq16` is an internal NodeTable field (not on-wire). Renaming it to `last_ref_core_seq16` is more precise but adds churn. Acceptable to keep as `last_core_seq16` in the struct if the comment is updated to say "stores the seq16 of the last accepted Core, used to gate Tail-1 (ref_core_seq16 match)."

### 7.3 No payloadVersion bump needed

The wire format is unchanged. Only in-memory field names and doc terminology change. `payloadVersion` remains `0x00` for Tail-1.

### 7.4 Tail-2 terminology

Tail-2 has no seq-like field at all. The "Common prefix" concept applies (same `payloadVersion | nodeId48` prefix), but no rename is needed. Mention in the "Common prefix" section that Tail-2 has no seq-like field in its functional payload.

---

## 8. Summary: What Was Ambiguous and What Is Now Clear

| Question | Answer |
|----------|--------|
| Does Tail-1 carry a per-frame seq16? | **No.** Tail-1 has no per-frame seq16 in its payload. |
| What is the only seq-like field in Tail-1? | `core_seq16` (to be renamed `ref_core_seq16`) — a back-reference to the Core it qualifies. |
| Is duplicate/OOO detection on Tail-1 handled? | **Yes, by design.** The `core_seq16 == lastCoreSeq` gate handles it. Duplicate Tail-1 for the same Core is idempotent. |
| Does Tail-1 need a per-frame seq16 added? | **No.** Adding it would waste 2 bytes and require a payloadVersion bump for no functional benefit. |
| What is the ambiguity in the docs? | `rx_semantics_v0.md §1` conflates "sender counter scope" with "payload presence." The sentence needs to be split. |
| What terminology changes are needed? | (1) Rename `core_seq16` → `ref_core_seq16` in Tail-1 docs and code. (2) Introduce "Common prefix / Functional payload" split in beacon encoding docs. (3) Clarify rx_semantics_v0.md §1. |
| Is the current code correct? | **Yes.** Code is consistent with the correct interpretation of canon. No behavioral changes needed. |
| Is a payloadVersion bump needed? | **No.** Wire format is unchanged. |

---

## 9. Doc References

| Doc | Relevant section | Finding |
|-----|-----------------|---------|
| `tail1_packet_encoding_v0.md` | §3 byte layout | `core_seq16` at offset 7 — only seq-like field; rename to `ref_core_seq16` |
| `beacon_payload_encoding_v0.md` | §3 summary table, §4.2 | Same rename; add "Common prefix" concept |
| `alive_packet_encoding_v0.md` | §3.1 | Explicitly states Alive uses same per-node seq16 counter as Core |
| `rx_semantics_v0.md` | §1 definitions | "seq16 scope" sentence conflates sender counter with payload presence; needs split |
| `rx_semantics_v0.md` | §2 | Tail-1 apply rule correctly references `core_seq16`; update name |
| `field_cadence_v0.md` | §2, §3 | References `core_seq16`; update name |
| `nodetable_fields_inventory_v0.md` | rows for posFlags, sats | References `core_seq16`; update name |

## 10. Code References

| File | Symbol | Finding |
|------|--------|---------|
| `firmware/protocol/tail1_codec.h` | `Tail1Fields::core_seq16` | Rename to `ref_core_seq16`; no wire change |
| `firmware/protocol/tail1_codec.h` | `encode_tail1_frame`, `decode_tail1_payload` | Update field access after rename |
| `firmware/src/domain/node_table.h` | `NodeEntry::last_core_seq16`, `has_core_seq16` | Keep or rename; update comment |
| `firmware/src/domain/node_table.h` | `apply_tail1(... core_seq16 ...)` | Rename parameter to `ref_core_seq16` |
| `firmware/src/domain/node_table.cpp` | `apply_tail1` body | Update parameter and stored field |
| `firmware/src/domain/beacon_logic.cpp` | `on_rx` Tail-1 branch | Update field access: `tail1.core_seq16` → `tail1.ref_core_seq16` |
| `firmware/test/test_beacon_logic/test_beacon_logic.cpp` | `test_tail1_*` | Update field accesses |
