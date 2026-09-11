# CaveMapWheel V1 firmware

This document owns the V1 firmware behavior, build instructions and client
protocol. Requirements describe intended behavior; implementation and acceptance
status are tracked in the [validation checklist](../validation.md). V1 covers
dry testing only.

Before connecting the circuit or battery, follow the
[electronics assembly guide](../hardware/electronics-assembly-guide.md).

## Layout

The [firmware architecture](architecture.md) defines module ownership and
runtime coordination.

- `src/device.*`: board-independent lifecycle, scheduling, counting, battery,
  snapshots, and telemetry encoding
- `src/sensor.*`: board-independent acquisition, classification, and faults
- `src/hardware.*`, `src/ble.*`, `src/diagnostics.*`: concrete device adapters
- `src/main.cpp`: cooperative application coordination
- `src/config.hpp`: build-time defaults, overrides, and validation
- `test`: PlatformIO native tests

No reusable PlatformIO `lib` is used; headers remain beside implementations in
`src`, as this firmware is one application.

## Build and test

Install the PlatformIO extension in VS Code, then use its environment selector,
or run:

```sh
pio test -e native
pio run -e xiao_debug
pio run -e xiao_power
```

Run commands from the repository root. After the assembly guide's pre-power
checks, use PlatformIO's Upload action for `xiao_debug`, or:

```sh
pio run -e xiao_debug -t upload
pio device monitor -b 115200
```

Choose the detected board port when prompted. The power profile disables
application USB; use the board's bootloader mode (double-tap reset) for recovery
or upload if automatic reset is unavailable. Flash a known-good diagnostic
build to roll back. No persistent session data needs migration.

`xiao_debug` starts in `ACTIVE` and emits diagnostics at 8 Hz. `xiao_power`
boots into `SHELF`, emits no diagnostics, detaches application USB, and keeps
the unused QSPI peripheral disabled. The bootloader remains the recovery and
upload path.

The Seeed PlatformIO platform is pinned by commit in `platformio.ini`. Both
target environments use the Adafruit nRF52 core because the firmware depends on
its Bluefruit API.

## Configuration

Defaults live in `src/config.hpp`. They can be overridden with
PlatformIO `build_flags`, including the optical thresholds, debounce count,
sample/probe periods, settling delays, wake window, inactivity timeout, and
battery hysteresis. D0/A0, D1, and D2 remain the canonical receiver ADC,
emitter-control, and receiver-bias connections.

The optical thresholds (`80` blocked and `240` open) and nominal battery ADC
conversion are provisional. The conversion defaults to the published 1 Mohm +
510 kohm network (`1510/510`) and can be overridden with
`CMW_BATTERY_DIVIDER_NUMERATOR` and `CMW_BATTERY_DIVIDER_DENOMINATOR`. Record
real blocked/open distributions, confirm the physical divider for the purchased
board revision, and compare battery readings with a multimeter before treating
either as calibrated.

## Installed-core pin findings

The commit-pinned `Seeed_XIAO_nRF52840` variant used for the successful target
build defines:

- `PIN_CHARGING_CURRENT` as Arduino pin 22, mapped to P0.13; variant startup
  configures it as input/high impedance (the documented 50 mA setting).
- `VBAT_ENABLE` as Arduino pin 14, mapped to P0.14. The current adapter releases
  it to high impedance between readings and the core initially drives it HIGH;
  both need correction/review against the published direct-divider circuit.
- `PIN_VBAT` as Arduino pin 32, mapped to P0.31.
- RGB pins as red 11, green 13, blue 12, with HIGH used for off by the variant.

These findings verify the selected software core, not the identity of the board
on the bench. Confirm the purchased board revision and battery conversion at two
voltages before completing the battery checks in [validation](../validation.md).

