# Issue realignment after S03/V1 Product Truth canon correction

**Scope:** Issue/PR coordination only. No code changes. No new implementation PRs. Product Truth (now canon) is authoritative; any conflicting issue/PR assumption is wrong and must be updated.

**Context:** Canon was corrected to match Product Truth. #418 and #417 have been reopened. #419 remains open (prior PR abandoned). PR #430 is obsolete and should not continue as the implementation vehicle.

---

## Canon assumptions used

- **Radio-driven NodeTable core:** Normalized separate fields (pos_Lat, pos_Lon, pos_fix_type, pos_sats, pos_accuracy_bucket, pos_flags_small, pos_age_s; battery_percent, charging, battery_est_rem_time, **uptime_10m**; tx_power_step, channel_throttle_step; role_id, max_silence_10s, hw_profile_id, fw_version_id). Pack at radio boundary only; NodeTable stores normalized product fields.
- **uptime_10m:** Canonical uptime field name (10-minute units). Replaces uptimeSec/uptime_sec as product truth.
- **pos_age_s:** **Node-owned canonical position freshness field.** In NodeTable and BLE; **persisted**. Age of last position sample (seconds).
- **No canonical NodeTable Tail/Core ref fields:** last_core_seq16, has_core_seq16, last_applied_tail_ref_core_seq16, has_applied_tail_ref_core_seq16 are **not** part of canonical NodeTable; must not be persisted or in BLE. Tail–Core correlation is runtime-local decoder logic only.
- **Single node_name:** One canonical field for self and remote; do not split; overwrite/conflict = UI/UX policy only.
- **User Profile:** role_id, min_interval_s, min_distance_m, max_silence_10s. Pointers: default_user_profile, current_user_profile, previous_user_profile.
- **Radio Profile:** channel_slot, rate_tier, tx_power_baseline_step. Pointers: default_radio_profile, current_radio_profile, previous_radio_profile.
- **Active-values plane:** Runtime/NodeTable/BLE use active profile **values**, not profile objects.
- **Profile-management plane:** list, read, create, update, delete, set current; **FW Default cannot be deleted**. S03 defines model/semantics for S04 BLE/mobile readiness.
- **last_seen_ms:** Internal anchor only; **not** persisted; **not** in BLE. Restored entries have no presence anchor until first RX/TX → **restored entries start stale**.
- **Receiver-injected:** last_seq, last_seen_ms, last_rx_rssi, snr_last (BLE yes; not persisted); last_payload_version_seen optional debug-only, not BLE, not persisted.
- **Derived:** last_seen_age_s, is_stale (BLE yes; not persisted); pos_age_s (BLE yes; **persisted**).

---

## #416

- **Recommended action:** Keep open; update body with a concise status note.
- **What to update:** Add a short “Canon correction (S03/V1)” status block at the top or in a clear section: (1) Canon was corrected to Product Truth (NodeTable field set, no Tail/Core ref fields in canon, uptime_10m, pos_age_s node-owned and persisted, profile pointers and profile-management plane, single node_name). (2) Implementation work is being realigned to the corrected canon. (3) #417, #418, #419, #420, #421, #422 (and related) are being updated accordingly; prior implementation that relied on old canon (e.g. persisting last_core_seq16/last_applied_tail_ref_core_seq16 or last_seen_ms) is superseded. (4) PR #430 is obsolete; do not use it as the implementation vehicle. New work should branch from main and follow corrected canon.
- **Comment to post:**  
  **Canon correction and realignment.** S03/V1 Product Truth has been promoted to canon. Key changes: NodeTable has no canonical Tail/Core ref fields (last_core_seq16, last_applied_tail_ref_core_seq16 etc.); pos_age_s is node-owned and persisted; uptime_10m is canonical; last_seen_ms is internal only and not persisted (restored entries start stale); single node_name; User/Radio profile pointers and profile-management plane defined. Implementation is being realigned: #417, #418, #419, #420, #421, #422 are being synced to this canon. PR #430 is obsolete. New implementation should follow the corrected canon docs.

---

## #417

