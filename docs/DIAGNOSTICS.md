# rtd-acquire diagnostic research and normalization

This document records the evidence used to derive the public `rtd-acquire`
diagnostic vocabulary. The vocabulary is intentionally not frozen yet.

The purpose of this survey is to avoid designing error names around a single
converter family and then discovering that common industrial or HVAC hardware
reports the same acquisition conditions differently—or at a different level of
specificity.

## Normalization rules

1. Record the native condition before proposing a normalized condition.
2. Treat vendor-listed possible causes as troubleshooting information, not as
   observed facts.
3. Group native diagnostics only when their documented meanings genuinely
   overlap.
4. Preserve the greatest useful specificity justified by a semantic group.
5. Do not make a common diagnostic vague merely because one outlier device
   reports only a broader condition. Give the outlier a broader normalized
   concept instead.
6. Do not strengthen a broad or ambiguous native condition into a more specific
   physical diagnosis.
7. Preserve `native_code` and `native_message` whenever practical so users have
   the exact manufacturer terminology to search in datasheets, manuals, and
   troubleshooting resources.
8. A normalized concept may legitimately be as specific as the native
   diagnostic when the evidence supports that level of specificity.
9. Similar English words are not enough to establish semantic equivalence.
10. The public vocabulary may contain broad and specific sibling concepts. It
    does not need to collapse every device into one least-common-denominator
    code.
11. Distinguish **device-detectable** from **software-observable**. A condition
    can contribute native evidence to `rtd-acquire` only if the interface used
    by the backend exposes it through a register, protocol status/message,
    process-data bit, analog fault convention, or another documented observable
    path. A local-only LED or undocumented internal decision is not enough.
12. A normalized condition may be supported by a documented **combination** of
    native flags or states. Preserve that composite native evidence; do not
    invent a single vendor code merely because `rtd-acquire` presents one
    normalized diagnostic. Beckhoff EL32xx `Overrange + Error` open-circuit
    detection is the current reference example.

## Research record format

For each native diagnostic or status condition, record:

- manufacturer and exact device/family;
- documentation revision/date when available;
- native code, bit, event number, object/property, or protocol value;
- native wording;
- what the condition itself establishes;
- exposure path: how `rtd-acquire` can actually observe the condition (register,
  HART/BACnet/EtherCAT status, analog fault current, etc.);
- vendor-listed possible causes, recorded separately;
- candidate normalized `rtd-acquire` concept;
- candidate normalized plain-language message;
- semantic-equivalence peers;
- whether a usable resistance can still be reported;
- likely `WARNING`/`FAULT` behavior when this follows from measurement
  trustworthiness rather than application policy;
- source document and page/section.

## Surveyed families

### Analog Devices MAX31865

Architecture: dedicated RTD resistance-to-digital converter, SPI.

This is the first implementation target, so its native fault map is treated as
an exhaustive first-driver map rather than a representative sample.

| Native evidence | Exposure | What it establishes | Candidate normalized treatment | Measurement trust notes |
| --- | --- | --- | --- | --- |
| Fault Status D7 `RTD High Threshold` | SPI Fault Status register 07h | RTD conversion result is greater than or equal to the configured high threshold | specific resistance-high-threshold concept | The conversion value itself remains an observed resistance; likely `WARNING` if otherwise trustworthy rather than automatically `FAULT` |
| Fault Status D6 `RTD Low Threshold` | SPI Fault Status register 07h | RTD conversion result is less than or equal to the configured low threshold | specific resistance-low-threshold concept | Same distinction as D7; threshold crossing is not itself proof that the resistance conversion is unusable |
| Fault Status D5 `REFIN- > 0.85 × VBIAS` | SPI Fault Status register 07h after fault-detection cycle | Exact REFIN-/VBIAS comparison | preserve reference/electrical comparison at this specificity; do not infer wiring cause | Treat trustworthiness according to the acquisition configuration/fault cycle; do not invent an open/short diagnosis |
| Fault Status D4 `REFIN- < 0.85 × VBIAS (FORCE- open)` | SPI Fault Status register 07h after fault-detection cycle | Exact REFIN-/VBIAS comparison while FORCE- is open | preserve comparison/state specificity | Vendor cause tables are troubleshooting evidence only |
| Fault Status D3 `RTDIN- < 0.85 × VBIAS (FORCE- open)` | SPI Fault Status register 07h after fault-detection cycle | Exact RTDIN-/VBIAS comparison while FORCE- is open | preserve comparison/state specificity | Vendor cause tables are troubleshooting evidence only |
| Fault Status D2 `Overvoltage/undervoltage fault` | SPI Fault Status register 07h; always-active pin protection | At least one protected RTD/interface pin is above VDD or below GND threshold; native bit does not say which polarity/pin | broader combined voltage-fault concept | ADC conversion updates halt while this condition persists, so a newly requested trustworthy resistance is unavailable |
| RTD Data LSB D0 `Fault` | SPI RTD data register 02h | One or more MAX31865 fault conditions have been detected | summary/evidence-presence bit only; do not create a second normalized diagnostic when specific Fault Status bits are available | Read Fault Status register for the actual native evidence |
| Fault-detection cycle D3:D2 state | SPI Configuration register 00h | fault-detection cycle is running or waiting for the next manual phase | operation state, not a diagnostic by itself | Driver may wait/complete the operation; a timeout would be a driver/transport outcome, not a native MAX31865 fault bit |

The datasheet separately provides tables of possible open/short wiring causes
for combinations of the D5:D3 results. Those tables are useful for debugging,
but the register evidence itself is the voltage-comparison pattern. The driver
must therefore preserve the native pattern and may expose possible causes as
documentation/troubleshooting help, not as measured facts.

The MAX31865 also demonstrates that the manufacturer's word **fault** does not
map one-to-one to `MeasurementStatus.FAULT`. A configured high/low resistance
threshold can be crossed while a usable resistance value still exists, whereas
D2 halts conversion updates and prevents a fresh trustworthy measurement.
`rtd-acquire` severity follows measurement trustworthiness, not the vendor's
choice of register name.

Sources:

- https://www.analog.com/en/products/max31865.html
- https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31865.pdf

### Texas Instruments ADS124S08 / ADS124S06

Architecture: configurable precision 24-bit ADC/front end, SPI.

The STATUS register is now exhaustively mapped for diagnostic design. It has
eight fields, but not all eight are faults and not every diagnostic capability
is represented by a status bit.

