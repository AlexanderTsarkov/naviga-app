# Session — Identity exchange (names + roles) v0.1 (Contract)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#194](https://github.com/AlexanderTsarkov/naviga-app/issues/194)

This contract defines the **v0.1 session-only** identity exchange: name propagation (and optional role hint) via a loss-tolerant, low-traffic packet type. **No names over radio in v0** (beacon/OOTB); this applies **only in Session/Group context**. Semantic truth is this contract; no OOTB or UI as normative source.

---

## 1) Goal / Non-goals

**Goal:** Specify a docs-only v0.1 contract for session-only identity exchange (names + roles): packet concept, encoding, normalization, epoch/cache semantics, traffic policy, and UX policy. Loss-tolerant and low-traffic.

**Non-goals:**

- No implementation; spec and design only.
- No secure claim / encryption / access control / addressability protocol.
- No beacon changes; Beacon Core/Tail-1/Tail-2 unchanged.
- No OOTB public naming; names are not broadcast on public channel.

---

## 2) Context and constraints

### 2.1 v0 constraint (no names over radio)

Per [NodeTable index §4](../../nodetable/index.md): **No human-readable names over radio in v0.** **networkName** is not broadcast in v0 (excluded from Beacon Core/Tail-1/Tail-2 and from OOTB public). **localAlias** and **ShortId** remain local-only; not sent. This contract defines **session-only** identity exchange so it stays loss-tolerant and low-traffic, without touching beacons or OOTB.

### 2.2 Session-only in v0.1

Identity exchange (NameChunk or equivalent) is allowed **only** when the device is in **Session/Group** context (non-public channel with presumed recipient). Traffic model: [traffic_model_v0](../../radio/policy/traffic_model_v0.md) — session-only packet class; no identity packets on OOTB public.

---

## 3) Packet concept: NameChunk

### 3.1 Concept

- **Non-periodic, session-only** packet type: **NameChunk**.
- Name (and optional role hint) are split into **chunks**; each chunk is sent in a separate NameChunk packet. Receiver reassembles by **nodeId + epoch**; **epoch** resets the buffer (e.g. on name change or session re-join).
- **Loss-tolerant:** Missing chunks leave name incomplete; receiver can request resend or wait for next burst; no mandatory retransmission in v0.1.
- **Low-traffic:** Not on beacon; not periodic; sent on join/change or in a short burst for late joiners.

### 3.2 Fields (NameChunk)

| Field | Purpose |
|-------|---------|
| **nodeId** | DeviceId (same as NodeTable); identifies the node whose name is carried. |
| **epoch** | Monotonic or session-local epoch; receiver uses it to reset chunk buffer (new name = new epoch). |
| **totalLen** | Total byte length of the full name (or name+role encoding) when reassembled. |
| **chunkIndex** | Index of this chunk (0-based). |
| **chunkPayload** | Raw bytes of this chunk (UTF-8 or table-coded; see §4). |

### 3.3 Receiver behavior

- **Cache chunks by (nodeId, epoch)** until reassembly is complete (sum of chunk lengths ≥ totalLen and indices form a complete set, or a defined completion rule).
- **New epoch** for same nodeId **discards** previous buffer.
- No mandatory retransmission in v0.1; partial name may be shown as incomplete or after TTL (see §6).

---

## 4) Encoding modes + hard limits (v0.1)

Name is a short **tag**, not a full legal name.

### 4.1 Two encoding modes

1) **Table-coded:** `lang_ref` + 1 byte/char from a strict alphabet table.
   - **nameLenMaxChars = 16**
   - No mixed alphabets; only characters from the selected table.
2) **UTF-8 bytes** (fallback):
   - **nameMaxUtf8Bytes = 24**

### 4.2 Allowed special characters (table-coded)

- **`-` and `_` only** (no spaces in v0.1).

### 4.3 Contract obligations

- **Size limits:** Total name length, max chunk size, max chunks per name — to be set in implementation contract; bounds above (16 chars / 24 B) are normative for v0.1.
- **Supported alphabet list** (for table-coded option) and **table version** — to be defined in a follow-up or appendix.
- **Mixed-language names:** Not in scope for v0.1 (no-mix alphabets; single lang_ref per name).

---

## 5) Normalization (v0.1)

- **Canonical on-air representation:** **lowercase only** (table-coded / UTF-8 mode).
- **UI rendering:** **Title Case** — first character uppercase, the rest unchanged.
- No multi-word names in v0.1 (spaces disallowed); separators **`-`** and **`_`** do not trigger extra capitalization.

---

## 6) Epoch semantics + cache invalidation

- **Epoch** identifies a distinct “version” of the name for a node. New name or session re-join should use a new epoch.
- **New epoch for same nodeId:** Receiver **discards** previous chunk buffer for that nodeId; reassembly starts fresh.
- **TTL for partial:** Incomplete reassembly (missing chunks) may be discarded after a policy-defined TTL or on receipt of a new epoch. Exact TTL is implementation-defined; contract only requires that stale partial state can be cleared.
- **Collision notes:** If two nodes use the same display name in session, UX policy (§8) applies (e.g. suffix by ShortId, or “unverified name” indicator). No protocol-level collision resolution in v0.1.

---

## 7) Traffic policy: rate limits + controlled join burst

- **Rate limits:** v0.1 defines a **max chunks per node per time window**. Exact numbers TBD; contract states the rule: no unbounded flood of identity packets.
- **Controlled join burst:** For late joiners, a **burst strategy** is allowed (e.g. send full name once after join), then only on change. “Everyone replies with identity” without rate limits is **not** allowed (lessons from Meshtastic).
- No concrete numeric timing defaults here; only rules and constraints.

---

## 8) UX policy: unverified name; collision; stale

- **Unverified name:** Names received over radio (session) are **not** verified by secure claim in v0.1. UX must make this visible (e.g. “unverified name” indicator or styling) where product requires it.
- **Collision handling:** If two nodes claim the same display name in session, UX or product rule applies: e.g. suffix by ShortId, or show “unverified” until disambiguated. Contract records the requirement; exact UX TBD.
- **Stale handling:** Stale or partial name state should be clearable (epoch reset, TTL); UI should not present stale name as current without policy-defined refresh or invalidation.

---

## 9) Open questions

- **Size limits (detail):** Max chunk size; max chunks per name per epoch; code-point limits for UTF-8 mode.
- **Mixed-language names:** Deferred for v0.1; single script per name.
- **Role hint:** Whether role (human dongle / dog collar / infra) is carried in the same NameChunk stream or separate; alignment with v0 role derived from hwProfileId (NodeTable inventory).
- **Request/response:** Whether receivers may request missing chunks (and under what rate limits).

---

## 10) Related

- **Issue:** [#194 Session: Identity exchange (names + roles) v0.1 (docs-only)](https://github.com/AlexanderTsarkov/naviga-app/issues/194)
- **NodeTable naming constraint:** [../../nodetable/index.md](../../nodetable/index.md) §4 — no names over radio in v0.
- **Traffic model:** [../../radio/policy/traffic_model_v0.md](../../radio/policy/traffic_model_v0.md) — session-only packet class.
- **Research (Meshtastic patterns):** [../../../research/meshtastic_naming_patterns.md](../../../research/meshtastic_naming_patterns.md) — guardrails and anti-patterns.
