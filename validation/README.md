# Validation capture helpers

`validation/` contains repository tooling and templates for reproducible
physical-validation evidence. It is not part of the installed `rtd_acquire`
Python API.

Create a local record from the repository root:

```sh
uv run python -m validation.create_record <record-id>
```

For a Linux/Python MAX31865 capture, install the Raspberry Pi extra and write
structured measurements into that record:

```sh
uv sync --locked --extra raspberry-pi
uv run python -m validation.capture_max31865 \
  .rtd-acquire-local/validation/<record-id> \
  <capture-label> \
  --spi-path /dev/spidev0.0 \
  --reference-resistance-ohms 430 \
  --wire-count 4 \
  --filter-frequency-hz 60 \
  --count 20
```

Optional arguments also record non-default SPI clock, MAX31865 fault thresholds,
and the input-filter time constant used by the driver timing policy.

The helper writes:

- `<label>.measurements.jsonl` with every public `Measurement`, normalized
  diagnostic, and native-evidence item;
- `<label>.summary.json` with status counts and resistance summary statistics;
  and
- `<label>.capture.json` with the exact acquisition configuration and SHA-256
  digests of the capture and summary files.

The helper never decides whether a run passes its acceptance budget. That
judgment remains explicit in the validation record and follows
`docs/HARDWARE_VALIDATION.md`.
