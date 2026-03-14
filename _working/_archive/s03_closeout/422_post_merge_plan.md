# #422 — Post-merge plan

## After PR merge

1. **Close #422** — with a short comment that it was completed via Path B (throttle/merge), validation 73/73 + devkit, and that the P0 chain is complete.

2. **Update #416** — add a short completion note for the P0 chain, e.g.:
   - "P0 chain (#417–#422) complete. #417 seq16 persistence; #418 NodeTable snapshot/restore; #419 NodeTable canonical field map; #420 TX formation v0.1 lock; #421 RX/apply canon alignment; #422 packetization Path B (throttle/merge, no hitchhiking, status lifecycle)."

3. **Master summary (what remains after P0)** — brief note, e.g. in a comment on #416 or in _working:
   - P0 done. Post-P0: v0.2 redesign (Node_Pos_Full, Node_Status) is out of scope for #422; Pos_Quality 2-byte layout, role_id on Informative wire, and any further packetization work are follow-ups. No new work started in this packaging pass.