| Native evidence / facility | Exposure | What it establishes | Candidate normalized treatment | Classification |
| --- | --- | --- | --- | --- |
| `FL_POR` | STATUS bit 7 | A power-on-reset event occurred and has not been cleared | device-reset/POR event if unexpected and operationally relevant | latched event/state; expected during initialization, not automatically a measurement warning/fault |
| `RDY` | STATUS bit 6 | ADC is or is not ready for communication | generally handle as readiness/operation state; only a failed wait/timeout should become an operation/communication failure | transient state, not a native acquisition fault by itself |
| `FL_P_RAILP` | STATUS bit 5 | positive PGA output is within 150 mV of AVDD | preserve positive-PGA-output / positive-rail specificity | conversion-cycle fault flag |
| `FL_P_RAILN` | STATUS bit 4 | positive PGA output is within 150 mV of AVSS | preserve positive-PGA-output / negative-rail specificity | conversion-cycle fault flag |
| `FL_N_RAILP` | STATUS bit 3 | negative PGA output is within 150 mV of AVDD | preserve negative-PGA-output / positive-rail specificity | conversion-cycle fault flag |
| `FL_N_RAILN` | STATUS bit 2 | negative PGA output is within 150 mV of AVSS | preserve negative-PGA-output / negative-rail specificity | conversion-cycle fault flag |
| `FL_REF_L1` | STATUS bit 1 | differential external reference is below 1/3 of `(AVDD - AVSS)` | `REFERENCE_LOW` family with threshold retained natively | conversion-cycle reference-monitor flag; TI notes it can be used to detect an open excitation lead in a specific 3-wire RTD topology, but that application-specific possible cause is not the generic reported fact |
| `FL_REF_L0` | STATUS bit 0 | differential external reference is below 0.3 V | `REFERENCE_LOW` family with threshold retained natively | conversion-cycle reference-monitor flag; TI says it can indicate a missing/floating reference, not that it uniquely proves one |
| conversion-data CRC mismatch | CRC byte appended to conversion data when enabled; mismatch is detected by the driver by comparing the received CRC with its calculation | TI explicitly defines a mismatch as a data-transmission error | data-CRC/data-integrity concept; preserve that this is driver-derived from device-provided CRC rather than a STATUS bit | integrity check; normally makes that received conversion untrustworthy |
| supply-monitor result | active `SYS_MON` ADC measurement of `(AVDD-AVSS)/4` or `DVDD/4` | an actual supply-related ADC measurement | no native `SUPPLY_FAULT` merely because the facility exists; a project/user threshold would be a separately configured assessment | active measurement facility, not automatic fault flag |
| burnout-current result | active `SYS_MON` measurement with selectable 0.2/1/10 µA burnout current sources | a full-scale or near-zero test result under the selected burnout test | if exposed, describe the test result rather than automatically `SENSOR_CIRCUIT_OPEN`/`SENSOR_CIRCUIT_SHORT` | active diagnostic test, not automatic status bit |
| SPI timeout facility | optional interface timeout resets SPI when a complete byte is not received within the documented interval | interface recovered from an interrupted serial transaction | no native timeout flag is exposed; a backend can report only what its own transaction/timing observation establishes | interface behavior, not STATUS diagnostic |
| offset/gain calibration commands and OFC/FSC registers | SPI commands/registers | calibration operation and resulting calibration coefficients | **no ADS124S08 `CALIBRATION_ERROR` mapping currently justified**: the datasheet documents calibration commands/results but no explicit calibration-failure status flag | calibration facility, not documented failure diagnostic |

The burnout facility is particularly important for the no-inference rule. TI
states that a full-scale result can be caused by an open sensor, an overloaded
sensor, or an absent reference. A near-zero reading can indicate a shorted
sensor, but the datasheet warns that distinguishing the shorted condition from
other cases can be difficult. `rtd-acquire` must therefore not turn these test
outcomes into an unconditional `SENSOR_CIRCUIT_OPEN` or `SENSOR_CIRCUIT_SHORT` diagnosis.
Likewise, an ordinary conversion code at positive or negative full scale is
not by itself proof of ADC saturation: the documented transfer function clips
there for signals at or beyond full scale, but the code does not encode which
side of that boundary produced it. `ADC_SATURATION` therefore remains supported
by devices such as AD7124 that expose an explicit saturation diagnostic, not by
inference from an ADS124S08 endpoint code alone.

The two reference flags may be simultaneously true for a sufficiently low
reference when both monitors are enabled. A future driver may normalize the
native combination to one user-facing low-reference diagnostic while preserving
both contributing native flags/thresholds. This is another example where one
normalized diagnostic can legitimately be supported by composite native
evidence.

Sources:

- https://www.ti.com/product/ADS124S08
- https://www.ti.com/lit/ds/symlink/ads124s08.pdf

### Analog Devices AD7124-4 / AD7124-8

Architecture: configurable precision 24-bit ADC/front end, SPI.

The error register includes highly specific acquisition-chain evidence,
including:

- ADC saturation;
- invalid conversion;
- calibration failure;
- positive input overvoltage and undervoltage;
- negative input overvoltage and undervoltage;
- external-reference detection failure;
- analog/digital supply-monitor failures;
- SPI ignored, clock-count, read, and write errors;
- SPI CRC errors;
- memory-map CRC errors;
- ROM CRC errors;
- LDO decoupling-capacitor diagnostics.

`REF_DET_ERR` is intentionally broader than a simple open-reference diagnosis:
the documented condition can mean an external reference is open or below the
specified voltage threshold. A normalized diagnostic must retain that
ambiguity.

Source:

- https://www.analog.com/media/en/technical-documentation/data-sheets/ad7124-8.pdf

### Pepperl+Fuchs KFD0-TR-1

Architecture: industrial Pt100 converter/transmitter, 4–20 mA.

The data sheet explicitly defines sensor burnout as an upscale fault output of
at least 22 mA. Because the converter itself assigns that meaning, a normalized
`SENSOR_BURNOUT` concept can preserve the native level of specificity.

Source:

- https://files.pepperl-fuchs.com/webcat/navi/productInfo/pds/038307_eng.pdf

### Rosemount 644

Architecture: industrial temperature transmitter, 4–20 mA/HART.

Documented diagnostic messages include conditions such as:

- `Sensor Open`;
- `Sensor Shorted`;
- `Reference Error`;
- `ASIC RCV Error`;
- `ASIC TX Error`;
- `ASIC Configuration Error`;
- sensor operating-range/limit conditions.

`Sensor Open` and `Sensor Shorted` are explicit device diagnoses. They provide
strong evidence that normalized open-circuit and short-circuit concepts can
exist at that same useful level of specificity rather than being weakened to a
generic `SENSOR_FAULT`. The current working names are
`SENSOR_CIRCUIT_OPEN` and `SENSOR_CIRCUIT_SHORT`.

The Rosemount `Reference Error` must not automatically be grouped with a
low-reference-voltage condition from an ADC: Rosemount documents a different
reference-resistor integrity meaning. This is a useful example of similar
vendor words that do not necessarily represent the same semantic group.

Source:

- https://www.emerson.com/is/content/emerson/en/measurement-instrumentation/technical/products/temperature/documents/doc-rosemount-00809-0400-4728.pdf

### Endress+Hauser iTEMP TMT82

Architecture: industrial temperature transmitter, 4–20 mA/HART.

Event-numbered sensor diagnostics include:

- `041 Sensor broken`;
- `042 Sensor corroded`;
- `043 Short-circuit`;
- `044 Sensor drift`;
- `045 Working area`.

`043 Short-circuit` is a strong peer for the normalized shorted-input-circuit
semantic group. `041 Sensor broken` remains separate from
`SENSOR_CIRCUIT_OPEN` until the manufacturer documentation establishes that the
event specifically represents an open circuit rather than a broader
sensor/wiring failure.

Source:

- https://bdih-download.endress.com/file/e2ba15e25aed4e6cc6e369f66cca69ff/BA01028TEN_2624-00.pdf

### Siemens SITRANS TH320 / TH420 family

Architecture: universal industrial temperature transmitter, 4–20 mA/HART or
PROFIBUS PA depending on model.

