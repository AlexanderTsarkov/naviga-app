# Naviga — architecture map (1 page)

## What exists now (OOTB vertical slice)

Firmware init → Radio TX/RX → Domain (NodeTable / BeaconLogic) → BLE bridge → IBleTransport → tests + docs.

Key properties:

- Domain is radio-agnostic.
- E220 is used as UART MODEM via external library, wrapped by an adapter implementing IRadio.
- M1Runtime is the single wiring/composition point.
- Legacy BLE service is disabled.

## Main layers

### 1) Platform / HAL (ESP32)

**Responsibilities:**

- ESP32 init, hardware pins, timing, logging hooks, BLE stack integration.

**Non-responsibilities:**

- Domain logic, protocol semantics, radio strategy.

### 2) Radio adapter layer

- Interface: `IRadio`
- Implementation: `E220Radio` (adapter)

**Responsibilities:**

- init/config (incl. RSSI append), TX/RX, parse RSSI vs payload, provide best-effort send/receive.

**Non-responsibilities:**

- chip-level driver work, SPI driver, real CAD/LBT.

**Channel sensing:**

- abstraction exists (`IChannelSense`) but for E220 returns UNSUPPORTED.
- default behavior: jitter-only, sense OFF.

### 3) Domain layer (pure logic)

- NodeTable
- BeaconLogic
- GEO_BEACON codec / protocol glue (if present)

**Responsibilities:**

- track nodes, decode/encode beacons/messages, rules for beacon timing/policy.

**Non-responsibilities:**

- ESP32 details, BLE details, modem details.

### 4) BLE bridge

- Interface: `IBleTransport`
- Implementation: `BleEsp32Transport` (transport)

**Responsibilities:**

- provide a stable transport boundary to mobile-side or host-side consumers.

**Non-responsibilities:**

- domain decisions, radio behavior.

### 5) Runtime composition

- `M1Runtime` wires:
  - radio ↔ domain ↔ BLE transport
  - policies (send jitter, channel sense ON/OFF)

**Responsibilities:**

- deterministic wiring and lifecycle control.

**Non-responsibilities:**

- implementing the actual logic of components.

## Data flow (conceptual)

Radio RX → adapter → domain ingest → NodeTable updates → BLE bridge publishes NodeTable view/state.

BLE commands (if any) → transport → runtime → domain/radio actions.

## Extension points (future)

- Mobile app consumes BLE transport surface (no domain duplication).
- Future radio layer improvements (metrics, reliability strategies) should not contaminate domain.
- JOIN / Mesh features build on domain + protocol, not on ESP32 glue.
