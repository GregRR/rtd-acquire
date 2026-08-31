# rtd-acquire hardware and diagnostics catalog

This document catalogs candidate acquisition hardware, the acquisition
architectures they represent, and the diagnostic evidence they actually expose.
It is intentionally broader than the set of implemented drivers.

## Evidence rules

For every device/configuration, keep these claims distinct:

- **Manufacturer-supported** — explicitly documented by the vendor.
- **Electrically compatible** — engineering analysis says the acquisition chain
  can cover the required resistance/topology/configuration.
- **rtd-acquire validated** — tested by this project against defined criteria.

Do not infer a more specific fault than the hardware reports. Datasheet
troubleshooting causes are not automatically diagnostic facts.

Also distinguish **detected by the device** from **observable by the backend**. A
condition can be emitted as native `rtd-acquire` evidence only when the chosen
interface exposes it in a documented machine-readable or electrical form.

## Diagnostic normalization policy

The public `rtd-acquire` diagnostic vocabulary is derived from native device
semantics rather than vendor terminology alone.

For each native diagnostic, record:

- exact device/family and documentation revision;
- native code, bit, event number, or protocol value;
- native wording;
- what the condition actually establishes;
- vendor-listed possible causes, kept separate from observed facts;
- proposed normalized `rtd-acquire` code and message;
- other devices with genuinely equivalent semantics;
- whether the normalized concept can retain the native level of specificity;
- expected severity/measurement usability, if this can be determined without
  depending on application policy;
- source document and section/page.

Normalization rules:

1. Normalize together only conditions whose meanings genuinely overlap.
2. Preserve the highest useful specificity shared by a meaningful group of
   supported devices.
3. Do not make a common normalized diagnostic more general merely because one
   outlier device reports only a broader condition; give the outlier a broader
   code instead.
4. Do not make a broad native condition more specific than its evidence.
5. Preserve `native_code` and `native_message` so users can search the exact
   manufacturer terminology when troubleshooting.

The normalized vocabulary may therefore contain both a broad family concept and
more specific sibling concepts, for example `REFERENCE_FAULT` alongside
`REFERENCE_LOW` and `REFERENCE_HIGH`, when the surveyed hardware justifies those
distinctions. This is a conceptual hierarchy; the implementation does not need
enum inheritance.

The detailed native-code/message research and semantic grouping record lives in
[`DIAGNOSTICS.md`](DIAGNOSTICS.md). This file keeps the hardware catalog and
compatibility context; `DIAGNOSTICS.md` is the evidence ledger used to freeze the
public diagnostic vocabulary.

## Priority catalog

| Device/family | Architecture | Typical interface | RTD relevance | Project priority |
| --- | --- | --- | --- | --- |
| Analog Devices MAX31865 | Dedicated RTD resistance-to-digital converter | SPI | Pt100–Pt1000 documented; other resistance ranges require configuration/validation | First implementation |
| TI ADS124S08 | Precision configurable 24-bit ADC/front end | SPI | Flexible ratiometric RTD acquisition; strong candidate for Pt and Ni families | Second implementation |
| TI ADS1220 | Precision configurable 24-bit ADC/front end | SPI | PGA/reference/IDAC architecture supports flexible ratiometric RTD acquisition | Later precision-ADC candidate |
| Analog Devices AD7124-4/-8 | Precision configurable 24-bit ADC/front end | SPI | Flexible RTD/resistive-sensor acquisition | High-priority catalog target |
| Pepperl+Fuchs KFD0-TR-1 | Industrial Pt100 transmitter/converter | 4–20 mA | Pt100-specific; non-linearized mode is especially relevant to project boundary | Industrial analog target |
| Siemens Desigo PXC4 family | HVAC/building universal input/controller | BACnet / controller I/O | LG-Ni1000, Pt1000 and resistance-input modes | HVAC catalog/validation target |
| Siemens Desigo Essentials EM1.8U | HVAC/building universal I/O | Modbus RTU | Raw R1000/R10000 resistance plus Ni1000/Pt1000 modes | High-priority HVAC digital target |
| Sumtech PT-100-485/MB | Low-cost PT100 acquisition module | Modbus RTU / RS-485 | Separate PT100 resistance register at 0.1 Ω scaling | Prototyping-oriented Modbus research candidate |
| Honeywell Unitary family | HVAC/building universal-I/O controller | BACnet IP / MS/TP / T1L | Pt100/Pt1000/Ni1000 TK5000/Ni1000 DIN/custom resistive | HVAC parity/diagnostic target |
| Rosemount 644 family | Industrial temperature transmitter | 4–20 mA / HART | RTD transmitter with explicit device diagnostics | Industrial diagnostic reference/target |
| Endress+Hauser iTEMP TMT82 | Industrial temperature transmitter | 4–20 mA / HART | RTD transmitter with event-numbered sensor diagnostics | Industrial diagnostic reference/target |
| Siemens SITRANS TH320/TH420 | Universal industrial resistance/temperature transmitter | 4–20 mA / HART / PROFIBUS PA | Pt, Ni, Cu and linear-ohms modes; includes Ni120/Ni1000 | High-priority industrial resistance target |
| Beckhoff EL32xx | Industrial RTD/resistance input terminal | EtherCAT | Direct-ohms output; Pt100/Pt500/Pt1000/Ni120/Ni1000 | High-priority industrial digital target |
| Phoenix Contact MINI MCR-2-RTD-UI | Universal RTD/resistance transducer | analog current/voltage | Pt, Ni, Cu; 0–4000 Ω linear resistance | Industrial analog catalog target |
| WIKA T32.xS | Universal temperature transmitter | 4–20 mA / HART | Resistance sensors; break/lead/drift monitoring | Industrial diagnostic reference/target |
| ABB TTH300 | Universal temperature transmitter | 4–20 mA / HART / fieldbus variants | RTD/resistance; explicit wire-break, short, lead-resistance, drift and range diagnostics | High-value diagnostic/reference target |
| Yokogawa YTA610/YTA710 | Universal temperature transmitter | 4–20 mA / HART / FOUNDATION Fieldbus | RTD/ohms; model-specific sensor-failure/short/drift diagnostics | High-value diagnostic/reference target |

