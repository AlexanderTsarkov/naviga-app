## Goal

Investigate and clarify the semantics of the OLED traffic counters: **PosTx**, **StTx**, **PosRx**, **StRx** (and their underlying `tx_sent_*` / `rx_ok_*` counters). Resolve any mismatch between what is displayed, what the labels imply, and how counters are incremented — via documentation, label/wording change, or counter-semantic correction. This is **observability semantics** work, not a radio reliability fix by default.

**Status:** Non-blocking; suitable for later cleanup / end-of-sprint tail work after current major track (#361 BLE-prep). Do not treat as immediate execution.

---

## Observed symptom on real nodes

On real two-node runs, the displayed **received** packet counters on one node can be **greater than** the corresponding **sent** packet counters on the peer.

- Example: Node A `PosTx = 82`, Node B `PosRx (from A) = 83`.
- For Status, the gap can be larger (e.g. `46` sent vs `50` received on the other side).
- In the opposite direction, counts may behave more intuitively (`Rx ≤ peer Tx`).

This suggests that current OLED counter labels look symmetric (PosTx/PosRx, StTx/StRx) while the underlying counter semantics are **not** actually symmetric or peer-mirrored.

---

## Why this matters

- **Debugging:** Misleading counters make it harder to reason about link behaviour and packet flow.
- **Product clarity:** If we expose these on OLED (or later via BLE), users/developers should understand what they mean.
- **Correctness:** The counters may represent different lifecycle points (e.g. “accepted decode” vs “sent on air”) rather than strict peer-mirrored totals; that should be explicit.

---

## Scope

- **In scope:** Semantics of `tx_sent_pos_full`, `tx_sent_status`, `rx_ok_pos_full`, `rx_ok_status` (and any related enqueue/drop/reject counters); where they are incremented; whether they are peer-specific or aggregate; OLED labels and wording.
- **Relevant areas:** `M1Runtime::handle_tx()`, `M1Runtime::handle_rx()`, `TrafficCounters`, OLED status data plumbing/rendering, v0.2 packet acceptance/duplicate/update semantics.
- **Out of scope:** BLE (#361); full observability redesign; treating this as a radio reliability bug unless investigation shows otherwise.

---

## Non-goals

- Do **not** implement the fix in this issue; investigation and clarification first.
- Do **not** reopen #447 or change HW validation outcome.
- Do **not** broaden into BLE or full observability redesign.
- Do **not** assume radio bug; likely problem class is counter/label semantics.

---

## Investigation questions

- What exactly does `tx_sent_*` mean (e.g. “frame handed to radio” vs “acknowledged by modem”)?
- What exactly does `rx_ok_*` mean (e.g. “accepted and applied” vs “valid decode only”)?
- At what lifecycle point is each counter incremented (code sites)?
- Are RX counters unique by `(origin, seq)` or do they count accepted frames/events more broadly (e.g. duplicates, retries)?
- Are counters **peer-specific** or **aggregate** across all remotes?
- Should OLED keep showing these raw counters as-is, or show different/more precise metrics (e.g. “accepted from peer X”)?
- Is a **label change** enough (e.g. “PosRx (accepted)”) or is a **code change** needed (e.g. peer-scoped or stricter definitions)?

---

## Suggested future execution outline

1. Reset counters on both nodes; run a deterministic two-node scenario (e.g. GNSS override).
2. Dump counters on both sides in a controlled window; compare TX, RX_OK, RX_REJECT and related.
3. Inspect increment sites in code (`handle_tx`, `handle_rx`, BeaconLogic, etc.).
4. Decide whether the fix is:
   - **documentation / label clarification**,
   - **OLED wording change**, or
   - **actual counter-semantic correction** (e.g. peer-specific or stricter definitions).
5. Implement the chosen fix in a follow-up change; keep scope minimal.

---

## Exit criteria

- [ ] Counter semantics and increment sites documented (or linked from code).
- [ ] Decision recorded: docs-only, label/wording change, or code change.
- [ ] If code/labels change: OLED (and any serial/debug output) consistent with the chosen semantics.
- [ ] No regression to #447 HW validation or current behaviour unless intentionally corrected.
