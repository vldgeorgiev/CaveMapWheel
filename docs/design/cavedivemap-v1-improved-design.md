# CaveDiveMap Optical Wheel BLE Sensor — Improved V1 Design

| Field | Value |
|---|---|
| Status | Canonical design for dry proof-of-concept implementation |
| Revision | V1.2 — documentation and symbol corrections; external circuit unchanged |
| Date | 2026-09-19 |
| Hardware | Seeed Studio XIAO nRF52840, SKU 102010448 |
| Schematic | [`cavedivemap-v1-improved-schematic.svg`](./cavedivemap-v1-improved-schematic.svg) |
| Rendered schematic | [`cavedivemap-v1-improved-schematic.png`](./cavedivemap-v1-improved-schematic.png) |
| Electronics assembly | [`electronics-assembly-guide.md`](../hardware/electronics-assembly-guide.md) |

## 1. Authority and scope

This document defines V1 scope and electrical requirements. The accompanying
schematic defines circuit connectivity, and the electronics assembly guide
defines the safe point-to-point build sequence.

V1 aims to prove the electronics, optical detection, BLE reporting, state machine, and measured power consumption in dry conditions. Acceptance is pending. It does not qualify the final enclosure, potting process, pressure performance, underwater BLE link, silt tolerance, or strong dive-light rejection.

Accepted project decisions:

- The final electronics are intended to become one solid, waterproof potted block after the schematic and firmware are stable.
- Pressure qualification of the finished potted assembly is deferred until after the dry proof of concept.
- Strong external dive-light saturation is not a V1 acceptance requirement.
- The device has no external power switch and uses deliberate wheel movement to wake.
- No permanent magnet, Hall sensor, reed switch, or magnetic wake mechanism is allowed near the phone.
- Direction sensing is not required.

## 2. Functional requirements

The sensor shall:

1. Detect a 45 mm wheel with four large openings using one transmissive optical channel.
2. Count one `BLOCKED -> OPEN` transition for each opening.
3. Send a cumulative pulse counter to a nearby phone over BLE.
4. Retain its session and counter when moving between `ACTIVE` and `SHELF` states.
5. Wake from `SHELF` after deliberate slow wheel rotation.
6. Return to `SHELF` after 15 minutes without a counted transition.
7. Use the XIAO onboard RGB LED for brief status indications.
8. Charge the protected 1S LiPo through XIAO USB-C during dry V1 use.

## 3. Wheel geometry and timing

- Nominal diameter: 45 mm.
- Circumference: `pi * 45 mm = 141.37 mm`.
- Openings: four.
- Nominal distance per count: `141.37 / 4 = 35.34 mm`.
- Maximum stated travel: 100 m in five minutes.
- Maximum rim speed: approximately 0.333 m/s.
- Maximum wheel speed: approximately 2.36 revolutions/s or 141 rpm.
- Maximum counted transitions: approximately 9.43/s.
- Approximate total optical state transitions: 18.86/s for balanced openings.

The phone must use a configurable calibrated distance-per-pulse value. The nominal 35.34 mm value is only the starting point; rolling diameter, deformation, and slip change the effective calibration.

## 4. Improved electrical design

### 4.1 Pin assignment

| XIAO connection | Symbol | Function | Idle state |
|---|---|---|---|
| D0 / A0 | `PIN_SENSOR_ADC` | Optical receiver ADC input | Analog input |
| D1 | `PIN_EMITTER_ENABLE` | Q1 base drive through R2 | Output LOW |
| D2 | `PIN_SENSOR_BIAS` | Switched receiver pull-up supply | Output LOW |
| 3V3 | — | Optical emitter supply and local decoupling | Powered |
| GND | — | Common ground | — |
| Battery pads | — | Protected 1S LiPo | Verify polarity physically |
| P0.13 | Board internal | XIAO charge-current selection | High impedance for approximately 50 mA; verify against board revision |
| P0.14 | Board internal | Battery-divider lower leg | Hold LOW for the published direct-divider circuit; verify physical revision and startup behavior |
| P0.31 / AIN7 | Board internal | Battery ADC input | Analog input |

