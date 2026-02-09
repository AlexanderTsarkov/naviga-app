# Connect Screen — State Machine Spec (“WANTED BEHAVIOR”)
“Source: Issue #112”
“Status: Canonical / WIP”
“Applies to: connect_controller.dart / connect_screen.dart”

> Goal: single source of truth for Connect screen behavior (scan/pause/connect/disconnect/switch), robust to BLE late/out-of-order events and device quirks.

Applies to:
- `app/lib/features/connect/connect_controller.dart`
- `app/lib/features/connect/connect_screen.dart`

---

## 1) Core model: Desired vs Observed

### Desired (Intent)
What UI wants, independent of BLE reality:
- `desired = ScanLoopOn`
- `desired = Connect(targetId)`   // user wants to end up connected to targetId

### Observed (Facts from external system)
What BLE stack reports (may be delayed, reordered, or duplicated):
- `obsConn = { state: Disconnected|Connecting|Connected|Disconnecting, deviceId? }`
- `obsScan = { isScanning: true|false }`

### Operation serialization
We also track an internal operation flag:
- `op = none | connecting(targetId) | disconnecting(prevId)`
Rule: while `op != none` → scan loop OFF (see invariants).

### Late-event protection (mandatory)
All async callbacks must be ignored unless they are relevant to the **current operation epoch**.
- `epoch` increments on every user action that changes intent (connect/switch/disconnect).
- Handlers accept events only if:
  - `eventEpoch == epoch` AND (deviceId matches current target/connected device)
  - Otherwise: log as stale/foreign and ignore.

This is not an optimization. This is required to survive late/out-of-order BLE events.

---

## 2) Readiness + Visibility policy (when scan is allowed)

Define derived boolean:

`scanAllowed = onConnectScreen && appForeground && ready && (connectedId == null) && (op == none)`

Where:
- `onConnectScreen`: Connect screen visible
- `appForeground`: lifecycle resumed/foreground
- `ready`: BT + Location Service + Location Permission are OK (current project definition)
- `connectedId`: factually connected device id (set only on BLE connected event)
- `op`: current serialized connect/disconnect operation

### Policy
- If `scanAllowed == true` → `desired = ScanLoopOn`
- Else if `connectedId != null` or user initiated connect → `desired = Connect(targetId)` (or maintain current connect intent)
- Else → `desired = ScanLoopOff` (not a visible state; just means scan loop must be stopped)

---

## 3) States (UI-level)
UI state is derived from observed + internal op:
- `Idle` (no scanning, scan loop not requested)
- `Scanning` (BLE scanning true)
- `Paused` (scan loop requested but currently in pause window; **must have pause timer active**)
- `Connecting(targetId)`
- `Connected(connectedId)`
- `Disconnecting(prevId)`
- `Error(lastError)` (optional; can be represented as failed status)

Important: `Connected` is a fact, not an intention. It only happens after BLE confirms connection.

---

## 4) Invariants (must always hold)

**I0 — Scan loop gating**
- If `scanAllowed == false` → scan loop OFF: `scanRequested=false`, no pause timer, no watchdog, BLE stopScan best-effort.
- If `scanAllowed == true` → scan loop ON: within ≤1s system is either:
  - `Scanning` OR
  - `Paused` with active pause timer
  Never dead `Idle` for long.

**I1 — No scan during connection operations**
If `op != none` (connecting/disconnecting):
- scan loop OFF (no scanRequested, no pause timer)

**I2 — “Paused implies timer”**
`Paused` is valid iff pause timer is active.
If `scanRequested==true && isScanning==false && pauseTimerInactive` → controller must self-heal within ≤1s:
- either schedule pause timer immediately OR restart scan window.

**I3 — Connected id is factual**
- `connectedId` is set **only** on `BLE_CONN_CONNECTED(deviceId)` accepted by epoch/device match.
- `connectedId` is cleared on `BLE_CONN_DISCONNECTED(connectedId)` accepted by epoch/device match,
  or via disconnect timeout fallback.

**I4 — Switch is serialized**
Switch A→B is never “connect B while still connected to A”.
- Always: disconnect A → then connect B (with timeouts + late-event handling).

**I5 — External events must be relevant**
Any BLE event that does not match current epoch + relevant deviceId must not mutate state.