This family is especially relevant to `rtd-acquire` because Siemens documents
broad resistance-sensor support and an explicit linear-ohms mode. Published
configuration/accuracy tables include Pt, Ni, and Cu families, including
Pt500, Pt1000, Ni120, and Ni1000. That makes it a strong industrial candidate
for project-wide RTD-family parity without requiring the transmitter's internal
temperature interpretation.

The 2021 operating instructions expose structured diagnostic IDs including:

- `8A Input 1 error` — a sensor error described as broken/shorted sensor is
  detected at Input 1;
- `8B Input 2 error` — same class of sensor error at Input 2;
- `8C Input 1 CJC error`;
- `8D Input 2 CJC error`;
- `8E Drift detected` — difference between Input 1 and Input 2 exceeds the
  configured maximum;
- `8F Backup enabled` — a sensor error occurred and the backup sensor is being
  used;
- `8G Backup error` — sensor error at the backup sensor, leaving no backup;
- `8H Drift detected, reference voltage FVR` — critical measurement error at
  the internal voltage reference;
- `8J Drift detected, reference voltage VREF` — critical measurement error at
  the internal voltage reference;
- `8L Drift detected at Input 1` and `8n Drift detected at Input 2` — critical
  measurement error at the named input;
- `8o Drift detected, ground voltage offset to terminal 3` — critical
  measurement error in the named ground-offset check;
- `bF Configuration not supported by device` — configuration is invalid or not
  supported, with behavior depending on how long the invalid state persists.

The important normalization lesson is `8A`/`8B`: even though Siemens says the
input monitoring detects broken/shorted sensors, the diagnostic ID exposed at
this level is `Input ... error` and does not distinguish which one occurred.
That makes it a candidate for the broader `SENSOR_INPUT_FAULT` concept rather
than forcing the more-specific sensor-circuit-open and sensor-circuit-short
groups to become vague.

The functional-safety documentation adds an important backend/configuration
nuance. The transmitter can be configured to detect `Broken`, `Shorted`, or
`Broken+Short`, and it provides separately configurable analog fault-current
values for broken-sensor and shorted-sensor alarms. This means the hardware's
internal capability is more specific than the HART `8A`/`8B` message. An
analog-current backend could preserve that specificity **only** when the
transmitter configuration is known and the configured current values uniquely
identify the two conditions. If the values overlap, or the backend has only the
broad HART input-error diagnostic, `rtd-acquire` must emit the broader
`SENSOR_INPUT_FAULT` diagnosis.

`8E Drift detected` is substantially more specific and appears semantically
similar to dual-sensor drift diagnostics from other industrial transmitters.
That is a strong candidate for a normalized `SENSOR_DRIFT` concept.

Sources:

- https://www.siemens.com/en-us/products/sitrans/th320-th420/
- https://cache.industry.siemens.com/dl/files/747/109802747/att_1082127/v1/A5E41864807-ADen_TH320420_TR320420_TF320420_HART_OI_en-US.pdf
- https://cache.industry.siemens.com/dl/files/072/109793072/att_1053491/v1/A5E41864869-ACen_THTRTF320420_TS500_FunctSafety.pdf
- https://cache.industry.siemens.com/dl/files/815/109764815/att_1268354/v1/FI01_us_kap02.pdf

### Beckhoff EL32xx RTD EtherCAT terminals

Architecture: industrial EtherCAT RTD/resistance-input modules.

The EL3202 family is particularly attractive for `rtd-acquire` because the
measured resistance can be output directly in ohms rather than converted to
temperature in the terminal. Current product documentation lists Pt100,
Pt200, Pt500, Pt1000, Ni100, Ni120, Ni1000, and direct resistance measurement
up to 4 kΩ depending on mode. Variants support different wiring/accuracy needs.

The current EL32xx process image documents software-observable per-channel
status including:

- `Underrange` — measurement range undershot;
- `Overrange` — measurement range exceeded;
- `Error` — process data invalid;
- `TxPDO State` — explicit validity of the associated process data;
- configurable `Limit 1` and `Limit 2` threshold states.

Importantly, Beckhoff documents `Overrange` together with `Error` as the
terminal's **open-circuit detection**. That means a backend observing that
specified combination can legitimately preserve an open-circuit diagnosis; it
need not collapse the condition into a generic sensor-circuit fault. The
current status-word documentation does not similarly establish that
`Underrange` alone means sensor short, so `rtd-acquire` must not infer that.

The separate overrange/underrange states should also not be conflated with
MAX31865 user-configurable resistance thresholds: exceeding a hardware
measurement range and crossing a configured alarm threshold are different
semantics even if both involve a high or low value.

This family also demonstrates that diagnostic interpretation can depend on a
**combination of native bits**, not necessarily one native code. For example,
`Overrange` by itself reports a range condition; the documented
`Overrange + Error` combination supports the stronger open-circuit diagnosis.
The backend should preserve the contributing native status bits in its native
evidence.

Sources:

- https://www.beckhoff.com/en-us/products/i-o/ethercat-terminals/el-ed3xxx-analog-input/el3202-0010.html
- https://infosys.beckhoff.com/content/1033/el32xx/10195494283.html
- https://infosys.beckhoff.com/content/1033/el32xx/10188876171.html
- https://download.beckhoff.com/download/document/io/ethercat-terminals/el32xx_en.pdf

### Phoenix Contact MINI MCR-2-RTD-UI

Architecture: configurable industrial RTD/resistance transducer with current or
voltage output.

The detailed device documentation explicitly says the module detects:

- cable/open circuit;
- short circuit;
- overrange;
- underrange;
- module errors.

The device can signal errors through the analog output, group fault-monitoring
system, optional switching output, and red LED. The analog mapping is itself
configuration-dependent. Some DIP-switch configurations intentionally assign
different analog values to cable break and short circuit, while others map them
to the same value. NE43 configurations can likewise collapse multiple error
classes into a common upscale or downscale value.

This is a particularly useful normalization example: the hardware can detect
specific conditions, but an **analog-only backend may or may not be able to
recover that specificity depending on the configured signaling table**. A
backend must therefore know the device's error-signaling configuration before
turning an analog fault value into cable-break, sensor-short, overrange, or
underrange evidence. A group fault-monitoring contact, by itself, is broader and
must not be treated as identifying the individual cause.

The module supports Pt, Ni, and Cu sensor types and linear resistance from 0 Ω
to 4000 Ω, making it a strong industrial analog and RTD-family-coverage target.

Sources:

- https://www.phoenixcontact.com/en-us/products/temperature-transmitter-mini-mcr-2-rtd-ui-2902049
- https://media.digikey.com/pdf/Data%20Sheets/Phoenix%20Contact%20PDFs/2902049_Ds.pdf

### WIKA T32.xS

Architecture: universal industrial temperature transmitter, 4–20 mA/HART.

The current operating instructions and safety documentation establish detection
of a substantially richer set of conditions than the analog alarm current can
identify by itself. Documented monitoring includes:

- sensor break;
- sensor short circuit for resistance-temperature sensors;
- inadmissibly high lead resistance where the wiring mode supports that check;
- sensor upper-limit and lower-limit violations;
- optional dual-sensor drift monitoring;
- configuration error;
- internal communication errors;
- ROM, EEPROM, RAM, program-counter, and stack-pointer errors;
- device-temperature and output-limit monitoring in applicable configurations;
- cyclic self-monitoring.

