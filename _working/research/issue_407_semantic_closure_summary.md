# #407 semantic cleanup — change summary

**Date:** 2025-03 (final semantic cleanup pass).  
**Goal:** Narrow remaining open points to items already decided or out of scope; prepare WIP set for final review and docs-only PR.

## Changes made

### 1. `docs/product/wip/areas/radio/policy/packet_context_tx_rules_v0_1.md`

- **§2a Scope:** Added explicit **Hitchhiking not allowed:** Node_Status is never enqueued in the same formation pass as Node_PosFull; status trigger and send are independent of the position path. No ad hoc piggybacking of status content into Node_PosFull or any other packet type.
- **§2a Steady-state:** Replaced “no ad hoc piggybacking into Node_PosFull” with: **Hitchhiking (same-pass or piggyback with PosFull) is not allowed.**
- **§4 Open points:** Renamed to “Open points and external dependencies”. Removed Hitchhiking and Protocol numbers from the open list. Pos_Quality remains as a single bullet: external/dependent work under [#359], not a blocker for #407 semantic closure. Added italic note: *Closed / out of scope for #407:* lifecycle agreed §2a; hitchhiking **closed — not allowed**; protocol/wire numbering = later contract/wire follow-up, not part of #407 closure; backward compat out of scope.

### 2. `_working/research/issue_407_update_draft.md`

- **Open (for later):** Now only Pos_Quality (external [#359], not a blocker); lifecycle, bootstrap, timing, and hitchhiking agreed in WIP §2a; protocol numbers out of scope for #407 closure.
- **Addendum:** Hitchhiking closed — not allowed; protocol numbers out of scope; remaining = Pos_Quality external only; #407 semantically ready for final review and docs-only PR prep.

### 3. `_working/research/issue_407_followup_comment.md`

- Hitchhiking closed — not allowed; protocol numbers out of scope; remaining = Pos_Quality as external dependency only; #407 semantically ready for final review and docs-only PR prep; issue stays open until that PR is handled.

## Result

- **Hitchhiking:** Explicitly closed as not allowed in main semantics (§2a) and in open-points note (§4). No wording implies partial status via other packet types.
- **Pos_Quality:** Clearly external/sibling [#359] only; not a blocker for #407 closure.
- **Protocol numbers:** Removed from #407 blocker/open list; stated as later contract/wire follow-up.
- **Draft and comment files:** Aligned with the above.

## Confirmation

**#407 is now semantically ready for final review and docs-only PR preparation.** Packet purpose, contents, creation rules, and send rules (including Node_Status lifecycle and “no hitchhiking”) are agreed and recorded. No open semantic blockers remain for #407; only external dependency (Pos_Quality bit layout under #359) and the actual docs-only PR remain.