---

## 5) Events

### UI / App events
- `EV_SCREEN_ENTER`
- `EV_SCREEN_EXIT`
- `EV_APP_FOREGROUND` / `EV_APP_BACKGROUND`
- `EV_READY_CHANGED(ready)`
- `EV_USER_CONNECT(targetId)`
- `EV_USER_DISCONNECT`
- `EV_USER_SWITCH_REQUEST(targetId)` (UX confirm is handled in Issue #114, but controller flow defined here)

### BLE events
- `EV_BLE_CONNECTED(deviceId)`
- `EV_BLE_DISCONNECTED(deviceId, reason)`
- `EV_BLE_SCAN_STARTED`
- `EV_BLE_SCAN_STOPPED`
- `EV_BLE_ERROR(op, deviceId?, error)`

### Timers
- `EV_SCAN_PAUSE_TIMEOUT`
- `EV_CONNECT_TIMEOUT`
- `EV_DISCONNECT_TIMEOUT`

---

## 6) Transition table (high-level)

Notation:
- `A -> B` = state transition
- Actions are controller actions (startScan, stopScan, connect, disconnect, schedule timers)
- All BLE event handlers must do epoch/device relevance check first.

### A) Scan loop states (no connection, no op)
**From any state** on `scanAllowed=false`:
- Action: stop scan loop + cancel timers
- Next: `Idle` (or remain in non-scan state)

**Idle/Scanning/Paused** on `scanAllowed=true`:
- Action: ensure scan loop requested; start scan window if needed
- Next: `Scanning` or `Paused` (but Paused must have timer)

**Scanning** on `EV_BLE_SCAN_STOPPED` and scanAllowed=true:
- Action: schedule pause timer
- Next: `Paused`

**Paused** on `EV_SCAN_PAUSE_TIMEOUT` and scanAllowed=true:
- Action: start scan window
- Next: wait for `EV_BLE_SCAN_STARTED` → `Scanning`

**(Any scan state)** on `EV_USER_CONNECT(X)`:
- Action: epoch++, set `targetId=X`, stop scan loop, set `op=connecting(X)`, initiate BLE connect
- Next: `Connecting(X)`

### B) Connecting
**Connecting(X)** on `EV_BLE_CONNECTED(X)`:
- Action: set `connectedId=X`, `op=none`, ensure scan loop OFF
- Next: `Connected(X)`

**Connecting(X)** on `EV_BLE_DISCONNECTED(X)` or `EV_BLE_ERROR(connect, X)`:
- Action: clear `connectedId=null`, `op=none`, clear target intent unless user set another, reconcile (scanAllowed? start scan)
- Next: `Idle/Scanning/Paused` depending on scanAllowed

**Connecting(X)** on `EV_USER_CONNECT(Y)` (user changes mind quickly):
- Action: epoch++, set `targetId=Y`, cancel/ignore old connect via epoch, initiate connect(Y)
- Next: `Connecting(Y)`

**Connecting(X)** on `EV_USER_DISCONNECT`:
- Action: epoch++, initiate disconnect best-effort (may be no-op), set `op=disconnecting(X)`
- Next: `Disconnecting(X)`

**Connecting(X)** on `EV_CONNECT_TIMEOUT`:
- Action: locally fail op; clear local truth; set `op=none`; reconcile scanAllowed
- Next: scan loop state if allowed

### C) Connected
**Connected(A)** on `EV_USER_DISCONNECT`:
- Action: epoch++, set `op=disconnecting(A)`, initiate BLE disconnect(A)
- Next: `Disconnecting(A)`

**Connected(A)** on `EV_USER_SWITCH_REQUEST(B)`:
- Action: epoch++, set `targetId=B`, set `op=disconnecting(A)`, initiate BLE disconnect(A)
- Next: `Disconnecting(A)` (then connect B after A disconnected/timeout)

**Connected(A)** on `EV_BLE_DISCONNECTED(A)`:
- Action: clear `connectedId=null`, `op=none`
- If `targetId != null` (switch intent): initiate connect(targetId) → `Connecting(targetId)`
- Else: reconcile scanAllowed → scan loop states

### D) Disconnecting
**Disconnecting(A)** on `EV_BLE_DISCONNECTED(A)`:
- Action: clear local connection truth; `op=none`
- If `targetId != null` (switch): initiate connect(targetId) → `Connecting(targetId)`
- Else: reconcile scanAllowed → scan loop states

**Disconnecting(A)** on `EV_DISCONNECT_TIMEOUT`:
- Action: locally consider disconnected (clear local truth), `op=none`
- Then same as above (switch? connect B : scan loop)

---

## 7) Timeouts (recommended)
- `DISCONNECT_TIMEOUT`: 2–4s (Android stacks may delay; use best-effort)
- `CONNECT_TIMEOUT`: 8–12s
- Scan windows: existing project values (e.g., 3s scan window + pause)

Timeout handling rule:
- Timeout ends local operation and triggers reconciliation.
- Any later BLE event from previous epoch/device must be ignored.

---

## 8) Manual scenario checklist (acceptance for implementation)

1) Open Connect screen, ready=true, no connection → scan starts automatically (or pause-with-timer then scan).
2) Leave Connect screen → scan stops and timers are cancelled.
3) App goes background while scanning → scan stops; resume → readiness refresh; scan restarts if allowed.
4) Connect to device X → scan stops during connecting; becomes Connected only after BLE confirms.
5) Disconnect from X → within ≤1s scan restarts if allowed.
6) Repeat connect/disconnect X 10 times (Samsung S8) → never stuck Idle/Paused.
7) Switch A→B (after user confirmation) → disconnect A, then connect B; no parallel scanning during ops.
8) Switch request during Connecting(A) to B → old A events ignored; final state matches B or scan loop if fails.
9) Late BLE disconnected/connected events from previous attempt do not override current state (validated by logs).
10) Readiness toggles (BT/LS/LP) while on Connect screen: when ready becomes false, scan stops; when becomes true, scan restarts if allowed.
11) “Paused without timer” never occurs; if detected, controller self-heals within ≤1s.

