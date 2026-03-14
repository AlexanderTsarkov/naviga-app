# Canon promotion — Product Truth S03/V1 report

**Scope:** Docs-only canon correction pass. Product Truth from WIP is primary; canon updated to match. No code changes. No implementation PR (#429/#430) changes.

---

## Canon files updated

- `docs/product/areas/nodetable/policy/seq_ref_version_link_metrics_v0.md`
- `docs/product/areas/nodetable/policy/packet_to_nodetable_mapping_v0.md`
- `docs/product/areas/nodetable/policy/nodetable_field_map_v0.md`
- `docs/product/areas/nodetable/policy/identity_naming_persistence_eviction_v0.md`
- `docs/product/areas/nodetable/policy/presence_and_age_semantics_v0.md`
- `docs/product/areas/nodetable/policy/s03_nodetable_slice_v0.md`
- `docs/product/areas/domain/policy/role_profiles_policy_v0.md`
- `docs/product/areas/radio/policy/radio_profiles_policy_v0.md`
- `docs/product/areas/nodetable/contract/tail1_packet_encoding_v0.md`
- `docs/product/areas/firmware/policy/provisioning_interface_v0.md`

---

## What was promoted

- **Seq/link metrics:** last_seq, last_rx_rssi, snr_last, last_payload_version_seen (optional debug only). Legacy ref fields (last_core_seq16, last_applied_tail_ref_core_seq16) removed from canonical NodeTable; Tail–Core correlation defined as runtime-local decoder logic only. Reference to Product Truth §7 added.
- **Packet → NodeTable mapping:** Position block normalized (pos_Lat, pos_Lon, pos_fix_type, pos_sats, pos_accuracy_bucket, pos_flags_small, pos_age_s). pos_age_s described as **node-owned canonical position freshness**; persisted. Core_Pos/Tail mapping: no NodeTable ref-state fields; Tail apply uses runtime-local correlation only. Operational: battery_percent, **uptime_10m** (canonical name). Informative: max_silence_10s, hw_profile_id, fw_version_id. Persistence §5: legacy ref fields removed; pos_age_s and Product Truth field set reflected.
- **Field map:** Policy ref for seq/link metrics updated (no ref_core_seq16/last_applied in canon). §6: legacy Tail/Core ref fields listed as out-of-scope for canonical NodeTable; link to Product Truth §7.
- **Identity/naming:** node_name defined as **single canonical field** covering self and remote; do not split into two data-model fields; overwrite/conflict = UI/UX policy only.
- **Presence/age:** pos_age_s defined as **node-owned canonical position freshness field**. Core_Tail presence bullet rephrased so it does not imply NodeTable stores ref-state.
- **S03 slice:** Product Truth added as source of truth; legacy Tail/Core ref fields listed in out-of-scope; snr_last canonical name in in-scope bullet.
- **Role profiles:** Canonical pointer names **default_user_profile, current_user_profile, previous_user_profile**. User Profile content: **role_id**, **min_interval_s**, **min_distance_m**, **max_silence_10s**. Profile-management plane: list, read, create, update, delete, set current; **FW Default cannot be deleted**. Active-values plane stated.
- **Radio profiles:** Radio Profile content: **channel_slot**, **rate_tier**, **tx_power_baseline_step**. Canonical pointer names **default_radio_profile, current_radio_profile, previous_radio_profile**. Profile-management plane: list, read, create, update, delete, set current; **FW Default cannot be deleted**. Active-values plane stated.
- **Tail1 encoding:** ref_core_seq16 in packet is transport/decoder-only; canonical NodeTable does not store last_core_seq16 or last_applied_tail_ref_core_seq16. §4.1 RX rules: correlation is runtime-local decoder logic; apply updates normalized Position block only; no ref-state in NodeTable. Example §6.3 updated.
- **Provisioning:** Canonical pointer names (current_user_profile, current_radio_profile, etc.) added; policy source table unchanged.

---

## Legacy canon assumptions removed

- **last_core_seq16** and **last_applied_tail_ref_core_seq16** (and has_core_seq16, has_applied_tail_ref_core_seq16) as canonical NodeTable fields — removed from seq_ref_version_link_metrics_v0, packet_to_nodetable_mapping_v0, nodetable_field_map_v0, s03_nodetable_slice_v0, tail1_packet_encoding_v0. Stated as runtime-local decoder only where Tail–Core correlation is described.
- **CurrentRoleId / PreviousRoleId** as sole product names — canon now uses **current_user_profile / previous_user_profile** (and default_user_profile) as canonical; storage names noted as alternate.
- **CurrentProfileId / PreviousProfileId** as sole product names — canon now uses **current_radio_profile / previous_radio_profile** (and default_radio_profile) as canonical.
- **uptimeSec** as canonical uptime field — replaced by **uptime_10m** in packet_to_nodetable_mapping and persistence notes.
- **Ref_core_seq16 must match lastCoreSeq** stored in NodeTable — replaced by “decoder may use runtime-local correlation state; not NodeTable fields” in tail1 and packet mapping.
- Persistence notes that listed ref_core_seq16/last_core_seq16 as persisted — removed; §5 in packet_to_nodetable_mapping now aligns with Product Truth (pos_age_s persisted; legacy ref not).

---

## Remaining canon conflicts

**None blocking corrected canon.** All targeted canon docs now match Product Truth. The master table (fields_v0_1.csv, build_master_table.py) remains in WIP; when the generator is updated to drop legacy ref rows and align field keys with Product Truth (e.g. uptime_10m, pos_age_s as node-owned), the machine-readable source will match. That is a follow-on generator/CSV change, not a canon-policy conflict.

---

## Follow-on implementation issues affected

- **#418** — NodeTable persistence/restore: canon now defines persisted set (pos_age_s, no legacy ref fields); snapshot/restore must follow Product Truth.
- **#419** — Self/remote presence: last_seen_ms internal, last_seen_age_s/is_stale derived; pos_age_s node-owned canonical; restored entries start stale.
- **#420** — BLE snapshot: field set and BLE/persisted flags per Product Truth; no legacy ref fields in BLE.
- **#421** — Profile selection: implementation should use current_user_profile, current_radio_profile (and defaults); profile-management plane (list/read/create/update/delete/set current, FW Default cannot be deleted) is canon-defined.
- **#422** — Active profile values in NodeTable: runtime/NodeTable use active profile values (tx_power_step, role_id, max_silence_10s, etc.), not profile objects; canon reflects this.
- **#358** — Packet → NodeTable mapping: canon updated; impl must not persist or expose legacy ref fields as product NodeTable.
- **#367, #368, #371, #372, #375, #376** — Scope unchanged; canon now aligns with Product Truth for field set and persistence.

---

## Promotion result

**Canon corrected and ready for issue realignment.**

All listed canon docs were updated so that (1) Product Truth is primary, (2) legacy Tail/Core ref fields are removed from canonical NodeTable truth, (3) canonical naming is normalized (uptime_10m, pos_age_s as node-owned, default/current/previous user and radio profile names), (4) profile truths and profile-management plane are canon-defined for S04 readiness, and (5) no wording re-legitimizes legacy NodeTable ref fields. Remaining work is generator/master-table alignment and implementation issue updates; canon is in a state to drive that realignment.

---

## Exit criteria

- [x] Canon updated to match Product Truth
- [x] Legacy Tail/Core ref fields removed from canonical NodeTable truth
- [x] Canonical naming normalized (uptime_10m, profile pointers, pos_age_s as node-owned)
- [x] Profile truths and profile-management plane promoted
- [x] Implementation issues affected are identified
