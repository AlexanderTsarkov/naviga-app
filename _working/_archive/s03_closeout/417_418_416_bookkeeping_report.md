# GitHub issue bookkeeping report — #417, #418, #416

## #417

- **Comment posted:** yes
- **Checkboxes/status synced:** not applicable (issue body has no checkboxes)
- **What was updated:** Posted final implementation comment summarizing: implemented via PR #428; narrow NVS adapter; restore > 0 => next TX uses restored + 1; per-successful-TX persistence; wraparound seq16 == 0 handled; tests/validation and merge reference.

## #418

- **Comment posted:** yes
- **Checkboxes/status synced:** not applicable (issue body has no checkboxes)
- **What was updated:** Posted final implementation comment summarizing: implemented via PR #429; dedicated NodeTable snapshot format (37-byte, v2); narrow persisted subset; derived/prohibited policy; last_seen_ms not persisted, restore sets 0; v2 format, v1 rejected; bounded write cadence; tests/validation and merge reference.

## #416

- **Comment posted:** yes
- **Checkboxes/status synced:** not applicable (umbrella body has scope buckets table and sub-issue list; no per-issue checkboxes for #417/#418 to sync)
- **What was updated:** Posted one concise progress comment: #417 done via PR #428 (seq16 persistence + restore + per-TX + wraparound); #418 done via PR #429 (NodeTable snapshot/restore, narrow subset, last_seen_ms not persisted, v2, bounded write); persistence/restore foundation in place; remaining work in open sub-issues (e.g. #419, #420, #421, #422).

## Notes

- #417 and #418 are already CLOSED; no state change. Neither issue had markdown checkboxes in the body, so no checkbox sync was needed.
- #416 remains OPEN; the new comment is additive and does not modify the issue body. The single existing comment was the legacy audit report; the new comment is the first implementation-progress update.
- Comment wording matches merged behavior (PR #428 and PR #429) and stays concise and factual.
