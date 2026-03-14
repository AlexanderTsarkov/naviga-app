**PR hygiene:** #439 included already-merged changes (Files changed had 23 files from #435/#436/#437). A clean **#438-only** PR was created from current `main` with a single cherry-picked cutover commit.

- **Use this PR:** [#440 — feat(#438): remove v0.1 RX compatibility and finalize v0.2-only cutover](https://github.com/AlexanderTsarkov/naviga-app/pull/440)  
  - **Files changed:** 11 files (docs, packet_header, beacon_logic, node_table, nodetable_snapshot, tests).  
  - Safe to review/merge as the #438 cutover.
- #439 is superseded by #440 for the #438 cutover; can be closed in favor of #440.
