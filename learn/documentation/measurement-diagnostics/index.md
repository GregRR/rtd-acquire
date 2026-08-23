# Measurement & diagnostics

A resistance number alone is often not enough. `rtd-acquire` also preserves the
information needed to judge whether that number is usable and why a device
reported a problem.

The public result model combines:

- a trustworthy resistance when one is available;
- a derived `ok` / `warning` / `fault` status;
- stable normalized diagnostic codes;
- original device/protocol evidence; and
- optional standard uncertainty in resistance.

Continue with:

- [Diagnostics](diagnostics.md)
- [Native evidence](native-evidence.md)
- [Resistance uncertainty](uncertainty.md)