## Canonical current rtd-sensor parity targets

This table is the canonical `rtd-acquire` list of current `rtd-sensor` parity
targets. The project should maintain at least one validated acquisition path for
each listed built-in family and reconcile this table whenever `rtd-sensor` adds
or removes a supported family.

The resistance envelope is the ideal-element resistance implied by the current
`rtd-sensor` characteristic over that characteristic's complete supported
temperature range. It is an **acquisition requirement**, not temperature-model
logic for a driver. A candidate acquisition chain must be able to measure the
required resistance interval with the intended wiring/configuration before it can
be considered electrically compatible with the full characteristic.

| RTD model | Nominal R0 | Characteristic span | Required ideal resistance envelope | rtd-acquire obligation |
| --- | ---: | ---: | ---: | --- |
| Pt100 | 100 Ω | -200 to 850 °C | 18.52008 to 390.481125 Ω | Required |
| Pt500 | 500 Ω | -200 to 850 °C | 92.6004 to 1952.405625 Ω | Required |
| Pt1000 | 1000 Ω | -200 to 850 °C | 185.2008 to 3904.81125 Ω | Required |
| Ni120 6720 | 120 Ω | -80 to 260 °C | ~66.6000 to 380.3099 Ω | Required |
| Ni1000 6180 | 1000 Ω | -60 to 250 °C | 695.202595 to 2891.5625 Ω | Required |
| Ni1000 TK5000 | 1000 Ω | -60 to 250 °C | 751.79284 to 2517.265625 Ω | Required |

The platinum values are derived from the IEC 60751 PT-385 characteristic used by
`rtd-sensor`; Pt100, Pt500, and Pt1000 therefore share one normalized curve and
differ only by scale. The nickel values come from the distinct 6720, 6180, and
TK5000 characteristics used by `rtd-sensor`. Ni120 is shown rounded because its
published piecewise coefficients require the small, explicitly bounded continuity
adjustments documented by the companion project.

These bounds describe the mathematical characteristic, not every physical RTD
product sold under the same family name. A probe's packaging, construction,
tolerance class, lead arrangement, or rated operating range may be narrower.
Hardware validation must record the actual sensor and acquisition configuration
rather than treating this table as a product-rating claim.

The versioned machine-readable counterpart of these requirements is
`compatibility/v1/rtd_families.json`; the claim-state vocabulary is in
`compatibility/v1/evidence_model.json`. The JSON data does not add stronger
support claims than this catalog.

A device need not support all models. Project-wide coverage is the goal. A new
family first triggers a compatibility/validation review of existing acquisition
paths; it does not automatically require a new device driver.

## Direct-resistance sources

Some instruments, RTD interfaces, DAQs, and industrial inputs already expose a
defensible RTD-element resistance in ohms. Those systems can feed `rtd-sensor`
directly without an `rtd-acquire` backend.

