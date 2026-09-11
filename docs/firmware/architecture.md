# Firmware architecture

Status: **implemented and software-verified on 2026-09-23**. This document owns
firmware module boundaries and coordination. The [firmware README](README.md)
owns behavior and protocol; [validation](../validation.md) owns physical
acceptance and unresolved hardware work.

## Design goals

The firmware is one small application. Its production code is intentionally
flat: 12 files, no reusable `lib`, framework, event bus, dependency-injection
container, heap-allocated action queue, or extra task scheduler. Board-neutral
logic remains testable on the host, while Arduino, Bluefruit and USB details
stay in small concrete adapters.

This refactor preserved the V1 behavior and did not implement the separately
tracked battery-interface corrections. Keep the battery disconnected until the
prerequisites in [validation](../validation.md#fix-before-battery-connection)
are satisfied.

## Source layout

```text
src/
  main.cpp             # Arduino entry points and concrete I/O coordination
  config.hpp           # Build-time defaults, overrides and validation
  device.hpp/.cpp      # Device state, lifecycle, scheduling and telemetry
  sensor.hpp/.cpp      # Acquisition, classification and sensor faults
  hardware.hpp/.cpp    # XIAO GPIO, ADC, power, session source and LEDs
  ble.hpp/.cpp         # Bluefruit service, advertising, read and notify
  diagnostics.hpp/.cpp # Optional USB JSONL output
test/
  test_optical/
  test_counter_telemetry/
  test_lifecycle/
  test_fault_battery/
```

Headers stay beside implementations. Tests remain under PlatformIO's standard
root `test/`; test-framework code is not included in device builds.

## Ownership

| Module | Owns | Boundary |
| --- | --- | --- |
| `sensor` | Optical types, wrap-safe time helpers, differential acquisition, debounce/hysteresis and sensor-fault state | Board-independent; acquisition accepts only the narrow `OpticalIo` interface |
| `device` | Lifecycle, wake gesture, session/count, battery hysteresis, all behavioral deadlines, snapshots and ten-byte encoding | Board-independent; owns one `SensorState`; never calls Arduino, BLE or USB |
| `config` | Compile-time defaults, overrides, UUIDs, profile metadata and static checks | Creates the concrete `DeviceConfig`; keeps timing injectable in native tests |
| `hardware` | Concrete `OpticalIo`, safe GPIO setup, ADC reads, battery conversion, session generation, indication, idle and profile-specific peripheral shutdown | Contains all XIAO-specific operations |
| `ble` | Bluefruit objects and connection/advertising state | Consumes encoded packets; never owns or resets counts |
| `diagnostics` | JSONL formatting and output | Consumes a full `DeviceSnapshot` plus BLE connection state; never decides lifecycle |
| `main` | Startup and ordered calls between concrete modules | Executes transitions and I/O; does not duplicate device state or deadlines |

The dependency direction is:

```text
sensor <- device <- config
   ^         ^
   |         +--- ble / diagnostics / main
   +------------- hardware / main
```

`OpticalIo` is the only polymorphic seam. It exists so native tests can verify
the exact emitter/bias/read order and safe cleanup after either ADC read fails.
No generic hardware abstraction is needed.

## State and snapshots

`Device` is the single owner of lifecycle, session, count, battery status,
sensor state and scheduling. It receives an explicit `DeviceConfig`, time,
session ID, battery readings and optical samples, so native tests do not need
Arduino globals or long production timeouts.

Two snapshots serve different contracts:

- `TelemetrySnapshot` contains only the six values encoded into the ten-byte
  BLE packet.
- `DeviceSnapshot` contains telemetry plus the latest sample, classification,
  counted flag, lifecycle and fault reason required by USB diagnostics.

Counter increment, modulo delta and battery hysteresis remain small pure
functions so their boundary cases are directly testable. Sensor classifier and
fault tracking are stateful but live together in `SensorState`; threshold
validity is wired into the fault state at construction.

## Runtime coordination

The application uses one cooperative Arduino loop:

1. `safeBegin()` establishes safe pins and, in the power profile, disables the
   application USB and unused QSPI peripheral. Main obtains a session, starts
   optional diagnostics, reads time and battery, then initializes `Device`.
2. `Device::update(now)` handles inactivity. Main executes any returned
   `ToShelf` transition once by stopping BLE and forcing optical outputs/LEDs off.
3. When one sample is due, main calls the stateless differential acquisition
   function and passes the result to `Device::onSample`. `SensorState` applies
   faults and classification; `Device` applies wake and count rules.
4. A returned `ToActive` starts BLE, displays the wake indication and writes the
   current packet. A counted event publishes a notification. A wake event and a
   count cannot occur together, so wake movement is not counted.
5. While active, main processes heartbeat first, battery second and diagnostics
   third, matching the previous ordering. Each consumed deadline advances from
   the current time, preventing catch-up bursts after a delayed loop.
6. Main reads a fresh time before idle. ACTIVE keeps the existing 1 ms idle;
   SHELF idles for `min(time-to-next-probe, 1000 ms)`.

`Transition {None, ToActive, ToShelf}` makes mode changes explicit. `Device`
commits each lifecycle change and resets its deadlines; main only performs the
corresponding concrete I/O effects.

## Profiles and native boundary

Both XIAO environments retain the pinned Seeed platform and Adafruit nRF52 core.
The debug/power flags and external behavior are unchanged. The native
environment explicitly includes only `device.cpp` and `sensor.cpp`, preventing
Arduino or Bluefruit headers from entering host tests.

## Refactor verification record

Before the refactor, 18 native tests passed and both `xiao_debug` and
`xiao_power` built successfully. After consolidation, 21 native tests passed and
both target profiles built successfully on 2026-09-23.

The final tests cover acquisition order and both failure cleanup paths,
classification establishment/debounce/hysteresis, sensor faults, battery
hysteresis, packet vectors, four-opening counting, counter rollover/modular delta, complete
shelf/wake/count/inactivity/shelf/wake behavior, session/count retention, exact
wake-window boundary, wrap-safe sampling, and no-catch-up behavioral deadlines.
They also pin the existing diagnostic behavior after an invalid sample.

These checks establish software behavior and successful compilation only. They
do not validate the concrete board, BLE radio behavior, current consumption,
battery interface, optical calibration, or any underwater use.
