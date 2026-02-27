# Position Quality (v0) — Policy

**Status:** Canon (policy).  
**Scope:** NodeTable v0; Tail-1 position-quality fields and derived UI state.  
**Related:** [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md), [nodetable_fields_inventory_v0](nodetable_fields_inventory_v0.md), [index §6](../index.md#6-position-v0).

---

## 1. Sent position-quality fields (v0)

In v0, the **only** position-quality fields sent on the wire are in **Tail-1** (attached to Core sample):

| Field     | Location | Meaning (v0) |
|----------|----------|----------------|
| **posFlags** | Tail-1 optional | Position-valid / quality flags; 0 = position not present or no fix. |
| **sats**     | Tail-1 optional | Satellite count when position is valid; 0 = not present. |

- **Source:** [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) §4.2 Tail-1 optional block.
- Tail-1 is **attached to Core sample** (same `ref_core_seq16`); position quality is therefore sample-scoped.

No other position-quality metrics (e.g. fixType, hdop, accuracy, age) are encoded in the beacon in v0. They may be introduced in a later tier/cadence or Tail-2 if needed.

---

## 2. Derived PositionQuality (UI / consumer)

**PositionQuality** is a **derived** notion for UI and consumers: it is not a separate beacon field.

- **Inputs (v0):**
  - **Core:** position present (valid lat/lon) or sentinel (position not present).
  - **Tail-1:** `posFlags` (position valid / quality), `sats` (satellite count when valid).
  - **Optional:** Age can be inferred receiver-side (e.g. from `lastRxAt` or sequence delta) when needed; it is not sent in v0.

- **Derived state (conceptual):**
  - **No position:** Core position absent (or not yet received) → show “no position” / “no fix”. Tail-1 `posFlags` qualifies the same Core sample and **does not revoke** position already received in Core; see [rx_semantics_v0](rx_semantics_v0.md).
  - **Position valid:** Core position present (and Tail-1 may indicate valid fix for that sample) → show position; optionally show “quality” (e.g. “N sats”) from `sats`.
  - **Quality band (optional):** UI may map `posFlags` + `sats` into a simple band (e.g. good / degraded) for display; exact mapping is implementation-defined and not specified here.

No code or wire format is defined in this doc; only the rule that PositionQuality is derived from the above inputs and that the only sent position-quality fields in v0 are Tail-1 `posFlags` and `sats`.

---

## 3. Placement rule (Tail-1 vs Tail-2 vs derived)

| Item | Placement | Rationale |
|------|-----------|-----------|
| **posFlags** | Tail-1 | Attached to Core sample; minimal quality indicator for the same sample. |
| **sats**     | Tail-1 | Same; satellite count for the same sample. |
| **fixType / hdop / accuracy / age** | Not sent in v0 | May be Tail-2 or later; not in current encoding. |
| **PositionQuality (state)** | Derived | UI/consumer state from Core + Tail-1 + optional receiver-side age. |

---

## 4. References

- **Encoding:** [beacon_payload_encoding_v0](../contract/beacon_payload_encoding_v0.md) — Tail-1 optional block (§4.2), examples (§5.3).
- **Inventory:** [nodetable_fields_inventory_v0](nodetable_fields_inventory_v0.md) — posFlags, sats (Tail-1); PositionQuality (derived).
- **Cadence:** [field_cadence_v0](field_cadence_v0.md) — Tail-1 cadence for posFlags/sats.
- **Index:** [NodeTable index §6](../index.md#6-position-v0) — Position and PositionQuality requirements (age, optional satCount/hdop/accuracy/fixType; in v0 only satCount via `sats` and quality via `posFlags` are sent).
