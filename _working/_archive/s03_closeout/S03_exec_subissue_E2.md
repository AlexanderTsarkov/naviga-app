## Goal

Update **current_state** (and optionally spec_map) when each major capability lands: persistence, NodeTable conformance, packetization, boot/OOTB, observability. Keep [current_state.md](docs/product/current_state.md) accurate so "What changed this iteration" and "Next focus" reflect reality.

## Definition of done

- After merging PRs for a major bucket (A–E): add or update row in "What changed this iteration" with links to PR/issue.
- "Next focus" and "Firmware" / "Domain" / "Radio" sections updated if behavior or contracts changed.
- No normative canon changes; only reflection of implementation state.

## Touched paths

- `docs/product/current_state.md`; optionally `docs/product/wip/spec_map_v0.md` if spec_map tracks implementation status.

## Tests

- N/A (docs). Checklist: each sub-issue closure in this umbrella has a corresponding current_state update before umbrella close.

## Parent

- Umbrella: S03 Execution planning (implement canon slice).
- Bucket: E — Observability & sanity (P2).