The manual also provides an important exposure example. The analog fault-tree
shows that a loop current below 4 mA or above 20 mA can correspond to multiple
conditions, including process value outside range, sensor burnout or short
circuit, and wrong sensor connection. Therefore **the analog current alone does
not support a specific `SENSOR_BURNOUT` or sensor-circuit-short diagnosis**. An
analog-only backend would need a broader transmitter/sensor-input alarm concept.

WIKA's current product/FAQ material explicitly states that HART instruments
set a status bit when a problem occurs and that an associated error message can
then be read with the corresponding HART command. The current HART configuration
tree also exposes a software-observable `Diagnostics/Service` path with device
`Status`, self-test, reset, and device-malfunction information. WIKA publishes a
T32.xS HART Device Description package for integration tools as well. Together
these establish that the HART path can expose richer diagnostic information than
the analog loop.

What the public documentation found so far does **not** provide is a stable,
one-to-one table from every internally detected T32.xS condition to the exact
HART/DD native identifier and message that a backend would receive. Therefore a
future WIKA HART backend may preserve the HART-provided status/message once the
actual DD/command behavior is verified, but the catalog must not invent those
identifiers in advance. Exact HART/DD identifiers remain a targeted research
item before a WIKA HART backend claims sensor-break, sensor-short,
high-lead-resistance, drift, or internal-memory faults at the native-code level.

This remains a direct example of why internal detection capability and
backend-observable evidence must be recorded separately.

Sources:

- https://www.wika.com/en-us/t32_xs.WIKA
- https://www.wika.co.jp/upload/OI_T32_xS_en_de_10181.pdf
- https://www.wika.com/media/Operating-instructions/Safety-manuals/oi_t32xs_safetymanual_v223_en_de_fr_es.pdf
- https://www.wika.com/en-eg/lp_temperature_transmitter.WIKA
- https://www.wika.com/en-us/document_categories_seo_html_36666.WIKA

### ABB TTH300

Architecture: universal head-mount temperature transmitter with HART and other
industrial fieldbus variants.

Current ABB HART documentation provides unusually clear native status wording.
Documented sensor/device status messages include:

- `S1 short-circuit` / `S2 short-circuit`;
- `S1 Wire break / sensor break` / the corresponding S2 condition;
- line resistance too high for S1 or S2;
- sensor drift detected;
- S1/S2 over sensor range and under sensor range;
- non-volatile data defect;
- device not calibrated;
- electronics failure;
- parameterization/configuration failure;
- diagnostics or analog-output simulation states.

Older/local diagnostic-number views of the same family also expose distinct
short-circuit and wire-break diagnostic numbers. This is strong cross-vendor
evidence that a sensor-circuit-short concept, an open/broken-circuit family,
`LEAD_RESISTANCE_HIGH`, `SENSOR_DRIFT`, sensor-range high/low, configuration,
simulation, and internal-integrity concepts can remain specific in the
normalized vocabulary.

The exact ABB wording `Wire break / sensor break` should be preserved natively.
It should not automatically be normalized to `SENSOR_CIRCUIT_OPEN` until we decide whether
Rosemount `Sensor Open`, Phoenix `Cable break`, ABB `Wire break / sensor break`,
and similar diagnoses are semantically equivalent enough to share one
normalized open-circuit concept.

Source:

- https://library.e.abb.com/public/014c7e73e81c4fd59b46123b7304c2b3/OI_TTH300_EN_H01.pdf

### Yokogawa YTA610 / YTA710

Architecture: universal industrial temperature transmitters with HART or
FOUNDATION Fieldbus variants.

The HART documentation exposes numbered alarms and status information to the
communicator. Relevant examples include sensor failure, sensor signal out of
measurable range, sensor-backup failures, sensor drift, and terminal-sensor
failure. The YTA710 additionally supports explicit RTD/ohms short-circuit
detection for 3- and 4-wire measurements and exposes `S1 Short` / `S2 Short`
alarms.

This family reinforces two normalization rules:

1. specific short-circuit diagnostics are common enough to deserve their own
   normalized concept; and
2. capabilities must be recorded per model/revision rather than inherited
   across a whole family—an explicit YTA710 short diagnostic must not be claimed
   for a YTA610 backend unless its own documentation establishes it.

Sources:

- https://www.yokogawa.com/us/solutions/products-and-services/measurement/field-instruments-products/temperature-transmitters/field-mount/yta610-temperature-transmitter/
- https://www.manualslib.com/manual/1656571/Yokogawa-Yta610.html

### Siemens Desigo Essentials EM1.8U

Architecture: HVAC/building universal-I/O expansion module with Modbus RTU.

This is a particularly valuable `rtd-acquire` reference because Siemens
explicitly supports both temperature-sensor modes and **raw resistance modes**.
The EM1.8U can expose `R1000` and `R10000` measurements in ohms through Modbus;
the `R1000` range is 700–1800 Ω, which covers the electrical range of common
Pt1000/Ni1000 building sensors without requiring the module's internal
resistance-to-temperature conversion.

The same Modbus interface provides a per-channel reliability register. Current
documentation defines these machine-readable values:

- `0x0000` — `No error`;
- `0x7FFA` — `Other error`;
- `0x7FFB` — `No sensor`;
- `0x7FFC` — `Under range`;
- `0x7FFD` — `Short circuit`;
- `0x7FFE` — `Over range`.

For under-range, short-circuit, and over-range reliability states, the module
retains the last valid channel value; `No sensor` and `Other error` report zero.
That retained value is **not a current trustworthy measurement**, so an
`rtd-acquire` backend must not treat it as a usable resistance merely because a
numeric register value is present.

This family gives us a modern HVAC/building example where the software exposure
path is fully documented: resistance and reliability are separate Modbus
registers. It strongly supports normalized concepts for short circuit,
overrange, underrange, and a broader other-error case. `No sensor` should remain
its own native concept until we establish whether it is semantically equivalent
to a physical open circuit across the relevant Siemens configurations.

Sources:

- https://sid.siemens.com/r/A6V14300949/25901888779___en-US_26440986635
- https://sid.siemens.com/r/A6V13841491/25014743435___en-US_25015603339
- https://sid.siemens.com/r/A6V13841491/25014743435___en-US_25015601803

### Honeywell Unitary controllers

Architecture: HVAC/building-automation universal-I/O controller with BACnet IP,
BACnet MS/TP, or BACnet T1L depending on model.

The current installation instructions are unusually relevant to `rtd-sensor`
parity. Universal inputs include Pt100, Pt1000, Ni1000 TK5000, Ni1000 Class B
DIN 43760, and a custom resistive characteristic covering 100 ohms to 100 kOhms.

Honeywell explicitly documents recognition thresholds for **sensor break (SB)**
and **short circuit (SC)** for selected sensor types. In particular:

- Pt1000: short-circuit recognition below 775 ohms; sensor-break recognition at
  the documented lower boundary;
- Ni1000 TK5000: short-circuit recognition below 850 ohms; sensor-break
  recognition at the documented lower boundary.

The controller also uses distinct local LED indications for broken sensor and
short circuit. This is strong evidence that the controller internally
distinguishes those conditions rather than exposing only a generic sensor
fault.

