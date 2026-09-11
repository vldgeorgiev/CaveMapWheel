# CaveMapWheel V1 electronics assembly guide

This guide covers only point-to-point assembly of the dry V1 electronics. It
does not cover component positioning, wheel alignment, enclosures, potting, or
other mechanical work. The canonical circuit is the
[V1.2 schematic](../design/cavedivemap-v1-improved-schematic.svg); if this guide
and the schematic disagree, stop and correct the documentation before wiring.
The complete component list is in the
[canonical design](../design/cavedivemap-v1-improved-design.md#8-bill-of-materials).

## 1. Prepare and identify the parts

Keep USB and the battery disconnected while assembling.

1. Confirm the board is the non-Sense Seeed XIAO nRF52840, SKU 102010448.
2. Measure and label R1 `100 ohm`, R2 `10 kohm`, R3 `10 kohm`, and R4
   `100 kohm`.
3. From the exact purchased-part markings and datasheets, identify:
   - LED1 anode and cathode (long leg anode, short leg cathode).
   - Q1 BC547B collector, base, and emitter (left to right with flat face forward: Collector, Base, Emitter)
   - Q2 TEPT4400 collector and emitter (long lead is emitter and its short lead is collector)
   - Battery positive and negative wires.
4. Identify XIAO pins `3V3`, `GND`, `D0/A0`, `D1`, and `D2`, plus the `BAT+`
   and `BAT-` pads. Do not make external connections to board-internal P0.13,
   P0.14, or P0.31.

Do not infer semiconductor lead order from a generic drawing or from lead
length alone.

## 2. Assemble the common supply connections

1. Establish one common GND connection for the circuit.
2. Connect C1 `100 nF` directly between XIAO `3V3` and `GND`.

## 3. Assemble the optical emitter circuit

1. Connect Q1 emitter to `GND`.
2. Connect R4 `100 kohm` from Q1 base to `GND`.
3. Connect XIAO `D1` through R2 `10 kohm` to Q1 base.
4. Connect Q1 collector to LED1 cathode.
5. Connect LED1 anode through R1 `100 ohm` to XIAO `3V3`.

The completed electrical path is:

```text
3V3 -> R1 -> LED1 anode; LED1 cathode -> Q1 collector
D1 -> R2 -> Q1 base; Q1 base -> R4 -> GND
Q1 emitter -> GND
```

## 4. Assemble the optical receiver circuit

Create a single `SENSOR_NODE`, then make these connections:

1. Connect Q2 TEPT4400 emitter to `GND`.
2. Connect Q2 collector to `SENSOR_NODE`.
3. Connect XIAO `D0/A0` directly to `SENSOR_NODE`.
4. Connect XIAO `D2` through R3 `10 kohm` to `SENSOR_NODE`.

Do not add a capacitor to `SENSOR_NODE`.

## 5. Inspect before applying power

With USB and battery still disconnected:

1. Compare every connection against the schematic, one net at a time.
2. Confirm all circuit grounds have continuity to XIAO `GND`.
3. Confirm `3V3` is not shorted to `GND`.
4. Confirm `BAT+` is not shorted to `BAT-`.
5. Recheck LED1, Q1, and Q2 polarity and lead assignments.
6. Inspect for solder bridges, loose strands, and uninsulated joints.

Use insulated wire and insulate every point-to-point joint.

## 6. First USB-powered check

Leave the battery disconnected.

1. Connect USB and flash the `xiao_debug` PlatformIO environment.
2. Disconnect USB and inspect the circuit again if anything heats, smells, or
   draws unexpectedly high current.
3. Use an oscilloscope to confirm D1 pulses at approximately 200 Hz and returns
   LOW between samples. The LED may look steadily lit at this rate; visual
   appearance does not establish whether it is pulsing.
4. Confirm the diagnostic serial stream is present and that the ADC readings
   respond when the optical path is alternately open and blocked.
5. Measure the on-pulse voltage across R1 with a differential probe, or two
   oscilloscope channels referenced to circuit GND and channel subtraction.
   Keep ordinary scope ground clips at circuit GND, never at either end of
   the high-side R1. Calculate `I_LED = V_R1(on) / 100 ohm` and record pulse
   width and current. An ordinary multimeter averages the pulses and cannot
   establish peak current. Leave this check pending if suitable equipment is
   unavailable.

## 7. Connect the battery last

Proceed only after the USB-powered checks pass and the
[battery-interface corrections](../design/cavedivemap-v1-improved-design.md#44-battery-and-charging)
have been implemented and checked for the actual board revision. The current
firmware/core still have the P0.14 idle/startup and ADC acquisition issues
described there; keep the battery disconnected until these are resolved.

1. Disconnect USB.
2. Measure the battery voltage; use the meter reading's sign to identify its
   positive and negative wires.
3. With the XIAO unpowered, confirm that its `BAT-` pad has continuity to `GND`
   and identify the remaining battery pad as `BAT+` from the board marking.
4. With the battery connector unplugged, solder its mating lead positive to
   `BAT+` and negative to `BAT-`, then insulate the joints. Never solder
   directly to the pouch cell tabs.
5. Verify the complete connector-to-pad polarity before plugging in the battery;
   the circuit powers up immediately when connected.
6. Verify the firmware battery reading against the multimeter before relying on
   low-battery telemetry.

The XIAO charge-current and battery-divider controls are board-internal. Do not
strap or rewire them. The selected software core leaves P0.13 in the nominal
approximately 50 mA charge configuration. Battery-divider operation must follow
the corrected requirements linked above; verify startup/reset as well as
normal operation before charging.

## 8. Assembly completion checklist

- [ ] Every net matches the V1.2 schematic.
- [ ] LED1 A/K, Q1 C/B/E, and Q2 C/E were checked against the purchased parts.
- [ ] `3V3`-to-`GND` and `BAT+`-to-`BAT-` checks show no short.
- [ ] USB-only startup and optical diagnostics pass.
- [ ] LED pulse current is measured and recorded.
- [ ] Battery polarity is verified before connection.
- [ ] P0.14 operation/startup and ADC acquisition corrections are verified.
- [ ] Battery voltage is compared with a multimeter.

Continue with the evidence-producing dry tests in the
[validation checklist](../validation.md). Do
not mark a hardware task complete from visual inspection alone.
