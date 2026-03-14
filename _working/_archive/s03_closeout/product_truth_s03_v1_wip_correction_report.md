# Product Truth S03/V1 — WIP correction report

**Iteration:** S03__2026-03__Execution.v1 (first line of `_working/ITERATION.md`).  
**Scope:** Docs-only. No canon edits. No code edits. No PR #430 usage.

---

## WIP files updated

**First pass:** product_truth_s03_v1.md (new), radio_profiles_model_s03.md, user_profile_s03_v1.md (new), ble_snapshot_s04_gate.md, README.md.

**Second pass (Tail/Core + uptime_10m):** product_truth_s03_v1.md, gnss_tail_completeness_and_budgets_s03.md, product_truth_s03_v1_wip_correction_report.md (this file).

---

## What was changed

- **product_truth_s03_v1.md (new):** Single authoritative WIP doc encoding Product Truth: radio-driven NodeTable core (position, battery, radio control, role/profile-driven); rules (pack at radio boundary, normalized fields, source-of-value notes); receiver-injected fields (last_seq, last_seen_ms, last_rx_rssi, snr_last, last_payload_version_seen) with BLE/persisted flags; derived fields (last_seen_age_s, is_stale, pos_age_s); identity (node_id, short_id, is_self, short_id_collision); single node_name; legacy removals (last_core_seq16, has_core_seq16, last_applied_tail_ref_core_seq16, has_applied_tail_ref_core_seq16); User Profile content and pointers; Radio Profile content and pointers; active-values plane; profile-management plane (list/read/create/update/delete/set current, FW Default cannot be deleted).
- **radio_profiles_model_s03.md:** Pointer semantics section now uses canonical names default_radio_profile, current_radio_profile, previous_radio_profile; §8 retitled to “Profile-management plane” and extended with update, delete, and “FW Default profile cannot be deleted”; explicit link to Product Truth §11.
- **user_profile_s03_v1.md (new):** WIP stub stating User Profile fields (role_id, min_interval_s, min_distance_m, max_silence_10s), pointers (default_user_profile, current_user_profile, previous_user_profile), profile-management plane, active-values plane; defers to product_truth_s03_v1.md as authoritative.
- **ble_snapshot_s04_gate.md:** §3 bullets updated to reference product_truth_s03_v1.md as authoritative WIP, field map and BLE/persistence rules, and packing-at-radio-boundary rule; §5 table “Accepted NodeTable canon” and “Accepted field map” rows updated to point to Product Truth and §1–§6.
- **README.md (wip):** New “S03/V1 Product Truth” section with link to product_truth_s03_v1.md and note that it is authoritative for correction and promotion.

---

## Conflicts found against current canon

- **seq_ref_version_link_metrics_v0 (canon):** Defines and requires **last_core_seq16**, **last_applied_tail_ref_core_seq16** with update rules. Product Truth §7 removes these from canonical S03/V1. Canon packet_to_nodetable_mapping_v0 and tail1_packet_encoding_v0 depend on last_core_seq16 for Tail–Core match; Product Truth does not yet specify replacement semantics for Tail application (promotion will need to resolve).
- **packet_to_nodetable_mapping_v0 (canon):** §3.3 and §3.4 reference last_core_seq16 (stored) and last_applied_tail_ref_core_seq16; §5 persistence notes list them. Product Truth §7 marks these as legacy/removed.
- **identity_naming_persistence_eviction_v0 (canon):** Uses **node_name** for self/remote and authority rule; does not state “single canonical node_name” and “do not split into two data-model fields.” Product Truth §6 is explicit; overwrite/conflict handling is UI/UX only. Minor alignment.
- **role_profiles_policy_v0 (canon):** Uses **minIntervalSec**, **minDisplacementM**, **CurrentRoleId**, **PreviousRoleId**. Product Truth §8 uses **min_interval_s**, **min_distance_m**, **default_user_profile**, **current_user_profile**, **previous_user_profile**. Naming and pointer semantics differ.
- **nodetable field map / master table:** Canon and fields_v0_1.csv (and build_master_table.py) still include ref_core_seq16 (with last_core_seq16 stored) and last_applied_tail_ref_core_seq16; Product Truth §7 removes these from canonical S03/V1. Canon also uses positionLat/positionLon, batteryPercent, uptimeSec, seq16→last_seq; Product Truth uses pos_Lat/pos_Lon, battery_percent, uptime, last_seq (naming alignment may be needed at promotion).
- **Radio profiles (canon/NVS):** Canon and radio_profiles_model_s03 previously used default_profile_id, current_profile_id, previous_profile_id; Product Truth §9 uses default_radio_profile, current_radio_profile, previous_radio_profile. WIP doc updated; canon radio_profiles_policy and NVS key names (e.g. rprof_cur) unchanged.