However, the current installation manual does not yet establish how that
distinction is exposed to a BACnet client as a machine-readable point status.
Until the BACnet/Niagara object exposure is documented, these conditions are
**device-detectable but not yet proven software-observable to an `rtd-acquire`
backend**. They therefore strengthen the semantic case for a specific sensor-break/open
family and a sensor-circuit-short concept, but `SENSOR_BREAK` must not be
collapsed into `SENSOR_CIRCUIT_OPEN` until the physical semantics and software
exposure are verified.
These detections must not yet be treated as a completed backend mapping.

Sources:

- https://prod-edam.honeywell.com/content/dam/honeywell-edam/hbt/en-us/documents/manuals-and-guides/installation-guides/hbt-bms-unitarycontroller24V-installationinstructions.pdf
- https://buildings.honeywell.com/us/en/products/by-category/control-panels/building-controls/zone-and-unitary-controllers/universal-bacnet-module

### Siemens Desigo PXC4 universal inputs

Architecture: HVAC/building-automation controller with universal inputs and
BACnet exposure.

Representative PXC4 universal inputs support LG-Ni1000, Pt1000, and raw
resistance modes, including 0–1000 Ω and 0–2500 Ω measurement ranges with
0.1 Ω resolution on current models. The BACnet analog-input objects expose
standard `reliability`, `status-flags`, and `event-state` properties, so point
health is software-observable. The exact reliability values the PXC4 firmware
assigns to Ni1000/Pt1000/raw-resistance wiring faults are still not documented
in the sources reviewed so far.

Until those values are established, a PXC4 backend may preserve the BACnet
reliability/status evidence it actually reads but must not invent sensor-open or
sensor-short diagnoses from generic point unreliability.

Sources:

- https://sid.siemens.com/r/A6V12957862/19744489483_21457179787__en-US_19935587083
- https://sid.siemens.com/r/A6V12954388/20185284747_28090979851__en-US_19516361099

## Detection versus exposure

The survey must distinguish three levels:

1. **Vendor-documented detection** — the hardware/controller says it can detect
   a condition.
2. **Native software exposure** — the condition has a documented value that the
   backend can read through its chosen interface.
3. **Normalized `rtd-acquire` mapping** — the exposed native evidence has been
   assigned a stable project code/message without strengthening its meaning.

Only level 2 can become `native_code`/`native_message` evidence in a real
backend. Level 1 is still valuable for vocabulary research, but it cannot be
reported by `rtd-acquire` unless an observable path is found.

Examples:

- MAX31865 fault-register bits: detected **and** directly observable over SPI.
- KFD0-TR-1 burnout: observable through the documented >=22 mA analog fault
  output.
- Beckhoff EL32xx open-circuit/range/data-validity status: observable in
  EtherCAT process/status data.
- Siemens EM1.8U raw resistance plus per-channel reliability: observable in
  documented Modbus registers.
- Honeywell Unitary sensor-break/short recognition: documented detection is
  clear, but BACnet exposure still needs verification.

## Emerging semantic groups

The identifiers below are candidate concepts, not frozen API spelling.

### Strongly supported specific groups

| Candidate concept | Current evidence | Notes |
| --- | --- | --- |
| `SENSOR_CIRCUIT_OPEN` | Rosemount explicit `Sensor Open`; Beckhoff EL32xx documented `Overrange + Error` open-circuit detection; Phoenix `Cable break`; ABB `Wire break / sensor break` supports the family but still merits exact-equivalence review | Current preferred working name because it states the electrical input-circuit condition without claiming whether the break is in the RTD element or field wiring; iTEMP `Sensor broken` remains outside the group pending clarification |
| `SENSOR_CIRCUIT_SHORT` | Rosemount `Sensor Shorted`; iTEMP `043 Short-circuit`; ABB S1/S2 short-circuit; Yokogawa YTA710 S1/S2 Short; Phoenix short-circuit detection; Siemens EM1.8U `0x7FFD Short circuit` | Strong repeated semantic group; circuit wording avoids locating the short specifically in the RTD element; Siemens SITRANS 8A/8B remains a broader outlier |
| `SENSOR_BURNOUT` | KFD0-TR-1 explicitly defines sensor burnout | Keep distinct from generic sensor fault/open unless evidence later proves equivalence |
| `SENSOR_DRIFT` | Siemens 8E; iTEMP 044; ABB sensor drift; Yokogawa AL.25; WIKA dual-sensor drift monitoring | Strong recurring concept, usually based on disagreement between redundant/dual sensors; retain each device's configured threshold and exact semantics natively |
| `LEAD_RESISTANCE_HIGH` | ABB explicit S1/S2 line-resistance high; WIKA detects inadmissibly high lead resistance | Strong acquisition-specific concept; WIKA software-visible identifier still needs exact HART/DD confirmation |
| `SENSOR_RANGE_HIGH` / `SENSOR_RANGE_LOW` | ABB over/under sensor range; Yokogawa sensor signal out of measurable range | Sensor/model-oriented transmitter range semantics; keep distinct from raw electrical input range and user-configured resistance thresholds |
| `INPUT_OVERRANGE` / `INPUT_UNDERRANGE` | Beckhoff EL32xx process status; Siemens EM1.8U `0x7FFE` / `0x7FFC` | Strong recurring raw-acquisition range concepts; do not silently reinterpret as sensor open/short unless the device documents that stronger combination |
| `REFERENCE_LOW` | ADS124S08 `FL_REF_L0` / `FL_REF_L1` | Thresholds differ and remain in native evidence; semantic group is low monitored reference |
| `ADC_SATURATION` | AD7124 explicit saturation | Seek peers in other ADC families before final wording |
| `CALIBRATION_ERROR` | AD7124 explicit ADC calibration error; industrial transmitter internal calibration errors | ADS124S08 does **not** currently join this group: it has calibration commands/registers but no documented calibration-failure status flag. Keep ADC/system calibration failure distinct from RTD-model calibration, which is outside `rtd-acquire` |
| data-CRC / integrity family | ADS124S08 conversion-data CRC mismatch (driver comparison of device-provided CRC); AD7124 SPI/memory/ROM CRC flags | Keep transmission-data integrity distinct from register/memory/ROM integrity where the failure domain matters; ADS124S08 has no CRC-error STATUS bit |

### Broad-outlier concepts likely needed

| Candidate concept | Why it is needed |
| --- | --- |
| `SENSOR_FAULT` or `SENSOR_INPUT_FAULT` | Siemens 8A/8B expose an input sensor error without distinguishing broken vs shorted |
| `NO_SENSOR` or `SENSOR_NOT_DETECTED` | Siemens EM1.8U exposes explicit `No sensor`, but equivalence to physical open circuit is not yet established and should not be assumed |
| `REFERENCE_FAULT` | AD7124 reference-detection error can mean open reference or reference below threshold |
| `VOLTAGE_FAULT` | MAX31865 combines overvoltage/undervoltage in one native flag |
| `HARDWARE_FAULT` | Some devices expose only a broad internal-device failure condition |

These broad concepts must coexist with, not replace, the more-specific groups.

## Semantics that must remain separate

Some superficially similar conditions should currently remain separate:

- **configured resistance high/low threshold** vs. **hardware overrange/
  underrange**;
- **reference below a measured threshold** vs. **reference resistor integrity
  error** vs. **reference open-or-low combined detection**;
- **sensor open** vs. **sensor broken** vs. **wire-break-or-short combined
  failure**;
- **burnout test result** vs. **device-declared sensor burnout**;
- **transport CRC** vs. **memory/register/ROM integrity CRC** when the domain of
  corruption matters;
- **sensor drift** vs. **reference-voltage drift/internal reference failure**;
- **device-internal communication/ASIC faults** vs. **failure of the transport
  used by `rtd-acquire` to communicate with that device**;
- **device-internal sensor monitoring** vs. **what a 4–20 mA alarm value alone
  can identify**; a single analog fault level may intentionally collapse several
  native/internal causes.

## Candidate normalized vocabulary worksheet

This is a naming/semantics worksheet, **not** the frozen `DiagnosticCode` enum.
It exists so every proposed code and message can be challenged against the
native evidence before becoming public API.

Naming goals:

- use plain acquisition/electrical language rather than vendor jargon;
- make the code no more specific than its evidence, but no less specific merely
  to accommodate a coarser outlier;
- describe an RTD **input circuit** rather than claiming the RTD element itself
  failed when the native diagnosis could also arise from field wiring;
- keep configured resistance thresholds, hardware input ranges, and a
  transmitter's sensor/model ranges distinct;
- keep external/reference-measurement problems distinct from internal-reference
  drift/integrity failures;
- keep data/transport integrity distinct from device memory/ROM integrity;
- do not create a public diagnostic for a mere operating state such as normal
  startup readiness.

### Sensor/input-circuit family

| Working code | Working normalized message | Evidence status / notes |
| --- | --- | --- |
| `SENSOR_CIRCUIT_OPEN` | The acquisition device reports an open RTD input circuit. | Preferred working name for Rosemount `Sensor Open`, Beckhoff documented open-circuit combination, Phoenix cable break, and likely ABB wire/sensor break. The phrase *input circuit* avoids claiming whether the break is in the RTD element or field wiring. Keep iTEMP `Sensor broken` separate until semantics are confirmed. |
| `SENSOR_CIRCUIT_SHORT` | The acquisition device reports a shorted RTD input circuit. | Strong repeated group: Rosemount, iTEMP, ABB, Yokogawa YTA710, Phoenix, Siemens EM1.8U. The circuit wording avoids overclaiming that the RTD element itself is necessarily the short location. |
| `SENSOR_BURNOUT` | The acquisition device reports sensor burnout. | Currently justified directly by KFD0-TR-1. Do not merge with open circuit merely because some industries use burnout loosely. |
| `SENSOR_INPUT_FAULT` | The acquisition device reports an RTD input fault but does not identify the condition more specifically. | Broad sibling for Siemens SITRANS 8A/8B and similar coarse native diagnoses. Must not replace the specific open/short concepts above. |
| `SENSOR_NOT_DETECTED` | The acquisition device reports that no sensor is detected. | Siemens EM1.8U gives explicit `No sensor`; do not equate it automatically with open circuit. |
| `SENSOR_DRIFT` | The acquisition device reports excessive sensor-input drift or disagreement. | Supported across Siemens, Endress+Hauser, ABB, Yokogawa, WIKA; native thresholds and whether the test compares redundant inputs remain native evidence. |
| `LEAD_RESISTANCE_HIGH` | The acquisition device reports excessive RTD lead resistance. | Strong evidence from ABB and WIKA; keep separate from open/short. |

### Resistance/range family

| Working code | Working normalized message | Evidence status / notes |
| --- | --- | --- |
| `RESISTANCE_HIGH_THRESHOLD` | The measured resistance met or exceeded the configured high threshold. | Direct MAX31865 D7 semantics. A usable resistance can still exist, so the likely status is `WARNING` when no other condition invalidates it. |
| `RESISTANCE_LOW_THRESHOLD` | The measured resistance met or fell below the configured low threshold. | Direct MAX31865 D6 semantics; same trustworthiness distinction as the high threshold. |
| `INPUT_OVERRANGE` | The acquisition input is above its supported measurement range. | Beckhoff and Siemens EM1.8U raw-acquisition semantics. Do not silently reinterpret as open circuit unless a documented native combination establishes that stronger diagnosis. |
| `INPUT_UNDERRANGE` | The acquisition input is below its supported measurement range. | Beckhoff and Siemens EM1.8U raw-acquisition semantics. Do not silently reinterpret as short circuit. |
| `SENSOR_RANGE_HIGH` | The acquisition device reports that its configured sensor range has been exceeded on the high side. | Industrial-transmitter concept. This preserves a native device report; `rtd-acquire` itself must not derive an RTD-model range conclusion. Final code spelling may need an explicit `DEVICE_`/`REPORTED_` cue if review finds the current name too easy to confuse with `rtd-sensor` interpretation. |
| `SENSOR_RANGE_LOW` | The acquisition device reports that its configured sensor range has been exceeded on the low side. | Same boundary concern as `SENSOR_RANGE_HIGH`. |

### Reference/electrical-front-end family

| Working code | Working normalized message | Evidence status / notes |
| --- | --- | --- |
| `REFERENCE_LOW` | The monitored acquisition reference is below its applicable threshold. | ADS124S08 `FL_REF_L0`/`FL_REF_L1`; native evidence retains which threshold(s) fired. |
| `REFERENCE_FAULT` | The acquisition device reports a reference fault that cannot be identified more specifically. | Broad sibling for AD7124 open-or-low reference detection and other coarse native reference failures. Do not weaken `REFERENCE_LOW` to accommodate it. |
| `REFERENCE_INPUT_ABOVE_THRESHOLD` | The reference input is above the converter's fault-detection threshold. | Working normalization for MAX31865 D5; retain the exact `0.85 × VBIAS` comparison natively. |
| `REFERENCE_INPUT_BELOW_THRESHOLD` | The reference input is below the converter's fault-detection threshold. | Working normalization for MAX31865 D4; retain FORCE-open test state and threshold natively. |
| `RTD_INPUT_BELOW_THRESHOLD` | The RTD input is below the converter's fault-detection threshold. | Working normalization for MAX31865 D3; retain FORCE-open test state and threshold natively. |
| `INPUT_VOLTAGE_FAULT` | The acquisition device reports an input-voltage fault but does not distinguish overvoltage from undervoltage. | MAX31865 D2. More-specific `INPUT_OVERVOLTAGE` / `INPUT_UNDERVOLTAGE` siblings remain appropriate for hardware that actually distinguishes polarity/direction. |

The MAX31865 D5:D3 working names intentionally use explicit threshold language rather than the
shorter cross-vendor concepts. They describe real observable comparisons unique
to that fault-detection architecture. Research may reveal better peers/names, but they
must not be translated into open/short diagnoses merely to make the vocabulary
look simpler.

### PGA/ADC and electrical-monitor family

The ADS124S08 exposes four separate PGA rail conditions. Because that native
specificity is real, the current worksheet retains all four rather than
collapsing them into one `PGA_RAIL_FAULT` merely for brevity. The AD7124 adds
similarly specific input-voltage and power-monitor diagnostics that should not
be thrown away merely because the MAX31865 lacks peers for them.