The P0.13, P0.14, and P0.31 behavior must be checked against the schematic and board support package for the exact physical XIAO revision before firmware is finalized.

### 4.2 Optical emitter

Electrical path:

```text
3V3 -> R1 100 ohm -> LED1 anode
LED1 cathode -> Q1 collector
Q1 emitter -> GND
D1 -> R2 10 kohm -> Q1 base
Q1 base -> R4 100 kohm -> GND
```

Parts:

- LED1: Optosupply OSG58A5111A, 525 nm green.
- Q1: BC547B NPN, exact physical C/B/E order verified from the purchased manufacturer's datasheet.
- R1: 100 ohm initial value.
- R2: 10 kohm.
- R4: 100 kohm base-emitter pull-down.

R4 ensures Q1 remains off while D1 is high impedance during reset or early boot. Firmware shall configure D1 as output LOW before enabling any other feature.

The nominal LED current estimate is:

```text
I_LED ~= (3.3 V - V_F - V_CE(sat)) / 100 ohm
```

With `V_F ~= 2.9 V` and `V_CE(sat) ~= 0.1 V`, the estimate is approximately 3 mA. This is not a guaranteed current because LED forward voltage varies. V1 must measure pulse current and optical separation on the actual components before changing R1. A lower resistor value may be evaluated only after measuring current and confirming transistor and LED limits.

### 4.3 Optical receiver with switched bias

Electrical path:

```text
D2 -> R3 10 kohm -> sensor node -> D0 / A0
sensor node -> TEPT4400 collector
TEPT4400 emitter -> GND
```

D2 supplies the receiver pull-up only while sampling:

- D2 HIGH: receiver bias enabled.
- D2 LOW: sensor node is pulled low through R3 and the receiver consumes no standing pull-up current.

This removes the potentially continuous current through R3 when ambient light makes the phototransistor conduct during shelf storage. The maximum current sourced by D2 through 10 kohm is approximately 0.33 mA.

Do not add a capacitor directly to the ADC node without rechecking settling time for the off/on differential samples. C1, 100 nF, is local supply decoupling between 3V3 and GND near the optical wiring; it is not an ADC low-pass filter.

### 4.4 Battery and charging

- Battery: Akyga AKY0384 / LP601730, protected 1S LiPo, 3.7 V nominal, 250 mAh.
- Use the approximately 50 mA XIAO charge setting for V1.
- Charge and program only in dry conditions through USB-C.
- Do not charge below 0 degrees C or above 45 degrees C.
- Disconnect USB before power-current measurements.

Follow the [electronics assembly guide](../hardware/electronics-assembly-guide.md)
for battery polarity checks, connection order, and first power-up. Never solder
directly to the pouch cell tabs.

The published XIAO schematic has a direct resistor chain:
`VBAT -> 1 Mohm -> P0.31/AIN7 -> 510 kohm -> P0.14`. P0.14 is not an
isolated power switch. For this circuit, hold P0.14 LOW during operation,
including between readings; releasing it to high impedance lets the ADC node
rise toward battery voltage. Driving it HIGH can also raise the ADC input
outside its permitted range. Keeping the divider enabled draws about 2.8
microamps at 4.2 V; include this in the shelf-current budget.