**Battery testing is pending firmware corrections.** Follow the
[corrected battery-interface requirements](../design/cavedivemap-v1-improved-design.md#44-battery-and-charging):
hold P0.14 LOW for the published circuit, resolve core and bootloader/reset
behavior, and configure ADC acquisition to at least 20 microseconds (40 for
margin). The current 3-microsecond core default is insufficient for the battery
divider; its separate 200-microsecond settling delay does not fix acquisition.
Complete these corrections before connecting the battery, then calibrate.

## BLE packet

The Read/Notify characteristic is exactly 10 bytes, little-endian:

| Offset | Size | Value |
| --- | ---: | --- |
| 0 | 1 | protocol version (`1`) |
| 1 | 1 | flags: active, low battery, sensor fault |
| 2 | 2 | nonzero session ID |
| 4 | 4 | cumulative pulse count, modulo 2^32 |
| 8 | 2 | battery millivolts |

Device name: `CaveMapWheel-V1`  
Service: `64d60f54-cf51-4254-9cb6-3b01301fdea9`  
Characteristic: `92809e18-683a-466a-8555-c9040518d114`

The complete local name is advertised in ACTIVE; the current transport places
the service UUID in the scan response. Clients must use active scanning to see
that advertisement field. The service exposes one Read/Notify characteristic.
Encode fields explicitly, never by sending a native C structure.

Flag assignments: bit 0 active, bit 1 low battery, bit 2 sensor fault or
insufficient optical margin; bits 3–7 are zero. The implemented fault conditions
are listed below; bit 2 is not a guarantee of calibrated optical margin.

Notify after every counted pulse while subscribed and send the current snapshot
at least once per second while connected, active and subscribed, even when
stationary. Reads return the current snapshot. Connection, subscription,
disconnection and packet loss do not reset or otherwise modify the counter.
Entering SHELF stops advertising and the active connection.

Within one session, clients calculate movement as unsigned 32-bit
`new_count - previous_count`, modulo 2^32. Thus `0xffffffff -> 0` is one pulse;
a later packet recovers counts missed through dropped notifications. A changed
session starts a new baseline. Clients detect stale active streams using a
configured heartbeat timeout and apply their calibrated distance-per-pulse;
distance and direction are not calculated by this firmware.

For a self-contained mobile-agent handoff, including decoder vectors, reconnect
logic and client checks, use the
[BLE client integration guide](../mobile/ble-protocol.md).

## Sampling and counting

Before starting USB, BLE or sampling, drive D1 and D2 LOW and all active-LOW
RGB channels HIGH. Acquire samples in the cooperative main loop, not interrupt
callbacks. Use wrap-safe deadlines without catch-up bursts.

At 200 Hz in ACTIVE, enable D2, wait for receiver settling, read D0 with D1 LOW,
then enable D1, wait for emitter settling and read D0 again. Calculate
`optical_delta = ambient_adc - lit_adc`. Restore D1 and D2 LOW after every
transaction, including ADC failures. The configured settling delays currently
default to 150 microseconds each; validate them on the assembled device.

Classify delta at or above `open_threshold` as OPEN, at or below
`blocked_threshold` as BLOCKED, and retain stable state between thresholds.
Require three consecutive classifications before a stable state change by
default. Threshold chatter must not count. Invalid ordering
(`open_threshold <= blocked_threshold`) latches a fault and prevents counting.

Only stable BLOCKED-to-OPEN transitions during ACTIVE increment the unsigned
32-bit counter, modulo 2^32. OPEN-to-BLOCKED and wake-gesture transitions do not
count. Four openings must produce four counts per full revolution, including at
141 rpm; physical verification requires 4,000 counts over 1,000 revolutions.

## Lifecycle and battery

| State/event | Required behavior |
| --- | --- |
| BOOT | Safe GPIO initialization; new nonzero 16-bit session, count zero; normal boot enters SHELF, debug may start ACTIVE |
| SHELF | System ON idle with approximately one-second differential probes; BLE, diagnostics and RGB off, D1/D2 LOW between probes |
| Wake | Two debounced stable optical changes within five seconds; discard an isolated/expired gesture; gesture movement adds no counts |
| ACTIVE | 200 Hz sensing and BLE; return to SHELF after 15 minutes without a counted pulse; every count refreshes the deadline |
| Shelf transitions | Retain session and count in RAM |
| Reset/restart/battery reconnection | New session, count zero; no nonvolatile persistence |

One-second probing requires deliberate slow wake motion; brief motion may be
missed. System OFF cannot support the required periodic probes. A random
16-bit session can eventually repeat; clients must not treat it as a globally
unique identifier.

Measure battery at boot and periodically in ACTIVE (currently every 60 seconds)
using the board-verified procedure in the electrical design. Low battery asserts
below 3500 mV and clears only above 3600 mV; equality retains the previous state.
Thresholds are build-configurable. Wake produces one brief non-red indication
(currently green) or red for low battery, then RGB off. RGB remains off in SHELF.

## USB diagnostics and faults

With diagnostics enabled, USB-connected ACTIVE operation must emit independent
UTF-8 JSON objects, one per LF-terminated line, at a configurable 5–10 Hz
(currently 8 Hz). The power profile emits no periodic diagnostic records.

| Required JSON type | Required keys |
| --- | --- |
| Number | `ambient_adc`, `lit_adc`, `optical_delta`, `pulse_count`, `session_id`, `battery_mv`, `open_threshold`, `blocked_threshold`, `settle_us`, `debounce_samples` |
| String | `optical_state`, `lifecycle_state`, `ble_state`, `firmware_build` |
| Boolean | `counted_transition`, `sensor_fault` |

Version 1 may add keys but must not remove these keys or change their types.
Current extra fields are numeric `receiver_settle_us` and string `fault_reason`;
`settle_us` reports emitter settling. Logging is a sampled diagnostic view;
use the cumulative counter for movement, not the number of logged transitions.

Invalid thresholds latch a sensor fault until restart with valid configuration.
An acquisition failure asserts a transient fault immediately. Either optical
reading continuously at an ADC rail for two seconds in ACTIVE asserts a rail
fault. Two seconds of valid, non-railed pairs clear transient faults; railed
shelf probes cannot clear them. Expose the fault in diagnostics and BLE bit 2.
Current rail bounds are <=2 and >=4093 at 12-bit resolution.

## Development rules

Keep deterministic behavior in `device.*` and `sensor.*`; board I/O belongs in
the concrete adapter files, and explicit coordination belongs in `main.cpp`.
Follow the [architecture document](architecture.md) when changing boundaries.
Headers stay beside implementations; there is no reusable `lib` package.
Native tests cover hysteresis, debounce, faults, counting, wake, timing,
sessions, packet/flag encoding and modular recovery arithmetic without a board.

Keep dependencies reproducible and pin changes explicit. Configuration remains
build-time; V1 does not require BLE writes, stored calibration, OTA updates or a
dedicated phone app. Use a generic BLE client for acceptance. Current targets,
bench evidence and remaining work belong in [validation](../validation.md).