| Working code | Working normalized message | Evidence status / notes |
| --- | --- | --- |
| `PGA_POSITIVE_OUTPUT_NEAR_POSITIVE_RAIL` | The positive PGA output is near the positive supply rail. | ADS124S08 native flag. |
| `PGA_POSITIVE_OUTPUT_NEAR_NEGATIVE_RAIL` | The positive PGA output is near the negative supply rail. | ADS124S08 native flag. |
| `PGA_NEGATIVE_OUTPUT_NEAR_POSITIVE_RAIL` | The negative PGA output is near the positive supply rail. | ADS124S08 native flag. |
| `PGA_NEGATIVE_OUTPUT_NEAR_NEGATIVE_RAIL` | The negative PGA output is near the negative supply rail. | ADS124S08 native flag. |
| `POSITIVE_INPUT_OVERVOLTAGE` | The positive ADC input is above its allowed voltage range. | AD7124 `AINP_OV_ERR`; retain the positive-input distinction. |
| `POSITIVE_INPUT_UNDERVOLTAGE` | The positive ADC input is below its allowed voltage range. | AD7124 `AINP_UV_ERR`. |
| `NEGATIVE_INPUT_OVERVOLTAGE` | The negative ADC input is above its allowed voltage range. | AD7124 `AINM_OV_ERR`. |
| `NEGATIVE_INPUT_UNDERVOLTAGE` | The negative ADC input is below its allowed voltage range. | AD7124 `AINM_UV_ERR`. |
| `ADC_SATURATION` | The acquisition device reports ADC saturation. | AD7124 explicit modulator-saturation flag. |
| `CONVERSION_ERROR` | The acquisition device reports an ADC conversion error. | AD7124 explicitly says the conversion is invalid. |
| `CALIBRATION_ERROR` | The acquisition device reports an acquisition-calibration failure. | AD7124 explicit failed-calibration flag; not justified for ADS124S08 merely because it supports calibration commands. |
| `ANALOG_SUPPLY_FAULT` | The acquisition device reports that its analog supply monitor is outside specification. | AD7124 analog-LDO monitor. |
| `DIGITAL_SUPPLY_FAULT` | The acquisition device reports that its digital supply monitor is outside specification. | AD7124 digital-LDO monitor. |
| `LDO_DECOUPLING_FAULT` | The acquisition device reports a required LDO decoupling-capacitor fault. | AD7124 explicit diagnostic; deliberately specific even though few peers currently exist. |

`ADC_SATURATION` and `CALIBRATION_ERROR` must only be emitted when the device or
backend has evidence for those conditions. ADS124S08 endpoint output codes and
calibration facilities do not by themselves support those diagnoses. Likewise,
ADS124S08's AVDD/DVDD monitor mux inputs are measurements, not automatic
`ANALOG_SUPPLY_FAULT`/`DIGITAL_SUPPLY_FAULT` flags unless a backend applies an
explicitly defined assessment rule.

### Integrity/internal-device family

The survey argues against one generic `CRC_ERROR`: the affected domain matters
for troubleshooting and whether a resistance can be trusted.

| Working code | Working normalized message | Notes |
| --- | --- | --- |
| `DATA_CRC_ERROR` | Conversion data failed its CRC integrity check. | ADS124S08 supports this through driver comparison of the device-provided CRC; the driver must preserve that it is derived rather than a STATUS flag. |
| `SPI_CRC_ERROR` | The acquisition device reports an SPI CRC error. | AD7124 checks serial read/write CRC. Keep distinct from ADS124S08 conversion-data-only CRC and from a backend's generic transport exception. |
| `SPI_CLOCK_COUNT_ERROR` | The acquisition device reports an invalid SPI clock count. | AD7124 explicitly detects a serial transaction whose SCLK count is not a multiple of eight. |
| `SPI_READ_ERROR` | The acquisition device reports an invalid SPI read operation. | AD7124 native diagnostic; this is device-reported protocol misuse, not the same thing as a host transport read exception. |
| `SPI_WRITE_ERROR` | The acquisition device reports an invalid SPI write operation. | AD7124 native diagnostic. |
| `SPI_WRITE_IGNORED` | The acquisition device reports that a write was ignored while the device was busy. | AD7124 `SPI_IGNORE_ERR`; preserve the busy/ignored-write semantics instead of calling it a generic communication failure. |
| `REGISTER_INTEGRITY_ERROR` | The acquisition device reports a register-memory integrity error. | AD7124 memory-map CRC and similar diagnostics; exact native mechanism retained. |
| `ROM_INTEGRITY_ERROR` | The acquisition device reports a ROM integrity error. | AD7124 and industrial-device self-test peers where confirmed. |
| `NONVOLATILE_MEMORY_ERROR` | The acquisition device reports a nonvolatile-data or nonvolatile-memory fault. | ABB provides a software-visible nonvolatile-data defect; WIKA also detects EEPROM errors but exact HART exposure remains under review. Keep more-specific ROM/register codes where available. |
| `CONFIGURATION_ERROR` | The acquisition device reports an invalid or unsupported configuration. | Recurrent industrial/device-level concept. Driver-side API argument validation remains a separate programming/operation error. |
| `HARDWARE_FAULT` | The acquisition device reports an internal hardware fault that cannot be identified more specifically. | Broad fallback only for genuinely coarse native internal-device evidence. |

A device's **internal** ASIC/SPI/communication diagnostic must not be conflated
with `rtd-acquire` failing to communicate with the device through its backend
transport. The latter belongs to the still-to-be-finalized operation-error
contract unless the design later deliberately represents some transport
failures as `Measurement` diagnostics as well.

### Redundancy and internal-reference concepts under review

The industrial survey also exposes useful acquisition-level states that are
orthogonal to a simple sensor fault and should not be forced into one:

| Working code | Working normalized message | Evidence status / notes |
| --- | --- | --- |
| `BACKUP_SENSOR_ACTIVE` | The acquisition device reports that a backup sensor is currently being used. | Siemens `8F Backup enabled`; similar redundancy-active concepts occur in iTEMP and other transmitters, but exact trigger semantics differ. Likely `WARNING` when a trustworthy backup-derived resistance remains available. |
| `REDUNDANCY_UNAVAILABLE` | The acquisition device reports that the configured redundant sensor path is unavailable. | Siemens `8G Backup error`, ABB redundancy-unavailable states, and Yokogawa backup failures support a recurring degraded-redundancy concept. |
| `INTERNAL_REFERENCE_FAULT` | The acquisition device reports a fault or critical drift in its internal reference system. | Siemens `8H`/`8J` are intentionally kept distinct from external acquisition-reference problems such as ADS/AD7124 reference-input diagnostics. Final wording may split drift from hard failure if more devices justify it. |
| `INPUT_CHANNEL_DRIFT` | The acquisition device reports excessive internal drift in an acquisition input channel. | Siemens `8L`/`8n`; keep distinct from `SENSOR_DRIFT`, which generally compares redundant sensor measurements. |
| `GROUND_OFFSET_FAULT` | The acquisition device reports an excessive ground-offset condition in its acquisition front end. | Siemens `8o`; a specific one-family concept is preferable to laundering it into a vague sensor fault. |