Configure the SAADC acquisition time to at least 20 microseconds for the
approximately 338 kohm divider source resistance (40 microseconds provides
additional margin). A settling delay before `analogRead()` does not replace
this setting. Then verify the nominal `1510/510` voltage scale against a
multimeter at two battery voltages. See [Nordic's acquisition-time table 95](https://docs-be.nordicsemi.com/bundle/nRF52-Series-PS/raw/resource/enus/nRF52840_PS_v1.1.pdf).

**Pending firmware correction:** the current adapter still releases P0.14 to
high impedance and leaves ADC acquisition at the core's 3-microsecond default;
the selected core also drives P0.14 HIGH before application setup. Resolve
application, core, and bootloader/reset behavior against the exact board
revision before battery connection or charging. See
[Seeed's charging warning](https://wiki.seeedstudio.com/XIAO_BLE/#q3-what-are-the-considerations-when-using-xiao-nrf52840-sense-for-battery-charging).

### 4.5 Status LED

The XIAO RGB LED is active LOW. Firmware shall explicitly drive every RGB channel HIGH before entering `SHELF`.

Required wake and shelf behavior is defined in the
[firmware README](../firmware/README.md#lifecycle-and-battery).

## 5. Firmware behavior and protocol

The [firmware README](../firmware/README.md) owns the sampling sequence,
classification/debounce, lifecycle, session/count semantics, BLE packet and
USB diagnostic contract. Hardware-facing battery requirements remain in
section 4.4 above. Use the firmware README for implementation and phone work.

## 6. Validation

The [validation checklist](../validation.md) owns pending fixes, software
verification, dry-device acceptance criteria and evidence records. It includes
optical calibration, the 1,000-revolution test, BLE and lifecycle tests, battery
calibration, and the below-30-microamp shelf / below-3-mA active targets.
Successful compilation and host tests do not constitute device acceptance.

## 7. Deferred work after dry V1

The following items are deliberately not V1 acceptance criteria:

- Potting material selection and process qualification.
- Void-free final encapsulation.
- Pressure cycling and depth qualification.
- Underwater BLE range and packet-loss testing.
- Silt deposition and cleaning behavior.
- Strong dive-light saturation rejection.
- Optical-grade windows, masks, filters, or automatic gain control.
- Final mechanical envelope and strain relief.
- Sealed charging strategy.

These are not assumed solved by a successful dry V1 test.

## 8. Bill of materials

| Ref | Qty | Part | Value / exact part |
|---|---:|---|---|
| U1 | 1 | BLE MCU board | Seeed XIAO nRF52840, SKU 102010448 |
| B1 | 1 | Protected LiPo | Akyga AKY0384 / LP601730, 3.7 V, 250 mAh |
| LED1 | 1 | Optical emitter | Optosupply OSG58A5111A, 525 nm |
| Q1 | 1 | NPN transistor | BC547B; verify purchased manufacturer pinout |
| Q2 | 1 | Phototransistor | Vishay TEPT4400; verify C/E lead mapping |
| R1 | 1 | Metal-film resistor | 100 ohm, 0.25 W or greater |
| R2 | 1 | Metal-film resistor | 10 kohm, 0.25 W or greater |
| R3 | 1 | Metal-film resistor | 10 kohm, 0.25 W or greater |
| R4 | 1 | Metal-film resistor | 100 kohm, 0.25 W or greater |
| C1 | 1 | Ceramic capacitor | 100 nF, at least 10 V |

For component identification, wiring order, continuity checks, and staged
power-up, follow the
[electronics assembly guide](../hardware/electronics-assembly-guide.md).

## 9. Primary references

- [Seeed XIAO nRF52840 documentation](https://wiki.seeedstudio.com/XIAO_BLE/)
- [Seeed XIAO nRF52840 schematic](https://files.seeedstudio.com/wiki/XIAO-BLE/Seeed_Studio_XIAO_nRF52840_PDF.pdf)
- [Akyga LP601730 specification](https://www.tme.eu/Document/aa005ead2ddfeeda132df40236910882/AKY0384.pdf)
- [Optosupply OSG58A5111A datasheet](https://www.optosupply.com/uppic/20231120303051.pdf)
- [Vishay TEPT4400 datasheet](https://www.vishay.com/docs/81341/tept4400.pdf)
- [Diotec BC546–BC549 datasheet](https://diotec.com/request/datasheet/bc546.pdf)
