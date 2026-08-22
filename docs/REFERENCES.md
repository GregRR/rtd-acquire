# References

This is the canonical bibliography for external manufacturer, industrial,
standards, scientific, validation, and interoperability sources used or retained
by `rtd-acquire`. Citations follow APA style as closely as the available
publication metadata permits.

The bibliography intentionally includes both sources that support implemented
behavior and sources retained for documented future hardware or diagnostic
research. Inclusion does not imply that every source defines a supported device
or released backend. Each entry therefore records its project role using one or
more of these labels:

- **Implementation basis** — directly supplies released device behavior,
  register semantics, electrical limits, timing, conversion rules, or another
  implemented acquisition rule.
- **Independent validation** — checks implemented behavior independently of the
  implementation source.
- **Corroborating/design source** — supports interpretation or an architecture
  or diagnostic-design choice without independently defining released numeric
  behavior.
- **Research/future** — retained for planned hardware, diagnostics, or
  interoperability work and not evidence for currently released support.
- **Documented discrepancy** — retained because a reputable source differs from
  another source or from the project's selected interpretation.
- **Design precedent** — informed conformance, provenance, interchange, or
  software-engineering design rather than device-specific acquisition behavior.

## Citation and provenance policy

When an external source materially supports register behavior, electrical or
protocol limits, timing, conversion/scaling, diagnostic semantics, calibration
or uncertainty treatment, validation data, acceptance criteria, or an
engineering design decision, the source must be added to or verified in this
file in the same change.

Source code should keep a short citation at the implementation point when that
materially improves traceability, for example:

```python
# Source: Analog Devices (2015), MAX31865 data sheet; see docs/REFERENCES.md.
```

Tests should distinguish independent validation from implementation provenance
when that distinction matters. The full citation belongs here rather than being
duplicated throughout source and test files.

A source listed as research/future does not become an implementation basis until
the relevant feature explicitly adopts it and records the applicable device,
revision, register/table/section, assumptions, and validation.

## Implemented converter and transport basis

Analog Devices. (2015). *MAX31865: RTD-to-digital converter data sheet*
(Rev. 3).
https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31865.pdf

**Project use:** Implementation basis for the first MAX31865 backend, including
the 15-bit RTD/reference resistance ratio, configuration and threshold
registers, 2-/3-/4-wire behavior, SPI mode and clock limits, fault-status bits,
automatic fault-detection cycle, input-settling guidance, and one-shot
conversion timing. It is also the implementation basis for the initial
MAX31865 conformance vectors and native diagnostic evidence.

## Precision ADC and converter research

Texas Instruments. (2017). *ADS124S0x low-power, low-noise, highly integrated,
6- and 12-channel, 4-kSPS, 24-bit, delta-sigma ADC with PGA and voltage
reference* (Rev. C) [Data sheet].
https://www.ti.com/lit/ds/symlink/ads124s08.pdf

**Project use:** Research/future and corroborating/design source for the planned
ADS124S08 acquisition family and for normalized diagnostic semantics involving
PGA rail monitoring, reference-low flags, CRC-protected conversion data,
burnout-test ambiguity, excitation sources, and system-monitor facilities.

Analog Devices. (2023). *AD7124-8: 8-channel, low noise, low power, 24-bit,
sigma-delta ADC with PGA and reference* (Rev. F) [Data sheet].
https://www.analog.com/media/en/technical-documentation/data-sheets/ad7124-8.pdf

**Project use:** Research/future and corroborating/design source for a precision
ADC backend and for the diagnostic vocabulary covering ADC saturation,
conversion/calibration errors, input over/undervoltage, reference and supply
faults, SPI errors, CRC/integrity faults, and LDO diagnostics.

Pepperl+Fuchs. (n.d.). *KFD0-TR-1 temperature converter* [Data sheet].
https://files.pepperl-fuchs.com/webcat/navi/productInfo/pds/038307_eng.pdf

**Project use:** Research/future source for a representative industrial Pt100
4–20 mA converter and for the explicitly documented sensor-burnout upscale
fault output.

## Industrial transmitters and universal I/O

Emerson. (2024). *Rosemount 644 temperature transmitter reference manual*
(Rev. CB; document 00809-0400-4728).
https://www.emerson.com/is/content/emerson/en/measurement-instrumentation/technical/products/temperature/documents/doc-rosemount-00809-0400-4728.pdf

**Project use:** Corroborating/design and research/future source for explicit
industrial transmitter diagnostics including sensor open, sensor short,
reference, internal communication, configuration, calibration, and sensor-range
conditions. It supports retaining specific normalized open/short concepts when
the backend actually exposes them.

Endress+Hauser. (2024). *iTEMP TMT82 operating instructions* (BA01028T).
https://bdih-download.endress.com/file/e2ba15e25aed4e6cc6e369f66cca69ff/BA01028TEN_2624-00.pdf

**Project use:** Corroborating/design and research/future source for numbered
industrial transmitter events including sensor broken, sensor corrosion,
short-circuit, sensor drift, and working-area conditions. The wording is also a
normalization counterexample: `Sensor broken` is not treated as equivalent to
an explicitly documented open circuit without stronger evidence.

Siemens. (n.d.). *SITRANS TH320/TH420, TR320/TR420, and TF320/TF420 HART
operating instructions* (A5E41864807).
https://cache.industry.siemens.com/dl/files/747/109802747/att_1082127/v1/A5E41864807-ADen_TH320420_TR320420_TF320420_HART_OI_en-US.pdf