These are deliberately provisional because they are industrially useful but are
not required by the first MAX31865/ADS124S08 implementation. Their presence in
the worksheet prevents later hardware from forcing unrelated native conditions
into an already-frozen broad bucket.

### Candidate maturity review

The broad survey is now stable enough to distinguish **mature semantic groups**
from names that still need targeted review. This is still not an API freeze and
no stable numeric IDs are assigned.

Strong candidates for the first enum because their semantics recur cleanly or
are directly required by the first drivers:

- `SENSOR_CIRCUIT_OPEN` and `SENSOR_CIRCUIT_SHORT`;
- broad sibling `SENSOR_INPUT_FAULT`;
- `SENSOR_DRIFT` and `LEAD_RESISTANCE_HIGH`;
- `RESISTANCE_HIGH_THRESHOLD` / `RESISTANCE_LOW_THRESHOLD`;
- `INPUT_OVERRANGE` / `INPUT_UNDERRANGE`;
- `REFERENCE_LOW` with broader sibling `REFERENCE_FAULT`;
- the four ADS124S08 PGA rail diagnoses;
- `ADC_SATURATION`, `CONVERSION_ERROR`, and `CALIBRATION_ERROR` where native
  hardware explicitly reports them;
- `DATA_CRC_ERROR`, `SPI_CRC_ERROR`, `REGISTER_INTEGRITY_ERROR`,
  `ROM_INTEGRITY_ERROR`, `CONFIGURATION_ERROR`, and broad `HARDWARE_FAULT`.

Still-provisional names/semantics that deserve another targeted pass before
freeze:

- `SENSOR_BURNOUT`: explicit and valid for KFD0-TR-1, but the term is used
  inconsistently elsewhere and must not become an alias for open circuit;
- `SENSOR_NOT_DETECTED`: currently a clean Siemens EM1.8U concept but not yet a
  broad cross-vendor group;
- `SENSOR_RANGE_HIGH` / `SENSOR_RANGE_LOW`: real transmitter-native semantics,
  but final naming must stay obviously distinct from `rtd-sensor` model-range
  interpretation;
- MAX31865-specific `REFERENCE_INPUT_ABOVE_THRESHOLD`,
  `REFERENCE_INPUT_BELOW_THRESHOLD`, `RTD_INPUT_BELOW_THRESHOLD`, and
  `INPUT_VOLTAGE_FAULT`: semantically grounded, but intentionally held open for
  naming review because few surveyed devices expose the same fault-detection
  architecture;
- device reset/restart events and active diagnostic-test outcomes, which may
  belong in a later event/test API instead of ordinary `Measurement`
  diagnostics.

This maturity split lets implementation proceed on well-supported concepts
without pretending the entire public vocabulary is frozen.

### State/facility observations currently excluded from the enum

These may be useful to drivers or separate diagnostic-test APIs, but the survey
does not currently justify ordinary `Measurement` diagnostic codes for them:

- ADS124S08 `RDY` during normal startup/reset;
- expected ADS124S08 `FL_POR` during initialization (an unexpected reset may
  later justify a reset/event diagnostic);
- ADS124S08 supply-monitor measurements without a separately defined threshold;
- ADS124S08 burnout-test full-scale/near-zero results without a careful active
  diagnostic-test API;
- ADS124S08 calibration commands merely existing/completing;
- MAX31865 fault-detection-cycle in-progress state.

## Research still required before `DiagnosticCode` freeze

One API-shape issue also remains intentionally open. Some normalized diagnoses
are justified by a **combination** of native states rather than one native
identifier—for example Beckhoff's documented `Overrange + Error` open-circuit
condition. The current singular `native_code`/`native_message` fields are easy
for normal one-to-one mappings but do not provide an obviously clean structured
representation for composite evidence. Before the `Diagnostic` contract is
frozen, decide whether to permit multiple native-evidence items, emit multiple
normalized diagnostics, or define another representation that does not invent a
fake combined vendor code.

The MAX31865 native fault/status map and ADS124S08 STATUS/facility map are now
complete enough for first-pass normalized-vocabulary design. Remaining research
is primarily about cross-vendor naming/semantic boundaries rather than missing
first-driver register bits.

High priority:

- obtain and catalog exact current WIKA T32 HART/DD diagnostic identifiers;
  WIKA confirms that HART exposes problem status and associated error messages,
  but the exact per-condition DD identifiers found so far are not publicly
  tabulated; the 4–20 mA alarm current alone is known to be broader than the
  internal monitoring;
- for Phoenix Contact MINI MCR-2-RTD-UI, decide which backend/configuration
  combinations we would actually support and therefore which of its distinct
  cable-break/short/range detections remain distinguishable to `rtd-acquire`;
- document Siemens Desigo PXC4 BACnet `Reliability` values actually emitted by
  RTD/resistance universal inputs;
- document how Honeywell Unitary controllers expose recognized sensor-break and
  short-circuit states to BACnet/Niagara clients, and whether raw/custom
  resistance can be consumed without the controller becoming the RTD-model
  authority;
- complete the current Siemens SITRANS TH320/420 diagnostic table beyond the
  now-recorded 8A–8J/8L/8n/8o/bF subset; the current research already shows that
  HART `8A`/`8B` collapse broken/shorted input faults while configured analog
  fault currents can distinguish them, so mappings must be backend- and
  configuration-aware;
- verify Beckhoff EL32xx behavior at the low-resistance/short-circuit boundary;
  the open-circuit, overrange, underrange, error, and data-validity paths are now
  documented, but no short-circuit inference should be added without explicit
  evidence;
- verify exact scope/meaning of iTEMP `Sensor broken` before deciding whether it
  can join `SENSOR_CIRCUIT_OPEN`;
- continue the now-broadened industrial survey (Rosemount, Endress+Hauser,
  Siemens, WIKA, ABB, Yokogawa, Phoenix Contact, Beckhoff) until the major
  semantic groups stop changing materially;
- continue surveying current HVAC/universal-input families beyond Siemens; the
  EM1.8U now supplies one fully documented raw-resistance + software-diagnostic
  reference path, while Honeywell/Siemens BACnet-specific mappings remain useful
  comparison targets.

Lower priority before 0.1 but useful before industrial releases:

- HART standardized device-status/NE107 mappings and how much native vendor
  detail remains accessible through common HART stacks;
- EtherCAT, Modbus, and BACnet transport-level diagnostics that should be
  represented separately from acquisition-device diagnostics;
- whether stable numeric diagnostic IDs should reserve ranges by semantic
  family or simply remain a flat public enum.

## Freeze criteria

The first `DiagnosticCode` vocabulary can be frozen when:

1. the first implementation device (MAX31865) is completely mapped;
2. at least one configurable precision ADC family is completely mapped;
3. multiple industrial transmitter/input families have been surveyed deeply
   enough to identify both specific semantic groups and broad outliers;
4. at least one HVAC/building universal-input family has documented status
   semantics;
5. every proposed normalized code has an evidence record explaining why that
   level of specificity is justified and, for implemented backends, the exact
   software-observable exposure path;
6. ambiguous native conditions have explicit broader mappings rather than
   inferred physical causes;
7. the candidate vocabulary has been reviewed for wording consistency,
   machine-readability, and Python/C portability.

The vocabulary remains extensible after release, but this gate is intended to
minimize avoidable renames or semantic changes in the early public API.
