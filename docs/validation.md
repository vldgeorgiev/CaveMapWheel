# V1 validation and remaining work

This is the single work and acceptance checklist for people and agents.
Requirements live in the [electrical design](design/cavedivemap-v1-improved-design.md)
and [firmware README](firmware/README.md); wiring steps live in the
[assembly guide](hardware/electronics-assembly-guide.md).

## Current status

Software implemented: optical acquisition and classification, counting, wake
gesture, lifecycle, BLE transport, diagnostics, fault tracking, nominal battery
conversion, and separate debug/power profiles. Implementation does not establish
physical correctness or current consumption.

Local validation on 2026-09-23 reported 21 passing native tests and successful
`xiao_debug` and `xiao_power` builds after the architecture refactor. This is
software evidence, not hardware evidence. No assembled-device acceptance
results are recorded here. V1 remains **unaccepted**.

## Fix before battery connection

- [ ] **Battery GPIO:** identify the actual board revision and verify P0.13,
  P0.14 and P0.31 against its circuit and installed core. For the published
  direct divider, keep P0.14 LOW between readings. Correct the application's
  high-impedance idle state; resolve the core's initial HIGH state and verify
  bootloader, reset, upload, normal operation and charging behavior.
- [ ] **Battery ADC:** configure acquisition to at least 20 microseconds
  (40 for margin) for the approximately 338 kohm source. The present core
  defaults to 3 microseconds; a separate pre-read delay does not fix this.
  Verify the divider ratio, reference and settling procedure before calibration.

