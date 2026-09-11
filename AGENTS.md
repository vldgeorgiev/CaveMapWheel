# AI Agent Instructions

## Canonical design

Before changing hardware, firmware, phone integration, tests, or mechanical design, read:

- [`docs/design/cavedivemap-v1-improved-design.md`](docs/design/cavedivemap-v1-improved-design.md)
- [`docs/design/cavedivemap-v1-improved-schematic.svg`](docs/design/cavedivemap-v1-improved-schematic.svg)
- [`docs/hardware/electronics-assembly-guide.md`](docs/hardware/electronics-assembly-guide.md) before assembling or rewiring electronics

The canonical V1 electrical design and schematic are maintained under
`docs/design/`; the canonical electronics assembly sequence is maintained under
`docs/hardware/`.

## Firmware and validation

Before changing firmware, tests, or phone integration, read
[`docs/firmware/README.md`](docs/firmware/README.md) for the behavioral contract
and [`docs/validation.md`](docs/validation.md) for pending fixes and acceptance.
Before implementing phone integration, also read
[`docs/mobile/ble-protocol.md`](docs/mobile/ble-protocol.md) for discovery,
decoding, session and reconnect requirements.
Read [`docs/firmware/architecture.md`](docs/firmware/architecture.md) for the
implemented module boundaries and runtime coordination.
Use ordinary code changes and update the affected documentation in the same
change. No separate proposal or change-management workflow is required.

Keep requirements in their owning document: circuit and components in the
electrical design, build/runtime/protocol behavior in the firmware README,
module ownership and runtime coordination in the firmware architecture document,
mobile client behavior in the BLE client integration guide,
assembly procedure in the assembly guide, and work status/evidence in the
validation checklist. Link to those documents instead of duplicating them.

Do not mark hardware calibration, endurance, battery, BLE-device, or power
measurements complete without recorded evidence from the assembled dry device.
Keep implemented, software-tested, and hardware-verified status distinct.

## Current V1 boundary

- V1 is a dry proof of concept.
- Do not claim pressure, underwater BLE, silt, strong dive-light, or potting qualification from dry tests.
- The final design is intended to be a solid waterproof potted block, but final potting and pressure validation are later work.
- Do not introduce magnets, Hall sensors, reed switches, magnetic wake mechanisms, direction sensing, an external power switch, wireless charging, a custom PCB, or perfboard into V1 unless the user changes the scope.
- Preserve switched receiver bias on D2, emitter control on D1, receiver ADC on D0/A0, and the documented cumulative-counter semantics unless a reviewed design change says otherwise.
- Verify the exact XIAO board revision and purchased component datasheets before relying on internal pin behavior or physical lead order.
- Keep local changes uncommitted unless the user explicitly requests a commit.

## Validation

Report dry bench, compile, and static checks accurately. They are not evidence of underwater, pressure, potting, or real-dive performance.
