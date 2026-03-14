# Mobile — BLE contract S04 (first phase)

**Status:** Canon (contract)  
**Related issue:** [#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361)  
**Context:** This document is the canonical BLE contract for the first phase (S04, [#460](https://github.com/AlexanderTsarkov/naviga-app/issues/460)). Discovery, NodeTable read/update, node name, and profiles are normative; GATT/UUID and byte-level format remain for a later design step.

---

## 1. Purpose

Capture the agreed BLE contract direction for the first phase after S03 stabilization: discovery/advertising, NodeTable read/update, authoritative node name, and profiles/config. The document is the normative source for the BLE contract; it does not define final GATT/UUID layout or byte-level payload format.

---

## 2. Why now

S03 stabilization is the gate for detailed BLE design. This contract records product decisions already agreed so that implementation can be sliced later without inventing new behavior. It provides the single reference for the BLE contract track (#361).

---

## 3. Scope

### 3.1 Discovery / advertising

First BLE phase includes enough for app discovery and filtering: Naviga identity/service marker, BLE contract version marker (see §11), display identity (NodeName + ShortID when NodeName is non-empty; ShortID only when NodeName is empty), and BLE RSSI for app-side sorting/proximity.

### 3.2 NodeTable read contract

Initial state is reconstructed through explicit reads, including paged NodeTable reads. App supports full snapshot (paged), targeted read of a specific node/record, and baseline read; subscription is enabled only after baseline loading is complete.

### 3.3 NodeTable update contract

App can subscribe to updates after baseline is loaded; updates are batched/coalesced with a fixed 2-second window (post-baseline only). Update payload is full changed record; no snapshot tokens or revision fences in first phase. Reconnect is treated as fresh connect (baseline again, then resubscribe); no missed-update replay or delta recovery in first phase.

### 3.4 Authoritative node name contract

BLE contract includes authoritative **self** `node_name` read/write. Remote authoritative `node_name` comes via NodeTable when available. Remote alias persistence (on-node) and mobile-local aliases are **not** part of the BLE contract.

### 3.5 Profiles/config contract

First contract includes surface for: radio profiles (list / read / select / write / create / delete) and user profiles (list / read / select / write / create / delete). Default profile protection rules are acknowledged. Implementation order is not over-specified yet.

---

## 4. Non-goals

- No final byte-level format yet.
- No final GATT/UUID map yet.
- No mobile UI design.
- No owner-local metadata stored on node.
- No remote alias persistence inside node.
- No JOIN/Mesh expansion.
- No broad optimization/compression work.
- No legacy BLE preservation requirement.

---

## 5. Design principles

- **Legacy BLE is not authoritative.** The legacy BLE service was created ad hoc and should not constrain the new design. It may be fully replaced.
- **First phase: broad product-facing contract.** Export all product-facing NodeTable fields; avoid premature pruning. BLE is not LoRa; early byte-level optimization is not the priority. Later field-by-field refinement is allowed after real usage shows a field unnecessary.
- **Inclusion bias for fields:** if a field has plausible user/product meaning, include it in the first phase. **Exclusion bias:** exclude only if clearly internal runtime, debug-only, legacy bookkeeping, or service-only radio boundary metadata.

**Field-set rule (NodeTable export):** In the first BLE phase, the node exports **all product-facing NodeTable fields**. Product-facing = fields describing node identity; observed state; position / position freshness; battery / radio / profile / role; node name; derived user-visible status needed by app list/map/node detail/basic update logic.

**Explicit exclusions from BLE export:** `last_seq`, `last_seen_ms`, `last_payload_version_seen`, `last_core_seq16`, `has_core_seq16`, `last_applied_tail_ref_core_seq16`, `has_applied_tail_ref_core_seq16`, `packet_header`, `payloadVersion`, `in_use`.

**Remain in first-phase export (not excluded):** `last_rx_rssi`, `snr_last`, `last_seen_age_s`, `is_stale`, `node_name`, and product-facing position / position-quality fields.

Not a blind RAM dump; not aggressive early pruning. Broad product-facing contract first; later field-by-field refinement allowed.

---

## 6. Discovery / advertising contract

- **Naviga identity / service marker** so the app can filter compatible nodes.
- **BLE contract version marker** in the discovery layer (see §11 for compatibility behavior).
- **Display identity:** NodeName + ShortID when NodeName is non-empty; ShortID only when NodeName is empty.
- **BLE RSSI** is relevant for app-side sorting and proximity handling.

---

## 7. NodeTable read contract

- **Lifecycle:** Connect → baseline read / initial explicit reads → paged NodeTable snapshot read → only then enable subscription. **Baseline** denotes the initial state load, including the full paged NodeTable snapshot and any other explicit reads required before enabling subscription.
- Initial state is reconstructed through **explicit reads**, including paged NodeTable reads.
- **Subscription is enabled only after baseline loading is complete.**
- App supports **full snapshot** (paged), **targeted read** of a specific node/record, and baseline read.
- Do not over-optimize the first version.

---

## 8. NodeTable update contract

- App can **subscribe to updates** only after baseline loading is complete.
- Updates are **batched/coalesced** rather than pushed immediately one-by-one. First-phase batching uses a **fixed 2-second coalescing window**. Within one window, multiple changes across records are accumulated and delivered together; if the same record changes multiple times within the same window, only the **latest full state** for that record is included. The batching window applies to **post-baseline updates**, not to initial state acquisition.
- App may **explicitly request refresh** for a specific record (targeted read remains a separate operation).
- **Update model:** When a record changes and is delivered over the BLE update/subscription path, the node sends the **full changed record**. First phase does **not** use changed-field masks or partial field delta encoding. Rationale: BLE is not the constrained LoRa channel; simplicity and consistency are preferred over premature optimization.
- Not every ticking field triggers updates (e.g. uptime-like fields do not by themselves trigger push).
- **First phase does not require** snapshot tokens, revision markers, or additional consistency fences for baseline reconstruction. Any changes that occur during baseline loading are expected to arrive shortly afterward through the normal batched subscription update flow.
- **Reconnect:** If BLE connection is lost and later re-established, the app treats the new connection as a **fresh session**. It performs baseline reads again, including paged NodeTable snapshot reads, and only then re-enables subscription for subsequent batched updates. First phase does **not** require missed-update replay or delta recovery.

---

## 9. Authoritative node name contract

- BLE contract includes **authoritative self `node_name`** read/write.
- **Remote** authoritative `node_name` comes via NodeTable when available.
- **Remote human-readable aliases** stored only for the user/app (mobile-local) are **not** part of the BLE contract; they belong to future mobile-local persistence and UI logic. Do not mix that into #361 BLE contract.
- **Remote local alias persistence** (on-node) is not in scope.

### 9.1 node_name label constraints (S04 #466)

`node_name` is a **short display label** for map/list/discovery. Implementations must enforce:

- **Product/UI limit:** max **12 characters** (user-facing).
- **Storage/transport ceiling:** max **24 bytes UTF-8** (BLE payload and node storage).
- **Allowed characters:** Latin letters, Cyrillic letters, digits `0`–`9`, symbols: `-`, `_`, `#`, `=`, `@`, `+`.
- **Disallowed:** emoji/pictograms, control characters, and any symbol not in the allowlist above.

Rationale: compact readable label, predictable BLE/storage footprint, and a future radio-friendly upper bound.

---

## 10. Profiles contract

**BLE contract surface** (unchanged): Full surface for both radio and user profiles—list, read, select, write, create, delete.

**First mobile implementation:** May use only **list** and **read**. Later mobile iterations may enable select / write / create / delete without reopening the BLE contract.

**Default profile protection:** Default profile cannot be deleted; default profile cannot be edited directly.

### 10.1 Radio profiles

Contract: list / read / select / write / create / delete. First mobile implementation uses list + read only.

### 10.2 User profiles

Contract: list / read / select / write / create / delete. First mobile implementation uses list + read only.

---

## 11. Versioning / compatibility

BLE compatibility is determined by the **dedicated BLE contract version**, not by firmware version and not by mobile app release version. Discovery must include **Naviga** identity/service marker and a **BLE contract version marker** (see §6). The BLE contract version uses **major.minor** format. **Firmware version**, **mobile app version**, and **BLE contract version** evolve independently.

**First-phase compatibility rule:** Normal connect is allowed **only on exact BLE contract version match** (both major and minor equal).

**Version bump discipline:** Firmware version bumps on firmware releases; app version bumps on app releases; BLE contract version bumps **only** when BLE-visible contract semantics change. If BLE-visible contract semantics do not change, BLE contract version stays unchanged even if firmware or app versions advance.

**When versions do not match (any mismatch):** Node remains **visible** in discovery; app marks it **incompatible**; normal connect and normal data workflow are **not allowed**. Direction: **newer node / older app** → direct user to **update the app**; **older node / newer app** → intended direction is **firmware update for the node** (firmware update flow not implemented yet; stated future path).

**Out of scope for first phase:** Exact firmware update transport/OTA design; diagnostic/read-only fallback behavior for incompatible versions (optional future).

---

## 12. Open questions

- Final **GATT/UUID** layout (to be defined in a later design step).
- Final **byte-level payload format** (to be defined in a later design step).
- Optional future: limited **diagnostic/read-only fallback** for incompatible BLE contract versions.

---

## 13. Out-of-scope for first implementation

- Final byte-level format and GATT/UUID map.
- Mobile UI design.
- Owner-local metadata on node.
- Remote alias persistence on node.
- JOIN/Mesh expansion.
- Broad optimization/compression.
- Treating legacy BLE as authoritative or constraining the new design.

---

## 14. Promotion criteria (satisfied at promotion)

Promotion to canon was appropriate when:

- Contract blocks (discovery, NodeTable read/update, node name, profiles) are agreed.
- Field-set rule is agreed.
- Lifecycle is explicit: baseline read first, then subscribe; reconnect behaves as fresh connect.
- First-phase compatibility rule (exact BLE contract version match) is explicit.
- Remaining open questions are limited to later design layers (GATT/UUID, byte format, optional fallback), not unresolved product behavior.
- Implementation tasks can be sliced without requiring new product decisions.

This document has been promoted to canon (S04, #460). The normative version is at `docs/product/areas/mobile/contract/ble_contract_s04_v0.md`.