---

## Downstream issues likely affected

- **#418:** NodeTable persistence/restore — Product Truth defines persisted vs non-persisted (e.g. last_seen_ms not persisted; pos_age_s persisted); legacy fields removed from canonical set may affect what is stored in snapshot.
- **#419:** Self/remote presence and last_seen — Product Truth §3–§4 fixes last_seen_ms (internal, not BLE, not persisted), last_seen_age_s and is_stale (BLE, not persisted); restored entries start stale until fresh contact.
- **#420:** BLE snapshot / export — Product Truth §1–§6 is the source for which fields are BLE vs not; legacy fields must not be added to BLE snapshot.
- **#421:** Profile selection / pointers — Product Truth §8–§9 and profile-management plane define User and Radio profile pointers and CRUD; implementation must use default_user_profile, current_user_profile, previous_user_profile and default_radio_profile, current_radio_profile, previous_radio_profile semantics.
- **#422:** Active profile values in NodeTable — Product Truth §10 (active-values plane) and §2 (tx_power_step, channel_throttle_step, role_id, max_silence_10s from active profiles) affect how NodeTable is populated from profiles.
- **#358:** Packet → NodeTable mapping — Canon mapping still references last_core_seq16 and last_applied_tail_ref_core_seq16; promotion will need to align with Product Truth §7 (legacy removal) and any new Tail-apply semantics.
- **#367, #368, #371, #372, #375, #376:** Execution issues referenced in S03 slice; Product Truth does not change their scope but clarifies BLE/persistence and field set for persistence and snapshot.

---

## Promotion readiness

**Ready to promote after review** (after second WIP pass).

Second pass resolved the Tail/Core blocker: §1 defines the Position block as a single normalized set; §1 and §7 state that NodeTable does not store Tail/Core ref-state; any correlation is runtime-local only. Uptime → **uptime_10m**. Remaining conflicts are against canon only; they do not block promotion. but canon and encoding contracts (tail1_packet_encoding_v0, beacon_payload_encoding_v0 RX rule) still define Tail–Core matching on last_core_seq16. A second WIP pass should either (1) document the intended replacement semantics for Tail application without these fields (e.g. in-product truth or a dedicated WIP note) so promotion does not leave a gap, or (2) explicitly mark “legacy removal” as post-S03 migration and keep current Tail–Core match behaviour in canon until a follow-up. Until that is decided and reflected in WIP, promotion would create unresolved conflict between Product Truth and canon encoding/packet-mapping docs.

---

## Exit criteria

- [x] WIP reflects the Product Truth from this chat
- [x] Conflicting canon areas are listed
- [x] Affected issues are identified
- [x] Promotion readiness is stated
- [x] Tail/Core legacy blocker resolved in WIP (second pass)
- [x] uptime_10m naming normalized in Product Truth WIP

---

## Second pass (Tail/Core blocker + uptime_10m)

- **product_truth_s03_v1.md:** §1 Position defined as single normalized block (pos_Lat, pos_Lon, pos_fix_type, pos_sats, pos_accuracy_bucket, pos_flags_small, pos_age_s); explicit statement that transport may use separate packet parts but NodeTable does not store Tail/Core ref-state; any correlation logic is runtime-local only. §7 strengthened: promotion does not depend on keeping legacy ref fields; §7 expanded with "Tail/Core correlation without canonical ref fields" (runtime-local decoder only; NodeTable stores only normalized Position block). **uptime** → **uptime_10m** in §1 and §2.
- **gnss_tail_completeness_and_budgets_s03.md:** §5 "Core/Tail pairing" rewritten: ref matching is runtime-local decoder logic only; canonical NodeTable does not store last_core_seq16/last_applied_tail_ref_core_seq16; link to product_truth_s03_v1 §1, §7.