Their existence still matters to this catalog because it defines the project
boundary and may reveal useful future backend classes. A wrapper should be
considered only when `rtd-acquire` adds independent value such as normalized
diagnostics, acquisition calibration/configuration, or common multi-channel
semantics. Direct interoperability by itself is not a driver requirement.

## Evaluated but outside the core contract

### Atlas Scientific EZO-RTD

The documented EZO-RTD interface returns an internally calculated temperature
in °C, K, or °F over UART or I²C. The current documented interface does not
establish a raw-resistance output suitable for the `rtd-acquire` resistance
contract.

Accordingly, EZO-RTD is not a current core backend target. `rtd-acquire` should
not reverse temperature back into resistance after the device has already
performed RTD interpretation. Reassess this decision only if a documented
resistance or sufficiently direct electrical-observation interface becomes
available.

Sources:

- https://atlas-scientific.com/embedded-solutions/ezo-rtd-temperature-circuit/
- https://files.atlas-scientific.com/EZO_RTD_Datasheet.pdf

## Host-platform adapters

### Raspberry Pi 4/5 through Linux `spidev`

The first Python host adapter is intentionally built on the Linux kernel
`spidev` userspace API rather than direct Raspberry Pi peripheral-register
access. Raspberry Pi documents SPI0 on the standard 40-pin header and supports
userspace SPI through `spidev`; the Python `spidev` package wraps that kernel
API and provides `open_path()` plus full-duplex `xfer2()` transactions.

For the standard SPI0 header mapping, MOSI is pin 19 (GPIO10), MISO is pin 21
(GPIO9), SCLK is pin 23 (GPIO11), CE0 is pin 24 (GPIO8), and CE1 is pin 26
(GPIO7). SPI0 is disabled by default on Raspberry Pi OS and must be enabled, for
example through `raspi-config` or `dtparam=spi=on`.

`rtd-acquire` does not depend on BCM2711- or RP1-specific register access. The
Linux adapter is therefore the same implementation for Raspberry Pi 4 and 5.
Support and validation remain separate claims:

- Raspberry Pi 4: implementation supported; physical validation pending on the
  project's available Pi 4 Model B.
- Raspberry Pi 5: implementation expected to use the same Linux interface;
  explicitly unvalidated until tested on physical Pi 5 hardware.
- Other Linux systems exposing a compatible `spidev` device: architecturally
  compatible, but not automatically claimed as project-validated platforms.

The adapter accepts a device path rather than assuming `/dev/spidev0.0`, which
also permits stable udev symlinks on systems where SPI bus numbering may vary.

### Arduino AVR / HERO through the portable C HALs

Introduced in `rtd-acquire 0.2.0`, the first embedded host adapter targets the
Arduino AVR / UNO R3-class core used by the inventr.io HERO. The adapter binds
Arduino `SPIClass`/`SPISettings`, caller-selected chip select, and blocking
Arduino delay facilities to the portable C SPI and delay HAL contracts; the
MAX31865 device logic itself remains platform-neutral C11.

The repository validates this adapter in two software layers: strict host C++11
tests using minimal Arduino/SPI stubs, and compilation of the real example for
`arduino:avr:uno` with the pinned Arduino AVR core. These checks establish API
and toolchain compatibility, not physical converter/RTD validation.

Support and validation remain separate claims:

- inventr.io HERO / Arduino AVR / UNO R3-class: adapter implemented and real
  Uno-core compilation validated; physical HERO + MAX31865 validation pending;
- non-AVR Arduino-compatible boards: not claimed as supported by this adapter;
  a separate platform adapter should be added when a concrete target requires
  one.

## Diagnostic capability survey

### Analog Devices MAX31865

The MAX31865 fault system is now exhaustively mapped for the first driver. Its
SPI Fault Status register exposes:

- D7 `RTD High Threshold` — conversion result at/above configured high threshold;
- D6 `RTD Low Threshold` — conversion result at/below configured low threshold;
- D5 `REFIN- > 0.85 × VBIAS`;
- D4 `REFIN- < 0.85 × VBIAS (FORCE- open)`;
- D3 `RTDIN- < 0.85 × VBIAS (FORCE- open)`;
- D2 combined `Overvoltage/undervoltage fault` on protected interface pins.

The RTD data register also contains a summary Fault bit, but the driver should
read the Fault Status register rather than emit a redundant generic diagnostic
when the specific bits are available. The fault-detection-cycle state is an
operation state rather than a diagnostic by itself.