- **Recommended action:** Keep open; sync description and acceptance criteria to corrected canon.
- **What to update:** (1) State that seq16 persistence and restore refer to **last_seq** only (the single canonical seq field in NodeTable); no persistence or use of last_core_seq16 or last_applied_tail_ref_core_seq16. (2) Any checklist or DoD that mentions “ref” or “Tail/Core” in NodeTable should be removed or rephrased to “last_seq only; no NodeTable ref-state fields.” (3) If the issue still references PR #428 or “done” status, add a note: “Reopened for alignment check after canon correction; any prior implementation must be verified against corrected canon (no NodeTable ref fields, uptime_10m where applicable).”
- **Comment to post:**  
  **Canon alignment check.** Reopened after S03/V1 canon correction. Canon now: only **last_seq** is the canonical seq field in NodeTable; **last_core_seq16** and **last_applied_tail_ref_core_seq16** are not part of NodeTable truth (runtime-local decoder only). Please ensure seq16 persistence/restore in this issue (and any related PR) uses only last_seq and does not persist or expose Tail/Core ref fields as product NodeTable. Uptime field name in NodeTable/product surface should be **uptime_10m** per canon.

---

## #418

- **Recommended action:** Keep open; **rewrite** description and acceptance criteria to match corrected canon.
- **What to update:** (1) **Persisted set:** Must include pos_age_s (node-owned canonical position freshness). Must **not** include last_core_seq16, has_core_seq16, last_applied_tail_ref_core_seq16, has_applied_tail_ref_core_seq16. Must **not** include last_seen_ms (internal anchor only; not persisted). (2) **Restore behavior:** Restored entries **start stale** until first presence event (no persisted last_seen_ms); last_seen_age_s and is_stale are derived after restore from “no anchor” until first RX/TX. (3) **Field names:** Use canonical names (e.g. uptime_10m, not uptime_sec; normalized Position block field names where applicable). (4) Remove any requirement to “persist last_seen_ms for lastSeenAge” or “persist last_core_seq16 for Tail ref gate.” (5) If the issue or a linked PR (e.g. #429) described a snapshot format that persisted legacy ref fields or last_seen_ms, add an explicit note that that format is superseded by canon; implementation must follow the corrected persistence rules above.
- **Comment to post:**  
  **Canon correction — #418 scope rewrite.** Canon was corrected: NodeTable has **no** canonical Tail/Core ref fields; **last_seen_ms** is not persisted (internal anchor only); **pos_age_s** is node-owned and persisted; restored entries **start stale** until first RX/TX. Snapshot/restore must: persist pos_age_s and the normalized NodeTable product set; **not** persist last_core_seq16, has_core_seq16, last_applied_tail_ref_core_seq16, has_applied_tail_ref_core_seq16, or last_seen_ms. Any prior snapshot format (e.g. in a closed PR) that persisted those is superseded. Issue description and DoD have been updated to match; implementation should follow the corrected canon.

---

## #419

- **Recommended action:** Keep open; sync description and scope to corrected canon.
- **What to update:** (1) Aligning “NodeTable to canon” means: **no** last_core_seq16, has_core_seq16, last_applied_tail_ref_core_seq16, has_applied_tail_ref_core_seq16 in NodeTable product/export; Tail–Core correlation only in runtime-local decoder state. (2) Position block: normalized fields (pos_Lat, pos_Lon, pos_fix_type, pos_sats, pos_accuracy_bucket, pos_flags_small, pos_age_s); pos_age_s is node-owned canonical position freshness. (3) Uptime: canonical name **uptime_10m**. (4) Single node_name; profile pointers per canon (default_user_profile, current_user_profile, etc.; default_radio_profile, current_radio_profile, etc.). (5) Remove or rephrase any step that “adds Tail ref fields to NodeTable” or “persists last_core_seq16.”
- **Comment to post:**  
  **Canon alignment.** Prior implementation abandoned (PR closed). Canon is now corrected: no Tail/Core ref fields in NodeTable; pos_age_s node-owned and persisted; uptime_10m; single node_name; profile pointers and active-values/profile-management plane as in canon. This issue’s scope is to align NodeTable (and any RX/apply logic) to this field set and these rules; Tail–Core correlation remains runtime-local only, not stored in NodeTable.

---

## #420

- **Recommended action:** Keep open; sync description to corrected canon.
- **What to update:** (1) BLE snapshot field set must match Product Truth: normalized Position block, battery block (battery_percent, charging, battery_est_rem_time, uptime_10m), last_rx_rssi, snr_last, last_seen_age_s, is_stale, pos_age_s, identity (node_id, short_id, is_self, short_id_collision), single node_name. (2) **Must not** include last_core_seq16, last_applied_tail_ref_core_seq16, or last_seen_ms in BLE export. (3) last_seq: in NodeTable but not in BLE per canon. (4) Any checklist or “fields to export” list should be replaced or validated against canon BLE/persistence matrix (see product_truth_s03_v1 §1–§6).
- **Comment to post:**  
  **Canon alignment.** BLE snapshot must follow corrected canon: export the normalized product set (Position block, battery, uptime_10m, last_rx_rssi, snr_last, last_seen_age_s, is_stale, pos_age_s, identity, node_name). Do **not** export last_core_seq16, last_applied_tail_ref_core_seq16, or last_seen_ms. last_seq is in NodeTable but not in BLE. Issue scope/checklist should match docs/product/areas/nodetable policy and Product Truth.

---

## #421

- **Recommended action:** Keep open; sync description to corrected canon.
- **What to update:** (1) Profile **pointers** use canonical names: **default_user_profile**, **current_user_profile**, **previous_user_profile** (User); **default_radio_profile**, **current_radio_profile**, **previous_radio_profile** (Radio). (2) **Profile-management plane** is canon-defined: list, read, create, update, delete, set current; **FW Default profile cannot be deleted**. (3) **Active-values plane:** Runtime/NodeTable/BLE use **active profile values**, not profile objects. (4) User Profile content: role_id, min_interval_s, min_distance_m, max_silence_10s. Radio Profile content: channel_slot, rate_tier, tx_power_baseline_step. (5) Remove or rephrase any reference to CurrentRoleId/PreviousRoleId or CurrentProfileId/PreviousProfileId as the only product names; canon uses the default/current/previous_user_profile and default/current/previous_radio_profile naming.
- **Comment to post:**  
  **Canon alignment.** Profile semantics now in canon: pointer names **default_user_profile**, **current_user_profile**, **previous_user_profile** (User) and **default_radio_profile**, **current_radio_profile**, **previous_radio_profile** (Radio). Profile-management: list, read, create, update, delete, set current; FW Default cannot be deleted. Active-values: operational state uses active profile values, not profile objects. Implementation should follow role_profiles_policy_v0 and radio_profiles_policy_v0 as updated.

---

## #422

- **Recommended action:** Keep open; sync description to corrected canon.
- **What to update:** (1) NodeTable and runtime use **active profile values** (e.g. tx_power_step from current_radio_profile, role_id and max_silence_10s from current_user_profile), not profile objects. (2) Fields populated from active profiles: role_id, max_silence_10s (user); tx_power_step baseline (radio); channel_throttle_step default 0; hw_profile_id, fw_version_id from build-time identity. (3) No reference to “profile object in NodeTable”; only the **values** that come from the active profile are stored or used in NodeTable/operational state.
- **Comment to post:**  
  **Canon alignment.** Canon defines **active-values plane**: runtime/NodeTable/BLE use active profile **values** (e.g. role_id, max_silence_10s, tx_power_step), not profile objects. This issue’s scope is to ensure NodeTable and operational state are populated from current_user_profile and current_radio_profile **values** only; no profile refs in the product field set.

---

## Other affected issues

- **#358** — Keep open; sync: Packet→NodeTable mapping doc is updated in canon; implementation must not persist or expose last_core_seq16/last_applied_tail_ref_core_seq16 as NodeTable fields; Tail apply uses runtime-local correlation only. Post short comment: “Canon packet_to_nodetable_mapping updated; impl must follow it (no NodeTable ref fields, uptime_10m, pos_age_s node-owned).”
- **#367** — Leave unchanged unless body explicitly listed legacy ref fields as persisted; if so, add one-line comment that canon no longer persists those; snapshot format follows #418/canon.
- **#368** — Leave unchanged; node_name is single canonical field and self-only NVS per canon; scope already aligned.
- **#371** — Leave unchanged; grace_s/expected_interval_s semantics unchanged in canon.
- **#372** — Leave unchanged; self presence TX set (Alive, Core_Pos, Core_Tail) unchanged; optional comment that NodeTable does not store ref-state for Tail.
- **#375** — Leave unchanged; last_rx_rssi in NodeTable, BLE yes, not persisted; scope aligned.
- **#376** — Leave unchanged; snr_last (canonical name) in NodeTable and BLE with NA sentinel; scope aligned.

---

## PR #430

- **Recommended action:** **Close as obsolete/superseded** if still open. Do not use as the implementation vehicle for S03 canon-aligned work.
- **Notes:** Canon was corrected after any implementation in #430; the PR almost certainly reflects old assumptions (e.g. NodeTable ref fields, last_seen_ms persistence, or old profile/field names). Closing it avoids confusion and makes clear that new work should branch from main and follow the corrected canon. If the PR is already closed, add a short comment on the PR or in #416 that “PR #430 is superseded by canon correction; further implementation follows corrected Product Truth and new branches from main.”

---

## Execution order

1. **Post comment on #416** with the canon correction and realignment status (so the umbrella states the new baseline).
2. **Update #416 body** with the short “Canon correction (S03/V1)” status block (what changed, that implementation is being realigned, #417/#418/… updated, PR #430 obsolete).
3. **Close PR #430** (if open) with a one-line close reason: “Superseded by S03/V1 canon correction; implementation will follow corrected Product Truth from main.”
4. **#418:** Rewrite issue body and acceptance criteria to match corrected canon (persisted set, no ref fields, no last_seen_ms, restored entries start stale, uptime_10m, pos_age_s); then post the #418 comment.
5. **#417:** Update body/checklist (last_seq only; no NodeTable ref fields; uptime_10m); post #417 comment.
6. **#419:** Update body/scope (no ref fields in NodeTable; pos_age_s node-owned; uptime_10m; profile pointers); post #419 comment.
7. **#420:** Update body/checklist (BLE field set per canon; no ref fields or last_seen_ms in BLE); post #420 comment.
8. **#421:** Update body (profile pointer names; profile-management plane; active-values); post #421 comment.
9. **#422:** Update body (active-values plane only; values from active profiles); post #422 comment.
10. **#358** (and optionally #367): Post short alignment comment only unless the issue body explicitly contradicts canon.

---

## Risks to avoid

- **Preserving stale scope:** Do not leave issue text that says “persist last_core_seq16” or “persist last_seen_ms” or “Tail ref in NodeTable”; rewrite or comment that canon has changed.
- **Treating PR #430 as current:** Do not reference #430 as the implementation to merge or build on; close it and direct work to new branches from main.
- **Mixing old and new canon:** Acceptance criteria and checklists must reference only the corrected canon (Product Truth / updated policy docs); remove or replace criteria that assume the old field set or persistence rules.
- **Creating new issues instead of updating:** Prefer rewriting #417, #418, #419, #420, #421, #422 and commenting on #358/#367; avoid spawning new “canon alignment” issues unless one is clearly missing.
- **Code or PR creation:** This realignment is issue/PR housekeeping only; no code edits and no new implementation PRs.

---

## Exit criteria

- [x] Affected S03 issues audited (#416, #417, #418, #419, #420, #421, #422; #358, #367, #368, #371, #372, #375, #376).
- [x] Per-issue action decided (keep open + rewrite/sync for #416–#422; comment-only for #358 and optionally #367; leave unchanged for #368, #371, #372, #375, #376).
- [x] Umbrella (#416) update prepared (status note + comment text).
- [x] PR #430 housekeeping decided (close as obsolete if open; note superseded by canon).
- [x] Safe update order defined (umbrella first, then #430, then #418 rewrite, then #417, #419, #420, #421, #422, then #358/#367 comments).