**Project use:** Research/future source for universal Pt/Ni/Cu and linear-ohms
industrial acquisition, HART-visible input diagnostics, sensor drift, backup
behavior, and internal-reference diagnostics.

Siemens. (n.d.). *SITRANS TH/TR/TF320/420 and TS500 functional safety manual*
(A5E41864869).
https://cache.industry.siemens.com/dl/files/072/109793072/att_1053491/v1/A5E41864869-ACen_THTRTF320420_TS500_FunctSafety.pdf

**Project use:** Corroborating/design and research/future source for separately
configurable broken- and shorted-sensor detection and fault-current behavior,
including the design rule that diagnostic specificity depends on device,
configuration, and observation interface.

Beckhoff Automation. (n.d.). *EL32xx analog input terminals: RTD/resistance
measurement* [Documentation].
https://download.beckhoff.com/download/document/io/ethercat-terminals/el32xx_en.pdf

**Project use:** Research/future and corroborating/design source for EtherCAT
RTD/resistance acquisition, raw resistance output, underrange/overrange/error
status, and composite native evidence. Beckhoff's documented `Overrange +
Error` open-circuit condition motivated support for multiple native-evidence
items behind one normalized diagnostic.

Siemens. (n.d.). *Desigo Essentials EM1.8U I/O module Modbus documentation*.
https://sid.siemens.com/r/A6V14300949/25901888779___en-US_26440986635

**Project use:** Research/future source for HVAC/building acquisition that
exposes raw resistance over Modbus RTU together with per-channel reliability
states such as no sensor, under range, short circuit, over range, and other
error.

Phoenix Contact. (n.d.). *MINI MCR-2-RTD-UI temperature measuring transducer*
[Data sheet].
https://media.digikey.com/pdf/Data%20Sheets/Phoenix%20Contact%20PDFs/2902049_Ds.pdf

**Project use:** Research/future and corroborating/design source for Pt/Ni/Cu
and linear-resistance acquisition, 2-/3-/4-wire connections, and configurable
analog fault encoding for cable break, short circuit, overrange, underrange,
and module errors.

WIKA. (n.d.). *T32.xS temperature transmitter operating instructions*.
https://www.wika.com/media/Operating-instructions/Operating-instructions/Temperature/Temperature-transmitters/oi_t32_xs_en_de.pdf

**Project use:** Research/future and corroborating/design source for sensor
break/short, excessive lead resistance, limits, drift, configuration, internal
communication, and memory/control-flow faults. It also supports the rule that a
coarse 4–20 mA alarm state must not be normalized as a more specific internal
fault unless the selected interface exposes that evidence.

ABB. (n.d.). *TTH300 head-mount temperature transmitter operating
instructions*.
https://library.e.abb.com/public/014c7e73e81c4fd59b46123b7304c2b3/OI_TTH300_EN_H01.pdf

**Project use:** Corroborating/design and research/future source independently
supporting specific sensor short, wire/sensor break, excessive lead resistance,
sensor drift, sensor-range, configuration, nonvolatile-memory, and internal
electronics diagnostic concepts.

Yokogawa Electric Corporation. (n.d.). *YTA610/YTA710 temperature transmitter
product and diagnostic documentation*.
https://www.yokogawa.com/us/solutions/products-and-services/measurement/field-instruments-products/temperature-transmitters/field-mount/yta610-temperature-transmitter/

**Project use:** Research/future and corroborating/design source for HART-visible
sensor failure, signal-range, backup, drift, and model-dependent short-circuit
diagnostics. It supports recording capability at model/revision granularity
rather than assuming every device in a product family exposes identical faults.

Honeywell. (n.d.). *Unitary controller 24 V installation instructions*.
https://prod-edam.honeywell.com/content/dam/honeywell-edam/hbt/en-us/documents/manuals-and-guides/installation-guides/hbt-bms-unitarycontroller24V-installationinstructions.pdf

**Project use:** Research/future source for HVAC universal inputs supporting
Pt100, Pt1000, Ni1000 TK5000, former-DIN Ni1000, and custom resistance curves,
and for documented sensor-break and short-circuit recognition. Exact
BACnet/Niagara exposure remains a research item before a backend mapping is
claimed.

Siemens. (n.d.). *Desigo PXC4/PXC.A universal-input and BACnet object
documentation*.
https://sid.siemens.com/r/A6V12954388/20185284747_28090979851__en-US_19516361099

**Project use:** Research/future source for Ni1000, Pt1000, and raw resistance
universal-input modes and for BACnet reliability/status exposure. The current
research establishes a software-visible validity path but does not yet establish
which sensor-specific reliability states are exposed for each resistance mode.

## Data interchange and conformance design sources

Bray, T. (2017). *The JavaScript Object Notation (JSON) data interchange
format* (RFC 8259). Internet Engineering Task Force.
https://doi.org/10.17487/RFC8259

**Project use:** Design precedent for the language-neutral JSON conformance
vectors. Device-specific numeric and diagnostic semantics remain grounded in
the applicable manufacturer references above.

## Provenance interpretation

Generated conformance vectors are not independent engineering validation of the
manufacturer behavior used to create them. The project uses distinct evidence
layers: manufacturer/industry sources define or corroborate device behavior;
deterministic vectors specify the normalized cross-language contract; and later
physical reference-resistance and RTD testing will independently validate real
hardware acquisition behavior.

When reputable sources disagree, the disagreement should be retained and
resolved explicitly rather than averaged or silently reconciled. A source in a
research/future section does not become evidence for supported hardware until
the relevant implementation and validation work adopt it.
