# Naviga — debug playbook

This is the fastest path from "it's broken" to "we know why".

## First triage (always)

1. Identify the symptom category:
   - **(A)** build/compile fails
   - **(B)** unit tests fail
   - **(C)** native E2E integration fails
   - **(D)** device/bench behavior wrong
   - **(E)** CI-only failure
2. Find the *last known good*:
   - main is expected to be green
   - check the last merged PR and its scope
3. Reduce scope:
   - reproduce with the smallest possible test or bench scenario

## A) Build / compile failures

**Checklist:**

- ensure toolchain / platform packages match repo expectations
- verify include paths and compile flags
- confirm no accidental dependency cycle
- if it's ESP32-specific: validate sdkconfig / platformio config (if used)

**Fast tactic:**

- revert/disable the last change locally and bisect by commit if needed

## B) Unit test failures (native)

**Checklist:**

- confirm test intent vs implementation changed
- check time-dependent logic (jitter/timers)
- check serialization/codec expectations
- validate mocks: they should match interface invariants

**Tactic:**

- run the failing suite alone
- log inputs/outputs at the boundary (codec, NodeTable events)

## C) Native E2E integration failures

**Checklist:**

- wiring mismatches (M1Runtime composition)
- transport boundary expectations changed
- radio adapter contract changed (payload vs RSSI parsing)
- ordering/timing (race) issues

**Tactic:**

- add minimal structured logging at boundaries:
  - radio receive event → domain ingest
  - domain state update → BLE publish

## D) Bench / device behavior wrong

**Start with observability:**

- enable logging v0 / key markers
- confirm E220 module config matches expectations (RSSI append etc.)
- verify power / wiring / UART pins
- confirm radio params are consistent across devices (channel, address, rate)

**Common culprits:**

- mismatched E220 configuration between nodes
- payload parsing broken by RSSI append handling
- timing/jitter too aggressive (messages collide)
- buffer sizing / truncation

**Tactic:**

- test in a "clean room" scenario:
  - 2 devices, fixed distance, no other changes
  - send single message type at low frequency

## E) CI-only failures

**Checklist:**

- test is flaky (timing)
- depends on ordering / uninitialized state
- platform differences (compiler version, sanitizer)
- missing files in build scripts

**Tactic:**

- rerun the job with more logs
- make the test deterministic:
  - fixed seeds
  - avoid real sleeps; use fake time if possible

## Golden rules

- Fix the root cause, then add/adjust a test so it can't come back silently.
- Avoid "debug by redesign".
- Keep PRs small: if debug fix requires refactor, split into:
  1. refactor-only PR (no behavior change)
  2. bugfix PR (minimal delta)