The vendor describes open/short wiring failures as possible causes of fault-bit
patterns, but those decoding tables are troubleshooting guidance. Normalized
`rtd-acquire` diagnostics must stay aligned to the actual threshold/electrical
comparisons unless the native evidence uniquely establishes a stronger cause.

D7/D6 also show why vendor terminology does not set `rtd-acquire` severity: a
threshold crossing can still accompany a usable resistance, while D2 halts ADC
updates until the voltage fault clears.

#### Locked initial configuration contract

The first Python configuration requires the actual reference resistance,
physical wire count, and 50/60 Hz filter selection. Optional low/high fault
thresholds are expressed in ohms and translated by the driver to the native
ratiometric register format.

The contract deliberately does not contain a Pt/Ni model identity or
temperature range. The MAX31865 reports the 15-bit ratio `RRTD / RREF`; the
reference resistor therefore determines the electrical scaling, while
`rtd-sensor` remains responsible for interpreting the resulting resistance.

The datasheet's recommended DC operating range for `RREF` is 350 Ω to 10 kΩ.
The device supports 2-, 3-, and 4-wire connections; only 3-wire mode requires
the dedicated compensation bit. Its digital filter can reject either 50 Hz or
60 Hz mains frequency and harmonics.

BIAS, one-shot, fault-clear, and fault-detection-cycle bits are operational
commands rather than static `MAX31865Config` fields. Their sequencing and
settling requirements will be defined with the driver.

The serial interface transfers address/data bytes MSB first. CPHA must be 1,
while either CPOL=0 or CPOL=1 is supported. CS is active low and remains low
across a multi-byte transaction; the specified SCLK range extends to 5 MHz.
`rtd-acquire` therefore models one SPI transfer as a transaction that owns CS
rather than requiring the MAX31865 driver to manipulate a generic GPIO pin.

Sources:

- https://www.analog.com/en/products/max31865.html
- https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31865.pdf

### TI ADS124S08

The ADS124S08 STATUS register is now exhaustively classified for diagnostic
design:

- `FL_POR`: latched power-on-reset event, normally initialization state rather
  than an automatic measurement fault;
- `RDY`: device communication readiness, a transient state rather than a fault;
- four PGA rail flags identifying which PGA output is near which AVDD/AVSS rail;
- `FL_REF_L1`: external differential reference below one-third of the analog
  supply span;
- `FL_REF_L0`: external differential reference below 0.3 V.

Additional diagnostic facilities are not STATUS bits and must not be described
as though they were:

- optional conversion-data CRC lets the **driver** detect a mismatch; TI defines
  a mismatch as a data-transmission error;
- AVDD/DVDD monitors are active ADC measurements, not automatic supply-fault
  flags;
- burnout currents create an active diagnostic test whose full-scale/near-zero
  results remain ambiguous and must not be automatically labeled open/short;
- the SPI timeout feature recovers an interrupted serial transaction but exposes
  no native timeout flag;
- offset/gain calibration commands update calibration registers, but the
  datasheet does not expose an explicit calibration-failure status flag, so an
  ADS124S08 `CALIBRATION_ERROR` mapping is not currently justified.

This distinction between status flags, active tests, measured monitor channels,
and driver-derived checks is part of the public diagnostic design, not just a
driver implementation detail.

Sources:

- https://www.ti.com/product/ADS124S08
- https://www.ti.com/lit/ds/symlink/ads124s08.pdf

### TI ADS1220

The ADS1220 is a later precision-ADC candidate that covers many of the same
architectural concerns as the planned ADS124S08 while using a smaller four-input
24-bit delta-sigma ADC. TI documents an SPI interface, programmable gain,
internal and external references, two matched programmable excitation-current
sources, and 50/60 Hz rejection. TI's RTD material uses the device for
ratiometric 2-, 3-, and 4-wire RTD measurement configurations.

That overlap makes ADS1220 useful as a lower-cost representative of the
configurable precision-ADC family, but it does **not** displace ADS124S08 from
the current roadmap. ADS124S08 already carries deliberate diagnostic research
and remains the second planned implementation family. ADS1220 should be
implemented later only when availability, cost, validation value, or user need
justifies a second independent configurable-ADC path.

Sources:

- https://www.ti.com/product/ADS1220
- https://www.ti.com/lit/ds/symlink/ads1220.pdf

### Analog Devices AD7124-8 / AD7124 family

The AD7124-8 error register demonstrates the range of diagnostics a precision
front end may provide. Documented flags include:

- ADC saturation;
- ADC conversion error;
- ADC calibration error;
- positive input overvoltage/undervoltage;
- negative input overvoltage/undervoltage;
- reference-detection error;
- analog/digital supply monitor errors;
- SPI ignored/clock-count/read/write errors;
- SPI CRC error;
- memory-map CRC error;
- ROM CRC error;
- LDO decoupling-capacitor diagnostic.

Some flags deliberately combine possible physical causes; generic diagnostics
must preserve that ambiguity.

Source:

- https://www.analog.com/media/en/technical-documentation/data-sheets/ad7124-8.pdf

### Pepperl+Fuchs KFD0-TR-1

The KFD0-TR-1 is a Pt100-oriented industrial converter with a 4–20 mA output.
Its data sheet explicitly defines sensor burnout as an upscale fault output of
at least 22 mA (output limited to 35 mA).

Because the device itself defines the meaning, a diagnostic equivalent to
"converter reported sensor burnout" is legitimate for this backend.

Source:

- https://files.pepperl-fuchs.com/webcat/navi/productInfo/pds/038307_eng.pdf

### Siemens Desigo PXC4 universal inputs

Representative PXC4 universal inputs explicitly support combinations including
LG-Ni1000, Pt1000, raw resistance modes, 0–10 V, and 0/4–20 mA inputs. This is a
useful HVAC/building-automation architecture because Ni1000/Pt1000 acquisition
is a normal controller function rather than an unusual converter configuration.

The PXC.A BACnet object model exposes `reliability`, `status-flags`,
`event-state`, and an update count for I/O objects. Siemens documents
`reliability` as indicating whether a physical input's present value is reliable
and, if not, why. An update-count value of zero represents an inconsistent
startup state whose runtime values should not be considered reliable.

This establishes that useful acquisition validity/status evidence is available
over BACnet, but it does **not** yet establish which sensor-specific reliability
enumerations are produced for Ni1000/Pt1000/resistance-input faults. That mapping
remains a targeted research item before any PXC4 diagnostic code is frozen.

Sources:

- https://sid.siemens.com/r/A6V12957862/19744489483_21457179787__en-US_19935587083
- https://sid.siemens.com/r/A6V12954388/20185284747_28090979851__en-US_19516361099

### Rosemount 644

The Rosemount 644 is strong evidence that some industrial devices report
higher-level sensor diagnoses explicitly. Current documentation includes, among
other conditions:

- `Sensor Open` — open sensor detected;
- `Sensor Shorted` — shorted sensor detected;
- `Reference Error` — reference resistors greater than 25% of known value;
- `ASIC RCV Error` — checksum or start/stop failure in ASIC communication;
- `ASIC TX Error` — A/D ASIC communication error;
- `ASIC Configuration Error` — internal registers not written correctly and
  associated calibration error;
- sensor operating-range and operating-limit conditions.

Because `Sensor Open` and `Sensor Shorted` are explicit device diagnoses, the
normalized vocabulary may legitimately preserve that same level of specificity
if the broader survey confirms those concepts are useful across supported
hardware. They do not need a deliberately weakened `*_REPORTED` public code;
`native_code`/`native_message` already preserve provenance. The `rtd-acquire`
message must still be phrased as an acquisition diagnostic rather than claiming
independent physical proof beyond what the transmitter reports.

Source:

- https://www.emerson.com/is/content/emerson/en/measurement-instrumentation/technical/products/temperature/documents/doc-rosemount-00809-0400-4728.pdf

### Endress+Hauser iTEMP TMT82

The iTEMP TMT82 provides event-numbered diagnostics and is useful for comparing
industrial transmitter terminology with Rosemount. Its documented sensor events
include:

- `041 Sensor broken` — factory status `F`, alarm;
- `042 Sensor corroded` — factory status `M`, warning in the documented mode;
- `043 Short-circuit` — factory status `F`, alarm;
- `044 Sensor drift` — warning-capable;
- `045 Working area` — alarm.

`043 Short-circuit` is a promising semantic match for a normalized sensor-short
concept, but `041 Sensor broken` must **not** automatically be equated with
Rosemount `Sensor Open` until Endress+Hauser documentation establishes that
"broken" specifically means an open circuit rather than a broader sensor/wiring
failure. This is an example of why wording similarity alone is insufficient for
normalization.

Source:

- https://bdih-download.endress.com/file/e2ba15e25aed4e6cc6e369f66cca69ff/BA01028TEN_2624-00.pdf

