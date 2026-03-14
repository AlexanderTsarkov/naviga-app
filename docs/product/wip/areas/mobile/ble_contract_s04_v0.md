# Mobile — BLE contract S04 (first phase) — WIP

**Status:** WIP / Candidate  
**Related issue:** [#361](https://github.com/AlexanderTsarkov/naviga-app/issues/361)  
**Context:** Post-S03 stabilization; preparing S04 BLE contract work. This doc captures agreed product direction for the first BLE phase. It is the working design artifact for the #361 track until WIP review and possible promotion to canon.

---

## 1. Purpose

Capture the agreed BLE contract direction for the first phase after S03 stabilization: discovery/advertising, NodeTable read/update, authoritative node name, and profiles/config. The document is a WIP design artifact—not canon—and does not define final GATT/UUID layout or byte-level payload format.

---

## 2. Why now

S03 stabilization is the gate for detailed BLE design. This WIP records product decisions already agreed so that implementation can be sliced later without inventing new behavior. It provides a single reference for the BLE-prep/design track (#361).

---

## 3. Scope

### 3.1 Discovery / advertising

First BLE phase includes enough for app discovery and filtering: Naviga identity/service marker, display identity (NodeName + ShortID when NodeName is non-empty; ShortID only when NodeName is empty), compatibility/version marker as part of discovery/versioning, and BLE RSSI for app-side sorting/proximity.

### 3.2 NodeTable read contract

App supports full snapshot on connect; paging is built in from day one; targeted read of a specific node/record exists. First version is not over-optimized.

### 3.3 NodeTable update contract

App can subscribe to updates; updates are batched/coalesced, not necessarily one-by-one; app may explicitly request refresh for a specific record. Exact batching interval is an open question. Not every ticking field triggers updates (e.g. uptime-like fields do not by themselves trigger push).

### 3.4 Authoritative node name contract

BLE contract includes authoritative **self** `node_name` read/write. Remote authoritative `node_name` comes via NodeTable when available. Remote local alias persistence is **not** part of the BLE contract.

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

- App supports **full snapshot on connect**.
- **Paging** is built in from day one.
- **Targeted read** of a specific node/record exists.
- Do not over-optimize the first version.

---

## 8. NodeTable update contract

- App can **subscribe to updates**.
- Updates are **batched/coalesced**, not necessarily pushed one-by-one.
- App may **explicitly request refresh** for a specific record.
- **Update model:** When a record changes and is delivered over the BLE update/subscription path, the node sends the **full changed record**. First phase does **not** use changed-field masks or partial field delta encoding. Targeted read remains available as a separate operation. Rationale: BLE is not the constrained LoRa channel; simplicity and consistency are preferred over premature optimization.
- Exact batching interval remains an **open question**.
- Not every ticking field triggers updates (e.g. uptime-like fields do not by themselves trigger push).

---

## 9. Authoritative node name contract

- BLE contract includes **authoritative self `node_name`** read/write.
- **Remote** authoritative `node_name` comes via NodeTable when available.
- **Remote human-readable aliases** stored only for the user/app (mobile-local) are **not** part of the BLE contract; they belong to future mobile-local persistence and UI logic. Do not mix that into #361 BLE contract.
- **Remote local alias persistence** (on-node) is not in scope.

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

Discovery must include **Naviga** identity/service marker and a **BLE contract version marker** (see §6). First-phase compatibility behavior:

- **Compatible** (app and node BLE contract versions match): normal connect allowed.
- **Incompatible newer node / older app** (node has newer BLE contract version than app supports): node remains **visible** in the app list; app marks it **incompatible**; normal connect and normal data workflow are **not allowed**; user should be directed to **update the app**.
- **Incompatible older node / newer app** (node has older BLE contract version than app supports): node remains **visible** in the app list; app marks it **incompatible**; normal connect and normal data workflow are **not allowed**; intended direction is **firmware update for the node**. Firmware update flow is not implemented yet; this is the stated future path.

**Out of scope / open for first phase:** exact compatibility matrix math; diagnostic/read-only fallback behavior; exact firmware update transport/OTA design.

---

## 12. Open questions

- Exact **batching interval** for NodeTable updates.
- Final **GATT/UUID** layout (out of scope for this WIP; to be defined in a later design step).
- Final **byte-level payload format** (out of scope for this WIP).

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

## 14. Promotion criteria

Promotion to canon becomes appropriate when:

- Contract blocks (discovery, NodeTable read/update, node name, profiles) are agreed.
- Field-set rule is agreed.
- Open questions are explicit.
- Implementation tasks can be sliced without requiring new product decisions.

Until then, this document remains WIP and is the working design artifact for the #361 track.
