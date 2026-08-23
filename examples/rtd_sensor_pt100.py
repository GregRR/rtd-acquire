"""Pass an acquired resistance to the separate rtd-sensor package.

Run from a source checkout with rtd-sensor installed separately, for example:

    uv run --with rtd-sensor python examples/rtd_sensor_pt100.py

The simulated device keeps this example runnable without acquisition hardware.
Replace it with any real AcquisitionDevice, including MAX31865, without changing
the rtd-sensor handoff.
"""

from rtd_acquire import AcquisitionDevice, Measurement
from rtd_acquire.simulation import SimulatedAcquisitionDevice
from rtd_sensor import pt100


def read_pt100_celsius(device: AcquisitionDevice) -> float:
    """Acquire one trustworthy resistance and interpret it as a Pt100."""

    measurement = device.read()
    if measurement.resistance_ohms is None:
        details = ", ".join(
            diagnostic.message for diagnostic in measurement.diagnostics
        )
        raise RuntimeError(f"acquisition fault: {details}")

    return pt100.resistance_to_celsius(measurement.resistance_ohms)


def main() -> None:
    """Run the package-boundary example with a deterministic resistance."""

    device = SimulatedAcquisitionDevice([Measurement(resistance_ohms=109.7346)])
    temperature_c = read_pt100_celsius(device)
    print(f"Pt100 temperature: {temperature_c:.3f} °C")


if __name__ == "__main__":
    main()