### Siemens SITRANS TH320 / TH420

The SITRANS TH320/TH420 family is a strong industrial `rtd-acquire` target, not
just a diagnostic reference. Siemens documents universal resistance inputs, a
linear-ohms mode, and Pt/Ni/Cu sensor families including Pt500, Pt1000, Ni120,
and Ni1000. Structured diagnostics include broad input errors as well as more
specific drift, backup, and internal-reference conditions. Siemens also allows
broken- and shorted-sensor detection plus separately configurable analog fault
currents; importantly, the documented HART `Input ... error` diagnostic still
collapses broken/shorted cases. This makes the family a concrete example where
diagnostic specificity depends on the selected backend and configuration. See
`DIAGNOSTICS.md` for the native-code mapping record.

Sources:

- https://www.siemens.com/en-us/products/sitrans/th320-th420/
- https://cache.industry.siemens.com/dl/files/747/109802747/att_1082127/v1/A5E41864807-ADen_TH320420_TR320420_TF320420_HART_OI_en-US.pdf
- https://cache.industry.siemens.com/dl/files/072/109793072/att_1053491/v1/A5E41864869-ACen_THTRTF320420_TS500_FunctSafety.pdf
- https://cache.industry.siemens.com/dl/files/815/109764815/att_1268354/v1/FI01_us_kap02.pdf

### Beckhoff EL32xx RTD/resistance terminals

Beckhoff EL32xx EtherCAT terminals can expose measured resistance directly in
ohms and support several current `rtd-sensor` parity targets, including Pt100,
Pt500, Pt1000, Ni120, and Ni1000. The EtherCAT process image provides separate
underrange, overrange, error, limit, and TxPDO-validity status. Beckhoff
explicitly documents `Overrange + Error` as open-circuit detection, giving a
backend a software-observable path to a specific open-circuit diagnosis while
still keeping ordinary overrange separate. This family is therefore a
particularly good candidate for industrial digital acquisition without forcing
temperature conversion into the hardware layer.

Sources:

- https://www.beckhoff.com/en-us/products/i-o/ethercat-terminals/el-ed3xxx-analog-input/el3202-0010.html
- https://infosys.beckhoff.com/content/1033/el32xx/10195494283.html
- https://download.beckhoff.com/download/document/io/ethercat-terminals/el32xx_en.pdf

### Siemens Desigo Essentials EM1.8U

The EM1.8U is now a high-priority HVAC/building digital target. It provides
raw resistance modes over Modbus RTU (`R1000` and `R10000`) rather than forcing
Ni1000/Pt1000 temperature conversion, and it exposes a separate per-channel
reliability register.

For the `R1000` mode, Siemens documents a 700–1800 Ω measurement range with
0.6 Ω hardware resolution and Modbus scaling that exposes values in ohms. The
reliability registers separately report `No sensor`, `Under range`, `Short
circuit`, `Over range`, `Other error`, or `No error`. That combination makes the
module a particularly clean example of the `rtd-acquire` boundary: raw
electrical resistance plus machine-readable acquisition status.

Sources:

- https://sid.siemens.com/r/A6V14300949/25901888779___en-US_26440986635
- https://sid.siemens.com/r/A6V13841491/25014743435___en-US_25015603339
- https://sid.siemens.com/r/A6V13841491/25014743435___en-US_25015601803

### Sumtech PT-100-485/MB

The PT-100-485/MB is a low-cost/prototyping-oriented Modbus RTU research
candidate rather than a replacement for the better-documented professional
industrial targets. Its protocol documentation exposes temperature and PT100
resistance separately; the resistance register uses 0.1 Ω scaling, and a
separate writable resistance-correction register is documented.

This makes the module useful for evaluating a simple raw-resistance Modbus path
without forcing the device's calculated temperature into `rtd-sensor`. The
current research does **not** establish a sufficiently rich independent
fault/status model, so it should remain a catalog candidate rather than the
selected first Modbus backend until diagnostics, validity behavior,
documentation provenance, and physical hardware can be evaluated more fully.

Sources:

- https://www.sumtech.co.th/manual/PT-100MB%20PT100%20RS485%20sensor%20Instructions.pdf
- https://www.sumtech.co.th/manual/PT-100-485MB%20PT100%20RS485%20sensor%20protocol.pdf

### Phoenix Contact MINI MCR-2-RTD-UI

