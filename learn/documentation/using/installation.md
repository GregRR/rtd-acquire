# Installation

## Python requirements

`rtd-acquire 0.1.0a1` requires Python 3.11 or newer.

Because the first release is an alpha prerelease, include `--pre` when
installing from PyPI:

```sh
python -m pip install --pre rtd-acquire
```

The base package has no required runtime dependencies.

## Raspberry Pi / Linux SPI extra

The Linux SPI adapter uses the optional `spidev` package:

```sh
python -m pip install --pre "rtd-acquire[raspberry-pi]"
```

The extra installs the Python backend; it does not enable SPI in the operating
system or configure your wiring.

## Development checkout

The project uses `uv` for its development environment:

```sh
git clone https://github.com/GregRR/rtd-acquire.git
cd rtd-acquire
uv sync
```

Include the Raspberry Pi extra when developing against Linux SPI:

```sh
uv sync --extra raspberry-pi
```

## rtd-sensor is separate

`rtd-sensor` is intentionally **not** an `rtd-acquire` dependency. Install it
separately if your application also needs resistance-to-temperature conversion
or RTD-model interpretation:

```sh
python -m pip install rtd-sensor
```

This keeps acquisition software usable without choosing a specific RTD model.