---

## 9) Logging expectations (for debugging)
Controller should log:
- epoch, op, targetId, connectedId
- scanAllowed, scanRequested, isScanning, pauseTimerActive
- every rejected BLE event: reason = stale epoch or deviceId mismatch# Connect Screen — State Machine Spec (“WANTED BEHAVIOR”)

> Goal: single source of truth for Connect screen behavior (scan/pause/connect/disconnect/switch), robust to BLE late/out-of-order events and device quirks.

Applies to:
- `app/lib/features/connect/connect_controller.dart`
- `app/lib/features/connect/connect_screen.dart`

---

## 1) Core model: Desired vs Observed

### Desired (Intent)
What UI wants, independent of BLE reality:
- `desired = ScanLoopOn`
- `desired = Connect(targetId)`   // user wants to end up connected to targetId

### Observed (Facts from external system)
What BLE stack reports (may be delayed, reordered, or duplicated):
- `obsConn = { state: Disconnected|Connecting|Connected|Disconnecting, deviceId? }`
- `obsScan = { isScanning: true|false }`

### Operation serialization
We also track an internal operation flag:
- `op = none | connecting(targetId) | disconnecting(prevId)`
Rule: while `op != none` → scan loop OFF (see invariants).

### Late-event protection (mandatory)
All async callbacks must be ignored unless they are relevant to the **current operation epoch**.
- `epoch` increments on every user action that changes intent (connect/switch/disconnect).
- Handlers accept events only if:
  - `eventEpoch == epoch` AND (deviceId matches current target/connected device)
  - Otherwise: log as stale/foreign and ignore.

This is not an optimization. This is required to survive late/out-of-order BLE events.

---

## 2) Readiness + Visibility policy (when scan is allowed)

Define derived boolean:

`scanAllowed = onConnectScreen && appForeground && ready && (connectedId == null) && (op == none)`

Where:
- `onConnectScreen`: Connect screen visible
- `appForeground`: lifecycle resumed/foreground
- `ready`: BT + Location Service + Location Permission are OK (current project definition)
- `connectedId`: factually connected device id (set only on BLE connected event)
- `op`: current serialized connect/disconnect operation

