---
title: Project
---

# Project

`rtd-acquire` is an open-source hardware-agnostic RTD resistance acquisition
library licensed under MPL-2.0.

## Current release

`0.2.0` adds the independent portable C11 MAX31865 implementation,
caller-owned C result contracts, Python/C conformance with an explicit binary32
numeric profile, and the Arduino AVR / HERO adapter to the Python acquisition
stack introduced in `0.1.0a1`.

Physical Raspberry Pi/MAX31865 and HERO/MAX31865 validation remains pending.
The project remains pre-1.0, and public APIs may change before `1.0`.

## Repository

- [GitHub repository](https://github.com/GregRR/rtd-acquire)
- [Issues](https://github.com/GregRR/rtd-acquire/issues)
- [Design](https://github.com/GregRR/rtd-acquire/blob/main/docs/DESIGN.md)
- [Roadmap](https://github.com/GregRR/rtd-acquire/blob/main/docs/ROADMAP.md)
- [Hardware catalog](https://github.com/GregRR/rtd-acquire/blob/main/docs/HARDWARE.md)
- [Diagnostic research](https://github.com/GregRR/rtd-acquire/blob/main/docs/DIAGNOSTICS.md)
- [Technical references](https://github.com/GregRR/rtd-acquire/blob/main/docs/REFERENCES.md)
- [Hardware validation](https://github.com/GregRR/rtd-acquire/blob/main/docs/HARDWARE_VALIDATION.md)
- [Changelog](https://github.com/GregRR/rtd-acquire/blob/main/docs/CHANGELOG.md)

## Companion project

[`rtd-sensor`](https://gregrr.github.io/rtd-sensor/) provides the RTD-model and
temperature-interpretation layer that begins where `rtd-acquire` stops.

[Visit the RTD Playground](https://gregrr.github.io/rtd-sensor/playground/){ .md-button .md-button--primary }