The MINI MCR-2-RTD-UI supports Pt, Ni, and Cu RTDs, 2-/3-/4-wire connections,
and linear resistance measurement from 0 to 4000 ohms with configurable analog
output. Its detailed documentation explicitly distinguishes cable break, short
circuit, overrange, underrange, and module errors internally.

Critically, the analog fault encoding is configurable. Some configurations give
cable break and short circuit different analog values, while others collapse
multiple errors into one upscale/downscale value. Therefore an analog backend's
diagnostic specificity is a property of **device + configuration + observation
interface**, not just of the transmitter model.

Sources:

- https://www.phoenixcontact.com/en-us/products/temperature-transmitter-mini-mcr-2-rtd-ui-2902049
- https://media.digikey.com/pdf/Data%20Sheets/Phoenix%20Contact%20PDFs/2902049_Ds.pdf

### WIKA T32.xS

The WIKA T32.xS documents sensor break, RTD sensor short circuit,
inadmissibly high lead resistance, sensor upper/lower limits, drift,
configuration errors, internal communication faults, and several internal
memory/control-flow failures. Its analog 4–20 mA alarm state can represent
multiple causes, so analog output alone does not preserve that specificity.
WIKA states that HART instruments set a problem status bit and allow the
associated error message to be read with the corresponding command; it also
publishes a HART DD/DTM. The exact per-condition HART/DD identifiers still need
to be captured before a HART backend freezes fine-grained native mappings.

Sources:

- https://www.wika.com/en-us/t32_xs.WIKA
- https://www.wika.com/media/Operating-instructions/Operating-instructions/Temperature/Temperature-transmitters/oi_t32_xs_en_de.pdf
- https://www.wika.com/media/Operating-instructions/Safety-manuals/oi_t32xs_safetymanual_v223_en_de_fr_es.pdf
- https://www.wika.com/en-eg/lp_temperature_transmitter.WIKA
- https://www.wika.com/en-us/document_categories_seo_html_36666.WIKA

### ABB TTH300

ABB's TTH300 is a useful industrial diagnostic reference because current HART
documentation explicitly distinguishes S1/S2 short-circuit, wire break/sensor
break, excessive line resistance, sensor drift, sensor-range over/under states,
configuration failure, nonvolatile-data failure, and other electronics states.
It therefore provides independent evidence for several specific normalized
concepts also seen in other manufacturers' hardware.

Source:

- https://library.e.abb.com/public/014c7e73e81c4fd59b46123b7304c2b3/OI_TTH300_EN_H01.pdf

### Yokogawa YTA610 / YTA710

The YTA610/YTA710 family supports RTDs and linear-ohms inputs and exposes HART
alarm/status information. The family includes sensor-failure, signal-range,
backup, and drift diagnostics. The YTA710 additionally documents explicit RTD/
ohms short-circuit detection with S1/S2 short alarms. This is a useful reminder
to catalog diagnostic capability at model/revision granularity rather than
assuming every model in a product family exposes the same evidence.

Sources:

- https://www.yokogawa.com/us/solutions/products-and-services/measurement/field-instruments-products/temperature-transmitters/field-mount/yta610-temperature-transmitter/
- https://www.manualslib.com/manual/1656571/Yokogawa-Yta610.html

### Honeywell Unitary controllers

Honeywell Unitary controllers are a valuable HVAC parity target because their
universal inputs explicitly include Pt100, Pt1000, Ni1000 TK5000, Ni1000 Class
B DIN 43760, and custom resistive characteristics from 100 ohms to 100 kOhms.
The documentation distinguishes sensor-break and short-circuit recognition for
Pt1000 and Ni1000 TK5000. The exact BACnet/Niagara exposure of those recognized
faults still needs research before a backend mapping is claimed.

Source:

- https://prod-edam.honeywell.com/content/dam/honeywell-edam/hbt/en-us/documents/manuals-and-guides/installation-guides/hbt-bms-unitarycontroller24V-installationinstructions.pdf

## Candidate normalized diagnostic concepts

These names are **not yet frozen API identifiers**. They are evidence-derived
concepts to refine after the initial catalog is broad enough to expose common
semantics and outliers.

The vocabulary should be allowed to contain both broad and specific concepts
when the hardware ecosystem supports that distinction.

### Measurement/input conditions

- resistance high threshold;
- resistance low threshold;
- input overvoltage;
- input undervoltage;
- positive/negative input overvoltage or undervoltage where the distinction is
  common and operationally useful;
- PGA/input near positive rail;
- PGA/input near negative rail;
- ADC saturation.

### Reference/acquisition-chain conditions