### Policy
- If `scanAllowed == true` → `desired = ScanLoopOn`
- Else if `connectedId != null` or user initiated connect → `desired = Connect(targetId)` (or maintain current connect intent)
- Else → `desired = ScanLoopOff` (not a visible state; just means scan loop must be stopped)

---

## 3) States (UI-level)
UI state is derived from observed + internal op:
- `Idle` (no scanning, scan loop not requested)
- `Scanning` (BLE scanning true)
- `Paused` (scan loop requested but currently in pause window; **must have pause timer active**)
- `Connecting(targetId)`
- `Connected(connectedId)`
- `Disconnecting(prevId)`
- `Error(lastError)` (optional; can be represented as failed status)

Important: `Connected` is a fact, not an intention. It only happens after BLE confirms connection.

---

## 4) Invariants (must always hold)

**I0 — Scan loop gating**
- If `scanAllowed == false` → scan loop OFF: `scanRequested=false`, no pause timer, no watchdog, BLE stopScan best-effort.
- If `scanAllowed == true` → scan loop ON: within ≤1s system is either:
  - `Scanning` OR
  - `Paused` with active pause timer
  Never dead `Idle` for long.

**I1 — No scan during connection operations**
If `op != none` (connecting/disconnecting):
- scan loop OFF (no scanRequested, no pause timer)

**I2 — “Paused implies timer”**
`Paused` is valid iff pause timer is active.
If `scanRequested==true && isScanning==false && pauseTimerInactive` → controller must self-heal within ≤1s:
- either schedule pause timer immediately OR restart scan window.

**I3 — Connected id is factual**
- `connectedId` is set **only** on `BLE_CONN_CONNECTED(deviceId)` accepted by epoch/device match.
- `connectedId` is cleared on `BLE_CONN_DISCONNECTED(connectedId)` accepted by epoch/device match,
  or via disconnect timeout fallback.

**I4 — Switch is serialized**
Switch A→B is never “connect B while still connected to A”.
- Always: disconnect A → then connect B (with timeouts + late-event handling).

**I5 — External events must be relevant**
Any BLE event that does not match current epoch + relevant deviceId must not mutate state.

---

## 5) Events

