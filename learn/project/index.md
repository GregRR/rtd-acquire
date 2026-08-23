---
title: Project
---

# Project

`rtd-acquire` is an open-source hardware-agnostic RTD resistance acquisition
library licensed under MPL-2.0.

## Current release

`0.1.0a1` is the first public alpha. It establishes the initial Python
measurement and diagnostic contracts, MAX31865 acquisition path, Linux SPI
adapter, simulation/emulation support, conformance foundation, and portable C
SPI HAL foundation.

Physical MAX31865 validation is still pending, and public APIs may change before
`0.1.0`.

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