- reference fault (broad fallback);
- reference low;
- reference high;
- reference invalid/unavailable when that distinct meaning is supported;
- analog/digital supply fault;
- excitation-related fault where directly reported.

### Conversion/calibration

- conversion error;
- calibration error;
- configuration error.

### Communication/integrity

- communication/transport error;
- timeout/no data;
- CRC/checksum error;
- read error;
- write error;
- clock-count/protocol framing error;
- register/memory integrity error;
- ROM integrity error.

### Sensor diagnoses explicitly reported by hardware

- sensor open;
- sensor short;
- sensor burnout;
- sensor drift/degradation/corrosion only if the survey establishes sufficiently
  clear and useful semantics for normalization.

A broad `SENSOR_FAULT` concept may coexist with these specific codes for devices
that do not distinguish the physical condition.

## Preliminary native-to-normalized mapping observations

This table records research direction, **not frozen API mappings**.

| Device | Native condition | Evidence level | Normalization direction |
| --- | --- | --- | --- |
| MAX31865 | RTD High Threshold | Exact threshold comparison | Keep a specific resistance-high-threshold concept |
| MAX31865 | RTD Low Threshold | Exact threshold comparison | Keep a specific resistance-low-threshold concept |
| MAX31865 | `REFIN- > 0.85 × VBIAS` | Exact electrical comparison | Preserve threshold/reference specificity; do not infer wiring cause |
| MAX31865 | combined overvoltage/undervoltage fault | Broad combined electrical condition | Requires a broader voltage-fault concept unless more native detail is available elsewhere |
| ADS124S08 | `FL_REF_L0` | reference < 0.3 V | Strong candidate for specific reference-low concept |
| ADS124S08 | `FL_REF_L1` | reference < 1/3 analog supply | Strong candidate for specific reference-low concept, with native threshold retained |
| ADS124S08 | PGA rail flags | specific PGA output near named supply rail | Preserve rail/direction specificity if useful across surveyed ADCs |
| ADS124S08 | burnout-test full scale | ambiguous: open sensor, overload, or absent reference are possible | Do not normalize to sensor-open |
| AD7124 | `AINP_OV_ERR`, `AINP_UV_ERR`, `AINM_OV_ERR`, `AINM_UV_ERR` | explicit input-side and direction conditions | Survey peers before deciding whether to retain input-side distinction or normalize only over/undervoltage |
| AD7124 | `REF_DET_ERR` | external reference open or < 0.7 V | Broader reference-invalid/fault concept; do not claim open reference |
| KFD0-TR-1 | sensor burnout, output >= 22 mA | explicit vendor diagnosis | Sensor-burnout concept may retain native specificity |
| Rosemount 644 | `Sensor Open` | explicit open sensor diagnosis | Sensor-open concept may retain native specificity |
| Rosemount 644 | `Sensor Shorted` | explicit shorted sensor diagnosis | Sensor-short concept may retain native specificity |
| iTEMP TMT82 | `043 Short-circuit` | explicit short-circuit event | Likely sensor-short semantic group; confirm event scope/configuration |
| iTEMP TMT82 | `041 Sensor broken` | explicit but wording broader than "open" | Keep separate until documentation proves equivalence to sensor-open |
| Phoenix MINI MCR-2-RTD-UI | `Cable break` / `Short circuit` | explicit detections whose analog encodings depend on configuration | Use specific normalized codes only where the selected configuration makes them distinguishable |
| ABB TTH300 | S1/S2 short-circuit | explicit HART status | Strong sensor-short peer |
| ABB TTH300 | S1/S2 wire break / sensor break | explicit HART status | Candidate open/broken-circuit peer; retain native wording |
| ABB TTH300 | line resistance S1/S2 too high | explicit HART status | Strong lead-resistance-high peer |
| Yokogawa YTA710 | S1/S2 Short | explicit HART alarm from RTD/ohms short diagnostic | Strong sensor-short peer; model-specific |
| Siemens PXC4 | BACnet `reliability` / `status-flags` | generic point reliability/health evidence | Research exact resistance-input reliability values before mapping |

## Validation records to add later

For each tested device/configuration, record:

- exact hardware/module revision;
- RTD/resistance range and wiring topology;
- reference resistor/reference source;
- excitation, gain, ADC/reference configuration where relevant;
- Python/C implementation versions;
- low/mid/high known-resistance results;
- repeatability/noise observations;
- fault/status tests that can be performed safely;
- manufacturer-supported/electrically-compatible/validated classification;
- independent reference instrument or resistor uncertainty where available.