### UI / App events
- `EV_SCREEN_ENTER`
- `EV_SCREEN_EXIT`
- `EV_APP_FOREGROUND` / `EV_APP_BACKGROUND`
- `EV_READY_CHANGED(ready)`
- `EV_USER_CONNECT(targetId)`
- `EV_USER_DISCONNECT`
- `EV_USER_SWITCH_REQUEST(targetId)` (UX confirm is handled in Issue #114, but controller flow defined here)

### BLE events
- `EV_BLE_CONNECTED(deviceId)`
- `EV_BLE_DISCONNECTED(deviceId, reason)`
- `EV_BLE_SCAN_STARTED`
- `EV_BLE_SCAN_STOPPED`
- `EV_BLE_ERROR(op, deviceId?, error)`

### Timers
- `EV_SCAN_PAUSE_TIMEOUT`
- `EV_CONNECT_TIMEOUT`
- `EV_DISCONNECT_TIMEOUT`

---

## 6) Transition table (high-level)

Notation:
- `A -> B` = state transition
- Actions are controller actions (startScan, stopScan, connect, disconnect, schedule timers)
- All BLE event handlers must do epoch/device relevance check first.

### A) Scan loop states (no connection, no op)
**From any state** on `scanAllowed=false`:
- Action: stop scan loop + cancel timers
- Next: `Idle` (or remain in non-scan state)

**Idle/Scanning/Paused** on `scanAllowed=true`:
- Action: ensure scan loop requested; start scan window if needed
- Next: `Scanning` or `Paused` (but Paused must have timer)

**Scanning** on `EV_BLE_SCAN_STOPPED` and scanAllowed=true:
- Action: schedule pause timer
- Next: `Paused`

**Paused** on `EV_SCAN_PAUSE_TIMEOUT` and scanAllowed=true:
- Action: start scan window
- Next: wait for `EV_BLE_SCAN_STARTED` → `Scanning`

**(Any scan state)** on `EV_USER_CONNECT(X)`:
- Action: epoch++, set `targetId=X`, stop scan loop, set `op=connecting(X)`, initiate BLE connect
- Next: `Connecting(X)`

### B) Connecting
**Connecting(X)** on `EV_BLE_CONNECTED(X)`:
- Action: set `connectedId=X`, `op=none`, ensure scan loop OFF
- Next: `Connected(X)`

**Connecting(X)** on `EV_BLE_DISCONNECTED(X)` or `EV_BLE_ERROR(connect, X)`:
- Action: clear `connectedId=null`, `op=none`, clear target intent unless user set another, reconcile (scanAllowed? start scan)
- Next: `Idle/Scanning/Paused` depending on scanAllowed

**Connecting(X)** on `EV_USER_CONNECT(Y)` (user changes mind quickly):
- Action: epoch++, set `targetId=Y`, cancel/ignore old connect via epoch, initiate connect(Y)
- Next: `Connecting(Y)`

**Connecting(X)** on `EV_USER_DISCONNECT`:
- Action: epoch++, initiate disconnect best-effort (may be no-op), set `op=disconnecting(X)`
- Next: `Disconnecting(X)`

**Connecting(X)** on `EV_CONNECT_TIMEOUT`:
- Action: locally fail op; clear local truth; set `op=none`; reconcile scanAllowed
- Next: scan loop state if allowed

### C) Connected
**Connected(A)** on `EV_USER_DISCONNECT`:
- Action: epoch++, set `op=disconnecting(A)`, initiate BLE disconnect(A)
- Next: `Disconnecting(A)`

**Connected(A)** on `EV_USER_SWITCH_REQUEST(B)`:
- Action: epoch++, set `targetId=B`, set `op=disconnecting(A)`, initiate BLE disconnect(A)
- Next: `Disconnecting(A)` (then connect B after A disconnected/timeout)

**Connected(A)** on `EV_BLE_DISCONNECTED(A)`:
- Action: clear `connectedId=null`, `op=none`
- If `targetId != null` (switch intent): initiate connect(targetId) → `Connecting(targetId)`
- Else: reconcile scanAllowed → scan loop states

### D) Disconnecting
**Disconnecting(A)** on `EV_BLE_DISCONNECTED(A)`:
- Action: clear local connection truth; `op=none`
- If `targetId != null` (switch): initiate connect(targetId) → `Connecting(targetId)`
- Else: reconcile scanAllowed → scan loop states

**Disconnecting(A)** on `EV_DISCONNECT_TIMEOUT`:
- Action: locally consider disconnected (clear local truth), `op=none`
- Then same as above (switch? connect B : scan loop)

---

## 7) Timeouts (recommended)
- `DISCONNECT_TIMEOUT`: 2–4s (Android stacks may delay; use best-effort)
- `CONNECT_TIMEOUT`: 8–12s
- Scan windows: existing project values (e.g., 3s scan window + pause)

Timeout handling rule:
- Timeout ends local operation and triggers reconciliation.
- Any later BLE event from previous epoch/device must be ignored.

---

## 8) Manual scenario checklist (acceptance for implementation)

1) Open Connect screen, ready=true, no connection → scan starts automatically (or pause-with-timer then scan).
2) Leave Connect screen → scan stops and timers are cancelled.
3) App goes background while scanning → scan stops; resume → readiness refresh; scan restarts if allowed.
4) Connect to device X → scan stops during connecting; becomes Connected only after BLE confirms.
5) Disconnect from X → within ≤1s scan restarts if allowed.
6) Repeat connect/disconnect X 10 times (Samsung S8) → never stuck Idle/Paused.
7) Switch A→B (after user confirmation) → disconnect A, then connect B; no parallel scanning during ops.
8) Switch request during Connecting(A) to B → old A events ignored; final state matches B or scan loop if fails.
9) Late BLE disconnected/connected events from previous attempt do not override current state (validated by logs).
10) Readiness toggles (BT/LS/LP) while on Connect screen: when ready becomes false, scan stops; when becomes true, scan restarts if allowed.
11) “Paused without timer” never occurs; if detected, controller self-heals within ≤1s.

---

## 9) Logging expectations (for debugging)
Controller should log:
- epoch, op, targetId, connectedId
- scanAllowed, scanRequested, isScanning, pauseTimerActive
- every rejected BLE event: reason = stale epoch or deviceId mismatch