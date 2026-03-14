# #438 — Post-merge plan (final protocol cutover)

## After PR merge

1. **Close #438** — with a short comment: v0.2-only RX/TX cutover complete; temporary v0.1 RX compatibility removed; decode_header and BeaconLogic accept only 0x02, 0x06, 0x07; apply_tail1/2/info and last_applied_tail_ref* removed; tests and docs updated.

2. **Canon state** — packet_migration_v01_v02.md and packet_sets_v0.md already state cutover complete; no further doc change required.

3. **Quality gates met**
   - No v0.1 RX acceptance after this issue.
   - No compat logic left; NodeTable canonical mapping unchanged.
   - test_beacon_logic, test_geo_beacon_codec, test_node_table_domain pass; devkit_e22_oled_gnss build SUCCESS.

## Final close recommendation

#438 is **ready to close** once the PR is merged. Protocol truth is v0.2-only (TX and RX). No follow-up code change required for this issue.