The [electrical design, section 4.4](design/cavedivemap-v1-improved-design.md#44-battery-and-charging)
contains the rationale and manufacturer references. Keep these boxes open until
both implementation and the necessary physical checks have evidence.

## Architecture refactor

Status: completed and software-verified on 2026-09-23. The
[firmware architecture](firmware/architecture.md) records the resulting design.
Battery-interface corrections above remain separate prerequisites for battery
connection.

- [x] Stage 1: record baseline native tests and both target builds; add meaningful
  lifecycle sequence/retention tests, including rollover and delayed-loop cases.
- [x] Stage 2: consolidate sensor logic; retain acquisition ordering/cleanup,
  classification and fault tests; verify native and both target builds.
- [x] Stage 3: consolidate device state/scheduling and encoding; verify integrated
  sequences and packet vectors with no board dependency; build both targets.
- [x] Stage 4: flatten adapters/configuration and simplify main; remove obsolete
  modules, update source filters, and verify native and both target builds.
- [x] Stage 5: record final test/build results and contract comparison; update
  current-layout documentation and architecture status, check stale references,
  and record any remaining adapter or bench-verification gaps.

Evidence: baseline 18/18 native tests and both profiles passed; final 21/21
native tests and both profiles passed. Final target sizes were 121,680 bytes
flash / 14,020 bytes RAM (`xiao_debug`) and 119,840 bytes flash / 14,020 bytes
RAM (`xiao_power`). No device was flashed or physically exercised.

## Software verification

Run after relevant firmware changes and record commands, build identifier and
results. Check the actual assertions when extending coverage.

- [ ] Build both pinned XIAO profiles and run `pio test -e native`. Confirm
  native source filtering excludes board APIs; debug and power settings differ
  as documented, and diagnostics are absent from the power build.
- [ ] Verify startup adapter call ordering, successful and failed ADC cleanup,
  and D1/D2 LOW after every acquisition path.
- [ ] Verify wrap-safe sample deadlines and delayed loops without catch-up
  bursts or duplicate acquisitions.
- [ ] Verify differential arithmetic, invalid thresholds, hysteresis retention,
  three-sample debounce and chatter rejection.
- [ ] Verify only active BLOCKED-to-OPEN transitions count: four openings give
  four counts; reverse edges and shelf gesture samples do not count.
- [ ] Verify counter rollover, reset, session retention and modular deltas.
- [ ] Verify two debounced shelf changes within five seconds wake; isolated,
  expired and noisy gestures do not. Verify boot-to-shelf, wake, 15-minute
  timeout and refresh only by counted pulses.
- [ ] Verify exact ten-byte little-endian encoding, flag bits, zero, typical,
  high-bit and maximum values, and recovery after dropped notifications.
- [ ] Verify battery hysteresis boundaries and transient/latched sensor faults,
  including that railed shelf samples cannot clear an existing rail fault.

These unchecked entries request recorded verification for the build being
accepted; they do not mean the previously reported software work was undone.

## Dry device acceptance

Record evidence for each item. Do not check a box from compilation or a mock.

- [ ] Before power: record XIAO revision, purchased BC547B maker and C/B/E,
  TEPT4400 C/E, LED A/K, battery/connector polarity and resistance across supply
  rails. Confirm the schematic connections and absence of shorts.
- [ ] On USB only: exercise emitter and receiver, verify startup GPIO states,
  D1/D2 between pulses/probes, and RGB active-LOW behavior. Measure LED pulse
  current and width using the assembly guide's oscilloscope method.
- [ ] Capture open/blocked ambient and lit distributions, optical delta,
  thresholds, settling and debounce at 200 samples/s. Record LED current,
  board revision, build and supply/battery voltage. If there is no repeatable
  hysteresis margin, retain the failed measurements and stop count acceptance.
- [ ] Count from slow rotation to 141 rpm, with BLE connected and disconnected.
  Confirm four counts per revolution and exactly 4,000 counts over 1,000
  revolutions at maximum speed, without misses or duplicates. Check sampling
  timing under BLE and diagnostic load.
- [ ] Verify normal boot enters SHELF; a deliberate slow gesture wakes without
  adding counts; isolated motion does not wake. Verify 15-minute inactivity,
  timer refresh on counts, retained session/count after shelf, and reset to a
  new nonzero session with count zero.
- [ ] Verify brief non-red normal wake and red low-battery wake indications,
  followed by RGB off, with no illuminated RGB channel in shelf.
- [ ] After battery-interface corrections: compare reported voltage with a
  multimeter at two voltages, record calibration constants and an explicit
  tolerance, and verify low below 3500 mV, recovery above 3600 mV and nominal
  50 mA charge configuration on the actual board.
- [ ] With a generic BLE client, discover the complete name and UUIDs, read and
  decode a current ten-byte snapshot, subscribe, and capture each counted pulse
  plus unchanged heartbeats at least once per second. Reconnect/drop packets
  and recover cumulative movement; verify advertising and connection stop in
  shelf. Check client handling of session changes and stale heartbeats.
- [ ] Capture USB diagnostics at the configured 5–10 Hz. Parse every line as
  one JSON object with all required fields and stable types. Inject invalid
  thresholds, acquisition failures and sustained rails; verify diagnostics and
  BLE fault bit, restart-only threshold recovery and two-second transient
  recovery. Confirm power firmware emits no periodic USB records.
- [ ] On battery, USB disconnected and logging disabled, measure shelf over
  enough one-second probe cycles: average below 30 microamps. Measure active
  sampling with BLE: average below 3 mA excluding RGB indications. Record
  instruments, intervals and conditions. Confirm USB/peripheral suspension and
  external flash state; disabling QSPI alone is not proof of flash sleep.
- [ ] Record final calibration, current measurements, limitations and passing
  evidence for every requirement above before accepting dry V1. If the framework
  cannot meet power targets, document a follow-up design decision; do not lower
  the targets silently. Dry acceptance does not qualify later underwater use.

## Evidence record

Append a record here or link a report/capture stored beside this document.
Leave unknown values explicit; never fabricate measurements.

| Field | Value to record |
| --- | --- |
| Date and operator | Pending |
| Hardware | Board revision, component identities, circuit revision |
| Firmware | Commit or source snapshot, profile, build ID, dependency versions |
| Configuration | Thresholds, debounce, settling, battery conversion |
| Conditions | USB/battery supply, voltage, BLE state, instruments |
| Checks | Checklist item, expected result, actual result, pass/fail |
| Evidence | Raw JSON, BLE capture, scope capture, current log/report paths |
| Outcome | Remaining defects, retest needed, or dry acceptance |
