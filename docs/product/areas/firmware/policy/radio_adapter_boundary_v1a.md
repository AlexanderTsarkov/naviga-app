# Firmware — Radio adapter boundary v1-A (policy)

**Status:** Canon (policy).  
**Parent audit:** [#229](https://github.com/AlexanderTsarkov/naviga-app/issues/229) · **Epic:** [#224](https://github.com/AlexanderTsarkov/naviga-app/issues/224) · **Umbrella:** [#147](https://github.com/AlexanderTsarkov/naviga-app/issues/147)

This policy fixes the **V1-A** boundary between the radio-agnostic domain/runtime and the radio adapter: the minimal IRadio interface used in V1-A, its relationship to the broader HAL contract, and the design of **radio_ready** / **rssi_available** as init-time parameters. It closes documentation gaps identified in [#259](https://github.com/AlexanderTsarkov/naviga-app/issues/259) and [#260](https://github.com/AlexanderTsarkov/naviga-app/issues/260).

---

## 1) Goal

- Define the **V1-A minimal IRadio contract** and state that it is an **intentional simplification** vs the richer HAL contract described in [hal_contracts_v0](../../../../firmware/hal_contracts_v0.md).
- Define **radio_ready** and **rssi_available** as **wiring-time capability flags** passed at runtime init, not as part of IRadio.
- State **boundaries**: domain is radio-agnostic; the single V1-A adapter (E220Radio) is an IRadio implementation; M1Runtime is the only composition point that receives IRadio + init flags.

---

## 2) Non-goals

- No code or HAL signature changes; no new IRadio methods.
- No protocol/on-air/NodeTable changes; no BLE, mesh/JOIN/CAD/LBT.
- No post-V1A topics: selection policy, autopower, channel discovery ([#175](https://github.com/AlexanderTsarkov/naviga-app/issues/175), [#180](https://github.com/AlexanderTsarkov/naviga-app/issues/180)).

---

## 3) IRadio in V1-A (minimal interface)

For V1-A, the runtime and domain use only a **minimal** radio interface:

| Method / capability | In V1-A IRadio? | Notes |
|--------------------|-----------------|--------|
| **send(payload)**  | Yes             | Returns **bool** (success/failure). No richer result enum in V1-A. |
| **recv(out, max_len, out_len)** | Yes | Poll-based; returns bool and length. |
| **last_rssi_dbm()** | Yes             | RSSI of last received packet (if available). |

**Explicitly out of scope for V1-A IRadio:**

- **RadioTxResult** (OK / BUSY_OR_CCA_FAIL / FAIL) — not in the V1-A interface; `send()` returns bool only.
- **getRadioStatusSnapshot()** (tx_count, rx_count, last_rx_rssi as structured snapshot) — not in V1-A.
- **getRadioCapabilityInfo()** (band_id, channel_min/max, power_min/max, module_model) — not in V1-A.
- **init**, **setPower**, **setFrequency** / **setConfig** — configuration is done at wiring/adapter level before the runtime holds an IRadio pointer; not part of the minimal runtime-facing interface for V1-A.

---

## 4) Relationship to hal_contracts_v0

The document [hal_contracts_v0.md](../../../../firmware/hal_contracts_v0.md) describes a **broader** HAL contract for IRadio: send → **RadioTxResult**, **getRadioStatusSnapshot()**, **getRadioCapabilityInfo()**, and TX diagnostics (tx_attempts, tx_ok, tx_busy, tx_fail). That contract is a **candidate future expansion**, not a promise for V1-A.

- **V1-A:** Implementation uses the minimal interface above (bool send, recv, last_rssi_dbm). Domain and runtime do not depend on RadioTxResult or snapshot/capability methods.
- **Future:** If the codebase later adopts RadioTxResult or getRadioStatusSnapshot/getRadioCapabilityInfo, all adapters (including E220Radio) must map to that contract; such expansion is out of scope for this policy.

---

## 5) radio_ready and rssi_available (init params, not IRadio)

**Design:** The runtime (e.g. M1Runtime) receives two **boolean capability flags** at **init** time:

- **radio_ready** — whether the radio adapter successfully completed bring-up and is ready for send/recv.
- **rssi_available** — whether the adapter can report RSSI for received packets (e.g. last_rssi_dbm() is meaningful).

These are **not** methods on IRadio. They are supplied by the **wiring layer** (e.g. app_services) when it constructs the runtime: the wiring layer instantiates the concrete adapter (e.g. E220Radio), calls adapter-specific init/begin, and then passes **IRadio\*** plus **radio_ready** and **rssi_available** into the runtime’s init.

**Rationale:**

- Keeps IRadio minimal and testable (mocks can set any combination of flags at init).
- Avoids pushing adapter-specific capability queries (e.g. “is_ready()”, “rssi_available()”) into the HAL interface before a second adapter justifies them.
- Single composition point (wiring) knows the concrete adapter; runtime remains radio-agnostic.

**Where they come from / where they are used:**

- **Origin:** Wiring (e.g. app_services) after calling the adapter’s begin() and, if applicable, is_ready() / rssi_available() (or equivalent) on the **concrete** adapter.
- **Consumption:** Runtime init stores the two bools and uses them for behaviour (e.g. skip TX/RX if !radio_ready; use last_rssi_dbm() only when rssi_available).

For V1-A, **no IRadio extension** is required. If a second adapter is added and both need to expose these capabilities, consider optional virtual methods or a small capability struct later; that is out of scope here.

---

## 6) Boundaries (invariants)

- **Domain** is radio-agnostic: no dependency on concrete radio types or E220-specific enums; only IRadio (and optional IChannelSense) from HAL where needed.
- **E220Radio** is the single IRadio adapter for V1-A; it wraps the E220 UART modem (library LoRa_E220), not a chip-level SPI driver.
- **M1Runtime** is the **only** composition point that receives IRadio\* and the init flags (radio_ready, rssi_available); it uses only IRadio::send, recv, last_rssi_dbm() in the tick path.
- **Boot/provisioning:** Phase A (module boot/verify-repair), Phase B (role + radio profile pointers), Phase C (start comms) — aligned with [boot_pipeline_v0](boot_pipeline_v0.md) and [radio_profiles_policy_v0](../../radio/policy/radio_profiles_policy_v0.md).

---

## 7) Implications / footguns

- **Do not** add radio_ready or rssi_available as IRadio methods without a clear need (e.g. a second adapter that must be queried at runtime). V1-A keeps them as init params only.
- **If** the HAL contract is later expanded (RadioTxResult, getRadioStatusSnapshot, getRadioCapabilityInfo), every adapter must map to the same contract; the current bool send() is sufficient for V1-A but does not match the broader hal_contracts_v0 description.
- **Wiring** must remain the single place that knows the concrete radio type and maps adapter-specific results (e.g. boot result) into generic forms when calling platform-agnostic APIs (e.g. provisioning).

---

## 8) Related

- Audit (domain vs radio adapter): [#229](https://github.com/AlexanderTsarkov/naviga-app/issues/229).
- Follow-up issues closed by this doc: [#259](https://github.com/AlexanderTsarkov/naviga-app/issues/259) (IRadio vs hal_contracts), [#260](https://github.com/AlexanderTsarkov/naviga-app/issues/260) (radio_ready / rssi_available design).
- HAL contract (broader, candidate future): [hal_contracts_v0](../../../../firmware/hal_contracts_v0.md).
- Boot and provisioning: [boot_pipeline_v0](boot_pipeline_v0.md), [provisioning_interface_v0](provisioning_interface_v0.md), [module_boot_config_v0](module_boot_config_v0.md).
