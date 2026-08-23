"""Deterministic generic acquisition-device simulation."""

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass

from ..core import AcquisitionError, ConfigurationError, Measurement


@dataclass(frozen=True, slots=True)
class SimulatedAcquisitionFailure:
    """One scripted failure of the acquisition operation itself."""

    message: str = "simulated acquisition failure"

    def __post_init__(self) -> None:
        if not isinstance(self.message, str) or not self.message.strip():
            raise ConfigurationError(
                "simulated acquisition failure message must be a non-empty string"
            )


class SimulatedAcquisitionDevice:
    """Replay a deterministic script through the ``AcquisitionDevice`` contract.

    Script entries are either already-validated ``Measurement`` objects or
    ``SimulatedAcquisitionFailure`` records. A failure raises the public
    ``AcquisitionError`` boundary and then advances to the next script entry.

    By default, reading past the end raises ``AcquisitionError`` so accidental
    over-reading is visible. ``repeat=True`` loops the entire script instead.
    This class models acquisition results and operation failures, not a specific
    converter, sensor temperature, or physical process.
    """

    def __init__(
        self,
        steps: Iterable[Measurement | SimulatedAcquisitionFailure],
        *,
        repeat: bool = False,
    ) -> None:
        script = tuple(steps)
        if not script:
            raise ConfigurationError("simulated acquisition script must not be empty")
        if not all(
            isinstance(step, (Measurement, SimulatedAcquisitionFailure))
            for step in script
        ):
            raise TypeError(
                "simulated acquisition steps must be Measurement or "
                "SimulatedAcquisitionFailure objects"
            )
        if not isinstance(repeat, bool):
            raise ConfigurationError("repeat must be a bool")

        self._steps = script
        self._repeat = repeat
        self._index = 0
        self._read_count = 0

    @property
    def read_count(self) -> int:
        """Number of scripted entries consumed since construction or reset."""

        return self._read_count

    def reset(self) -> None:
        """Restart the script and reset the consumed-entry counter."""

        self._index = 0
        self._read_count = 0

    def read(self) -> Measurement:
        """Return or raise the next deterministic scripted acquisition result."""

        if self._index >= len(self._steps):
            if not self._repeat:
                raise AcquisitionError("simulated acquisition script exhausted")
            self._index = 0

        step = self._steps[self._index]
        self._index += 1
        self._read_count += 1

        if isinstance(step, SimulatedAcquisitionFailure):
            raise AcquisitionError(step.message)
        return step
