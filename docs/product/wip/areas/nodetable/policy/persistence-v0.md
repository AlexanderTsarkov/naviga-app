# NodeTable — Retention tiers & eviction (Policy v0)

**Work Area:** Product Specs WIP · **Parent:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147) · **Issue:** [#157](https://github.com/AlexanderTsarkov/naviga-app/issues/157)

Step 1: Retention tiers (T0/T1/T2), tier assignment rules, eviction rules, and data-state semantics for **fresh / stale / evicted**. This doc defines **data states and policy only**; it does **not** prescribe UI rendering, snapshot/restore format, serialization schema, cadence, flash limits, or write limits.

---

## 1) Scope guardrails

- **In scope:** Three retention tiers; how a node enters/leaves each tier; eviction rules per tier; definitions of **fresh**, **stale**, and **evicted** as **data states** (not UI).
- **Out of scope (v0):** Snapshot/restore format and serialization schema; cadence of persistence or flash/write limits; UI presentation of tiers or states (except one policy statement: T0 may be hidden when stale by default).

---

## 2) Definitions

| Term | Meaning |
|------|--------|
| **Tier** | Retention tier: T0 (Ephemeral), T1 (Session), T2 (Pinned). Determines how long a node is kept and under what conditions it may be evicted. |
| **Retention** | Policy that governs how long node data is kept in NodeTable and under what conditions it is removed (evicted). |
| **Stale (data state)** | Derived state when **lastSeenAge** exceeds a staleness boundary (boundary value chosen elsewhere). Stale does not imply eviction. |
| **Fresh (data state)** | Node data that is not stale (lastSeenAge within Online/Uncertain or within the stale boundary depending on product definition). |
| **Evicted (data state)** | Node has been **removed** from the live NodeTable by eviction policy (or user action for T2). Eviction is a **transition**: the node record is dropped; “evicted” is the outcome, not a stored field. |
| **Eviction** | The act of removing a node record from NodeTable according to tier-specific rules. |

---

## 3) Tiers (strictly three)

| Tier | Name | Intent |
|------|------|--------|
| **T0** | Ephemeral | Short-lived, best-effort retention. Default for newly received (RX) nodes. No guarantee of retention; aggressive eviction. |
| **T1** | Session | Session membership / allowed participation. Nodes accepted into a session (e.g. hunting session with N participants) stay in T1 for the session without requiring per-node pinning. |
| **T2** | Pinned (LocalUser address book) | User-pinned; LocalUser annotations / address book. Never auto-evicted; removal only by user action. |

**Default for new RX node:** **T0**. Any node first observed via RX enters NodeTable in T0 unless tier assignment rules (below) place it in T1 or T2.

---

## 4) Tier T0 — Ephemeral

- **Intent:** Transient nodes (one-off, noisy, untrusted, or simply not yet promoted). Quarantine-style and one-off nodes stay in T0 and are evicted first.
- **How node enters T0:** Default for any newly received (RX) node; or demotion from T1/T2 by policy or user action (if/when supported).
- **How node leaves T0:** Promotion to T1 (e.g. session membership) or T2 (user pin / address book); or **eviction** (removal).
- **Stale (T0):** Same as global definition: lastSeenAge exceeds staleness boundary. T0 nodes may be **hidden when stale by default** (policy; UI may hide them). T0 nodes are **not** auto-removed from the table merely for being stale; eviction is a separate step (see eviction rule).
- **Eviction rule (T0):** **Aggressive.** Evict when capacity or policy requires; T0 is the first candidate. No specific numeric threshold prescribed here; “evict T0 first” and “quarantine / one-off stay in T0 and are evicted first” is the rule.

---

## 5) Tier T1 — Session

- **Intent:** Session membership / allowed participation. Example: hunting session with 50 participants; nodes accepted into the session stay in T1 without pinning each. Not “often seen” or heuristic-based.
- **How node enters T1:** Assigned by session membership / participation rules (e.g. “accepted into current session”). Not by automatic “often seen” promotion.
- **How node leaves T1:** Session ends or membership revoked; or promotion to T2 (user pin); or **eviction** if inactivity rule applies.
- **Stale (T1):** Same as global definition. T1 nodes are **not** auto-removed when stale; they remain in the table. Stale is a data state only; no automatic eviction solely because of staleness for T1.
- **Eviction rule (T1):** Evict **only if** session participation/inactivity (membership-level) exceeds an **inactivity threshold** — not radio lastSeen. No specific numbers in this doc; threshold is policy/implementation. **Capacity pressure never evicts T1; only inactivity threshold may.**

---

## 6) Tier T2 — Pinned (LocalUser address book)

- **Intent:** LocalUser annotations and address book. User-explicit “keep” / pin.
- **How node enters T2:** User action (pin, add to address book, or equivalent).
- **How node leaves T2:** **Only by user action** (unpin, remove from address book). No automatic eviction.
- **Stale (T2):** Same as global definition. T2 nodes are **not** auto-removed when stale; they remain in the table.
- **Eviction rule (T2):** **None by policy.** T2 is never auto-evicted. Only user action removes a T2 node (or clears the pin so it may fall back to T1/T0 if applicable).

---

## 7) Tier assignment rules

- **LocalUser overrides:** If the user has pinned a node (T2), that node is in T2. LocalUser “address book” / pin state wins over session or ephemeral.
- **Session membership (T1):** Nodes that are explicitly part of the current session (e.g. “accepted into hunting session”) are in T1. Assignment to T1 is by session rules, not by “often seen” or similar heuristics.
- **Default (T0):** Any new RX node for which no T2 (pin) or T1 (session) rule applies is **T0**.
- **Precedence (conceptual):** T2 (LocalUser) > T1 (session) > T0 (default). When in doubt, new node stays T0.

*(Relationship/Affinity and source precedence are in [source-precedence-v0](source-precedence-v0.md); retention tier is separate from Relationship but may interact with “keep in map” / persistence flags in implementation.)*

---

## 8) Eviction summary

| Tier | Eviction rule |
|------|----------------|
| **T0** | Aggressive; first candidate when capacity or policy requires; quarantine/one-off evicted first. |
| **T1** | Only if session participation/inactivity (membership-level) exceeds inactivity threshold; capacity never evicts T1. |
| **T2** | Only by user action; never auto-evicted. |

---

## 9) Open questions / TODO (Step 2–4)

- **Snapshot/restore:** Which tiers are persisted and in what form (Step 2+).
- **Inactivity threshold (T1):** Define or bind numeric/instrumented threshold for session participation/inactivity (membership-level).
- **Capacity and ordering:** When evicting T0, ordering rule (e.g. oldest lastSeen first, or by staleness); any global capacity cap.
- **Demotion:** Whether T2→T1 or T1→T0 demotion is supported and under what conditions.
- **Quarantine vs T0:** Whether “quarantine” is a distinct flag or purely “stays in T0 and is evicted first”; align with Relationship (e.g. Ignored) if needed.
- **T1 eviction vs staleness:** Tie T1 eviction to session-level inactivity only (threshold TBD); see Related for non-normative staleness boundary examples.

---

## 10) Related

- **NodeTable contract (WIP):** [../index.md](../index.md)
- **Source precedence (policy):** [source-precedence-v0.md](source-precedence-v0.md) — [#156](https://github.com/AlexanderTsarkov/naviga-app/issues/156)
- **OOTB profiles & activity (non-normative example):** [ootb-profiles-v0.md](ootb-profiles-v0.md) — activity interpretation and staleness boundary examples.
