# CaveMapWheel V1 BLE client integration

This is the self-contained handoff for an agent implementing the mobile BLE
client. It describes protocol version 1 as implemented by the current firmware.
The canonical behavior contract remains the
[firmware README](../firmware/README.md#ble-packet); if the documents disagree,
stop and correct them together.

Status: firmware encoding is covered by host tests and both firmware profiles
compile. Communication with a physical device has not yet been accepted. V1 is
a dry proof of concept.

## GATT and discovery

| Item | Value |
| --- | --- |
| Local name | `CaveMapWheel-V1` |
| Service UUID | `64d60f54-cf51-4254-9cb6-3b01301fdea9` |
| Telemetry characteristic UUID | `92809e18-683a-466a-8555-c9040518d114` |
| Characteristic operations | Read and Notify |
| Writes | Not supported |
| Pairing, bonding, authentication | Not required or implemented |

Use an **active BLE scan**. The complete local name is in the primary
advertisement, while the 128-bit service UUID is in the scan response. A
passive scan may see the name but miss the service UUID. Discover and identify
the device by the service UUID; treat the local name as a useful display/filter,
not as a unique identity.

The device advertises only in `ACTIVE`:

- The diagnostic firmware profile starts in `ACTIVE` and advertises after boot.
- The power profile starts in `SHELF`. Ask the user to rotate the wheel slowly
  and deliberately to wake it before or while scanning.
- After 15 minutes without a counted pulse, the device enters `SHELF`, stops
  advertising, and disconnects an active client. This is expected behavior.
- `SHELF` still probes the wheel approximately once per second; the device is
  not necessarily powered off merely because it is absent from a scan.

## Telemetry packet

The characteristic value is exactly **10 bytes**. All multi-byte integers are
unsigned and little-endian. Never decode it as a native language structure.

| Offset | Size | Type | Meaning |
| ---: | ---: | --- | --- |
| 0 | 1 | `uint8` | Protocol version; currently `1` |
| 1 | 1 | bit field | Status flags |
| 2 | 2 | `uint16_le` | Nonzero session ID |
| 4 | 4 | `uint32_le` | Cumulative pulse count, modulo 2^32 |
| 8 | 2 | `uint16_le` | Battery voltage in millivolts |

Flag byte:

| Bit | Mask | Meaning when set |
| ---: | ---: | --- |
| 0 | `0x01` | Device is `ACTIVE` |
| 1 | `0x02` | Low-battery hysteresis is asserted |
| 2 | `0x04` | Sensor/configuration fault is asserted |
| 3–7 | `0xF8` | Reserved; zero in protocol version 1 |

The firmware sends packets only while BLE is running, so version 1 clients will
normally see the active flag set. Do not expect a final packet with `active =
false`: entry into `SHELF` stops BLE and disconnects the client.

### Decoder requirements

1. Require a value length of exactly 10 bytes. Reject and log other lengths.
2. Read byte 0 before interpreting the remaining bytes. Reject unsupported
   protocol versions rather than silently applying the version 1 layout.
3. Parse the session, count, and voltage as unsigned little-endian values.
4. Expose known flag bits independently. Ignore reserved bits for UI behavior,
   but retain/log them to help diagnose a newer firmware version.
5. Preserve the full unsigned 32-bit counter even if the app language's default
   integer type is signed.

Language-neutral helpers:

```text
u16le(b, i) = b[i] | (b[i + 1] << 8)

u32le(b, i) = b[i]
            | (b[i + 1] << 8)
            | (b[i + 2] << 16)
            | (b[i + 3] << 24)

version     = b[0]
flags       = b[1]
sessionId   = u16le(b, 2)
pulseCount  = u32le(b, 4)
batteryMv   = u16le(b, 8)
```

In languages with signed bitwise operators, promote each byte to a sufficiently
wide unsigned or 64-bit integer before shifting.

### Golden decoder vector

The firmware host tests use this exact packet:

```text
01 07 CD AB 21 43 65 87 34 12
```

Expected interpretation:

```text
version       = 1
active        = true
lowBattery    = true
sensorFault   = true
sessionId     = 0xABCD
pulseCount    = 0x87654321
batteryMv     = 0x1234
```

The voltage in this synthetic vector is intentionally just an encoding test,
not a realistic battery reading.

## Notifications and reads

The characteristic contains the latest snapshot and supports both Read and
Notify.

- The firmware updates and notifies after every counted pulse when a client is
  connected and subscribed.
- While connected, active, and subscribed, it also writes/notifies the current
  snapshot at least once per second even when the wheel is stationary.
- A read returns the current characteristic value.
- Connection, subscription, disconnection, and dropped packets never reset the
  firmware counter.

Recommended connection sequence:

1. Connect and discover the service and characteristic.
2. Enable notifications.
3. Read the characteristic once for an initial/current snapshot.
4. Serialize read results and notifications through the same packet-processing
   path. Duplicate counts are normal and produce a delta of zero.

Use a configurable stale-stream timer. Since the current heartbeat is at least
once per second, approximately three seconds is a reasonable initial UI warning
threshold, but it is a client policy rather than part of the wire protocol.
A disconnect can mean `SHELF`, range loss, radio trouble, or reset; do not infer
the cause from disconnection alone.

## Session and movement calculation

`pulseCount` is cumulative within a firmware session. It increments only for a
stable `BLOCKED -> OPEN` transition while `ACTIVE`. There are nominally four
counts per wheel revolution. The firmware reports neither distance nor
direction.

Maintain the last accepted `(sessionId, pulseCount)` for the device:

```text
if there is no previous sample:
    store sessionId and pulseCount
    deltaPulses = 0
else if sessionId != previousSessionId:
    store the new baseline
    deltaPulses = 0
    report that the device session changed
else:
    deltaPulses = (pulseCount - previousPulseCount) modulo 2^32
    store pulseCount
```

For a signed-language implementation, the equivalent calculation using a
64-bit container is:

```text
deltaPulses = (newCount - oldCount) & 0xFFFFFFFF
```

Thus `0xFFFFFFFF -> 0x00000000` is one pulse. A later cumulative packet recovers
movement missed through dropped notifications. Retain the baseline over a short
disconnect/reconnect so the same session can recover missed pulses. If the app
has no trusted baseline, the first packet establishes one and adds no distance.

A reset, restart, or battery reconnection creates a new nonzero 16-bit session
and resets the count to zero. Session IDs are random but only 16 bits and can
eventually repeat; they are not globally unique device or recording IDs. Do not
calculate movement across a detected session change.

Convert pulses to distance only in the app using a user/configuration supplied
distance-per-pulse calibration:

```text
distanceIncrement = deltaPulses * calibratedDistancePerPulse
```

The nominal geometry suggests 35.34 mm per pulse, but wheel deformation and
slip require physical calibration. Do not hard-code the nominal value as an
accepted measurement. No direction information is available in V1.

## Status interpretation and limitations

- `lowBattery` asserts below 3500 mV and clears only above 3600 mV. Equality
  retains the previous state.
- `sensorFault` covers invalid optical thresholds, ADC acquisition failure, or
  a sustained ADC-rail condition. It does not prove that the optical thresholds
  have adequate calibrated margin.
- Battery conversion and low-battery behavior are not yet physically calibrated
  on the assembled device. Display the value/status as provisional during PoC
  work; do not use it for a safety-critical decision.
- The GATT service is open and unencrypted. A nearby client can read and
  subscribe without pairing. There are no remote commands, configuration
  writes, counter resets, firmware updates, or control operations in V1.
- Underwater BLE range and reliability have not been qualified. The current
  acceptance scope is dry testing only.

## Mobile implementation checks

- [ ] Active scanning finds the service UUID from the scan response.
- [ ] The app handles a sleeping device by prompting for deliberate wheel motion
  and continuing or restarting its scan.
- [ ] Decoder rejects wrong lengths and unsupported versions.
- [ ] Golden vector above decodes exactly, using unsigned little-endian values.
- [ ] Flag combinations and reserved bits are covered by unit tests.
- [ ] Counter equality produces zero delta; rollover produces one; a cumulative
  jump recovers the full missed-pulse delta.
- [ ] A session change establishes a new baseline and adds no distance.
- [ ] Notifications are enabled and an initial characteristic read is processed.
- [ ] Heartbeat packets do not add movement when the count is unchanged.
- [ ] Reconnect within the same session preserves the previous baseline.
- [ ] Expected shelf disconnection and stale/range-loss UX are handled without
  claiming a specific disconnect cause.
- [ ] Low-battery and sensor-fault states are visible without presenting the
  uncalibrated V1 data as accepted hardware evidence.

Before declaring mobile integration accepted, capture real discovery, read,
notification, reconnect, session-change, and dropped-packet evidence using the
assembled dry device as required by the [validation checklist](../validation.md).
