# NodeTable — Persistence cadence & limits (Policy v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 4

This doc defines **persistence set limits** (per-tier and overall), **snapshot write triggers**, **debounce and minimum write interval**, **dirty tracking** (concept), and **flash-friendly / crash-consistency** invariants. It does **not** define snapshot format/schema (Step 2), merge rules (Step 3), or normative OOTB/UI.

---

## 1) Scope guardrails

- **In scope (Step 4 only):** Limits on what is persisted (by tier + overall cap); when to trigger a snapshot write (immediate vs rate-limited); debounce and minimum-interval policy; dirty-tracking concept; crash-consistency and flash invariants.
- **Out of scope:** Snapshot format/serialization (Step 2). Merge rules (Step 3). Implementation details. No normative OOTB/UI; numeric defaults are configurable or policy-defined, not product truth.

---

## 2) Limits model (per-tier + overall)

- **Tier-aware limits:** The persisted set is governed by **per-tier caps or quotas** plus an **overall persisted-set cap**. T0 must **not** crowd out T1 or T2: T1 and T2 nodes have priority for persistence slots.
- **T0 (Ephemeral):** Persisted set is **minimal**. Keep a small number of the most recent Ephemeral nodes (e.g. for early testing when no T1/T2 exist). The count **N** is intentionally small and **configurable**; exact N is not fixed in this doc.
- **T2 (Pinned):** **Safety cap = 100** nodes. T2 is never evicted by capacity; only by user action (unpin). Up to the cap, all T2 nodes are persisted.
- **T1 (Session):** **Target** on the order of ~50; **safety cap = 100**. T1 is not evicted by capacity; only by inactivity or session/membership end per [persistence-v0](persistence-v0.md). No numeric inactivity thresholds defined here.
- **Overall cap:** The total persisted set (T0 + T1 + T2) is bounded. When at capacity, T0 is reduced first (eviction order per persistence-v0); T1/T2 are not evicted by capacity.

---

## 3) Write triggers

- **Immediate write** on **explicit LocalUser changes:** pin/unpin, trust/affinity change, tier change, localAlias edit, and similar user-driven updates. These trigger a snapshot write as soon as the change is applied (no debounce).
- **Rate-limited (debounced + minimum-interval) write** for:
  - Session membership changes (node added/removed from session).
  - Important-node position or state changes (policy may define “important” e.g. T2 or T1; no fixed list here).
  - Session close/end events.
- Separation is strict: **immediate** for LocalUser; **rate-limited** for system/session/position-driven updates so that frequent RX or session churn does not thrash persistence.

---

## 4) Debounce & minimum interval (mechanism)

- **Debounce:** A write triggered by a rate-limited event is **not** performed immediately; it is scheduled and may be coalesced with other such triggers within a **debounce window**. Only after the window expires without further triggers (or at window end) is the write performed. Exact window duration is **configurable**; not fixed here.
- **Minimum write interval:** No two snapshot writes (for rate-limited triggers) occur closer than a **minimum interval**. This protects flash and avoids write storms. Exact interval is **configurable**; not fixed here.
- Immediate LocalUser writes are **exempt** from the minimum interval so user actions are persisted promptly; they may still be coalesced with a pending rate-limited write if policy allows (implementation detail).

---

## 5) Dirty tracking (concept)

- **Dirty tracking** is the concept that the system tracks whether the in-memory NodeTable has changed since the last successful snapshot write. When “dirty” and a write trigger fires, a write is performed; when “clean”, no write is needed for that trigger type.
- In **v0**, the implementation may still write a **full snapshot** on each trigger (no incremental format required). Dirty tracking is a policy concept to define *when* to write; the *what* (full vs incremental) is out of scope for this doc.
- This doc does not specify the implementation of dirty tracking (e.g. per-field vs per-record flags); only that the concept exists and that writes are driven by the triggers in §3 plus debounce/interval in §4.

---

## 6) Flash & crash-consistency invariants

- **Restore:** On startup (or after crash), the system **restores the last successful snapshot**. There is no “partial” or “in-progress” snapshot used for restore; only a fully completed write is considered valid.
- **Atomic writes:** Writes should be **atomic** at the mechanism level: either the entire snapshot is committed (e.g. write to secondary then swap, or single atomic sector write), or the previous snapshot remains the one used for restore. No half-updated state is exposed to restore.
- **Flash-friendly:** Minimum write interval and debounce reduce write frequency. Policy should avoid unnecessary writes so that flash wear and power are bounded. Exact limits (e.g. max writes per day) are implementation or product configuration; not fixed here.

---

## 7) Open questions / TODO

- **Numeric defaults:** Concrete values for debounce window, minimum write interval, and T0 persisted N — to be set by implementation or product configuration; document defaults when chosen.
- **Incremental snapshot:** Whether a future design supports incremental or delta snapshot writes; out of scope for v0.
- **Dirty granularity:** Whether dirty tracking is per-node, per-tier, or global; implementation detail, not required for v0 policy.

---

## 8) Related

- **Retention tiers & eviction:** [persistence-v0.md](persistence-v0.md) — [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 1
- **Snapshot semantics (what is persisted/derived):** [snapshot-semantics-v0.md](snapshot-semantics-v0.md) — [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 2
- **Restore/merge rules:** [restore-merge-rules-v0.md](restore-merge-rules-v0.md) — [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157) Step 3
- **Source precedence:** [source-precedence-v0.md](source-precedence-v0.md) — [#156](https://github.com/AlexanderTsarkov/naviga-app/issues/156)
- **NodeTable contract (WIP):** [../index.md](../index.md)
