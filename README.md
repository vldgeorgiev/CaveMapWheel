# CaveMapWheel

Dry proof of concept for a four-opening optical wheel sensor using a Seeed
XIAO nRF52840 and BLE cumulative-count telemetry. Firmware is implemented and
has passed host tests and target builds; assembled-device acceptance is pending.

## Documentation

| Document | What it owns |
| --- | --- |
| [Electrical design](docs/design/cavedivemap-v1-improved-design.md) | Components, pins, circuit requirements, battery interface, scope |
| [Schematic](docs/design/cavedivemap-v1-improved-schematic.svg) ([PNG](docs/design/cavedivemap-v1-improved-schematic.png)) | Circuit connectivity |
| [Electronics assembly](docs/hardware/electronics-assembly-guide.md) | Wiring order and first power-up |
| [Firmware](docs/firmware/README.md) | Build/upload, behavior, configuration, BLE and diagnostics |
| [Firmware architecture](docs/firmware/architecture.md) | Current module ownership, dependencies and runtime coordination |
| [BLE client integration](docs/mobile/ble-protocol.md) | Mobile discovery, packet decoding, sessions, reconnects and client checks |
| [Validation and remaining work](docs/validation.md) | Pending fixes, acceptance checks, evidence and status |
| [Agent instructions](AGENTS.md) | Working rules and document ownership |

Start with the assembly guide for electronics work, or open this repository
root in VS Code with PlatformIO for firmware work. Keep the battery disconnected
until the battery-interface corrections in the validation checklist are resolved.

V1 covers dry testing only. Potting, pressure, underwater BLE, silt, and strong
dive-light qualification are later work.
